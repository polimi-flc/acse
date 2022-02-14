/*
 * Andrea Di Biagio
 * Politecnico di Milano, 2007
 * 
 * utils.h
 * Formal Languages & Compilers Machine, 2007/2008
 * 
 * Contains important functions to access the list of symbols and other
 * utility functions and macros.
 */

#ifndef UTILS_H
#define UTILS_H

#include "program.h"
#include "list.h"


/* maximum and minimum between two values */
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) > (y) ? (y) : (x))


typedef struct t_axe_declaration
{
   int isArray;           /* must be TRUE if the current variable is an array */
   int arraySize;         /* the size of the array. This information is useful
                           * only if the field `isArray' is TRUE */
   int init_val;          /* initial value of the current variable. */
   char *ID;              /* variable identifier (should never be a NULL pointer
                           * or an empty string "") */
} t_axe_declaration;

typedef struct t_while_statement
{
   t_axe_label *label_condition;   /* this label points to the expression
                                    * that is used as loop condition */
   t_axe_label *label_end;         /* this label points to the instruction
                                    * that follows the while construct */
} t_while_statement;


/* In debug builds (NDEBUG not defined), prints a message on the standard
 * output like `printf'. Otherwise, does nothing and returns zero. */
extern int debugPrintf(const char *fmt, ...);

/* create an instance of `t_axe_variable' */
extern t_axe_declaration *initializeDeclaration(
      char *ID, int isArray, int arraySize, int init_val);

/* create a variable for each `t_axe_declaration' inside
 * the list `variables'. Each new variable will be of type
 * `varType'. */
extern void addVariablesFromDecls(
      t_program_infos *program, int varType, t_list *variables);

/* Notify the end of the program. This function is directly called
 * from the parser when the parsing process is ended */
extern void setProgramEnd(t_program_infos *program);

#endif