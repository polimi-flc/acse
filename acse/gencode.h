/// @file gencode.h
/// @brief Code generation functions.

#ifndef GENCODE_H
#define GENCODE_H

#include "program.h"


/*----------------------------------------------------
 *                   ARITHMETIC
 *---------------------------------------------------*/

t_instruction *genADDInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);
t_instruction *genSUBInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);
t_instruction *genANDInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);
t_instruction *genORInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);
t_instruction *genXORInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);
t_instruction *genMULInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);
t_instruction *genDIVInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);
t_instruction *genSLLInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);
t_instruction *genSRLInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);
t_instruction *genSRAInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);

/*----------------------------------------------------
 *                   ARITHMETIC WITH CONSTANT
 *---------------------------------------------------*/

/* Used in order to create and assign to the current `program'
 * an ADDI instruction. The semantic of an ADDI instruction
 * is the following: ADDI r_dest, r_source1, immediate. `RDest' is a register
 * location identifier: the result of the ADDI instruction will be
 * stored in that register. Using an RTL (Register Transfer Language)
 * representation we can say that an ADDI instruction of the form:
 * ADDI R1 R2 #IMM can be represented in the following manner: R1 <-- R2 + IMM.
 * `Rsource1' and `#IMM' are the two operands of the binary numeric
 * operation. `r_dest' is a register location, `immediate' is an immediate
 * value. The content of `r_source1' is added to the value of `immediate'
 * and the result is then stored into the register `RDest'. */
t_instruction *genADDIInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);

/* Used in order to create and assign to the current `program'
 * a SUBI instruction. The semantic of an SUBI instruction
 * is the following: SUBI r_dest, r_source1, immediate. `RDest' is a register
 * location identifier: the result of the SUBI instruction will be
 * stored in that register. Using an RTL representation we can say
 * that a SUBI instruction of the form: SUBI R1 R2 #IMM can be represented
 * in the following manner: R1 <-- R2 - IMM.
 * `Rsource1' and `#IMM' are the two operands of the binary numeric
 * operation. `r_dest' is a register location, `immediate' is an immediate
 * value. The content of `r_source1' is subtracted to the value of `immediate'
 * and the result is then stored into the register `RDest'. */
t_instruction *genSUBIInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);

/* Used in order to create and assign to the current `program'
 * an ANDBI instruction. An example RTL representation of ANDBI R1 R2 #IMM is:
 * R1 <-- R2 & IMM (bitwise AND).
 * `r_source1' and `immediate' are the two operands of the binary numeric
 * comparison. `r_dest' is a register location, `immediate' is an immediate
 * value. */
t_instruction *genANDIInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);

/* Used in order to create and assign to the current `program'
 * an ORBI instruction. An example RTL representation of ORBI R1 R2 #IMM is:
 * R1 <-- R2 | IMM.
 * `r_source1' and `immediate' are the two operands of the binary numeric
 * comparison. `r_dest' is a register location, `immediate' is an immediate
 * value. */
t_instruction *genORIInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);

/* Used in order to create and assign to the current `program'
 * a EORBI instruction. An example RTL representation of EORBI R1 R2 #IMM is:
 * R1 <-- R2 ^ IMM.
 * `r_source1' and `immediate' are the two operands of the binary numeric
 * comparison. `r_dest' is a register location, `immediate' is an immediate
 * value. */
t_instruction *genXORIInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);

/* Used in order to create and assign to the current `program'
 * a MULI instruction. An example RTL representation of MULI is:
 * R1 <-- R2 * IMM.
 * `r_source1' and `immediate' are the two operands of the binary numeric
 * comparison. `r_dest' is a register location, `immediate' is an immediate
 * value. */
t_instruction *genMULIInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);

/* Used in order to create and assign to the current `program'
 * a DIVI instruction. An example RTL representation of DIVI R1 R2 #IMM is:
 * R1 <-- R2 / IMM.
 * `r_source1' and `immediate' are the two operands of the binary numeric
 * comparison. `r_dest' is a register location, `immediate' is an immediate
 * value. */
t_instruction *genDIVIInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);

/* Used in order to create and assign to the current `program'
 * a SHLI instruction. An example RTL representation of SHLI R1 R2 #IMM is:
 * R1 <-- R2 / IMM.
 * `r_source1' and `immediate' are the two operands of the binary numeric
 * comparison. `r_dest' is a register location, `immediate' is an immediate
 * value. */
t_instruction *genSLLIInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);

t_instruction *genSRLIInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);

