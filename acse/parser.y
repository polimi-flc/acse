/// @file parser.y
/// @brief The bison grammar file that describes the LANCE language and
///        the semantic actions used to translate it to assembly code.

%{
#include <stdio.h>     
#include <stdlib.h>
#include <limits.h>
#include "program.h"
#include "acse.h"
#include "list.h"
#include "gencode.h"
#include "utils.h"
#include "scanner.h"
#include "parser.h"

// Global Variables

/* The number of errors and warnings found in the code. */
int num_error;
              
/* The singleton instance of `program'.
 *   An instance of `t_program' holds in its internal structure all the
 * fundamental informations about the program being compiled:
 *   - the list of instructions
 *   - the list of data directives (static allocations)
 *   - the list of variables
 *   - the list of labels
 *   - ... */
static t_program *program;

void yyerror(const char *msg)
{
  emitError("%s", msg);
}

%}

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
  t_symbol *var;
  t_listNode *list;
  t_label *label;
  t_ifStatement if_stmt;
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
%token LPAR RPAR LSQUARE RSQUARE LBRACE RBRACE
%token COMMA SEMI PLUS MINUS MUL_OP DIV_OP
%token AND_OP OR_OP NOT_OP
%token ASSIGN LT GT SHL_OP SHR_OP EQ NOTEQ LTEQ GTEQ
%token ANDAND OROR
%token TYPE
%token RETURN
%token READ WRITE ELSE

// These are the tokens with a semantic value of the given type.
%token <if_stmt> IF
%token <while_stmt> WHILE
%token <label> DO
%token <string> IDENTIFIER
%token <integer> NUMBER

/******************************************************************************
 * NON-TERMINAL SYMBOL SEMANTIC VALUE TYPE DECLARATIONS
 *
 * Here we declare the type of the semantic values of non-terminal symbols.
 *   We only declare the type of non-terminal symbols of which we actually use
 * their semantic value.
 ******************************************************************************/

%type <var> var_id
%type <integer> exp

/******************************************************************************
 * OPERATOR PRECEDENCE AND ASSOCIATIVITY
 *
 * Precedence is given by the declaration order. Associativity is given by the
 * specific keyword used (%left, %right).
 ******************************************************************************/

%left OROR
%left ANDAND
%left OR_OP
%left AND_OP
%left EQ NOTEQ
%left LT GT LTEQ GTEQ
%left SHL_OP SHR_OP
%left PLUS MINUS
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
 *   1. Declarations (zero or more),
 *   2. A list of instructions (zero or more). */
program
  : var_declarations statements EOF_TOK
  {
    // Generate the epilog of the program, that is, a call to the
    // `exit' syscall.
    genProgramEpilog(program);
    // Return from yyparse()
    YYACCEPT;
  }
;

/* This non-terminal appears at the beginning of the program and represents
 * all the declarations. */
var_declarations
  : var_declarations var_declaration
  | /* empty */
;

/* Each declaration consists of a type, a list of declarators, and a
 * terminating semicolon. */
var_declaration
  : TYPE declarator_list SEMI
;

declarator_list
  : declarator_list COMMA declarator
  | declarator
;

/* A declarator specifies either a scalar variable name or an array name
 * and size. */
declarator
  : IDENTIFIER
  {
    createSymbol(program, $1, TYPE_INT, 0);
  }
  | IDENTIFIER LSQUARE NUMBER RSQUARE
  {
    createSymbol(program, $1, TYPE_INT_ARRAY, $3);
  }
;

/* A block of code is a list of statements enclosed between braces */
code_block
  : LBRACE statements RBRACE
;

statements
  : statements statement
  | /* empty */
;

statement
  : assign_statement SEMI
  | if_statement
  | while_statement
  | do_while_statement SEMI
  | return_statement SEMI
  | read_statement SEMI
  | write_statement SEMI
  | SEMI
;

/* An assignment statement stores the value of an expression in the memory
 * location of a given scalar variable or array element. */
