# 1 "srl.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "srl.S"
# See LICENSE for license details.

#*****************************************************************************
# srl.S
#-----------------------------------------------------------------------------

# Test srl instruction.


# 1 "riscv_test.h" 1
# 11 "srl.S" 2
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
# 12 "srl.S" 2


.text; .global _start; .global srl_ret; _start: lui s0,%hi(test_name); addi s0,s0,%lo(test_name); name_print_loop: lb a0,0(s0); beqz a0,prname_done; li a7,11; ecall; addi s0,s0,1; j name_print_loop; test_name: .ascii "srl"; .byte '.','.',0x00; .balign 4, 0; prname_done:

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  test_2: li x1, 0xffff8000; li x2, 0; srl x3, x1, x2;; li x29, 0xffff8000; li x28, 2; bne x3, x29, fail;;
  test_3: li x1, 0xffff8000; li x2, 1; srl x3, x1, x2;; li x29, 0x7fffc000; li x28, 3; bne x3, x29, fail;;
  test_4: li x1, 0xffff8000; li x2, 7; srl x3, x1, x2;; li x29, 0x01ffff00; li x28, 4; bne x3, x29, fail;;
  test_5: li x1, 0xffff8000; li x2, 14; srl x3, x1, x2;; li x29, 0x0003fffe; li x28, 5; bne x3, x29, fail;;
  test_6: li x1, 0xffff8001; li x2, 15; srl x3, x1, x2;; li x29, 0x0001ffff; li x28, 6; bne x3, x29, fail;;

  test_7: li x1, 0xffffffff; li x2, 0; srl x3, x1, x2;; li x29, 0xffffffff; li x28, 7; bne x3, x29, fail;;
  test_8: li x1, 0xffffffff; li x2, 1; srl x3, x1, x2;; li x29, 0x7fffffff; li x28, 8; bne x3, x29, fail;;
  test_9: li x1, 0xffffffff; li x2, 7; srl x3, x1, x2;; li x29, 0x01ffffff; li x28, 9; bne x3, x29, fail;;
  test_10: li x1, 0xffffffff; li x2, 14; srl x3, x1, x2;; li x29, 0x0003ffff; li x28, 10; bne x3, x29, fail;;
  test_11: li x1, 0xffffffff; li x2, 31; srl x3, x1, x2;; li x29, 0x00000001; li x28, 11; bne x3, x29, fail;;

  test_12: li x1, 0x21212121; li x2, 0; srl x3, x1, x2;; li x29, 0x21212121; li x28, 12; bne x3, x29, fail;;
  test_13: li x1, 0x21212121; li x2, 1; srl x3, x1, x2;; li x29, 0x10909090; li x28, 13; bne x3, x29, fail;;
  test_14: li x1, 0x21212121; li x2, 7; srl x3, x1, x2;; li x29, 0x00424242; li x28, 14; bne x3, x29, fail;;
  test_15: li x1, 0x21212121; li x2, 14; srl x3, x1, x2;; li x29, 0x00008484; li x28, 15; bne x3, x29, fail;;
  test_16: li x1, 0x21212121; li x2, 31; srl x3, x1, x2;; li x29, 0x00000000; li x28, 16; bne x3, x29, fail;;

  # Verify that shifts only use bottom five bits

  test_17: li x1, 0x21212121; li x2, 0xffffffe0; srl x3, x1, x2;; li x29, 0x21212121; li x28, 17; bne x3, x29, fail;;
  test_18: li x1, 0x21212121; li x2, 0xffffffe1; srl x3, x1, x2;; li x29, 0x10909090; li x28, 18; bne x3, x29, fail;;
  test_19: li x1, 0x21212121; li x2, 0xffffffe7; srl x3, x1, x2;; li x29, 0x00424242; li x28, 19; bne x3, x29, fail;;
  test_20: li x1, 0x21212121; li x2, 0xffffffee; srl x3, x1, x2;; li x29, 0x00008484; li x28, 20; bne x3, x29, fail;;
  test_21: li x1, 0x21212121; li x2, 0xffffffff; srl x3, x1, x2;; li x29, 0x00000000; li x28, 21; bne x3, x29, fail;;

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  test_22: li x1, 0xffff8000; li x2, 1; srl x1, x1, x2;; li x29, 0x7fffc000; li x28, 22; bne x1, x29, fail;;
  test_23: li x1, 0xffff8000; li x2, 14; srl x2, x1, x2;; li x29, 0x0003fffe; li x28, 23; bne x2, x29, fail;;
  test_24: li x1, 7; srl x1, x1, x1;; li x29, 0; li x28, 24; bne x1, x29, fail;;

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  test_25: li x4, 0; 1: li x1, 0xffff8000; li x2, 1; srl x3, x1, x2; addi x6, x3, 0; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 0x7fffc000; li x28, 25; bne x6, x29, fail;;
  test_26: li x4, 0; 1: li x1, 0xffff8000; li x2, 14; srl x3, x1, x2; nop; addi x6, x3, 0; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 0x0003fffe; li x28, 26; bne x6, x29, fail;;
  test_27: li x4, 0; 1: li x1, 0xffff8000; li x2, 15; srl x3, x1, x2; nop; nop; addi x6, x3, 0; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 0x0001ffff; li x28, 27; bne x6, x29, fail;;

  test_28: li x4, 0; 1: li x1, 0xffff8000; li x2, 1; srl x3, x1, x2; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 0x7fffc000; li x28, 28; bne x3, x29, fail;;
  test_29: li x4, 0; 1: li x1, 0xffff8000; li x2, 7; nop; srl x3, x1, x2; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 0x01ffff00; li x28, 29; bne x3, x29, fail;;
  test_30: li x4, 0; 1: li x1, 0xffff8000; li x2, 15; nop; nop; srl x3, x1, x2; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 0x0001ffff; li x28, 30; bne x3, x29, fail;;
  test_31: li x4, 0; 1: li x1, 0xffff8000; nop; li x2, 1; srl x3, x1, x2; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 0x7fffc000; li x28, 31; bne x3, x29, fail;;
  test_32: li x4, 0; 1: li x1, 0xffff8000; nop; li x2, 7; nop; srl x3, x1, x2; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 0x01ffff00; li x28, 32; bne x3, x29, fail;;
  test_33: li x4, 0; 1: li x1, 0xffff8000; nop; nop; li x2, 15; srl x3, x1, x2; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 0x0001ffff; li x28, 33; bne x3, x29, fail;;

  test_34: li x4, 0; 1: li x2, 1; li x1, 0xffff8000; srl x3, x1, x2; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 0x7fffc000; li x28, 34; bne x3, x29, fail;;
  test_35: li x4, 0; 1: li x2, 7; li x1, 0xffff8000; nop; srl x3, x1, x2; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 0x01ffff00; li x28, 35; bne x3, x29, fail;;
  test_36: li x4, 0; 1: li x2, 15; li x1, 0xffff8000; nop; nop; srl x3, x1, x2; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 0x0001ffff; li x28, 36; bne x3, x29, fail;;
  test_37: li x4, 0; 1: li x2, 1; nop; li x1, 0xffff8000; srl x3, x1, x2; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 0x7fffc000; li x28, 37; bne x3, x29, fail;;
  test_38: li x4, 0; 1: li x2, 7; nop; li x1, 0xffff8000; nop; srl x3, x1, x2; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 0x01ffff00; li x28, 38; bne x3, x29, fail;;
  test_39: li x4, 0; 1: li x2, 15; nop; nop; li x1, 0xffff8000; srl x3, x1, x2; addi x4, x4, 1; li x5, 2; bne x4, x5, 1b; li x29, 0x0001ffff; li x28, 39; bne x3, x29, fail;;

  test_40: li x1, 15; srl x2, x0, x1;; li x29, 0; li x28, 40; bne x2, x29, fail;;
  test_41: li x1, 32; srl x2, x1, x0;; li x29, 32; li x28, 41; bne x2, x29, fail;;
  test_42: srl x1, x0, x0;; li x29, 0; li x28, 42; bne x1, x29, fail;;
  test_43: li x1, 1024; li x2, 2048; srl x0, x1, x2;; li x29, 0; li x28, 43; bne x0, x29, fail;;

  bne x0, x28, pass; fail: j fail_print; fail_string: .ascii "FAIL\n\0"; .balign 4, 0; fail_print: la s0,fail_string; fail_print_loop: lb a0,0(s0); beqz a0,fail_print_exit; li a7,11; ecall; addi s0,s0,1; j fail_print_loop; fail_print_exit: li a7,93; li a0,1; ecall;; pass: j pass_print; pass_string: .ascii "PASS!\n\0"; .balign 4, 0; pass_print: la s0,pass_string; pass_print_loop: lb a0,0(s0); beqz a0,pass_print_exit; li a7,11; ecall; addi s0,s0,1; j pass_print_loop; pass_print_exit: jal zero,srl_ret;

srl_ret: li a7,93; li a0,0; ecall;

  .data
.balign 4;

 


