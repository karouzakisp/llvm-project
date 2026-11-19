; REQUIRES: riscv-registered-target
; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+f,+d,+c -disable-wia=true -passes='default<O2>' -verify-each %s | llc -mtriple=riscv64 -mattr=+m,+a,+f,+d,+c -disable-wia=true -verify-machineinstrs | FileCheck %s
; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+f,+d,+c -disable-wia=false -passes='default<O2>' -verify-each %s | llc -mtriple=riscv64 -mattr=+m,+a,+f,+d,+c -disable-wia=false -verify-machineinstrs | FileCheck %s

; Reduced from Lua's register-or-constant operand decoding. Bit 8 of a
; nine-bit field selects the array, and its low eight bits select the element.
; Preserve a direct sign-bit test. Extracting the complete field and comparing
; it with 256 introduces an unnecessary shift and threshold register.
;
; CHECK-LABEL: lua_decode_operand:
; CHECK:       slli [[FLAG:a[0-7]]], a0, 41
; CHECK-NOT:   srli [[FLAG]],
; CHECK:       b{{gez|ltz}} [[FLAG]],

target triple = "riscv64-unknown-linux-gnu"

define ptr @lua_decode_operand(i32 signext %insn, ptr %constants,
                              ptr %registers) minsize {
entry:
  %shift = lshr i32 %insn, 14
  %bits = and i32 %shift, 511
  %flag = and i32 %bits, 256
  %test = icmp ne i32 %flag, 0
  br i1 %test, label %constant, label %reg

constant:
  %index = and i32 %bits, -257
  %ci = zext nneg i32 %index to i64
  %cp = getelementptr inbounds [16 x i8], ptr %constants, i64 %ci
  br label %exit

reg:
  %ri = zext nneg i32 %bits to i64
  %rp = getelementptr inbounds [16 x i8], ptr %registers, i64 %ri
  br label %exit

exit:
  %result = phi ptr [ %cp, %constant ], [ %rp, %reg ]
  ret ptr %result
}