assign_statement
  : var_id ASSIGN exp
  {
    genStoreRegisterToVariable(program, $1, $3);
  }
  | var_id LSQUARE exp RSQUARE ASSIGN exp
  {
    genStoreRegisterToArrayElement(program, $1, $3, $6);
  }
;

/* An if statements first computes the expression, then jumps to the `else' part
 * if the expression is equal to zero. Otherwise the `then' part is executed.
 * After the `then' part the `else' part needs to be jumped over. */
if_statement
  : IF LPAR exp RPAR
  {
    // Generate a jump to the else part if the expression is equal to zero.
    $1.l_else = createLabel(program);
    genBEQ(program, $3, REG_0, $1.l_else);
  }
  code_block
  {
    // After the `then' part, generate a jump to the end of the statement
    $1.l_exit = createLabel(program);
    genJ(program, $1.l_exit);
    // Assign the label which points to the first instruction of the else part
    assignLabel(program, $1.l_else);
  }
  else_part
  {
    // Assign the label to the end of the statement
    assignLabel(program, $1.l_exit);
  }
;

/* The `else' part may be missing, in that case no code is generated. */
else_part
  : ELSE code_block
  | /* empty */
;

/* A while statement repeats the execution of its code block as long as the
 * expression is different than zero. The expression is computed at the
 * beginning of each loop iteration. */
while_statement
  : WHILE
  {
    // Assign a label at the beginning of the loop for the back-edge
    $1.l_loop = createLabel(program);
    assignLabel(program, $1.l_loop);
  }
  LPAR exp RPAR
  {
    // Generate a jump out of the loop if the condition is equal to zero
    $1.l_exit = createLabel(program);
    genBEQ(program, $4, REG_0, $1.l_exit);
  }
  code_block
  {
    // Generate a jump back to the beginning of the loop after its body
    genJ(program, $1.l_loop);
    // Assign the label to the end of the loop
    assignLabel(program, $1.l_exit);
  }
;

/* A do-while statement repeats the execution of its code block as long as the
 * expression is different than zero. The expression is computed at the
 * end of each loop iteration. */
do_while_statement
  : DO
  {
    // Assign a label at the beginning of the loop for the back-edge
    $1 = createLabel(program);
    assignLabel(program, $1);
  }
  code_block WHILE LPAR exp RPAR
  {
    // Generate a jump to the beginning of the loop to repeat the code block
    // if the condition is not equal to zero
    genBNE(program, $6, REG_0, $1);
  }
;

/* A return statement simply exits from the program, and hence translates to a
 * call to the `exit' syscall. */
return_statement
  : RETURN
  {
    genExit0Syscall(program);
  }
;

/* A read statement translates to a ReadInt syscall. The value it returns is
 * then stored in the appropriate variable. */
read_statement
  : READ LPAR var_id RPAR 
  {
    t_regID r_tmp = getNewRegister(program);
    genReadIntSyscall(program, r_tmp);
    genStoreRegisterToVariable(program, $3, r_tmp);
  }
;

/* A read statement translates to a PrintInt syscall, followed by a PrintChar
 * syscall which prints a newline. */  
write_statement 
  : WRITE LPAR exp RPAR 
  {
    // Generate a call to the PrintInt syscall.
    genPrintIntSyscall(program, $3);
    // Also generate code to print a newline after the integer
    t_regID r_temp = getNewRegister(program);
    genLI(program, r_temp, '\n');
    genPrintCharSyscall(program, r_temp);
  }
;

/* The exp rule represents the syntax of expressions. The semantic value of
 * the rule is the register ID that will contain the value of the expression
 * at runtime.
 *   All semantic actions which implement expression operators must handle both
 * the case in which the operands are expression values representing constants
 * or register IDs. */
