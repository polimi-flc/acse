/*
 * Andrea Di Biagio
 * Politecnico di Milano, 2007
 * 
 * array.h
 * Formal Languages & Compilers Machine, 2007/2008
 *
 * Code generation for array management (load/store)
 */

#ifndef ARRAY_H
#define ARRAY_H

#include "program.h"
#include "expressions.h"

/* This function generates instructions that load the content of
 * an element of an array in a register.  This function takes as
 * input: a variable identifier (ID) that refers to an array
 * value; an index value that refers to a specific element of
 * the array.  It returns the location identifier for the
 * register that will contain the value of the array element at
 * position `index'.  `index' is an expression: its value can be
 * either a register location (i.e., the value of `index' is
 * stored inside a register) or an immediate value. */
extern int genLoadArrayElement(
      t_program_infos *program, char *ID, t_axe_expression index);

/* This function generates instructions that load the address of
 * an element of an array in a regester.  This function takes as
 * input: a variable identifier (ID) that refers to an array
 * value; an index value that refers to a specific element of
 * the array.  It returns the location identifier for the
 * register that will contain the address of the array element
 * at position `index'.  `index' is an expression: its value can
 * be either a register location (i.e., the value of `index' is
 * stored inside a register) or an immediate value. */
extern int genLoadArrayAddress(
      t_program_infos *program, char *ID, t_axe_expression index);

/* This function generates instructions that store a value
 * specified by `data' into the element at position `index' of
 * the array `ID'.  This function takes as input: a variable
 * identifier (ID) that refers to an array value; an index value
 * that refers to a specific element of the array; a value to be
 * stored (data).  `data' and `index' are expressions: their
 * value can be either register locations (i.e. their values are
 * stored inside a register) or immediate values. */
extern void genStoreArrayElement(t_program_infos *program, char *ID,
      t_axe_expression index, t_axe_expression data);

#endif