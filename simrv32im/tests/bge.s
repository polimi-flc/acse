# 1 "bge.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "bge.S"
# See LICENSE for license details.

#*****************************************************************************
# bge.S
#-----------------------------------------------------------------------------

# Test bge instruction.


# 1 "riscv_test.h" 1
# 11 "bge.S" 2
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
# 12 "bge.S" 2


.text; .global _start; .global bge_ret; _start: lui s0,%hi(test_name); addi s0,s0,%lo(test_name); name_print_loop: lb a0,0(s0); beqz a0,prname_done; li a7,11; ecall; addi s0,s0,1; j name_print_loop; test_name: .ascii "bge"; .byte '.','.',0x00; .balign 4, 0; prname_done:

  #-------------------------------------------------------------
  # Branch tests
  #-------------------------------------------------------------

  # Each test checks both forward and backward branches

  test_2: li x28, 2; li x1, 0; li x2, 0; bge x1, x2, 2f; bne x0, x28, fail; 1: bne x0, x28, 3f; 2: bge x1, x2, 1b; bne x0, x28, fail; 3:;
  test_3: li x28, 3; li x1, 1; li x2, 1; bge x1, x2, 2f; bne x0, x28, fail; 1: bne x0, x28, 3f; 2: bge x1, x2, 1b; bne x0, x28, fail; 3:;
  test_4: li x28, 4; li x1, -1; li x2, -1; bge x1, x2, 2f; bne x0, x28, fail; 1: bne x0, x28, 3f; 2: bge x1, x2, 1b; bne x0, x28, fail; 3:;
  test_5: li x28, 5; li x1, 1; li x2, 0; bge x1, x2, 2f; bne x0, x28, fail; 1: bne x0, x28, 3f; 2: bge x1, x2, 1b; bne x0, x28, fail; 3:;
  test_6: li x28, 6; li x1, 1; li x2, -1; bge x1, x2, 2f; bne x0, x28, fail; 1: bne x0, x28, 3f; 2: bge x1, x2, 1b; bne x0, x28, fail; 3:;
  test_7: li x28, 7; li x1, -1; li x2, -2; bge x1, x2, 2f; bne x0, x28, fail; 1: bne x0, x28, 3f; 2: bge x1, x2, 1b; bne x0, x28, fail; 3:;

  test_8: li x28, 8; li x1, 0; li x2, 1; bge x1, x2, 1f; bne x0, x28, 2f; 1: bne x0, x28, fail; 2: bge x1, x2, 1b; 3:;
  test_9: li x28, 9; li x1, -1; li x2, 1; bge x1, x2, 1f; bne x0, x28, 2f; 1: bne x0, x28, fail; 2: bge x1, x2, 1b; 3:;
  test_10: li x28, 10; li x1, -2; li x2, -1; bge x1, x2, 1f; bne x0, x28, 2f; 1: bne x0, x28, fail; 2: bge x1, x2, 1b; 3:;
  test_11: li x28, 11; li x1, -2; li x2, 1; bge x1, x2, 1f; bne x0, x28, 2f; 1: bne x0, x28, fail; 2: bge x1, x2, 1b; 3:;

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  test_12: li x28, 12; li x4, 0; 1: li x1, -1; li x2, 0; bge x1, x2, fail; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b;
  test_13: li x28, 13; li x4, 0; 1: li x1, -1; li x2, 0; nop; bge x1, x2, fail; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b;
  test_14: li x28, 14; li x4, 0; 1: li x1, -1; li x2, 0; nop; nop; bge x1, x2, fail; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b;
  test_15: li x28, 15; li x4, 0; 1: li x1, -1; nop; li x2, 0; bge x1, x2, fail; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b;
  test_16: li x28, 16; li x4, 0; 1: li x1, -1; nop; li x2, 0; nop; bge x1, x2, fail; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b;
  test_17: li x28, 17; li x4, 0; 1: li x1, -1; nop; nop; li x2, 0; bge x1, x2, fail; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b;

  test_18: li x28, 18; li x4, 0; 1: li x1, -1; li x2, 0; bge x1, x2, fail; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b;
  test_19: li x28, 19; li x4, 0; 1: li x1, -1; li x2, 0; nop; bge x1, x2, fail; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b;
  test_20: li x28, 20; li x4, 0; 1: li x1, -1; li x2, 0; nop; nop; bge x1, x2, fail; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b;
  test_21: li x28, 21; li x4, 0; 1: li x1, -1; nop; li x2, 0; bge x1, x2, fail; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b;
  test_22: li x28, 22; li x4, 0; 1: li x1, -1; nop; li x2, 0; nop; bge x1, x2, fail; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b;
  test_23: li x28, 23; li x4, 0; 1: li x1, -1; nop; nop; li x2, 0; bge x1, x2, fail; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b;

  #-------------------------------------------------------------
  # Test delay slot instructions not executed nor bypassed
  #-------------------------------------------------------------

  test_24: li x1, 1; bge x1, x0, 1f; addi x1, x1, 1; addi x1, x1, 1; addi x1, x1, 1; addi x1, x1, 1; 1: addi x1, x1, 1; addi x1, x1, 1;; li x29, 3; li x28, 24; bne x1, x29, fail;
# 67 "bge.S"
  bne x0, x28, pass; fail: j fail_print; fail_string: .ascii "FAIL\n\0"; .balign 4, 0; fail_print: la s0,fail_string; fail_print_loop: lb a0,0(s0); beqz a0,fail_print_exit; li a7,11; ecall; addi s0,s0,1; j fail_print_loop; fail_print_exit: li a7,93; li a0,1; ecall;; pass: j pass_print; pass_string: .ascii "PASS!\n\0"; .balign 4, 0; pass_print: la s0,pass_string; pass_print_loop: lb a0,0(s0); beqz a0,pass_print_exit; li a7,11; ecall; addi s0,s0,1; j pass_print_loop; pass_print_exit: jal zero,bge_ret;

bge_ret: li a7,93; li a0,0; ecall;

  .data
.balign 4;

 


