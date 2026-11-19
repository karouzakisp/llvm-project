; REQUIRES: riscv-registered-target
; RUN: opt -mtriple=riscv64 -passes=wia,verify -S < %s | FileCheck %s
;
; Explicit invocation is available before default pipeline enablement.

define i8 @narrow(i8 %x) {
; CHECK-LABEL: define i8 @narrow(
; CHECK-NEXT: %sum = add i8 %x, %x
; CHECK-NEXT: ret i8 %sum
  %ext = zext i8 %x to i64
  %sum = add i64 %ext, %ext
  %trunc = trunc i64 %sum to i8
  ret i8 %trunc
}
