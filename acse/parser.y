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
  t_expValue expr;
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
%type <expr> exp

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
    genStoreVariable(program, $1, $3);
  }
  | var_id LSQUARE exp RSQUARE ASSIGN exp
  {
    genStoreArrayElement(program, $1, $3, $6);
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
    if ($3.type == REGISTER) {
      genBEQ(program, $3.registerId, REG_0, $1.l_else);
    } else {
      // If the expression was constant, check the condition at compile time.
      if ($3.constant == 0)
        genJ(program, $1.l_else);
    }
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
    if ($4.type == REGISTER) {
      genBEQ(program, $4.registerId, REG_0, $1.l_exit);
    } else {
      // If the expression was constant, check the condition at compile time.
      if ($4.constant == 0)
        genJ(program, $1.l_exit);
    }
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
    if ($6.type == REGISTER) {
      genBNE(program, $6.registerId, REG_0, $1);
    } else {
      // If the expression was constant, check the condition at compile time.
      if ($6.constant != 0)
        genJ(program, $1);
    }
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
    genStoreVariable(program, $3, registerExpValue(r_tmp));
  }
;

/* A read statement translates to a PrintInt syscall, followed by a PrintChar
 * syscall which prints a newline. */  
write_statement 
  : WRITE LPAR exp RPAR 
  {
    // If necessary generate code to materialize the expression to a register,
    // and then generate a call to the PrintInt syscall.
    t_regID r_temp = genExpValueToRegister(program, $3);
    genPrintIntSyscall(program, r_temp);
    // Also generate code to print a newline after the integer
    genLI(program, r_temp, '\n');
    genPrintCharSyscall(program, r_temp);
  }
;

/* The exp rule represents the syntax of expressions. The semantic value of
 * the rule is a struct of type `t_expValue', which wraps either a
 * integer representing a compile-time constant or a register ID.
 *   All semantic actions which implement expression operators must handle both
 * the case in which the operands are expression values representing constants
 * or register IDs. */
