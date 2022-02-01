/*
 * Daniele Cattaneo
 * Politecnico di Milano, 2022
 * 
 * axe_target_info.h
 * Formal Languages & Compilers Machine, 2007-2022
 * 
 * Properties of the target machine
 */

#include <assert.h>
#include "axe_target_info.h"

int isHaltOrRetInstruction(t_axe_instruction *instr)
{
   if (instr == NULL) {
      return 0;
   }

   return instr->opcode == OPC_HALT;
}

/* test if the current instruction `instr' is a BT or a BF */
int isUnconditionalJump(t_axe_instruction *instr)
{
   if (isJumpInstruction(instr))
   {
      if (instr->opcode == OPC_J)
         return 1;
   }

   return 0;
}

/* test if the current instruction `instr' is a branch instruction */
int isJumpInstruction(t_axe_instruction *instr)
{
   if (instr == NULL)
      return 0;

   switch (instr->opcode) {
      case OPC_J:
      case OPC_BEQ:
      case OPC_BNE:
      case OPC_BLT:
      case OPC_BLTU:
      case OPC_BGE:
      case OPC_BGEU:
      case OPC_BGT:
      case OPC_BGTU:
      case OPC_BLE:
      case OPC_BLEU: return 1;
      default: return 0;
   }
}

int isCallInstruction(t_axe_instruction *instr)
{
   return instr->opcode == OPC_ECALL;
}

int instructionUsesPSW(t_axe_instruction *instr)
{
   return 0;
}

int instructionDefinesPSW(t_axe_instruction *instr)
{
   return 0;
}

int getSpillRegister(int i)
{
   assert(i < NUM_SPILL_REGS);
   return i + REG_T3;
}

t_list *getListOfGenPurposeRegisters(void)
{
   static const int regs[NUM_GP_REGS] = {
      REG_S0, REG_S1, REG_S2, REG_S3, REG_S4, REG_S5,
      REG_S6, REG_S7, REG_S8, REG_S9, REG_S10, REG_S11,
      REG_T0, REG_T1, REG_T2,
      REG_A0, REG_A1, REG_A2, REG_A3, REG_A4, REG_A5, REG_A6, REG_A7
   };
   int i;
   t_list *res = NULL;
   
   for (i = NUM_GP_REGS-1; i >= 0; i--) {
      res = addFirst(res, INTDATA(regs[i]));
   }
   return res;
}

t_list *getListOfCallerSaveRegisters(void)
{
   static const int regs[] = {
      REG_T0, REG_T1, REG_T2,
      REG_A0, REG_A1, REG_A2, REG_A3, REG_A4, REG_A5, REG_A6, REG_A7,
      REG_INVALID
   };
   int i;
   t_list *res = NULL;

   for (i = 0; regs[i] != REG_INVALID; i++) {
      res = addFirst(res, INTDATA(regs[i]));
   }
   return res;
}

