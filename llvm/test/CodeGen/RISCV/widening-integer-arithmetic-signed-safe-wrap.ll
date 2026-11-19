; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c -passes=wia,verify -S < %s | FileCheck %s

define i64 @safe_wrap_signed_add_i8(ptr %p) {
; CHECK-LABEL: define i64 @safe_wrap_signed_add_i8(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %a = load i8, ptr %p, align 1
; CHECK-NEXT:    %0 = sext i8 %a to i64
; CHECK-NEXT:    %x = add nsw i64 %0, 0
; CHECK-NEXT:    %wrap = add i64 %x, 79
; CHECK-NEXT:    %cmp = icmp ult i64 %wrap, 131
; CHECK-NEXT:    %cmp.ext = zext i1 %cmp to i64
; CHECK-NEXT:    %r = add i64 %x, %cmp.ext
; CHECK-NEXT:    ret i64 %r
;
entry:
  %a = load i8, ptr %p, align 1
  %x = add nsw i8 %a, 0
  %x.ext = sext i8 %x to i64
  %wrap = add i8 %x, -49
  %cmp = icmp slt i8 %wrap, 3
  %cmp.ext = zext i1 %cmp to i64
  %r = add i64 %x.ext, %cmp.ext
  ret i64 %r
}

define i64 @safe_wrap_signed_sub_i8(ptr %p) {
; CHECK-LABEL: define i64 @safe_wrap_signed_sub_i8(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %a = load i8, ptr %p, align 1
; CHECK-NEXT:    %0 = sext i8 %a to i64
; CHECK-NEXT:    %x = add nsw i64 %0, 0
; CHECK-NEXT:    %wrap = sub i64 %x, -79
; CHECK-NEXT:    %cmp = icmp ult i64 %wrap, 131
; CHECK-NEXT:    %cmp.ext = zext i1 %cmp to i64
; CHECK-NEXT:    %r = add i64 %x, %cmp.ext
; CHECK-NEXT:    ret i64 %r
;
entry:
  %a = load i8, ptr %p, align 1
  %x = add nsw i8 %a, 0
  %x.ext = sext i8 %x to i64
  %wrap = sub i8 %x, 49
  %cmp = icmp slt i8 %wrap, 3
  %cmp.ext = zext i1 %cmp to i64
  %r = add i64 %x.ext, %cmp.ext
  ret i64 %r
}

define i64 @safe_wrap_signed_add_i8_sle(ptr %p) {
; CHECK-LABEL: define i64 @safe_wrap_signed_add_i8_sle(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %a = load i8, ptr %p, align 1
; CHECK-NEXT:    %0 = sext i8 %a to i64
; CHECK-NEXT:    %x = add nsw i64 %0, 0
; CHECK-NEXT:    %wrap = add i64 %x, 79
; CHECK-NEXT:    %cmp = icmp ule i64 %wrap, 131
; CHECK-NEXT:    %cmp.ext = zext i1 %cmp to i64
; CHECK-NEXT:    %r = add i64 %x, %cmp.ext
; CHECK-NEXT:    ret i64 %r
;
entry:
  %a = load i8, ptr %p, align 1
  %x = add nsw i8 %a, 0
  %x.ext = sext i8 %x to i64
  %wrap = add i8 %x, -49
  %cmp = icmp sle i8 %wrap, 3
  %cmp.ext = zext i1 %cmp to i64
  %r = add i64 %x.ext, %cmp.ext
  ret i64 %r
}

define i64 @safe_wrap_signed_sub_i8_sgt(ptr %p) {
; CHECK-LABEL: define i64 @safe_wrap_signed_sub_i8_sgt(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %a = load i8, ptr %p, align 1
; CHECK-NEXT:    %0 = sext i8 %a to i64
; CHECK-NEXT:    %x = add nsw i64 %0, 0
; CHECK-NEXT:    %wrap = sub i64 %x, -79
; CHECK-NEXT:    %cmp = icmp ugt i64 %wrap, 131
; CHECK-NEXT:    %cmp.ext = zext i1 %cmp to i64
; CHECK-NEXT:    %r = add i64 %x, %cmp.ext
; CHECK-NEXT:    ret i64 %r
;
entry:
  %a = load i8, ptr %p, align 1
  %x = add nsw i8 %a, 0
  %x.ext = sext i8 %x to i64
  %wrap = sub i8 %x, 49
  %cmp = icmp sgt i8 %wrap, 3
  %cmp.ext = zext i1 %cmp to i64
  %r = add i64 %x.ext, %cmp.ext
  ret i64 %r
}

define i64 @safe_wrap_signed_add_i8_cut(ptr %p) {
; CHECK-LABEL: define i64 @safe_wrap_signed_add_i8_cut(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %a = load i8, ptr %p, align 1
; CHECK-NEXT:    %0 = sext i8 %a to i64
; CHECK-NEXT:    %x = add nsw i64 %0, 0
; CHECK-NEXT:    %wrap = add i64 %x, -28
; CHECK-NEXT:    %cmp = icmp ult i64 %wrap, -125
; CHECK-NEXT:    %cmp.ext = zext i1 %cmp to i64
; CHECK-NEXT:    %r = add i64 %x, %cmp.ext
; CHECK-NEXT:    ret i64 %r
;
entry:
  %a = load i8, ptr %p, align 1
  %x = add nsw i8 %a, 0
  %x.ext = sext i8 %x to i64
  %wrap = add i8 %x, 100
  %cmp = icmp slt i8 %wrap, 3
  %cmp.ext = zext i1 %cmp to i64
  %r = add i64 %x.ext, %cmp.ext
  ret i64 %r
}

define i64 @safe_wrap_signed_add_i8_sge_cut(ptr %p) {
; CHECK-LABEL: define i64 @safe_wrap_signed_add_i8_sge_cut(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %a = load i8, ptr %p, align 1
; CHECK-NEXT:    %0 = sext i8 %a to i64
; CHECK-NEXT:    %x = add nsw i64 %0, 0
; CHECK-NEXT:    %wrap = add i64 %x, -118
; CHECK-NEXT:    %cmp = icmp uge i64 %wrap, -133
; CHECK-NEXT:    %cmp.ext = zext i1 %cmp to i64
; CHECK-NEXT:    %r = add i64 %x, %cmp.ext
; CHECK-NEXT:    ret i64 %r
;
entry:
  %a = load i8, ptr %p, align 1
  %x = add nsw i8 %a, 0
  %x.ext = sext i8 %x to i64
  %wrap = add i8 %x, 10
  %cmp = icmp sge i8 %wrap, -5
  %cmp.ext = zext i1 %cmp to i64
  %r = add i64 %x.ext, %cmp.ext
  ret i64 %r
}

define i64 @safe_wrap_signed_sub_i8_cut(ptr %p) {
; CHECK-LABEL: define i64 @safe_wrap_signed_sub_i8_cut(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %a = load i8, ptr %p, align 1
; CHECK-NEXT:    %0 = sext i8 %a to i64
; CHECK-NEXT:    %x = add nsw i64 %0, 0
; CHECK-NEXT:    %wrap = sub i64 %x, 28
; CHECK-NEXT:    %cmp = icmp ult i64 %wrap, -125
; CHECK-NEXT:    %cmp.ext = zext i1 %cmp to i64
; CHECK-NEXT:    %r = add i64 %x, %cmp.ext
; CHECK-NEXT:    ret i64 %r
;
entry:
  %a = load i8, ptr %p, align 1
  %x = add nsw i8 %a, 0
  %x.ext = sext i8 %x to i64
  %wrap = sub i8 %x, -100
  %cmp = icmp slt i8 %wrap, 3
  %cmp.ext = zext i1 %cmp to i64
  %r = add i64 %x.ext, %cmp.ext
  ret i64 %r
}

define i64 @safe_wrap_signed_sub_i8_min_const(ptr %p) {
; CHECK-LABEL: define i64 @safe_wrap_signed_sub_i8_min_const(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %a = load i8, ptr %p, align 1
; CHECK-NEXT:    %0 = sext i8 %a to i64
; CHECK-NEXT:    %x = add nsw i64 %0, 0
; CHECK-NEXT:    %wrap = sub i64 %x, 0
; CHECK-NEXT:    %cmp = icmp uge i64 %wrap, -128
; CHECK-NEXT:    %cmp.ext = zext i1 %cmp to i64
; CHECK-NEXT:    %r = add i64 %x, %cmp.ext
; CHECK-NEXT:    ret i64 %r
;
entry:
  %a = load i8, ptr %p, align 1
  %x = add nsw i8 %a, 0
  %x.ext = sext i8 %x to i64
  %wrap = sub i8 %x, -128
  %cmp = icmp sge i8 %wrap, 0
  %cmp.ext = zext i1 %cmp to i64
  %r = add i64 %x.ext, %cmp.ext
  ret i64 %r
}

define i64 @safe_wrap_signed_add_i16(ptr %p) {
; CHECK-LABEL: define i64 @safe_wrap_signed_add_i16(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %a = load i16, ptr %p, align 2
; CHECK-NEXT:    %0 = sext i16 %a to i64
; CHECK-NEXT:    %x = add nsw i64 %0, 0
; CHECK-NEXT:    %wrap = add i64 %x, 0
; CHECK-NEXT:    %cmp = icmp ult i64 %wrap, 8
; CHECK-NEXT:    %cmp.ext = zext i1 %cmp to i64
; CHECK-NEXT:    %r = add i64 %x, %cmp.ext
; CHECK-NEXT:    ret i64 %r
;
entry:
  %a = load i16, ptr %p, align 2
  %x = add nsw i16 %a, 0
  %x.ext = sext i16 %x to i64
  %wrap = add i16 %x, -32768
  %cmp = icmp slt i16 %wrap, -32760
  %cmp.ext = zext i1 %cmp to i64
  %r = add i64 %x.ext, %cmp.ext
  ret i64 %r
}

define i64 @safe_wrap_signed_sub_i16(ptr %p) {
; CHECK-LABEL: define i64 @safe_wrap_signed_sub_i16(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %a = load i16, ptr %p, align 2
; CHECK-NEXT:    %0 = sext i16 %a to i64
; CHECK-NEXT:    %x = add nsw i64 %0, 0
; CHECK-NEXT:    %wrap = sub i64 %x, 0
; CHECK-NEXT:    %cmp = icmp ult i64 %wrap, 8
; CHECK-NEXT:    %cmp.ext = zext i1 %cmp to i64
; CHECK-NEXT:    %r = add i64 %x, %cmp.ext
; CHECK-NEXT:    ret i64 %r
;
entry:
  %a = load i16, ptr %p, align 2
  %x = add nsw i16 %a, 0
  %x.ext = sext i16 %x to i64
  %wrap = sub i16 %x, -32768
  %cmp = icmp slt i16 %wrap, -32760
  %cmp.ext = zext i1 %cmp to i64
  %r = add i64 %x.ext, %cmp.ext
  ret i64 %r
}

define i64 @safe_wrap_signed_add_i16_cut(ptr %p) {
; CHECK-LABEL: define i64 @safe_wrap_signed_add_i16_cut(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %a = load i16, ptr %p, align 2
; CHECK-NEXT:    %0 = sext i16 %a to i64
; CHECK-NEXT:    %x = add nsw i64 %0, 0
; CHECK-NEXT:    %wrap = add i64 %x, 8
; CHECK-NEXT:    %cmp = icmp ult i64 %wrap, -1
; CHECK-NEXT:    %cmp.ext = zext i1 %cmp to i64
; CHECK-NEXT:    %r = add i64 %x, %cmp.ext
; CHECK-NEXT:    ret i64 %r
;
entry:
  %a = load i16, ptr %p, align 2
  %x = add nsw i16 %a, 0
  %x.ext = sext i16 %x to i64
  %wrap = add i16 %x, -32760
  %cmp = icmp slt i16 %wrap, 32767
  %cmp.ext = zext i1 %cmp to i64
  %r = add i64 %x.ext, %cmp.ext
  ret i64 %r
}

define i64 @no_safe_wrap_zext_root(ptr %p) {
; CHECK-LABEL: define i64 @no_safe_wrap_zext_root(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %a = load i8, ptr %p, align 1
; CHECK-NEXT:    %x = add nuw i8 %a, 0
; CHECK-NEXT:    %x.ext = zext i8 %x to i64
; CHECK-NEXT:    %wrap = add i8 %x, -49
; CHECK-NEXT:    %cmp = icmp slt i8 %wrap, 3
; CHECK-NEXT:    %cmp.ext = zext i1 %cmp to i64
; CHECK-NEXT:    %r = add i64 %x.ext, %cmp.ext
; CHECK-NEXT:    ret i64 %r
;
entry:
  %a = load i8, ptr %p, align 1
  %x = add nuw i8 %a, 0
  %x.ext = zext i8 %x to i64
  %wrap = add i8 %x, -49
  %cmp = icmp slt i8 %wrap, 3
  %cmp.ext = zext i1 %cmp to i64
  %r = add i64 %x.ext, %cmp.ext
  ret i64 %r
}

define i64 @safe_wrap_signed_add_i8_zext_trunc_nsw_source(i64 %a) {
; CHECK-LABEL: define i64 @safe_wrap_signed_add_i8_zext_trunc_nsw_source(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %wide = add nsw i64 %a, 0
; CHECK-NEXT:    %0 = and i64 %wide, 255
; CHECK-NEXT:    %wrap = add i64 %wide, 79
; CHECK-NEXT:    %cmp = icmp ult i64 %wrap, 131
; CHECK-NEXT:    %cmp.ext = zext i1 %cmp to i64
; CHECK-NEXT:    %r = add i64 %0, %cmp.ext
; CHECK-NEXT:    ret i64 %r
;
entry:
  %wide = add nsw i64 %a, 0
  %x = trunc nsw i64 %wide to i8
  %x.ext = zext i8 %x to i64
  %wrap = add i8 %x, -49
  %cmp = icmp slt i8 %wrap, 3
  %cmp.ext = zext i1 %cmp to i64
  %r = add i64 %x.ext, %cmp.ext
  ret i64 %r
}