exp
  : NUMBER
  {
    $$ = getNewRegister(program);
    genLI(program, $$, $1);
  }
  | var_id 
  {
    $$ = genLoadVariable(program, $1);
  }
  | var_id LSQUARE exp RSQUARE
  {
    $$ = genLoadArrayElement(program, $1, $3);
  }
  | LPAR exp RPAR
  {
    $$ = $2;
  }
  | MINUS exp
  {
    $$ = getNewRegister(program);
    genSUB(program, $$, REG_0, $2);
  }
  | exp PLUS exp
  {
    $$ = getNewRegister(program);
    genADD(program, $$, $1, $3);
  }
  | exp MINUS exp
  {
    $$ = getNewRegister(program);
    genSUB(program, $$, $1, $3);
  }
  | exp MUL_OP exp
  {
    $$ = getNewRegister(program);
    genMUL(program, $$, $1, $3);
  }
  | exp DIV_OP exp
  {
    $$ = getNewRegister(program);
    genDIV(program, $$, $1, $3);
  }
  | exp AND_OP exp 
  {
    $$ = getNewRegister(program);
    genAND(program, $$, $1, $3);
  }
  | exp OR_OP exp
  {
    $$ = getNewRegister(program);
    genOR(program, $$, $1, $3);
  }
  | exp SHL_OP exp
  {
    $$ = getNewRegister(program);
    genSLL(program, $$, $1, $3);
  }
  | exp SHR_OP exp
  {
    $$ = getNewRegister(program);
    genSRA(program, $$, $1, $3);
  }
  | exp LT exp
  {
    $$ = getNewRegister(program);
    genSLT(program, $$, $1, $3);
  }
  | exp GT exp
  {
    $$ = getNewRegister(program);
    genSGT(program, $$, $1, $3);
  }
  | exp EQ exp
  {
    $$ = getNewRegister(program);
    genSEQ(program, $$, $1, $3);
  }
  | exp NOTEQ exp
  {
    $$ = getNewRegister(program);
    genSNE(program, $$, $1, $3);
  }
  | exp LTEQ exp
  {
    $$ = getNewRegister(program);
    genSLE(program, $$, $1, $3);
  }
  | exp GTEQ exp
  {
    $$ = getNewRegister(program);
    genSGE(program, $$, $1, $3);
  }
  | NOT_OP exp
  {
    $$ = getNewRegister(program);
    genSEQ(program, $$, $2, REG_0);
  }
  | exp ANDAND exp
  {
    t_regID normalizedOp1 = getNewRegister(program);
    genSNE(program, normalizedOp1, $1, REG_0);
    t_regID normalizedOp2 = getNewRegister(program);
    genSNE(program, normalizedOp2, $3, REG_0);
    $$ = getNewRegister(program);
    genAND(program, $$, normalizedOp1, normalizedOp2);
  }
  | exp OROR exp
  {
    t_regID normalizedOp1 = getNewRegister(program);
    genSNE(program, normalizedOp1, $1, REG_0);
    t_regID normalizedOp2 = getNewRegister(program);
    genSNE(program, normalizedOp2, $3, REG_0);
    $$ = getNewRegister(program);
    genOR(program, $$, normalizedOp1, normalizedOp2);
  }
;

var_id
  : IDENTIFIER
  {
    t_symbol *var = getSymbol(program, $1);
    if (var == NULL) {
      yyerror("variable not declared");
      YYERROR;
    }
    $$ = var;
    free($1);
  }
;

%%

int parseProgram(t_program *_program, FILE *fp)
{
  /* Initialize all the global variables */
  lineNum = 1;
  num_error = 0;
  
  /* parse the program */
  program = _program;
  yyin = fp;
  yyparse();

  if (num_error == 0) {
#ifndef NDEBUG
    char *logFileName = getLogFileName("frontend");
    debugPrintf(" -> Writing the output of parsing to \"%s\"\n", logFileName);
    FILE *logFile = fopen(logFileName, "w");
    dumpProgram(program, logFile);
    fclose(logFile);
    free(logFileName);
#endif
  }

  /* do not attach a line number to the instructions generated by the
   * transformations that follow. */
  lineNum = -1;
  return num_error;
}
