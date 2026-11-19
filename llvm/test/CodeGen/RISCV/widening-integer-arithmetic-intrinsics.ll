; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c,+zbb -passes=wia -verify-each -S < %s | FileCheck %s

declare i8 @llvm.abs.i8(i8, i1 immarg)
declare i8 @llvm.ctpop.i8(i8)
declare i8 @llvm.cttz.i8(i8, i1 immarg)
declare i8 @llvm.ctlz.i8(i8, i1 immarg)
declare void @llvm.assume(i1)

; abs needs sign-filled input, even when its result is zero-extended.
define i64 @abs_zext(ptr %p) {
; CHECK-LABEL: define i64 @abs_zext(
; CHECK:       [[X:%.*]] = load i8, ptr %p
; CHECK-NEXT:  [[WIDE:%.*]] = sext i8 [[X]] to i64
; CHECK-NEXT:  [[ABS:%.*]] = call i64 @llvm.abs.i64(i64 [[WIDE]], i1 false)
; CHECK-NEXT:  ret i64 [[ABS]]
entry:
  %x = load i8, ptr %p
  %abs = call i8 @llvm.abs.i8(i8 %x, i1 false)
  %ext = zext i8 %abs to i64
  ret i64 %ext
}

; abs.i8(-128, false) has bit pattern 0x80. Its zero-extension is 128.
define i64 @abs_min_zext() {
; CHECK-LABEL: define i64 @abs_min_zext(
; CHECK:       [[ABS:%.*]] = call i64 @llvm.abs.i64(i64 -128, i1 false)
; CHECK-NEXT:  ret i64 [[ABS]]
entry:
  %abs = call i8 @llvm.abs.i8(i8 -128, i1 false)
  %ext = zext i8 %abs to i64
  ret i64 %ext
}

; Its sign-extension is -128, so a widened abs returning 128 is invalid.
define i64 @abs_min_sext() {
; CHECK-LABEL: define i64 @abs_min_sext(
; CHECK:       [[ABS:%.*]] = call i8 @llvm.abs.i8(i8 -128, i1 false)
; CHECK-NEXT:  [[EXT:%.*]] = sext i8 [[ABS]] to i64
; CHECK-NEXT:  ret i64 [[EXT]]
entry:
  %abs = call i8 @llvm.abs.i8(i8 -128, i1 false)
  %ext = sext i8 %abs to i64
  ret i64 %ext
}

; With the poison flag, every defined narrow result is nonnegative.
; The rewriter may relax the flag to false after proving the widening legal.
define i64 @abs_poison_sext(ptr %p) {
; CHECK-LABEL: define i64 @abs_poison_sext(
; CHECK:       [[X:%.*]] = load i8, ptr %p
; CHECK-NEXT:  [[WIDE:%.*]] = sext i8 [[X]] to i64
; CHECK-NEXT:  [[ABS:%.*]] = call i64 @llvm.abs.i64(i64 [[WIDE]], i1 false)
; CHECK-NEXT:  ret i64 [[ABS]]
entry:
  %x = load i8, ptr %p
  %abs = call i8 @llvm.abs.i8(i8 %x, i1 true)
  %ext = sext i8 %abs to i64
  ret i64 %ext
}

; Extra upper bits must be zero to preserve the population count.
define i64 @ctpop_zext(ptr %p) {
; CHECK-LABEL: define i64 @ctpop_zext(
; CHECK:       [[X:%.*]] = load i8, ptr %p
; CHECK-NEXT:  [[WIDE:%.*]] = zext i8 [[X]] to i64
; CHECK-NEXT:  [[COUNT:%.*]] = call i64 @llvm.ctpop.i64(i64 [[WIDE]])
; CHECK-NEXT:  ret i64 [[COUNT]]
entry:
  %x = load i8, ptr %p
  %count = call i8 @llvm.ctpop.i8(i8 %x)
  %ext = zext i8 %count to i64
  ret i64 %ext
}

; Sign-extending 0x80 would add set bits and change the answer from 1.
define i64 @ctpop_high_bit() {
; CHECK-LABEL: define i64 @ctpop_high_bit(
; CHECK:       [[COUNT:%.*]] = call i64 @llvm.ctpop.i64(i64 128)
; CHECK-NEXT:  ret i64 [[COUNT]]
entry:
  %count = call i8 @llvm.ctpop.i8(i8 -128)
  %ext = zext i8 %count to i64
  ret i64 %ext
}

; Zero may occur, but the original result would be poison in that case.
define i64 @cttz_zero_poison(ptr %p) {
; CHECK-LABEL: define i64 @cttz_zero_poison(
; CHECK:       [[X:%.*]] = load i8, ptr %p
; CHECK-NEXT:  [[WIDE:%.*]] = zext i8 [[X]] to i64
; CHECK-NEXT:  [[COUNT:%.*]] = call i64 @llvm.cttz.i64(i64 [[WIDE]], i1 false)
; CHECK-NEXT:  ret i64 [[COUNT]]
entry:
  %x = load i8, ptr %p
  %count = call i8 @llvm.cttz.i8(i8 %x, i1 true)
  %ext = zext i8 %count to i64
  ret i64 %ext
}

; Setting bit 3 proves nonzero without making the count constant.
define i64 @cttz_known_nonzero(ptr %p) {
; CHECK-LABEL: define i64 @cttz_known_nonzero(
; CHECK:       [[X:%.*]] = load i8, ptr %p
; CHECK-NEXT:  [[WIDE:%.*]] = zext i8 [[X]] to i64
; CHECK-NEXT:  [[NONZERO:%.*]] = or i64 [[WIDE]], 8
; CHECK-NEXT:  [[COUNT:%.*]] = call i64 @llvm.cttz.i64(i64 [[NONZERO]], i1 false)
; CHECK-NEXT:  ret i64 [[COUNT]]
entry:
  %x = load i8, ptr %p
  %nonzero = or i8 %x, 8
  %count = call i8 @llvm.cttz.i8(i8 %nonzero, i1 false)
  %ext = zext i8 %count to i64
  ret i64 %ext
}

; The nonzero fact is available at the call through its SimplifyQuery context.
define i64 @cttz_assumed_nonzero(ptr %p) {
; CHECK-LABEL: define i64 @cttz_assumed_nonzero(
; CHECK:       [[X:%.*]] = load i8, ptr %p
; CHECK-NEXT:  [[NONZERO:%.*]] = icmp ne i8 [[X]], 0
; CHECK-NEXT:  call void @llvm.assume(i1 [[NONZERO]])
; CHECK-NEXT:  [[WIDE:%.*]] = zext i8 [[X]] to i64
; CHECK-NEXT:  [[COUNT:%.*]] = call i64 @llvm.cttz.i64(i64 [[WIDE]], i1 false)
; CHECK-NEXT:  ret i64 [[COUNT]]
entry:
  %x = load i8, ptr %p
  %nonzero = icmp ne i8 %x, 0
  call void @llvm.assume(i1 %nonzero)
  %count = call i8 @llvm.cttz.i8(i8 %x, i1 false)
  %ext = zext i8 %count to i64
  ret i64 %ext
}

; A defined zero input must still produce 8, not the promoted width.
define i64 @cttz_maybe_zero(ptr %p) {
; CHECK-LABEL: define i64 @cttz_maybe_zero(
; CHECK:       [[X:%.*]] = load i8, ptr %p
; CHECK-NEXT:  [[COUNT:%.*]] = call i8 @llvm.cttz.i8(i8 [[X]], i1 false)
; CHECK-NEXT:  [[EXT:%.*]] = zext i8 [[COUNT]] to i64
; CHECK-NEXT:  ret i64 [[EXT]]
entry:
  %x = load i8, ptr %p
  %count = call i8 @llvm.cttz.i8(i8 %x, i1 false)
  %ext = zext i8 %count to i64
  ret i64 %ext
}

; Nonnegative is insufficient: masking the sign bit still permits zero.
define i64 @cttz_nonnegative_maybe_zero(ptr %p) {
; CHECK-LABEL: define i64 @cttz_nonnegative_maybe_zero(
; CHECK:       [[X:%.*]] = load i8, ptr %p
; CHECK-NEXT:  [[NONNEG:%.*]] = and i8 [[X]], 127
; CHECK-NEXT:  [[COUNT:%.*]] = call i8 @llvm.cttz.i8(i8 [[NONNEG]], i1 false)
; CHECK-NEXT:  [[EXT:%.*]] = zext i8 [[COUNT]] to i64
; CHECK-NEXT:  ret i64 [[EXT]]
entry:
  %x = load i8, ptr %p
  %nonnegative = and i8 %x, 127
  %count = call i8 @llvm.cttz.i8(i8 %nonnegative, i1 false)
  %ext = zext i8 %count to i64
  ret i64 %ext
}

define i64 @cttz_zero() {
; CHECK-LABEL: define i64 @cttz_zero(
; CHECK:       [[COUNT:%.*]] = call i8 @llvm.cttz.i8(i8 0, i1 false)
; CHECK-NEXT:  [[EXT:%.*]] = zext i8 [[COUNT]] to i64
; CHECK-NEXT:  ret i64 [[EXT]]
entry:
  %count = call i8 @llvm.cttz.i8(i8 0, i1 false)
  %ext = zext i8 %count to i64
  ret i64 %ext
}

; Both the select condition and the intrinsic flag must remain i1.
define i64 @cttz_select(i1 %cond) {
; CHECK-LABEL: define i64 @cttz_select(
; CHECK:       [[SELECT:%.*]] = select i1 %cond, i64 8, i64 16
; CHECK-NEXT:  [[COUNT:%.*]] = call i64 @llvm.cttz.i64(i64 [[SELECT]], i1 false)
; CHECK-NEXT:  ret i64 [[COUNT]]
entry:
  %selected = select i1 %cond, i8 8, i8 16
  %count = call i8 @llvm.cttz.i8(i8 %selected, i1 true)
  %ext = zext i8 %count to i64
  ret i64 %ext
}

; ctlz needs a result correction when widened and remains unsupported.
define i64 @ctlz_not_promoted(ptr %p) {
; CHECK-LABEL: define i64 @ctlz_not_promoted(
; CHECK:       [[X:%.*]] = load i8, ptr %p
; CHECK-NEXT:  [[COUNT:%.*]] = call i8 @llvm.ctlz.i8(i8 [[X]], i1 false)
; CHECK-NEXT:  [[EXT:%.*]] = zext i8 [[COUNT]] to i64
; CHECK-NEXT:  ret i64 [[EXT]]
entry:
  %x = load i8, ptr %p
  %count = call i8 @llvm.ctlz.i8(i8 %x, i1 false)
  %ext = zext i8 %count to i64
  ret i64 %ext
}
