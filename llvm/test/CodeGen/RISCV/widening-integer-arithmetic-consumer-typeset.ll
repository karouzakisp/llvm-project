; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c -passes=wia,verify -S < %s | FileCheck %s
; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c -passes=wia,wia,verify -S < %s | FileCheck %s

target triple = "riscv64"

; Pointer comparisons are valid IR, but cannot enter a scalar-integer
; promotion graph. Do not query their operand type as an IntegerType.
; CHECK-LABEL: define i8 @pointer_compare_boolean_extension(
; CHECK: icmp eq ptr %p, %q
; CHECK: ret i8
define i8 @pointer_compare_boolean_extension(ptr %p, ptr %q) minsize {
  %cmp = icmp eq ptr %p, %q
  %not = xor i1 %cmp, true
  %result = zext i1 %not to i8
  ret i8 %result
}

; An i1 comparison result does not make its i64 operands eligible for
; promotion to i8. Reject the wider arithmetic before forming constants.
; CHECK-LABEL: define i8 @wide_compare_boolean_extension(
; CHECK: [[REM:%.*]] = srem i64 {{%.*}}, 20
; CHECK: icmp ne i64 [[REM]], 0
; CHECK: ret i8
define i8 @wide_compare_boolean_extension(i1 %quit) minsize {
entry:
  br label %loop
loop:
  %i = phi i64 [ 0, %entry ], [ %inc, %loop ]
  %inc = add nsw i64 %i, 1
  %rem = srem i64 %inc, 20
  %cmp = icmp ne i64 %rem, 0
  %not = xor i1 %cmp, true
  %result = zext i1 %not to i8
  br i1 %quit, label %exit, label %loop
exit:
  ret i8 %result
}
