%{
/// @file parser.y

/*************************************************************************

                   Compiler for the language LANCE

***************************************************************************/

#include <stdio.h>       
#include <stdlib.h>
#include <assert.h>
#include "program.h"
#include "target_asm_print.h"
#include "target_transform.h"
#include "target_info.h"
#include "errors.h"
#include "list.h"
#include "expressions.h"
#include "gencode.h"
#include "utils.h"
#include "variables.h"
#include "cflow_graph.h"
#include "reg_alloc.h"
#include "options.h"

/* Global Variables */

/* This variable will keep track of the source code line number.
 *   Every time a newline is encountered while scanning the input file, the
 * lexer increases this variable by 1.
 *   This way, when the parser returns an error or a warning, it will look at
 * this variable to report where the error happened. */
int line_num;

/* The number of errors and warnings found in the code. */
int num_error;
int num_warning;     
                     
/* The singleton instance of `program'.
 *   An instance of `t_program' holds in its internal structure all the
 * fundamental informations about the program being compiled:
 *   - the list of instructions
 *   - the list of data directives (static allocations)
 *   - the list of variables
 *   - the list of labels
 *   - ... */
t_program *program;

/* This global variable is the file read by the Flex-generated scanner */
extern FILE *yyin;


/* Forward Declarations */

/* yylex() is the function generated by Flex that returns a the next token
 * scanned from the input. */
extern int yylex(void);

/* yyerror() is a function defined later in this file used by the bison-
 * generated parser to notify that a syntax error occurred. */
extern void yyerror(const char*);

%}

/* EXPECT DECLARATION: notifies bison that the grammar contains the specified
 * number of shift-reduce conflicts. If the number of conflicts matches what is
 * specified, bison will automatically resolve them. */
%expect 1

/* AXIOM DECLARATION. The starting non-terminal of the grammar will be
 * `program'. */
%start program

/******************************************************************************
 * UNION DECLARATION
 *
 * Specifies the set of types available for the semantic values of terminal
 * and non-terminal symbols.
 ******************************************************************************/
%union {
   int integer;
   char *string;
   t_expressionValue expr;
   t_declaration *decl;
   t_listNode *list;
   t_label *label;
   t_whileStatement while_stmt;
}

/******************************************************************************
 * TOKEN SYMBOL DECLARATIONS
 *
 * Here we declare all the token symbols that can be produced by the scanner.
 * Bison will automatically produce a #define that assigns a number (or token
 * identifier) to each one of these tokens.
 *   We also declare the type for the semantic values of some of these tokens.
 ******************************************************************************/

%token EOF_TOK /* end of file */
%token LBRACE RBRACE LPAR RPAR LSQUARE RSQUARE
%token SEMI PLUS MINUS MUL_OP DIV_OP
%token AND_OP OR_OP NOT_OP
%token ASSIGN LT GT SHL_OP SHR_OP EQ NOTEQ LTEQ GTEQ
%token ANDAND OROR
%token COMMA
%token RETURN
%token READ
%token WRITE

/* These are the tokens with a semantic value of the given type. */
%token <label> DO
%token <while_stmt> WHILE
%token <label> IF
%token <label> ELSE
%token <integer> TYPE
%token <string> IDENTIFIER
%token <integer> NUMBER

/******************************************************************************
 * NON-TERMINAL SYMBOL SEMANTIC VALUE TYPE DECLARATIONS
 *
 * Here we declare the type of the semantic values of non-terminal symbols.
 *   We only declare the type of non-terminal symbols of which we actually use
 * their semantic value.
 ******************************************************************************/

%type <expr> exp
%type <decl> declaration
%type <list> declaration_list
%type <label> if_stmt

/******************************************************************************
 * OPERATOR PRECEDENCE AND ASSOCIATIVITY
 *
 * Precedence is given by the declaration order. Associativity is given by the
 * specific keyword used (%left, %right).
 ******************************************************************************/

%left COMMA
%left ASSIGN
%left OROR
%left ANDAND
%left OR_OP
%left AND_OP
%left EQ NOTEQ
%left LT GT LTEQ GTEQ
%left SHL_OP SHR_OP
%left MINUS PLUS
%left MUL_OP DIV_OP
%right NOT_OP

/******************************************************************************
 * GRAMMAR AND SEMANTIC ACTIONS
 *
 * The grammar of the language follows. The semantic actions are the pieces of
 * C code enclosed in {} brackets: they are executed when the rule has been
 * parsed and recognized up to the point where the semantic action appears.
 ******************************************************************************/
%% 

