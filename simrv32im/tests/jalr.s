# 1 "jalr.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "jalr.S"
# See LICENSE for license details.

#*****************************************************************************
# jalr.S
#-----------------------------------------------------------------------------

# Test jalr instruction.


# 1 "riscv_test.h" 1
# 11 "jalr.S" 2
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
# 12 "jalr.S" 2

#.option norvc


.text; .global _start; .global jalr_ret; _start: lui s0,%hi(test_name); addi s0,s0,%lo(test_name); name_print_loop: lb a0,0(s0); beqz a0,prname_done; li a7,11; ecall; addi s0,s0,1; j name_print_loop; test_name: .ascii "jalr"; .byte '.','.',0x00; .balign 4, 0; prname_done:

  #-------------------------------------------------------------
  # Test 2: Basic test
  #-------------------------------------------------------------

test_2:
  li x28, 2
  li x31, 0
  la x2, target_2

linkaddr_2:
  jalr x19, x2, 0
  nop
  nop

  j fail

target_2:
  la x1, linkaddr_2
  addi x1, x1, 4
  bne x1, x19, fail

  #-------------------------------------------------------------
  # Test 3: Check r0 target and that r31 is not modified
  #-------------------------------------------------------------

test_3:
  li x28, 3
  li x31, 0
  la x3, target_3

linkaddr_3:
  jalr x0, x3, 0
  nop

  j fail

target_3:
  bne x31, x0, fail

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  test_4: li x28, 4; li x4, 0; 1: la x6, 2f; jalr x19, x6, 0; bne x0, x28, fail; 2: addi x4, x4, 1; li x5, 2; bne x4, x5, 1b;
  test_5: li x28, 5; li x4, 0; 1: la x6, 2f; nop; jalr x19, x6, 0; bne x0, x28, fail; 2: addi x4, x4, 1; li x5, 2; bne x4, x5, 1b;
  test_6: li x28, 6; li x4, 0; 1: la x6, 2f; nop; nop; jalr x19, x6, 0; bne x0, x28, fail; 2: addi x4, x4, 1; li x5, 2; bne x4, x5, 1b;

  #-------------------------------------------------------------
  # Test delay slot instructions not executed nor bypassed
  #-------------------------------------------------------------

  test_7: li x1, 1; la x2, 1f; jalr x19, x2, -4; addi x1, x1, 1; addi x1, x1, 1; addi x1, x1, 1; addi x1, x1, 1; 1: addi x1, x1, 1; addi x1, x1, 1;; li x29, 4; li x28, 7; bne x1, x29, fail;
# 81 "jalr.S"
  bne x0, x28, pass; fail: j fail_print; fail_string: .ascii "FAIL\n\0"; .balign 4, 0; fail_print: la s0,fail_string; fail_print_loop: lb a0,0(s0); beqz a0,fail_print_exit; li a7,11; ecall; addi s0,s0,1; j fail_print_loop; fail_print_exit: li a7,93; li a0,1; ecall;; pass: j pass_print; pass_string: .ascii "PASS!\n\0"; .balign 4, 0; pass_print: la s0,pass_string; pass_print_loop: lb a0,0(s0); beqz a0,pass_print_exit; li a7,11; ecall; addi s0,s0,1; j pass_print_loop; pass_print_exit: jal zero,jalr_ret;

jalr_ret: li a7,93; li a0,0; ecall;

  .data
.balign 4;

 

