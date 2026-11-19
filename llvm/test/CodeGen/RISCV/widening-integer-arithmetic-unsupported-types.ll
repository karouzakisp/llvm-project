; RUN: opt -mtriple=riscv64 -passes=wia,verify -S < %s | FileCheck %s

; WIA only supports scalar integer widths that are powers of two up to 64.
; Reject other cast roots before entering the planner's type-specific helpers.

@bytes = private global [16 x i8] zeroinitializer

; This used to assert while computing the maximum signed source value for a
; possible sign-filled GEP index, before rejecting the unsupported width.
define i8 @wide_gep(ptr %src) minsize {
; CHECK-LABEL: define i8 @wide_gep(
; CHECK:       %x = load i128, ptr %src
; CHECK-NEXT:  %ext = zext i128 %x to i256
; CHECK-NEXT:  %p = getelementptr inbounds [16 x i8], ptr @bytes, i64 0, i256 %ext
; CHECK-NEXT:  %r = load i8, ptr %p
; CHECK-NEXT:  ret i8 %r
  %x = load i128, ptr %src
  %ext = zext i128 %x to i256
  %p = getelementptr inbounds [16 x i8], ptr @bytes, i64 0, i256 %ext
  %r = load i8, ptr %p
  ret i8 %r
}

define i7 @odd_source(i7 %x) minsize {
; CHECK-LABEL: define i7 @odd_source(
; CHECK:       %ext = zext i7 %x to i64
; CHECK-NEXT:  %add = add i64 %ext, 1
; CHECK-NEXT:  %ret = trunc i64 %add to i7
; CHECK-NEXT:  ret i7 %ret
  %ext = zext i7 %x to i64
  %add = add i64 %ext, 1
  %ret = trunc i64 %add to i7
  ret i7 %ret
}

define i8 @odd_destination(i8 %x) minsize {
; CHECK-LABEL: define i8 @odd_destination(
; CHECK:       %ext = sext i8 %x to i24
; CHECK-NEXT:  %add = add i24 %ext, 1
; CHECK-NEXT:  %ret = trunc i24 %add to i8
; CHECK-NEXT:  ret i8 %ret
  %ext = sext i8 %x to i24
  %add = add i24 %ext, 1
  %ret = trunc i24 %add to i8
  ret i8 %ret
}

define i8 @oversized_destination(i8 %x) minsize {
; CHECK-LABEL: define i8 @oversized_destination(
; CHECK:       %ext = sext i8 %x to i128
; CHECK-NEXT:  %add = add i128 %ext, 1
; CHECK-NEXT:  %ret = trunc i128 %add to i8
; CHECK-NEXT:  ret i8 %ret
  %ext = sext i8 %x to i128
  %add = add i128 %ext, 1
  %ret = trunc i128 %add to i8
  ret i8 %ret
}

define i64 @odd_trunc_source(i24 %x) minsize {
; CHECK-LABEL: define i64 @odd_trunc_source(
; CHECK:       %tr = trunc i24 %x to i8
; CHECK-NEXT:  %add = add i8 %tr, 1
; CHECK-NEXT:  %ext = zext i8 %add to i64
; CHECK-NEXT:  ret i64 %ext
  %tr = trunc i24 %x to i8
  %add = add i8 %tr, 1
  %ext = zext i8 %add to i64
  ret i64 %ext
}

define i64 @odd_trunc_destination(i64 %x) minsize {
; CHECK-LABEL: define i64 @odd_trunc_destination(
; CHECK:       %tr = trunc i64 %x to i7
; CHECK-NEXT:  %bits = or i7 %tr, 1
; CHECK-NEXT:  %ext = zext i7 %bits to i64
; CHECK-NEXT:  ret i64 %ext
  %tr = trunc i64 %x to i7
  %bits = or i7 %tr, 1
  %ext = zext i7 %bits to i64
  ret i64 %ext
}

define <2 x i8> @fixed_vector(<2 x i8> %x) minsize {
; CHECK-LABEL: define <2 x i8> @fixed_vector(
; CHECK:       %ext = zext <2 x i8> %x to <2 x i16>
; CHECK-NEXT:  %add = add <2 x i16> %ext, %ext
; CHECK-NEXT:  %ret = trunc <2 x i16> %add to <2 x i8>
; CHECK-NEXT:  ret <2 x i8> %ret
  %ext = zext <2 x i8> %x to <2 x i16>
  %add = add <2 x i16> %ext, %ext
  %ret = trunc <2 x i16> %add to <2 x i8>
  ret <2 x i8> %ret
}

define <vscale x 2 x i8> @scalable_vector(<vscale x 2 x i8> %x) minsize {
; CHECK-LABEL: define <vscale x 2 x i8> @scalable_vector(
; CHECK:       %ext = sext <vscale x 2 x i8> %x to <vscale x 2 x i16>
; CHECK-NEXT:  %add = add <vscale x 2 x i16> %ext, %ext
; CHECK-NEXT:  %ret = trunc <vscale x 2 x i16> %add to <vscale x 2 x i8>
; CHECK-NEXT:  ret <vscale x 2 x i8> %ret
  %ext = sext <vscale x 2 x i8> %x to <vscale x 2 x i16>
  %add = add <vscale x 2 x i16> %ext, %ext
  %ret = trunc <vscale x 2 x i16> %add to <vscale x 2 x i8>
  ret <vscale x 2 x i8> %ret
}

; Supported cast roots still allow the arithmetic graph to narrow.
define i8 @supported_scalar(i8 %x) minsize {
; CHECK-LABEL: define i8 @supported_scalar(
; CHECK:       %add = add i8 %x, 1
; CHECK-NEXT:  ret i8 %add
  %ext = zext i8 %x to i64
  %add = add i64 %ext, 1
  %ret = trunc i64 %add to i8
  ret i8 %ret
}