exp
  : NUMBER
  {
    $$ = constantExpValue($1);
  }
  | var_id 
  {
    $$ = registerExpValue(genLoadVariable(program, $1));
  }
  | var_id LSQUARE exp RSQUARE
  {
    $$ = registerExpValue(genLoadArrayElement(program, $1, $3));
  }
  | LPAR exp RPAR
  {
    $$ = $2;
  }
  | MINUS exp
  {
    if ($2.type == CONSTANT) {
      $$ = constantExpValue(-($2.constant));
    } else {
      $$ = registerExpValue(getNewRegister(program));
      genSUB(program, $$.registerId, REG_0, $2.registerId);
    }
  }
  | exp PLUS exp
  {
    if ($1.type == CONSTANT && $3.type == CONSTANT) {
      $$ = constantExpValue($1.constant + $3.constant);
    } else {
      $$ = registerExpValue(getNewRegister(program));
      t_regID rs1 = genExpValueToRegister(program, $1);
      t_regID rs2 = genExpValueToRegister(program, $3);
      genADD(program, $$.registerId, rs1, rs2);
    }
  }
  | exp MINUS exp
  {
    if ($1.type == CONSTANT && $3.type == CONSTANT) {
      $$ = constantExpValue($1.constant - $3.constant);
    } else {
      $$ = registerExpValue(getNewRegister(program));
      t_regID rs1 = genExpValueToRegister(program, $1);
      t_regID rs2 = genExpValueToRegister(program, $3);
      genSUB(program, $$.registerId, rs1, rs2);
    }
  }
  | exp MUL_OP exp
  {
    if ($1.type == CONSTANT && $3.type == CONSTANT) {
      $$ = constantExpValue($1.constant * $3.constant);
    } else {
      $$ = registerExpValue(getNewRegister(program));
      t_regID rs1 = genExpValueToRegister(program, $1);
      t_regID rs2 = genExpValueToRegister(program, $3);
      genMUL(program, $$.registerId, rs1, rs2);
    }
  }
  | exp DIV_OP exp
  {
    if ($1.type == CONSTANT && $3.type == CONSTANT) {
      if ($3.constant == 0) {
        emitWarning("division by zero");
        $$ = constantExpValue(INT_MAX);
      } else if ($1.constant == INT_MIN && $3.constant == -1) {
        emitWarning("overflow");
        $$ = constantExpValue(INT_MIN);
      } else
        $$ = constantExpValue($1.constant / $3.constant);
    } else {
      $$ = registerExpValue(getNewRegister(program));
      t_regID rs1 = genExpValueToRegister(program, $1);
      t_regID rs2 = genExpValueToRegister(program, $3);
      genDIV(program, $$.registerId, rs1, rs2);
    }
  }
  | exp AND_OP exp 
  {
    if ($1.type == CONSTANT && $3.type == CONSTANT) {
      $$ = constantExpValue($1.constant & $3.constant);
    } else {
      $$ = registerExpValue(getNewRegister(program));
      t_regID rs1 = genExpValueToRegister(program, $1);
      t_regID rs2 = genExpValueToRegister(program, $3);
      genAND(program, $$.registerId, rs1, rs2);
    }
  }
  | exp OR_OP exp
  {
    if ($1.type == CONSTANT && $3.type == CONSTANT) {
      $$ = constantExpValue($1.constant | $3.constant);
    } else {
      $$ = registerExpValue(getNewRegister(program));
      t_regID rs1 = genExpValueToRegister(program, $1);
      t_regID rs2 = genExpValueToRegister(program, $3);
      genOR(program, $$.registerId, rs1, rs2);
    }
  }
  | exp SHL_OP exp
  {
    if ($1.type == CONSTANT && $3.type == CONSTANT) {
      if ($3.constant < 0 || $3.constant >= 32)
        emitWarning("shift amount is less than 0 or greater than 31");
      $$ = constantExpValue($1.constant << ($3.constant & 0x1F));
    } else {
      $$ = registerExpValue(getNewRegister(program));
      t_regID rs1 = genExpValueToRegister(program, $1);
      t_regID rs2 = genExpValueToRegister(program, $3);
      genSLL(program, $$.registerId, rs1, rs2);
    }
  }
  | exp SHR_OP exp
  {
    if ($1.type == CONSTANT && $3.type == CONSTANT) {
      if ($3.constant < 0 || $3.constant >= 32)
        emitWarning("shift amount is less than 0 or greater than 31");
      int constRes = SHIFT_RIGHT_ARITH($1.constant, $3.constant & 0x1F);
      $$ = constantExpValue(constRes);
    } else {
      $$ = registerExpValue(getNewRegister(program));
      t_regID rs1 = genExpValueToRegister(program, $1);
      t_regID rs2 = genExpValueToRegister(program, $3);
      genSRA(program, $$.registerId, rs1, rs2);
    }
  }
  | exp LT exp
  {
    if ($1.type == CONSTANT && $3.type == CONSTANT) {
      $$ = constantExpValue($1.constant < $3.constant);
    } else {
      $$ = registerExpValue(getNewRegister(program));
      t_regID rs1 = genExpValueToRegister(program, $1);
      t_regID rs2 = genExpValueToRegister(program, $3);
      genSLT(program, $$.registerId, rs1, rs2);
    }
  }
  | exp GT exp
  {
    if ($1.type == CONSTANT && $3.type == CONSTANT) {
      $$ = constantExpValue($1.constant > $3.constant);
    } else {
      $$ = registerExpValue(getNewRegister(program));
      t_regID rs1 = genExpValueToRegister(program, $1);
      t_regID rs2 = genExpValueToRegister(program, $3);
      genSGT(program, $$.registerId, rs1, rs2);
    }
  }
  | exp EQ exp
  {
    if ($1.type == CONSTANT && $3.type == CONSTANT) {
      $$ = constantExpValue($1.constant == $3.constant);
    } else {
      $$ = registerExpValue(getNewRegister(program));
      t_regID rs1 = genExpValueToRegister(program, $1);
      t_regID rs2 = genExpValueToRegister(program, $3);
      genSEQ(program, $$.registerId, rs1, rs2);
    }
  }
  | exp NOTEQ exp
  {
    if ($1.type == CONSTANT && $3.type == CONSTANT) {
      $$ = constantExpValue($1.constant != $3.constant);
    } else {
      $$ = registerExpValue(getNewRegister(program));
      t_regID rs1 = genExpValueToRegister(program, $1);
      t_regID rs2 = genExpValueToRegister(program, $3);
      genSNE(program, $$.registerId, rs1, rs2);
    }
  }
  | exp LTEQ exp
  {
    if ($1.type == CONSTANT && $3.type == CONSTANT) {
      $$ = constantExpValue($1.constant <= $3.constant);
    } else {
      $$ = registerExpValue(getNewRegister(program));
      t_regID rs1 = genExpValueToRegister(program, $1);
      t_regID rs2 = genExpValueToRegister(program, $3);
      genSLE(program, $$.registerId, rs1, rs2);
    }
  }
  | exp GTEQ exp
  {
    if ($1.type == CONSTANT && $3.type == CONSTANT) {
      $$ = constantExpValue($1.constant >= $3.constant);
    } else {
      $$ = registerExpValue(getNewRegister(program));
      t_regID rs1 = genExpValueToRegister(program, $1);
      t_regID rs2 = genExpValueToRegister(program, $3);
      genSGE(program, $$.registerId, rs1, rs2);
    }
  }
  | NOT_OP exp
  {
    if ($2.type == CONSTANT) {
      $$ = constantExpValue(!($2.constant));
    } else {
      $$ = registerExpValue(getNewRegister(program));
      genSEQ(program, $$.registerId, $2.registerId, REG_0);
    }
  }
  | exp ANDAND exp
  {
    t_expValue normLhs = genNormalizeBoolExpValue(program, $1);
    t_expValue normRhs = genNormalizeBoolExpValue(program, $3);
    if (normLhs.type == CONSTANT && normRhs.type == CONSTANT) {
      $$ = constantExpValue(normLhs.constant & normRhs.constant);
    } else {
      $$ = registerExpValue(getNewRegister(program));
      t_regID rs1 = genExpValueToRegister(program, normLhs);
      t_regID rs2 = genExpValueToRegister(program, normRhs);
      genAND(program, $$.registerId, rs1, rs2);
    }
  }
  | exp OROR exp
  {
    t_expValue normLhs = genNormalizeBoolExpValue(program, $1);
    t_expValue normRhs = genNormalizeBoolExpValue(program, $3);
    if ($1.type == CONSTANT && $3.type == CONSTANT) {
      $$ = constantExpValue(normLhs.constant | normRhs.constant);
    } else {
      $$ = registerExpValue(getNewRegister(program));
      t_regID rs1 = genExpValueToRegister(program, normLhs);
      t_regID rs2 = genExpValueToRegister(program, normRhs);
      genOR(program, $$.registerId, rs1, rs2);
    }
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
