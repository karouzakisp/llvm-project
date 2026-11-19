; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c -passes=wia -S < %s | FileCheck %s

define i32 @select_i8_load_load(ptr %p, ptr %q, i1 zeroext %c) {
; CHECK-LABEL: define i32 @select_i8_load_load(
; CHECK:   [[A:%.*]] = load i8, ptr %p, align 1
; CHECK:   [[B:%.*]] = load i8, ptr %q, align 1
; CHECK:   [[S:%.*]] = select i1 %c, i8 [[A]], i8 [[B]]
; CHECK:   [[Z:%.*]] = zext i8 [[S]] to i32
; CHECK:   ret i32 [[Z]]
entry:
  %a = load i8, ptr %p, align 1
  %b = load i8, ptr %q, align 1
  %s = select i1 %c, i8 %a, i8 %b
  %z = zext i8 %s to i32
  ret i32 %z
}

define i32 @select_i8_load_zero(ptr %p, i1 zeroext %c) {
; CHECK-LABEL: define i32 @select_i8_load_zero(
; CHECK:   [[A:%.*]] = load i8, ptr %p, align 1
; CHECK:   [[S:%.*]] = select i1 %c, i8 [[A]], i8 0
; CHECK:   [[Z:%.*]] = zext i8 [[S]] to i32
; CHECK:   ret i32 [[Z]]
entry:
  %a = load i8, ptr %p, align 1
  %s = select i1 %c, i8 %a, i8 0
  %z = zext i8 %s to i32
  ret i32 %z
}