/* Used in order to create and assign to the current `program'
 * a SHRI instruction. An example RTL representation of SHRI R1 R2 #IMM is:
 * R1 <-- R2 / IMM.
 * `r_source1' and `immediate' are the two operands of the binary numeric
 * comparison. `r_dest' is a register location, `immediate' is an immediate
 * value. */
t_instruction *genSRAIInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);


/*----------------------------------------------------
 *                   COMPARISON
 *---------------------------------------------------*/

t_instruction *genSEQInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);
t_instruction *genSNEInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);
t_instruction *genSLTInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);
t_instruction *genSLTUInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);
t_instruction *genSGEInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);
t_instruction *genSGEUInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);
t_instruction *genSGTInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);
t_instruction *genSGTUInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);
t_instruction *genSLEInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);
t_instruction *genSLEUInstruction(
    t_program *program, int r_dest, int r_src1, int r_src2);


/*----------------------------------------------------
 *                   COMPARISON WITH CONSTANT
 *---------------------------------------------------*/

t_instruction *genSEQIInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);
t_instruction *genSNEIInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);
t_instruction *genSLTIInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);
t_instruction *genSLTIUInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);
t_instruction *genSGEIInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);
t_instruction *genSGEIUInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);
t_instruction *genSGTIInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);
t_instruction *genSGTIUInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);
t_instruction *genSLEIInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);
t_instruction *genSLEIUInstruction(
    t_program *program, int r_dest, int r_src1, int immediate);


/*----------------------------------------------------
 *                   JUMP
 *---------------------------------------------------*/

t_instruction *genJInstruction(t_program *program, t_label *label);

t_instruction *genBEQInstruction(
    t_program *program, int rs1, int rs2, t_label *label);
t_instruction *genBEQInstruction(
    t_program *program, int rs1, int rs2, t_label *label);
t_instruction *genBNEInstruction(
    t_program *program, int rs1, int rs2, t_label *label);
t_instruction *genBLTInstruction(
    t_program *program, int rs1, int rs2, t_label *label);
t_instruction *genBLTUInstruction(
    t_program *program, int rs1, int rs2, t_label *label);
t_instruction *genBGEInstruction(
    t_program *program, int rs1, int rs2, t_label *label);
t_instruction *genBGEUInstruction(
    t_program *program, int rs1, int rs2, t_label *label);
t_instruction *genBGTInstruction(
    t_program *program, int rs1, int rs2, t_label *label);
t_instruction *genBGTUInstruction(
    t_program *program, int rs1, int rs2, t_label *label);
t_instruction *genBLEInstruction(
    t_program *program, int rs1, int rs2, t_label *label);
t_instruction *genBLEUInstruction(
    t_program *program, int rs1, int rs2, t_label *label);


/*----------------------------------------------------
 *                  LOAD/STORE
 *---------------------------------------------------*/

t_instruction *genLIInstruction(t_program *program, int r_dest, int immediate);

/* A MOVA instruction copies an address value into a register.
 * An address can be either an instance of `t_label'
 * or a number (numeric address) */
t_instruction *genLAInstruction(t_program *program, int r_dest, t_label *label);

t_instruction *genLWInstruction(
    t_program *program, int r_dest, int immediate, int rs1);
t_instruction *genSWInstruction(
    t_program *program, int rs2, int immediate, int rs1);

t_instruction *genLWGlobalInstruction(
    t_program *program, int r_dest, t_label *label);
t_instruction *genSWGlobalInstruction(
    t_program *program, int rs2, t_label *label);


/*----------------------------------------------------
 *                   OTHER
 *---------------------------------------------------*/

/* By calling this function, a new NOP instruction will be added
 * to `program'. A NOP instruction doesn't make use of
 * any kind of parameter */
t_instruction *genNOPInstruction(t_program *program);

t_instruction *genECALLInstruction(t_program *program);
t_instruction *genEBREAKInstruction(t_program *program);


/*----------------------------------------------------
 *                  SYSTEM CALLS
 *---------------------------------------------------*/

/* By calling this function, a new HALT instruction will be added
 * to `program'. An HALT instruction doesn't require
 * any kind of parameter */
t_instruction *genExit0Syscall(t_program *program);

/* A READ instruction requires only one parameter:
 * A destination register (where the value
 * read from standard input will be loaded). */
t_instruction *genReadIntSyscall(t_program *program, int r_dest);

/* A WRITE instruction requires only one parameter:
 * A destination register (where the value
 * that will be written to the standard output is located). */
t_instruction *genPrintIntSyscall(t_program *program, int r_src1);

/* A WRITE instruction requires only one parameter:
 * A destination register (where the value
 * that will be written to the standard output is located). */
t_instruction *genPrintCharSyscall(t_program *program, int r_src1);


#endif