/* `program' is the starting non-terminal of the grammar.
 * A program is composed by:
      1. declarations (zero or more);
      2. A list of instructions. (at least one instruction!).
 * When the rule associated with the non-terminal `program' is executed,
 * the parser notifies it to the `program' singleton instance. */
program  : var_declarations statements EOF_TOK
         {
            /* Notify the end of the program. Once called
             * the function `setProgramEnd' - if necessary -
             * introduces a `HALT' instruction into the
             * list of instructions. */
            setProgramEnd(program);

            /* return from yyparse() */
            YYACCEPT;
         }
;

var_declarations  : var_declarations var_declaration   { /* does nothing */ }
                  | /* empty */                        { /* does nothing */ }
;

var_declaration   : TYPE declaration_list SEMI
                  {
                     /* update the program infos by adding new variables */
                     addVariablesFromDecls(program, $1, $2);
                  }
;

declaration_list  : declaration_list COMMA declaration
                  {  /* add the new declaration to the list of declarations */
                     $$ = addElement($1, $3, -1);
                  }
                  | declaration
                  {
                     /* add the new declaration to the list of declarations */
                     $$ = addElement(NULL, $1, -1);
                  }
;

declaration : IDENTIFIER
            {
               /* create a new instance of t_declaration */
               $$ = newDeclaration($1, 0, 0);
               free($1);
            }
            | IDENTIFIER LSQUARE NUMBER RSQUARE
            {
               /* create a new instance of t_declaration */
               $$ = newDeclaration($1, 1, $3);
               free($1);
            }
;

/* A block of code can be either a single statement or
 * a set of statements enclosed between braces */
code_block  : statement                   { /* does nothing */ }
            | LBRACE statements RBRACE    { /* does nothing */ }
;

/* One or more code statements */
statements  : statements statement        { /* does nothing */ }
            | statement                   { /* does nothing */ }
;

/* A statement can be either an assignment statement or a control statement
 * or a read/write statement or a semicolon */
statement   : assign_statement SEMI       { /* does nothing */ }
            | control_statement           { /* does nothing */ }
            | read_write_statement SEMI   { /* does nothing */ }
            | SEMI                        { genNOPInstruction(program); }
;

control_statement : if_statement                { /* does nothing */ }
                  | while_statement             { /* does nothing */ }
                  | do_while_statement SEMI     { /* does nothing */ }
                  | return_statement SEMI       { /* does nothing */ }
;

read_write_statement : read_statement           { /* does nothing */ }
                     | write_statement          { /* does nothing */ }
;

assign_statement : IDENTIFIER LSQUARE exp RSQUARE ASSIGN exp
            {
               /* Notify to `program' that the value $6
                * have to be assigned to the location
                * addressed by $1[$3]. Where $1 is obviously
                * the array/pointer identifier, $3 is an expression
                * that holds an integer value. That value will be
                * used as an index for the array $1 */
               genStoreArrayElement(program, $1, $3, $6);

               /* free the memory associated with the IDENTIFIER.
                * The use of the free instruction is required
                * because of the value associated with IDENTIFIER.
                * The value of IDENTIFIER is a string created
                * by a call to the function `strdup' (see Acse.lex) */
               free($1);
            }
            | IDENTIFIER ASSIGN exp
            {
               /* in order to assign a value to a variable, we have to
                * know where the variable is located (i.e. in which register).
                * the function `getRegLocationOfScalar' is used in order
                * to retrieve the register location assigned to
                * a given identifier.
                * `getRegLocationOfScalar' perform a query on the list
                * of variables in order to discover the correct location of
                * the variable with $1 as identifier */
               
               /* get the location of the variable with the given ID. */
               int location = getRegLocationOfScalar(program, $1);

               /* update the value of location */
               if ($3.type == CONSTANT)
                  genMoveImmediate(program, location, $3.immediate);
               else
                  genADDIInstruction(program, location, $3.registerId, 0);

               /* free the memory associated with the IDENTIFIER */
               free($1);
            }
;
            
if_statement   : if_stmt
               {
                  /* fix the `label_else' */
                  assignLabel(program, $1);
               }
               | if_stmt ELSE
               {
                  /* reserve a new label that points to the address where to
                   * jump if `exp' is verified */
                  $2 = createLabel(program);
   
                  /* exit from the if-else */
                  genJInstruction(program, $2);
   
                  /* fix the `label_else' */
                  assignLabel(program, $1);
               }
               code_block
               {
                  /* fix the `label_else' */
                  assignLabel(program, $2);
               }
;
            
