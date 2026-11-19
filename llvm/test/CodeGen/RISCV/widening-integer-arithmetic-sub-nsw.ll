; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c -passes=wia,verify -S < %s | FileCheck %s
; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c -passes=wia,wia,verify -S < %s | FileCheck %s

target triple = "riscv64"

; Subtracting two sign-extended i16 values fits signed i17, so the
; promoted i64 subtraction can retain nsw.
; CHECK-LABEL: define i64 @signed_sub_loads(
; CHECK: [[A:%.*]] = load i16, ptr %p
; CHECK: [[B:%.*]] = load i16, ptr %q
; CHECK: [[WA:%.*]] = sext i16 [[A]] to i64
; CHECK: [[WB:%.*]] = sext i16 [[B]] to i64
; CHECK: [[V:%.*]] = sub nsw i64 [[WA]], [[WB]]
; CHECK-NEXT: ret i64 [[V]]
define i64 @signed_sub_loads(ptr %p, ptr %q) minsize {
  %a = load i16, ptr %p, align 2
  %b = load i16, ptr %q, align 2
  %v = sub nsw i16 %a, %b
  %r = sext i16 %v to i64
  ret i64 %r
}

; Cover a constant in the left operand as well.
; CHECK-LABEL: define i64 @signed_sub_constant(
; CHECK: [[A:%.*]] = load i16, ptr %p
; CHECK: [[WA:%.*]] = sext i16 [[A]] to i64
; CHECK: [[V:%.*]] = sub nsw i64 7, [[WA]]
; CHECK-NEXT: ret i64 [[V]]
define i64 @signed_sub_constant(ptr %p) minsize {
  %a = load i16, ptr %p, align 2
  %v = sub nsw i16 7, %a
  %r = sext i16 %v to i64
  ret i64 %r
}
