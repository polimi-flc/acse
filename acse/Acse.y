%{
/*
 * Andrea Di Biagio
 * Politecnico di Milano, 2007
 * 
 * Acse.y
 * Formal Languages & Compilers Machine, 2007/2008
 */

/*************************************************************************

                   Compiler for the language LANCE

***************************************************************************/

#include <stdio.h>       
#include <stdlib.h>
#include <assert.h>
#include "axe_engine.h"
#include "axe_target_asm_print.h"
#include "axe_target_transform.h"
#include "axe_errors.h"
#include "collections.h"
#include "axe_expressions.h"
#include "axe_gencode.h"
#include "axe_utils.h"
#include "axe_array.h"
#include "axe_cflow_graph.h"
#include "axe_reg_alloc.h"
#include "axe_io_manager.h"
#include "axe_debug.h"

/* global variables */
int line_num;        /* this variable will keep track of the
                      * source code line number. Every time that a newline
                      * is encountered while parsing the input file, this
                      * value is increased by 1. This value is then used
                      * for error tracking: if the parser returns an error
                      * or a warning, this value is used in order to notify
                      * in which line of code the error has been found */
int num_error;       /* the number of errors found in the code. This value
                      * is increased by 1 every time a new error is found
                      * in the code. */
int num_warning;     /* As for the `num_error' global variable, this one
                      * keeps track of all the warning messages displayed */

/* errorcode is defined inside "axe_engine.c" */
extern int errorcode;   /* this variable is used to test if an error is found
                         * while parsing the input file. It also is set
                         * to notify if the compiler internal state is invalid.
                         * When the parsing process is started, the value
                         * of `errorcode' is set to the value of the macro
                         * `AXE_OK' defined in "axe_constants.h".
                         * As long as everything (the parsed source code and
                         * the internal state of the compiler) is correct,
                         * the value of `errorcode' is set to `AXE_OK'.
                         * When an error occurs (because the input file contains
                         * one or more syntax errors or because something went
                         * wrong in the machine internal state), the errorcode
                         * is set to a value that is different from `AXE_OK'. */
extern const char *errormsg; /* When errorcode is not equal to AXE_OK,
                         * this variable may be set to an error message to print
                         * if desired. */
                     
/* program informations */
t_program_infos *program;  /* The singleton instance of `program'.
                            * An instance of `t_program_infos' holds in its
                            * internal structure, all the useful informations
                            * about a program. For example: the assembly
                            * (code and directives);
                            * the label manager (see axe_labels.h) etc. */

t_io_infos *file_infos;    /* input and output files used by the compiler */


extern int yylex(void);
extern void yyerror(const char*);

%}
%expect 1

/*=========================================================================
                          SEMANTIC RECORDS
=========================================================================*/

%union {            
   int intval;
   char *svalue;
   t_axe_expression expr;
   t_axe_declaration *decl;
   t_list *list;
   t_axe_label *label;
   t_while_statement while_stmt;
} 
/*=========================================================================
                               TOKENS 
=========================================================================*/
%start program

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

%token <label> DO
%token <while_stmt> WHILE
%token <label> IF
%token <label> ELSE
%token <intval> TYPE
%token <svalue> IDENTIFIER
%token <intval> NUMBER

%type <expr> exp
%type <decl> declaration
%type <list> declaration_list
%type <label> if_stmt

/*=========================================================================
                          OPERATOR PRECEDENCES
 =========================================================================*/

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

/*=========================================================================
                         BISON GRAMMAR
=========================================================================*/
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

var_declarations : var_declarations var_declaration   { /* does nothing */ }
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

declaration : IDENTIFIER ASSIGN NUMBER
            {
               /* create a new instance of t_axe_declaration */
               $$ = initializeDeclaration($1, 0, 0, $3);

               /* test if an `out of memory' occurred */
               if ($$ == NULL)
                  fatalError(AXE_OUT_OF_MEMORY);
            }
            | IDENTIFIER LSQUARE NUMBER RSQUARE
            {
               /* create a new instance of t_axe_declaration */
               $$ = initializeDeclaration($1, 1, $3, 0);

                  /* test if an `out of memory' occurred */
               if ($$ == NULL)
                  fatalError(AXE_OUT_OF_MEMORY);
            }
            | IDENTIFIER
            {
               /* create a new instance of t_axe_declaration */
               $$ = initializeDeclaration($1, 0, 0, 0);
               
               /* test if an `out of memory' occurred */
               if ($$ == NULL)
                  fatalError(AXE_OUT_OF_MEMORY);
            }
