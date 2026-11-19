; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c,+zba,+zbb -passes=wia -verify-each -S %s | FileCheck %s

; Narrowing %wide's users replaces %cmp. The later plan for %result
; still references the original comparison as an external operand.
; Although %mid survives, its operand changes. Revalidate that the source
; can still be materialized before applying the later plan.
;
; Require the first narrowing to happen and the result to remain valid.
; Allow the boolean extensions to remain separate or be combined.
; CHECK-LABEL: define i64 @sqlite_boolean_chain(
; CHECK:       [[BIT:%.*]] = and i16 %flags, 1
; CHECK-NEXT:  [[CMP:%.*]] = icmp eq i16 [[BIT]], 0
; CHECK:       [[RESULT:%.*]] = zext {{.*}} to i64
; CHECK-NEXT:  ret i64 [[RESULT]]
define i64 @sqlite_boolean_chain(i16 %flags) minsize {
entry:
  %wide = zext i16 %flags to i32
  %bit = and i32 %wide, 1
  %cmp = icmp eq i32 %bit, 0
  %mid = zext i1 %cmp to i32
  %result = zext nneg i32 %mid to i64
  ret i64 %result
}