if_stmt  : IF
         {
            /* the label that points to the address where to jump if
             * `exp' is not verified */
            $1 = createLabel(program);
         }
         LPAR exp RPAR
         {
            if ($4.type == CONSTANT) {
               if ($4.immediate == 0)
                  genJInstruction(program, $1);
            } else {
               /* if `exp' returns FALSE, jump to the label $1 */
               genBEQInstruction(program, $4.registerId, REG_0, $1);
            }
         }
         code_block
         {
            $$ = $1;
         }
;

while_statement   : WHILE
                  {
                     /* reserve and fix a new label */
                     $1.label_condition = assignNewLabel(program);
                  }
                  LPAR exp RPAR
                  {
                     /* reserve a new label. This new label will point
                      * to the first instruction after the while code
                      * block */
                     $1.label_end = createLabel(program);

                     if ($4.type == CONSTANT) {
                        if ($4.immediate == 0)
                           genJInstruction(program, $1.label_end);
                     } else {
                        /* if `exp' returns FALSE, jump to the label 
                         * $1.label_end */
                        genBEQInstruction(program, $4.registerId, REG_0, 
                                          $1.label_end);
                     }
                  }
                  code_block
                  {
                     /* jump to the beginning of the loop */
                     genJInstruction(program, $1.label_condition);

                     /* fix the label `label_end' */
                     assignLabel(program, $1.label_end);
                  }
;
                  
do_while_statement   : DO
                     {
                        /* the label that points to the address where to jump if
                         * `exp' is not verified */
                        $1 = createLabel(program);
                        
                        /* fix the label */
                        assignLabel(program, $1);
                     }
                     code_block WHILE LPAR exp RPAR
                     {
                        if ($6.type == CONSTANT) {
                           if ($6.immediate != 0)
                              genJInstruction(program, $1);
                        } else {
                           /* if `exp' returns TRUE, jump to the label $1 */
                           genBNEInstruction(program, $6.registerId, REG_0, $1);
                        }
                     }
;

return_statement  : RETURN
                  {
                     /* insert an HALT instruction */
                     genExit0Syscall(program);
                  }
;

read_statement : READ LPAR IDENTIFIER RPAR 
            {
               /* read from standard input an integer value and assign
                * it to a variable associated with the given identifier */
               /* get the location of the variable with the given ID */
               
               /* lookup the variable table and fetch the register location
                * associated with the IDENTIFIER $3. */
               int location = getRegLocationOfScalar(program, $3);

               /* insert a read instruction */
               genReadIntSyscall(program, location);

               /* free the memory associated with the IDENTIFIER */
               free($3);
            }
;
            
write_statement : WRITE LPAR exp RPAR 
            {
               int location;
               if ($3.type == CONSTANT) {
                  /* load `immediate' into a new register. Returns the new
                   * register identifier or REG_INVALID if an error occurs */
                  location = genLoadImmediate(program, $3.immediate);
               } else
                  location = $3.registerId;

               /* write to standard output an integer value */
               genPrintIntSyscall(program, location);

               /* write a newline to standard output */
               location = genLoadImmediate(program, '\n');
               genPrintCharSyscall(program, location);
            }
;

