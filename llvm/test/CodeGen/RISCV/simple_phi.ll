; ModuleID = 'mytest.cpp'
source_filename = "mytest.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@_ZL7Numbers = internal unnamed_addr constant [7 x i32] [i32 5, i32 20, i32 70, i32 164, i32 320, i32 25, i32 555], align 16

; Function Attrs: mustprogress uwtable
define dso_local noundef i64 @_Z3bars(i16 noundef signext %x) local_unnamed_addr #0 {
entry:
  %conv = sext i16 %x to i64
  %conv1 = sext i16 %x to i32
  %cmp27 = icmp sgt i16 %x, -530
  br i1 %cmp27, label %for.body.lr.ph, label %for.cond.cleanup

for.body.lr.ph:                                   ; preds = %entry
  %cmp3 = icmp slt i16 %x, 120
  %0 = tail call i32 @llvm.smax.i32(i32 %conv1, i32 -529)
  %1 = add nsw i32 %0, 530
  %wide.trip.count = zext i32 %1 to i64
  br label %for.body

for.cond.cleanup:                                 ; preds = %for.inc, %entry
  %y.0.lcssa = phi i64 [ %conv, %entry ], [ %y.1, %for.inc ]
  %add16 = add nsw i64 %y.0.lcssa, 5
  ret i64 %add16

for.body:                                         ; preds = %for.body.lr.ph, %for.inc
  %indvars.iv = phi i64 [ 0, %for.body.lr.ph ], [ %indvars.iv.next, %for.inc ]
  %y.028 = phi i64 [ %conv, %for.body.lr.ph ], [ %y.1, %for.inc ]
  br i1 %cmp3, label %if.then, label %if.else

if.then:                                          ; preds = %for.body
  %arrayidx = getelementptr inbounds [7 x i32], ptr @_ZL7Numbers, i64 0, i64 %indvars.iv
  %2 = load i32, ptr %arrayidx, align 4, !tbaa !5
  %add5 = add nsw i32 %2, %conv1
  br label %for.inc

if.else:                                          ; preds = %for.body
  %call = tail call noundef i32 @_Z3foov()
  %mul = shl nsw i32 %call, 1
  %arrayidx11 = getelementptr inbounds [7 x i32], ptr @_ZL7Numbers, i64 0, i64 %indvars.iv
  %3 = load i32, ptr %arrayidx11, align 4, !tbaa !5
  %4 = add i32 %3, %conv1
  %sub12 = sub i32 %mul, %4
  br label %for.inc

for.inc:                                          ; preds = %if.then, %if.else
  %conv6.pn.in = phi i32 [ %add5, %if.then ], [ %sub12, %if.else ]
  %conv6.pn = sext i32 %conv6.pn.in to i64
  %y.1 = add nsw i64 %y.028, %conv6.pn
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond.not = icmp eq i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond.not, label %for.cond.cleanup, label %for.body, !llvm.loop !9
}

declare noundef i32 @_Z3foov() local_unnamed_addr #1

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.smax.i32(i32, i32) #2

attributes #0 = { mustprogress uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"clang version 16.0.6 (git@github.com:karouzakisp/llvm-project.git d64e19dfd056458bb782c4e6af596f2b08a64998)"}
!5 = !{!6, !6, i64 0}
!6 = !{!"int", !7, i64 0}
!7 = !{!"omnipotent char", !8, i64 0}
!8 = !{!"Simple C++ TBAA"}
!9 = distinct !{!9, !10}
!10 = !{!"llvm.loop.mustprogress"}
