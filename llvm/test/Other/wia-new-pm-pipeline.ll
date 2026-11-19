; REQUIRES: x86-registered-target
;
; Automatic new-PM WIA runs early on RV64 and only transforms minsize functions.
; It is absent on RV32, x86, and AArch64, at O0, and when disabled.
; Explicit invocation remains supported, including functions without minsize.
;
; RUN: opt -mtriple=x86_64-unknown-linux-gnu -passes='default<O2>' \
; RUN:   -debug-pass-manager -disable-output < %s 2>&1 | \
; RUN:   FileCheck %s --check-prefix=ORDER --implicit-check-not=WIAPass
; RUN: opt -mtriple=x86_64-unknown-linux-gnu -passes='lto<O2>' \
; RUN:   -debug-pass-manager -disable-output < %s 2>&1 | \
; RUN:   FileCheck %s --check-prefix=DEFAULT --implicit-check-not=WIAPass
; RUN: opt -mtriple=x86_64-unknown-linux-gnu -disable-wia \
; RUN:   -passes='default<O2>' -debug-pass-manager -disable-output < %s 2>&1 | \
; RUN:   FileCheck %s --check-prefix=DISABLED --implicit-check-not=WIAPass
; RUN: opt -mtriple=x86_64-unknown-linux-gnu -passes=wia,verify \
; RUN:   -debug-pass-manager -disable-output < %s 2>&1 | \
; RUN:   FileCheck %s --check-prefix=EXPLICIT
;
; RUN: %if riscv-registered-target %{ opt -mtriple=riscv64 -passes='default<O2>' \
; RUN:   -debug-pass-manager -disable-output < %s 2>&1 | \
; RUN:   FileCheck %s --check-prefix=EARLY %}
; RUN: %if riscv-registered-target %{ opt -mtriple=riscv32 -passes='default<O2>' \
; RUN:   -debug-pass-manager -disable-output < %s 2>&1 | \
; RUN:   FileCheck %s --check-prefix=NO-WIA --implicit-check-not=WIAPass %}
; RUN: %if riscv-registered-target %{ opt -mtriple=riscv64 -disable-wia \
; RUN:   -passes='default<O2>' -debug-pass-manager -disable-output < %s 2>&1 | \
; RUN:   FileCheck %s --check-prefix=NO-WIA --implicit-check-not=WIAPass %}
; RUN: %if riscv-registered-target %{ opt -mtriple=riscv64 -passes='default<O0>' \
; RUN:   -debug-pass-manager -disable-output < %s 2>&1 | \
; RUN:   FileCheck %s --check-prefix=NO-WIA --implicit-check-not=WIAPass %}
; RUN: %if aarch64-registered-target %{ opt -mtriple=aarch64 -passes='default<O2>' \
; RUN:   -debug-pass-manager -disable-output < %s 2>&1 | \
; RUN:   FileCheck %s --check-prefix=NO-WIA --implicit-check-not=WIAPass %}
; RUN: %if aarch64-registered-target %{ opt -mtriple=aarch64 -disable-wia \
; RUN:   -passes='default<O2>' -debug-pass-manager -disable-output < %s 2>&1 | \
; RUN:   FileCheck %s --check-prefix=NO-WIA --implicit-check-not=WIAPass %}
; RUN: %if aarch64-registered-target %{ opt -mtriple=aarch64 -passes='default<O0>' \
; RUN:   -debug-pass-manager -disable-output < %s 2>&1 | \
; RUN:   FileCheck %s --check-prefix=NO-WIA --implicit-check-not=WIAPass %}
;
; Analyses may run between these consecutive transformation passes.
; EARLY: Running pass: PromotePass on f
; EARLY-NOT: Running pass:
; EARLY: Running pass: WIAPass on f
; EARLY-NOT: Running pass:
; EARLY: Running pass: InstCombinePass on f
; NO-WIA: Running pass:
;
; ORDER: Running pass: LoopVectorizePass
; ORDER: Running pass: SLPVectorizerPass
; ORDER: Running pass: VectorCombinePass
; DEFAULT: Running pass:
; EXPLICIT: Running pass: WIAPass on f
;
; DISABLED: Running pass: LoopVectorizePass

define i64 @f(i32 %x) {
  %ext = sext i32 %x to i64
  ret i64 %ext
}

define i64 @minsize_f(i32 %x) minsize {
  %ext = sext i32 %x to i64
  ret i64 %ext
}
