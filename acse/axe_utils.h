/*
 * Andrea Di Biagio
 * Politecnico di Milano, 2007
 * 
 * axe_utils.h
 * Formal Languages & Compilers Machine, 2007/2008
 * 
 * Contains important functions to access the list of symbols and other
 * utility functions and macros.
 */

#ifndef _AXE_UTILS_H
#define _AXE_UTILS_H

#include "axe_engine.h"
#include "axe_constants.h"
#include "collections.h"

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

/* create an instance that will mantain infos about a while statement */
extern t_while_statement createWhileStatement();

/* create an instance of `t_axe_variable' */
extern t_axe_declaration *initializeDeclaration(
      char *ID, int isArray, int arraySize, int init_val);

/* create a variable for each `t_axe_declaration' inside
 * the list `variables'. Each new variable will be of type
 * `varType'. */
extern void addVariablesFromDecls(
      t_program_infos *program, int varType, t_list *variables);

/* Given a variable/symbol identifier (ID) this function
 * returns a register location where the value is stored
 * (the value of the variable identified by `ID').
 * If the variable/symbol has never been loaded from memory
 * to a register, first this function searches
 * for a free register, then it assign the variable with the given
 * ID to the register just found.
 * Once computed, the location (a register identifier) is returned
 * as output to the caller.
 * This function generates a LOAD instruction
 * only if the flag `genLoad' is set to 1; otherwise it simply reserve
 * a register location for a new variable in the symbol table.
 * If an error occurs, getRegisterForSymbol returns a REG_INVALID errorcode */
extern int getRegisterForSymbol(t_program_infos *program, char *ID, int genLoad);

/* Generate the instruction to load an `immediate' value into a new register.
 * It returns the new register identifier or REG_INVALID if an error occurs */
extern int genLoadImmediate(t_program_infos *program, int immediate);

/* Generate the instruction to move an `immediate' value into a register. */
extern void genMoveImmediate(t_program_infos *program, int dest, int imm);

/* Returns 1 if `instr` is a jump (branch) instruction. */
extern int isJumpInstruction(t_axe_instruction *instr);

/* Returns 1 if `instr` is a unconditional jump instruction (BT, BF) */
extern int isUnconditionalJump(t_axe_instruction *instr);

/* Returns 1 if `instr` is either the HALT instruction or the RET
 * instruction. */
extern int isHaltOrRetInstruction(t_axe_instruction *instr);

/* Returns 1 if `instr` is the LOAD instruction. */
extern int isLoadInstruction(t_axe_instruction *instr);

/* Returns 1 if the opcode corresponds to an instruction with an immediate
 * argument (i.e. if the instruction mnemonic ends with `I`). */
extern int isImmediateArgumentInstrOpcode(int opcode);

/* Switches the immediate form of an opcode. For example, ADDI is transformed
 * to ADD, and ADD is transformed to ADDI. Returns the original opcode in case
 * there is no immediate or non-immediate available. */
extern int switchOpcodeImmediateForm(int orig);

/* Set the list of allowed machine registers for a specific register object
 * to the specified list of register identifiers. The list must be terminated
 * by REG_INVALID or -1. */
extern void setMCRegisterWhitelist(t_axe_register *regObj, ...);

/* Returns 1 if `instr` performs a move of either a register value, an
 * immediate, or a memory address pointer, to a register.
 * For example, instructions in the form "ADD R2, R0, R1" are considered move
 * instructions.
 * Stores in the given object pointers the destination register of the move and
 * the moved value. The object pointers can be NULL. Only one out of
 * `*outSrcReg`, `*outSrcAddr`, and `*outSrcImm` is set to the source of the
 * moved value. The other objects are set to NULL in the case of `*outSrcReg`
 * and `*outSrcAddr`, and not modified in the case of `*outSrcReg`. (it
 * follows that `*outSrcImm` is valid if and only if both `*outSrcReg` and
 * `*outSrcAddr` are NULL). */
extern int isMoveInstruction(t_axe_instruction *instr, t_axe_register **outDest,
      t_axe_register **outSrcReg, t_axe_address **outSrcAddr, int *outSrcImm);

/* Notify the end of the program. This function is directly called
 * from the parser when the parsing process is ended */
extern void setProgramEnd(t_program_infos *program);

/* Once called, this function destroys all the data structures
 * associated with the compiler (program, RA, etc.). This function
 * is typically automatically called before exiting from the main
 * or when the compiler encounters some error. */
extern void shutdownCompiler();

/* Once called, this function initialize all the data structures
 * associated with the compiler (program, RA etc..) and all the
 * global variables in the system. This function
 * is typically automatically called at the beginning of the main
 * and should NEVER be called from the user code */
extern void initializeCompiler(int argc, char **argv);

#endif
