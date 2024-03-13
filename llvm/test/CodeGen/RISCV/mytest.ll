; ModuleID = 'mytest.cpp'
source_filename = "mytest.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@_ZL7Numbers = internal constant [7 x i32] [i32 5, i32 20, i32 70, i32 164, i32 320, i32 25, i32 555], align 16

; Function Attrs: mustprogress noinline optnone uwtable
define dso_local noundef i64 @_Z3bars(i16 noundef signext %x) #0 {
entry:
  %x.addr = alloca i16, align 2
  %y = alloca i64, align 8
  %z = alloca i32, align 4
  %i = alloca i32, align 4
  store i16 %x, ptr %x.addr, align 2
  %0 = load i16, ptr %x.addr, align 2
  %conv = sext i16 %0 to i64
  store i64 %conv, ptr %y, align 8
  store i32 5, ptr %z, align 4
  store i32 0, ptr %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %1 = load i32, ptr %i, align 4
  %2 = load i16, ptr %x.addr, align 2
  %conv1 = sext i16 %2 to i32
  %add = add nsw i32 530, %conv1
  %cmp = icmp slt i32 %1, %add
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %3 = load i16, ptr %x.addr, align 2
  %conv2 = sext i16 %3 to i32
  %cmp3 = icmp slt i32 %conv2, 120
  br i1 %cmp3, label %if.then, label %if.else

if.then:                                          ; preds = %for.body
  %4 = load i16, ptr %x.addr, align 2
  %conv4 = sext i16 %4 to i32
  %5 = load i32, ptr %i, align 4
  %idxprom = sext i32 %5 to i64
  %arrayidx = getelementptr inbounds [7 x i32], ptr @_ZL7Numbers, i64 0, i64 %idxprom
  %6 = load i32, ptr %arrayidx, align 4
  %add5 = add nsw i32 %conv4, %6
  %conv6 = sext i32 %add5 to i64
  %7 = load i64, ptr %y, align 8
  %add7 = add nsw i64 %7, %conv6
  store i64 %add7, ptr %y, align 8
  br label %if.end

if.else:                                          ; preds = %for.body
  %8 = load i16, ptr %x.addr, align 2
  %conv8 = sext i16 %8 to i32
  %sub = sub nsw i32 0, %conv8
  %call = call noundef i32 @_Z3foov()
  %mul = mul nsw i32 2, %call
  %add9 = add nsw i32 %sub, %mul
  %9 = load i32, ptr %i, align 4
  %idxprom10 = sext i32 %9 to i64
  %arrayidx11 = getelementptr inbounds [7 x i32], ptr @_ZL7Numbers, i64 0, i64 %idxprom10
  %10 = load i32, ptr %arrayidx11, align 4
  %sub12 = sub nsw i32 %add9, %10
  %conv13 = sext i32 %sub12 to i64
  %11 = load i64, ptr %y, align 8
  %add14 = add nsw i64 %11, %conv13
  store i64 %add14, ptr %y, align 8
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  br label %for.inc

for.inc:                                          ; preds = %if.end
  %12 = load i32, ptr %i, align 4
  %inc = add nsw i32 %12, 1
  store i32 %inc, ptr %i, align 4
  br label %for.cond, !llvm.loop !6

for.end:                                          ; preds = %for.cond
  %13 = load i64, ptr %y, align 8
  %14 = load i32, ptr %z, align 4
  %conv15 = sext i32 %14 to i64
  %add16 = add nsw i64 %13, %conv15
  ret i64 %add16
}

declare noundef i32 @_Z3foov() #1

attributes #0 = { mustprogress noinline optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 16.0.6 (git@github.com:karouzakisp/llvm-project.git 399857e13456327b4505d2ee0e7bb9289284d5b5)"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