exp: NUMBER
   { 
      $$ = getConstantExprValue($1);
   }
   | IDENTIFIER 
   {
      /* get the location of the symbol with the given ID */
      int variableReg = getRegLocationOfScalar(program, $1);
      
      /* return that register as the expression value */
      $$ = getRegisterExprValue(variableReg);

      /* free the memory associated with the IDENTIFIER */
      free($1);
   }
   | IDENTIFIER LSQUARE exp RSQUARE
   {
      /* load the value IDENTIFIER[exp]
         * into `arrayElement' */
      int reg = genLoadArrayElement(program, $1, $3);

      /* create a new expression */
      $$ = getRegisterExprValue(reg);

      /* free the memory associated with the IDENTIFIER */
      free($1);
   }
   | NOT_OP exp
   {
      if ($2.type == CONSTANT) {
         /* CONSTANT (constant) expression: compute the value at
            * compile-time and place the result in a new CONSTANT
            * expression */
         $$ = getConstantExprValue(!($2.immediate));
      } else {
         /* REGISTER expression: generate the code that will compute
          * the result at compile time */

         /* Reserve a new register for the result */
         int res_reg = getNewRegister(program);

         /* Generate a SUBI instruction which will store the negated
            * logic value into the register we reserved */
         genSEQInstruction(program, res_reg, $2.registerId, REG_0);
         
         /* Return a REGISTER expression with the result register */
         $$ = getRegisterExprValue(res_reg);
      }
   }
   | MINUS exp
   {
      if ($2.type == CONSTANT) {
         $$ = getConstantExprValue(-($2.immediate));
      } else {
         /* create an expression for register REG_0 */
         t_expressionValue exp_r0 = getRegisterExprValue(REG_0);
         $$ = handleBinaryOperator(program, exp_r0, $2, OP_SUB);
      }
   }
   | exp AND_OP exp { $$ = handleBinaryOperator(program, $1, $3, OP_ANDB); }
   | exp OR_OP exp  { $$ = handleBinaryOperator(program, $1, $3, OP_ORB); }
   | exp PLUS exp   { $$ = handleBinaryOperator(program, $1, $3, OP_ADD); }
   | exp MINUS exp  { $$ = handleBinaryOperator(program, $1, $3, OP_SUB); }
   | exp MUL_OP exp { $$ = handleBinaryOperator(program, $1, $3, OP_MUL); }
   | exp DIV_OP exp { $$ = handleBinaryOperator(program, $1, $3, OP_DIV); }
   | exp LT exp     { $$ = handleBinaryOperator(program, $1, $3, OP_LT); }
   | exp GT exp     { $$ = handleBinaryOperator(program, $1, $3, OP_GT); }
   | exp EQ exp     { $$ = handleBinaryOperator(program, $1, $3, OP_EQ); }
   | exp NOTEQ exp  { $$ = handleBinaryOperator(program, $1, $3, OP_NOTEQ); }
   | exp LTEQ exp   { $$ = handleBinaryOperator(program, $1, $3, OP_LTEQ); }
   | exp GTEQ exp   { $$ = handleBinaryOperator(program, $1, $3, OP_GTEQ); }
   | exp SHL_OP exp { $$ = handleBinaryOperator(program, $1, $3, OP_SHL); }
   | exp SHR_OP exp { $$ = handleBinaryOperator(program, $1, $3, OP_SHR); }
   | exp ANDAND exp { $$ = handleBinaryOperator(program, $1, $3, OP_ANDL); }
   | exp OROR exp   { $$ = handleBinaryOperator(program, $1, $3, OP_ORL); }
   | LPAR exp RPAR  { $$ = $2; }
;

%%
/******************************************************************************
 * Parser wrapper function
 ******************************************************************************/

int parseProgram(t_program *program)
{
   /* Initialize all the global variables */
   line_num = 1;
   num_error = 0;
   num_warning = 0;

   /* Open the input file */
   if (compilerOptions.inputFileName != NULL) {
      yyin = fopen(compilerOptions.inputFileName, "r");
      if (yyin == NULL)
         fatalError(ERROR_FOPEN_ERROR);
      debugPrintf(" -> Reading input from \"%s\"\n", compilerOptions.inputFileName);
   } else {
      yyin = stdin;
      debugPrintf(" -> Reading from standard input\n");
   }
   
   /* parse the program */
   yyparse();

   /* Check if an error occurred while parsing and in that case exit now */
   if (num_error > 0)
      return 1;

#ifndef NDEBUG
   char *logFileName = getLogFileName("frontend");
   debugPrintf(" -> Writing the output of parsing to \"%s\"\n", logFileName);
   FILE *logFile = fopen(logFileName, "w");
   dumpProgram(program, logFile);
   fclose(logFile);
   free(logFileName);
#endif

   /* do not attach a line number to the instructions generated by the
    * transformations that follow. */
   line_num = -1;
   return 0;
}

/******************************************************************************
 * MAIN
 ******************************************************************************/

int main(int argc, char *argv[])
{
   /* Read the options on the command line */
   int error = parseCompilerOptions(argc, argv);
   if (error != 0)
      return error;

   /* initialize the translation infos */
   program = newProgram();

   debugPrintf("Parsing the input program\n");
   error = parseProgram(program);

   if (error != 0) {
      /* Syntax errors have happened... */
      fprintf(stderr, "Input contains errors, no assembly file written.\n");
      fprintf(stderr, "%d error(s) found.\n", num_error);
   } else {
      debugPrintf("Lowering of pseudo-instructions to machine instructions.\n");
      doTargetSpecificTransformations(program);

      debugPrintf("Performing register allocation.\n");
      doRegisterAllocation(program);

      debugPrintf("Writing the assembly file.\n");
      writeAssembly(program);
   }
   
   debugPrintf("Finalizing the compiler data structures.\n");
   deleteProgram(program);

   debugPrintf("Done.\n");
   return 0;
}

/******************************************************************************
 * YYERROR
 ******************************************************************************/
void yyerror(const char* msg)
{
   emitSyntaxError(msg);
}