;

/* A block of code can be either a single statement or
 * a set of statements enclosed between braces */
code_block  : statement                  { /* does nothing */ }
            | LBRACE statements RBRACE   { /* does nothing */ }
;

/* One or more code statements */
statements  : statements statement       { /* does nothing */ }
            | statement                  { /* does nothing */ }
;

/* A statement can be either an assignment statement or a control statement
 * or a read/write statement or a semicolon */
statement   : assign_statement SEMI      { /* does nothing */ }
            | control_statement          { /* does nothing */ }
            | read_write_statement SEMI  { /* does nothing */ }
            | SEMI            { genNOPInstruction(program); }
;

control_statement : if_statement         { /* does nothing */ }
            | while_statement            { /* does nothing */ }
            | do_while_statement SEMI    { /* does nothing */ }
            | return_statement SEMI      { /* does nothing */ }
;

read_write_statement : read_statement  { /* does nothing */ }
                     | write_statement { /* does nothing */ }
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
               int location;

               /* in order to assign a value to a variable, we have to
                * know where the variable is located (i.e. in which register).
                * the function `getRegLocationOfScalar' is used in order
                * to retrieve the register location assigned to
                * a given identifier.
                * `getRegLocationOfScalar' perform a query on the list
                * of variables in order to discover the correct location of
                * the variable with $1 as identifier */
               
               /* get the location of the variable with the given ID. */
               location = getRegLocationOfScalar(program, $1);

               /* update the value of location */
               if ($3.type == IMMEDIATE)
                  genMoveImmediate(program, location, $3.immediate);
               else
                  genADDInstruction(program, location, REG_0, $3.registerId);

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
                  $2 = newLabel(program);
   
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
            
if_stmt  :  IF
               {
                  /* the label that points to the address where to jump if
                   * `exp' is not verified */
                  $1 = newLabel(program);
               }
               LPAR exp RPAR
               {
                  if ($4.type == IMMEDIATE) {
                     if ($4.immediate == 0)
                        genJInstruction(program, $1);
                  } else {
                     /* if `exp' returns FALSE, jump to the label $1 */
                     genBEQInstruction(program, $4.registerId, REG_0, $1);
                  }
               }
               code_block { $$ = $1; }
;

