# 1 "addi.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "addi.S"
# See LICENSE for license details.

#*****************************************************************************
# addi.S
#-----------------------------------------------------------------------------

# Test addi instruction.


# 1 "riscv_test.h" 1
# 11 "addi.S" 2
# 1 "test_macros.h" 1






#-----------------------------------------------------------------------
# Helper macros
#-----------------------------------------------------------------------
# 18 "test_macros.h"
# We use a macro hack to simpify code generation for various numbers
# of bubble cycles.
# 34 "test_macros.h"
#-----------------------------------------------------------------------
# RV64UI MACROS
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# Tests for instructions with immediate operand
#-----------------------------------------------------------------------
# 90 "test_macros.h"
#-----------------------------------------------------------------------
# Tests for vector config instructions
#-----------------------------------------------------------------------
# 118 "test_macros.h"
#-----------------------------------------------------------------------
# Tests for an instruction with register operands
#-----------------------------------------------------------------------
# 146 "test_macros.h"
#-----------------------------------------------------------------------
# Tests for an instruction with register-register operands
#-----------------------------------------------------------------------
# 240 "test_macros.h"
#-----------------------------------------------------------------------
# Test memory instructions
#-----------------------------------------------------------------------
# 317 "test_macros.h"
#-----------------------------------------------------------------------
# Test branch instructions
#-----------------------------------------------------------------------
# 402 "test_macros.h"
#-----------------------------------------------------------------------
# Test jump instructions
#-----------------------------------------------------------------------
# 431 "test_macros.h"
#-----------------------------------------------------------------------
# RV64UF MACROS
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# Tests floating-point instructions
#-----------------------------------------------------------------------
# 567 "test_macros.h"
#-----------------------------------------------------------------------
# Pass and fail code (assumes test num is in x28)
#-----------------------------------------------------------------------
# 579 "test_macros.h"
#-----------------------------------------------------------------------
# Test data section
#-----------------------------------------------------------------------
# 12 "addi.S" 2


.text; .global _start; .global addi_ret; _start: lui s0,%hi(test_name); addi s0,s0,%lo(test_name); name_print_loop: lb a0,0(s0); beqz a0,prname_done; li a7,11; ecall; addi s0,s0,1; j name_print_loop; test_name: .ascii "addi"; .byte '.','.',0x00; .balign 4, 0; prname_done:

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  test_2: li x1, 0x00000000; addi x3, x1, 0x000;; li x29, 0x00000000; li x28, 2; bne x3, x29, fail;;
  test_3: li x1, 0x00000001; addi x3, x1, 0x001;; li x29, 0x00000002; li x28, 3; bne x3, x29, fail;;
  test_4: li x1, 0x00000003; addi x3, x1, 0x007;; li x29, 0x0000000a; li x28, 4; bne x3, x29, fail;;

  test_5: li x1, 0x00000000; addi x3, x1, -0x800;; li x29, 0xfffff800; li x28, 5; bne x3, x29, fail;;
  test_6: li x1, 0x80000000; addi x3, x1, 0x000;; li x29, 0x80000000; li x28, 6; bne x3, x29, fail;;
  test_7: li x1, 0x80000000; addi x3, x1, -0x800;; li x29, 0x7ffff800; li x28, 7; bne x3, x29, fail;;

  test_8: li x1, 0x00000000; addi x3, x1, 0x7ff;; li x29, 0x000007ff; li x28, 8; bne x3, x29, fail;;
  test_9: li x1, 0x7fffffff; addi x3, x1, 0x000;; li x29, 0x7fffffff; li x28, 9; bne x3, x29, fail;;
  test_10: li x1, 0x7fffffff; addi x3, x1, 0x7ff;; li x29, 0x800007fe; li x28, 10; bne x3, x29, fail;;

  test_11: li x1, 0x80000000; addi x3, x1, 0x7ff;; li x29, 0x800007ff; li x28, 11; bne x3, x29, fail;;
  test_12: li x1, 0x7fffffff; addi x3, x1, -0x800;; li x29, 0x7ffff7ff; li x28, 12; bne x3, x29, fail;;

  test_13: li x1, 0x00000000; addi x3, x1, -1;; li x29, 0xffffffff; li x28, 13; bne x3, x29, fail;;
  test_14: li x1, 0xffffffff; addi x3, x1, 0x001;; li x29, 0x00000000; li x28, 14; bne x3, x29, fail;;
  test_15: li x1, 0xffffffff; addi x3, x1, -1;; li x29, 0xfffffffe; li x28, 15; bne x3, x29, fail;;

  test_16: li x1, 0x7fffffff; addi x3, x1, 0x001;; li x29, 0x80000000; li x28, 16; bne x3, x29, fail;;

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  test_17: li x1, 13; addi x1, x1, 11;; li x29, 24; li x28, 17; bne x1, x29, fail;;

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  test_18: li x4, 0; 1: li x1, 13; addi x3, x1, 11; addi x6, x3, 0; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 24; li x28, 18; bne x6, x29, fail;;
  test_19: li x4, 0; 1: li x1, 13; addi x3, x1, 10; nop; addi x6, x3, 0; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 23; li x28, 19; bne x6, x29, fail;;
  test_20: li x4, 0; 1: li x1, 13; addi x3, x1, 9; nop; nop; addi x6, x3, 0; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 22; li x28, 20; bne x6, x29, fail;;

  test_21: li x4, 0; 1: li x1, 13; addi x3, x1, 11; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 24; li x28, 21; bne x3, x29, fail;;
  test_22: li x4, 0; 1: li x1, 13; nop; addi x3, x1, 10; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 23; li x28, 22; bne x3, x29, fail;;
  test_23: li x4, 0; 1: li x1, 13; nop; nop; addi x3, x1, 9; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 22; li x28, 23; bne x3, x29, fail;;

  test_24: addi x1, x0, 32;; li x29, 32; li x28, 24; bne x1, x29, fail;;
  test_25: li x1, 33; addi x0, x1, 50;; li x29, 0; li x28, 25; bne x0, x29, fail;;

  bne x0, x28, pass; fail: j fail_print; fail_string: .ascii "FAIL\n\0"; .balign 4, 0; fail_print: la s0,fail_string; fail_print_loop: lb a0,0(s0); beqz a0,fail_print_exit; li a7,11; ecall; addi s0,s0,1; j fail_print_loop; fail_print_exit: li a7,93; li a0,1; ecall;; pass: j pass_print; pass_string: .ascii "PASS!\n\0"; .balign 4, 0; pass_print: la s0,pass_string; pass_print_loop: lb a0,0(s0); beqz a0,pass_print_exit; li a7,11; ecall; addi s0,s0,1; j pass_print_loop; pass_print_exit: jal zero,addi_ret;

addi_ret: li a7,93; li a0,0; ecall;

  .data
.balign 4;

 

