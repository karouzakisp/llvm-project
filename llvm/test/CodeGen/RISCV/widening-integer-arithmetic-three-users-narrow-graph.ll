; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c -passes=wia -S < %s | \
; RUN:   llc -mtriple=riscv64 -mattr=+m,+a,+c -verify-machineinstrs | FileCheck %s

; CHECK-LABEL: three_users_narrow_graph:
; CHECK-NOT: slliw
; CHECK: slli
; CHECK-NOT: slliw
; CHECK: ret
define i8 @three_users_narrow_graph(i8 %x, i8 %y, ptr %p) {
entry:
  %ext = zext i8 %x to i32

  ; User 1 of %ext: low-bit-closed
  %u1 = add i32 %ext, 300

  ; User 2 of %ext: low-bit-closed
  %u2 = xor i32 %ext, 170

  ; User 3 of %ext: low-bit-closed
  %u3 = shl i32 %ext, 1

  ; Keep all users alive, but only demand low 8 bits.
  %t1 = trunc i32 %u1 to i8
  %t2 = trunc i32 %u2 to i8
  %t3 = trunc i32 %u3 to i8

  %a = add i8 %t1, %t2
  %b = add i8 %a, %t3
  ret i8 %b
}

; CHECK-LABEL: wide_user_blocks_narrow_graph:
; CHECK: zext.b
; CHECK-NEXT: slli
; CHECK: ret
define i32 @wide_user_blocks_narrow_graph(i8 %x) {
entry:
  %ext = zext i8 %x to i32
  %u = shl i32 %ext, 1
  ret i32 %u
}