while_statement  : WHILE
                  {
                     /* reserve and fix a new label */
                     $1.label_condition = assignNewLabel(program);
                  }
                  LPAR exp RPAR
                  {
                     /* reserve a new label. This new label will point
                      * to the first instruction after the while code
                      * block */
                     $1.label_end = newLabel(program);

                     if ($4.type == IMMEDIATE) {
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
                  
do_while_statement  : DO
                     {
                        /* the label that points to the address where to jump if
                         * `exp' is not verified */
                        $1 = newLabel(program);
                        
                        /* fix the label */
                        assignLabel(program, $1);
                     }
                     code_block WHILE LPAR exp RPAR
                     {
                        if ($6.type == IMMEDIATE) {
                           if ($6.immediate != 0)
                              genJInstruction(program, $1);
                        } else {
                           /* if `exp' returns TRUE, jump to the label $1 */
                           genBNEInstruction(program, $6.registerId, REG_0, $1);
                        }
                     }
;

return_statement : RETURN
            {
               /* insert an HALT instruction */
               genHALTInstruction(program);
            }
;

read_statement : READ LPAR IDENTIFIER RPAR 
            {
               int location;
               
               /* read from standard input an integer value and assign
                * it to a variable associated with the given identifier */
               /* get the location of the variable with the given ID */
               
               /* lookup the variable table and fetch the register location
                * associated with the IDENTIFIER $3. */
               location = getRegLocationOfScalar(program, $3);

               /* insert a read instruction */
               genREADInstruction (program, location);

               /* free the memory associated with the IDENTIFIER */
               free($3);
            }
;
            
write_statement : WRITE LPAR exp RPAR 
            {
               int location;

               if ($3.type == IMMEDIATE)
               {
                  /* load `immediate' into a new register. Returns the new
                   * register identifier or REG_INVALID if an error occurs */
                  location = genLoadImmediate(program, $3.immediate);
               }
               else
                  location = $3.registerId;

               /* write to standard output an integer value */
               genWRITEInstruction (program, location);
            }
;

exp: NUMBER      { $$ = getImmediateExpression($1); }
   | IDENTIFIER  {
                     int variableReg, expValReg;
                     /* get the location of the symbol with the given ID */
                     variableReg = getRegLocationOfScalar(program, $1);
                     /* generate code that copies the value of the variable in
                      * a new register to freeze the expression's value */
                     expValReg = getNewRegister(program);
                     genADDIInstruction(program, expValReg, variableReg, 0);
                     /* return that register as the expression value */
                     $$ = getRegisterExpression(expValReg);
                     /* free the memory associated with the IDENTIFIER */
                     free($1);
   }
   | IDENTIFIER LSQUARE exp RSQUARE {
                     int reg;
                     
                     /* load the value IDENTIFIER[exp]
                      * into `arrayElement' */
                     reg = genLoadArrayElement(program, $1, $3);

                     /* create a new expression */
                     $$ = getRegisterExpression(reg);

                     /* free the memory associated with the IDENTIFIER */
                     free($1);
   }
   | NOT_OP exp {
               if ($2.type == IMMEDIATE)
               {
                  /* IMMEDIATE (constant) expression: compute the value at
                   * compile-time and place the result in a new IMMEDIATE
                   * expression */
                  $$ = getImmediateExpression(!($2.immediate));
               }
               else
               {
                  /* REGISTER expression: generate the code that will compute
                   * the result at compile time */

                  /* Reserve a new register for the result */
                  int res_reg = getNewRegister(program);

                  /* Generate a SUBI instruction which will store the negated
                   * logic value into the register we reserved */
                  genSEQInstruction(program, res_reg, $2.registerId, REG_0);
                  
                  /* Return a REGISTER expression with the result register */
                  $$ = getRegisterExpression(res_reg);
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
   | MINUS exp {
                  if ($2.type == IMMEDIATE)
                  {
                     $$ = getImmediateExpression(-($2.immediate));
                  }
                  else
                  {
                     /* create an expression for register REG_0 */
                     t_axe_expression exp_r0 = getRegisterExpression(REG_0);
                     $$ = handleBinaryOperator(program, exp_r0, $2, OP_SUB);
                  }
               }
;

%%
/*=========================================================================
                                  MAIN
=========================================================================*/
int main (int argc, char **argv)
{
   /* initialize all the compiler data structures and global variables */
   initializeCompiler(argc, argv);
   
   /* initialize the line number */
   line_num = 1;
   /* parse the program */
   yyparse();
   /* do not attach a line number to the instructions generated by the
    * transformations that follow. */
   line_num = -1;
   
   debugPrintf("Parsing process completed. \n");
   
#ifndef NDEBUG
   printProgramInfos(program, file_infos->frontend_output);
#endif
   /* test if the parsing process completed succesfully */
   checkConsistency();

   doTargetSpecificTransformations(program);

   doRegisterAllocation(program);

   debugPrintf("Writing the assembly file...\n");
   writeAssembly(program, file_infos->output_file_name);
      
   debugPrintf("Assembly written on file \"%s\".\n", file_infos->output_file_name);
   
   /* shutdown the compiler */
   shutdownCompiler(0);

   return 0;
}

/*=========================================================================
                                 YYERROR
=========================================================================*/
void yyerror(const char* msg)
{
   errorcode = AXE_SYNTAX_ERROR;
   free((void *)errormsg);
   errormsg = strdup(msg);
}
