; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c -passes=wia,verify -S < %s | FileCheck %s
; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c -passes=wia,verify -S < %s | llc -mtriple=riscv64 -mattr=+m,+a,+c -O2 -disable-wia -o - | FileCheck %s --check-prefix=ASM

target triple = "riscv64"

declare void @consume(i32 signext)
declare void @consume16(i16 signext)

; Use i16 so this tests a profitable signed repair rather than a free
; i32-to-i64 extension after RV64 word arithmetic. The SIGN proof permits
; trunc nsw at the call boundary; codegen should reuse the wide sum.
; CHECK-LABEL: define i64 @signed_add_repair(
; CHECK:       entry:
; CHECK-NEXT:  [[A:%.*]] = sext i16 %a to i64
; CHECK-NEXT:  [[V:%.*]] = add nsw i64 [[A]], 1
; CHECK-NEXT:  [[REPAIR:%.*]] = trunc nsw i64 [[V]] to i16
; CHECK-NEXT:  call void @consume16(i16 signext [[REPAIR]])
; CHECK-NEXT:  ret i64 [[V]]
; ASM-LABEL: signed_add_repair:
; ASM:       addi [[SUM:s[0-9]+]], a0, 1
; ASM-NEXT:  mv a0, [[SUM]]
; ASM-NEXT:  call consume16
define i64 @signed_add_repair(i16 signext %a) minsize {
entry:
  %v = add nsw i16 %a, 1
  call void @consume16(i16 signext %v)
  %w = sext i16 %v to i64
  ret i64 %w
}

; No arithmetic nsw flag exists to copy here. XOR of sign-extended operands
; produces a sign-extended result, which independently justifies trunc nsw.
; CHECK-LABEL: define i64 @signed_bits_repair(
; CHECK:       entry:
; CHECK-NEXT:  [[A:%.*]] = sext i32 %a to i64
; CHECK-NEXT:  [[B:%.*]] = sext i32 %b to i64
; CHECK-NEXT:  [[V:%.*]] = xor i64 [[A]], [[B]]
; CHECK-NEXT:  [[REPAIR:%.*]] = trunc nsw i64 [[V]] to i32
; CHECK-NEXT:  call void @consume(i32 signext [[REPAIR]])
; CHECK-NEXT:  ret i64 [[V]]
define i64 @signed_bits_repair(i32 signext %a, i32 signext %b) minsize {
entry:
  %v = xor i32 %a, %b
  call void @consume(i32 signext %v)
  %w = sext i32 %v to i64
  ret i64 %w
}

; The repair belongs after the select. With cond=false, overflowing narrow
; arithmetic is masked and the original program still passes and returns 5.
; CHECK-LABEL: define i64 @masked_overflow_repair(
; CHECK:       entry:
; CHECK-NEXT:  [[A:%.*]] = sext i32 %a to i64
; CHECK-NEXT:  [[B:%.*]] = sext i32 %b to i64
; CHECK-NEXT:  [[SUM:%.*]] = add nsw i64 [[A]], [[B]]
; CHECK-NEXT:  [[V:%.*]] = select i1 %cond, i64 [[SUM]], i64 5
; CHECK-NEXT:  [[REPAIR:%.*]] = trunc nsw i64 [[V]] to i32
; CHECK-NEXT:  call void @consume(i32 signext [[REPAIR]])
; CHECK-NEXT:  ret i64 [[V]]
define i64 @masked_overflow_repair(i1 %cond, i32 signext %a,
                                 i32 signext %b) minsize {
entry:
  %sum = add nsw i32 %a, %b
  %v = select i1 %cond, i32 %sum, i32 5
  call void @consume(i32 signext %v)
  %w = sext i32 %v to i64
  ret i64 %w
}

; This graph is promoted with ZEROS fill. Multiple extension exits make it
; profitable despite the call repair. Every result has bit 31 set, so trunc
; nsw would introduce poison: for a=0, wide v=2147483648 does not fit signed i32.
; The signext call attribute alone does not justify the flag.
; CHECK-LABEL: define i64 @unsigned_repair(
; CHECK:       entry:
; CHECK-NEXT:  [[A:%.*]] = load i32, ptr %p, align 4
; CHECK-NEXT:  [[EXT:%.*]] = zext i32 [[A]] to i64
; CHECK-NEXT:  [[V:%.*]] = or i64 [[EXT]], 2147483648
; CHECK-NEXT:  store i64 [[V]], ptr %out1, align 8
; CHECK-NEXT:  store i64 [[V]], ptr %out2, align 8
; CHECK-NEXT:  [[REPAIR:%.*]] = trunc i64 [[V]] to i32
; CHECK-NEXT:  call void @consume(i32 signext [[REPAIR]])
; CHECK-NEXT:  ret i64 [[V]]
define i64 @unsigned_repair(ptr %p, ptr %out1, ptr %out2) minsize {
entry:
  %a = load i32, ptr %p, align 4
  %v = or i32 %a, -2147483648
  %w1 = zext i32 %v to i64
  store i64 %w1, ptr %out1, align 8
  %w2 = zext i32 %v to i64
  store i64 %w2, ptr %out2, align 8
  call void @consume(i32 signext %v)
  %w = zext i32 %v to i64
  ret i64 %w
}

; Without a no-wrap proof, the signed graph must not widen this addition.
; For a=INT_MAX and b=1 the original result is defined and equals INT_MIN.
; A wide sum followed by trunc nsw would instead make the call argument poison.
; CHECK-LABEL: define i64 @wrapping_add(
; CHECK:       entry:
; CHECK-NEXT:  [[V:%.*]] = add i32 %a, %b
; CHECK-NEXT:  call void @consume(i32 signext [[V]])
; CHECK-NEXT:  [[W:%.*]] = sext i32 [[V]] to i64
; CHECK-NEXT:  ret i64 [[W]]
define i64 @wrapping_add(i32 signext %a, i32 signext %b) minsize {
entry:
  %v = add i32 %a, %b
  call void @consume(i32 signext %v)
  %w = sext i32 %v to i64
  ret i64 %w
}
