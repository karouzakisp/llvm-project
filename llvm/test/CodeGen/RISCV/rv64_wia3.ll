;RUN: opt %s -mtriple=riscv64 | FileCheck %s


; Function Attrs: mustprogress noinline norecurse nounwind optnone uwtable
define dso_local noundef i32 @main() #0 {
entry:
  %retval = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %c = alloca i32, align 4
  %bb = alloca i16, align 2
  store i32 0, ptr %retval, align 4
  %call = call i64 @time(ptr noundef null) #2
  %conv = trunc i64 %call to i32
  call void @srand(i32 noundef %conv) #2
  %call1 = call i32 @rand() #2
  %rem = srem i32 %call1, 1000
  store i32 %rem, ptr %a, align 4
  %call2 = call i32 @rand() #2
  %rem3 = srem i32 %call2, 1000
  store i32 %rem3, ptr %b, align 4
  %0 = load i32, ptr %a, align 4
  %1 = load i32, ptr %b, align 4
  %xor = xor i32 %0, %1
  store i32 %xor, ptr %c, align 4
  %2 = load i32, ptr %c, align 4
  %and = and i32 %2, 24
  %conv4 = trunc i32 %and to i16
  store i16 %conv4, ptr %bb, align 2
  %3 = load i16, ptr %bb, align 2
  %conv5 = sext i16 %3 to i32
  ret i32 %conv5
}

; Function Attrs: nounwind
declare void @srand(i32 noundef) #1

; Function Attrs: nounwind
declare i64 @time(ptr noundef) #1

; Function Attrs: nounwind
declare i32 @rand() #1

attributes #0 = { mustprogress noinline norecurse nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 16.0.6 (git@github.com:karouzakisp/llvm-project.git 15f6f65a06f37ce57594619d2a33222c522190af)"}
