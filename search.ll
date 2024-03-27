; ModuleID = 'search.cpp'
source_filename = "search.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.gamestate_t = type { i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, [1000 x i32], [1000 x %struct.move_x], i64, i32, i32, i32, i32, i32, i32, i32, i32 }
%struct.move_x = type { i32, i32, i32, i32, i64, i64 }
%struct.scoreboard_t = type { i32, i32, [8 x %struct.anon.0], [8 x i32], [8 x %struct.state_t] }
%struct.anon.0 = type { i32, i32, i32 }
%struct.state_t = type { i32, [64 x i32], i64, i64, i64, [13 x i64], i32, i32, [13 x i32], i32, i32, i32, i32, i32, i32, i32, i64, i64, [64 x %struct.move_x], [64 x i32], [64 x i32], [64 x %struct.anon], i64, i64, i32, [64 x i32], i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, [1000 x i64] }
%struct.anon = type { i32, i32, i32, i32 }

@history_h = dso_local local_unnamed_addr global [8 x [12 x [64 x i32]]] zeroinitializer, align 16
@history_hit = dso_local local_unnamed_addr global [8 x [12 x [64 x i32]]] zeroinitializer, align 16
@history_tot = dso_local local_unnamed_addr global [8 x [12 x [64 x i32]]] zeroinitializer, align 16
@gamestate = external global %struct.gamestate_t, align 8
@contempt = external local_unnamed_addr global i32, align 4
@material = external local_unnamed_addr constant [14 x i32], align 16
@_ZL8rc_index = internal unnamed_addr constant [14 x i32] [i32 0, i32 1, i32 1, i32 2, i32 2, i32 3, i32 3, i32 4, i32 4, i32 5, i32 5, i32 2, i32 2, i32 0], align 16
@_ZZ11search_rootP7state_tiiiE7changes = internal unnamed_addr global i32 0, align 4
@_ZZ11search_rootP7state_tiiiE5bmove = internal unnamed_addr global i32 0, align 4
@multipv_scores = external local_unnamed_addr global [240 x i32], align 16
@root_scores = external local_unnamed_addr global [240 x i32], align 16
@uci_mode = external local_unnamed_addr global i32, align 4
@uci_multipv = external local_unnamed_addr global i32, align 4
@uci_showrefutations = external local_unnamed_addr global i32, align 4
@.str = private unnamed_addr constant [20 x i8] c"info refutation %s \00", align 1
@is_pondering = external local_unnamed_addr global i32, align 4
@scoreboard = external local_unnamed_addr global %struct.scoreboard_t, align 8
@uci_limitstrength = external local_unnamed_addr global i32, align 4
@uci_elo = external local_unnamed_addr global i32, align 4
@_ZZ5thinkP11gamestate_tP7state_tE15lastsearchscore = internal unnamed_addr global i32 0, align 4
@.str.1 = private unnamed_addr constant [41 x i8] c"info depth 1 time 0 nodes 1 score cp %d\0A\00", align 1
@.str.2 = private unnamed_addr constant [13 x i8] c"bestmove %s\0A\00", align 1
@.str.3 = private unnamed_addr constant [16 x i8] c"Opening phase.\0A\00", align 1
@.str.4 = private unnamed_addr constant [19 x i8] c"Middlegame phase.\0A\00", align 1
@.str.5 = private unnamed_addr constant [16 x i8] c"Endgame phase.\0A\00", align 1
@is_analyzing = external local_unnamed_addr global i32, align 4
@.str.6 = private unnamed_addr constant [20 x i8] c"Time for move : %d\0A\00", align 1
@.str.7 = private unnamed_addr constant [50 x i8] c"info string Time for move: %ds, early break: %ds\0A\00", align 1
@.str.8 = private unnamed_addr constant [85 x i8] c"info string Nonsense in temp_move, time_failure %d failed %d time_exit %d result %d\0A\00", align 1
@.str.9 = private unnamed_addr constant [15 x i8] c"bestmove 0000\0A\00", align 1
@buffered_count = external local_unnamed_addr global i32, align 4
@.str.10 = private unnamed_addr constant [23 x i8] c"bestmove %s ponder %s\0A\00", align 1
@time_check_log = external local_unnamed_addr global i32, align 4
@reltable._Z5thinkP11gamestate_tP7state_t = private unnamed_addr constant [3 x i32] [i32 trunc (i64 sub (i64 ptrtoint (ptr @.str.3 to i64), i64 ptrtoint (ptr @reltable._Z5thinkP11gamestate_tP7state_t to i64)) to i32), i32 trunc (i64 sub (i64 ptrtoint (ptr @.str.4 to i64), i64 ptrtoint (ptr @reltable._Z5thinkP11gamestate_tP7state_t to i64)) to i32), i32 trunc (i64 sub (i64 ptrtoint (ptr @.str.5 to i64), i64 ptrtoint (ptr @reltable._Z5thinkP11gamestate_tP7state_t to i64)) to i32)], align 4

; Function Attrs: mustprogress uwtable
define dso_local noundef i32 @_Z7qsearchP7state_tiiii(ptr noundef %s, i32 noundef %alpha, i32 noundef %beta, i32 noundef %depth, i32 noundef %qply) local_unnamed_addr #0 {
entry:
  call void @mcount()
  %best = alloca i32, align 4
  %bound = alloca i32, align 4
  %xdummy = alloca i32, align 4
  %moves = alloca [240 x i32], align 16
  %move_ordering = alloca [240 x i32], align 16
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %best) #9
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %bound) #9
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %xdummy) #9
  call void @llvm.lifetime.start.p0(i64 960, ptr nonnull %moves) #9
  call void @llvm.lifetime.start.p0(i64 960, ptr nonnull %move_ordering) #9
  %nodes = getelementptr %struct.state_t, ptr %s, i64 0, i32 22
  %0 = load <2 x i64>, ptr %nodes, align 8, !tbaa !6
  %1 = add <2 x i64> %0, <i64 1, i64 1>
  store <2 x i64> %1, ptr %nodes, align 8, !tbaa !6
  %ply = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 14
  %2 = load i32, ptr %ply, align 8, !tbaa !10
  %maxply = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 24
  %3 = load i32, ptr %maxply, align 8, !tbaa !13
  %cmp = icmp sgt i32 %2, %3
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  store i32 %2, ptr %maxply, align 8, !tbaa !13
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %4 = extractelement <2 x i64> %1, i64 0
  %call = tail call fastcc noundef i32 @_ZL17search_time_checkP7state_t(i64 %4)
  %tobool.not = icmp eq i32 %call, 0
  br i1 %tobool.not, label %if.end5, label %cleanup344

if.end5:                                          ; preds = %if.end
  %call6 = tail call noundef i32 @_Z7is_drawP11gamestate_tP7state_t(ptr noundef nonnull @gamestate, ptr noundef nonnull %s)
  %tobool7.not = icmp eq i32 %call6, 0
  br i1 %tobool7.not, label %lor.lhs.false, label %if.then9

lor.lhs.false:                                    ; preds = %if.end5
  %fifty = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 15
  %5 = load i32, ptr %fifty, align 4, !tbaa !14
  %cmp8 = icmp sgt i32 %5, 99
  br i1 %cmp8, label %if.then9, label %if.end11

if.then9:                                         ; preds = %lor.lhs.false, %if.end5
  %6 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 3), align 4, !tbaa !15
  %white_to_move = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 11
  %7 = load i32, ptr %white_to_move, align 4, !tbaa !17
  %cmp10 = icmp eq i32 %6, %7
  %8 = load i32, ptr @contempt, align 4
  %sub = sub nsw i32 0, %8
  %cond = select i1 %cmp10, i32 %8, i32 %sub
  br label %cleanup344

if.end11:                                         ; preds = %lor.lhs.false
  %call12 = call noundef i32 @_Z7ProbeTTP7state_tPiiiPjS1_S1_S1_S1_i(ptr noundef nonnull %s, ptr noundef nonnull %bound, i32 noundef %alpha, i32 noundef %beta, ptr noundef nonnull %best, ptr noundef nonnull %xdummy, ptr noundef nonnull %xdummy, ptr noundef nonnull %xdummy, ptr noundef nonnull %xdummy, i32 noundef 0)
  switch i32 %call12, label %sw.epilog [
    i32 3, label %sw.bb
    i32 1, label %sw.bb13
    i32 2, label %sw.bb17
    i32 4, label %sw.bb21
  ]

sw.bb:                                            ; preds = %if.end11
  %9 = load i32, ptr %bound, align 4, !tbaa !18
  br label %cleanup344

sw.bb13:                                          ; preds = %if.end11
  %10 = load i32, ptr %bound, align 4, !tbaa !18
  %cmp14.not = icmp sgt i32 %10, %alpha
  br i1 %cmp14.not, label %sw.epilog, label %cleanup344

sw.bb17:                                          ; preds = %if.end11
  %11 = load i32, ptr %bound, align 4, !tbaa !18
  %cmp18.not = icmp slt i32 %11, %beta
  br i1 %cmp18.not, label %sw.epilog, label %cleanup344

sw.bb21:                                          ; preds = %if.end11
  store i32 65535, ptr %best, align 4, !tbaa !18
  br label %sw.epilog

sw.epilog:                                        ; preds = %sw.bb17, %sw.bb13, %if.end11, %sw.bb21
  %12 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4, !tbaa !19
  %mul = shl nsw i32 %12, 1
  %cmp22 = icmp slt i32 %mul, %qply
  br i1 %cmp22, label %if.then26, label %lor.lhs.false23

lor.lhs.false23:                                  ; preds = %sw.epilog
  %13 = load i32, ptr %ply, align 8, !tbaa !10
  %cmp25 = icmp sgt i32 %13, 60
  br i1 %cmp25, label %if.then26, label %if.end28

if.then26:                                        ; preds = %lor.lhs.false23, %sw.epilog
  %call27 = call noundef i32 @_Z4evalP7state_tiii(ptr noundef nonnull %s, i32 noundef %alpha, i32 noundef %beta, i32 noundef 0)
  br label %cleanup344

if.end28:                                         ; preds = %lor.lhs.false23
  %idxprom = sext i32 %13 to i64
  %arrayidx = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 25, i64 %idxprom
  %14 = load i32, ptr %arrayidx, align 4, !tbaa !18
  %call30 = call noundef i32 @_Z13retrieve_evalP7state_t(ptr noundef nonnull %s)
  %add = add nsw i32 %call30, 50
  %tobool31 = icmp ne i32 %14, 0
  br i1 %tobool31, label %if.end118.thread, label %if.then32

if.then32:                                        ; preds = %if.end28
  %cmp33.not = icmp slt i32 %call30, %beta
  br i1 %cmp33.not, label %if.else, label %if.then34

if.then34:                                        ; preds = %if.then32
  %15 = load i32, ptr %best, align 4, !tbaa !18
  call void @_Z7StoreTTP7state_tiiijiiii(ptr noundef nonnull %s, i32 noundef %call30, i32 noundef %alpha, i32 noundef %beta, i32 noundef %15, i32 noundef 0, i32 noundef 0, i32 noundef 0, i32 noundef 0)
  br label %cleanup344

if.else:                                          ; preds = %if.then32
  %cmp35 = icmp sgt i32 %call30, %alpha
  br i1 %cmp35, label %if.end118, label %if.else37

if.else37:                                        ; preds = %if.else
  %add38 = add nsw i32 %call30, 985
  %cmp39.not = icmp sgt i32 %add38, %alpha
  br i1 %cmp39.not, label %if.else43, label %if.then40

if.then40:                                        ; preds = %if.else37
  %16 = load i32, ptr %best, align 4, !tbaa !18
  call void @_Z7StoreTTP7state_tiiijiiii(ptr noundef nonnull %s, i32 noundef %add38, i32 noundef %alpha, i32 noundef %beta, i32 noundef %16, i32 noundef 0, i32 noundef 0, i32 noundef 0, i32 noundef 0)
  br label %cleanup344

if.else43:                                        ; preds = %if.else37
  %white_to_move45 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 11
  %17 = load i32, ptr %white_to_move45, align 4, !tbaa !17
  %tobool46.not = icmp eq i32 %17, 0
  br i1 %tobool46.not, label %if.else80, label %if.then47

if.then47:                                        ; preds = %if.else43
  %arrayidx48 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 10
  %18 = load i32, ptr %arrayidx48, align 4, !tbaa !18
  %tobool49.not = icmp eq i32 %18, 0
  br i1 %tobool49.not, label %if.then50, label %if.end118

if.then50:                                        ; preds = %if.then47
  %arrayidx51 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 8
  %19 = load i32, ptr %arrayidx51, align 4, !tbaa !18
  %tobool52.not = icmp eq i32 %19, 0
  br i1 %tobool52.not, label %if.then53, label %if.else71

if.then53:                                        ; preds = %if.then50
  %arrayidx54 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 12
  %20 = load i32, ptr %arrayidx54, align 4, !tbaa !18
  %tobool55.not = icmp eq i32 %20, 0
  br i1 %tobool55.not, label %land.lhs.true, label %if.else64

land.lhs.true:                                    ; preds = %if.then53
  %arrayidx56 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 4
  %21 = load i32, ptr %arrayidx56, align 4, !tbaa !18
  %tobool57.not = icmp eq i32 %21, 0
  br i1 %tobool57.not, label %if.then58, label %if.else64

if.then58:                                        ; preds = %land.lhs.true
  %add59 = add nsw i32 %call30, 135
  %cmp60.not = icmp sgt i32 %add59, %alpha
  br i1 %cmp60.not, label %if.end118, label %cleanup344

if.else64:                                        ; preds = %land.lhs.true, %if.then53
  %add65 = add nsw i32 %call30, 380
  %cmp66.not = icmp sgt i32 %add65, %alpha
  br i1 %cmp66.not, label %if.end118, label %cleanup344

if.else71:                                        ; preds = %if.then50
  %add72 = add nsw i32 %call30, 540
  %cmp73.not = icmp sgt i32 %add72, %alpha
  br i1 %cmp73.not, label %if.end118, label %if.then74

if.then74:                                        ; preds = %if.else71
  %22 = load i32, ptr %best, align 4, !tbaa !18
  call void @_Z7StoreTTP7state_tiiijiiii(ptr noundef nonnull %s, i32 noundef %add72, i32 noundef %alpha, i32 noundef %beta, i32 noundef %22, i32 noundef 0, i32 noundef 0, i32 noundef 0, i32 noundef 0)
  br label %cleanup344

if.else80:                                        ; preds = %if.else43
  %arrayidx81 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 9
  %23 = load i32, ptr %arrayidx81, align 4, !tbaa !18
  %tobool82.not = icmp eq i32 %23, 0
  br i1 %tobool82.not, label %if.then83, label %if.end118

if.then83:                                        ; preds = %if.else80
  %arrayidx84 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 7
  %24 = load i32, ptr %arrayidx84, align 4, !tbaa !18
  %tobool85.not = icmp eq i32 %24, 0
  br i1 %tobool85.not, label %if.then86, label %if.else105

if.then86:                                        ; preds = %if.then83
  %arrayidx87 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 11
  %25 = load i32, ptr %arrayidx87, align 4, !tbaa !18
  %tobool88.not = icmp eq i32 %25, 0
  br i1 %tobool88.not, label %land.lhs.true89, label %if.else98

land.lhs.true89:                                  ; preds = %if.then86
  %arrayidx90 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 3
  %26 = load i32, ptr %arrayidx90, align 4, !tbaa !18
  %tobool91.not = icmp eq i32 %26, 0
  br i1 %tobool91.not, label %if.then92, label %if.else98

if.then92:                                        ; preds = %land.lhs.true89
  %add93 = add nsw i32 %call30, 135
  %cmp94.not = icmp sgt i32 %add93, %alpha
  br i1 %cmp94.not, label %if.end118, label %cleanup344

if.else98:                                        ; preds = %land.lhs.true89, %if.then86
  %add99 = add nsw i32 %call30, 380
  %cmp100.not = icmp sgt i32 %add99, %alpha
  br i1 %cmp100.not, label %if.end118, label %cleanup344

if.else105:                                       ; preds = %if.then83
  %add106 = add nsw i32 %call30, 540
  %cmp107.not = icmp sgt i32 %add106, %alpha
  br i1 %cmp107.not, label %if.end118, label %if.then108

if.then108:                                       ; preds = %if.else105
  %27 = load i32, ptr %best, align 4, !tbaa !18
  call void @_Z7StoreTTP7state_tiiijiiii(ptr noundef nonnull %s, i32 noundef %add106, i32 noundef %alpha, i32 noundef %beta, i32 noundef %27, i32 noundef 0, i32 noundef 0, i32 noundef 0, i32 noundef 0)
  br label %cleanup344

if.end118:                                        ; preds = %if.else80, %if.else105, %if.then92, %if.else98, %if.then47, %if.else71, %if.then58, %if.else64, %if.else
  %alpha.addr.0 = phi i32 [ %call30, %if.else ], [ %alpha, %if.else64 ], [ %alpha, %if.then58 ], [ %alpha, %if.else71 ], [ %alpha, %if.then47 ], [ %alpha, %if.else98 ], [ %alpha, %if.then92 ], [ %alpha, %if.else105 ], [ %alpha, %if.else80 ]
  %sub121 = sub nsw i32 %alpha.addr.0, %add
  %28 = load i32, ptr %best, align 4, !tbaa !18
  %cmp124 = icmp sgt i32 %depth, -6
  %call132 = call noundef i32 @_Z12gen_capturesP7state_tPi(ptr noundef nonnull %s, ptr noundef nonnull %moves)
  br label %if.end143

if.end118.thread:                                 ; preds = %if.end28
  %29 = load i32, ptr %best, align 4, !tbaa !18
  %call129 = call noundef i32 @_Z12gen_evasionsP7state_tPii(ptr noundef nonnull %s, ptr noundef nonnull %moves, i32 noundef %14)
  br label %if.end143

if.end143:                                        ; preds = %if.end118.thread, %if.end118
  %30 = phi i32 [ %28, %if.end118 ], [ %29, %if.end118.thread ]
  %delta.0549 = phi i32 [ %sub121, %if.end118 ], [ 0, %if.end118.thread ]
  %alpha.addr.0546 = phi i32 [ %alpha.addr.0, %if.end118 ], [ %alpha, %if.end118.thread ]
  %tobool317 = phi i1 [ %cmp124, %if.end118 ], [ false, %if.end118.thread ]
  %num_moves.0 = phi i32 [ %call132, %if.end118 ], [ %call129, %if.end118.thread ]
  %white_to_move231 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 11
  %hash = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 16
  %sub270 = sub nsw i32 0, %beta
  %add295 = add nsw i32 %qply, 1
  %cmp327 = icmp sgt i32 %depth, -1
  br label %mpass

mpass:                                            ; preds = %mpass.backedge, %if.end143
  %sbest.0 = phi i32 [ %30, %if.end143 ], [ %sbest.1.ph.ph, %mpass.backedge ]
  %no_moves.0 = phi i32 [ 1, %if.end143 ], [ %no_moves.1.ph, %mpass.backedge ]
  %score.0 = phi i32 [ -32000, %if.end143 ], [ %score.1.ph, %mpass.backedge ]
  %cmp144 = phi i1 [ false, %if.end143 ], [ %or.cond367, %mpass.backedge ]
  %cmp149 = phi i1 [ false, %if.end143 ], [ %cmp149.be, %mpass.backedge ]
  %cmp319 = phi i1 [ true, %if.end143 ], [ false, %mpass.backedge ]
  %num_moves.1 = phi i32 [ %num_moves.0, %if.end143 ], [ %num_moves.2, %mpass.backedge ]
  %alpha.addr.1 = phi i32 [ %alpha.addr.0546, %if.end143 ], [ %alpha.addr.2.ph.ph, %mpass.backedge ]
  br i1 %cmp144, label %if.then145, label %if.else148

if.then145:                                       ; preds = %mpass
  %call147 = call noundef i32 @_Z15gen_good_checksP7state_tPi(ptr noundef %s, ptr noundef nonnull %moves)
  br label %if.end154

if.else148:                                       ; preds = %mpass
  br i1 %cmp149, label %if.then150, label %if.end154

if.then150:                                       ; preds = %if.else148
  %call152 = call noundef i32 @_Z9gen_quietP7state_tPi(ptr noundef %s, ptr noundef nonnull %moves)
  br label %if.end154

if.end154:                                        ; preds = %if.else148, %if.then150, %if.then145
  %num_moves.2 = phi i32 [ %call147, %if.then145 ], [ %call152, %if.then150 ], [ %num_moves.1, %if.else148 ]
  %31 = load i32, ptr %best, align 4, !tbaa !18
  call fastcc void @_ZL16fast_order_movesP7state_tPiS1_ij(ptr noundef %s, ptr noundef nonnull %moves, ptr noundef nonnull %move_ordering, i32 noundef %num_moves.2, i32 noundef %31)
  %or.cond = or i1 %cmp144, %cmp149
  %32 = sext i32 %num_moves.2 to i64
  %33 = add i32 %num_moves.2, -2
  br label %while.cond.outer.outer

while.cond.outer.outer:                           ; preds = %if.then308, %if.end154
  %i.0.ph.ph = phi i32 [ %.us-phi579, %if.then308 ], [ -1, %if.end154 ]
  %sbest.1.ph.ph = phi i32 [ %conv312, %if.then308 ], [ %sbest.0, %if.end154 ]
  %no_moves.1.ph.ph = phi i32 [ %no_moves.2, %if.then308 ], [ %no_moves.0, %if.end154 ]
  %score.1.ph.ph = phi i32 [ %score.3, %if.then308 ], [ %score.0, %if.end154 ]
  %alpha.addr.2.ph.ph = phi i32 [ %score.3, %if.then308 ], [ %alpha.addr.1, %if.end154 ]
  %add272 = sub i32 60, %alpha.addr.2.ph.ph
  %sub271 = sub nsw i32 0, %alpha.addr.2.ph.ph
  br label %while.cond.outer

while.cond.outer:                                 ; preds = %while.cond.outer.outer, %if.end304
  %i.0.ph = phi i32 [ %.us-phi579, %if.end304 ], [ %i.0.ph.ph, %while.cond.outer.outer ]
  %no_moves.1.ph = phi i32 [ %no_moves.2, %if.end304 ], [ %no_moves.1.ph.ph, %while.cond.outer.outer ]
  %score.1.ph = phi i32 [ %score.3, %if.end304 ], [ %score.1.ph.ph, %while.cond.outer.outer ]
  br i1 %tobool31, label %while.cond.outer.split.us, label %while.cond.preheader

while.cond.preheader:                             ; preds = %while.cond.outer
  %34 = sext i32 %i.0.ph to i64
  br label %while.cond

while.cond.outer.split.us:                        ; preds = %while.cond.outer
  %inc.i.us = add nsw i32 %i.0.ph, 1
  %cmp.i.us = icmp slt i32 %i.0.ph, 9
  %cmp165.i.not.us = icmp slt i32 %inc.i.us, %num_moves.2
  br i1 %cmp.i.us, label %for.cond.preheader.i.us, label %_ZL15remove_one_fastPiS_S_i.exit.us

_ZL15remove_one_fastPiS_S_i.exit.us:              ; preds = %while.cond.outer.split.us
  br i1 %cmp165.i.not.us, label %if.end246, label %endpass

for.cond.preheader.i.us:                          ; preds = %while.cond.outer.split.us
  br i1 %cmp165.i.not.us, label %for.body.preheader.i.us, label %endpass

for.body.preheader.i.us:                          ; preds = %for.cond.preheader.i.us
  %35 = sext i32 %i.0.ph to i64
  %36 = add nsw i64 %35, 1
  %37 = xor i32 %i.0.ph, -1
  %38 = add i32 %num_moves.2, %37
  %39 = sub i32 %33, %i.0.ph
  %xtraiter = and i32 %38, 3
  %40 = icmp ult i32 %39, 3
  br i1 %40, label %if.end12.i.us.unr-lcssa, label %for.body.preheader.i.us.new

for.body.preheader.i.us.new:                      ; preds = %for.body.preheader.i.us
  %unroll_iter = and i32 %38, -4
  br label %for.body.i.us

for.body.i.us:                                    ; preds = %for.body.i.us, %for.body.preheader.i.us.new
  %indvars.iv.i.us = phi i64 [ %36, %for.body.preheader.i.us.new ], [ %indvars.iv.next.i.us.3, %for.body.i.us ]
  %tmp.068.i.us = phi i32 [ undef, %for.body.preheader.i.us.new ], [ %spec.select60.i.us.3, %for.body.i.us ]
  %best.067.i.us = phi i32 [ -1000000, %for.body.preheader.i.us.new ], [ %spec.select.i.us.3, %for.body.i.us ]
  %niter = phi i32 [ 0, %for.body.preheader.i.us.new ], [ %niter.next.3, %for.body.i.us ]
  %arrayidx.i.us = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.i.us
  %41 = load i32, ptr %arrayidx.i.us, align 4, !tbaa !18
  %cmp2.i.us = icmp sgt i32 %41, %best.067.i.us
  %spec.select.i.us = call i32 @llvm.smax.i32(i32 %41, i32 %best.067.i.us)
  %42 = trunc i64 %indvars.iv.i.us to i32
  %spec.select60.i.us = select i1 %cmp2.i.us, i32 %42, i32 %tmp.068.i.us
  %indvars.iv.next.i.us = add nsw i64 %indvars.iv.i.us, 1
  %arrayidx.i.us.1 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.next.i.us
  %43 = load i32, ptr %arrayidx.i.us.1, align 4, !tbaa !18
  %cmp2.i.us.1 = icmp sgt i32 %43, %spec.select.i.us
  %spec.select.i.us.1 = call i32 @llvm.smax.i32(i32 %43, i32 %spec.select.i.us)
  %44 = trunc i64 %indvars.iv.next.i.us to i32
  %spec.select60.i.us.1 = select i1 %cmp2.i.us.1, i32 %44, i32 %spec.select60.i.us
  %indvars.iv.next.i.us.1 = add nsw i64 %indvars.iv.i.us, 2
  %arrayidx.i.us.2 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.next.i.us.1
  %45 = load i32, ptr %arrayidx.i.us.2, align 4, !tbaa !18
  %cmp2.i.us.2 = icmp sgt i32 %45, %spec.select.i.us.1
  %spec.select.i.us.2 = call i32 @llvm.smax.i32(i32 %45, i32 %spec.select.i.us.1)
  %46 = trunc i64 %indvars.iv.next.i.us.1 to i32
  %spec.select60.i.us.2 = select i1 %cmp2.i.us.2, i32 %46, i32 %spec.select60.i.us.1
  %indvars.iv.next.i.us.2 = add nsw i64 %indvars.iv.i.us, 3
  %arrayidx.i.us.3 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.next.i.us.2
  %47 = load i32, ptr %arrayidx.i.us.3, align 4, !tbaa !18
  %cmp2.i.us.3 = icmp sgt i32 %47, %spec.select.i.us.2
  %spec.select.i.us.3 = call i32 @llvm.smax.i32(i32 %47, i32 %spec.select.i.us.2)
  %48 = trunc i64 %indvars.iv.next.i.us.2 to i32
  %spec.select60.i.us.3 = select i1 %cmp2.i.us.3, i32 %48, i32 %spec.select60.i.us.2
  %indvars.iv.next.i.us.3 = add nsw i64 %indvars.iv.i.us, 4
  %niter.next.3 = add i32 %niter, 4
  %niter.ncmp.3 = icmp eq i32 %niter.next.3, %unroll_iter
  br i1 %niter.ncmp.3, label %if.end12.i.us.unr-lcssa, label %for.body.i.us, !llvm.loop !20

if.end12.i.us.unr-lcssa:                          ; preds = %for.body.i.us, %for.body.preheader.i.us
  %spec.select.i.us.lcssa.ph = phi i32 [ undef, %for.body.preheader.i.us ], [ %spec.select.i.us.3, %for.body.i.us ]
  %indvars.iv.i.us.unr = phi i64 [ %36, %for.body.preheader.i.us ], [ %indvars.iv.next.i.us.3, %for.body.i.us ]
  %tmp.068.i.us.unr = phi i32 [ undef, %for.body.preheader.i.us ], [ %spec.select60.i.us.3, %for.body.i.us ]
  %best.067.i.us.unr = phi i32 [ -1000000, %for.body.preheader.i.us ], [ %spec.select.i.us.3, %for.body.i.us ]
  %lcmp.mod.not = icmp eq i32 %xtraiter, 0
  br i1 %lcmp.mod.not, label %if.end12.i.us, label %for.body.i.us.epil

for.body.i.us.epil:                               ; preds = %if.end12.i.us.unr-lcssa, %for.body.i.us.epil
  %indvars.iv.i.us.epil = phi i64 [ %indvars.iv.next.i.us.epil, %for.body.i.us.epil ], [ %indvars.iv.i.us.unr, %if.end12.i.us.unr-lcssa ]
  %tmp.068.i.us.epil = phi i32 [ %spec.select60.i.us.epil, %for.body.i.us.epil ], [ %tmp.068.i.us.unr, %if.end12.i.us.unr-lcssa ]
  %best.067.i.us.epil = phi i32 [ %spec.select.i.us.epil, %for.body.i.us.epil ], [ %best.067.i.us.unr, %if.end12.i.us.unr-lcssa ]
  %epil.iter = phi i32 [ %epil.iter.next, %for.body.i.us.epil ], [ 0, %if.end12.i.us.unr-lcssa ]
  %arrayidx.i.us.epil = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.i.us.epil
  %49 = load i32, ptr %arrayidx.i.us.epil, align 4, !tbaa !18
  %cmp2.i.us.epil = icmp sgt i32 %49, %best.067.i.us.epil
  %spec.select.i.us.epil = call i32 @llvm.smax.i32(i32 %49, i32 %best.067.i.us.epil)
  %50 = trunc i64 %indvars.iv.i.us.epil to i32
  %spec.select60.i.us.epil = select i1 %cmp2.i.us.epil, i32 %50, i32 %tmp.068.i.us.epil
  %indvars.iv.next.i.us.epil = add nsw i64 %indvars.iv.i.us.epil, 1
  %epil.iter.next = add i32 %epil.iter, 1
  %epil.iter.cmp.not = icmp eq i32 %epil.iter.next, %xtraiter
  br i1 %epil.iter.cmp.not, label %if.end12.i.us, label %for.body.i.us.epil, !llvm.loop !22

if.end12.i.us:                                    ; preds = %for.body.i.us.epil, %if.end12.i.us.unr-lcssa
  %spec.select.i.us.lcssa = phi i32 [ %spec.select.i.us.lcssa.ph, %if.end12.i.us.unr-lcssa ], [ %spec.select.i.us.epil, %for.body.i.us.epil ]
  %spec.select60.i.us.lcssa = phi i32 [ %tmp.068.i.us.unr, %if.end12.i.us.unr-lcssa ], [ %spec.select60.i.us.epil, %for.body.i.us.epil ]
  %cmp13.i.us = icmp sgt i32 %spec.select.i.us.lcssa, -1000000
  br i1 %cmp13.i.us, label %_ZL15remove_one_fastPiS_S_i.exit.thread562.us, label %endpass

_ZL15remove_one_fastPiS_S_i.exit.thread562.us:    ; preds = %if.end12.i.us
  %51 = sext i32 %spec.select60.i.us.lcssa to i64
  %idxprom15.i.us = sext i32 %inc.i.us to i64
  %arrayidx16.i.us = getelementptr inbounds i32, ptr %move_ordering, i64 %idxprom15.i.us
  %52 = load i32, ptr %arrayidx16.i.us, align 4, !tbaa !18
  %arrayidx18.i.us = getelementptr inbounds i32, ptr %move_ordering, i64 %51
  store i32 %52, ptr %arrayidx18.i.us, align 4, !tbaa !18
  store i32 %spec.select.i.us.lcssa, ptr %arrayidx16.i.us, align 4, !tbaa !18
  %arrayidx22.i.us = getelementptr inbounds i32, ptr %moves, i64 %idxprom15.i.us
  %53 = load i32, ptr %arrayidx22.i.us, align 4, !tbaa !18
  %arrayidx24.i.us = getelementptr inbounds i32, ptr %moves, i64 %51
  %54 = load i32, ptr %arrayidx24.i.us, align 4, !tbaa !18
  store i32 %54, ptr %arrayidx22.i.us, align 4, !tbaa !18
  store i32 %53, ptr %arrayidx24.i.us, align 4, !tbaa !18
  br label %if.end246

while.cond:                                       ; preds = %while.cond.backedge, %while.cond.preheader
  %indvars.iv = phi i64 [ %34, %while.cond.preheader ], [ %indvars.iv.next, %while.cond.backedge ]
  %indvars.iv.next = add i64 %indvars.iv, 1
  %cmp.i = icmp slt i64 %indvars.iv, 9
  %cmp165.i.not = icmp slt i64 %indvars.iv.next, %32
  br i1 %cmp.i, label %for.cond.preheader.i, label %_ZL15remove_one_fastPiS_S_i.exit

for.cond.preheader.i:                             ; preds = %while.cond
  br i1 %cmp165.i.not, label %for.body.i, label %endpass

for.body.i:                                       ; preds = %for.cond.preheader.i, %for.body.i
  %indvars.iv.i = phi i64 [ %indvars.iv.next.i, %for.body.i ], [ %indvars.iv.next, %for.cond.preheader.i ]
  %tmp.068.i = phi i32 [ %spec.select60.i, %for.body.i ], [ undef, %for.cond.preheader.i ]
  %best.067.i = phi i32 [ %spec.select.i, %for.body.i ], [ -1000000, %for.cond.preheader.i ]
  %arrayidx.i = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.i
  %55 = load i32, ptr %arrayidx.i, align 4, !tbaa !18
  %cmp2.i = icmp sgt i32 %55, %best.067.i
  %spec.select.i = call i32 @llvm.smax.i32(i32 %55, i32 %best.067.i)
  %56 = trunc i64 %indvars.iv.i to i32
  %spec.select60.i = select i1 %cmp2.i, i32 %56, i32 %tmp.068.i
  %indvars.iv.next.i = add nsw i64 %indvars.iv.i, 1
  %lftr.wideiv.i = trunc i64 %indvars.iv.next.i to i32
  %exitcond.not.i = icmp eq i32 %num_moves.2, %lftr.wideiv.i
  br i1 %exitcond.not.i, label %if.end12.i, label %for.body.i, !llvm.loop !20

if.end12.i:                                       ; preds = %for.body.i
  %cmp13.i = icmp sgt i32 %spec.select.i, -1000000
  br i1 %cmp13.i, label %_ZL15remove_one_fastPiS_S_i.exit.thread562, label %endpass

_ZL15remove_one_fastPiS_S_i.exit.thread562:       ; preds = %if.end12.i
  %57 = sext i32 %spec.select60.i to i64
  %arrayidx16.i = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.next
  %58 = load i32, ptr %arrayidx16.i, align 4, !tbaa !18
  %arrayidx18.i = getelementptr inbounds i32, ptr %move_ordering, i64 %57
  store i32 %58, ptr %arrayidx18.i, align 4, !tbaa !18
  store i32 %spec.select.i, ptr %arrayidx16.i, align 4, !tbaa !18
  %arrayidx22.i = getelementptr inbounds i32, ptr %moves, i64 %indvars.iv.next
  %59 = load i32, ptr %arrayidx22.i, align 4, !tbaa !18
  %arrayidx24.i = getelementptr inbounds i32, ptr %moves, i64 %57
  %60 = load i32, ptr %arrayidx24.i, align 4, !tbaa !18
  store i32 %60, ptr %arrayidx22.i, align 4, !tbaa !18
  store i32 %59, ptr %arrayidx24.i, align 4, !tbaa !18
  br label %while.body

_ZL15remove_one_fastPiS_S_i.exit:                 ; preds = %while.cond
  br i1 %cmp165.i.not, label %while.body, label %endpass

while.body:                                       ; preds = %_ZL15remove_one_fastPiS_S_i.exit.thread562, %_ZL15remove_one_fastPiS_S_i.exit
  br i1 %cmp319, label %if.then164, label %if.end179

if.then164:                                       ; preds = %while.body
  %arrayidx166 = getelementptr inbounds [240 x i32], ptr %moves, i64 0, i64 %indvars.iv.next
  %61 = load i32, ptr %arrayidx166, align 4, !tbaa !18
  %62 = lshr i32 %61, 19
  %and = and i32 %62, 15
  %idxprom167 = zext i32 %and to i64
  %arrayidx168 = getelementptr inbounds [14 x i32], ptr @material, i64 0, i64 %idxprom167
  %63 = load i32, ptr %arrayidx168, align 4, !tbaa !18
  %call169 = call i32 @llvm.abs.i32(i32 %63, i1 true)
  %cmp170.not = icmp sle i32 %call169, %delta.0549
  %64 = and i32 %61, 61440
  %tobool176.not = icmp eq i32 %64, 0
  %or.cond525 = select i1 %cmp170.not, i1 %tobool176.not, i1 false
  br i1 %or.cond525, label %endpass, label %if.end179

if.end179:                                        ; preds = %if.then164, %while.body
  %arrayidx185 = getelementptr inbounds [240 x i32], ptr %moves, i64 0, i64 %indvars.iv.next
  %65 = load i32, ptr %arrayidx185, align 4, !tbaa !18
  %66 = lshr i32 %65, 19
  %and187 = and i32 %66, 15
  br i1 %or.cond, label %land.lhs.true183, label %lor.lhs.false212

land.lhs.true183:                                 ; preds = %if.end179
  %cmp188.not = icmp eq i32 %and187, 13
  br i1 %cmp188.not, label %if.end199, label %land.lhs.true189

land.lhs.true189:                                 ; preds = %land.lhs.true183
  %idxprom194 = zext i32 %and187 to i64
  %arrayidx195 = getelementptr inbounds [14 x i32], ptr @material, i64 0, i64 %idxprom194
  %67 = load i32, ptr %arrayidx195, align 4, !tbaa !18
  %call196 = call i32 @llvm.abs.i32(i32 %67, i1 true)
  %cmp197 = icmp sgt i32 %call196, %delta.0549
  br i1 %cmp197, label %while.cond.backedge, label %if.end199

while.cond.backedge:                              ; preds = %land.lhs.true189, %if.then201, %if.then230
  br label %while.cond, !llvm.loop !24

if.end199:                                        ; preds = %land.lhs.true189, %land.lhs.true183
  br i1 %cmp149, label %if.then201, label %if.then230

if.then201:                                       ; preds = %if.end199
  %68 = lshr i32 %65, 6
  %and.i = and i32 %68, 63
  %idxprom.i = zext i32 %and.i to i64
  %arrayidx.i528 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 1, i64 %idxprom.i
  %69 = load i32, ptr %arrayidx.i528, align 4, !tbaa !18
  %sub.i = add nsw i32 %69, -1
  %and1.i = and i32 %65, 63
  %70 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom2.i = sext i32 %70 to i64
  %idxprom4.i = sext i32 %sub.i to i64
  %idxprom6.i = zext i32 %and1.i to i64
  %arrayidx7.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom2.i, i64 %idxprom4.i, i64 %idxprom6.i
  %71 = load i32, ptr %arrayidx7.i, align 4, !tbaa !18
  %arrayidx14.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom2.i, i64 %idxprom4.i, i64 %idxprom6.i
  %72 = load i32, ptr %arrayidx14.i, align 4, !tbaa !18
  %sub15.i = sub nsw i32 %72, %71
  %cmp.i529.not = icmp slt i32 %71, %sub15.i
  br i1 %cmp.i529.not, label %while.cond.backedge, label %if.then230

lor.lhs.false212:                                 ; preds = %if.end179
  %idxprom217 = zext i32 %and187 to i64
  %arrayidx218 = getelementptr inbounds [14 x i32], ptr @material, i64 0, i64 %idxprom217
  %73 = load i32, ptr %arrayidx218, align 4, !tbaa !18
  %call219 = call i32 @llvm.abs.i32(i32 %73, i1 true)
  %74 = lshr i32 %65, 6
  %and223 = and i32 %74, 63
  %idxprom224 = zext i32 %and223 to i64
  %arrayidx225 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 1, i64 %idxprom224
  %75 = load i32, ptr %arrayidx225, align 4, !tbaa !18
  %idxprom226 = sext i32 %75 to i64
  %arrayidx227 = getelementptr inbounds [14 x i32], ptr @material, i64 0, i64 %idxprom226
  %76 = load i32, ptr %arrayidx227, align 4, !tbaa !18
  %call228 = call i32 @llvm.abs.i32(i32 %76, i1 true)
  %cmp229 = icmp ult i32 %call219, %call228
  br i1 %cmp229, label %if.then230, label %if.end246.loopexit

if.then230:                                       ; preds = %if.end199, %if.then201, %lor.lhs.false212
  %77 = load i32, ptr %white_to_move231, align 4, !tbaa !17
  %tobool232.not = icmp eq i32 %77, 0
  %cond233 = zext i1 %tobool232.not to i32
  %78 = lshr i32 %65, 6
  %and237 = and i32 %78, 63
  %and240 = and i32 %65, 63
  %call241 = call noundef i32 @_Z3seeP7state_tiiii(ptr noundef %s, i32 noundef %cond233, i32 noundef %and237, i32 noundef %and240, i32 noundef 0)
  %cmp242 = icmp slt i32 %call241, -50
  br i1 %cmp242, label %while.cond.backedge, label %if.end246.loopexit

if.end246.loopexit:                               ; preds = %lor.lhs.false212, %if.then230
  %79 = trunc i64 %indvars.iv.next to i32
  br label %if.end246

if.end246:                                        ; preds = %if.end246.loopexit, %_ZL15remove_one_fastPiS_S_i.exit.us, %_ZL15remove_one_fastPiS_S_i.exit.thread562.us
  %.us-phi579 = phi i32 [ %inc.i.us, %_ZL15remove_one_fastPiS_S_i.exit.thread562.us ], [ %inc.i.us, %_ZL15remove_one_fastPiS_S_i.exit.us ], [ %79, %if.end246.loopexit ]
  %idxprom247 = sext i32 %.us-phi579 to i64
  %arrayidx248 = getelementptr inbounds [240 x i32], ptr %moves, i64 0, i64 %idxprom247
  %80 = load i32, ptr %arrayidx248, align 4, !tbaa !18
  call void @_Z4makeP7state_ti(ptr noundef %s, i32 noundef %80)
  %81 = load i32, ptr %arrayidx248, align 4, !tbaa !18
  %call251 = call noundef i32 @_Z11check_legalP7state_ti(ptr noundef %s, i32 noundef %81)
  %tobool252.not = icmp ne i32 %call251, 0
  br i1 %tobool252.not, label %if.then253, label %if.end299

if.then253:                                       ; preds = %if.end246
  %82 = load i64, ptr %hash, align 8, !tbaa !26
  %83 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 15), align 4, !tbaa !27
  %84 = load i32, ptr %ply, align 8, !tbaa !10
  %add255 = add i32 %84, -1
  %sub256 = add i32 %add255, %83
  %idxprom257 = sext i32 %sub256 to i64
  %arrayidx258 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 36, i64 %idxprom257
  store i64 %82, ptr %arrayidx258, align 8, !tbaa !6
  %85 = load i32, ptr %arrayidx248, align 4, !tbaa !18
  %idxprom263 = sext i32 %add255 to i64
  %arrayidx264 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 19, i64 %idxprom263
  store i32 %85, ptr %arrayidx264, align 4, !tbaa !18
  %call265 = call noundef i32 @_Z8in_checkP7state_t(ptr noundef %s)
  %86 = load i32, ptr %ply, align 8, !tbaa !10
  %idxprom268 = sext i32 %86 to i64
  %arrayidx269 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 25, i64 %idxprom268
  store i32 %call265, ptr %arrayidx269, align 4, !tbaa !18
  %tobool273 = icmp ne i32 %call265, 0
  %conv = zext i1 %tobool273 to i32
  %call275 = call noundef i32 @_Z4evalP7state_tiii(ptr noundef %s, i32 noundef %sub270, i32 noundef %add272, i32 noundef %conv)
  %sub276 = sub nsw i32 0, %call275
  %cmp279 = icmp sge i32 %alpha.addr.2.ph.ph, %sub276
  %or.cond526.not = select i1 %cmp149, i1 %cmp279, i1 false
  br i1 %or.cond526.not, label %if.end299, label %if.then280

if.then280:                                       ; preds = %if.then253
  %87 = or i32 %call265, %14
  %or.cond365.not = icmp eq i32 %87, 0
  %sub290.sub288.v = select i1 %or.cond365.not, i32 -8, i32 -1
  %sub290.sub288 = select i1 %cmp149, i32 0, i32 %sub290.sub288.v
  %newdepth.0 = add nsw i32 %sub290.sub288, %depth
  %call296 = call noundef i32 @_Z7qsearchP7state_tiiii(ptr noundef nonnull %s, i32 noundef %sub270, i32 noundef %sub271, i32 noundef %newdepth.0, i32 noundef %add295)
  %sub297 = sub nsw i32 0, %call296
  br label %if.end299

if.end299:                                        ; preds = %if.then280, %if.then253, %if.end246
  %no_moves.2 = phi i32 [ %no_moves.1.ph, %if.end246 ], [ 0, %if.then253 ], [ 0, %if.then280 ]
  %score.3 = phi i32 [ %score.1.ph, %if.end246 ], [ %score.1.ph, %if.then253 ], [ %sub297, %if.then280 ]
  %88 = load i32, ptr %arrayidx248, align 4, !tbaa !18
  call void @_Z6unmakeP7state_ti(ptr noundef %s, i32 noundef %88)
  %89 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %tobool302.not = icmp eq i32 %89, 0
  br i1 %tobool302.not, label %if.end304, label %cleanup344

if.end304:                                        ; preds = %if.end299
  %cmp305 = icmp sgt i32 %score.3, %alpha.addr.2.ph.ph
  %or.cond366 = and i1 %tobool252.not, %cmp305
  br i1 %or.cond366, label %if.then308, label %while.cond.outer, !llvm.loop !24

if.then308:                                       ; preds = %if.end304
  %90 = load i32, ptr %arrayidx248, align 4, !tbaa !18
  %call311 = call noundef zeroext i16 @_Z12compact_movei(i32 noundef %90)
  %conv312 = zext i16 %call311 to i32
  %cmp313.not = icmp slt i32 %score.3, %beta
  br i1 %cmp313.not, label %while.cond.outer.outer, label %if.then314, !llvm.loop !24

if.then314:                                       ; preds = %if.then308
  call void @_Z7StoreTTP7state_tiiijiiii(ptr noundef %s, i32 noundef %score.3, i32 noundef %alpha, i32 noundef %beta, i32 noundef %conv312, i32 noundef 0, i32 noundef 0, i32 noundef 0, i32 noundef 0)
  br label %cleanup344

endpass:                                          ; preds = %_ZL15remove_one_fastPiS_S_i.exit.us, %for.cond.preheader.i.us, %if.end12.i.us, %_ZL15remove_one_fastPiS_S_i.exit, %if.then164, %if.end12.i, %for.cond.preheader.i
  %or.cond367 = and i1 %tobool317, %cmp319
  br i1 %or.cond367, label %mpass.backedge, label %if.else322

mpass.backedge:                                   ; preds = %endpass, %if.else322
  %cmp149.be = xor i1 %or.cond367, true
  br label %mpass

if.else322:                                       ; preds = %endpass
  %91 = and i1 %cmp327, %cmp144
  %or.cond370 = and i1 %tobool317, %91
  %cmp330 = icmp sgt i32 %add, %alpha.addr.2.ph.ph
  %or.cond527 = select i1 %or.cond370, i1 %cmp330, i1 false
  br i1 %or.cond527, label %mpass.backedge, label %if.end336

if.end336:                                        ; preds = %if.else322
  %tobool337 = icmp ne i32 %no_moves.1.ph, 0
  %or.cond369 = and i1 %tobool31, %tobool337
  br i1 %or.cond369, label %if.then340, label %if.end343

if.then340:                                       ; preds = %if.end336
  %92 = load i32, ptr %ply, align 8, !tbaa !10
  %add342 = add nsw i32 %92, -32000
  br label %if.end343

if.end343:                                        ; preds = %if.then340, %if.end336
  %alpha.addr.4 = phi i32 [ %add342, %if.then340 ], [ %alpha.addr.2.ph.ph, %if.end336 ]
  call void @_Z7StoreTTP7state_tiiijiiii(ptr noundef %s, i32 noundef %alpha.addr.4, i32 noundef %alpha, i32 noundef %beta, i32 noundef %sbest.1.ph.ph, i32 noundef 0, i32 noundef 0, i32 noundef 0, i32 noundef 0)
  br label %cleanup344

cleanup344:                                       ; preds = %if.end299, %if.else98, %if.then92, %if.else64, %if.then58, %if.then74, %if.then108, %sw.bb17, %sw.bb13, %if.end, %if.end343, %if.then314, %if.then40, %if.then34, %if.then26, %sw.bb, %if.then9
  %retval.1 = phi i32 [ %cond, %if.then9 ], [ %call27, %if.then26 ], [ %score.3, %if.then314 ], [ %alpha.addr.4, %if.end343 ], [ %call30, %if.then34 ], [ %add38, %if.then40 ], [ %9, %sw.bb ], [ 0, %if.end ], [ %10, %sw.bb13 ], [ %11, %sw.bb17 ], [ %add99, %if.else98 ], [ %add93, %if.then92 ], [ %add65, %if.else64 ], [ %add59, %if.then58 ], [ %add72, %if.then74 ], [ %add106, %if.then108 ], [ 0, %if.end299 ]
  call void @llvm.lifetime.end.p0(i64 960, ptr nonnull %move_ordering) #9
  call void @llvm.lifetime.end.p0(i64 960, ptr nonnull %moves) #9
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %xdummy) #9
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %bound) #9
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %best) #9
  ret i32 %retval.1
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #1

; Function Attrs: mustprogress uwtable
define internal fastcc noundef i32 @_ZL17search_time_checkP7state_t(i64 %s.4080.val) unnamed_addr #0 {
entry:
  call void @mcount()
  %0 = load i32, ptr @time_check_log, align 4, !tbaa !18
  %notmask = shl nsw i32 -1, %0
  %sub = xor i32 %notmask, -1
  %conv = sext i32 %sub to i64
  %and = and i64 %conv, %s.4080.val
  %tobool.not = icmp eq i64 %and, 0
  br i1 %tobool.not, label %if.then, label %cleanup

if.then:                                          ; preds = %entry
  %call = tail call noundef i32 @_Z9interruptv()
  %tobool1.not = icmp eq i32 %call, 0
  br i1 %tobool1.not, label %if.else, label %if.then2

if.then2:                                         ; preds = %if.then
  store i32 1, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  br label %cleanup

if.else:                                          ; preds = %if.then
  %call3 = tail call noundef i32 @_Z5rtimev()
  %1 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 19), align 8, !tbaa !29
  %call4 = tail call noundef i32 @_Z9rdifftimeii(i32 noundef %call3, i32 noundef %1)
  %2 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8, !tbaa !30
  %cmp.not = icmp sge i32 %call4, %2
  %3 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4
  %cmp5 = icmp sgt i32 %3, 2
  %or.cond = select i1 %cmp.not, i1 %cmp5, i1 false
  br i1 %or.cond, label %if.then6, label %cleanup

if.then6:                                         ; preds = %if.else
  %4 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 22), align 4, !tbaa !31
  %cmp7 = icmp eq i32 %4, 1
  %5 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 24), align 4
  %cmp9.not = icmp ne i32 %5, 2
  %or.cond121.not13 = select i1 %cmp7, i1 %cmp9.not, i1 false
  %6 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 13), align 4
  %tobool11.not = icmp eq i32 %6, 0
  %or.cond122 = select i1 %or.cond121.not13, i1 %tobool11.not, i1 false
  br i1 %or.cond122, label %land.rhs, label %if.else42

land.rhs:                                         ; preds = %if.then6
  %7 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 10), align 8, !tbaa !32
  %mul = mul nsw i32 %2, 7
  %.sroa.speculated10 = tail call i32 @llvm.smax.i32(i32 %mul, i32 1000)
  %cmp15 = icmp sgt i32 %7, %.sroa.speculated10
  br i1 %cmp15, label %if.then16, label %if.else42

if.then16:                                        ; preds = %land.rhs
  %tobool19.not = icmp eq i32 %5, 0
  br i1 %tobool19.not, label %if.then20, label %if.end

if.then20:                                        ; preds = %if.then16
  %call21 = tail call noundef i32 @_Z13allocate_timeP11gamestate_ti(ptr noundef nonnull @gamestate, i32 noundef 1)
  %8 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8, !tbaa !30
  %add = add nsw i32 %8, %call21
  store i32 %add, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8, !tbaa !30
  %call23 = tail call noundef i32 @_Z13allocate_timeP11gamestate_ti(ptr noundef nonnull @gamestate, i32 noundef 1)
  %9 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8, !tbaa !30
  %add25 = add nsw i32 %9, %call23
  store i32 %add25, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8, !tbaa !30
  br label %if.end

if.end:                                           ; preds = %if.then20, %if.then16
  %call26 = tail call noundef i32 @_Z13allocate_timeP11gamestate_ti(ptr noundef nonnull @gamestate, i32 noundef 1)
  %10 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8, !tbaa !30
  %add28 = add nsw i32 %10, %call26
  store i32 %add28, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8, !tbaa !30
  %call29 = tail call noundef i32 @_Z13allocate_timeP11gamestate_ti(ptr noundef nonnull @gamestate, i32 noundef 1)
  %11 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8, !tbaa !30
  %add31 = add nsw i32 %11, %call29
  store i32 %add31, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8, !tbaa !30
  %call32 = tail call noundef i32 @_Z13allocate_timeP11gamestate_ti(ptr noundef nonnull @gamestate, i32 noundef 1)
  %12 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8, !tbaa !30
  %add34 = add nsw i32 %12, %call32
  %13 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 10), align 8, !tbaa !32
  %sub38 = add nsw i32 %13, -50
  %.sroa.speculated7 = tail call i32 @llvm.smin.i32(i32 %sub38, i32 %add34)
  store i32 %.sroa.speculated7, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8, !tbaa !30
  store i32 2, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 24), align 4, !tbaa !33
  br label %cleanup

if.else42:                                        ; preds = %if.then6, %land.rhs
  %14 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 23), align 8, !tbaa !34
  %cmp43 = icmp eq i32 %14, 1
  %tobool46.not = icmp eq i32 %5, 0
  %or.cond123 = select i1 %cmp43, i1 %tobool46.not, i1 false
  %or.cond124 = select i1 %or.cond123, i1 %tobool11.not, i1 false
  br i1 %or.cond124, label %land.rhs50, label %if.else74

land.rhs50:                                       ; preds = %if.else42
  %15 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 10), align 8, !tbaa !32
  %mul54 = mul nsw i32 %2, 6
  %.sroa.speculated3 = tail call i32 @llvm.smax.i32(i32 %mul54, i32 1000)
  %cmp57 = icmp sgt i32 %15, %.sroa.speculated3
  br i1 %cmp57, label %if.then59, label %if.else74

if.then59:                                        ; preds = %land.rhs50
  store i32 1, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 24), align 4, !tbaa !33
  %call62 = tail call noundef i32 @_Z13allocate_timeP11gamestate_ti(ptr noundef nonnull @gamestate, i32 noundef 1)
  %16 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8, !tbaa !30
  %add64 = add nsw i32 %16, %call62
  store i32 %add64, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8, !tbaa !30
  %call65 = tail call noundef i32 @_Z13allocate_timeP11gamestate_ti(ptr noundef nonnull @gamestate, i32 noundef 1)
  %17 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8, !tbaa !30
  %add67 = add nsw i32 %17, %call65
  %18 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 10), align 8, !tbaa !32
  %sub71 = add nsw i32 %18, -50
  %.sroa.speculated = tail call i32 @llvm.smin.i32(i32 %sub71, i32 %add67)
  store i32 %.sroa.speculated, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8, !tbaa !30
  br label %cleanup

if.else74:                                        ; preds = %if.else42, %land.rhs50
  store i32 1, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  br label %cleanup

cleanup:                                          ; preds = %entry, %if.else, %if.then59, %if.end, %if.else74, %if.then2
  %retval.0 = phi i32 [ 1, %if.then2 ], [ 1, %if.else74 ], [ 0, %if.end ], [ 0, %if.then59 ], [ 0, %if.else ], [ 0, %entry ]
  ret i32 %retval.0
}

declare noundef i32 @_Z7is_drawP11gamestate_tP7state_t(ptr noundef, ptr noundef) local_unnamed_addr #2

declare noundef i32 @_Z7ProbeTTP7state_tPiiiPjS1_S1_S1_S1_i(ptr noundef, ptr noundef, i32 noundef, i32 noundef, ptr noundef, ptr noundef, ptr noundef, ptr noundef, ptr noundef, i32 noundef) local_unnamed_addr #2

declare noundef i32 @_Z4evalP7state_tiii(ptr noundef, i32 noundef, i32 noundef, i32 noundef) local_unnamed_addr #2

declare noundef i32 @_Z13retrieve_evalP7state_t(ptr noundef) local_unnamed_addr #2

declare void @_Z7StoreTTP7state_tiiijiiii(ptr noundef, i32 noundef, i32 noundef, i32 noundef, i32 noundef, i32 noundef, i32 noundef, i32 noundef, i32 noundef) local_unnamed_addr #2

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #1

declare noundef i32 @_Z12gen_evasionsP7state_tPii(ptr noundef, ptr noundef, i32 noundef) local_unnamed_addr #2

declare noundef i32 @_Z12gen_capturesP7state_tPi(ptr noundef, ptr noundef) local_unnamed_addr #2

declare noundef i32 @_Z15gen_good_checksP7state_tPi(ptr noundef, ptr noundef) local_unnamed_addr #2

declare noundef i32 @_Z9gen_quietP7state_tPi(ptr noundef, ptr noundef) local_unnamed_addr #2

; Function Attrs: mustprogress uwtable
define internal fastcc void @_ZL16fast_order_movesP7state_tPiS1_ij(ptr nocapture noundef readonly %s, ptr nocapture noundef readonly %moves, ptr nocapture noundef writeonly %move_ordering, i32 noundef %num_moves, i32 noundef %best) unnamed_addr #0 {
entry:
  call void @mcount()
  %cmp155 = icmp sgt i32 %num_moves, 0
  br i1 %cmp155, label %for.body.lr.ph, label %for.end

for.body.lr.ph:                                   ; preds = %entry
  %ply = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 14
  %wide.trip.count = zext i32 %num_moves to i64
  br label %for.body

for.body:                                         ; preds = %for.body.lr.ph, %for.inc
  %indvars.iv = phi i64 [ 0, %for.body.lr.ph ], [ %indvars.iv.next, %for.inc ]
  %arrayidx = getelementptr inbounds i32, ptr %moves, i64 %indvars.iv
  %0 = load i32, ptr %arrayidx, align 4, !tbaa !18
  %1 = lshr i32 %0, 6
  %and = and i32 %1, 63
  %and3 = and i32 %0, 63
  %2 = lshr i32 %0, 19
  %and7 = and i32 %2, 15
  %3 = lshr i32 %0, 12
  %and11 = and i32 %3, 15
  %call = tail call noundef zeroext i16 @_Z12compact_movei(i32 noundef %0)
  %conv = zext i16 %call to i32
  %cmp14 = icmp eq i32 %conv, %best
  br i1 %cmp14, label %for.inc, label %if.else

if.else:                                          ; preds = %for.body
  %cmp17.not = icmp eq i32 %and7, 13
  br i1 %cmp17.not, label %if.else29, label %if.then18

if.then18:                                        ; preds = %if.else
  %idxprom19 = zext i32 %and7 to i64
  %arrayidx20 = getelementptr inbounds [14 x i32], ptr @material, i64 0, i64 %idxprom19
  %4 = load i32, ptr %arrayidx20, align 4, !tbaa !18
  %call21 = tail call i32 @llvm.abs.i32(i32 %4, i1 true)
  %mul = shl nsw i32 %call21, 12
  %add = add nuw nsw i32 %mul, 10000000
  %idxprom22 = zext i32 %and to i64
  %arrayidx23 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 1, i64 %idxprom22
  %5 = load i32, ptr %arrayidx23, align 4, !tbaa !18
  %idxprom24 = sext i32 %5 to i64
  %arrayidx25 = getelementptr inbounds [14 x i32], ptr @material, i64 0, i64 %idxprom24
  %6 = load i32, ptr %arrayidx25, align 4, !tbaa !18
  %call26 = tail call i32 @llvm.abs.i32(i32 %6, i1 true)
  %sub = sub nsw i32 %add, %call26
  br label %for.inc

if.else29:                                        ; preds = %if.else
  %tobool.not = icmp eq i32 %and11, 0
  br i1 %tobool.not, label %if.else37, label %if.then30

if.then30:                                        ; preds = %if.else29
  %idxprom31 = zext i32 %and11 to i64
  %arrayidx32 = getelementptr inbounds [14 x i32], ptr @material, i64 0, i64 %idxprom31
  %7 = load i32, ptr %arrayidx32, align 4, !tbaa !18
  %call33 = tail call i32 @llvm.abs.i32(i32 %7, i1 true)
  %add34 = add nuw nsw i32 %call33, 9000000
  br label %for.inc

if.else37:                                        ; preds = %if.else29
  %8 = load i32, ptr %arrayidx, align 4, !tbaa !18
  %9 = load i32, ptr %ply, align 8, !tbaa !10
  %idxprom40 = sext i32 %9 to i64
  %arrayidx41 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 21, i64 %idxprom40
  %10 = load i32, ptr %arrayidx41, align 8, !tbaa !35
  %cmp42 = icmp eq i32 %8, %10
  br i1 %cmp42, label %for.inc, label %if.else46

if.else46:                                        ; preds = %if.else37
  %killer2 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 21, i64 %idxprom40, i32 1
  %11 = load i32, ptr %killer2, align 4, !tbaa !37
  %cmp53 = icmp eq i32 %8, %11
  br i1 %cmp53, label %for.inc, label %if.else57

if.else57:                                        ; preds = %if.else46
  %killer3 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 21, i64 %idxprom40, i32 2
  %12 = load i32, ptr %killer3, align 8, !tbaa !38
  %cmp64 = icmp eq i32 %8, %12
  br i1 %cmp64, label %for.inc, label %if.else68

if.else68:                                        ; preds = %if.else57
  %killer4 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 21, i64 %idxprom40, i32 3
  %13 = load i32, ptr %killer4, align 4, !tbaa !39
  %cmp75 = icmp eq i32 %8, %13
  br i1 %cmp75, label %for.inc, label %if.else79

if.else79:                                        ; preds = %if.else68
  %idxprom81 = zext i32 %and to i64
  %arrayidx82 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 1, i64 %idxprom81
  %14 = load i32, ptr %arrayidx82, align 4, !tbaa !18
  %sub83 = add nsw i32 %14, -1
  %15 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom84 = sext i32 %15 to i64
  %idxprom86 = sext i32 %sub83 to i64
  %idxprom88 = zext i32 %and3 to i64
  %arrayidx89 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom84, i64 %idxprom86, i64 %idxprom88
  %16 = load i32, ptr %arrayidx89, align 4, !tbaa !18
  %arrayidx96 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom84, i64 %idxprom86, i64 %idxprom81
  %17 = load i32, ptr %arrayidx96, align 4, !tbaa !18
  %sub97 = sub nsw i32 %16, %17
  br label %for.inc

for.inc:                                          ; preds = %if.else68, %if.else57, %if.else46, %if.else37, %for.body, %if.then30, %if.else79, %if.then18
  %.sink = phi i32 [ %add34, %if.then30 ], [ %sub97, %if.else79 ], [ %sub, %if.then18 ], [ 128000000, %for.body ], [ 8000000, %if.else37 ], [ 7000000, %if.else46 ], [ 6000000, %if.else57 ], [ 5000000, %if.else68 ]
  %arrayidx16 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv
  store i32 %.sink, ptr %arrayidx16, align 4, !tbaa !18
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond.not = icmp eq i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond.not, label %for.end, label %for.body, !llvm.loop !40

for.end:                                          ; preds = %for.inc, %entry
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable
define internal fastcc noundef i32 @_ZL15remove_one_fastPiS_S_i(ptr nocapture noundef %marker, ptr nocapture noundef %move_ordering, ptr nocapture noundef %moves, i32 noundef %num_moves) unnamed_addr #3 {
entry:
  call void @mcount()
  %0 = load i32, ptr %marker, align 4, !tbaa !18
  %inc = add nsw i32 %0, 1
  store i32 %inc, ptr %marker, align 4, !tbaa !18
  %cmp = icmp slt i32 %0, 9
  %cmp165 = icmp slt i32 %inc, %num_moves
  br i1 %cmp, label %for.cond.preheader, label %if.else

for.cond.preheader:                               ; preds = %entry
  br i1 %cmp165, label %for.body.preheader, label %cleanup

for.body.preheader:                               ; preds = %for.cond.preheader
  %1 = sext i32 %0 to i64
  %2 = add nsw i64 %1, 1
  %3 = xor i32 %0, -1
  %4 = add i32 %3, %num_moves
  %5 = add i32 %num_moves, -2
  %6 = sub i32 %5, %0
  %xtraiter = and i32 %4, 3
  %7 = icmp ult i32 %6, 3
  br i1 %7, label %if.end12.unr-lcssa, label %for.body.preheader.new

for.body.preheader.new:                           ; preds = %for.body.preheader
  %unroll_iter = and i32 %4, -4
  br label %for.body

for.body:                                         ; preds = %for.body, %for.body.preheader.new
  %indvars.iv = phi i64 [ %2, %for.body.preheader.new ], [ %indvars.iv.next.3, %for.body ]
  %tmp.068 = phi i32 [ undef, %for.body.preheader.new ], [ %spec.select60.3, %for.body ]
  %best.067 = phi i32 [ -1000000, %for.body.preheader.new ], [ %spec.select.3, %for.body ]
  %niter = phi i32 [ 0, %for.body.preheader.new ], [ %niter.next.3, %for.body ]
  %arrayidx = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv
  %8 = load i32, ptr %arrayidx, align 4, !tbaa !18
  %cmp2 = icmp sgt i32 %8, %best.067
  %spec.select = tail call i32 @llvm.smax.i32(i32 %8, i32 %best.067)
  %9 = trunc i64 %indvars.iv to i32
  %spec.select60 = select i1 %cmp2, i32 %9, i32 %tmp.068
  %indvars.iv.next = add nsw i64 %indvars.iv, 1
  %arrayidx.1 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.next
  %10 = load i32, ptr %arrayidx.1, align 4, !tbaa !18
  %cmp2.1 = icmp sgt i32 %10, %spec.select
  %spec.select.1 = tail call i32 @llvm.smax.i32(i32 %10, i32 %spec.select)
  %11 = trunc i64 %indvars.iv.next to i32
  %spec.select60.1 = select i1 %cmp2.1, i32 %11, i32 %spec.select60
  %indvars.iv.next.1 = add nsw i64 %indvars.iv, 2
  %arrayidx.2 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.next.1
  %12 = load i32, ptr %arrayidx.2, align 4, !tbaa !18
  %cmp2.2 = icmp sgt i32 %12, %spec.select.1
  %spec.select.2 = tail call i32 @llvm.smax.i32(i32 %12, i32 %spec.select.1)
  %13 = trunc i64 %indvars.iv.next.1 to i32
  %spec.select60.2 = select i1 %cmp2.2, i32 %13, i32 %spec.select60.1
  %indvars.iv.next.2 = add nsw i64 %indvars.iv, 3
  %arrayidx.3 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.next.2
  %14 = load i32, ptr %arrayidx.3, align 4, !tbaa !18
  %cmp2.3 = icmp sgt i32 %14, %spec.select.2
  %spec.select.3 = tail call i32 @llvm.smax.i32(i32 %14, i32 %spec.select.2)
  %15 = trunc i64 %indvars.iv.next.2 to i32
  %spec.select60.3 = select i1 %cmp2.3, i32 %15, i32 %spec.select60.2
  %indvars.iv.next.3 = add nsw i64 %indvars.iv, 4
  %niter.next.3 = add i32 %niter, 4
  %niter.ncmp.3 = icmp eq i32 %niter.next.3, %unroll_iter
  br i1 %niter.ncmp.3, label %if.end12.unr-lcssa, label %for.body, !llvm.loop !20

if.else:                                          ; preds = %entry
  %spec.select64 = zext i1 %cmp165 to i32
  br label %cleanup

if.end12.unr-lcssa:                               ; preds = %for.body, %for.body.preheader
  %spec.select.lcssa.ph = phi i32 [ undef, %for.body.preheader ], [ %spec.select.3, %for.body ]
  %indvars.iv.unr = phi i64 [ %2, %for.body.preheader ], [ %indvars.iv.next.3, %for.body ]
  %tmp.068.unr = phi i32 [ undef, %for.body.preheader ], [ %spec.select60.3, %for.body ]
  %best.067.unr = phi i32 [ -1000000, %for.body.preheader ], [ %spec.select.3, %for.body ]
  %lcmp.mod.not = icmp eq i32 %xtraiter, 0
  br i1 %lcmp.mod.not, label %if.end12, label %for.body.epil

for.body.epil:                                    ; preds = %if.end12.unr-lcssa, %for.body.epil
  %indvars.iv.epil = phi i64 [ %indvars.iv.next.epil, %for.body.epil ], [ %indvars.iv.unr, %if.end12.unr-lcssa ]
  %tmp.068.epil = phi i32 [ %spec.select60.epil, %for.body.epil ], [ %tmp.068.unr, %if.end12.unr-lcssa ]
  %best.067.epil = phi i32 [ %spec.select.epil, %for.body.epil ], [ %best.067.unr, %if.end12.unr-lcssa ]
  %epil.iter = phi i32 [ %epil.iter.next, %for.body.epil ], [ 0, %if.end12.unr-lcssa ]
  %arrayidx.epil = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.epil
  %16 = load i32, ptr %arrayidx.epil, align 4, !tbaa !18
  %cmp2.epil = icmp sgt i32 %16, %best.067.epil
  %spec.select.epil = tail call i32 @llvm.smax.i32(i32 %16, i32 %best.067.epil)
  %17 = trunc i64 %indvars.iv.epil to i32
  %spec.select60.epil = select i1 %cmp2.epil, i32 %17, i32 %tmp.068.epil
  %indvars.iv.next.epil = add nsw i64 %indvars.iv.epil, 1
  %epil.iter.next = add i32 %epil.iter, 1
  %epil.iter.cmp.not = icmp eq i32 %epil.iter.next, %xtraiter
  br i1 %epil.iter.cmp.not, label %if.end12, label %for.body.epil, !llvm.loop !41

if.end12:                                         ; preds = %for.body.epil, %if.end12.unr-lcssa
  %spec.select.lcssa = phi i32 [ %spec.select.lcssa.ph, %if.end12.unr-lcssa ], [ %spec.select.epil, %for.body.epil ]
  %spec.select60.lcssa = phi i32 [ %tmp.068.unr, %if.end12.unr-lcssa ], [ %spec.select60.epil, %for.body.epil ]
  %cmp13 = icmp sgt i32 %spec.select.lcssa, -1000000
  br i1 %cmp13, label %if.then14, label %cleanup

if.then14:                                        ; preds = %if.end12
  %18 = sext i32 %spec.select60.lcssa to i64
  %idxprom15 = sext i32 %inc to i64
  %arrayidx16 = getelementptr inbounds i32, ptr %move_ordering, i64 %idxprom15
  %19 = load i32, ptr %arrayidx16, align 4, !tbaa !18
  %arrayidx18 = getelementptr inbounds i32, ptr %move_ordering, i64 %18
  store i32 %19, ptr %arrayidx18, align 4, !tbaa !18
  store i32 %spec.select.lcssa, ptr %arrayidx16, align 4, !tbaa !18
  %arrayidx22 = getelementptr inbounds i32, ptr %moves, i64 %idxprom15
  %20 = load i32, ptr %arrayidx22, align 4, !tbaa !18
  %arrayidx24 = getelementptr inbounds i32, ptr %moves, i64 %18
  %21 = load i32, ptr %arrayidx24, align 4, !tbaa !18
  store i32 %21, ptr %arrayidx22, align 4, !tbaa !18
  store i32 %20, ptr %arrayidx24, align 4, !tbaa !18
  br label %cleanup

cleanup:                                          ; preds = %for.cond.preheader, %if.else, %if.end12, %if.then14
  %retval.0 = phi i32 [ 1, %if.then14 ], [ 0, %if.end12 ], [ %spec.select64, %if.else ], [ 0, %for.cond.preheader ]
  ret i32 %retval.0
}

declare noundef i32 @_Z3seeP7state_tiiii(ptr noundef, i32 noundef, i32 noundef, i32 noundef, i32 noundef) local_unnamed_addr #2

declare void @_Z4makeP7state_ti(ptr noundef, i32 noundef) local_unnamed_addr #2

declare noundef i32 @_Z11check_legalP7state_ti(ptr noundef, i32 noundef) local_unnamed_addr #2

declare noundef i32 @_Z8in_checkP7state_t(ptr noundef) local_unnamed_addr #2

declare void @_Z6unmakeP7state_ti(ptr noundef, i32 noundef) local_unnamed_addr #2

declare noundef zeroext i16 @_Z12compact_movei(i32 noundef) local_unnamed_addr #2

; Function Attrs: mustprogress uwtable
define dso_local noundef i32 @_Z6searchP7state_tiiiii(ptr noundef %s, i32 noundef %alpha, i32 noundef %beta, i32 noundef %depth, i32 noundef %is_null, i32 noundef %cutnode) local_unnamed_addr #0 {
entry:
  call void @mcount()
  %moves = alloca [240 x i32], align 16
  %move_ordering = alloca [240 x i32], align 16
  %i = alloca i32, align 4
  %bound = alloca i32, align 4
  %threat = alloca i32, align 4
  %donull = alloca i32, align 4
  %best = alloca i32, align 4
  %singular = alloca i32, align 4
  %nosingular = alloca i32, align 4
  %searched_moves = alloca [240 x i32], align 16
  %tmp = alloca i32, align 4
  %mv = alloca i32, align 4
  call void @llvm.lifetime.start.p0(i64 960, ptr nonnull %moves) #9
  call void @llvm.lifetime.start.p0(i64 960, ptr nonnull %move_ordering) #9
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %i) #9
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %bound) #9
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %threat) #9
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %donull) #9
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %best) #9
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %singular) #9
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %nosingular) #9
  call void @llvm.lifetime.start.p0(i64 960, ptr nonnull %searched_moves) #9
  %cmp = icmp slt i32 %depth, 1
  br i1 %cmp, label %if.then, label %lor.lhs.false

lor.lhs.false:                                    ; preds = %entry
  %ply = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 14
  %0 = load i32, ptr %ply, align 8, !tbaa !10
  %cmp1 = icmp sgt i32 %0, 59
  br i1 %cmp1, label %if.then, label %if.end

if.then:                                          ; preds = %lor.lhs.false, %entry
  %call = tail call noundef i32 @_Z7qsearchP7state_tiiii(ptr noundef %s, i32 noundef %alpha, i32 noundef %beta, i32 noundef 0, i32 noundef 0)
  br label %cleanup1026

if.end:                                           ; preds = %lor.lhs.false
  %nodes = getelementptr %struct.state_t, ptr %s, i64 0, i32 22
  %1 = load i64, ptr %nodes, align 8, !tbaa !42
  %inc = add i64 %1, 1
  store i64 %inc, ptr %nodes, align 8, !tbaa !42
  %call2 = tail call fastcc noundef i32 @_ZL17search_time_checkP7state_t(i64 %inc)
  %tobool.not = icmp eq i32 %call2, 0
  br i1 %tobool.not, label %if.end4, label %cleanup1026

if.end4:                                          ; preds = %if.end
  %call5 = tail call noundef i32 @_Z7is_drawP11gamestate_tP7state_t(ptr noundef nonnull @gamestate, ptr noundef nonnull %s)
  %tobool6.not = icmp eq i32 %call5, 0
  br i1 %tobool6.not, label %lor.lhs.false7, label %if.then9

lor.lhs.false7:                                   ; preds = %if.end4
  %fifty = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 15
  %2 = load i32, ptr %fifty, align 4, !tbaa !14
  %cmp8 = icmp sgt i32 %2, 99
  br i1 %cmp8, label %if.then9, label %if.end11

if.then9:                                         ; preds = %lor.lhs.false7, %if.end4
  %3 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 3), align 4, !tbaa !15
  %white_to_move = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 11
  %4 = load i32, ptr %white_to_move, align 4, !tbaa !17
  %cmp10 = icmp eq i32 %3, %4
  %5 = load i32, ptr @contempt, align 4
  %sub = sub nsw i32 0, %5
  %cond = select i1 %cmp10, i32 %5, i32 %sub
  br label %cleanup1026

if.end11:                                         ; preds = %lor.lhs.false7
  %6 = load i32, ptr %ply, align 8, !tbaa !10
  %add = add nsw i32 %6, -32000
  %cmp13 = icmp sgt i32 %add, %alpha
  br i1 %cmp13, label %if.then14, label %if.end18

if.then14:                                        ; preds = %if.end11
  %cmp15.not = icmp slt i32 %add, %beta
  br i1 %cmp15.not, label %if.end18, label %cleanup1026

if.end18:                                         ; preds = %if.then14, %if.end11
  %alpha.addr.0 = phi i32 [ %add, %if.then14 ], [ %alpha, %if.end11 ]
  %sub21 = sub i32 31999, %6
  %cmp22 = icmp slt i32 %sub21, %beta
  br i1 %cmp22, label %if.then23, label %if.end27

if.then23:                                        ; preds = %if.end18
  %cmp24.not = icmp sgt i32 %sub21, %alpha.addr.0
  br i1 %cmp24.not, label %if.end27, label %cleanup1026

if.end27:                                         ; preds = %if.then23, %if.end18
  %beta.addr.0 = phi i32 [ %sub21, %if.then23 ], [ %beta, %if.end18 ]
  %call28 = call noundef i32 @_Z7ProbeTTP7state_tPiiiPjS1_S1_S1_S1_i(ptr noundef nonnull %s, ptr noundef nonnull %bound, i32 noundef %alpha.addr.0, i32 noundef %beta.addr.0, ptr noundef nonnull %best, ptr noundef nonnull %threat, ptr noundef nonnull %donull, ptr noundef nonnull %singular, ptr noundef nonnull %nosingular, i32 noundef %depth)
  switch i32 %call28, label %sw.epilog [
    i32 3, label %sw.bb
    i32 1, label %sw.bb29
    i32 2, label %sw.bb33
    i32 0, label %sw.bb37
    i32 4, label %sw.bb41
  ]

sw.bb:                                            ; preds = %if.end27
  %7 = load i32, ptr %bound, align 4, !tbaa !18
  br label %cleanup1026

sw.bb29:                                          ; preds = %if.end27
  %8 = load i32, ptr %bound, align 4, !tbaa !18
  %cmp30.not = icmp sgt i32 %8, %alpha.addr.0
  br i1 %cmp30.not, label %sw.epilog, label %cleanup1026

sw.bb33:                                          ; preds = %if.end27
  %9 = load i32, ptr %bound, align 4, !tbaa !18
  %cmp34.not = icmp slt i32 %9, %beta.addr.0
  br i1 %cmp34.not, label %sw.epilog, label %cleanup1026

sw.bb37:                                          ; preds = %if.end27
  %10 = load i32, ptr %bound, align 4, !tbaa !18
  %cmp38.not = icmp slt i32 %10, %beta.addr.0
  %spec.select = select i1 %cmp38.not, i32 %cutnode, i32 1
  br label %sw.epilog

sw.bb41:                                          ; preds = %if.end27
  store i32 65535, ptr %best, align 4, !tbaa !18
  store i32 0, ptr %threat, align 4, !tbaa !18
  store i32 0, ptr %singular, align 4, !tbaa !18
  store i32 0, ptr %nosingular, align 4, !tbaa !18
  br label %sw.epilog

sw.epilog:                                        ; preds = %sw.bb37, %sw.bb33, %sw.bb29, %if.end27, %sw.bb41
  %cutnode.addr.0 = phi i32 [ %cutnode, %if.end27 ], [ %cutnode, %sw.bb41 ], [ 0, %sw.bb29 ], [ 1, %sw.bb33 ], [ %spec.select, %sw.bb37 ]
  %11 = load i32, ptr %ply, align 8, !tbaa !10
  %idxprom = sext i32 %11 to i64
  %arrayidx = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 25, i64 %idxprom
  %12 = load i32, ptr %arrayidx, align 4, !tbaa !18
  %call43 = call noundef i32 @_Z13retrieve_evalP7state_t(ptr noundef nonnull %s)
  %tobool44 = icmp ne i32 %12, 0
  %tobool44.not = xor i1 %tobool44, true
  %add45 = add nsw i32 %alpha.addr.0, 1
  %cmp46 = icmp eq i32 %beta.addr.0, %add45
  %or.cond1532 = select i1 %tobool44.not, i1 %cmp46, i1 false
  br i1 %or.cond1532, label %if.then47, label %if.end68

if.then47:                                        ; preds = %sw.epilog
  %cmp48 = icmp ult i32 %depth, 5
  br i1 %cmp48, label %if.then49, label %if.else

if.then49:                                        ; preds = %if.then47
  %sub50 = add nsw i32 %call43, -75
  %cmp51.not = icmp slt i32 %sub50, %beta.addr.0
  br i1 %cmp51.not, label %if.end54, label %if.then52

if.then52:                                        ; preds = %if.then49
  %13 = load i32, ptr %best, align 4, !tbaa !18
  %14 = load i32, ptr %threat, align 4, !tbaa !18
  %15 = load i32, ptr %singular, align 4, !tbaa !18
  %16 = load i32, ptr %nosingular, align 4, !tbaa !18
  call void @_Z7StoreTTP7state_tiiijiiii(ptr noundef nonnull %s, i32 noundef %sub50, i32 noundef %alpha.addr.0, i32 noundef %beta.addr.0, i32 noundef %13, i32 noundef %14, i32 noundef %15, i32 noundef %16, i32 noundef %depth)
  br label %cleanup1026

if.end54:                                         ; preds = %if.then49
  %cmp55 = icmp slt i32 %call43, %beta.addr.0
  br i1 %cmp55, label %if.then56, label %if.end68

if.then56:                                        ; preds = %if.end54
  %call57 = call noundef i32 @_Z7qsearchP7state_tiiii(ptr noundef nonnull %s, i32 noundef %alpha.addr.0, i32 noundef %beta.addr.0, i32 noundef 0, i32 noundef 0)
  br label %cleanup1026

if.else:                                          ; preds = %if.then47
  %cmp59 = icmp ult i32 %depth, 9
  br i1 %cmp59, label %if.then60, label %if.end68

if.then60:                                        ; preds = %if.else
  %sub61 = add nsw i32 %call43, -125
  %cmp62.not = icmp slt i32 %sub61, %beta.addr.0
  br i1 %cmp62.not, label %if.end68, label %if.then63

if.then63:                                        ; preds = %if.then60
  %17 = load i32, ptr %best, align 4, !tbaa !18
  %18 = load i32, ptr %threat, align 4, !tbaa !18
  %19 = load i32, ptr %singular, align 4, !tbaa !18
  %20 = load i32, ptr %nosingular, align 4, !tbaa !18
  call void @_Z7StoreTTP7state_tiiijiiii(ptr noundef nonnull %s, i32 noundef %sub61, i32 noundef %alpha.addr.0, i32 noundef %beta.addr.0, i32 noundef %17, i32 noundef %18, i32 noundef %19, i32 noundef %20, i32 noundef %depth)
  br label %cleanup1026

if.end68:                                         ; preds = %if.end54, %if.then60, %if.else, %sw.epilog
  %arrayidx69 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 9
  %21 = load i32, ptr %arrayidx69, align 4, !tbaa !18
  %arrayidx71 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 7
  %22 = load i32, ptr %arrayidx71, align 4, !tbaa !18
  %add72 = add nsw i32 %22, %21
  %arrayidx74 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 11
  %23 = load i32, ptr %arrayidx74, align 4, !tbaa !18
  %add75 = add nsw i32 %add72, %23
  %arrayidx77 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 3
  %24 = load i32, ptr %arrayidx77, align 4, !tbaa !18
  %add78 = add nsw i32 %add75, %24
  %arrayidx80 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 10
  %25 = load i32, ptr %arrayidx80, align 8, !tbaa !18
  %arrayidx82 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 8
  %26 = load i32, ptr %arrayidx82, align 8, !tbaa !18
  %add83 = add nsw i32 %26, %25
  %arrayidx85 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 12
  %27 = load i32, ptr %arrayidx85, align 8, !tbaa !18
  %add86 = add nsw i32 %add83, %27
  %arrayidx88 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 4
  %28 = load i32, ptr %arrayidx88, align 8, !tbaa !18
  %add89 = add nsw i32 %add86, %28
  store i32 0, ptr %threat, align 4, !tbaa !18
  %cmp90 = icmp eq i32 %is_null, 0
  br i1 %cmp90, label %land.lhs.true91, label %if.else189

land.lhs.true91:                                  ; preds = %if.end68
  %white_to_move92 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 11
  %29 = load i32, ptr %white_to_move92, align 4, !tbaa !17
  %tobool93.not = icmp eq i32 %29, 0
  %cond97 = select i1 %tobool93.not, i32 %add89, i32 %add78
  %tobool98 = icmp eq i32 %cond97, 0
  %or.cond = or i1 %tobool44, %tobool98
  %30 = load i32, ptr %donull, align 4
  %tobool102 = icmp ne i32 %30, 0
  %not.or.cond = xor i1 %or.cond, true
  %or.cond1061.not = select i1 %not.or.cond, i1 %tobool102, i1 false
  %cmp109 = icmp ugt i32 %depth, 4
  %or.cond1063 = and i1 %cmp109, %cmp46
  %or.cond1623 = select i1 %or.cond1061.not, i1 %or.cond1063, i1 false
  br i1 %or.cond1623, label %if.then110, label %if.else189

if.then110:                                       ; preds = %land.lhs.true91
  %31 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 1), align 4, !tbaa !43
  %cmp111 = icmp eq i32 %31, 2
  br i1 %cmp111, label %if.then112, label %if.then129

if.then112:                                       ; preds = %if.then110
  %cmp114 = icmp ult i32 %depth, 25
  br i1 %cmp114, label %if.then115, label %if.else118

if.then115:                                       ; preds = %if.then112
  %sub116 = add nsw i32 %beta.addr.0, -1
  %call117 = call noundef i32 @_Z7qsearchP7state_tiiii(ptr noundef nonnull %s, i32 noundef %sub116, i32 noundef %beta.addr.0, i32 noundef 0, i32 noundef 0)
  br label %if.end121

if.else118:                                       ; preds = %if.then112
  %sub113 = add nsw i32 %depth, -24
  %sub119 = add nsw i32 %beta.addr.0, -1
  %call120 = call noundef i32 @_Z6searchP7state_tiiiii(ptr noundef nonnull %s, i32 noundef %sub119, i32 noundef %beta.addr.0, i32 noundef %sub113, i32 noundef 1, i32 noundef %cutnode.addr.0)
  br label %if.end121

if.end121:                                        ; preds = %if.else118, %if.then115
  %score.0 = phi i32 [ %call117, %if.then115 ], [ %call120, %if.else118 ]
  %32 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %tobool122.not = icmp eq i32 %32, 0
  br i1 %tobool122.not, label %if.end125, label %cleanup1026

if.end125:                                        ; preds = %if.end121
  %.pre = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 1), align 4, !tbaa !43
  %cmp126.not = icmp eq i32 %.pre, 2
  %cmp128.not = icmp slt i32 %score.0, %beta.addr.0
  %or.cond1533 = and i1 %cmp128.not, %cmp126.not
  br i1 %or.cond1533, label %if.end216, label %if.then129

if.then129:                                       ; preds = %if.then110, %if.end125
  %ep_square = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 10
  %33 = load i32, ptr %ep_square, align 8, !tbaa !44
  store i32 0, ptr %ep_square, align 8, !tbaa !44
  %34 = load i32, ptr %white_to_move92, align 4, !tbaa !17
  %xor = xor i32 %34, 1
  store i32 %xor, ptr %white_to_move92, align 4, !tbaa !17
  %35 = load <2 x i32>, ptr %ply, align 8, !tbaa !18
  %36 = add nsw <2 x i32> %35, <i32 1, i32 1>
  store <2 x i32> %36, ptr %ply, align 8, !tbaa !18
  %37 = extractelement <2 x i32> %35, i64 0
  %idxprom138 = sext i32 %37 to i64
  %arrayidx139 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 19, i64 %idxprom138
  store i32 0, ptr %arrayidx139, align 4, !tbaa !18
  %38 = load i32, ptr %ply, align 8, !tbaa !10
  %idxprom142 = sext i32 %38 to i64
  %arrayidx143 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 25, i64 %idxprom142
  store i32 0, ptr %arrayidx143, align 4, !tbaa !18
  %39 = load i32, ptr %ply, align 8, !tbaa !10
  %sub145 = add nsw i32 %39, -1
  %idxprom146 = sext i32 %sub145 to i64
  %arrayidx147 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 20, i64 %idxprom146
  %40 = load i32, ptr %arrayidx147, align 4, !tbaa !18
  %idxprom150 = sext i32 %39 to i64
  %arrayidx151 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 20, i64 %idxprom150
  store i32 %40, ptr %arrayidx151, align 4, !tbaa !18
  %cmp154 = icmp ult i32 %depth, 17
  br i1 %cmp154, label %if.then155, label %if.else161

if.then155:                                       ; preds = %if.then129
  %sub156 = sub nsw i32 0, %beta.addr.0
  %add158 = sub i32 1, %beta.addr.0
  %call159 = call noundef i32 @_Z7qsearchP7state_tiiii(ptr noundef nonnull %s, i32 noundef %sub156, i32 noundef %add158, i32 noundef 0, i32 noundef 0)
  br label %if.end168

if.else161:                                       ; preds = %if.then129
  %sub153 = add nsw i32 %depth, -16
  %sub162 = sub nsw i32 0, %beta.addr.0
  %add164 = sub i32 1, %beta.addr.0
  %tobool165.not = icmp eq i32 %cutnode.addr.0, 0
  %conv = zext i1 %tobool165.not to i32
  %call166 = call noundef i32 @_Z6searchP7state_tiiiii(ptr noundef nonnull %s, i32 noundef %sub162, i32 noundef %add164, i32 noundef %sub153, i32 noundef 1, i32 noundef %conv)
  br label %if.end168

if.end168:                                        ; preds = %if.else161, %if.then155
  %call159.pn = phi i32 [ %call159, %if.then155 ], [ %call166, %if.else161 ]
  %score.2 = sub nsw i32 0, %call159.pn
  %41 = load <2 x i32>, ptr %ply, align 8, !tbaa !18
  %42 = add nsw <2 x i32> %41, <i32 -1, i32 -1>
  store <2 x i32> %42, ptr %ply, align 8, !tbaa !18
  %43 = load i32, ptr %white_to_move92, align 4, !tbaa !17
  %xor173 = xor i32 %43, 1
  store i32 %xor173, ptr %white_to_move92, align 4, !tbaa !17
  store i32 %33, ptr %ep_square, align 8, !tbaa !44
  %44 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %tobool175.not = icmp eq i32 %44, 0
  br i1 %tobool175.not, label %if.end177, label %cleanup1026

if.end177:                                        ; preds = %if.end168
  %cmp178.not = icmp sgt i32 %beta.addr.0, %score.2
  br i1 %cmp178.not, label %if.else180, label %if.then179

if.then179:                                       ; preds = %if.end177
  %45 = load i32, ptr %best, align 4, !tbaa !18
  %46 = load i32, ptr %threat, align 4, !tbaa !18
  %47 = load i32, ptr %nosingular, align 4, !tbaa !18
  call void @_Z7StoreTTP7state_tiiijiiii(ptr noundef nonnull %s, i32 noundef %score.2, i32 noundef %alpha.addr.0, i32 noundef %beta.addr.0, i32 noundef %45, i32 noundef %46, i32 noundef 0, i32 noundef %47, i32 noundef %depth)
  br label %cleanup1026

if.else180:                                       ; preds = %if.end177
  %cmp181 = icmp sgt i32 %call159.pn, 31400
  br i1 %cmp181, label %if.then182, label %if.end216

if.then182:                                       ; preds = %if.else180
  store i32 1, ptr %threat, align 4, !tbaa !18
  br label %if.end216

if.else189:                                       ; preds = %land.lhs.true91, %if.end68
  %cmp193 = icmp ult i32 %depth, 13
  %or.cond1064 = and i1 %cmp193, %cmp46
  %sub195 = add nsw i32 %beta.addr.0, -300
  %cmp196 = icmp slt i32 %call43, %sub195
  %or.cond1534 = select i1 %or.cond1064, i1 %cmp196, i1 false
  br i1 %or.cond1534, label %if.then197, label %if.end209

if.then197:                                       ; preds = %if.else189
  %call198 = call noundef i32 @_Z7qsearchP7state_tiiii(ptr noundef nonnull %s, i32 noundef %alpha.addr.0, i32 noundef %beta.addr.0, i32 noundef 0, i32 noundef 0)
  %48 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %tobool199.not = icmp eq i32 %48, 0
  br i1 %tobool199.not, label %if.end201, label %cleanup1026

if.end201:                                        ; preds = %if.then197
  %cmp202.not = icmp sgt i32 %call198, %alpha.addr.0
  br i1 %cmp202.not, label %if.end209, label %if.then203

if.then203:                                       ; preds = %if.end201
  %49 = load i32, ptr %best, align 4, !tbaa !18
  %50 = load i32, ptr %threat, align 4, !tbaa !18
  %51 = load i32, ptr %singular, align 4, !tbaa !18
  %52 = load i32, ptr %nosingular, align 4, !tbaa !18
  call void @_Z7StoreTTP7state_tiiijiiii(ptr noundef nonnull %s, i32 noundef %alpha.addr.0, i32 noundef %alpha.addr.0, i32 noundef %beta.addr.0, i32 noundef %49, i32 noundef %50, i32 noundef %51, i32 noundef %52, i32 noundef %depth)
  br label %cleanup1026

if.end209:                                        ; preds = %if.end201, %if.else189
  br i1 %tobool44, label %if.then218, label %if.end216

if.end216:                                        ; preds = %if.then182, %if.else180, %if.end125, %if.end209
  %call215 = call noundef i32 @_Z3genP7state_tPi(ptr noundef nonnull %s, ptr noundef nonnull %moves)
  br label %if.end237

if.then218:                                       ; preds = %if.end209
  %call212 = call noundef i32 @_Z12gen_evasionsP7state_tPii(ptr noundef nonnull %s, ptr noundef nonnull %moves, i32 noundef %12)
  %tobool219.not = icmp eq i32 %call212, 0
  br i1 %tobool219.not, label %if.end237, label %for.cond.preheader

for.cond.preheader:                               ; preds = %if.then218
  %cmp2211636 = icmp sgt i32 %call212, 0
  br i1 %cmp2211636, label %for.body.preheader, label %if.end237

for.body.preheader:                               ; preds = %for.cond.preheader
  %53 = zext i32 %call212 to i64
  br label %for.body

for.body:                                         ; preds = %for.body.preheader, %for.body
  %indvars.iv = phi i64 [ 0, %for.body.preheader ], [ %indvars.iv.next, %for.body ]
  %legalmoves.01638 = phi i32 [ 0, %for.body.preheader ], [ %spec.select1535, %for.body ]
  %arrayidx224 = getelementptr inbounds [240 x i32], ptr %moves, i64 0, i64 %indvars.iv
  %54 = load i32, ptr %arrayidx224, align 4, !tbaa !18
  call void @_Z4makeP7state_ti(ptr noundef %s, i32 noundef %54)
  %55 = load i32, ptr %arrayidx224, align 4, !tbaa !18
  %call227 = call noundef i32 @_Z11check_legalP7state_ti(ptr noundef %s, i32 noundef %55)
  %tobool228.not = icmp ne i32 %call227, 0
  %inc230 = zext i1 %tobool228.not to i32
  %spec.select1535 = add nuw nsw i32 %legalmoves.01638, %inc230
  %56 = load i32, ptr %arrayidx224, align 4, !tbaa !18
  call void @_Z6unmakeP7state_ti(ptr noundef %s, i32 noundef %56)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %cmp221 = icmp ult i64 %indvars.iv.next, %53
  %cmp222 = icmp ult i32 %spec.select1535, 2
  %57 = select i1 %cmp221, i1 %cmp222, i1 false
  br i1 %57, label %for.body, label %if.end237, !llvm.loop !45

if.end237:                                        ; preds = %for.body, %for.cond.preheader, %if.end216, %if.then218
  %num_moves.01609 = phi i32 [ 0, %if.then218 ], [ %call215, %if.end216 ], [ %call212, %for.cond.preheader ], [ %call212, %for.body ]
  %legalmoves.2 = phi i32 [ 0, %if.then218 ], [ %call215, %if.end216 ], [ 0, %for.cond.preheader ], [ %spec.select1535, %for.body ]
  %58 = load i32, ptr %best, align 4, !tbaa !18
  call fastcc void @_ZL11order_movesP7state_tPiS1_ij(ptr noundef %s, ptr noundef nonnull %moves, ptr noundef nonnull %move_ordering, i32 noundef %num_moves.01609, i32 noundef %58)
  %cmp240 = icmp sgt i32 %depth, 19
  br i1 %cmp240, label %land.lhs.true241, label %if.end288

land.lhs.true241:                                 ; preds = %if.end237
  %cmp243 = icmp ne i32 %beta.addr.0, %add45
  %59 = load i32, ptr %best, align 4
  %cmp245 = icmp eq i32 %59, 65535
  %or.cond1065 = select i1 %cmp243, i1 %cmp245, i1 false
  br i1 %or.cond1065, label %for.cond247.preheader, label %if.end288

for.cond247.preheader:                            ; preds = %land.lhs.true241
  %cmp2481640 = icmp sgt i32 %num_moves.01609, 0
  br i1 %cmp2481640, label %for.body249.preheader, label %for.end273.thread

for.end273.thread:                                ; preds = %for.cond247.preheader
  store i32 0, ptr %i, align 4, !tbaa !18
  br label %if.then275

for.body249.preheader:                            ; preds = %for.cond247.preheader
  %wide.trip.count = zext i32 %num_moves.01609 to i64
  br label %for.body249

for.body249:                                      ; preds = %for.body249.preheader, %for.inc271
  %indvars.iv1698 = phi i64 [ 0, %for.body249.preheader ], [ %indvars.iv.next1699, %for.inc271 ]
  %goodmove.01642 = phi i32 [ 0, %for.body249.preheader ], [ %goodmove.1, %for.inc271 ]
  %arrayidx251 = getelementptr inbounds [240 x i32], ptr %moves, i64 0, i64 %indvars.iv1698
  %60 = load i32, ptr %arrayidx251, align 4, !tbaa !18
  %61 = lshr i32 %60, 19
  %and = and i32 %61, 15
  %cmp252.not = icmp eq i32 %and, 13
  br i1 %cmp252.not, label %for.inc271, label %land.lhs.true253

land.lhs.true253:                                 ; preds = %for.body249
  %62 = lshr i32 %60, 6
  %and262 = and i32 %62, 63
  %idxprom263 = zext i32 %and262 to i64
  %arrayidx264 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 1, i64 %idxprom263
  %63 = load i32, ptr %arrayidx264, align 4, !tbaa !18
  %idxprom265 = sext i32 %63 to i64
  %arrayidx266 = getelementptr inbounds [14 x i32], ptr @material, i64 0, i64 %idxprom265
  %64 = load i32, ptr %arrayidx266, align 4, !tbaa !18
  %call267 = call i32 @llvm.abs.i32(i32 %64, i1 true)
  %cmp268 = icmp ugt i32 %and, %call267
  %spec.select1536 = select i1 %cmp268, i32 1, i32 %goodmove.01642
  br label %for.inc271

for.inc271:                                       ; preds = %land.lhs.true253, %for.body249
  %goodmove.1 = phi i32 [ %goodmove.01642, %for.body249 ], [ %spec.select1536, %land.lhs.true253 ]
  %indvars.iv.next1699 = add nuw nsw i64 %indvars.iv1698, 1
  %exitcond.not = icmp eq i64 %indvars.iv.next1699, %wide.trip.count
  br i1 %exitcond.not, label %for.end273, label %for.body249, !llvm.loop !46

for.end273:                                       ; preds = %for.inc271
  %tobool274.not = icmp eq i32 %goodmove.1, 0
  br i1 %tobool274.not, label %if.then275, label %if.end288

if.then275:                                       ; preds = %for.end273.thread, %for.end273
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %tmp) #9
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %mv) #9
  %shr276 = lshr i32 %depth, 1
  %call277 = call noundef i32 @_Z6searchP7state_tiiiii(ptr noundef %s, i32 noundef %alpha.addr.0, i32 noundef %beta.addr.0, i32 noundef %shr276, i32 noundef 0, i32 noundef %cutnode.addr.0)
  %call278 = call noundef i32 @_Z7ProbeTTP7state_tPiiiPjS1_S1_S1_S1_i(ptr noundef %s, ptr noundef nonnull %tmp, i32 noundef 0, i32 noundef 0, ptr noundef nonnull %mv, ptr noundef nonnull %tmp, ptr noundef nonnull %tmp, ptr noundef nonnull %tmp, ptr noundef nonnull %tmp, i32 noundef 0)
  %cmp279.not = icmp eq i32 %call278, 4
  %65 = load i32, ptr %best, align 4
  %66 = load i32, ptr %mv, align 4
  %.sink = select i1 %cmp279.not, i32 %65, i32 %66
  call fastcc void @_ZL11order_movesP7state_tPiS1_ij(ptr noundef %s, ptr noundef nonnull %moves, ptr noundef nonnull %move_ordering, i32 noundef %num_moves.01609, i32 noundef %.sink)
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %mv) #9
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %tmp) #9
  br label %if.end288

if.end288:                                        ; preds = %for.end273, %if.then275, %land.lhs.true241, %if.end237
  %67 = load i32, ptr %threat, align 4
  %tobool291 = icmp eq i32 %67, 0
  %or.cond1066 = select i1 %tobool44.not, i1 %tobool291, i1 false
  %cmp293 = icmp sgt i32 %depth, 15
  %or.cond1067 = and i1 %cmp293, %or.cond1066
  %cmp295 = icmp sgt i32 %legalmoves.2, 8
  %or.cond1068 = select i1 %or.cond1067, i1 %cmp295, i1 false
  br i1 %or.cond1068, label %land.lhs.true296, label %if.end405

land.lhs.true296:                                 ; preds = %if.end288
  %68 = load i32, ptr %ply, align 8, !tbaa !10
  %sub299 = add nsw i32 %68, -1
  %idxprom300 = sext i32 %sub299 to i64
  %arrayidx301 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 25, i64 %idxprom300
  %69 = load i32, ptr %arrayidx301, align 4, !tbaa !18
  %tobool302.not = icmp eq i32 %69, 0
  br i1 %tobool302.not, label %land.lhs.true303, label %if.end405

land.lhs.true303:                                 ; preds = %land.lhs.true296
  %cmp305 = icmp slt i32 %68, 3
  br i1 %cmp305, label %if.then323, label %lor.lhs.false306

lor.lhs.false306:                                 ; preds = %land.lhs.true303
  %sub309 = add nsw i32 %68, -2
  %idxprom310 = zext i32 %sub309 to i64
  %arrayidx311 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 25, i64 %idxprom310
  %70 = load i32, ptr %arrayidx311, align 4, !tbaa !18
  %tobool312.not = icmp eq i32 %70, 0
  br i1 %tobool312.not, label %land.lhs.true313, label %if.end405

land.lhs.true313:                                 ; preds = %lor.lhs.false306
  %cmp315 = icmp ult i32 %68, 4
  br i1 %cmp315, label %if.then323, label %lor.lhs.false316

lor.lhs.false316:                                 ; preds = %land.lhs.true313
  %sub319 = add nsw i32 %68, -3
  %idxprom320 = zext i32 %sub319 to i64
  %arrayidx321 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 25, i64 %idxprom320
  %71 = load i32, ptr %arrayidx321, align 4, !tbaa !18
  %tobool322.not = icmp eq i32 %71, 0
  br i1 %tobool322.not, label %if.then323, label %if.end405

if.then323:                                       ; preds = %land.lhs.true303, %lor.lhs.false316, %land.lhs.true313
  store i32 -1, ptr %i, align 4, !tbaa !18
  %call326 = call fastcc noundef i32 @_ZL15remove_one_fastPiS_S_i(ptr noundef nonnull %i, ptr noundef nonnull %move_ordering, ptr noundef nonnull %moves, i32 noundef %num_moves.01609), !range !47
  %tobool3271645.not = icmp eq i32 %call326, 0
  br i1 %tobool3271645.not, label %if.end405, label %while.body.lr.ph

while.body.lr.ph:                                 ; preds = %if.then323
  %hash = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 16
  %sub358 = sub nsw i32 0, %beta.addr.0
  %add360 = sub i32 50, %alpha.addr.0
  %cmp361 = icmp ugt i32 %depth, 16
  %cmp365 = icmp ult i32 %depth, 17
  %sub357 = add nsw i32 %depth, -16
  %add375 = sub i32 1, %beta.addr.0
  %tobool376.not = icmp eq i32 %cutnode.addr.0, 0
  %conv378 = zext i1 %tobool376.not to i32
  br label %while.body

while.body:                                       ; preds = %while.body.lr.ph, %if.end397
  %m_c.01648 = phi i32 [ 0, %while.body.lr.ph ], [ %m_c.1, %if.end397 ]
  %m_s.01647 = phi i32 [ 0, %while.body.lr.ph ], [ %inc331, %if.end397 ]
  %score.31646 = phi i32 [ -32000, %while.body.lr.ph ], [ %score.5, %if.end397 ]
  %inc331 = add nuw nsw i32 %m_s.01647, 1
  %72 = load i32, ptr %i, align 4, !tbaa !18
  %idxprom332 = sext i32 %72 to i64
  %arrayidx333 = getelementptr inbounds [240 x i32], ptr %moves, i64 0, i64 %idxprom332
  %73 = load i32, ptr %arrayidx333, align 4, !tbaa !18
  call void @_Z4makeP7state_ti(ptr noundef %s, i32 noundef %73)
  %74 = load i32, ptr %arrayidx333, align 4, !tbaa !18
  %call336 = call noundef i32 @_Z11check_legalP7state_ti(ptr noundef %s, i32 noundef %74)
  %tobool337.not = icmp ne i32 %call336, 0
  br i1 %tobool337.not, label %if.then338, label %if.end382

if.then338:                                       ; preds = %while.body
  %75 = load i64, ptr %hash, align 8, !tbaa !26
  %76 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 15), align 4, !tbaa !27
  %77 = load i32, ptr %ply, align 8, !tbaa !10
  %add340 = add i32 %77, -1
  %sub341 = add i32 %add340, %76
  %idxprom342 = sext i32 %sub341 to i64
  %arrayidx343 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 36, i64 %idxprom342
  store i64 %75, ptr %arrayidx343, align 8, !tbaa !6
  %78 = load i32, ptr %arrayidx333, align 4, !tbaa !18
  %idxprom349 = sext i32 %add340 to i64
  %arrayidx350 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 19, i64 %idxprom349
  store i32 %78, ptr %arrayidx350, align 4, !tbaa !18
  %call351 = call noundef i32 @_Z8in_checkP7state_t(ptr noundef %s)
  %79 = load i32, ptr %ply, align 8, !tbaa !10
  %idxprom354 = sext i32 %79 to i64
  %arrayidx355 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 25, i64 %idxprom354
  store i32 %call351, ptr %arrayidx355, align 4, !tbaa !18
  %tobool362 = icmp ne i32 %call351, 0
  %80 = or i1 %cmp361, %tobool362
  %conv363 = zext i1 %80 to i32
  %call364 = call noundef i32 @_Z4evalP7state_tiii(ptr noundef %s, i32 noundef %sub358, i32 noundef %add360, i32 noundef %conv363)
  br i1 %cmp365, label %if.then366, label %if.else372

if.then366:                                       ; preds = %if.then338
  %call370 = call noundef i32 @_Z7qsearchP7state_tiiii(ptr noundef nonnull %s, i32 noundef %sub358, i32 noundef %add375, i32 noundef 0, i32 noundef 0)
  br label %if.end381

if.else372:                                       ; preds = %if.then338
  %call379 = call noundef i32 @_Z6searchP7state_tiiiii(ptr noundef nonnull %s, i32 noundef %sub358, i32 noundef %add375, i32 noundef %sub357, i32 noundef 0, i32 noundef %conv378)
  br label %if.end381

if.end381:                                        ; preds = %if.else372, %if.then366
  %call370.pn = phi i32 [ %call370, %if.then366 ], [ %call379, %if.else372 ]
  %score.4 = sub nsw i32 0, %call370.pn
  br label %if.end382

if.end382:                                        ; preds = %if.end381, %while.body
  %score.5 = phi i32 [ %score.4, %if.end381 ], [ %score.31646, %while.body ]
  %81 = load i32, ptr %arrayidx333, align 4, !tbaa !18
  call void @_Z6unmakeP7state_ti(ptr noundef %s, i32 noundef %81)
  %82 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %tobool385.not = icmp eq i32 %82, 0
  br i1 %tobool385.not, label %if.then386, label %if.end405.loopexit

if.then386:                                       ; preds = %if.end382
  %cmp387 = icmp sge i32 %score.5, %beta.addr.0
  %or.cond1069 = and i1 %tobool337.not, %cmp387
  br i1 %or.cond1069, label %if.then390, label %if.end397

if.then390:                                       ; preds = %if.then386
  %cmp392 = icmp sgt i32 %m_c.01648, 0
  br i1 %cmp392, label %cleanup401, label %if.end397

if.end397:                                        ; preds = %if.then386, %if.then390
  %m_c.1 = phi i32 [ 1, %if.then390 ], [ %m_c.01648, %if.then386 ]
  %call400 = call fastcc noundef i32 @_ZL15remove_one_fastPiS_S_i(ptr noundef nonnull %i, ptr noundef nonnull %move_ordering, ptr noundef nonnull %moves, i32 noundef %num_moves.01609), !range !47
  %tobool327 = icmp ne i32 %call400, 0
  %cmp329 = icmp ult i32 %m_s.01647, 2
  %83 = select i1 %tobool327, i1 %cmp329, i1 false
  br i1 %83, label %while.body, label %if.end405.loopexit, !llvm.loop !48

cleanup401:                                       ; preds = %if.then390
  %84 = load i32, ptr %best, align 4, !tbaa !18
  %85 = load i32, ptr %threat, align 4, !tbaa !18
  %86 = load i32, ptr %nosingular, align 4, !tbaa !18
  call void @_Z7StoreTTP7state_tiiijiiii(ptr noundef %s, i32 noundef %beta.addr.0, i32 noundef %alpha.addr.0, i32 noundef %beta.addr.0, i32 noundef %84, i32 noundef %85, i32 noundef 0, i32 noundef %86, i32 noundef %depth)
  br label %cleanup1026

if.end405.loopexit:                               ; preds = %if.end397, %if.end382
  %.pre1712 = load i32, ptr %threat, align 4
  br label %if.end405

if.end405:                                        ; preds = %if.end405.loopexit, %if.then323, %lor.lhs.false316, %lor.lhs.false306, %land.lhs.true296, %if.end288
  %87 = phi i32 [ 0, %land.lhs.true296 ], [ 0, %lor.lhs.false316 ], [ 0, %lor.lhs.false306 ], [ %67, %if.end288 ], [ 0, %if.then323 ], [ %.pre1712, %if.end405.loopexit ]
  %score.8 = phi i32 [ -32000, %land.lhs.true296 ], [ -32000, %lor.lhs.false316 ], [ -32000, %lor.lhs.false306 ], [ -32000, %if.end288 ], [ -32000, %if.then323 ], [ %score.5, %if.end405.loopexit ]
  %88 = load i32, ptr %singular, align 4, !tbaa !18
  %tobool406 = icmp eq i32 %88, 0
  %89 = load i32, ptr %nosingular, align 4
  %tobool408 = icmp eq i32 %89, 0
  %or.cond1070.not = select i1 %tobool406, i1 %tobool408, i1 false
  %tobool410 = icmp eq i32 %87, 0
  %or.cond1071 = select i1 %or.cond1070.not, i1 %tobool410, i1 false
  %or.cond1072 = and i1 %cmp240, %or.cond1071
  %cmp414 = icmp sgt i32 %legalmoves.2, 1
  %or.cond1073 = select i1 %or.cond1072, i1 %cmp414, i1 false
  %90 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 1), align 4
  %cmp416 = icmp ne i32 %90, 2
  %or.cond1074 = select i1 %or.cond1073, i1 %cmp416, i1 false
  br i1 %or.cond1074, label %if.then417, label %if.end507

if.then417:                                       ; preds = %if.end405
  %sub418 = add nsw i32 %depth, -24
  %call419 = call noundef i32 @_Z6searchP7state_tiiiii(ptr noundef %s, i32 noundef %alpha.addr.0, i32 noundef %beta.addr.0, i32 noundef %sub418, i32 noundef 0, i32 noundef %cutnode.addr.0)
  %cmp420 = icmp sgt i32 %call419, %alpha.addr.0
  br i1 %cmp420, label %if.then421, label %if.end507

if.then421:                                       ; preds = %if.then417
  store i32 -1, ptr %i, align 4, !tbaa !18
  %call424 = call fastcc noundef i32 @_ZL15remove_one_fastPiS_S_i(ptr noundef nonnull %i, ptr noundef nonnull %move_ordering, ptr noundef nonnull %moves, i32 noundef %num_moves.01609), !range !47
  %tobool4261652 = icmp ne i32 %call424, 0
  %91 = load i32, ptr %singular, align 4
  %cmp4281653 = icmp slt i32 %91, 2
  %or.cond10751654 = select i1 %tobool4261652, i1 %cmp4281653, i1 false
  br i1 %or.cond10751654, label %while.body432.lr.ph, label %if.end507

while.body432.lr.ph:                              ; preds = %if.then421
  %i.promoted = load i32, ptr %i, align 4, !tbaa !18
  %hash440 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 16
  %sub461 = add nsw i32 %depth, -16
  %sub462 = sub nsw i32 0, %beta.addr.0
  %add464 = sub i32 50, %alpha.addr.0
  %sub463 = sub nsw i32 0, %alpha.addr.0
  %sub474 = xor i32 %alpha.addr.0, -1
  %tobool476.not = icmp eq i32 %cutnode.addr.0, 0
  %conv478 = zext i1 %tobool476.not to i32
  %sub488 = sub i32 49, %alpha.addr.0
  %sub489 = add nsw i32 %alpha.addr.0, -50
  %92 = sext i32 %i.promoted to i64
  %93 = sext i32 %num_moves.01609 to i64
  br label %while.body432

while.body432:                                    ; preds = %while.body432.lr.ph, %_ZL15remove_one_fastPiS_S_i.exit
  %indvars.iv1701 = phi i64 [ %92, %while.body432.lr.ph ], [ %indvars.iv.next1702, %_ZL15remove_one_fastPiS_S_i.exit ]
  %s_c.01658 = phi i32 [ 0, %while.body432.lr.ph ], [ %s_c.2, %_ZL15remove_one_fastPiS_S_i.exit ]
  %score.91657 = phi i32 [ %score.8, %while.body432.lr.ph ], [ %score.11, %_ZL15remove_one_fastPiS_S_i.exit ]
  %first.01656 = phi i32 [ 1, %while.body432.lr.ph ], [ %first.1, %_ZL15remove_one_fastPiS_S_i.exit ]
  %arrayidx434 = getelementptr inbounds [240 x i32], ptr %moves, i64 0, i64 %indvars.iv1701
  %94 = load i32, ptr %arrayidx434, align 4, !tbaa !18
  call void @_Z4makeP7state_ti(ptr noundef %s, i32 noundef %94)
  %95 = load i32, ptr %arrayidx434, align 4, !tbaa !18
  %call437 = call noundef i32 @_Z11check_legalP7state_ti(ptr noundef %s, i32 noundef %95)
  %tobool438.not = icmp eq i32 %call437, 0
  br i1 %tobool438.not, label %if.end499, label %if.then439

if.then439:                                       ; preds = %while.body432
  %96 = load i64, ptr %hash440, align 8, !tbaa !26
  %97 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 15), align 4, !tbaa !27
  %98 = load i32, ptr %ply, align 8, !tbaa !10
  %add443 = add i32 %98, -1
  %sub444 = add i32 %add443, %97
  %idxprom445 = sext i32 %sub444 to i64
  %arrayidx446 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 36, i64 %idxprom445
  store i64 %96, ptr %arrayidx446, align 8, !tbaa !6
  %99 = load i32, ptr %arrayidx434, align 4, !tbaa !18
  %idxprom452 = sext i32 %add443 to i64
  %arrayidx453 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 19, i64 %idxprom452
  store i32 %99, ptr %arrayidx453, align 4, !tbaa !18
  %inc454 = add nsw i32 %s_c.01658, 1
  %call455 = call noundef i32 @_Z8in_checkP7state_t(ptr noundef %s)
  %100 = load i32, ptr %ply, align 8, !tbaa !10
  %idxprom458 = sext i32 %100 to i64
  %arrayidx459 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 25, i64 %idxprom458
  store i32 %call455, ptr %arrayidx459, align 4, !tbaa !18
  %call470 = call noundef i32 @_Z4evalP7state_tiii(ptr noundef %s, i32 noundef %sub462, i32 noundef %add464, i32 noundef 1)
  %tobool471.not = icmp eq i32 %first.01656, 0
  br i1 %tobool471.not, label %if.else486, label %if.then472

if.then472:                                       ; preds = %if.then439
  %call479 = call noundef i32 @_Z6searchP7state_tiiiii(ptr noundef nonnull %s, i32 noundef %sub474, i32 noundef %sub463, i32 noundef %sub461, i32 noundef 0, i32 noundef %conv478)
  %sub480 = sub nsw i32 0, %call479
  %cmp481 = icmp slt i32 %alpha.addr.0, %sub480
  br i1 %cmp481, label %if.then482, label %if.else483

if.then482:                                       ; preds = %if.then472
  store i32 1, ptr %singular, align 4, !tbaa !18
  br label %if.end499

if.else483:                                       ; preds = %if.then472
  store i32 0, ptr %singular, align 4, !tbaa !18
  %add484 = add nsw i32 %s_c.01658, 11
  br label %if.end499

if.else486:                                       ; preds = %if.then439
  %call491 = call noundef i32 @_Z6searchP7state_tiiiii(ptr noundef nonnull %s, i32 noundef %sub488, i32 noundef %add464, i32 noundef %sub461, i32 noundef 0, i32 noundef 0)
  %sub492 = sub nsw i32 0, %call491
  %cmp494 = icmp slt i32 %sub489, %sub492
  br i1 %cmp494, label %if.then495, label %if.end499

if.then495:                                       ; preds = %if.else486
  store i32 0, ptr %singular, align 4, !tbaa !18
  %add496 = add nsw i32 %s_c.01658, 11
  br label %if.end499

if.end499:                                        ; preds = %if.else483, %if.then482, %if.then495, %if.else486, %while.body432
  %first.1 = phi i32 [ %first.01656, %while.body432 ], [ 0, %if.else486 ], [ 0, %if.then495 ], [ 0, %if.then482 ], [ 0, %if.else483 ]
  %score.11 = phi i32 [ %score.91657, %while.body432 ], [ %sub492, %if.else486 ], [ %sub492, %if.then495 ], [ %sub480, %if.then482 ], [ %sub480, %if.else483 ]
  %s_c.2 = phi i32 [ %s_c.01658, %while.body432 ], [ %inc454, %if.else486 ], [ %add496, %if.then495 ], [ %inc454, %if.then482 ], [ %add484, %if.else483 ]
  %101 = load i32, ptr %arrayidx434, align 4, !tbaa !18
  call void @_Z6unmakeP7state_ti(ptr noundef %s, i32 noundef %101)
  %indvars.iv.next1702 = add i64 %indvars.iv1701, 1
  %cmp.i = icmp slt i64 %indvars.iv1701, 9
  %cmp165.i = icmp slt i64 %indvars.iv.next1702, %93
  br i1 %cmp.i, label %for.cond.preheader.i, label %_ZL15remove_one_fastPiS_S_i.exit

for.cond.preheader.i:                             ; preds = %if.end499
  br i1 %cmp165.i, label %for.body.i, label %if.end507

for.body.i:                                       ; preds = %for.cond.preheader.i, %for.body.i
  %indvars.iv.i = phi i64 [ %indvars.iv.next.i, %for.body.i ], [ %indvars.iv.next1702, %for.cond.preheader.i ]
  %tmp.068.i = phi i32 [ %spec.select60.i, %for.body.i ], [ undef, %for.cond.preheader.i ]
  %best.067.i = phi i32 [ %spec.select.i, %for.body.i ], [ -1000000, %for.cond.preheader.i ]
  %arrayidx.i = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.i
  %102 = load i32, ptr %arrayidx.i, align 4, !tbaa !18
  %cmp2.i = icmp sgt i32 %102, %best.067.i
  %spec.select.i = call i32 @llvm.smax.i32(i32 %102, i32 %best.067.i)
  %103 = trunc i64 %indvars.iv.i to i32
  %spec.select60.i = select i1 %cmp2.i, i32 %103, i32 %tmp.068.i
  %indvars.iv.next.i = add nsw i64 %indvars.iv.i, 1
  %lftr.wideiv.i = trunc i64 %indvars.iv.next.i to i32
  %exitcond.not.i = icmp eq i32 %num_moves.01609, %lftr.wideiv.i
  br i1 %exitcond.not.i, label %if.end12.i, label %for.body.i, !llvm.loop !20

if.end12.i:                                       ; preds = %for.body.i
  %cmp13.i = icmp sgt i32 %spec.select.i, -1000000
  br i1 %cmp13.i, label %if.then14.i, label %if.end507

if.then14.i:                                      ; preds = %if.end12.i
  %104 = sext i32 %spec.select60.i to i64
  %arrayidx16.i = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.next1702
  %105 = load i32, ptr %arrayidx16.i, align 4, !tbaa !18
  %arrayidx18.i = getelementptr inbounds i32, ptr %move_ordering, i64 %104
  store i32 %105, ptr %arrayidx18.i, align 4, !tbaa !18
  store i32 %spec.select.i, ptr %arrayidx16.i, align 4, !tbaa !18
  %arrayidx22.i = getelementptr inbounds i32, ptr %moves, i64 %indvars.iv.next1702
  %106 = load i32, ptr %arrayidx22.i, align 4, !tbaa !18
  %arrayidx24.i = getelementptr inbounds i32, ptr %moves, i64 %104
  %107 = load i32, ptr %arrayidx24.i, align 4, !tbaa !18
  store i32 %107, ptr %arrayidx22.i, align 4, !tbaa !18
  store i32 %106, ptr %arrayidx24.i, align 4, !tbaa !18
  br label %_ZL15remove_one_fastPiS_S_i.exit

_ZL15remove_one_fastPiS_S_i.exit:                 ; preds = %if.end499, %if.then14.i
  %retval.0.i = phi i1 [ true, %if.then14.i ], [ %cmp165.i, %if.end499 ]
  %108 = load i32, ptr %singular, align 4
  %cmp428 = icmp slt i32 %108, 2
  %or.cond1075 = select i1 %retval.0.i, i1 %cmp428, i1 false
  %cmp430 = icmp slt i32 %s_c.2, 3
  %or.cond1091 = select i1 %or.cond1075, i1 %cmp430, i1 false
  br i1 %or.cond1091, label %while.body432, label %if.end507, !llvm.loop !49

if.end507:                                        ; preds = %for.cond.preheader.i, %if.end12.i, %_ZL15remove_one_fastPiS_S_i.exit, %if.then421, %if.then417, %if.end405
  %score.13 = phi i32 [ %score.8, %if.end405 ], [ %score.8, %if.then417 ], [ %score.8, %if.then421 ], [ %score.11, %_ZL15remove_one_fastPiS_S_i.exit ], [ %score.11, %if.end12.i ], [ %score.11, %for.cond.preheader.i ]
  br i1 %cmp46, label %land.end514, label %land.rhs510

land.rhs510:                                      ; preds = %if.end507
  %109 = load i32, ptr %ply, align 8, !tbaa !10
  %110 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4, !tbaa !19
  %mul = shl nsw i32 %110, 1
  %cmp512 = icmp sle i32 %109, %mul
  br label %land.end514

land.end514:                                      ; preds = %land.rhs510, %if.end507
  %111 = phi i1 [ false, %if.end507 ], [ %cmp512, %land.rhs510 ]
  store i32 -1, ptr %i, align 4, !tbaa !18
  %call518 = call fastcc noundef i32 @_ZL15remove_one_fastPiS_S_i(ptr noundef nonnull %i, ptr noundef nonnull %move_ordering, ptr noundef nonnull %moves, i32 noundef %num_moves.01609), !range !47
  %tobool520.not16631669 = icmp eq i32 %call518, 0
  br i1 %tobool520.not16631669, label %while.end998, label %while.body521.lr.ph.lr.ph

while.body521.lr.ph.lr.ph:                        ; preds = %land.end514
  %cmp527 = icmp eq i32 %legalmoves.2, 1
  %or.cond1076 = select i1 %tobool44, i1 %cmp527, i1 false
  %spec.select1537 = select i1 %or.cond1076, i32 4, i32 0
  %add562 = or i32 %spec.select1537, 2
  %spec.select1546 = select i1 %111, i32 %add562, i32 %spec.select1537
  %white_to_move610 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 11
  %. = select i1 %111, i32 3, i32 1
  %add677 = add nsw i32 %add89, %add78
  %cmp678 = icmp eq i32 %add677, 1
  %div1529 = lshr i32 %depth, 2
  %add690 = add nuw nsw i32 %div1529, 1
  %cmp700 = icmp slt i32 %depth, 25
  %cmp717 = icmp slt i32 %depth, 9
  %cmp728 = icmp ult i32 %depth, 13
  %add730 = add nsw i32 %call43, 100
  %add734 = add nsw i32 %call43, 300
  %add719 = add nsw i32 %call43, 75
  %add723 = add nsw i32 %call43, 200
  %sub825 = sub nsw i32 0, %beta.addr.0
  %hash838 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 16
  %cmp852 = icmp sgt i32 %depth, 4
  %tobool892.not = icmp eq i32 %cutnode.addr.0, 0
  %conv894 = zext i1 %tobool892.not to i32
  %112 = add i32 %num_moves.01609, -2
  br label %while.body521.lr.ph

while.body521.lr.ph:                              ; preds = %while.body521.lr.ph.lr.ph, %cleanup993
  %alpha.addr.1.ph1675 = phi i32 [ %alpha.addr.0, %while.body521.lr.ph.lr.ph ], [ %alpha.addr.4, %cleanup993 ]
  %score.14.ph1674 = phi i32 [ %score.13, %while.body521.lr.ph.lr.ph ], [ %score.18, %cleanup993 ]
  %no_moves.0.ph1673 = phi i32 [ 1, %while.body521.lr.ph.lr.ph ], [ %no_moves.2, %cleanup993 ]
  %best_score.0.ph1672 = phi i32 [ -32000, %while.body521.lr.ph.lr.ph ], [ %best_score.3, %cleanup993 ]
  %first.2.ph1671 = phi i32 [ 1, %while.body521.lr.ph.lr.ph ], [ %first.4, %cleanup993 ]
  %mn.0.ph1670 = phi i32 [ 1, %while.body521.lr.ph.lr.ph ], [ %mn.2, %cleanup993 ]
  %tobool639 = icmp ne i32 %first.2.ph1671, 0
  %cmp691 = icmp sgt i32 %mn.0.ph1670, %add690
  %add704 = add nsw i32 %alpha.addr.1.ph1675, 1
  %cmp705 = icmp eq i32 %beta.addr.0, %add704
  %113 = load i32, ptr %ply, align 8, !tbaa !10
  %cmp523.peel = icmp slt i32 %113, 60
  %114 = load i32, ptr %i, align 4, !tbaa !18
  %idxprom532.peel = sext i32 %114 to i64
  %arrayidx533.peel = getelementptr inbounds [240 x i32], ptr %moves, i64 0, i64 %idxprom532.peel
  %115 = load i32, ptr %arrayidx533.peel, align 4, !tbaa !18
  br i1 %cmp523.peel, label %if.then524.peel, label %if.end683.peel

if.then524.peel:                                  ; preds = %while.body521.lr.ph
  %116 = lshr i32 %115, 6
  %and535.peel = and i32 %116, 63
  %idxprom536.peel = zext i32 %and535.peel to i64
  %arrayidx537.peel = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 1, i64 %idxprom536.peel
  %117 = load i32, ptr %arrayidx537.peel, align 4, !tbaa !18
  %add538.peel = add nsw i32 %117, 1
  %shr539.mask.peel = and i32 %add538.peel, -2
  %cmp540.peel = icmp eq i32 %shr539.mask.peel, 2
  br i1 %cmp540.peel, label %land.lhs.true541.peel, label %if.end566.peel

land.lhs.true541.peel:                            ; preds = %if.then524.peel
  %and544.peel = lshr i32 %115, 3
  %shr545.peel = and i32 %and544.peel, 7
  switch i32 %shr545.peel, label %lor.lhs.false553.peel [
    i32 1, label %if.then559.peel
    i32 6, label %if.then559.peel
  ]

lor.lhs.false553.peel:                            ; preds = %land.lhs.true541.peel
  %118 = and i32 %115, 61440
  %tobool558.not.peel = icmp eq i32 %118, 0
  br i1 %tobool558.not.peel, label %if.end566.peel, label %if.then559.peel

if.then559.peel:                                  ; preds = %lor.lhs.false553.peel, %land.lhs.true541.peel, %land.lhs.true541.peel
  br label %if.end566.peel

if.end566.peel:                                   ; preds = %if.then559.peel, %lor.lhs.false553.peel, %if.then524.peel
  %extend.1.peel = phi i32 [ %spec.select1537, %lor.lhs.false553.peel ], [ %spec.select1537, %if.then524.peel ], [ %spec.select1546, %if.then559.peel ]
  %119 = lshr i32 %115, 19
  %and570.peel = and i32 %119, 15
  %cmp571.not.peel = icmp eq i32 %and570.peel, 13
  br i1 %cmp571.not.peel, label %if.end634.peel, label %land.lhs.true572.peel

land.lhs.true572.peel:                            ; preds = %if.end566.peel
  %sub575.peel = add nsw i32 %113, -1
  %idxprom576.peel = sext i32 %sub575.peel to i64
  %arrayidx577.peel = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 19, i64 %idxprom576.peel
  %120 = load i32, ptr %arrayidx577.peel, align 4, !tbaa !18
  %121 = lshr i32 %120, 19
  %and579.peel = and i32 %121, 15
  %cmp580.not.peel = icmp eq i32 %and579.peel, 13
  br i1 %cmp580.not.peel, label %if.end634.peel, label %land.lhs.true581.peel

land.lhs.true581.peel:                            ; preds = %land.lhs.true572.peel
  %idxprom586.peel = zext i32 %and570.peel to i64
  %arrayidx587.peel = getelementptr inbounds [14 x i32], ptr @_ZL8rc_index, i64 0, i64 %idxprom586.peel
  %122 = load i32, ptr %arrayidx587.peel, align 4, !tbaa !18
  %idxprom595.peel = zext i32 %and579.peel to i64
  %arrayidx596.peel = getelementptr inbounds [14 x i32], ptr @_ZL8rc_index, i64 0, i64 %idxprom595.peel
  %123 = load i32, ptr %arrayidx596.peel, align 4, !tbaa !18
  %cmp597.peel = icmp eq i32 %122, %123
  br i1 %cmp597.peel, label %land.lhs.true598.peel, label %if.end634.peel

land.lhs.true598.peel:                            ; preds = %land.lhs.true581.peel
  %and601.peel = and i32 %115, 63
  %and607.peel = and i32 %120, 63
  %cmp608.peel = icmp eq i32 %and601.peel, %and607.peel
  br i1 %cmp608.peel, label %if.then609.peel, label %if.end634.peel

if.then609.peel:                                  ; preds = %land.lhs.true598.peel
  %124 = load i32, ptr %white_to_move610, align 4, !tbaa !17
  %tobool611.not.peel = icmp eq i32 %124, 0
  %cond612.peel = zext i1 %tobool611.not.peel to i32
  %125 = lshr i32 %115, 12
  %and623.peel = and i32 %125, 15
  %call624.peel = call noundef i32 @_Z3seeP7state_tiiii(ptr noundef nonnull %s, i32 noundef %cond612.peel, i32 noundef %and535.peel, i32 noundef %and601.peel, i32 noundef %and623.peel)
  %cmp625.peel = icmp sgt i32 %call624.peel, 0
  br i1 %cmp625.peel, label %if.then626.peel, label %if.end634.peel

if.then626.peel:                                  ; preds = %if.then609.peel
  br i1 %111, label %if.then628.peel, label %if.else630.peel

if.else630.peel:                                  ; preds = %if.then626.peel
  %add631.peel = add nuw nsw i32 %extend.1.peel, 1
  br label %if.end634.peel

if.then628.peel:                                  ; preds = %if.then626.peel
  %add629.peel = add nuw nsw i32 %extend.1.peel, 3
  br label %if.end634.peel

if.end634.peel:                                   ; preds = %if.then628.peel, %if.else630.peel, %if.then609.peel, %land.lhs.true598.peel, %land.lhs.true581.peel, %land.lhs.true572.peel, %if.end566.peel
  %extend.3.peel = phi i32 [ %extend.1.peel, %land.lhs.true598.peel ], [ %extend.1.peel, %land.lhs.true581.peel ], [ %extend.1.peel, %land.lhs.true572.peel ], [ %extend.1.peel, %if.end566.peel ], [ %add629.peel, %if.then628.peel ], [ %add631.peel, %if.else630.peel ], [ %extend.1.peel, %if.then609.peel ]
  %126 = load i32, ptr %singular, align 4, !tbaa !18
  %cmp635.peel = icmp eq i32 %126, 1
  %tobool637.peel = icmp ne i32 %extend.3.peel, 0
  %or.cond1077.peel = select i1 %cmp635.peel, i1 %tobool637.peel, i1 false
  %or.cond1078.peel = select i1 %or.cond1077.peel, i1 %tobool639, i1 false
  br i1 %or.cond1078.peel, label %if.then640.peel, label %if.else641.peel

if.else641.peel:                                  ; preds = %if.end634.peel
  %tobool642.peel = icmp eq i32 %extend.3.peel, 0
  %or.cond1079.peel = select i1 %tobool642.peel, i1 %cmp635.peel, i1 false
  %or.cond1080.peel = select i1 %or.cond1079.peel, i1 %tobool639, i1 false
  br i1 %or.cond1080.peel, label %if.then647.peel, label %if.end655.peel

if.then647.peel:                                  ; preds = %if.else641.peel
  store i32 0, ptr %nosingular, align 4, !tbaa !18
  br label %if.end655.peel

if.then640.peel:                                  ; preds = %if.end634.peel
  store i32 1, ptr %nosingular, align 4, !tbaa !18
  br label %if.end655.peel

if.end655.peel:                                   ; preds = %if.then640.peel, %if.then647.peel, %if.else641.peel
  %extend.4.peel = phi i32 [ %extend.3.peel, %if.then640.peel ], [ %extend.3.peel, %if.else641.peel ], [ %., %if.then647.peel ]
  %spec.store.select.peel = call i32 @llvm.smin.i32(i32 %extend.4.peel, i32 4)
  %127 = load i32, ptr %arrayidx533.peel, align 4, !tbaa !18
  %128 = lshr i32 %127, 19
  %and662.peel = and i32 %128, 15
  switch i32 %and662.peel, label %if.then676.peel [
    i32 13, label %if.end683.peel
    i32 1, label %if.end683.peel
    i32 2, label %if.end683.peel
  ]

if.then676.peel:                                  ; preds = %if.end655.peel
  %add680.peel = add nuw nsw i32 %spec.store.select.peel, 4
  %spec.select1538.peel = select i1 %cmp678, i32 %add680.peel, i32 %spec.store.select.peel
  br label %if.end683.peel

if.end683.peel:                                   ; preds = %while.body521.lr.ph, %if.then676.peel, %if.end655.peel, %if.end655.peel, %if.end655.peel
  %129 = phi i32 [ %127, %if.then676.peel ], [ %127, %if.end655.peel ], [ %127, %if.end655.peel ], [ %127, %if.end655.peel ], [ %115, %while.body521.lr.ph ]
  %extend.5.peel = phi i32 [ %spec.select1538.peel, %if.then676.peel ], [ %spec.store.select.peel, %if.end655.peel ], [ %spec.store.select.peel, %if.end655.peel ], [ %spec.store.select.peel, %if.end655.peel ], [ 0, %while.body521.lr.ph ]
  %130 = and i32 %129, 7864320
  %cmp688.peel = icmp eq i32 %130, 6815744
  br i1 %cmp688.peel, label %land.lhs.true689.peel, label %if.end716

land.lhs.true689.peel:                            ; preds = %if.end683.peel
  br i1 %cmp691, label %land.lhs.true692.peel, label %if.end716

land.lhs.true692.peel:                            ; preds = %land.lhs.true689.peel
  %131 = lshr i32 %129, 6
  %and.i.peel = and i32 %131, 63
  %idxprom.i.peel = zext i32 %and.i.peel to i64
  %arrayidx.i1549.peel = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 1, i64 %idxprom.i.peel
  %132 = load i32, ptr %arrayidx.i1549.peel, align 4, !tbaa !18
  %sub.i.peel = add nsw i32 %132, -1
  %and1.i.peel = and i32 %129, 63
  %133 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom2.i.peel = sext i32 %133 to i64
  %idxprom4.i.peel = sext i32 %sub.i.peel to i64
  %idxprom6.i.peel = zext i32 %and1.i.peel to i64
  %arrayidx7.i.peel = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom2.i.peel, i64 %idxprom4.i.peel, i64 %idxprom6.i.peel
  %134 = load i32, ptr %arrayidx7.i.peel, align 4, !tbaa !18
  %arrayidx14.i.peel = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom2.i.peel, i64 %idxprom4.i.peel, i64 %idxprom6.i.peel
  %135 = load i32, ptr %arrayidx14.i.peel, align 4, !tbaa !18
  %sub15.i.peel = sub nsw i32 %135, %134
  %mul.i.peel = mul nsw i32 %134, %add690
  %cmp.i1550.peel = icmp slt i32 %mul.i.peel, %sub15.i.peel
  %or.cond1081.not1530.not.peel = and i1 %cmp700, %cmp.i1550.peel
  %tobool702.peel = icmp eq i32 %extend.5.peel, 0
  %or.cond1082.peel = select i1 %or.cond1081.not1530.not.peel, i1 %tobool702.peel, i1 false
  %or.cond1539.peel = select i1 %or.cond1082.peel, i1 %cmp705, i1 false
  %136 = and i32 %129, 61440
  %tobool711.not.peel = icmp eq i32 %136, 0
  %or.cond1624.peel = select i1 %or.cond1539.peel, i1 %tobool711.not.peel, i1 false
  br i1 %or.cond1624.peel, label %if.then712.peel, label %if.end716

if.then712.peel:                                  ; preds = %land.lhs.true692.peel
  %call715.peel = call fastcc noundef i32 @_ZL15remove_one_fastPiS_S_i(ptr noundef nonnull %i, ptr noundef nonnull %move_ordering, ptr noundef nonnull %moves, i32 noundef %num_moves.01609), !range !47
  %tobool520.not.peel = icmp eq i32 %call715.peel, 0
  br i1 %tobool520.not.peel, label %while.end998.thread, label %while.body521

while.body521:                                    ; preds = %if.then712.peel, %if.then712
  %137 = load i32, ptr %ply, align 8, !tbaa !10
  %cmp523 = icmp slt i32 %137, 60
  %138 = load i32, ptr %i, align 4, !tbaa !18
  %idxprom532 = sext i32 %138 to i64
  %arrayidx533 = getelementptr inbounds [240 x i32], ptr %moves, i64 0, i64 %idxprom532
  %139 = load i32, ptr %arrayidx533, align 4, !tbaa !18
  br i1 %cmp523, label %if.then524, label %if.end683

if.then524:                                       ; preds = %while.body521
  %140 = lshr i32 %139, 6
  %and535 = and i32 %140, 63
  %idxprom536 = zext i32 %and535 to i64
  %arrayidx537 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 1, i64 %idxprom536
  %141 = load i32, ptr %arrayidx537, align 4, !tbaa !18
  %add538 = add nsw i32 %141, 1
  %shr539.mask = and i32 %add538, -2
  %cmp540 = icmp eq i32 %shr539.mask, 2
  br i1 %cmp540, label %land.lhs.true541, label %if.end566

land.lhs.true541:                                 ; preds = %if.then524
  %and544 = lshr i32 %139, 3
  %shr545 = and i32 %and544, 7
  switch i32 %shr545, label %lor.lhs.false553 [
    i32 1, label %if.then559
    i32 6, label %if.then559
  ]

lor.lhs.false553:                                 ; preds = %land.lhs.true541
  %142 = and i32 %139, 61440
  %tobool558.not = icmp eq i32 %142, 0
  br i1 %tobool558.not, label %if.end566, label %if.then559

if.then559:                                       ; preds = %land.lhs.true541, %land.lhs.true541, %lor.lhs.false553
  br label %if.end566

if.end566:                                        ; preds = %if.then559, %lor.lhs.false553, %if.then524
  %extend.1 = phi i32 [ %spec.select1537, %lor.lhs.false553 ], [ %spec.select1537, %if.then524 ], [ %spec.select1546, %if.then559 ]
  %143 = lshr i32 %139, 19
  %and570 = and i32 %143, 15
  %cmp571.not = icmp eq i32 %and570, 13
  br i1 %cmp571.not, label %if.end634, label %land.lhs.true572

land.lhs.true572:                                 ; preds = %if.end566
  %sub575 = add nsw i32 %137, -1
  %idxprom576 = sext i32 %sub575 to i64
  %arrayidx577 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 19, i64 %idxprom576
  %144 = load i32, ptr %arrayidx577, align 4, !tbaa !18
  %145 = lshr i32 %144, 19
  %and579 = and i32 %145, 15
  %cmp580.not = icmp eq i32 %and579, 13
  br i1 %cmp580.not, label %if.end634, label %land.lhs.true581

land.lhs.true581:                                 ; preds = %land.lhs.true572
  %idxprom586 = zext i32 %and570 to i64
  %arrayidx587 = getelementptr inbounds [14 x i32], ptr @_ZL8rc_index, i64 0, i64 %idxprom586
  %146 = load i32, ptr %arrayidx587, align 4, !tbaa !18
  %idxprom595 = zext i32 %and579 to i64
  %arrayidx596 = getelementptr inbounds [14 x i32], ptr @_ZL8rc_index, i64 0, i64 %idxprom595
  %147 = load i32, ptr %arrayidx596, align 4, !tbaa !18
  %cmp597 = icmp eq i32 %146, %147
  br i1 %cmp597, label %land.lhs.true598, label %if.end634

land.lhs.true598:                                 ; preds = %land.lhs.true581
  %and601 = and i32 %139, 63
  %and607 = and i32 %144, 63
  %cmp608 = icmp eq i32 %and601, %and607
  br i1 %cmp608, label %if.then609, label %if.end634

if.then609:                                       ; preds = %land.lhs.true598
  %148 = load i32, ptr %white_to_move610, align 4, !tbaa !17
  %tobool611.not = icmp eq i32 %148, 0
  %cond612 = zext i1 %tobool611.not to i32
  %149 = lshr i32 %139, 12
  %and623 = and i32 %149, 15
  %call624 = call noundef i32 @_Z3seeP7state_tiiii(ptr noundef nonnull %s, i32 noundef %cond612, i32 noundef %and535, i32 noundef %and601, i32 noundef %and623)
  %cmp625 = icmp sgt i32 %call624, 0
  br i1 %cmp625, label %if.then626, label %if.end634

if.then626:                                       ; preds = %if.then609
  br i1 %111, label %if.then628, label %if.else630

if.then628:                                       ; preds = %if.then626
  %add629 = add nuw nsw i32 %extend.1, 3
  br label %if.end634

if.else630:                                       ; preds = %if.then626
  %add631 = add nuw nsw i32 %extend.1, 1
  br label %if.end634

if.end634:                                        ; preds = %if.then609, %if.else630, %if.then628, %land.lhs.true598, %land.lhs.true581, %land.lhs.true572, %if.end566
  %extend.3 = phi i32 [ %extend.1, %land.lhs.true598 ], [ %extend.1, %land.lhs.true581 ], [ %extend.1, %land.lhs.true572 ], [ %extend.1, %if.end566 ], [ %add629, %if.then628 ], [ %add631, %if.else630 ], [ %extend.1, %if.then609 ]
  %150 = load i32, ptr %singular, align 4, !tbaa !18
  %cmp635 = icmp eq i32 %150, 1
  %tobool637 = icmp ne i32 %extend.3, 0
  %or.cond1077 = select i1 %cmp635, i1 %tobool637, i1 false
  %or.cond1078 = select i1 %or.cond1077, i1 %tobool639, i1 false
  br i1 %or.cond1078, label %if.then640, label %if.else641

if.then640:                                       ; preds = %if.end634
  store i32 1, ptr %nosingular, align 4, !tbaa !18
  br label %if.end655

if.else641:                                       ; preds = %if.end634
  %tobool642 = icmp eq i32 %extend.3, 0
  %or.cond1079 = select i1 %tobool642, i1 %cmp635, i1 false
  %or.cond1080 = select i1 %or.cond1079, i1 %tobool639, i1 false
  br i1 %or.cond1080, label %if.then647, label %if.end655

if.then647:                                       ; preds = %if.else641
  store i32 0, ptr %nosingular, align 4, !tbaa !18
  br label %if.end655

if.end655:                                        ; preds = %if.then647, %if.else641, %if.then640
  %extend.4 = phi i32 [ %extend.3, %if.then640 ], [ %extend.3, %if.else641 ], [ %., %if.then647 ]
  %spec.store.select = call i32 @llvm.smin.i32(i32 %extend.4, i32 4)
  %151 = load i32, ptr %arrayidx533, align 4, !tbaa !18
  %152 = lshr i32 %151, 19
  %and662 = and i32 %152, 15
  switch i32 %and662, label %if.then676 [
    i32 13, label %if.end683
    i32 1, label %if.end683
    i32 2, label %if.end683
  ]

if.then676:                                       ; preds = %if.end655
  %add680 = add nuw nsw i32 %spec.store.select, 4
  %spec.select1538 = select i1 %cmp678, i32 %add680, i32 %spec.store.select
  br label %if.end683

if.end683:                                        ; preds = %while.body521, %if.then676, %if.end655, %if.end655, %if.end655
  %153 = phi i32 [ %151, %if.then676 ], [ %151, %if.end655 ], [ %151, %if.end655 ], [ %151, %if.end655 ], [ %139, %while.body521 ]
  %extend.5 = phi i32 [ %spec.select1538, %if.then676 ], [ %spec.store.select, %if.end655 ], [ %spec.store.select, %if.end655 ], [ %spec.store.select, %if.end655 ], [ 0, %while.body521 ]
  %154 = and i32 %153, 7864320
  %cmp688 = icmp eq i32 %154, 6815744
  br i1 %cmp688, label %land.lhs.true692, label %if.end716.loopexit

land.lhs.true692:                                 ; preds = %if.end683
  %155 = lshr i32 %153, 6
  %and.i = and i32 %155, 63
  %idxprom.i = zext i32 %and.i to i64
  %arrayidx.i1549 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 1, i64 %idxprom.i
  %156 = load i32, ptr %arrayidx.i1549, align 4, !tbaa !18
  %sub.i = add nsw i32 %156, -1
  %and1.i = and i32 %153, 63
  %157 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom2.i = sext i32 %157 to i64
  %idxprom4.i = sext i32 %sub.i to i64
  %idxprom6.i = zext i32 %and1.i to i64
  %arrayidx7.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom2.i, i64 %idxprom4.i, i64 %idxprom6.i
  %158 = load i32, ptr %arrayidx7.i, align 4, !tbaa !18
  %arrayidx14.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom2.i, i64 %idxprom4.i, i64 %idxprom6.i
  %159 = load i32, ptr %arrayidx14.i, align 4, !tbaa !18
  %sub15.i = sub nsw i32 %159, %158
  %mul.i = mul nsw i32 %158, %add690
  %cmp.i1550 = icmp slt i32 %mul.i, %sub15.i
  %tobool702 = icmp eq i32 %extend.5, 0
  %or.cond1082 = select i1 %cmp.i1550, i1 %tobool702, i1 false
  %160 = and i32 %153, 61440
  %tobool711.not = icmp eq i32 %160, 0
  %or.cond1624 = select i1 %or.cond1082, i1 %tobool711.not, i1 false
  br i1 %or.cond1624, label %if.then712, label %if.end716.loopexit

if.then712:                                       ; preds = %land.lhs.true692
  %call715 = call fastcc noundef i32 @_ZL15remove_one_fastPiS_S_i(ptr noundef nonnull %i, ptr noundef nonnull %move_ordering, ptr noundef nonnull %moves, i32 noundef %num_moves.01609), !range !47
  %tobool520.not = icmp eq i32 %call715, 0
  br i1 %tobool520.not, label %while.end998.thread, label %while.body521, !llvm.loop !50

if.end716.loopexit:                               ; preds = %if.end683, %land.lhs.true692
  %idxprom532.le = sext i32 %138 to i64
  br label %if.end716

if.end716:                                        ; preds = %if.end716.loopexit, %land.lhs.true692.peel, %land.lhs.true689.peel, %if.end683.peel
  %idxprom684.le.pre-phi = phi i64 [ %idxprom532.peel, %land.lhs.true692.peel ], [ %idxprom532.peel, %land.lhs.true689.peel ], [ %idxprom532.peel, %if.end683.peel ], [ %idxprom532.le, %if.end716.loopexit ]
  %extend.5.lcssa = phi i32 [ %extend.5.peel, %land.lhs.true692.peel ], [ %extend.5.peel, %land.lhs.true689.peel ], [ %extend.5.peel, %if.end683.peel ], [ %extend.5, %if.end716.loopexit ]
  %.lcssa1684 = phi i32 [ %114, %land.lhs.true692.peel ], [ %114, %land.lhs.true689.peel ], [ %114, %if.end683.peel ], [ %138, %if.end716.loopexit ]
  %.lcssa = phi i32 [ %129, %land.lhs.true692.peel ], [ %129, %land.lhs.true689.peel ], [ %129, %if.end683.peel ], [ %153, %if.end716.loopexit ]
  %cmp688.lcssa = phi i1 [ true, %land.lhs.true692.peel ], [ true, %land.lhs.true689.peel ], [ false, %if.end683.peel ], [ %cmp688, %if.end716.loopexit ]
  %no_moves.01664.lcssa = phi i32 [ %no_moves.0.ph1673, %land.lhs.true692.peel ], [ %no_moves.0.ph1673, %land.lhs.true689.peel ], [ %no_moves.0.ph1673, %if.end683.peel ], [ 0, %if.end716.loopexit ]
  %arrayidx685.le = getelementptr inbounds [240 x i32], ptr %moves, i64 0, i64 %idxprom684.le.pre-phi
  br i1 %cmp717, label %if.then718, label %if.else727

if.then718:                                       ; preds = %if.end716
  %cmp720 = icmp slt i32 %add719, %alpha.addr.1.ph1675
  %cmp724 = icmp slt i32 %add723, %alpha.addr.1.ph1675
  br label %if.end739

if.else727:                                       ; preds = %if.end716
  br i1 %cmp728, label %if.then729, label %if.end739

if.then729:                                       ; preds = %if.else727
  %cmp731 = icmp slt i32 %add730, %alpha.addr.1.ph1675
  %cmp735 = icmp slt i32 %add734, %alpha.addr.1.ph1675
  br label %if.end739

if.end739:                                        ; preds = %if.then729, %if.then718, %if.else727
  %afutprun.2.shrunk = phi i1 [ false, %if.else727 ], [ %cmp720, %if.then718 ], [ %cmp731, %if.then729 ]
  %tobool787 = phi i1 [ false, %if.else727 ], [ %cmp724, %if.then718 ], [ %cmp735, %if.then729 ]
  br i1 %cmp688.lcssa, label %if.end762, label %if.then745

if.then745:                                       ; preds = %if.end739
  %161 = load i32, ptr %white_to_move610, align 4, !tbaa !17
  %tobool747.not = icmp eq i32 %161, 0
  %cond748 = zext i1 %tobool747.not to i32
  %162 = lshr i32 %.lcssa, 6
  %and752 = and i32 %162, 63
  %and755 = and i32 %.lcssa, 63
  %163 = lshr i32 %.lcssa, 12
  %and759 = and i32 %163, 15
  %call760 = call noundef i32 @_Z3seeP7state_tiiii(ptr noundef nonnull %s, i32 noundef %cond748, i32 noundef %and752, i32 noundef %and755, i32 noundef %and759)
  %.pre1717 = load i32, ptr %arrayidx685.le, align 4, !tbaa !18
  br label %if.end762

if.end762:                                        ; preds = %if.end739, %if.then745
  %164 = phi i32 [ %.pre1717, %if.then745 ], [ %.lcssa, %if.end739 ]
  %capval.0 = phi i32 [ %call760, %if.then745 ], [ -1000000, %if.end739 ]
  call void @_Z4makeP7state_ti(ptr noundef nonnull %s, i32 noundef %164)
  %165 = load i32, ptr %arrayidx685.le, align 4, !tbaa !18
  %call767 = call noundef i32 @_Z11check_legalP7state_ti(ptr noundef nonnull %s, i32 noundef %165)
  %tobool768.not = icmp eq i32 %call767, 0
  br i1 %tobool768.not, label %if.end950, label %if.then769

if.then769:                                       ; preds = %if.end762
  %call770 = call noundef i32 @_Z8in_checkP7state_t(ptr noundef nonnull %s)
  %tobool771 = icmp ne i32 %call770, 0
  br i1 %tobool771, label %if.then772, label %if.end779

if.then772:                                       ; preds = %if.then769
  br i1 %111, label %if.then774, label %if.else776

if.then774:                                       ; preds = %if.then772
  %add775 = add nuw nsw i32 %extend.5.lcssa, 4
  br label %if.end779

if.else776:                                       ; preds = %if.then772
  %add777 = add nuw nsw i32 %extend.5.lcssa, 2
  br label %if.end779

if.end779:                                        ; preds = %if.then774, %if.else776, %if.then769
  %extend.6 = phi i32 [ %add775, %if.then774 ], [ %add777, %if.else776 ], [ %extend.5.lcssa, %if.then769 ]
  %166 = or i32 %call770, %12
  %or.cond1083.not = icmp eq i32 %166, 0
  %or.cond1542 = select i1 %or.cond1083.not, i1 %cmp705, i1 false
  br i1 %or.cond1542, label %if.then786, label %if.end821

if.then786:                                       ; preds = %if.end779
  %cmp789 = icmp slt i32 %capval.0, 86
  %or.cond1092 = and i1 %tobool787, %cmp789
  br i1 %or.cond1092, label %land.lhs.true790, label %if.end803

land.lhs.true790:                                 ; preds = %if.then786
  %167 = load i32, ptr %arrayidx685.le, align 4, !tbaa !18
  %168 = and i32 %167, 61440
  %tobool795.not = icmp eq i32 %168, 0
  br i1 %tobool795.not, label %if.then796, label %if.end803

if.then796:                                       ; preds = %land.lhs.true790
  call void @_Z6unmakeP7state_ti(ptr noundef nonnull %s, i32 noundef %167)
  %call801 = call fastcc noundef i32 @_ZL15remove_one_fastPiS_S_i(ptr noundef nonnull %i, ptr noundef nonnull %move_ordering, ptr noundef nonnull %moves, i32 noundef %num_moves.01609), !range !47
  br label %cleanup993, !llvm.loop !52

if.end803:                                        ; preds = %land.lhs.true790, %if.then786
  %cmp806 = icmp slt i32 %capval.0, -50
  %or.cond1093 = and i1 %afutprun.2.shrunk, %cmp806
  br i1 %or.cond1093, label %land.lhs.true807, label %if.end821

land.lhs.true807:                                 ; preds = %if.end803
  %169 = load i32, ptr %arrayidx685.le, align 4, !tbaa !18
  %170 = and i32 %169, 61440
  %tobool812.not = icmp eq i32 %170, 0
  br i1 %tobool812.not, label %if.then813, label %if.end821

if.then813:                                       ; preds = %land.lhs.true807
  call void @_Z6unmakeP7state_ti(ptr noundef nonnull %s, i32 noundef %169)
  %call818 = call fastcc noundef i32 @_ZL15remove_one_fastPiS_S_i(ptr noundef nonnull %i, ptr noundef nonnull %move_ordering, ptr noundef nonnull %moves, i32 noundef %num_moves.01609), !range !47
  br label %cleanup993, !llvm.loop !52

if.end821:                                        ; preds = %if.end803, %land.lhs.true807, %if.end779
  %add823 = add nsw i32 %extend.6, %depth
  %sub826 = sub nsw i32 0, %alpha.addr.1.ph1675
  %add827 = sub i32 130, %alpha.addr.1.ph1675
  %cmp828 = icmp sgt i32 %add823, 4
  %171 = or i1 %tobool771, %cmp828
  %conv832 = zext i1 %171 to i32
  %call833 = call noundef i32 @_Z4evalP7state_tiii(ptr noundef nonnull %s, i32 noundef %sub825, i32 noundef %add827, i32 noundef %conv832)
  %172 = load i32, ptr %ply, align 8, !tbaa !10
  %idxprom836 = sext i32 %172 to i64
  %arrayidx837 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 25, i64 %idxprom836
  store i32 %call770, ptr %arrayidx837, align 4, !tbaa !18
  %173 = load i64, ptr %hash838, align 8, !tbaa !26
  %174 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 15), align 4, !tbaa !27
  %175 = load i32, ptr %ply, align 8, !tbaa !10
  %add841 = add i32 %175, -1
  %sub842 = add i32 %add841, %174
  %idxprom843 = sext i32 %sub842 to i64
  %arrayidx844 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 36, i64 %idxprom843
  store i64 %173, ptr %arrayidx844, align 8, !tbaa !6
  %176 = load i32, ptr %arrayidx685.le, align 4, !tbaa !18
  %idxprom850 = sext i32 %add841 to i64
  %arrayidx851 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 19, i64 %idxprom850
  store i32 %176, ptr %arrayidx851, align 4, !tbaa !18
  %cmp854 = icmp sgt i32 %mn.0.ph1670, 3
  %or.cond1084 = select i1 %cmp852, i1 %cmp854, i1 false
  br i1 %or.cond1084, label %land.lhs.true855, label %if.end880

land.lhs.true855:                                 ; preds = %if.end821
  %177 = or i32 %extend.6, %call770
  %178 = icmp eq i32 %177, 0
  %or.cond1086.not1528 = select i1 %cmp705, i1 %178, i1 false
  %cmp863 = icmp slt i32 %capval.0, -50
  %or.cond1087 = and i1 %cmp863, %or.cond1086.not1528
  br i1 %or.cond1087, label %land.lhs.true864, label %if.end880

land.lhs.true864:                                 ; preds = %land.lhs.true855
  %and.i1551 = and i32 %176, 63
  %idxprom.i1552 = zext i32 %and.i1551 to i64
  %arrayidx.i1553 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 1, i64 %idxprom.i1552
  %179 = load i32, ptr %arrayidx.i1553, align 4, !tbaa !18
  %sub.i1554 = add nsw i32 %179, -1
  %180 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom2.i1555 = sext i32 %180 to i64
  %idxprom4.i1556 = sext i32 %sub.i1554 to i64
  %arrayidx7.i1557 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom2.i1555, i64 %idxprom4.i1556, i64 %idxprom.i1552
  %181 = load i32, ptr %arrayidx7.i1557, align 4, !tbaa !18
  %add.i = shl i32 %181, 7
  %mul.i1558 = add i32 %add.i, 128
  %arrayidx14.i1559 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom2.i1555, i64 %idxprom4.i1556, i64 %idxprom.i1552
  %182 = load i32, ptr %arrayidx14.i1559, align 4, !tbaa !18
  %add15.i = add nsw i32 %182, 1
  %div.i = sdiv i32 %mul.i1558, %add15.i
  %cmp868 = icmp slt i32 %div.i, 80
  %183 = and i32 %176, 61440
  %tobool874.not = icmp eq i32 %183, 0
  %or.cond1625 = select i1 %cmp868, i1 %tobool874.not, i1 false
  br i1 %or.cond1625, label %if.then875, label %if.end880

if.then875:                                       ; preds = %land.lhs.true864
  %sub876 = add nsw i32 %extend.6, -4
  %add878 = add nsw i32 %sub876, %depth
  br label %if.end880

if.end880:                                        ; preds = %if.then875, %land.lhs.true864, %land.lhs.true855, %if.end821
  %tobool919 = phi i1 [ true, %if.then875 ], [ false, %land.lhs.true864 ], [ false, %land.lhs.true855 ], [ false, %if.end821 ]
  %huber.0 = phi i32 [ 4, %if.then875 ], [ 0, %land.lhs.true864 ], [ 0, %land.lhs.true855 ], [ 0, %if.end821 ]
  %extend.7 = phi i32 [ %sub876, %if.then875 ], [ %extend.6, %land.lhs.true864 ], [ %extend.6, %land.lhs.true855 ], [ %extend.6, %if.end821 ]
  %newdepth822.0.in = phi i32 [ %add878, %if.then875 ], [ %add823, %land.lhs.true864 ], [ %add823, %land.lhs.true855 ], [ %add823, %if.end821 ]
  %newdepth822.0 = add nsw i32 %newdepth822.0.in, -4
  %cmp881 = icmp eq i32 %first.2.ph1671, 1
  %cmp883 = icmp slt i32 %newdepth822.0.in, 5
  br i1 %cmp881, label %if.then882, label %if.else898

if.then882:                                       ; preds = %if.end880
  br i1 %cmp883, label %if.then884, label %if.else889

if.then884:                                       ; preds = %if.then882
  %call887 = call noundef i32 @_Z7qsearchP7state_tiiii(ptr noundef nonnull %s, i32 noundef %sub825, i32 noundef %sub826, i32 noundef 0, i32 noundef 0)
  %sub888 = sub nsw i32 0, %call887
  br label %if.end946

if.else889:                                       ; preds = %if.then882
  %call895 = call noundef i32 @_Z6searchP7state_tiiiii(ptr noundef nonnull %s, i32 noundef %sub825, i32 noundef %sub826, i32 noundef %newdepth822.0, i32 noundef 0, i32 noundef %conv894)
  %sub896 = sub nsw i32 0, %call895
  br label %if.end946

if.else898:                                       ; preds = %if.end880
  %sub902 = xor i32 %alpha.addr.1.ph1675, -1
  br i1 %cmp883, label %if.then900, label %if.else906

if.then900:                                       ; preds = %if.else898
  %call904 = call noundef i32 @_Z7qsearchP7state_tiiii(ptr noundef nonnull %s, i32 noundef %sub902, i32 noundef %sub826, i32 noundef 0, i32 noundef 0)
  br label %if.end912

if.else906:                                       ; preds = %if.else898
  %call910 = call noundef i32 @_Z6searchP7state_tiiiii(ptr noundef nonnull %s, i32 noundef %sub902, i32 noundef %sub826, i32 noundef %newdepth822.0, i32 noundef 0, i32 noundef 1)
  br label %if.end912

if.end912:                                        ; preds = %if.else906, %if.then900
  %call904.pn = phi i32 [ %call904, %if.then900 ], [ %call910, %if.else906 ]
  %score.15 = sub nsw i32 0, %call904.pn
  %cmp913 = icmp slt i32 %best_score.0.ph1672, %score.15
  %184 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8
  %tobool915 = icmp eq i32 %184, 0
  %or.cond1088.not1626 = select i1 %cmp913, i1 %tobool915, i1 false
  %cmp917 = icmp slt i32 %alpha.addr.1.ph1675, %score.15
  %or.cond1543 = select i1 %or.cond1088.not1626, i1 %cmp917, i1 false
  %cmp923 = icmp sgt i32 %beta.addr.0, %score.15
  %or.cond1089 = or i1 %tobool919, %cmp923
  %or.cond1627 = select i1 %or.cond1543, i1 %or.cond1089, i1 false
  br i1 %or.cond1627, label %if.then926, label %if.end946

if.then926:                                       ; preds = %if.end912
  %add921 = select i1 %tobool919, i32 %huber.0, i32 0
  %spec.select1544 = add i32 %add921, %depth
  %add927 = add i32 %spec.select1544, %extend.7
  %cmp929 = icmp slt i32 %add927, 5
  br i1 %cmp929, label %if.then930, label %if.else935

if.then930:                                       ; preds = %if.then926
  %call933 = call noundef i32 @_Z7qsearchP7state_tiiii(ptr noundef nonnull %s, i32 noundef %sub825, i32 noundef %sub826, i32 noundef 0, i32 noundef 0)
  %sub934 = sub nsw i32 0, %call933
  br label %if.end946

if.else935:                                       ; preds = %if.then926
  %sub928 = add nsw i32 %add927, -4
  %cond939 = zext i1 %tobool919 to i32
  %call940 = call noundef i32 @_Z6searchP7state_tiiiii(ptr noundef nonnull %s, i32 noundef %sub825, i32 noundef %sub826, i32 noundef %sub928, i32 noundef 0, i32 noundef %cond939)
  %sub941 = sub nsw i32 0, %call940
  br label %if.end946

if.end946:                                        ; preds = %if.end912, %if.else935, %if.then930, %if.then884, %if.else889
  %score.16 = phi i32 [ %sub888, %if.then884 ], [ %sub896, %if.else889 ], [ %score.15, %if.end912 ], [ %sub934, %if.then930 ], [ %sub941, %if.else935 ]
  %spec.select1545 = call i32 @llvm.smax.i32(i32 %score.16, i32 %best_score.0.ph1672)
  br label %if.end950

if.end950:                                        ; preds = %if.end946, %if.end762
  %best_score.2 = phi i32 [ %spec.select1545, %if.end946 ], [ %best_score.0.ph1672, %if.end762 ]
  %no_moves.1 = phi i32 [ 0, %if.end946 ], [ %no_moves.01664.lcssa, %if.end762 ]
  %score.17 = phi i32 [ %score.16, %if.end946 ], [ %score.14.ph1674, %if.end762 ]
  %185 = load i32, ptr %arrayidx685.le, align 4, !tbaa !18
  call void @_Z6unmakeP7state_ti(ptr noundef nonnull %s, i32 noundef %185)
  %186 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %tobool953.not = icmp eq i32 %186, 0
  br i1 %tobool953.not, label %if.end955, label %cleanup1026

if.end955:                                        ; preds = %if.end950
  br i1 %tobool768.not, label %if.end989, label %if.then957

if.then957:                                       ; preds = %if.end955
  %cmp958 = icmp sgt i32 %score.17, %alpha.addr.1.ph1675
  br i1 %cmp958, label %if.then959, label %if.end982

if.then959:                                       ; preds = %if.then957
  %cmp960.not = icmp slt i32 %score.17, %beta.addr.0
  %187 = load i32, ptr %arrayidx685.le, align 4, !tbaa !18
  br i1 %cmp960.not, label %if.end977, label %if.then961

if.then961:                                       ; preds = %if.then959
  call fastcc void @_ZL12history_goodP7state_tii(ptr noundef nonnull %s, i32 noundef %187, i32 noundef %depth)
  %cmp9661678 = icmp sgt i32 %mn.0.ph1670, 1
  br i1 %cmp9661678, label %for.body967.lr.ph, label %for.end972

for.body967.lr.ph:                                ; preds = %if.then961
  %sub965 = add nsw i32 %mn.0.ph1670, -1
  %add.i1563 = add nsw i32 %depth, 3
  %div.i1564 = sdiv i32 %add.i1563, 4
  %wide.trip.count1710 = zext i32 %sub965 to i64
  br label %for.body967

for.body967:                                      ; preds = %for.body967.lr.ph, %_ZL11history_badP7state_tii.exit
  %indvars.iv1707 = phi i64 [ 0, %for.body967.lr.ph ], [ %indvars.iv.next1708, %_ZL11history_badP7state_tii.exit ]
  %arrayidx969 = getelementptr inbounds [240 x i32], ptr %searched_moves, i64 0, i64 %indvars.iv1707
  %188 = load i32, ptr %arrayidx969, align 4, !tbaa !18
  %189 = and i32 %188, 7925760
  %or.cond.i = icmp eq i32 %189, 6815744
  br i1 %or.cond.i, label %if.then.i, label %_ZL11history_badP7state_tii.exit

if.then.i:                                        ; preds = %for.body967
  %190 = lshr i32 %188, 6
  %and4.i = and i32 %190, 63
  %idxprom.i1560 = zext i32 %and4.i to i64
  %arrayidx.i1561 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 1, i64 %idxprom.i1560
  %191 = load i32, ptr %arrayidx.i1561, align 4, !tbaa !18
  %sub.i1562 = add nsw i32 %191, -1
  %and5.i = and i32 %188, 63
  %192 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom6.i1565 = sext i32 %192 to i64
  %idxprom8.i = sext i32 %sub.i1562 to i64
  %idxprom10.i = zext i32 %and5.i to i64
  %arrayidx11.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom6.i1565, i64 %idxprom8.i, i64 %idxprom10.i
  %193 = load i32, ptr %arrayidx11.i, align 4, !tbaa !18
  %add12.i = add nsw i32 %193, %div.i1564
  store i32 %add12.i, ptr %arrayidx11.i, align 4, !tbaa !18
  %194 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom14.i = sext i32 %194 to i64
  %arrayidx19.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom14.i, i64 %idxprom8.i, i64 %idxprom10.i
  %195 = load i32, ptr %arrayidx19.i, align 4, !tbaa !18
  %cmp20.i = icmp sgt i32 %195, 16384
  br i1 %cmp20.i, label %for.body25.i, label %_ZL11history_badP7state_tii.exit

for.body25.i:                                     ; preds = %if.then.i, %for.body25.i
  %indvars.iv.i1566 = phi i64 [ %indvars.iv.next.i1567, %for.body25.i ], [ 0, %if.then.i ]
  %196 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom27.i = sext i32 %196 to i64
  %arrayidx32.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom27.i, i64 0, i64 %indvars.iv.i1566
  %197 = load i32, ptr %arrayidx32.i, align 4, !tbaa !18
  %add33.i = add nsw i32 %197, 1
  %shr34.i = ashr i32 %add33.i, 1
  store i32 %shr34.i, ptr %arrayidx32.i, align 4, !tbaa !18
  %198 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom43.i = sext i32 %198 to i64
  %arrayidx48.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom43.i, i64 0, i64 %indvars.iv.i1566
  %199 = load i32, ptr %arrayidx48.i, align 4, !tbaa !18
  %add49.i = add nsw i32 %199, 1
  %shr50.i = ashr i32 %add49.i, 1
  store i32 %shr50.i, ptr %arrayidx48.i, align 4, !tbaa !18
  %indvars.iv.next.i1567 = add nuw nsw i64 %indvars.iv.i1566, 1
  %exitcond.not.i1568 = icmp eq i64 %indvars.iv.next.i1567, 64
  br i1 %exitcond.not.i1568, label %for.body25.1.i, label %for.body25.i, !llvm.loop !53

for.body25.1.i:                                   ; preds = %for.body25.i, %for.body25.1.i
  %indvars.iv.1.i = phi i64 [ %indvars.iv.next.1.i, %for.body25.1.i ], [ 0, %for.body25.i ]
  %200 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom27.1.i = sext i32 %200 to i64
  %arrayidx32.1.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom27.1.i, i64 1, i64 %indvars.iv.1.i
  %201 = load i32, ptr %arrayidx32.1.i, align 4, !tbaa !18
  %add33.1.i = add nsw i32 %201, 1
  %shr34.1.i = ashr i32 %add33.1.i, 1
  store i32 %shr34.1.i, ptr %arrayidx32.1.i, align 4, !tbaa !18
  %202 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom43.1.i = sext i32 %202 to i64
  %arrayidx48.1.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom43.1.i, i64 1, i64 %indvars.iv.1.i
  %203 = load i32, ptr %arrayidx48.1.i, align 4, !tbaa !18
  %add49.1.i = add nsw i32 %203, 1
  %shr50.1.i = ashr i32 %add49.1.i, 1
  store i32 %shr50.1.i, ptr %arrayidx48.1.i, align 4, !tbaa !18
  %indvars.iv.next.1.i = add nuw nsw i64 %indvars.iv.1.i, 1
  %exitcond.1.not.i = icmp eq i64 %indvars.iv.next.1.i, 64
  br i1 %exitcond.1.not.i, label %for.body25.2.i, label %for.body25.1.i, !llvm.loop !53

for.body25.2.i:                                   ; preds = %for.body25.1.i, %for.body25.2.i
  %indvars.iv.2.i = phi i64 [ %indvars.iv.next.2.i, %for.body25.2.i ], [ 0, %for.body25.1.i ]
  %204 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom27.2.i = sext i32 %204 to i64
  %arrayidx32.2.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom27.2.i, i64 2, i64 %indvars.iv.2.i
  %205 = load i32, ptr %arrayidx32.2.i, align 4, !tbaa !18
  %add33.2.i = add nsw i32 %205, 1
  %shr34.2.i = ashr i32 %add33.2.i, 1
  store i32 %shr34.2.i, ptr %arrayidx32.2.i, align 4, !tbaa !18
  %206 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom43.2.i = sext i32 %206 to i64
  %arrayidx48.2.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom43.2.i, i64 2, i64 %indvars.iv.2.i
  %207 = load i32, ptr %arrayidx48.2.i, align 4, !tbaa !18
  %add49.2.i = add nsw i32 %207, 1
  %shr50.2.i = ashr i32 %add49.2.i, 1
  store i32 %shr50.2.i, ptr %arrayidx48.2.i, align 4, !tbaa !18
  %indvars.iv.next.2.i = add nuw nsw i64 %indvars.iv.2.i, 1
  %exitcond.2.not.i = icmp eq i64 %indvars.iv.next.2.i, 64
  br i1 %exitcond.2.not.i, label %for.body25.3.i, label %for.body25.2.i, !llvm.loop !53

for.body25.3.i:                                   ; preds = %for.body25.2.i, %for.body25.3.i
  %indvars.iv.3.i = phi i64 [ %indvars.iv.next.3.i, %for.body25.3.i ], [ 0, %for.body25.2.i ]
  %208 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom27.3.i = sext i32 %208 to i64
  %arrayidx32.3.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom27.3.i, i64 3, i64 %indvars.iv.3.i
  %209 = load i32, ptr %arrayidx32.3.i, align 4, !tbaa !18
  %add33.3.i = add nsw i32 %209, 1
  %shr34.3.i = ashr i32 %add33.3.i, 1
  store i32 %shr34.3.i, ptr %arrayidx32.3.i, align 4, !tbaa !18
  %210 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom43.3.i = sext i32 %210 to i64
  %arrayidx48.3.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom43.3.i, i64 3, i64 %indvars.iv.3.i
  %211 = load i32, ptr %arrayidx48.3.i, align 4, !tbaa !18
  %add49.3.i = add nsw i32 %211, 1
  %shr50.3.i = ashr i32 %add49.3.i, 1
  store i32 %shr50.3.i, ptr %arrayidx48.3.i, align 4, !tbaa !18
  %indvars.iv.next.3.i = add nuw nsw i64 %indvars.iv.3.i, 1
  %exitcond.3.not.i = icmp eq i64 %indvars.iv.next.3.i, 64
  br i1 %exitcond.3.not.i, label %for.body25.4.i, label %for.body25.3.i, !llvm.loop !53

for.body25.4.i:                                   ; preds = %for.body25.3.i, %for.body25.4.i
  %indvars.iv.4.i = phi i64 [ %indvars.iv.next.4.i, %for.body25.4.i ], [ 0, %for.body25.3.i ]
  %212 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom27.4.i = sext i32 %212 to i64
  %arrayidx32.4.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom27.4.i, i64 4, i64 %indvars.iv.4.i
  %213 = load i32, ptr %arrayidx32.4.i, align 4, !tbaa !18
  %add33.4.i = add nsw i32 %213, 1
  %shr34.4.i = ashr i32 %add33.4.i, 1
  store i32 %shr34.4.i, ptr %arrayidx32.4.i, align 4, !tbaa !18
  %214 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom43.4.i = sext i32 %214 to i64
  %arrayidx48.4.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom43.4.i, i64 4, i64 %indvars.iv.4.i
  %215 = load i32, ptr %arrayidx48.4.i, align 4, !tbaa !18
  %add49.4.i = add nsw i32 %215, 1
  %shr50.4.i = ashr i32 %add49.4.i, 1
  store i32 %shr50.4.i, ptr %arrayidx48.4.i, align 4, !tbaa !18
  %indvars.iv.next.4.i = add nuw nsw i64 %indvars.iv.4.i, 1
  %exitcond.4.not.i = icmp eq i64 %indvars.iv.next.4.i, 64
  br i1 %exitcond.4.not.i, label %for.body25.5.i, label %for.body25.4.i, !llvm.loop !53

for.body25.5.i:                                   ; preds = %for.body25.4.i, %for.body25.5.i
  %indvars.iv.5.i = phi i64 [ %indvars.iv.next.5.i, %for.body25.5.i ], [ 0, %for.body25.4.i ]
  %216 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom27.5.i = sext i32 %216 to i64
  %arrayidx32.5.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom27.5.i, i64 5, i64 %indvars.iv.5.i
  %217 = load i32, ptr %arrayidx32.5.i, align 4, !tbaa !18
  %add33.5.i = add nsw i32 %217, 1
  %shr34.5.i = ashr i32 %add33.5.i, 1
  store i32 %shr34.5.i, ptr %arrayidx32.5.i, align 4, !tbaa !18
  %218 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom43.5.i = sext i32 %218 to i64
  %arrayidx48.5.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom43.5.i, i64 5, i64 %indvars.iv.5.i
  %219 = load i32, ptr %arrayidx48.5.i, align 4, !tbaa !18
  %add49.5.i = add nsw i32 %219, 1
  %shr50.5.i = ashr i32 %add49.5.i, 1
  store i32 %shr50.5.i, ptr %arrayidx48.5.i, align 4, !tbaa !18
  %indvars.iv.next.5.i = add nuw nsw i64 %indvars.iv.5.i, 1
  %exitcond.5.not.i = icmp eq i64 %indvars.iv.next.5.i, 64
  br i1 %exitcond.5.not.i, label %for.body25.6.i, label %for.body25.5.i, !llvm.loop !53

for.body25.6.i:                                   ; preds = %for.body25.5.i, %for.body25.6.i
  %indvars.iv.6.i = phi i64 [ %indvars.iv.next.6.i, %for.body25.6.i ], [ 0, %for.body25.5.i ]
  %220 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom27.6.i = sext i32 %220 to i64
  %arrayidx32.6.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom27.6.i, i64 6, i64 %indvars.iv.6.i
  %221 = load i32, ptr %arrayidx32.6.i, align 4, !tbaa !18
  %add33.6.i = add nsw i32 %221, 1
  %shr34.6.i = ashr i32 %add33.6.i, 1
  store i32 %shr34.6.i, ptr %arrayidx32.6.i, align 4, !tbaa !18
  %222 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom43.6.i = sext i32 %222 to i64
  %arrayidx48.6.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom43.6.i, i64 6, i64 %indvars.iv.6.i
  %223 = load i32, ptr %arrayidx48.6.i, align 4, !tbaa !18
  %add49.6.i = add nsw i32 %223, 1
  %shr50.6.i = ashr i32 %add49.6.i, 1
  store i32 %shr50.6.i, ptr %arrayidx48.6.i, align 4, !tbaa !18
  %indvars.iv.next.6.i = add nuw nsw i64 %indvars.iv.6.i, 1
  %exitcond.6.not.i = icmp eq i64 %indvars.iv.next.6.i, 64
  br i1 %exitcond.6.not.i, label %for.body25.7.i, label %for.body25.6.i, !llvm.loop !53

for.body25.7.i:                                   ; preds = %for.body25.6.i, %for.body25.7.i
  %indvars.iv.7.i = phi i64 [ %indvars.iv.next.7.i, %for.body25.7.i ], [ 0, %for.body25.6.i ]
  %224 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom27.7.i = sext i32 %224 to i64
  %arrayidx32.7.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom27.7.i, i64 7, i64 %indvars.iv.7.i
  %225 = load i32, ptr %arrayidx32.7.i, align 4, !tbaa !18
  %add33.7.i = add nsw i32 %225, 1
  %shr34.7.i = ashr i32 %add33.7.i, 1
  store i32 %shr34.7.i, ptr %arrayidx32.7.i, align 4, !tbaa !18
  %226 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom43.7.i = sext i32 %226 to i64
  %arrayidx48.7.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom43.7.i, i64 7, i64 %indvars.iv.7.i
  %227 = load i32, ptr %arrayidx48.7.i, align 4, !tbaa !18
  %add49.7.i = add nsw i32 %227, 1
  %shr50.7.i = ashr i32 %add49.7.i, 1
  store i32 %shr50.7.i, ptr %arrayidx48.7.i, align 4, !tbaa !18
  %indvars.iv.next.7.i = add nuw nsw i64 %indvars.iv.7.i, 1
  %exitcond.7.not.i = icmp eq i64 %indvars.iv.next.7.i, 64
  br i1 %exitcond.7.not.i, label %for.body25.8.i, label %for.body25.7.i, !llvm.loop !53

for.body25.8.i:                                   ; preds = %for.body25.7.i, %for.body25.8.i
  %indvars.iv.8.i = phi i64 [ %indvars.iv.next.8.i, %for.body25.8.i ], [ 0, %for.body25.7.i ]
  %228 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom27.8.i = sext i32 %228 to i64
  %arrayidx32.8.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom27.8.i, i64 8, i64 %indvars.iv.8.i
  %229 = load i32, ptr %arrayidx32.8.i, align 4, !tbaa !18
  %add33.8.i = add nsw i32 %229, 1
  %shr34.8.i = ashr i32 %add33.8.i, 1
  store i32 %shr34.8.i, ptr %arrayidx32.8.i, align 4, !tbaa !18
  %230 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom43.8.i = sext i32 %230 to i64
  %arrayidx48.8.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom43.8.i, i64 8, i64 %indvars.iv.8.i
  %231 = load i32, ptr %arrayidx48.8.i, align 4, !tbaa !18
  %add49.8.i = add nsw i32 %231, 1
  %shr50.8.i = ashr i32 %add49.8.i, 1
  store i32 %shr50.8.i, ptr %arrayidx48.8.i, align 4, !tbaa !18
  %indvars.iv.next.8.i = add nuw nsw i64 %indvars.iv.8.i, 1
  %exitcond.8.not.i = icmp eq i64 %indvars.iv.next.8.i, 64
  br i1 %exitcond.8.not.i, label %for.body25.9.i, label %for.body25.8.i, !llvm.loop !53

for.body25.9.i:                                   ; preds = %for.body25.8.i, %for.body25.9.i
  %indvars.iv.9.i = phi i64 [ %indvars.iv.next.9.i, %for.body25.9.i ], [ 0, %for.body25.8.i ]
  %232 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom27.9.i = sext i32 %232 to i64
  %arrayidx32.9.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom27.9.i, i64 9, i64 %indvars.iv.9.i
  %233 = load i32, ptr %arrayidx32.9.i, align 4, !tbaa !18
  %add33.9.i = add nsw i32 %233, 1
  %shr34.9.i = ashr i32 %add33.9.i, 1
  store i32 %shr34.9.i, ptr %arrayidx32.9.i, align 4, !tbaa !18
  %234 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom43.9.i = sext i32 %234 to i64
  %arrayidx48.9.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom43.9.i, i64 9, i64 %indvars.iv.9.i
  %235 = load i32, ptr %arrayidx48.9.i, align 4, !tbaa !18
  %add49.9.i = add nsw i32 %235, 1
  %shr50.9.i = ashr i32 %add49.9.i, 1
  store i32 %shr50.9.i, ptr %arrayidx48.9.i, align 4, !tbaa !18
  %indvars.iv.next.9.i = add nuw nsw i64 %indvars.iv.9.i, 1
  %exitcond.9.not.i = icmp eq i64 %indvars.iv.next.9.i, 64
  br i1 %exitcond.9.not.i, label %for.body25.10.i, label %for.body25.9.i, !llvm.loop !53

for.body25.10.i:                                  ; preds = %for.body25.9.i, %for.body25.10.i
  %indvars.iv.10.i = phi i64 [ %indvars.iv.next.10.i, %for.body25.10.i ], [ 0, %for.body25.9.i ]
  %236 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom27.10.i = sext i32 %236 to i64
  %arrayidx32.10.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom27.10.i, i64 10, i64 %indvars.iv.10.i
  %237 = load i32, ptr %arrayidx32.10.i, align 4, !tbaa !18
  %add33.10.i = add nsw i32 %237, 1
  %shr34.10.i = ashr i32 %add33.10.i, 1
  store i32 %shr34.10.i, ptr %arrayidx32.10.i, align 4, !tbaa !18
  %238 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom43.10.i = sext i32 %238 to i64
  %arrayidx48.10.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom43.10.i, i64 10, i64 %indvars.iv.10.i
  %239 = load i32, ptr %arrayidx48.10.i, align 4, !tbaa !18
  %add49.10.i = add nsw i32 %239, 1
  %shr50.10.i = ashr i32 %add49.10.i, 1
  store i32 %shr50.10.i, ptr %arrayidx48.10.i, align 4, !tbaa !18
  %indvars.iv.next.10.i = add nuw nsw i64 %indvars.iv.10.i, 1
  %exitcond.10.not.i = icmp eq i64 %indvars.iv.next.10.i, 64
  br i1 %exitcond.10.not.i, label %for.body25.11.i, label %for.body25.10.i, !llvm.loop !53

for.body25.11.i:                                  ; preds = %for.body25.10.i, %for.body25.11.i
  %indvars.iv.11.i = phi i64 [ %indvars.iv.next.11.i, %for.body25.11.i ], [ 0, %for.body25.10.i ]
  %240 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom27.11.i = sext i32 %240 to i64
  %arrayidx32.11.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom27.11.i, i64 11, i64 %indvars.iv.11.i
  %241 = load i32, ptr %arrayidx32.11.i, align 4, !tbaa !18
  %add33.11.i = add nsw i32 %241, 1
  %shr34.11.i = ashr i32 %add33.11.i, 1
  store i32 %shr34.11.i, ptr %arrayidx32.11.i, align 4, !tbaa !18
  %242 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom43.11.i = sext i32 %242 to i64
  %arrayidx48.11.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom43.11.i, i64 11, i64 %indvars.iv.11.i
  %243 = load i32, ptr %arrayidx48.11.i, align 4, !tbaa !18
  %add49.11.i = add nsw i32 %243, 1
  %shr50.11.i = ashr i32 %add49.11.i, 1
  store i32 %shr50.11.i, ptr %arrayidx48.11.i, align 4, !tbaa !18
  %indvars.iv.next.11.i = add nuw nsw i64 %indvars.iv.11.i, 1
  %exitcond.11.not.i = icmp eq i64 %indvars.iv.next.11.i, 64
  br i1 %exitcond.11.not.i, label %_ZL11history_badP7state_tii.exit, label %for.body25.11.i, !llvm.loop !53

_ZL11history_badP7state_tii.exit:                 ; preds = %for.body25.11.i, %for.body967, %if.then.i
  %indvars.iv.next1708 = add nuw nsw i64 %indvars.iv1707, 1
  %exitcond1711.not = icmp eq i64 %indvars.iv.next1708, %wide.trip.count1710
  br i1 %exitcond1711.not, label %for.end972, label %for.body967, !llvm.loop !54

for.end972:                                       ; preds = %_ZL11history_badP7state_tii.exit, %if.then961
  %244 = load i32, ptr %arrayidx685.le, align 4, !tbaa !18
  %call975 = call noundef zeroext i16 @_Z12compact_movei(i32 noundef %244)
  %conv976 = zext i16 %call975 to i32
  %245 = load i32, ptr %threat, align 4, !tbaa !18
  %246 = load i32, ptr %singular, align 4, !tbaa !18
  %247 = load i32, ptr %nosingular, align 4, !tbaa !18
  call void @_Z7StoreTTP7state_tiiijiiii(ptr noundef %s, i32 noundef %score.17, i32 noundef %alpha.addr.0, i32 noundef %beta.addr.0, i32 noundef %conv976, i32 noundef %245, i32 noundef %246, i32 noundef %247, i32 noundef %depth)
  br label %cleanup1026

if.end977:                                        ; preds = %if.then959
  %call980 = call noundef zeroext i16 @_Z12compact_movei(i32 noundef %187)
  %conv981 = zext i16 %call980 to i32
  store i32 %conv981, ptr %best, align 4, !tbaa !18
  br label %if.end982

if.end982:                                        ; preds = %if.end977, %if.then957
  %alpha.addr.2 = phi i32 [ %score.17, %if.end977 ], [ %alpha.addr.1.ph1675, %if.then957 ]
  %248 = load i32, ptr %arrayidx685.le, align 4, !tbaa !18
  %sub985 = add nsw i32 %mn.0.ph1670, -1
  %idxprom986 = sext i32 %sub985 to i64
  %arrayidx987 = getelementptr inbounds [240 x i32], ptr %searched_moves, i64 0, i64 %idxprom986
  store i32 %248, ptr %arrayidx987, align 4, !tbaa !18
  %inc988 = add nsw i32 %mn.0.ph1670, 1
  br label %if.end989

if.end989:                                        ; preds = %if.end982, %if.end955
  %mn.1 = phi i32 [ %inc988, %if.end982 ], [ %mn.0.ph1670, %if.end955 ]
  %first.3 = phi i32 [ 0, %if.end982 ], [ %first.2.ph1671, %if.end955 ]
  %alpha.addr.3 = phi i32 [ %alpha.addr.2, %if.end982 ], [ %alpha.addr.1.ph1675, %if.end955 ]
  %inc.i1569 = add nsw i32 %.lcssa1684, 1
  store i32 %inc.i1569, ptr %i, align 4, !tbaa !18
  %cmp.i1570 = icmp slt i32 %.lcssa1684, 9
  %cmp165.i1571 = icmp slt i32 %inc.i1569, %num_moves.01609
  br i1 %cmp.i1570, label %for.cond.preheader.i1572, label %if.else.i1586

for.cond.preheader.i1572:                         ; preds = %if.end989
  br i1 %cmp165.i1571, label %for.body.preheader.i1573, label %while.end998

for.body.preheader.i1573:                         ; preds = %for.cond.preheader.i1572
  %249 = add nsw i64 %idxprom684.le.pre-phi, 1
  %250 = trunc i64 %idxprom684.le.pre-phi to i32
  %251 = xor i32 %250, -1
  %252 = add i32 %num_moves.01609, %251
  %253 = sub i32 %112, %250
  %xtraiter = and i32 %252, 3
  %254 = icmp ult i32 %253, 3
  br i1 %254, label %if.end12.i1588.unr-lcssa, label %for.body.preheader.i1573.new

for.body.preheader.i1573.new:                     ; preds = %for.body.preheader.i1573
  %unroll_iter = and i32 %252, -4
  br label %for.body.i1584

for.body.i1584:                                   ; preds = %for.body.i1584, %for.body.preheader.i1573.new
  %indvars.iv.i1574 = phi i64 [ %249, %for.body.preheader.i1573.new ], [ %indvars.iv.next.i1581.3, %for.body.i1584 ]
  %tmp.068.i1575 = phi i32 [ undef, %for.body.preheader.i1573.new ], [ %spec.select60.i1580.3, %for.body.i1584 ]
  %best.067.i1576 = phi i32 [ -1000000, %for.body.preheader.i1573.new ], [ %spec.select.i1579.3, %for.body.i1584 ]
  %niter = phi i32 [ 0, %for.body.preheader.i1573.new ], [ %niter.next.3, %for.body.i1584 ]
  %arrayidx.i1577 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.i1574
  %255 = load i32, ptr %arrayidx.i1577, align 4, !tbaa !18
  %cmp2.i1578 = icmp sgt i32 %255, %best.067.i1576
  %spec.select.i1579 = call i32 @llvm.smax.i32(i32 %255, i32 %best.067.i1576)
  %256 = trunc i64 %indvars.iv.i1574 to i32
  %spec.select60.i1580 = select i1 %cmp2.i1578, i32 %256, i32 %tmp.068.i1575
  %indvars.iv.next.i1581 = add nsw i64 %indvars.iv.i1574, 1
  %arrayidx.i1577.1 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.next.i1581
  %257 = load i32, ptr %arrayidx.i1577.1, align 4, !tbaa !18
  %cmp2.i1578.1 = icmp sgt i32 %257, %spec.select.i1579
  %spec.select.i1579.1 = call i32 @llvm.smax.i32(i32 %257, i32 %spec.select.i1579)
  %258 = trunc i64 %indvars.iv.next.i1581 to i32
  %spec.select60.i1580.1 = select i1 %cmp2.i1578.1, i32 %258, i32 %spec.select60.i1580
  %indvars.iv.next.i1581.1 = add nsw i64 %indvars.iv.i1574, 2
  %arrayidx.i1577.2 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.next.i1581.1
  %259 = load i32, ptr %arrayidx.i1577.2, align 4, !tbaa !18
  %cmp2.i1578.2 = icmp sgt i32 %259, %spec.select.i1579.1
  %spec.select.i1579.2 = call i32 @llvm.smax.i32(i32 %259, i32 %spec.select.i1579.1)
  %260 = trunc i64 %indvars.iv.next.i1581.1 to i32
  %spec.select60.i1580.2 = select i1 %cmp2.i1578.2, i32 %260, i32 %spec.select60.i1580.1
  %indvars.iv.next.i1581.2 = add nsw i64 %indvars.iv.i1574, 3
  %arrayidx.i1577.3 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.next.i1581.2
  %261 = load i32, ptr %arrayidx.i1577.3, align 4, !tbaa !18
  %cmp2.i1578.3 = icmp sgt i32 %261, %spec.select.i1579.2
  %spec.select.i1579.3 = call i32 @llvm.smax.i32(i32 %261, i32 %spec.select.i1579.2)
  %262 = trunc i64 %indvars.iv.next.i1581.2 to i32
  %spec.select60.i1580.3 = select i1 %cmp2.i1578.3, i32 %262, i32 %spec.select60.i1580.2
  %indvars.iv.next.i1581.3 = add nsw i64 %indvars.iv.i1574, 4
  %niter.next.3 = add i32 %niter, 4
  %niter.ncmp.3 = icmp eq i32 %niter.next.3, %unroll_iter
  br i1 %niter.ncmp.3, label %if.end12.i1588.unr-lcssa, label %for.body.i1584, !llvm.loop !20

if.else.i1586:                                    ; preds = %if.end989
  %spec.select64.i1585 = zext i1 %cmp165.i1571 to i32
  br label %cleanup993

if.end12.i1588.unr-lcssa:                         ; preds = %for.body.i1584, %for.body.preheader.i1573
  %spec.select.i1579.lcssa.ph = phi i32 [ undef, %for.body.preheader.i1573 ], [ %spec.select.i1579.3, %for.body.i1584 ]
  %indvars.iv.i1574.unr = phi i64 [ %249, %for.body.preheader.i1573 ], [ %indvars.iv.next.i1581.3, %for.body.i1584 ]
  %tmp.068.i1575.unr = phi i32 [ undef, %for.body.preheader.i1573 ], [ %spec.select60.i1580.3, %for.body.i1584 ]
  %best.067.i1576.unr = phi i32 [ -1000000, %for.body.preheader.i1573 ], [ %spec.select.i1579.3, %for.body.i1584 ]
  %lcmp.mod.not = icmp eq i32 %xtraiter, 0
  br i1 %lcmp.mod.not, label %if.end12.i1588, label %for.body.i1584.epil

for.body.i1584.epil:                              ; preds = %if.end12.i1588.unr-lcssa, %for.body.i1584.epil
  %indvars.iv.i1574.epil = phi i64 [ %indvars.iv.next.i1581.epil, %for.body.i1584.epil ], [ %indvars.iv.i1574.unr, %if.end12.i1588.unr-lcssa ]
  %tmp.068.i1575.epil = phi i32 [ %spec.select60.i1580.epil, %for.body.i1584.epil ], [ %tmp.068.i1575.unr, %if.end12.i1588.unr-lcssa ]
  %best.067.i1576.epil = phi i32 [ %spec.select.i1579.epil, %for.body.i1584.epil ], [ %best.067.i1576.unr, %if.end12.i1588.unr-lcssa ]
  %epil.iter = phi i32 [ %epil.iter.next, %for.body.i1584.epil ], [ 0, %if.end12.i1588.unr-lcssa ]
  %arrayidx.i1577.epil = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.i1574.epil
  %263 = load i32, ptr %arrayidx.i1577.epil, align 4, !tbaa !18
  %cmp2.i1578.epil = icmp sgt i32 %263, %best.067.i1576.epil
  %spec.select.i1579.epil = call i32 @llvm.smax.i32(i32 %263, i32 %best.067.i1576.epil)
  %264 = trunc i64 %indvars.iv.i1574.epil to i32
  %spec.select60.i1580.epil = select i1 %cmp2.i1578.epil, i32 %264, i32 %tmp.068.i1575.epil
  %indvars.iv.next.i1581.epil = add nsw i64 %indvars.iv.i1574.epil, 1
  %epil.iter.next = add i32 %epil.iter, 1
  %epil.iter.cmp.not = icmp eq i32 %epil.iter.next, %xtraiter
  br i1 %epil.iter.cmp.not, label %if.end12.i1588, label %for.body.i1584.epil, !llvm.loop !55

if.end12.i1588:                                   ; preds = %for.body.i1584.epil, %if.end12.i1588.unr-lcssa
  %spec.select.i1579.lcssa = phi i32 [ %spec.select.i1579.lcssa.ph, %if.end12.i1588.unr-lcssa ], [ %spec.select.i1579.epil, %for.body.i1584.epil ]
  %spec.select60.i1580.lcssa = phi i32 [ %tmp.068.i1575.unr, %if.end12.i1588.unr-lcssa ], [ %spec.select60.i1580.epil, %for.body.i1584.epil ]
  %cmp13.i1587 = icmp sgt i32 %spec.select.i1579.lcssa, -1000000
  br i1 %cmp13.i1587, label %if.then14.i1594, label %while.end998

if.then14.i1594:                                  ; preds = %if.end12.i1588
  %265 = sext i32 %spec.select60.i1580.lcssa to i64
  %idxprom15.i1589 = sext i32 %inc.i1569 to i64
  %arrayidx16.i1590 = getelementptr inbounds i32, ptr %move_ordering, i64 %idxprom15.i1589
  %266 = load i32, ptr %arrayidx16.i1590, align 4, !tbaa !18
  %arrayidx18.i1591 = getelementptr inbounds i32, ptr %move_ordering, i64 %265
  store i32 %266, ptr %arrayidx18.i1591, align 4, !tbaa !18
  store i32 %spec.select.i1579.lcssa, ptr %arrayidx16.i1590, align 4, !tbaa !18
  %arrayidx22.i1592 = getelementptr inbounds i32, ptr %moves, i64 %idxprom15.i1589
  %267 = load i32, ptr %arrayidx22.i1592, align 4, !tbaa !18
  %arrayidx24.i1593 = getelementptr inbounds i32, ptr %moves, i64 %265
  %268 = load i32, ptr %arrayidx24.i1593, align 4, !tbaa !18
  store i32 %268, ptr %arrayidx22.i1592, align 4, !tbaa !18
  store i32 %267, ptr %arrayidx24.i1593, align 4, !tbaa !18
  br label %cleanup993

cleanup993:                                       ; preds = %if.then14.i1594, %if.else.i1586, %if.then813, %if.then796
  %remoneflag.3 = phi i32 [ %call818, %if.then813 ], [ %call801, %if.then796 ], [ 1, %if.then14.i1594 ], [ %spec.select64.i1585, %if.else.i1586 ]
  %mn.2 = phi i32 [ %mn.0.ph1670, %if.then813 ], [ %mn.0.ph1670, %if.then796 ], [ %mn.1, %if.then14.i1594 ], [ %mn.1, %if.else.i1586 ]
  %first.4 = phi i32 [ %first.2.ph1671, %if.then813 ], [ %first.2.ph1671, %if.then796 ], [ %first.3, %if.then14.i1594 ], [ %first.3, %if.else.i1586 ]
  %best_score.3 = phi i32 [ %alpha.addr.1.ph1675, %if.then813 ], [ %alpha.addr.1.ph1675, %if.then796 ], [ %best_score.2, %if.then14.i1594 ], [ %best_score.2, %if.else.i1586 ]
  %no_moves.2 = phi i32 [ 0, %if.then813 ], [ 0, %if.then796 ], [ %no_moves.1, %if.then14.i1594 ], [ %no_moves.1, %if.else.i1586 ]
  %score.18 = phi i32 [ %score.14.ph1674, %if.then813 ], [ %score.14.ph1674, %if.then796 ], [ %score.17, %if.then14.i1594 ], [ %score.17, %if.else.i1586 ]
  %alpha.addr.4 = phi i32 [ %alpha.addr.1.ph1675, %if.then813 ], [ %alpha.addr.1.ph1675, %if.then796 ], [ %alpha.addr.3, %if.then14.i1594 ], [ %alpha.addr.3, %if.else.i1586 ]
  %tobool520.not1663 = icmp eq i32 %remoneflag.3, 0
  br i1 %tobool520.not1663, label %while.end998, label %while.body521.lr.ph

while.end998.thread:                              ; preds = %if.then712.peel, %if.then712
  %269 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %tobool999.not1742 = icmp eq i32 %269, 0
  br label %if.else1014

while.end998:                                     ; preds = %for.cond.preheader.i1572, %if.end12.i1588, %cleanup993, %land.end514
  %best_score.0.ph.lcssa1633 = phi i32 [ -32000, %land.end514 ], [ %best_score.2, %for.cond.preheader.i1572 ], [ %best_score.2, %if.end12.i1588 ], [ %best_score.3, %cleanup993 ]
  %no_moves.0.lcssa = phi i32 [ 1, %land.end514 ], [ %no_moves.1, %for.cond.preheader.i1572 ], [ %no_moves.1, %if.end12.i1588 ], [ %no_moves.2, %cleanup993 ]
  %270 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %tobool999.not = icmp eq i32 %270, 0
  %tobool1002 = icmp ne i32 %no_moves.0.lcssa, 0
  %or.cond1090 = select i1 %tobool1002, i1 %tobool999.not, i1 false
  br i1 %or.cond1090, label %if.then1005, label %if.else1014

if.then1005:                                      ; preds = %while.end998
  %call1006 = call noundef i32 @_Z8in_checkP7state_t(ptr noundef %s)
  %tobool1007.not = icmp eq i32 %call1006, 0
  br i1 %tobool1007.not, label %if.else1013, label %if.then1008

if.then1008:                                      ; preds = %if.then1005
  %271 = load i32, ptr %ply, align 8, !tbaa !10
  %add1010 = add nsw i32 %271, -32000
  %272 = load i32, ptr %threat, align 4, !tbaa !18
  %273 = load i32, ptr %singular, align 4, !tbaa !18
  %274 = load i32, ptr %nosingular, align 4, !tbaa !18
  call void @_Z7StoreTTP7state_tiiijiiii(ptr noundef %s, i32 noundef %add1010, i32 noundef %alpha.addr.0, i32 noundef %beta.addr.0, i32 noundef 0, i32 noundef %272, i32 noundef %273, i32 noundef %274, i32 noundef %depth)
  %275 = load i32, ptr %ply, align 8, !tbaa !10
  %add1012 = add nsw i32 %275, -32000
  br label %cleanup1026

if.else1013:                                      ; preds = %if.then1005
  %276 = load i32, ptr %threat, align 4, !tbaa !18
  %277 = load i32, ptr %singular, align 4, !tbaa !18
  %278 = load i32, ptr %nosingular, align 4, !tbaa !18
  call void @_Z7StoreTTP7state_tiiijiiii(ptr noundef %s, i32 noundef 0, i32 noundef %alpha.addr.0, i32 noundef %beta.addr.0, i32 noundef 0, i32 noundef %276, i32 noundef %277, i32 noundef %278, i32 noundef %depth)
  br label %cleanup1026

if.else1014:                                      ; preds = %while.end998.thread, %while.end998
  %tobool999.not1746 = phi i1 [ %tobool999.not1742, %while.end998.thread ], [ %tobool999.not, %while.end998 ]
  %best_score.0.ph.lcssa16331745 = phi i32 [ %best_score.0.ph1672, %while.end998.thread ], [ %best_score.0.ph.lcssa1633, %while.end998 ]
  %279 = load i32, ptr %fifty, align 4, !tbaa !14
  %cmp1016 = icmp slt i32 %279, 99
  %brmerge.not = select i1 %cmp1016, i1 %tobool999.not1746, i1 false
  %.mux = select i1 %cmp1016, i32 %best_score.0.ph.lcssa16331745, i32 0
  br i1 %brmerge.not, label %if.then1021, label %cleanup1026

if.then1021:                                      ; preds = %if.else1014
  %280 = load i32, ptr %best, align 4, !tbaa !18
  %281 = load i32, ptr %threat, align 4, !tbaa !18
  %282 = load i32, ptr %singular, align 4, !tbaa !18
  %283 = load i32, ptr %nosingular, align 4, !tbaa !18
  call void @_Z7StoreTTP7state_tiiijiiii(ptr noundef nonnull %s, i32 noundef %best_score.0.ph.lcssa16331745, i32 noundef %alpha.addr.0, i32 noundef %beta.addr.0, i32 noundef %280, i32 noundef %281, i32 noundef %282, i32 noundef %283, i32 noundef %depth)
  br label %cleanup1026

cleanup1026:                                      ; preds = %if.end950, %for.end972, %if.then197, %if.then203, %cleanup401, %if.then179, %if.end168, %if.else1014, %if.then52, %if.then56, %if.then63, %if.end121, %if.then1008, %if.else1013, %if.then1021, %sw.bb33, %sw.bb29, %if.then23, %if.then14, %if.end, %sw.bb, %if.then9, %if.then
  %retval.12 = phi i32 [ %call, %if.then ], [ %cond, %if.then9 ], [ %7, %sw.bb ], [ 0, %if.end ], [ %add, %if.then14 ], [ %sub21, %if.then23 ], [ %8, %sw.bb29 ], [ %9, %sw.bb33 ], [ %beta.addr.0, %cleanup401 ], [ 0, %if.end121 ], [ %call43, %if.then52 ], [ %call57, %if.then56 ], [ %call43, %if.then63 ], [ %add1012, %if.then1008 ], [ 0, %if.else1013 ], [ %.mux, %if.else1014 ], [ %best_score.0.ph.lcssa16331745, %if.then1021 ], [ %score.2, %if.then179 ], [ 0, %if.end168 ], [ 0, %if.then197 ], [ %alpha.addr.0, %if.then203 ], [ %score.17, %for.end972 ], [ 0, %if.end950 ]
  call void @llvm.lifetime.end.p0(i64 960, ptr nonnull %searched_moves) #9
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %nosingular) #9
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %singular) #9
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %best) #9
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %donull) #9
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %threat) #9
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %bound) #9
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %i) #9
  call void @llvm.lifetime.end.p0(i64 960, ptr nonnull %move_ordering) #9
  call void @llvm.lifetime.end.p0(i64 960, ptr nonnull %moves) #9
  ret i32 %retval.12
}

declare noundef i32 @_Z3genP7state_tPi(ptr noundef, ptr noundef) local_unnamed_addr #2

; Function Attrs: mustprogress uwtable
define internal fastcc void @_ZL11order_movesP7state_tPiS1_ij(ptr noundef %s, ptr nocapture noundef readonly %moves, ptr nocapture noundef writeonly %move_ordering, i32 noundef %num_moves, i32 noundef %best) unnamed_addr #0 {
entry:
  call void @mcount()
  %cmp224 = icmp sgt i32 %num_moves, 0
  br i1 %cmp224, label %for.body.lr.ph, label %for.end

for.body.lr.ph:                                   ; preds = %entry
  %ply = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 14
  %cmp5.i = icmp ult i32 %num_moves, 6
  %cmp8.i = icmp ult i32 %num_moves, 9
  %spec.select.i = select i1 %cmp8.i, i32 40, i32 100
  %.pn.i = select i1 %cmp5.i, i32 20, i32 %spec.select.i
  %arrayidx19.i = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 9
  %arrayidx21.i = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 7
  %arrayidx24.i = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 11
  %arrayidx27.i = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 3
  %arrayidx30.i = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 10
  %arrayidx33.i = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 12
  %arrayidx36.i = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 4
  %arrayidx39.i = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 8
  %arrayidx42.i = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 1
  %arrayidx45.i = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 8, i64 2
  %white_to_move.i = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 11
  %wide.trip.count = zext i32 %num_moves to i64
  br label %for.body

for.body:                                         ; preds = %for.body.lr.ph, %for.inc
  %indvars.iv = phi i64 [ 0, %for.body.lr.ph ], [ %indvars.iv.next, %for.inc ]
  %arrayidx = getelementptr inbounds i32, ptr %moves, i64 %indvars.iv
  %0 = load i32, ptr %arrayidx, align 4, !tbaa !18
  %1 = lshr i32 %0, 6
  %and = and i32 %1, 63
  %and3 = and i32 %0, 63
  %2 = lshr i32 %0, 12
  %and7 = and i32 %2, 15
  %3 = lshr i32 %0, 19
  %and11 = and i32 %3, 15
  %4 = load i32, ptr @uci_multipv, align 4, !tbaa !18
  %cmp12 = icmp sgt i32 %4, 1
  br i1 %cmp12, label %land.lhs.true, label %if.end

land.lhs.true:                                    ; preds = %for.body
  %5 = load i32, ptr %ply, align 8, !tbaa !10
  %cmp13 = icmp eq i32 %5, 1
  %6 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4
  %cmp15 = icmp sgt i32 %6, 2
  %or.cond = select i1 %cmp13, i1 %cmp15, i1 false
  br i1 %or.cond, label %if.then, label %if.end

if.then:                                          ; preds = %land.lhs.true
  %arrayidx17 = getelementptr inbounds [240 x i32], ptr @root_scores, i64 0, i64 %indvars.iv
  %7 = load i32, ptr %arrayidx17, align 4, !tbaa !18
  %add = add nsw i32 %7, 100000
  %arrayidx19 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv
  store i32 %add, ptr %arrayidx19, align 4, !tbaa !18
  br label %for.inc

if.end:                                           ; preds = %land.lhs.true, %for.body
  %8 = load i32, ptr @uci_limitstrength, align 4, !tbaa !18
  %tobool.not = icmp eq i32 %8, 0
  br i1 %tobool.not, label %if.end33, label %land.lhs.true20

land.lhs.true20:                                  ; preds = %if.end
  %9 = load i32, ptr %ply, align 8, !tbaa !10
  %cmp22 = icmp sgt i32 %9, 1
  br i1 %cmp22, label %if.then23, label %if.end33

if.then23:                                        ; preds = %land.lhs.true20
  %arrayidx27 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv
  %idxprom.i = zext i32 %9 to i64
  %arrayidx.i = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 25, i64 %idxprom.i
  %10 = load i32, ptr %arrayidx.i, align 4, !tbaa !18
  %tobool.not.i = icmp eq i32 %10, 0
  br i1 %tobool.not.i, label %if.end.i, label %if.end33

if.end.i:                                         ; preds = %if.then23
  %11 = load i32, ptr @uci_elo, align 4, !tbaa !18
  %call.i = tail call noundef i32 @_Z14elo_to_blunderi(i32 noundef %11)
  %cmp.i = icmp sgt i32 %call.i, 99
  br i1 %cmp.i, label %if.end33, label %if.end2.i

if.end2.i:                                        ; preds = %if.end.i
  %12 = load i32, ptr @uci_elo, align 4, !tbaa !18
  %call3.i = tail call noundef i32 @_Z14elo_to_blunderi(i32 noundef %12)
  %call4.i = tail call noundef i32 @_Z8myrandomv()
  %rem.i = urem i32 %call4.i, 200
  %13 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4, !tbaa !19
  %cmp13.i = icmp slt i32 %13, 5
  %sub15.i = sub nsw i32 5, %13
  %add.i = select i1 %cmp13.i, i32 %sub15.i, i32 0
  %14 = load i32, ptr %ply, align 8, !tbaa !10
  %mul.i = shl nsw i32 %14, 1
  %15 = load i32, ptr %arrayidx19.i, align 4, !tbaa !18
  %16 = load i32, ptr %arrayidx21.i, align 4, !tbaa !18
  %17 = load i32, ptr %arrayidx24.i, align 4, !tbaa !18
  %18 = load i32, ptr %arrayidx27.i, align 4, !tbaa !18
  %19 = load i32, ptr %arrayidx30.i, align 8, !tbaa !18
  %20 = load i32, ptr %arrayidx33.i, align 8, !tbaa !18
  %21 = load i32, ptr %arrayidx36.i, align 8, !tbaa !18
  %22 = load i32, ptr %arrayidx39.i, align 8, !tbaa !18
  %23 = load i32, ptr %arrayidx42.i, align 4, !tbaa !18
  %24 = load i32, ptr %arrayidx45.i, align 8, !tbaa !18
  %add22.i = sub i32 %.pn.i, %call3.i
  %add25.i = add i32 %add22.i, %add.i
  %add28.i = add i32 %add25.i, %mul.i
  %add31.i = add i32 %add28.i, %15
  %add34.i = add i32 %add31.i, %16
  %add37.i = add i32 %add34.i, %17
  %add40.i = add i32 %add37.i, %18
  %add43.i = add i32 %add40.i, %19
  %add46.i = add i32 %add43.i, %20
  %prob.0.i = add i32 %add46.i, %21
  %prob.1.i = add i32 %prob.0.i, %22
  %add18.i = add i32 %prob.1.i, %23
  %add47.i = add i32 %add18.i, %24
  %idxprom50.i = zext i32 %and to i64
  %arrayidx51.i = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 1, i64 %idxprom50.i
  %25 = load i32, ptr %arrayidx51.i, align 4, !tbaa !18
  %cmp52.i = icmp eq i32 %and11, %25
  %sub54.i = add nsw i32 %add47.i, -30
  %prob.2.i = select i1 %cmp52.i, i32 %sub54.i, i32 %add47.i
  %.off227.i = add i32 %25, -1
  %switch228.i = icmp ult i32 %.off227.i, 2
  %sub69.i = add nsw i32 %prob.2.i, -30
  %spec.select229.i = select i1 %switch228.i, i32 %sub69.i, i32 %prob.2.i
  %cmp73.not.i = icmp eq i32 %and11, 13
  %sub75.i = add nsw i32 %spec.select229.i, -10
  %spec.select223.i = select i1 %cmp73.not.i, i32 %spec.select229.i, i32 %sub75.i
  %and.off.i = add nsw i32 %and11, -9
  %switch.i = icmp ult i32 %and.off.i, 2
  %sub85.i = add nsw i32 %spec.select223.i, -30
  %spec.select224.i = select i1 %switch.i, i32 %sub85.i, i32 %spec.select223.i
  switch i32 %and7, label %if.then101.i [
    i32 0, label %if.end103.i
    i32 13, label %if.end103.i
    i32 9, label %if.end103.i
    i32 10, label %if.end103.i
  ]

if.then101.i:                                     ; preds = %if.end2.i
  %add102.i = add nsw i32 %spec.select224.i, 40
  br label %if.end103.i

if.end103.i:                                      ; preds = %if.then101.i, %if.end2.i, %if.end2.i, %if.end2.i, %if.end2.i
  %prob.6.i = phi i32 [ %add102.i, %if.then101.i ], [ %spec.select224.i, %if.end2.i ], [ %spec.select224.i, %if.end2.i ], [ %spec.select224.i, %if.end2.i ], [ %spec.select224.i, %if.end2.i ]
  %call107.i = tail call noundef i32 @_Z12taxicab_distii(i32 noundef %and, i32 noundef %and3)
  %mul108.i = shl nsw i32 %call107.i, 1
  %add109.i = add nsw i32 %mul108.i, %prob.6.i
  %26 = load i32, ptr %arrayidx51.i, align 4, !tbaa !18
  %.off.i = add i32 %26, -3
  %switch225.i = icmp ult i32 %.off.i, 2
  %add124.i = add nsw i32 %add109.i, 20
  %spec.select226.i = select i1 %switch225.i, i32 %add124.i, i32 %add109.i
  %27 = load i32, ptr %white_to_move.i, align 4, !tbaa !17
  %tobool126.not.i = icmp eq i32 %27, 0
  %call147.i = tail call noundef i32 @_Z4ranki(i32 noundef %and)
  %call149.i = tail call noundef i32 @_Z4ranki(i32 noundef %and3)
  br i1 %tobool126.not.i, label %if.else144.i, label %if.then127.i

if.then127.i:                                     ; preds = %if.end103.i
  %cmp133.i = icmp sgt i32 %call147.i, %call149.i
  br i1 %cmp133.i, label %if.end161.sink.split.i, label %if.end161.i

if.else144.i:                                     ; preds = %if.end103.i
  %cmp150.i = icmp slt i32 %call147.i, %call149.i
  br i1 %cmp150.i, label %if.end161.sink.split.i, label %if.end161.i

if.end161.sink.split.i:                           ; preds = %if.else144.i, %if.then127.i
  %call154.i = tail call noundef i32 @_Z4ranki(i32 noundef %and)
  %call156.i = tail call noundef i32 @_Z4ranki(i32 noundef %and3)
  %sub157.i = sub nsw i32 %call154.i, %call156.i
  %mul158.i = shl nsw i32 %sub157.i, 2
  %add159.i = add nsw i32 %mul158.i, %spec.select226.i
  br label %if.end161.i

if.end161.i:                                      ; preds = %if.end161.sink.split.i, %if.else144.i, %if.then127.i
  %prob.8.i = phi i32 [ %spec.select226.i, %if.then127.i ], [ %spec.select226.i, %if.else144.i ], [ %add159.i, %if.end161.sink.split.i ]
  %cmp162.not.i = icmp slt i32 %prob.8.i, %rem.i
  br i1 %cmp162.not.i, label %if.end33, label %if.then29

if.then29:                                        ; preds = %if.end161.i
  store i32 -1000000, ptr %arrayidx27, align 4, !tbaa !18
  br label %for.inc

if.end33:                                         ; preds = %if.end161.i, %if.end.i, %if.then23, %land.lhs.true20, %if.end
  %28 = load i32, ptr %arrayidx, align 4, !tbaa !18
  %call36 = tail call noundef zeroext i16 @_Z12compact_movei(i32 noundef %28)
  %conv = zext i16 %call36 to i32
  %cmp37 = icmp eq i32 %conv, %best
  br i1 %cmp37, label %if.then38, label %if.else

if.then38:                                        ; preds = %if.end33
  %arrayidx40 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv
  store i32 128000000, ptr %arrayidx40, align 4, !tbaa !18
  br label %for.inc

if.else:                                          ; preds = %if.end33
  %cmp41 = icmp ne i32 %and11, 13
  %tobool42 = icmp ne i32 %and7, 0
  %or.cond147 = select i1 %cmp41, i1 true, i1 %tobool42
  br i1 %or.cond147, label %if.then43, label %if.else77

if.then43:                                        ; preds = %if.else
  %idxprom44 = zext i32 %and11 to i64
  %arrayidx45 = getelementptr inbounds [14 x i32], ptr @material, i64 0, i64 %idxprom44
  %29 = load i32, ptr %arrayidx45, align 4, !tbaa !18
  %call46 = tail call i32 @llvm.abs.i32(i32 %29, i1 true)
  %idxprom47 = zext i32 %and to i64
  %arrayidx48 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 1, i64 %idxprom47
  %30 = load i32, ptr %arrayidx48, align 4, !tbaa !18
  %idxprom49 = sext i32 %30 to i64
  %arrayidx50 = getelementptr inbounds [14 x i32], ptr @material, i64 0, i64 %idxprom49
  %31 = load i32, ptr %arrayidx50, align 4, !tbaa !18
  %call51 = tail call i32 @llvm.abs.i32(i32 %31, i1 true)
  %sub = sub nsw i32 %call46, %call51
  %cmp52 = icmp sgt i32 %sub, 0
  br i1 %cmp52, label %if.then69, label %if.end67

if.end67:                                         ; preds = %if.then43
  %32 = load i32, ptr %white_to_move.i, align 4, !tbaa !17
  %tobool65.not = icmp eq i32 %32, 0
  %cond = zext i1 %tobool65.not to i32
  %call66 = tail call noundef i32 @_Z3seeP7state_tiiii(ptr noundef nonnull %s, i32 noundef %cond, i32 noundef %and, i32 noundef %and3, i32 noundef %and7)
  %cmp68 = icmp sgt i32 %call66, -1
  br i1 %cmp68, label %if.then69, label %if.else73

if.then69:                                        ; preds = %if.then43, %if.end67
  %seev.0223 = phi i32 [ %call66, %if.end67 ], [ %sub, %if.then43 ]
  %add70 = add nuw nsw i32 %seev.0223, 10000000
  %arrayidx72 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv
  store i32 %add70, ptr %arrayidx72, align 4, !tbaa !18
  br label %for.inc

if.else73:                                        ; preds = %if.end67
  %arrayidx75 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv
  store i32 %call66, ptr %arrayidx75, align 4, !tbaa !18
  br label %for.inc

if.else77:                                        ; preds = %if.else
  %33 = load i32, ptr %arrayidx, align 4, !tbaa !18
  %34 = load i32, ptr %ply, align 8, !tbaa !10
  %idxprom81 = sext i32 %34 to i64
  %arrayidx82 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 21, i64 %idxprom81
  %35 = load i32, ptr %arrayidx82, align 8, !tbaa !35
  %cmp83 = icmp eq i32 %33, %35
  br i1 %cmp83, label %if.then84, label %if.else87

if.then84:                                        ; preds = %if.else77
  %arrayidx86 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv
  store i32 8000000, ptr %arrayidx86, align 4, !tbaa !18
  br label %for.inc

if.else87:                                        ; preds = %if.else77
  %killer2 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 21, i64 %idxprom81, i32 1
  %36 = load i32, ptr %killer2, align 4, !tbaa !37
  %cmp94 = icmp eq i32 %33, %36
  br i1 %cmp94, label %if.then95, label %if.else98

if.then95:                                        ; preds = %if.else87
  %arrayidx97 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv
  store i32 7000000, ptr %arrayidx97, align 4, !tbaa !18
  br label %for.inc

if.else98:                                        ; preds = %if.else87
  %killer3 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 21, i64 %idxprom81, i32 2
  %37 = load i32, ptr %killer3, align 8, !tbaa !38
  %cmp105 = icmp eq i32 %33, %37
  br i1 %cmp105, label %if.then106, label %if.else109

if.then106:                                       ; preds = %if.else98
  %arrayidx108 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv
  store i32 6000000, ptr %arrayidx108, align 4, !tbaa !18
  br label %for.inc

if.else109:                                       ; preds = %if.else98
  %killer4 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 21, i64 %idxprom81, i32 3
  %38 = load i32, ptr %killer4, align 4, !tbaa !39
  %cmp116 = icmp eq i32 %33, %38
  br i1 %cmp116, label %if.then117, label %if.else120

if.then117:                                       ; preds = %if.else109
  %arrayidx119 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv
  store i32 5000000, ptr %arrayidx119, align 4, !tbaa !18
  br label %for.inc

if.else120:                                       ; preds = %if.else109
  %idxprom122 = zext i32 %and to i64
  %arrayidx123 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 1, i64 %idxprom122
  %39 = load i32, ptr %arrayidx123, align 4, !tbaa !18
  %sub124 = add nsw i32 %39, -1
  %40 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom125 = sext i32 %40 to i64
  %idxprom127 = sext i32 %sub124 to i64
  %idxprom129 = zext i32 %and3 to i64
  %arrayidx130 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom125, i64 %idxprom127, i64 %idxprom129
  %41 = load i32, ptr %arrayidx130, align 4, !tbaa !18
  %arrayidx137 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom125, i64 %idxprom127, i64 %idxprom122
  %42 = load i32, ptr %arrayidx137, align 4, !tbaa !18
  %sub138 = sub nsw i32 %41, %42
  %arrayidx140 = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv
  store i32 %sub138, ptr %arrayidx140, align 4, !tbaa !18
  br label %for.inc

for.inc:                                          ; preds = %if.then38, %if.then84, %if.then106, %if.else120, %if.then117, %if.then95, %if.then69, %if.else73, %if.then29, %if.then
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond.not = icmp eq i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond.not, label %for.end, label %for.body, !llvm.loop !56

for.end:                                          ; preds = %for.inc, %entry
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind memory(readwrite, inaccessiblemem: none) uwtable
define internal fastcc void @_ZL12history_goodP7state_tii(ptr nocapture noundef %s, i32 noundef %move, i32 noundef %depth) unnamed_addr #4 {
entry:
  call void @mcount()
  %0 = and i32 %move, 7925760
  %or.cond = icmp eq i32 %0, 6815744
  br i1 %or.cond, label %if.then, label %if.end157

if.then:                                          ; preds = %entry
  %1 = lshr i32 %move, 6
  %and4 = and i32 %1, 63
  %idxprom = zext i32 %and4 to i64
  %arrayidx = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 1, i64 %idxprom
  %2 = load i32, ptr %arrayidx, align 4, !tbaa !18
  %sub = add nsw i32 %2, -1
  %and5 = and i32 %move, 63
  %add = add nsw i32 %depth, 3
  %div = sdiv i32 %add, 4
  %3 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom6 = sext i32 %3 to i64
  %idxprom8 = sext i32 %sub to i64
  %idxprom10 = zext i32 %and5 to i64
  %arrayidx11 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom6, i64 %idxprom8, i64 %idxprom10
  %4 = load i32, ptr %arrayidx11, align 4, !tbaa !18
  %add12 = add nsw i32 %4, %div
  store i32 %add12, ptr %arrayidx11, align 4, !tbaa !18
  %5 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom16 = sext i32 %5 to i64
  %arrayidx21 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom16, i64 %idxprom8, i64 %idxprom10
  %6 = load i32, ptr %arrayidx21, align 4, !tbaa !18
  %add22 = add nsw i32 %6, %div
  store i32 %add22, ptr %arrayidx21, align 4, !tbaa !18
  %7 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom24 = sext i32 %7 to i64
  %arrayidx29 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom24, i64 %idxprom8, i64 %idxprom10
  %8 = load i32, ptr %arrayidx29, align 4, !tbaa !18
  %cmp30 = icmp sgt i32 %8, 16384
  br i1 %cmp30, label %for.body35, label %if.end

for.body35:                                       ; preds = %if.then, %for.body35
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.body35 ], [ 0, %if.then ]
  %9 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom37 = sext i32 %9 to i64
  %arrayidx42 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom37, i64 0, i64 %indvars.iv
  %10 = load i32, ptr %arrayidx42, align 4, !tbaa !18
  %add43 = add nsw i32 %10, 1
  %shr44 = ashr i32 %add43, 1
  store i32 %shr44, ptr %arrayidx42, align 4, !tbaa !18
  %11 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom53 = sext i32 %11 to i64
  %arrayidx58 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom53, i64 0, i64 %indvars.iv
  %12 = load i32, ptr %arrayidx58, align 4, !tbaa !18
  %add59 = add nsw i32 %12, 1
  %shr60 = ashr i32 %add59, 1
  store i32 %shr60, ptr %arrayidx58, align 4, !tbaa !18
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond.not = icmp eq i64 %indvars.iv.next, 64
  br i1 %exitcond.not, label %for.body35.1, label %for.body35, !llvm.loop !57

for.body35.1:                                     ; preds = %for.body35, %for.body35.1
  %indvars.iv.1 = phi i64 [ %indvars.iv.next.1, %for.body35.1 ], [ 0, %for.body35 ]
  %13 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom37.1 = sext i32 %13 to i64
  %arrayidx42.1 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom37.1, i64 1, i64 %indvars.iv.1
  %14 = load i32, ptr %arrayidx42.1, align 4, !tbaa !18
  %add43.1 = add nsw i32 %14, 1
  %shr44.1 = ashr i32 %add43.1, 1
  store i32 %shr44.1, ptr %arrayidx42.1, align 4, !tbaa !18
  %15 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom53.1 = sext i32 %15 to i64
  %arrayidx58.1 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom53.1, i64 1, i64 %indvars.iv.1
  %16 = load i32, ptr %arrayidx58.1, align 4, !tbaa !18
  %add59.1 = add nsw i32 %16, 1
  %shr60.1 = ashr i32 %add59.1, 1
  store i32 %shr60.1, ptr %arrayidx58.1, align 4, !tbaa !18
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %exitcond.1.not = icmp eq i64 %indvars.iv.next.1, 64
  br i1 %exitcond.1.not, label %for.body35.2, label %for.body35.1, !llvm.loop !57

for.body35.2:                                     ; preds = %for.body35.1, %for.body35.2
  %indvars.iv.2 = phi i64 [ %indvars.iv.next.2, %for.body35.2 ], [ 0, %for.body35.1 ]
  %17 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom37.2 = sext i32 %17 to i64
  %arrayidx42.2 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom37.2, i64 2, i64 %indvars.iv.2
  %18 = load i32, ptr %arrayidx42.2, align 4, !tbaa !18
  %add43.2 = add nsw i32 %18, 1
  %shr44.2 = ashr i32 %add43.2, 1
  store i32 %shr44.2, ptr %arrayidx42.2, align 4, !tbaa !18
  %19 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom53.2 = sext i32 %19 to i64
  %arrayidx58.2 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom53.2, i64 2, i64 %indvars.iv.2
  %20 = load i32, ptr %arrayidx58.2, align 4, !tbaa !18
  %add59.2 = add nsw i32 %20, 1
  %shr60.2 = ashr i32 %add59.2, 1
  store i32 %shr60.2, ptr %arrayidx58.2, align 4, !tbaa !18
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv.2, 1
  %exitcond.2.not = icmp eq i64 %indvars.iv.next.2, 64
  br i1 %exitcond.2.not, label %for.body35.3, label %for.body35.2, !llvm.loop !57

for.body35.3:                                     ; preds = %for.body35.2, %for.body35.3
  %indvars.iv.3 = phi i64 [ %indvars.iv.next.3, %for.body35.3 ], [ 0, %for.body35.2 ]
  %21 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom37.3 = sext i32 %21 to i64
  %arrayidx42.3 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom37.3, i64 3, i64 %indvars.iv.3
  %22 = load i32, ptr %arrayidx42.3, align 4, !tbaa !18
  %add43.3 = add nsw i32 %22, 1
  %shr44.3 = ashr i32 %add43.3, 1
  store i32 %shr44.3, ptr %arrayidx42.3, align 4, !tbaa !18
  %23 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom53.3 = sext i32 %23 to i64
  %arrayidx58.3 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom53.3, i64 3, i64 %indvars.iv.3
  %24 = load i32, ptr %arrayidx58.3, align 4, !tbaa !18
  %add59.3 = add nsw i32 %24, 1
  %shr60.3 = ashr i32 %add59.3, 1
  store i32 %shr60.3, ptr %arrayidx58.3, align 4, !tbaa !18
  %indvars.iv.next.3 = add nuw nsw i64 %indvars.iv.3, 1
  %exitcond.3.not = icmp eq i64 %indvars.iv.next.3, 64
  br i1 %exitcond.3.not, label %for.body35.4, label %for.body35.3, !llvm.loop !57

for.body35.4:                                     ; preds = %for.body35.3, %for.body35.4
  %indvars.iv.4 = phi i64 [ %indvars.iv.next.4, %for.body35.4 ], [ 0, %for.body35.3 ]
  %25 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom37.4 = sext i32 %25 to i64
  %arrayidx42.4 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom37.4, i64 4, i64 %indvars.iv.4
  %26 = load i32, ptr %arrayidx42.4, align 4, !tbaa !18
  %add43.4 = add nsw i32 %26, 1
  %shr44.4 = ashr i32 %add43.4, 1
  store i32 %shr44.4, ptr %arrayidx42.4, align 4, !tbaa !18
  %27 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom53.4 = sext i32 %27 to i64
  %arrayidx58.4 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom53.4, i64 4, i64 %indvars.iv.4
  %28 = load i32, ptr %arrayidx58.4, align 4, !tbaa !18
  %add59.4 = add nsw i32 %28, 1
  %shr60.4 = ashr i32 %add59.4, 1
  store i32 %shr60.4, ptr %arrayidx58.4, align 4, !tbaa !18
  %indvars.iv.next.4 = add nuw nsw i64 %indvars.iv.4, 1
  %exitcond.4.not = icmp eq i64 %indvars.iv.next.4, 64
  br i1 %exitcond.4.not, label %for.body35.5, label %for.body35.4, !llvm.loop !57

for.body35.5:                                     ; preds = %for.body35.4, %for.body35.5
  %indvars.iv.5 = phi i64 [ %indvars.iv.next.5, %for.body35.5 ], [ 0, %for.body35.4 ]
  %29 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom37.5 = sext i32 %29 to i64
  %arrayidx42.5 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom37.5, i64 5, i64 %indvars.iv.5
  %30 = load i32, ptr %arrayidx42.5, align 4, !tbaa !18
  %add43.5 = add nsw i32 %30, 1
  %shr44.5 = ashr i32 %add43.5, 1
  store i32 %shr44.5, ptr %arrayidx42.5, align 4, !tbaa !18
  %31 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom53.5 = sext i32 %31 to i64
  %arrayidx58.5 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom53.5, i64 5, i64 %indvars.iv.5
  %32 = load i32, ptr %arrayidx58.5, align 4, !tbaa !18
  %add59.5 = add nsw i32 %32, 1
  %shr60.5 = ashr i32 %add59.5, 1
  store i32 %shr60.5, ptr %arrayidx58.5, align 4, !tbaa !18
  %indvars.iv.next.5 = add nuw nsw i64 %indvars.iv.5, 1
  %exitcond.5.not = icmp eq i64 %indvars.iv.next.5, 64
  br i1 %exitcond.5.not, label %for.body35.6, label %for.body35.5, !llvm.loop !57

for.body35.6:                                     ; preds = %for.body35.5, %for.body35.6
  %indvars.iv.6 = phi i64 [ %indvars.iv.next.6, %for.body35.6 ], [ 0, %for.body35.5 ]
  %33 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom37.6 = sext i32 %33 to i64
  %arrayidx42.6 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom37.6, i64 6, i64 %indvars.iv.6
  %34 = load i32, ptr %arrayidx42.6, align 4, !tbaa !18
  %add43.6 = add nsw i32 %34, 1
  %shr44.6 = ashr i32 %add43.6, 1
  store i32 %shr44.6, ptr %arrayidx42.6, align 4, !tbaa !18
  %35 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom53.6 = sext i32 %35 to i64
  %arrayidx58.6 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom53.6, i64 6, i64 %indvars.iv.6
  %36 = load i32, ptr %arrayidx58.6, align 4, !tbaa !18
  %add59.6 = add nsw i32 %36, 1
  %shr60.6 = ashr i32 %add59.6, 1
  store i32 %shr60.6, ptr %arrayidx58.6, align 4, !tbaa !18
  %indvars.iv.next.6 = add nuw nsw i64 %indvars.iv.6, 1
  %exitcond.6.not = icmp eq i64 %indvars.iv.next.6, 64
  br i1 %exitcond.6.not, label %for.body35.7, label %for.body35.6, !llvm.loop !57

for.body35.7:                                     ; preds = %for.body35.6, %for.body35.7
  %indvars.iv.7 = phi i64 [ %indvars.iv.next.7, %for.body35.7 ], [ 0, %for.body35.6 ]
  %37 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom37.7 = sext i32 %37 to i64
  %arrayidx42.7 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom37.7, i64 7, i64 %indvars.iv.7
  %38 = load i32, ptr %arrayidx42.7, align 4, !tbaa !18
  %add43.7 = add nsw i32 %38, 1
  %shr44.7 = ashr i32 %add43.7, 1
  store i32 %shr44.7, ptr %arrayidx42.7, align 4, !tbaa !18
  %39 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom53.7 = sext i32 %39 to i64
  %arrayidx58.7 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom53.7, i64 7, i64 %indvars.iv.7
  %40 = load i32, ptr %arrayidx58.7, align 4, !tbaa !18
  %add59.7 = add nsw i32 %40, 1
  %shr60.7 = ashr i32 %add59.7, 1
  store i32 %shr60.7, ptr %arrayidx58.7, align 4, !tbaa !18
  %indvars.iv.next.7 = add nuw nsw i64 %indvars.iv.7, 1
  %exitcond.7.not = icmp eq i64 %indvars.iv.next.7, 64
  br i1 %exitcond.7.not, label %for.body35.8, label %for.body35.7, !llvm.loop !57

for.body35.8:                                     ; preds = %for.body35.7, %for.body35.8
  %indvars.iv.8 = phi i64 [ %indvars.iv.next.8, %for.body35.8 ], [ 0, %for.body35.7 ]
  %41 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom37.8 = sext i32 %41 to i64
  %arrayidx42.8 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom37.8, i64 8, i64 %indvars.iv.8
  %42 = load i32, ptr %arrayidx42.8, align 4, !tbaa !18
  %add43.8 = add nsw i32 %42, 1
  %shr44.8 = ashr i32 %add43.8, 1
  store i32 %shr44.8, ptr %arrayidx42.8, align 4, !tbaa !18
  %43 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom53.8 = sext i32 %43 to i64
  %arrayidx58.8 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom53.8, i64 8, i64 %indvars.iv.8
  %44 = load i32, ptr %arrayidx58.8, align 4, !tbaa !18
  %add59.8 = add nsw i32 %44, 1
  %shr60.8 = ashr i32 %add59.8, 1
  store i32 %shr60.8, ptr %arrayidx58.8, align 4, !tbaa !18
  %indvars.iv.next.8 = add nuw nsw i64 %indvars.iv.8, 1
  %exitcond.8.not = icmp eq i64 %indvars.iv.next.8, 64
  br i1 %exitcond.8.not, label %for.body35.9, label %for.body35.8, !llvm.loop !57

for.body35.9:                                     ; preds = %for.body35.8, %for.body35.9
  %indvars.iv.9 = phi i64 [ %indvars.iv.next.9, %for.body35.9 ], [ 0, %for.body35.8 ]
  %45 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom37.9 = sext i32 %45 to i64
  %arrayidx42.9 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom37.9, i64 9, i64 %indvars.iv.9
  %46 = load i32, ptr %arrayidx42.9, align 4, !tbaa !18
  %add43.9 = add nsw i32 %46, 1
  %shr44.9 = ashr i32 %add43.9, 1
  store i32 %shr44.9, ptr %arrayidx42.9, align 4, !tbaa !18
  %47 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom53.9 = sext i32 %47 to i64
  %arrayidx58.9 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom53.9, i64 9, i64 %indvars.iv.9
  %48 = load i32, ptr %arrayidx58.9, align 4, !tbaa !18
  %add59.9 = add nsw i32 %48, 1
  %shr60.9 = ashr i32 %add59.9, 1
  store i32 %shr60.9, ptr %arrayidx58.9, align 4, !tbaa !18
  %indvars.iv.next.9 = add nuw nsw i64 %indvars.iv.9, 1
  %exitcond.9.not = icmp eq i64 %indvars.iv.next.9, 64
  br i1 %exitcond.9.not, label %for.body35.10, label %for.body35.9, !llvm.loop !57

for.body35.10:                                    ; preds = %for.body35.9, %for.body35.10
  %indvars.iv.10 = phi i64 [ %indvars.iv.next.10, %for.body35.10 ], [ 0, %for.body35.9 ]
  %49 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom37.10 = sext i32 %49 to i64
  %arrayidx42.10 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom37.10, i64 10, i64 %indvars.iv.10
  %50 = load i32, ptr %arrayidx42.10, align 4, !tbaa !18
  %add43.10 = add nsw i32 %50, 1
  %shr44.10 = ashr i32 %add43.10, 1
  store i32 %shr44.10, ptr %arrayidx42.10, align 4, !tbaa !18
  %51 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom53.10 = sext i32 %51 to i64
  %arrayidx58.10 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom53.10, i64 10, i64 %indvars.iv.10
  %52 = load i32, ptr %arrayidx58.10, align 4, !tbaa !18
  %add59.10 = add nsw i32 %52, 1
  %shr60.10 = ashr i32 %add59.10, 1
  store i32 %shr60.10, ptr %arrayidx58.10, align 4, !tbaa !18
  %indvars.iv.next.10 = add nuw nsw i64 %indvars.iv.10, 1
  %exitcond.10.not = icmp eq i64 %indvars.iv.next.10, 64
  br i1 %exitcond.10.not, label %for.body35.11, label %for.body35.10, !llvm.loop !57

for.body35.11:                                    ; preds = %for.body35.10, %for.body35.11
  %indvars.iv.11 = phi i64 [ %indvars.iv.next.11, %for.body35.11 ], [ 0, %for.body35.10 ]
  %53 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom37.11 = sext i32 %53 to i64
  %arrayidx42.11 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom37.11, i64 11, i64 %indvars.iv.11
  %54 = load i32, ptr %arrayidx42.11, align 4, !tbaa !18
  %add43.11 = add nsw i32 %54, 1
  %shr44.11 = ashr i32 %add43.11, 1
  store i32 %shr44.11, ptr %arrayidx42.11, align 4, !tbaa !18
  %55 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom53.11 = sext i32 %55 to i64
  %arrayidx58.11 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom53.11, i64 11, i64 %indvars.iv.11
  %56 = load i32, ptr %arrayidx58.11, align 4, !tbaa !18
  %add59.11 = add nsw i32 %56, 1
  %shr60.11 = ashr i32 %add59.11, 1
  store i32 %shr60.11, ptr %arrayidx58.11, align 4, !tbaa !18
  %indvars.iv.next.11 = add nuw nsw i64 %indvars.iv.11, 1
  %exitcond.11.not = icmp eq i64 %indvars.iv.next.11, 64
  br i1 %exitcond.11.not, label %for.inc68.11, label %for.body35.11, !llvm.loop !57

for.inc68.11:                                     ; preds = %for.body35.11
  %.pre = load i32, ptr %s, align 8, !tbaa !25
  %.pre240 = sext i32 %.pre to i64
  br label %if.end

if.end:                                           ; preds = %for.inc68.11, %if.then
  %idxprom74.pre-phi = phi i64 [ %.pre240, %for.inc68.11 ], [ %idxprom24, %if.then ]
  %arrayidx79 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom74.pre-phi, i64 %idxprom8, i64 %idxprom10
  %57 = load i32, ptr %arrayidx79, align 4, !tbaa !18
  %add80 = add nsw i32 %57, %div
  store i32 %add80, ptr %arrayidx79, align 4, !tbaa !18
  %58 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom82 = sext i32 %58 to i64
  %arrayidx87 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom82, i64 %idxprom8, i64 %idxprom10
  %59 = load i32, ptr %arrayidx87, align 4, !tbaa !18
  %cmp88 = icmp sgt i32 %59, 500000
  br i1 %cmp88, label %for.body95, label %if.end118

for.body95:                                       ; preds = %if.end, %for.body95
  %indvars.iv232 = phi i64 [ %indvars.iv.next233.1246, %for.body95 ], [ 0, %if.end ]
  %60 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97 = sext i32 %60 to i64
  %arrayidx102 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97, i64 0, i64 %indvars.iv232
  %61 = load i32, ptr %arrayidx102, align 8, !tbaa !18
  %add103 = add nsw i32 %61, 1
  %shr104 = ashr i32 %add103, 1
  store i32 %shr104, ptr %arrayidx102, align 8, !tbaa !18
  %indvars.iv.next233 = or i64 %indvars.iv232, 1
  %62 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.1242 = sext i32 %62 to i64
  %arrayidx102.1243 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.1242, i64 0, i64 %indvars.iv.next233
  %63 = load i32, ptr %arrayidx102.1243, align 4, !tbaa !18
  %add103.1244 = add nsw i32 %63, 1
  %shr104.1245 = ashr i32 %add103.1244, 1
  store i32 %shr104.1245, ptr %arrayidx102.1243, align 4, !tbaa !18
  %indvars.iv.next233.1246 = add nuw nsw i64 %indvars.iv232, 2
  %exitcond235.not.1 = icmp eq i64 %indvars.iv.next233.1246, 64
  br i1 %exitcond235.not.1, label %for.body95.1, label %for.body95, !llvm.loop !58

for.body95.1:                                     ; preds = %for.body95, %for.body95.1
  %indvars.iv232.1 = phi i64 [ %indvars.iv.next233.1.1, %for.body95.1 ], [ 0, %for.body95 ]
  %64 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.1 = sext i32 %64 to i64
  %arrayidx102.1 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.1, i64 1, i64 %indvars.iv232.1
  %65 = load i32, ptr %arrayidx102.1, align 8, !tbaa !18
  %add103.1 = add nsw i32 %65, 1
  %shr104.1 = ashr i32 %add103.1, 1
  store i32 %shr104.1, ptr %arrayidx102.1, align 8, !tbaa !18
  %indvars.iv.next233.1 = or i64 %indvars.iv232.1, 1
  %66 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.1.1 = sext i32 %66 to i64
  %arrayidx102.1.1 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.1.1, i64 1, i64 %indvars.iv.next233.1
  %67 = load i32, ptr %arrayidx102.1.1, align 4, !tbaa !18
  %add103.1.1 = add nsw i32 %67, 1
  %shr104.1.1 = ashr i32 %add103.1.1, 1
  store i32 %shr104.1.1, ptr %arrayidx102.1.1, align 4, !tbaa !18
  %indvars.iv.next233.1.1 = add nuw nsw i64 %indvars.iv232.1, 2
  %exitcond235.1.not.1 = icmp eq i64 %indvars.iv.next233.1.1, 64
  br i1 %exitcond235.1.not.1, label %for.body95.2, label %for.body95.1, !llvm.loop !58

for.body95.2:                                     ; preds = %for.body95.1, %for.body95.2
  %indvars.iv232.2 = phi i64 [ %indvars.iv.next233.2.1, %for.body95.2 ], [ 0, %for.body95.1 ]
  %68 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.2 = sext i32 %68 to i64
  %arrayidx102.2 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.2, i64 2, i64 %indvars.iv232.2
  %69 = load i32, ptr %arrayidx102.2, align 8, !tbaa !18
  %add103.2 = add nsw i32 %69, 1
  %shr104.2 = ashr i32 %add103.2, 1
  store i32 %shr104.2, ptr %arrayidx102.2, align 8, !tbaa !18
  %indvars.iv.next233.2 = or i64 %indvars.iv232.2, 1
  %70 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.2.1 = sext i32 %70 to i64
  %arrayidx102.2.1 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.2.1, i64 2, i64 %indvars.iv.next233.2
  %71 = load i32, ptr %arrayidx102.2.1, align 4, !tbaa !18
  %add103.2.1 = add nsw i32 %71, 1
  %shr104.2.1 = ashr i32 %add103.2.1, 1
  store i32 %shr104.2.1, ptr %arrayidx102.2.1, align 4, !tbaa !18
  %indvars.iv.next233.2.1 = add nuw nsw i64 %indvars.iv232.2, 2
  %exitcond235.2.not.1 = icmp eq i64 %indvars.iv.next233.2.1, 64
  br i1 %exitcond235.2.not.1, label %for.body95.3, label %for.body95.2, !llvm.loop !58

for.body95.3:                                     ; preds = %for.body95.2, %for.body95.3
  %indvars.iv232.3 = phi i64 [ %indvars.iv.next233.3.1, %for.body95.3 ], [ 0, %for.body95.2 ]
  %72 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.3 = sext i32 %72 to i64
  %arrayidx102.3 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.3, i64 3, i64 %indvars.iv232.3
  %73 = load i32, ptr %arrayidx102.3, align 8, !tbaa !18
  %add103.3 = add nsw i32 %73, 1
  %shr104.3 = ashr i32 %add103.3, 1
  store i32 %shr104.3, ptr %arrayidx102.3, align 8, !tbaa !18
  %indvars.iv.next233.3 = or i64 %indvars.iv232.3, 1
  %74 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.3.1 = sext i32 %74 to i64
  %arrayidx102.3.1 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.3.1, i64 3, i64 %indvars.iv.next233.3
  %75 = load i32, ptr %arrayidx102.3.1, align 4, !tbaa !18
  %add103.3.1 = add nsw i32 %75, 1
  %shr104.3.1 = ashr i32 %add103.3.1, 1
  store i32 %shr104.3.1, ptr %arrayidx102.3.1, align 4, !tbaa !18
  %indvars.iv.next233.3.1 = add nuw nsw i64 %indvars.iv232.3, 2
  %exitcond235.3.not.1 = icmp eq i64 %indvars.iv.next233.3.1, 64
  br i1 %exitcond235.3.not.1, label %for.body95.4, label %for.body95.3, !llvm.loop !58

for.body95.4:                                     ; preds = %for.body95.3, %for.body95.4
  %indvars.iv232.4 = phi i64 [ %indvars.iv.next233.4.1, %for.body95.4 ], [ 0, %for.body95.3 ]
  %76 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.4 = sext i32 %76 to i64
  %arrayidx102.4 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.4, i64 4, i64 %indvars.iv232.4
  %77 = load i32, ptr %arrayidx102.4, align 8, !tbaa !18
  %add103.4 = add nsw i32 %77, 1
  %shr104.4 = ashr i32 %add103.4, 1
  store i32 %shr104.4, ptr %arrayidx102.4, align 8, !tbaa !18
  %indvars.iv.next233.4 = or i64 %indvars.iv232.4, 1
  %78 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.4.1 = sext i32 %78 to i64
  %arrayidx102.4.1 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.4.1, i64 4, i64 %indvars.iv.next233.4
  %79 = load i32, ptr %arrayidx102.4.1, align 4, !tbaa !18
  %add103.4.1 = add nsw i32 %79, 1
  %shr104.4.1 = ashr i32 %add103.4.1, 1
  store i32 %shr104.4.1, ptr %arrayidx102.4.1, align 4, !tbaa !18
  %indvars.iv.next233.4.1 = add nuw nsw i64 %indvars.iv232.4, 2
  %exitcond235.4.not.1 = icmp eq i64 %indvars.iv.next233.4.1, 64
  br i1 %exitcond235.4.not.1, label %for.body95.5, label %for.body95.4, !llvm.loop !58

for.body95.5:                                     ; preds = %for.body95.4, %for.body95.5
  %indvars.iv232.5 = phi i64 [ %indvars.iv.next233.5.1, %for.body95.5 ], [ 0, %for.body95.4 ]
  %80 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.5 = sext i32 %80 to i64
  %arrayidx102.5 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.5, i64 5, i64 %indvars.iv232.5
  %81 = load i32, ptr %arrayidx102.5, align 8, !tbaa !18
  %add103.5 = add nsw i32 %81, 1
  %shr104.5 = ashr i32 %add103.5, 1
  store i32 %shr104.5, ptr %arrayidx102.5, align 8, !tbaa !18
  %indvars.iv.next233.5 = or i64 %indvars.iv232.5, 1
  %82 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.5.1 = sext i32 %82 to i64
  %arrayidx102.5.1 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.5.1, i64 5, i64 %indvars.iv.next233.5
  %83 = load i32, ptr %arrayidx102.5.1, align 4, !tbaa !18
  %add103.5.1 = add nsw i32 %83, 1
  %shr104.5.1 = ashr i32 %add103.5.1, 1
  store i32 %shr104.5.1, ptr %arrayidx102.5.1, align 4, !tbaa !18
  %indvars.iv.next233.5.1 = add nuw nsw i64 %indvars.iv232.5, 2
  %exitcond235.5.not.1 = icmp eq i64 %indvars.iv.next233.5.1, 64
  br i1 %exitcond235.5.not.1, label %for.body95.6, label %for.body95.5, !llvm.loop !58

for.body95.6:                                     ; preds = %for.body95.5, %for.body95.6
  %indvars.iv232.6 = phi i64 [ %indvars.iv.next233.6.1, %for.body95.6 ], [ 0, %for.body95.5 ]
  %84 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.6 = sext i32 %84 to i64
  %arrayidx102.6 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.6, i64 6, i64 %indvars.iv232.6
  %85 = load i32, ptr %arrayidx102.6, align 8, !tbaa !18
  %add103.6 = add nsw i32 %85, 1
  %shr104.6 = ashr i32 %add103.6, 1
  store i32 %shr104.6, ptr %arrayidx102.6, align 8, !tbaa !18
  %indvars.iv.next233.6 = or i64 %indvars.iv232.6, 1
  %86 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.6.1 = sext i32 %86 to i64
  %arrayidx102.6.1 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.6.1, i64 6, i64 %indvars.iv.next233.6
  %87 = load i32, ptr %arrayidx102.6.1, align 4, !tbaa !18
  %add103.6.1 = add nsw i32 %87, 1
  %shr104.6.1 = ashr i32 %add103.6.1, 1
  store i32 %shr104.6.1, ptr %arrayidx102.6.1, align 4, !tbaa !18
  %indvars.iv.next233.6.1 = add nuw nsw i64 %indvars.iv232.6, 2
  %exitcond235.6.not.1 = icmp eq i64 %indvars.iv.next233.6.1, 64
  br i1 %exitcond235.6.not.1, label %for.body95.7, label %for.body95.6, !llvm.loop !58

for.body95.7:                                     ; preds = %for.body95.6, %for.body95.7
  %indvars.iv232.7 = phi i64 [ %indvars.iv.next233.7.1, %for.body95.7 ], [ 0, %for.body95.6 ]
  %88 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.7 = sext i32 %88 to i64
  %arrayidx102.7 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.7, i64 7, i64 %indvars.iv232.7
  %89 = load i32, ptr %arrayidx102.7, align 8, !tbaa !18
  %add103.7 = add nsw i32 %89, 1
  %shr104.7 = ashr i32 %add103.7, 1
  store i32 %shr104.7, ptr %arrayidx102.7, align 8, !tbaa !18
  %indvars.iv.next233.7 = or i64 %indvars.iv232.7, 1
  %90 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.7.1 = sext i32 %90 to i64
  %arrayidx102.7.1 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.7.1, i64 7, i64 %indvars.iv.next233.7
  %91 = load i32, ptr %arrayidx102.7.1, align 4, !tbaa !18
  %add103.7.1 = add nsw i32 %91, 1
  %shr104.7.1 = ashr i32 %add103.7.1, 1
  store i32 %shr104.7.1, ptr %arrayidx102.7.1, align 4, !tbaa !18
  %indvars.iv.next233.7.1 = add nuw nsw i64 %indvars.iv232.7, 2
  %exitcond235.7.not.1 = icmp eq i64 %indvars.iv.next233.7.1, 64
  br i1 %exitcond235.7.not.1, label %for.body95.8, label %for.body95.7, !llvm.loop !58

for.body95.8:                                     ; preds = %for.body95.7, %for.body95.8
  %indvars.iv232.8 = phi i64 [ %indvars.iv.next233.8.1, %for.body95.8 ], [ 0, %for.body95.7 ]
  %92 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.8 = sext i32 %92 to i64
  %arrayidx102.8 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.8, i64 8, i64 %indvars.iv232.8
  %93 = load i32, ptr %arrayidx102.8, align 8, !tbaa !18
  %add103.8 = add nsw i32 %93, 1
  %shr104.8 = ashr i32 %add103.8, 1
  store i32 %shr104.8, ptr %arrayidx102.8, align 8, !tbaa !18
  %indvars.iv.next233.8 = or i64 %indvars.iv232.8, 1
  %94 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.8.1 = sext i32 %94 to i64
  %arrayidx102.8.1 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.8.1, i64 8, i64 %indvars.iv.next233.8
  %95 = load i32, ptr %arrayidx102.8.1, align 4, !tbaa !18
  %add103.8.1 = add nsw i32 %95, 1
  %shr104.8.1 = ashr i32 %add103.8.1, 1
  store i32 %shr104.8.1, ptr %arrayidx102.8.1, align 4, !tbaa !18
  %indvars.iv.next233.8.1 = add nuw nsw i64 %indvars.iv232.8, 2
  %exitcond235.8.not.1 = icmp eq i64 %indvars.iv.next233.8.1, 64
  br i1 %exitcond235.8.not.1, label %for.body95.9, label %for.body95.8, !llvm.loop !58

for.body95.9:                                     ; preds = %for.body95.8, %for.body95.9
  %indvars.iv232.9 = phi i64 [ %indvars.iv.next233.9.1, %for.body95.9 ], [ 0, %for.body95.8 ]
  %96 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.9 = sext i32 %96 to i64
  %arrayidx102.9 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.9, i64 9, i64 %indvars.iv232.9
  %97 = load i32, ptr %arrayidx102.9, align 8, !tbaa !18
  %add103.9 = add nsw i32 %97, 1
  %shr104.9 = ashr i32 %add103.9, 1
  store i32 %shr104.9, ptr %arrayidx102.9, align 8, !tbaa !18
  %indvars.iv.next233.9 = or i64 %indvars.iv232.9, 1
  %98 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.9.1 = sext i32 %98 to i64
  %arrayidx102.9.1 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.9.1, i64 9, i64 %indvars.iv.next233.9
  %99 = load i32, ptr %arrayidx102.9.1, align 4, !tbaa !18
  %add103.9.1 = add nsw i32 %99, 1
  %shr104.9.1 = ashr i32 %add103.9.1, 1
  store i32 %shr104.9.1, ptr %arrayidx102.9.1, align 4, !tbaa !18
  %indvars.iv.next233.9.1 = add nuw nsw i64 %indvars.iv232.9, 2
  %exitcond235.9.not.1 = icmp eq i64 %indvars.iv.next233.9.1, 64
  br i1 %exitcond235.9.not.1, label %for.body95.10, label %for.body95.9, !llvm.loop !58

for.body95.10:                                    ; preds = %for.body95.9, %for.body95.10
  %indvars.iv232.10 = phi i64 [ %indvars.iv.next233.10.1, %for.body95.10 ], [ 0, %for.body95.9 ]
  %100 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.10 = sext i32 %100 to i64
  %arrayidx102.10 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.10, i64 10, i64 %indvars.iv232.10
  %101 = load i32, ptr %arrayidx102.10, align 8, !tbaa !18
  %add103.10 = add nsw i32 %101, 1
  %shr104.10 = ashr i32 %add103.10, 1
  store i32 %shr104.10, ptr %arrayidx102.10, align 8, !tbaa !18
  %indvars.iv.next233.10 = or i64 %indvars.iv232.10, 1
  %102 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.10.1 = sext i32 %102 to i64
  %arrayidx102.10.1 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.10.1, i64 10, i64 %indvars.iv.next233.10
  %103 = load i32, ptr %arrayidx102.10.1, align 4, !tbaa !18
  %add103.10.1 = add nsw i32 %103, 1
  %shr104.10.1 = ashr i32 %add103.10.1, 1
  store i32 %shr104.10.1, ptr %arrayidx102.10.1, align 4, !tbaa !18
  %indvars.iv.next233.10.1 = add nuw nsw i64 %indvars.iv232.10, 2
  %exitcond235.10.not.1 = icmp eq i64 %indvars.iv.next233.10.1, 64
  br i1 %exitcond235.10.not.1, label %for.body95.11, label %for.body95.10, !llvm.loop !58

for.body95.11:                                    ; preds = %for.body95.10, %for.body95.11
  %indvars.iv232.11 = phi i64 [ %indvars.iv.next233.11.1, %for.body95.11 ], [ 0, %for.body95.10 ]
  %104 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.11 = sext i32 %104 to i64
  %arrayidx102.11 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.11, i64 11, i64 %indvars.iv232.11
  %105 = load i32, ptr %arrayidx102.11, align 8, !tbaa !18
  %add103.11 = add nsw i32 %105, 1
  %shr104.11 = ashr i32 %add103.11, 1
  store i32 %shr104.11, ptr %arrayidx102.11, align 8, !tbaa !18
  %indvars.iv.next233.11 = or i64 %indvars.iv232.11, 1
  %106 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom97.11.1 = sext i32 %106 to i64
  %arrayidx102.11.1 = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_h, i64 0, i64 %idxprom97.11.1, i64 11, i64 %indvars.iv.next233.11
  %107 = load i32, ptr %arrayidx102.11.1, align 4, !tbaa !18
  %add103.11.1 = add nsw i32 %107, 1
  %shr104.11.1 = ashr i32 %add103.11.1, 1
  store i32 %shr104.11.1, ptr %arrayidx102.11.1, align 4, !tbaa !18
  %indvars.iv.next233.11.1 = add nuw nsw i64 %indvars.iv232.11, 2
  %exitcond235.11.not.1 = icmp eq i64 %indvars.iv.next233.11.1, 64
  br i1 %exitcond235.11.not.1, label %if.end118, label %for.body95.11, !llvm.loop !58

if.end118:                                        ; preds = %for.body95.11, %if.end
  %ply = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 14
  %108 = load i32, ptr %ply, align 8, !tbaa !10
  %idxprom119 = sext i32 %108 to i64
  %arrayidx120 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 21, i64 %idxprom119
  %109 = load i32, ptr %arrayidx120, align 8, !tbaa !35
  %cmp121.not = icmp eq i32 %109, %move
  br i1 %cmp121.not, label %if.end157, label %if.then122

if.then122:                                       ; preds = %if.end118
  %killer2 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 21, i64 %idxprom119, i32 1
  %110 = load i32, ptr %killer2, align 4, !tbaa !37
  %cmp127.not = icmp eq i32 %110, %move
  br i1 %cmp127.not, label %if.end145, label %if.then128

if.then128:                                       ; preds = %if.then122
  %killer3 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 21, i64 %idxprom119, i32 2
  %111 = load i32, ptr %killer3, align 8, !tbaa !38
  %cmp133.not = icmp eq i32 %111, %move
  br i1 %cmp133.not, label %if.end139, label %if.then134

if.then134:                                       ; preds = %if.then128
  %killer4 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 21, i64 %idxprom119, i32 3
  store i32 %111, ptr %killer4, align 4, !tbaa !39
  br label %if.end139

if.end139:                                        ; preds = %if.then134, %if.then128
  store i32 %110, ptr %killer3, align 8, !tbaa !38
  br label %if.end145

if.end145:                                        ; preds = %if.end139, %if.then122
  store i32 %109, ptr %killer2, align 4, !tbaa !37
  store i32 %move, ptr %arrayidx120, align 8, !tbaa !35
  br label %if.end157

if.end157:                                        ; preds = %if.end118, %if.end145, %entry
  ret void
}

; Function Attrs: mustprogress uwtable
define dso_local noundef i32 @_Z14rootmovesearchP7state_tiiiii(ptr noundef %s, i32 noundef %alpha, i32 noundef %beta, i32 noundef %depth, i32 noundef %is_null, i32 noundef %cutnode) local_unnamed_addr #0 {
entry:
  call void @mcount()
  %call = tail call noundef i32 @_Z6searchP7state_tiiiii(ptr noundef %s, i32 noundef %alpha, i32 noundef %beta, i32 noundef %depth, i32 noundef %is_null, i32 noundef %cutnode)
  %0 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %tobool.not = icmp eq i32 %0, 0
  %call. = select i1 %tobool.not, i32 %call, i32 0
  ret i32 %call.
}

; Function Attrs: mustprogress uwtable
define dso_local noundef i32 @_Z11search_rootP7state_tiii(ptr noundef %s, i32 noundef %originalalpha, i32 noundef %originalbeta, i32 noundef %depth) local_unnamed_addr #0 {
entry:
  call void @mcount()
  %moves = alloca [240 x i32], align 16
  %move_ordering = alloca [240 x i32], align 16
  %dummy = alloca i32, align 4
  %dummy2 = alloca i32, align 4
  %searching_move = alloca [512 x i8], align 16
  call void @llvm.lifetime.start.p0(i64 960, ptr nonnull %moves) #9
  call void @llvm.lifetime.start.p0(i64 960, ptr nonnull %move_ordering) #9
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %dummy) #9
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %dummy2) #9
  call void @llvm.lifetime.start.p0(i64 512, ptr nonnull %searching_move) #9
  %0 = load i32, ptr @_ZZ11search_rootP7state_tiiiE5bmove, align 4, !tbaa !18
  %ply = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 14
  store i32 1, ptr %ply, align 8, !tbaa !10
  store i32 -32000, ptr @gamestate, align 8, !tbaa !59
  %arrayidx = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 25, i64 1
  %1 = load i32, ptr %arrayidx, align 4, !tbaa !18
  %tobool = icmp ne i32 %1, 0
  %add = add nsw i32 %depth, 4
  %spec.select = select i1 %tobool, i32 %add, i32 %depth
  %call = call noundef i32 @_Z7ProbeTTP7state_tPiiiPjS1_S1_S1_S1_i(ptr noundef %s, ptr noundef nonnull %dummy, i32 noundef %originalalpha, i32 noundef %originalbeta, ptr noundef nonnull %dummy2, ptr noundef nonnull %dummy, ptr noundef nonnull %dummy, ptr noundef nonnull %dummy, ptr noundef nonnull %dummy, i32 noundef %spec.select)
  %call2 = call noundef i32 @_Z8in_checkP7state_t(ptr noundef %s)
  %tobool3.not = icmp eq i32 %call2, 0
  br i1 %tobool3.not, label %if.else, label %if.then4

if.then4:                                         ; preds = %entry
  %call5 = call noundef i32 @_Z8in_checkP7state_t(ptr noundef nonnull %s)
  %call6 = call noundef i32 @_Z12gen_evasionsP7state_tPii(ptr noundef nonnull %s, ptr noundef nonnull %moves, i32 noundef %call5)
  br label %if.end9

if.else:                                          ; preds = %entry
  %call8 = call noundef i32 @_Z3genP7state_tPi(ptr noundef nonnull %s, ptr noundef nonnull %moves)
  br label %if.end9

if.end9:                                          ; preds = %if.else, %if.then4
  %num_moves.0 = phi i32 [ %call6, %if.then4 ], [ %call8, %if.else ]
  %2 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 20), align 4, !tbaa !60
  call fastcc void @_ZL11order_movesP7state_tPiS1_ij(ptr noundef nonnull %s, ptr noundef nonnull %moves, ptr noundef nonnull %move_ordering, i32 noundef %num_moves.0, i32 noundef %2)
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr @multipv_scores, align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 8), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 16), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 24), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr @root_scores, align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 8), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 16), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 24), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 32), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 40), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 48), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 56), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 32), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 40), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 48), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 56), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 64), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 72), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 80), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 88), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 64), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 72), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 80), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 88), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 96), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 104), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 112), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 120), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 96), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 104), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 112), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 120), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 128), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 136), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 144), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 152), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 128), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 136), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 144), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 152), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 160), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 168), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 176), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 184), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 160), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 168), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 176), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 184), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 192), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 200), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 208), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 216), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 192), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 200), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 208), align 16, !tbaa !18
  store <8 x i32> <i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000, i32 -32000>, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 216), align 16, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 224), align 16, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 224), align 16, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 225), align 4, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 225), align 4, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 226), align 8, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 226), align 8, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 227), align 4, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 227), align 4, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 228), align 16, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 228), align 16, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 229), align 4, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 229), align 4, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 230), align 8, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 230), align 8, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 231), align 4, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 231), align 4, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 232), align 16, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 232), align 16, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 233), align 4, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 233), align 4, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 234), align 8, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 234), align 8, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 235), align 4, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 235), align 4, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 236), align 16, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 236), align 16, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 237), align 4, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 237), align 4, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 238), align 8, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 238), align 8, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @multipv_scores, i64 0, i64 239), align 4, !tbaa !18
  store i32 -32000, ptr getelementptr inbounds ([240 x i32], ptr @root_scores, i64 0, i64 239), align 4, !tbaa !18
  %call16 = call noundef i32 @_Z4evalP7state_tiii(ptr noundef %s, i32 noundef %originalalpha, i32 noundef %originalbeta, i32 noundef 1)
  %hash = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 16
  %nodes = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 22
  %sub59 = sub nsw i32 0, %originalbeta
  %add213 = add i32 %spec.select, -4
  %3 = sext i32 %num_moves.0 to i64
  br label %while.cond.outer

while.cond.outer:                                 ; preds = %while.cond.outer.backedge, %if.end9
  %indvars.iv627.ph = phi i64 [ -1, %if.end9 ], [ %indvars.iv.next628, %while.cond.outer.backedge ]
  %no_moves.0.ph = phi i32 [ 1, %if.end9 ], [ %no_moves.1, %while.cond.outer.backedge ]
  %cmp126 = phi i1 [ true, %if.end9 ], [ false, %while.cond.outer.backedge ]
  %tobool303.not = phi i1 [ false, %if.end9 ], [ true, %while.cond.outer.backedge ]
  %alpha.0.ph = phi i32 [ %originalalpha, %if.end9 ], [ %alpha.1, %while.cond.outer.backedge ]
  %root_score.0.ph = phi i32 [ -32000, %if.end9 ], [ %root_score.5, %while.cond.outer.backedge ]
  %mc.0.ph = phi i32 [ 0, %if.end9 ], [ %mc.1, %while.cond.outer.backedge ]
  %best_move.0.ph = phi i32 [ %0, %if.end9 ], [ %best_move.4, %while.cond.outer.backedge ]
  %sub60 = sub nsw i32 0, %alpha.0.ph
  %sub211 = xor i32 %alpha.0.ph, -1
  br label %while.cond

while.cond:                                       ; preds = %while.cond.outer, %if.end288
  %indvars.iv627 = phi i64 [ %indvars.iv.next628, %if.end288 ], [ %indvars.iv627.ph, %while.cond.outer ]
  %no_moves.0 = phi i32 [ %no_moves.1, %if.end288 ], [ %no_moves.0.ph, %while.cond.outer ]
  %root_score.0 = phi i32 [ %root_score.5, %if.end288 ], [ %root_score.0.ph, %while.cond.outer ]
  %mc.0 = phi i32 [ %mc.1, %if.end288 ], [ %mc.0.ph, %while.cond.outer ]
  %best_move.0 = phi i32 [ %best_move.3, %if.end288 ], [ %best_move.0.ph, %while.cond.outer ]
  %indvars.iv.next628 = add i64 %indvars.iv627, 1
  %cmp.i = icmp slt i64 %indvars.iv627, 9
  %cmp165.i.not = icmp slt i64 %indvars.iv.next628, %3
  br i1 %cmp.i, label %for.cond.preheader.i, label %_ZL15remove_one_fastPiS_S_i.exit

for.cond.preheader.i:                             ; preds = %while.cond
  br i1 %cmp165.i.not, label %for.body.i, label %while.end

for.body.i:                                       ; preds = %for.cond.preheader.i, %for.body.i
  %indvars.iv.i = phi i64 [ %indvars.iv.next.i, %for.body.i ], [ %indvars.iv.next628, %for.cond.preheader.i ]
  %tmp.068.i = phi i32 [ %spec.select60.i, %for.body.i ], [ undef, %for.cond.preheader.i ]
  %best.067.i = phi i32 [ %spec.select.i, %for.body.i ], [ -1000000, %for.cond.preheader.i ]
  %arrayidx.i = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.i
  %4 = load i32, ptr %arrayidx.i, align 4, !tbaa !18
  %cmp2.i = icmp sgt i32 %4, %best.067.i
  %spec.select.i = call i32 @llvm.smax.i32(i32 %4, i32 %best.067.i)
  %5 = trunc i64 %indvars.iv.i to i32
  %spec.select60.i = select i1 %cmp2.i, i32 %5, i32 %tmp.068.i
  %indvars.iv.next.i = add nsw i64 %indvars.iv.i, 1
  %lftr.wideiv = trunc i64 %indvars.iv.next.i to i32
  %exitcond626 = icmp eq i32 %num_moves.0, %lftr.wideiv
  br i1 %exitcond626, label %if.end12.i, label %for.body.i, !llvm.loop !20

if.end12.i:                                       ; preds = %for.body.i
  %cmp13.i = icmp sgt i32 %spec.select.i, -1000000
  br i1 %cmp13.i, label %_ZL15remove_one_fastPiS_S_i.exit.thread599, label %while.end

_ZL15remove_one_fastPiS_S_i.exit.thread599:       ; preds = %if.end12.i
  %6 = sext i32 %spec.select60.i to i64
  %arrayidx16.i = getelementptr inbounds i32, ptr %move_ordering, i64 %indvars.iv.next628
  %7 = load i32, ptr %arrayidx16.i, align 4, !tbaa !18
  %arrayidx18.i = getelementptr inbounds i32, ptr %move_ordering, i64 %6
  store i32 %7, ptr %arrayidx18.i, align 4, !tbaa !18
  store i32 %spec.select.i, ptr %arrayidx16.i, align 4, !tbaa !18
  %arrayidx22.i = getelementptr inbounds i32, ptr %moves, i64 %indvars.iv.next628
  %8 = load i32, ptr %arrayidx22.i, align 4, !tbaa !18
  %arrayidx24.i = getelementptr inbounds i32, ptr %moves, i64 %6
  %9 = load i32, ptr %arrayidx24.i, align 4, !tbaa !18
  store i32 %9, ptr %arrayidx22.i, align 4, !tbaa !18
  store i32 %8, ptr %arrayidx24.i, align 4, !tbaa !18
  br label %while.body

_ZL15remove_one_fastPiS_S_i.exit:                 ; preds = %while.cond
  br i1 %cmp165.i.not, label %while.body, label %while.end

while.body:                                       ; preds = %_ZL15remove_one_fastPiS_S_i.exit.thread599, %_ZL15remove_one_fastPiS_S_i.exit
  %arrayidx22 = getelementptr inbounds [240 x i32], ptr %moves, i64 0, i64 %indvars.iv.next628
  %10 = load i32, ptr %arrayidx22, align 4, !tbaa !18
  call void @_Z4makeP7state_ti(ptr noundef %s, i32 noundef %10)
  %11 = load i64, ptr %hash, align 8, !tbaa !26
  %12 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 15), align 4, !tbaa !27
  %13 = load i32, ptr %ply, align 8, !tbaa !10
  %add24 = add i32 %13, -1
  %sub = add i32 %add24, %12
  %idxprom25 = sext i32 %sub to i64
  %arrayidx26 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 36, i64 %idxprom25
  store i64 %11, ptr %arrayidx26, align 8, !tbaa !6
  %14 = load i32, ptr %arrayidx22, align 4, !tbaa !18
  %idxprom31 = sext i32 %add24 to i64
  %arrayidx32 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 19, i64 %idxprom31
  store i32 %14, ptr %arrayidx32, align 4, !tbaa !18
  %15 = load i64, ptr %nodes, align 8, !tbaa !42
  %call35 = call noundef i32 @_Z11check_legalP7state_ti(ptr noundef %s, i32 noundef %14)
  %tobool36.not = icmp eq i32 %call35, 0
  br i1 %tobool36.not, label %if.end271, label %if.then37

if.then37:                                        ; preds = %while.body
  %16 = load i32, ptr %arrayidx22, align 4, !tbaa !18
  call void @_Z6unmakeP7state_ti(ptr noundef nonnull %s, i32 noundef %16)
  %inc40 = add nsw i32 %mc.0, 1
  %17 = load i32, ptr @uci_mode, align 4, !tbaa !18
  %tobool42.not = icmp eq i32 %17, 0
  %18 = load i32, ptr %arrayidx22, align 4, !tbaa !18
  br i1 %tobool42.not, label %if.then43, label %if.else47

if.then43:                                        ; preds = %if.then37
  call void @_Z11comp_to_sanP7state_tiPc(ptr noundef nonnull %s, i32 noundef %18, ptr noundef nonnull %searching_move)
  br label %if.end51

if.else47:                                        ; preds = %if.then37
  call void @_Z13comp_to_coordP7state_tiPc(ptr noundef nonnull %s, i32 noundef %18, ptr noundef nonnull %searching_move)
  br label %if.end51

if.end51:                                         ; preds = %if.else47, %if.then43
  %19 = load i32, ptr %arrayidx22, align 4, !tbaa !18
  call void @_Z4makeP7state_ti(ptr noundef nonnull %s, i32 noundef %19)
  %call54 = call noundef i32 @_Z8in_checkP7state_t(ptr noundef nonnull %s)
  %20 = load i32, ptr %ply, align 8, !tbaa !10
  %idxprom57 = sext i32 %20 to i64
  %arrayidx58 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 25, i64 %idxprom57
  store i32 %call54, ptr %arrayidx58, align 4, !tbaa !18
  %call61 = call noundef i32 @_Z4evalP7state_tiii(ptr noundef nonnull %s, i32 noundef %sub59, i32 noundef %sub60, i32 noundef 1)
  %21 = load i32, ptr %ply, align 8, !tbaa !10
  %idxprom64 = sext i32 %21 to i64
  %arrayidx65 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 25, i64 %idxprom64
  %22 = load i32, ptr %arrayidx65, align 4, !tbaa !18
  %tobool66.not = icmp eq i32 %22, 0
  br i1 %tobool66.not, label %if.else69, label %if.end125

if.else69:                                        ; preds = %if.end51
  %23 = load i32, ptr %arrayidx22, align 4, !tbaa !18
  %and = and i32 %23, 63
  %idxprom72 = zext i32 %and to i64
  %arrayidx73 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 1, i64 %idxprom72
  %24 = load i32, ptr %arrayidx73, align 4, !tbaa !18
  %cmp74 = icmp eq i32 %24, 1
  br i1 %cmp74, label %land.lhs.true, label %lor.lhs.false

land.lhs.true:                                    ; preds = %if.else69
  %call78 = call noundef i32 @_Z4ranki(i32 noundef %and)
  %cmp79 = icmp sgt i32 %call78, 6
  br i1 %cmp79, label %if.end125, label %land.lhs.true.lor.lhs.false_crit_edge

land.lhs.true.lor.lhs.false_crit_edge:            ; preds = %land.lhs.true
  %.pre = load i32, ptr %arrayidx22, align 4, !tbaa !18
  %.pre632 = and i32 %.pre, 63
  %.pre633 = zext i32 %.pre632 to i64
  br label %lor.lhs.false

lor.lhs.false:                                    ; preds = %land.lhs.true.lor.lhs.false_crit_edge, %if.else69
  %idxprom84.pre-phi = phi i64 [ %.pre633, %land.lhs.true.lor.lhs.false_crit_edge ], [ %idxprom72, %if.else69 ]
  %and83.pre-phi = phi i32 [ %.pre632, %land.lhs.true.lor.lhs.false_crit_edge ], [ %and, %if.else69 ]
  %25 = phi i32 [ %.pre, %land.lhs.true.lor.lhs.false_crit_edge ], [ %23, %if.else69 ]
  %arrayidx85 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 1, i64 %idxprom84.pre-phi
  %26 = load i32, ptr %arrayidx85, align 4, !tbaa !18
  %cmp86 = icmp eq i32 %26, 2
  br i1 %cmp86, label %land.lhs.true87, label %lor.lhs.false93

land.lhs.true87:                                  ; preds = %lor.lhs.false
  %call91 = call noundef i32 @_Z4ranki(i32 noundef %and83.pre-phi)
  %cmp92 = icmp slt i32 %call91, 3
  br i1 %cmp92, label %if.end125, label %land.lhs.true87.lor.lhs.false93_crit_edge

land.lhs.true87.lor.lhs.false93_crit_edge:        ; preds = %land.lhs.true87
  %.pre630 = load i32, ptr %arrayidx22, align 4, !tbaa !18
  br label %lor.lhs.false93

lor.lhs.false93:                                  ; preds = %land.lhs.true87.lor.lhs.false93_crit_edge, %lor.lhs.false
  %27 = phi i32 [ %.pre630, %land.lhs.true87.lor.lhs.false93_crit_edge ], [ %25, %lor.lhs.false ]
  %28 = and i32 %27, 61440
  %tobool97.not = icmp eq i32 %28, 0
  br i1 %tobool97.not, label %if.end101, label %if.end125

if.end101:                                        ; preds = %lor.lhs.false93
  %cmp106 = icmp slt i32 %mc.0, 3
  %or.cond381.not = select i1 %tobool, i1 true, i1 %cmp106
  br i1 %or.cond381.not, label %if.end125, label %land.lhs.true107

land.lhs.true107:                                 ; preds = %if.end101
  %and.i = and i32 %27, 63
  %idxprom.i = zext i32 %and.i to i64
  %arrayidx.i559 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 1, i64 %idxprom.i
  %29 = load i32, ptr %arrayidx.i559, align 4, !tbaa !18
  %sub.i = add nsw i32 %29, -1
  %30 = load i32, ptr %s, align 8, !tbaa !25
  %idxprom2.i = sext i32 %30 to i64
  %idxprom4.i = sext i32 %sub.i to i64
  %arrayidx7.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_hit, i64 0, i64 %idxprom2.i, i64 %idxprom4.i, i64 %idxprom.i
  %31 = load i32, ptr %arrayidx7.i, align 4, !tbaa !18
  %add.i = shl i32 %31, 7
  %mul.i = add i32 %add.i, 128
  %arrayidx14.i = getelementptr inbounds [8 x [12 x [64 x i32]]], ptr @history_tot, i64 0, i64 %idxprom2.i, i64 %idxprom4.i, i64 %idxprom.i
  %32 = load i32, ptr %arrayidx14.i, align 4, !tbaa !18
  %add15.i = add nsw i32 %32, 1
  %div.i = sdiv i32 %mul.i, %add15.i
  %cmp111 = icmp slt i32 %div.i, 80
  br i1 %cmp111, label %land.lhs.true112, label %if.end125

land.lhs.true112:                                 ; preds = %land.lhs.true107
  %33 = and i32 %27, 7925760
  %or.cond555 = icmp eq i32 %33, 6815744
  %spec.select558 = select i1 %or.cond555, i32 -4, i32 0
  br label %if.end125

if.end125:                                        ; preds = %land.lhs.true, %land.lhs.true87, %lor.lhs.false93, %if.end51, %land.lhs.true112, %land.lhs.true107, %if.end101
  %extend.0607 = phi i32 [ 0, %land.lhs.true107 ], [ 0, %if.end101 ], [ 0, %land.lhs.true112 ], [ 4, %if.end51 ], [ 3, %lor.lhs.false93 ], [ 3, %land.lhs.true87 ], [ 3, %land.lhs.true ]
  %tobool222 = phi i1 [ false, %land.lhs.true107 ], [ false, %if.end101 ], [ %or.cond555, %land.lhs.true112 ], [ false, %if.end51 ], [ false, %lor.lhs.false93 ], [ false, %land.lhs.true87 ], [ false, %land.lhs.true ]
  %huber.0.neg = phi i32 [ 0, %land.lhs.true107 ], [ 0, %if.end101 ], [ %spec.select558, %land.lhs.true112 ], [ 0, %if.end51 ], [ 0, %lor.lhs.false93 ], [ 0, %land.lhs.true87 ], [ 0, %land.lhs.true ]
  %34 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4
  %cmp128 = icmp slt i32 %34, 2
  %or.cond382 = select i1 %cmp126, i1 true, i1 %cmp128
  %35 = load i32, ptr @uci_multipv, align 4
  %cmp130 = icmp sgt i32 %35, 1
  %or.cond383 = select i1 %or.cond382, i1 true, i1 %cmp130
  %sub137 = add i32 %add213, %extend.0607
  br i1 %or.cond383, label %if.then131, label %if.else209

if.then131:                                       ; preds = %if.end125
  %cmp132 = icmp eq i32 %35, 1
  %sub60. = select i1 %cmp132, i32 %sub60, i32 1000000
  %call.i560 = call noundef i32 @_Z6searchP7state_tiiiii(ptr noundef nonnull %s, i32 noundef %sub59, i32 noundef %sub60., i32 noundef %sub137, i32 noundef 0, i32 noundef 0)
  %.sink = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %tobool.not.i561 = icmp eq i32 %.sink, 0
  %call..i562 = select i1 %tobool.not.i561, i32 %call.i560, i32 0
  %root_score.1 = sub nsw i32 0, %call..i562
  %idxprom147 = sext i32 %inc40 to i64
  %arrayidx148 = getelementptr inbounds [240 x i32], ptr @multipv_scores, i64 0, i64 %idxprom147
  store i32 %root_score.1, ptr %arrayidx148, align 4, !tbaa !18
  %arrayidx150 = getelementptr inbounds [240 x i32], ptr @root_scores, i64 0, i64 %indvars.iv.next628
  store i32 %root_score.1, ptr %arrayidx150, align 4, !tbaa !18
  %tobool151.not = icmp eq i32 %.sink, 0
  br i1 %tobool151.not, label %if.then152, label %if.end257.thread

if.end257.thread:                                 ; preds = %if.then131
  %36 = load i32, ptr @gamestate, align 8, !tbaa !59
  br label %if.end262

if.then152:                                       ; preds = %if.then131
  %cmp153.not = icmp slt i32 %root_score.1, %originalbeta
  br i1 %cmp153.not, label %if.else159, label %if.then154

if.then154:                                       ; preds = %if.then152
  %37 = load i32, ptr %arrayidx22, align 4, !tbaa !18
  call void @_Z16post_fh_thinkingP7state_tiiPci(ptr noundef nonnull %s, i32 noundef %root_score.1, i32 noundef %37, ptr noundef nonnull %searching_move, i32 noundef %inc40)
  br label %if.end197thread-pre-split

if.else159:                                       ; preds = %if.then152
  %cmp160 = icmp sge i32 %alpha.0.ph, %root_score.1
  %or.cond384 = and i1 %cmp160, %cmp126
  br i1 %or.cond384, label %if.then163, label %if.else183

if.then163:                                       ; preds = %if.else159
  store i32 1, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 22), align 4, !tbaa !31
  %38 = load i32, ptr %arrayidx22, align 4, !tbaa !18
  call void @_Z16post_fl_thinkingP7state_tiiPci(ptr noundef nonnull %s, i32 noundef %root_score.1, i32 noundef %38, ptr noundef nonnull %searching_move, i32 noundef %inc40)
  %sub170 = add i32 %add213, %extend.0607
  %call.i563 = call noundef i32 @_Z6searchP7state_tiiiii(ptr noundef nonnull %s, i32 noundef %sub59, i32 noundef 1000000, i32 noundef %sub170, i32 noundef 0, i32 noundef 0)
  %39 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %tobool.not.i564 = icmp eq i32 %39, 0
  %call..i565 = select i1 %tobool.not.i564, i32 %call.i563, i32 0
  %sub172 = sub nsw i32 0, %call..i565
  %40 = load i32, ptr @uci_multipv, align 4
  %cmp175 = icmp eq i32 %40, 1
  %or.cond401 = select i1 %tobool.not.i564, i1 %cmp175, i1 false
  br i1 %or.cond401, label %if.then176, label %if.end197

if.then176:                                       ; preds = %if.then163
  %41 = load i32, ptr %arrayidx22, align 4, !tbaa !18
  call void @_Z13post_thinkingP7state_tiiPci(ptr noundef nonnull %s, i32 noundef %sub172, i32 noundef %41, ptr noundef nonnull %searching_move, i32 noundef %inc40)
  br label %if.end197thread-pre-split

if.else183:                                       ; preds = %if.else159
  %cmp184 = icmp slt i32 %alpha.0.ph, %root_score.1
  br i1 %cmp184, label %land.lhs.true185, label %if.end197thread-pre-split

land.lhs.true185:                                 ; preds = %if.else183
  %42 = load i32, ptr @uci_multipv, align 4
  %cmp188 = icmp eq i32 %42, 1
  br i1 %cmp188, label %if.then189, label %if.end197

if.then189:                                       ; preds = %land.lhs.true185
  %43 = load i32, ptr %arrayidx22, align 4, !tbaa !18
  call void @_Z13post_thinkingP7state_tiiPci(ptr noundef nonnull %s, i32 noundef %root_score.1, i32 noundef %43, ptr noundef nonnull %searching_move, i32 noundef %inc40)
  br label %if.end197thread-pre-split

if.end197thread-pre-split:                        ; preds = %if.then154, %if.else183, %if.then189, %if.then176
  %root_score.2.ph = phi i32 [ %root_score.1, %if.else183 ], [ %root_score.1, %if.then189 ], [ %sub172, %if.then176 ], [ %root_score.1, %if.then154 ]
  %.pr = load i32, ptr @uci_multipv, align 4, !tbaa !18
  br label %if.end197

if.end197:                                        ; preds = %if.end197thread-pre-split, %if.then163, %land.lhs.true185
  %44 = phi i32 [ %.pr, %if.end197thread-pre-split ], [ %40, %if.then163 ], [ %42, %land.lhs.true185 ]
  %root_score.2 = phi i32 [ %root_score.2.ph, %if.end197thread-pre-split ], [ %sub172, %if.then163 ], [ %root_score.1, %land.lhs.true185 ]
  %cmp198 = icmp sgt i32 %44, 1
  br i1 %cmp198, label %if.then199, label %if.end202

if.then199:                                       ; preds = %if.end197
  %45 = load i32, ptr %arrayidx22, align 4, !tbaa !18
  call void @_Z21post_multipv_thinkingP7state_tiii(ptr noundef nonnull %s, i32 noundef %root_score.2, i32 noundef %inc40, i32 noundef %45)
  br label %if.end202

if.end202:                                        ; preds = %if.then199, %if.end197
  %46 = load i32, ptr @gamestate, align 8, !tbaa !59
  %cmp203 = icmp sle i32 %root_score.2, %46
  %47 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8
  %tobool205 = icmp ne i32 %47, 0
  %or.cond385 = select i1 %cmp203, i1 true, i1 %tobool205
  br i1 %or.cond385, label %if.end257, label %if.then206

if.then206:                                       ; preds = %if.end202
  store i32 %root_score.2, ptr @gamestate, align 8, !tbaa !59
  br label %if.end257

if.else209:                                       ; preds = %if.end125
  %sub215 = add i32 %huber.0.neg, %sub137
  %call.i566 = call noundef i32 @_Z6searchP7state_tiiiii(ptr noundef nonnull %s, i32 noundef %sub211, i32 noundef %sub60, i32 noundef %sub215, i32 noundef 0, i32 noundef 1)
  %48 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %tobool.not.i567 = icmp eq i32 %48, 0
  %call..i568 = select i1 %tobool.not.i567, i32 %call.i566, i32 0
  %sub217 = sub nsw i32 0, %call..i568
  %cmp218 = icmp slt i32 %alpha.0.ph, %sub217
  br i1 %cmp218, label %land.lhs.true219, label %if.end247

land.lhs.true219:                                 ; preds = %if.else209
  %cmp220 = icmp slt i32 %sub217, %originalbeta
  %or.cond386 = or i1 %tobool222, %cmp220
  %or.cond387.not = select i1 %or.cond386, i1 %tobool.not.i567, i1 false
  br i1 %or.cond387.not, label %if.then225, label %if.end247

if.then225:                                       ; preds = %land.lhs.true219
  store i32 1, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 23), align 8, !tbaa !34
  %call.i569 = call noundef i32 @_Z6searchP7state_tiiiii(ptr noundef nonnull %s, i32 noundef %sub59, i32 noundef %sub60, i32 noundef %sub137, i32 noundef 0, i32 noundef 0)
  %49 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %tobool.not.i570 = icmp eq i32 %49, 0
  %call..i571 = select i1 %tobool.not.i570, i32 %call.i569, i32 0
  %sub231 = sub nsw i32 0, %call..i571
  store i32 0, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 23), align 8, !tbaa !34
  %cmp232 = icmp sge i32 %alpha.0.ph, %sub231
  %tobool234 = icmp ne i32 %49, 0
  %or.cond388 = select i1 %cmp232, i1 true, i1 %tobool234
  br i1 %or.cond388, label %if.end247, label %if.then235

if.then235:                                       ; preds = %if.then225
  store i32 %sub231, ptr @gamestate, align 8, !tbaa !59
  %50 = load i32, ptr %arrayidx22, align 4, !tbaa !18
  %call238 = call noundef zeroext i16 @_Z12compact_movei(i32 noundef %50)
  %conv = zext i16 %call238 to i32
  store i32 %conv, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 20), align 4, !tbaa !60
  %cmp239 = icmp slt i32 %sub231, %originalbeta
  br i1 %cmp239, label %if.then240, label %if.end247

if.then240:                                       ; preds = %if.then235
  %51 = load i32, ptr %arrayidx22, align 4, !tbaa !18
  call void @_Z13post_thinkingP7state_tiiPci(ptr noundef nonnull %s, i32 noundef %sub231, i32 noundef %51, ptr noundef nonnull %searching_move, i32 noundef %inc40)
  br label %if.end247

if.end247:                                        ; preds = %if.then225, %if.then240, %if.then235, %land.lhs.true219, %if.else209
  %root_score.3 = phi i32 [ %sub217, %land.lhs.true219 ], [ %sub231, %if.then225 ], [ %sub231, %if.then240 ], [ %sub231, %if.then235 ], [ %sub217, %if.else209 ]
  %best_move.1 = phi i32 [ %best_move.0, %land.lhs.true219 ], [ %best_move.0, %if.then225 ], [ %50, %if.then240 ], [ %50, %if.then235 ], [ %best_move.0, %if.else209 ]
  %cmp248 = icmp slt i32 %root_score.3, %originalbeta
  %52 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8
  %tobool250 = icmp ne i32 %52, 0
  %or.cond389 = select i1 %cmp248, i1 true, i1 %tobool250
  br i1 %or.cond389, label %if.end257, label %if.then251

if.then251:                                       ; preds = %if.end247
  %53 = load i32, ptr %arrayidx22, align 4, !tbaa !18
  call void @_Z16post_fh_thinkingP7state_tiiPci(ptr noundef nonnull %s, i32 noundef %root_score.3, i32 noundef %53, ptr noundef nonnull %searching_move, i32 noundef %inc40)
  %.pre631 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8
  br label %if.end257

if.end257:                                        ; preds = %if.end247, %if.then251, %if.then206, %if.end202
  %54 = phi i32 [ %47, %if.end202 ], [ 0, %if.then206 ], [ %52, %if.end247 ], [ %.pre631, %if.then251 ]
  %root_score.4 = phi i32 [ %root_score.2, %if.end202 ], [ %root_score.2, %if.then206 ], [ %root_score.3, %if.end247 ], [ %root_score.3, %if.then251 ]
  %best_move.2 = phi i32 [ %best_move.0, %if.end202 ], [ %best_move.0, %if.then206 ], [ %best_move.1, %if.end247 ], [ %best_move.1, %if.then251 ]
  %55 = load i32, ptr @gamestate, align 8, !tbaa !59
  %cmp258 = icmp sle i32 %root_score.4, %55
  %tobool260 = icmp ne i32 %54, 0
  %or.cond390 = select i1 %cmp258, i1 true, i1 %tobool260
  br i1 %or.cond390, label %if.end262, label %if.end262.thread

if.end262.thread:                                 ; preds = %if.end257
  store i32 %root_score.4, ptr @gamestate, align 8, !tbaa !59
  br label %if.end271

if.end262:                                        ; preds = %if.end257.thread, %if.end257
  %tobool260641 = phi i1 [ true, %if.end257.thread ], [ %tobool260, %if.end257 ]
  %56 = phi i32 [ %36, %if.end257.thread ], [ %55, %if.end257 ]
  %best_move.2640 = phi i32 [ %best_move.0, %if.end257.thread ], [ %best_move.2, %if.end257 ]
  %root_score.4639 = phi i32 [ %root_score.1, %if.end257.thread ], [ %root_score.4, %if.end257 ]
  %cmp265 = icmp eq i32 %56, -32000
  %or.cond391 = select i1 %tobool260641, i1 %cmp265, i1 false
  %tobool267 = icmp ne i32 %no_moves.0, 0
  %or.cond403 = select i1 %or.cond391, i1 %tobool267, i1 false
  br i1 %or.cond403, label %if.then268, label %if.end271

if.then268:                                       ; preds = %if.end262
  store i32 1, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 26), align 4, !tbaa !61
  br label %if.end271

if.end271:                                        ; preds = %if.end262.thread, %if.end262, %if.then268, %while.body
  %no_moves.1 = phi i32 [ %no_moves.0, %while.body ], [ 0, %if.then268 ], [ 0, %if.end262 ], [ 0, %if.end262.thread ]
  %root_score.5 = phi i32 [ %root_score.0, %while.body ], [ %root_score.4639, %if.then268 ], [ %root_score.4639, %if.end262 ], [ %root_score.4, %if.end262.thread ]
  %mc.1 = phi i32 [ %mc.0, %while.body ], [ %inc40, %if.then268 ], [ %inc40, %if.end262 ], [ %inc40, %if.end262.thread ]
  %best_move.3 = phi i32 [ %best_move.0, %while.body ], [ %best_move.2640, %if.then268 ], [ %best_move.2640, %if.end262 ], [ %best_move.2, %if.end262.thread ]
  %57 = load i32, ptr @uci_mode, align 4, !tbaa !18
  %tobool272 = icmp ne i32 %57, 0
  %58 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4
  %cmp274 = icmp sgt i32 %58, 4
  %or.cond392 = select i1 %tobool272, i1 %cmp274, i1 false
  br i1 %or.cond392, label %land.lhs.true275, label %if.end283

land.lhs.true275:                                 ; preds = %if.end271
  %call276 = call noundef i32 @_Z5rtimev()
  %59 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 19), align 8, !tbaa !29
  %call277 = call noundef i32 @_Z9rdifftimeii(i32 noundef %call276, i32 noundef %59)
  %cmp278 = icmp sgt i32 %call277, 149
  %60 = load i32, ptr @uci_showrefutations, align 4
  %tobool280 = icmp ne i32 %60, 0
  %or.cond393 = select i1 %cmp278, i1 %tobool280, i1 false
  br i1 %or.cond393, label %if.then281, label %if.end283

if.then281:                                       ; preds = %land.lhs.true275
  call void (ptr, ...) @_Z8myprintfPKcz(ptr noundef nonnull @.str, ptr noundef nonnull %searching_move)
  call void @_Z24extract_current_bestlineP7state_t(ptr noundef nonnull %s)
  br label %if.end283

if.end283:                                        ; preds = %if.then281, %land.lhs.true275, %if.end271
  %61 = load i32, ptr %arrayidx22, align 4, !tbaa !18
  call void @_Z6unmakeP7state_ti(ptr noundef nonnull %s, i32 noundef %61)
  %62 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %tobool286.not = icmp eq i32 %62, 0
  br i1 %tobool286.not, label %if.end288, label %cleanup

if.end288:                                        ; preds = %if.end283
  br i1 %tobool36.not, label %while.cond, label %if.then290, !llvm.loop !62

if.then290:                                       ; preds = %if.end288
  %cmp291 = icmp sgt i32 %root_score.5, %alpha.0.ph
  br i1 %cmp291, label %if.then292, label %if.end302

if.then292:                                       ; preds = %if.then290
  %63 = load i32, ptr %arrayidx22, align 4, !tbaa !18
  call fastcc void @_ZL12history_goodP7state_tii(ptr noundef nonnull %s, i32 noundef %63, i32 noundef %spec.select)
  %64 = load i32, ptr %arrayidx22, align 4, !tbaa !18
  store i32 %root_score.5, ptr @gamestate, align 8, !tbaa !59
  %call297 = call noundef zeroext i16 @_Z12compact_movei(i32 noundef %64)
  %conv298 = zext i16 %call297 to i32
  store i32 %conv298, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 20), align 4, !tbaa !60
  %65 = load i32, ptr @gamestate, align 8, !tbaa !59
  %cmp299.not = icmp slt i32 %65, %originalbeta
  br i1 %cmp299.not, label %if.end302, label %if.then300

if.then300:                                       ; preds = %if.then292
  call void @_Z7StoreTTP7state_tiiijiiii(ptr noundef nonnull %s, i32 noundef %65, i32 noundef %originalalpha, i32 noundef %originalbeta, i32 noundef %conv298, i32 noundef 0, i32 noundef 0, i32 noundef 0, i32 noundef %spec.select)
  br label %cleanup

if.end302:                                        ; preds = %if.then292, %if.then290
  %alpha.1 = phi i32 [ %root_score.5, %if.then292 ], [ %alpha.0.ph, %if.then290 ]
  %best_move.4 = phi i32 [ %64, %if.then292 ], [ %best_move.3, %if.then290 ]
  br i1 %tobool303.not, label %while.cond.outer.backedge, label %if.then304

while.cond.outer.backedge:                        ; preds = %if.end302, %if.then304
  br label %while.cond.outer, !llvm.loop !62

if.then304:                                       ; preds = %if.end302
  %66 = load i64, ptr %nodes, align 8, !tbaa !42
  %sub306 = sub i64 %66, %15
  store i64 %sub306, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 18), align 8, !tbaa !63
  br label %while.cond.outer.backedge

while.end:                                        ; preds = %for.cond.preheader.i, %if.end12.i, %_ZL15remove_one_fastPiS_S_i.exit
  %67 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4, !tbaa !19
  %cmp309 = icmp eq i32 %67, 4
  br i1 %cmp309, label %if.end319.sink.split, label %if.else311

if.else311:                                       ; preds = %while.end
  %cmp312 = icmp slt i32 %67, 5
  %68 = load i32, ptr @_ZZ11search_rootP7state_tiiiE5bmove, align 4
  %cmp314.not = icmp eq i32 %best_move.0, %68
  %or.cond556 = select i1 %cmp312, i1 true, i1 %cmp314.not
  br i1 %or.cond556, label %if.end319, label %if.then315

if.then315:                                       ; preds = %if.else311
  %69 = load i32, ptr @_ZZ11search_rootP7state_tiiiE7changes, align 4, !tbaa !18
  %inc316 = add nsw i32 %69, 1
  br label %if.end319.sink.split

if.end319.sink.split:                             ; preds = %while.end, %if.then315
  %inc316.sink = phi i32 [ %inc316, %if.then315 ], [ 0, %while.end ]
  store i32 %inc316.sink, ptr @_ZZ11search_rootP7state_tiiiE7changes, align 4, !tbaa !18
  br label %if.end319

if.end319:                                        ; preds = %if.end319.sink.split, %if.else311
  %70 = load i64, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 18), align 8, !tbaa !63
  %mul = mul i64 %70, 100
  %71 = load i64, ptr %nodes, align 8, !tbaa !42
  %div = udiv i64 %mul, %71
  %cmp321 = icmp ult i64 %div, 75
  %cmp323 = icmp slt i32 %67, 7
  %or.cond394.not554 = select i1 %cmp321, i1 true, i1 %cmp323
  %72 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8
  %cmp325 = icmp eq i32 %72, 99999999
  %or.cond395 = select i1 %or.cond394.not554, i1 true, i1 %cmp325
  %73 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 22), align 4
  %tobool327 = icmp ne i32 %73, 0
  %or.cond396 = select i1 %or.cond395, i1 true, i1 %tobool327
  br i1 %or.cond396, label %if.end330, label %if.then328

if.then328:                                       ; preds = %if.end319
  %div329 = sdiv i32 %72, 2
  store i32 %div329, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8, !tbaa !30
  br label %if.end330

if.end330:                                        ; preds = %if.then328, %if.end319
  %74 = load i32, ptr @_ZZ11search_rootP7state_tiiiE7changes, align 4, !tbaa !18
  %cmp331 = icmp sgt i32 %74, 3
  br i1 %cmp331, label %if.then332, label %if.else333

if.then332:                                       ; preds = %if.end330
  store i32 1, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 23), align 8, !tbaa !34
  %dec = add nsw i32 %74, -1
  store i32 %dec, ptr @_ZZ11search_rootP7state_tiiiE7changes, align 4, !tbaa !18
  br label %if.end337

if.else333:                                       ; preds = %if.end330
  %75 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 23), align 8, !tbaa !34
  %tobool334.not = icmp eq i32 %75, 0
  br i1 %tobool334.not, label %if.end337, label %if.then335

if.then335:                                       ; preds = %if.else333
  store i32 0, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 23), align 8, !tbaa !34
  br label %if.end337

if.end337:                                        ; preds = %if.else333, %if.then335, %if.then332
  store i32 %best_move.0, ptr @_ZZ11search_rootP7state_tiiiE5bmove, align 4, !tbaa !18
  %tobool338 = icmp eq i32 %no_moves.0, 0
  %76 = load i32, ptr @is_pondering, align 4
  %tobool340 = icmp ne i32 %76, 0
  %or.cond397 = select i1 %tobool338, i1 true, i1 %tobool340
  %77 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 26), align 4
  %tobool342 = icmp ne i32 %77, 0
  %or.cond398 = select i1 %or.cond397, i1 true, i1 %tobool342
  br i1 %or.cond398, label %if.else353, label %if.then343

if.then343:                                       ; preds = %if.end337
  %call344 = call noundef i32 @_Z8in_checkP7state_t(ptr noundef nonnull %s)
  %tobool345.not = icmp eq i32 %call344, 0
  br i1 %tobool345.not, label %if.else351, label %if.then346

if.then346:                                       ; preds = %if.then343
  %white_to_move = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 11
  %78 = load i32, ptr %white_to_move, align 4, !tbaa !17
  %cmp347 = icmp eq i32 %78, 1
  br i1 %cmp347, label %if.then348, label %if.else349

if.then348:                                       ; preds = %if.then346
  store i32 2, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 4), align 8, !tbaa !64
  br label %if.end361

if.else349:                                       ; preds = %if.then346
  store i32 3, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 4), align 8, !tbaa !64
  br label %if.end361

if.else351:                                       ; preds = %if.then343
  store i32 1, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 4), align 8, !tbaa !64
  br label %if.end361

if.else353:                                       ; preds = %if.end337
  %fifty = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 15
  %79 = load i32, ptr %fifty, align 4, !tbaa !14
  %cmp354 = icmp slt i32 %79, 99
  %or.cond399 = select i1 %cmp354, i1 true, i1 %tobool340
  %80 = load i32, ptr @uci_mode, align 4
  %tobool358 = icmp ne i32 %80, 0
  %or.cond400 = select i1 %or.cond399, i1 true, i1 %tobool358
  br i1 %or.cond400, label %if.end361, label %if.then359

if.then359:                                       ; preds = %if.else353
  store i32 4, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 4), align 8, !tbaa !64
  store i32 0, ptr @gamestate, align 8, !tbaa !59
  br label %if.end361

if.end361:                                        ; preds = %if.else353, %if.then359, %if.else351, %if.else349, %if.then348
  %81 = load i32, ptr @gamestate, align 8, !tbaa !59
  %82 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 20), align 4, !tbaa !60
  call void @_Z7StoreTTP7state_tiiijiiii(ptr noundef nonnull %s, i32 noundef %81, i32 noundef %originalalpha, i32 noundef %originalbeta, i32 noundef %82, i32 noundef 0, i32 noundef 0, i32 noundef 0, i32 noundef %spec.select)
  br label %cleanup

cleanup:                                          ; preds = %if.end283, %if.end361, %if.then300
  %retval.0 = phi i32 [ %64, %if.then300 ], [ %best_move.0, %if.end361 ], [ %best_move.3, %if.end283 ]
  call void @llvm.lifetime.end.p0(i64 512, ptr nonnull %searching_move) #9
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %dummy2) #9
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %dummy) #9
  call void @llvm.lifetime.end.p0(i64 960, ptr nonnull %move_ordering) #9
  call void @llvm.lifetime.end.p0(i64 960, ptr nonnull %moves) #9
  ret i32 %retval.0
}

declare void @_Z11comp_to_sanP7state_tiPc(ptr noundef, i32 noundef, ptr noundef) local_unnamed_addr #2

declare void @_Z13comp_to_coordP7state_tiPc(ptr noundef, i32 noundef, ptr noundef) local_unnamed_addr #2

declare noundef i32 @_Z4ranki(i32 noundef) local_unnamed_addr #2

declare void @_Z16post_fh_thinkingP7state_tiiPci(ptr noundef, i32 noundef, i32 noundef, ptr noundef, i32 noundef) local_unnamed_addr #2

declare void @_Z16post_fl_thinkingP7state_tiiPci(ptr noundef, i32 noundef, i32 noundef, ptr noundef, i32 noundef) local_unnamed_addr #2

declare void @_Z13post_thinkingP7state_tiiPci(ptr noundef, i32 noundef, i32 noundef, ptr noundef, i32 noundef) local_unnamed_addr #2

declare void @_Z21post_multipv_thinkingP7state_tiii(ptr noundef, i32 noundef, i32 noundef, i32 noundef) local_unnamed_addr #2

declare noundef i32 @_Z9rdifftimeii(i32 noundef, i32 noundef) local_unnamed_addr #2

declare noundef i32 @_Z5rtimev() local_unnamed_addr #2

declare void @_Z8myprintfPKcz(ptr noundef, ...) local_unnamed_addr #2

declare void @_Z24extract_current_bestlineP7state_t(ptr noundef) local_unnamed_addr #2

; Function Attrs: mustprogress nofree norecurse nosync nounwind memory(write, inaccessiblemem: none) uwtable
define dso_local void @_Z21reset_search_countersP7state_t(ptr nocapture noundef writeonly %s) local_unnamed_addr #5 {
entry:
  call void @mcount()
  %nodes = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 22
  %ply = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 14
  store i32 0, ptr %ply, align 8, !tbaa !10
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(20) %nodes, i8 0, i64 20, i1 false)
  %TTProbes = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 26
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(16) getelementptr inbounds (%struct.scoreboard_t, ptr @scoreboard, i64 0, i32 4, i64 0, i32 22), i8 0, i64 16, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(16) getelementptr inbounds (%struct.scoreboard_t, ptr @scoreboard, i64 0, i32 4, i64 1, i32 22), i8 0, i64 16, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(16) getelementptr inbounds (%struct.scoreboard_t, ptr @scoreboard, i64 0, i32 4, i64 2, i32 22), i8 0, i64 16, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(16) getelementptr inbounds (%struct.scoreboard_t, ptr @scoreboard, i64 0, i32 4, i64 3, i32 22), i8 0, i64 16, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(16) getelementptr inbounds (%struct.scoreboard_t, ptr @scoreboard, i64 0, i32 4, i64 4, i32 22), i8 0, i64 16, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(16) getelementptr inbounds (%struct.scoreboard_t, ptr @scoreboard, i64 0, i32 4, i64 5, i32 22), i8 0, i64 16, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(16) getelementptr inbounds (%struct.scoreboard_t, ptr @scoreboard, i64 0, i32 4, i64 6, i32 22), i8 0, i64 16, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(16) getelementptr inbounds (%struct.scoreboard_t, ptr @scoreboard, i64 0, i32 4, i64 7, i32 22), i8 0, i64 16, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 4 dereferenceable(16) %TTProbes, i8 0, i64 16, i1 false)
  store i32 0, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 24), align 4, !tbaa !33
  ret void
}

; Function Attrs: mustprogress uwtable
define dso_local void @_Z17reset_search_dataP7state_t(ptr nocapture noundef writeonly %s) local_unnamed_addr #0 {
entry:
  call void @mcount()
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 16 dereferenceable(24576) @history_h, i8 0, i64 24576, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 16 dereferenceable(24576) @history_hit, i8 0, i64 24576, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 16 dereferenceable(24576) @history_tot, i8 0, i64 24576, i1 false)
  %uglygep = getelementptr i8, ptr %s, i64 3056
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(1024) %uglygep, i8 0, i64 1024, i1 false), !tbaa !18
  store i64 0, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 18), align 8, !tbaa !63
  store i32 0, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 20), align 4, !tbaa !60
  %0 = load i32, ptr @uci_limitstrength, align 4, !tbaa !18
  %tobool.not = icmp eq i32 %0, 0
  br i1 %tobool.not, label %if.end, label %if.then

if.then:                                          ; preds = %entry
  %1 = load i32, ptr @uci_elo, align 4, !tbaa !18
  %call = tail call noundef i32 @_Z12elo_to_depthi(i32 noundef %1)
  %2 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 14), align 8, !tbaa !18
  %.sroa.speculated = tail call i32 @llvm.smin.i32(i32 %call, i32 %2)
  store i32 %.sroa.speculated, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 14), align 8, !tbaa !65
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  ret void
}

; Function Attrs: mustprogress nocallback nofree nounwind willreturn memory(argmem: write)
declare void @llvm.memset.p0.i64(ptr nocapture writeonly, i8, i64, i1 immarg) #6

declare noundef i32 @_Z12elo_to_depthi(i32 noundef) local_unnamed_addr #2

; Function Attrs: mustprogress uwtable
define dso_local noundef i32 @_Z5thinkP11gamestate_tP7state_t(ptr nocapture noundef readnone %g, ptr noundef %s) local_unnamed_addr #0 {
entry:
  call void @mcount()
  %output = alloca [512 x i8], align 16
  %output2 = alloca [512 x i8], align 16
  %moves = alloca [240 x i32], align 16
  call void @llvm.lifetime.start.p0(i64 512, ptr nonnull %output) #9
  call void @llvm.lifetime.start.p0(i64 512, ptr nonnull %output2) #9
  call void @llvm.lifetime.start.p0(i64 960, ptr nonnull %moves) #9
  tail call void @_Z11clear_dp_ttv()
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 16 dereferenceable(24576) @history_h, i8 0, i64 24576, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 16 dereferenceable(24576) @history_hit, i8 0, i64 24576, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 16 dereferenceable(24576) @history_tot, i8 0, i64 24576, i1 false)
  %uglygep.i = getelementptr i8, ptr %s, i64 3056
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(1024) %uglygep.i, i8 0, i64 1024, i1 false), !tbaa !18
  store i64 0, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 18), align 8, !tbaa !63
  store i32 0, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 20), align 4, !tbaa !60
  %0 = load i32, ptr @uci_limitstrength, align 4, !tbaa !18
  %tobool.not.i = icmp eq i32 %0, 0
  br i1 %tobool.not.i, label %_Z17reset_search_dataP7state_t.exit, label %if.then.i

if.then.i:                                        ; preds = %entry
  %1 = load i32, ptr @uci_elo, align 4, !tbaa !18
  %call.i = tail call noundef i32 @_Z12elo_to_depthi(i32 noundef %1)
  %2 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 14), align 8, !tbaa !18
  %.sroa.speculated.i = tail call i32 @llvm.smin.i32(i32 %call.i, i32 %2)
  store i32 %.sroa.speculated.i, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 14), align 8, !tbaa !65
  br label %_Z17reset_search_dataP7state_t.exit

_Z17reset_search_dataP7state_t.exit:              ; preds = %entry, %if.then.i
  %nodes.i = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 22
  %ply.i = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 14
  store i32 0, ptr %ply.i, align 8, !tbaa !10
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(20) %nodes.i, i8 0, i64 20, i1 false)
  %TTProbes.i = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 26
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(16) getelementptr inbounds (%struct.scoreboard_t, ptr @scoreboard, i64 0, i32 4, i64 0, i32 22), i8 0, i64 16, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(16) getelementptr inbounds (%struct.scoreboard_t, ptr @scoreboard, i64 0, i32 4, i64 1, i32 22), i8 0, i64 16, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(16) getelementptr inbounds (%struct.scoreboard_t, ptr @scoreboard, i64 0, i32 4, i64 2, i32 22), i8 0, i64 16, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(16) getelementptr inbounds (%struct.scoreboard_t, ptr @scoreboard, i64 0, i32 4, i64 3, i32 22), i8 0, i64 16, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(16) getelementptr inbounds (%struct.scoreboard_t, ptr @scoreboard, i64 0, i32 4, i64 4, i32 22), i8 0, i64 16, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(16) getelementptr inbounds (%struct.scoreboard_t, ptr @scoreboard, i64 0, i32 4, i64 5, i32 22), i8 0, i64 16, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(16) getelementptr inbounds (%struct.scoreboard_t, ptr @scoreboard, i64 0, i32 4, i64 6, i32 22), i8 0, i64 16, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(16) getelementptr inbounds (%struct.scoreboard_t, ptr @scoreboard, i64 0, i32 4, i64 7, i32 22), i8 0, i64 16, i1 false)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 4 dereferenceable(16) %TTProbes.i, i8 0, i64 16, i1 false)
  store i32 0, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 24), align 4, !tbaa !33
  %call = tail call noundef i32 @_Z5rtimev()
  store i32 %call, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 19), align 8, !tbaa !29
  store i32 1, ptr %ply.i, align 8, !tbaa !10
  %hash = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 16
  %3 = load i64, ptr %hash, align 8, !tbaa !26
  %4 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 15), align 4, !tbaa !27
  %idxprom = sext i32 %4 to i64
  %arrayidx = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 36, i64 %idxprom
  store i64 %3, ptr %arrayidx, align 8, !tbaa !6
  %call1 = tail call noundef i32 @_Z8in_checkP7state_t(ptr noundef nonnull %s)
  %5 = load i32, ptr %ply.i, align 8, !tbaa !10
  %idxprom3 = sext i32 %5 to i64
  %arrayidx4 = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 25, i64 %idxprom3
  store i32 %call1, ptr %arrayidx4, align 4, !tbaa !18
  %tobool.not = icmp eq i32 %call1, 0
  br i1 %tobool.not, label %if.else, label %if.then

if.then:                                          ; preds = %_Z17reset_search_dataP7state_t.exit
  %call5 = call noundef i32 @_Z12gen_evasionsP7state_tPii(ptr noundef nonnull %s, ptr noundef nonnull %moves, i32 noundef %call1)
  br label %if.end

if.else:                                          ; preds = %_Z17reset_search_dataP7state_t.exit
  %call7 = call noundef i32 @_Z3genP7state_tPi(ptr noundef nonnull %s, ptr noundef nonnull %moves)
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %num_moves.0 = phi i32 [ %call5, %if.then ], [ %call7, %if.else ]
  %cmp418 = icmp sgt i32 %num_moves.0, 0
  br i1 %cmp418, label %for.body.preheader, label %if.end33

for.body.preheader:                               ; preds = %if.end
  %wide.trip.count = zext i32 %num_moves.0 to i64
  br label %for.body

for.body:                                         ; preds = %for.body.preheader, %for.body
  %indvars.iv = phi i64 [ 0, %for.body.preheader ], [ %indvars.iv.next, %for.body ]
  %legals.0421 = phi i32 [ 0, %for.body.preheader ], [ %spec.select409, %for.body ]
  %lastlegal.0420 = phi i32 [ 0, %for.body.preheader ], [ %spec.select, %for.body ]
  %arrayidx9 = getelementptr inbounds [240 x i32], ptr %moves, i64 0, i64 %indvars.iv
  %6 = load i32, ptr %arrayidx9, align 4, !tbaa !18
  call void @_Z4makeP7state_ti(ptr noundef %s, i32 noundef %6)
  %7 = load i32, ptr %arrayidx9, align 4, !tbaa !18
  %call12 = call noundef i32 @_Z11check_legalP7state_ti(ptr noundef %s, i32 noundef %7)
  %tobool13.not = icmp ne i32 %call12, 0
  %8 = trunc i64 %indvars.iv to i32
  %spec.select = select i1 %tobool13.not, i32 %8, i32 %lastlegal.0420
  %inc = zext i1 %tobool13.not to i32
  %spec.select409 = add nuw nsw i32 %legals.0421, %inc
  %9 = load i32, ptr %arrayidx9, align 4, !tbaa !18
  call void @_Z6unmakeP7state_ti(ptr noundef %s, i32 noundef %9)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond.not = icmp eq i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond.not, label %for.end, label %for.body, !llvm.loop !66

for.end:                                          ; preds = %for.body
  %10 = load i32, ptr @is_pondering, align 4, !tbaa !18
  %tobool19 = icmp eq i32 %10, 0
  %cmp21 = icmp eq i32 %spec.select409, 1
  %or.cond329 = select i1 %tobool19, i1 %cmp21, i1 false
  br i1 %or.cond329, label %if.then22, label %if.end33

if.then22:                                        ; preds = %for.end
  %11 = load i32, ptr @uci_mode, align 4, !tbaa !18
  %tobool23.not = icmp eq i32 %11, 0
  %.pre468 = sext i32 %spec.select to i64
  br i1 %tobool23.not, label %if.end29, label %if.then24

if.then24:                                        ; preds = %if.then22
  %arrayidx26 = getelementptr inbounds [240 x i32], ptr %moves, i64 0, i64 %.pre468
  %12 = load i32, ptr %arrayidx26, align 4, !tbaa !18
  call void @_Z13comp_to_coordP7state_tiPc(ptr noundef %s, i32 noundef %12, ptr noundef nonnull %output)
  %13 = load i32, ptr @_ZZ5thinkP11gamestate_tP7state_tE15lastsearchscore, align 4, !tbaa !18
  call void (ptr, ...) @_Z8myprintfPKcz(ptr noundef nonnull @.str.1, i32 noundef %13)
  call void (ptr, ...) @_Z8myprintfPKcz(ptr noundef nonnull @.str.2, ptr noundef nonnull %output)
  br label %if.end29

if.end29:                                         ; preds = %if.then22, %if.then24
  %arrayidx31 = getelementptr inbounds [240 x i32], ptr %moves, i64 0, i64 %.pre468
  %14 = load i32, ptr %arrayidx31, align 4, !tbaa !18
  br label %cleanup

if.end33:                                         ; preds = %if.end, %for.end
  call void @_Z11check_phaseP11gamestate_tP7state_t(ptr noundef nonnull @gamestate, ptr noundef %s)
  %15 = load i32, ptr @uci_mode, align 4, !tbaa !18
  %tobool34.not = icmp eq i32 %15, 0
  br i1 %tobool34.not, label %if.then35, label %if.end38

if.then35:                                        ; preds = %if.end33
  %16 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 1), align 4, !tbaa !43
  %17 = icmp ult i32 %16, 3
  br i1 %17, label %switch.lookup, label %if.end38

switch.lookup:                                    ; preds = %if.then35
  %18 = sext i32 %16 to i64
  %reltable.shift = shl i64 %18, 2
  %reltable.intrinsic = call ptr @llvm.load.relative.i64(ptr @reltable._Z5thinkP11gamestate_tP7state_t, i64 %reltable.shift)
  call void (ptr, ...) @_Z8myprintfPKcz(ptr noundef nonnull %reltable.intrinsic)
  br label %if.end38

if.end38:                                         ; preds = %if.then35, %switch.lookup, %if.end33
  %19 = load i32, ptr @is_pondering, align 4, !tbaa !18
  %tobool39 = icmp ne i32 %19, 0
  %20 = load i32, ptr @is_analyzing, align 4
  %tobool40 = icmp ne i32 %20, 0
  %or.cond = select i1 %tobool39, i1 true, i1 %tobool40
  br i1 %or.cond, label %if.end48.thread, label %if.then41

if.then41:                                        ; preds = %if.end38
  %21 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 13), align 4, !tbaa !67
  %tobool42.not = icmp eq i32 %21, 0
  br i1 %tobool42.not, label %if.then43, label %if.end48

if.then43:                                        ; preds = %if.then41
  %call44 = call noundef i32 @_Z13allocate_timeP11gamestate_ti(ptr noundef nonnull @gamestate, i32 noundef 1)
  br label %if.end48

if.end48:                                         ; preds = %if.then41, %if.then43
  %call44.sink = phi i32 [ %call44, %if.then43 ], [ %21, %if.then41 ]
  store i32 %call44.sink, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8, !tbaa !30
  %22 = load i32, ptr @uci_mode, align 4, !tbaa !18
  %tobool49.not = icmp eq i32 %22, 0
  br i1 %tobool49.not, label %if.then50, label %if.else51

if.end48.thread:                                  ; preds = %if.end38
  store i32 99999999, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8, !tbaa !30
  %23 = load i32, ptr @uci_mode, align 4, !tbaa !18
  %tobool49.not474 = icmp eq i32 %23, 0
  br i1 %tobool49.not474, label %if.then50, label %if.end58

if.then50:                                        ; preds = %if.end48.thread, %if.end48
  %24 = phi i32 [ 99999999, %if.end48.thread ], [ %call44.sink, %if.end48 ]
  call void (ptr, ...) @_Z8myprintfPKcz(ptr noundef nonnull @.str.6, i32 noundef %24)
  br label %if.end58

if.else51:                                        ; preds = %if.end48
  %cmp52.not = icmp eq i32 %call44.sink, 99999999
  br i1 %cmp52.not, label %if.end58, label %if.then53

if.then53:                                        ; preds = %if.else51
  %div = sdiv i32 %call44.sink, 100
  %conv = sitofp i32 %call44.sink to double
  %div54 = fdiv double %conv, 2.500000e+00
  %div55 = fdiv double %div54, 1.000000e+02
  %conv56 = fptosi double %div55 to i32
  call void (ptr, ...) @_Z8myprintfPKcz(ptr noundef nonnull @.str.7, i32 noundef %div, i32 noundef %conv56)
  br label %if.end58

if.end58:                                         ; preds = %if.end48.thread, %if.else51, %if.then53, %if.then50
  store i32 0, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  store i32 0, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 26), align 4, !tbaa !61
  store i32 0, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 22), align 4, !tbaa !31
  store i32 0, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 23), align 8, !tbaa !34
  store i32 0, ptr @gamestate, align 8, !tbaa !59
  store i32 1, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4, !tbaa !19
  %25 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 14), align 8, !tbaa !18
  %.sroa.speculated423 = call i32 @llvm.smin.i32(i32 %25, i32 40)
  %cmp61.not424 = icmp slt i32 %.sroa.speculated423, 1
  br i1 %cmp61.not424, label %if.then194, label %for.body62.lr.ph

for.body62.lr.ph:                                 ; preds = %if.end58
  %TTStores = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 28
  %maxply = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 24
  br label %for.body62

for.body62:                                       ; preds = %for.body62.lr.ph, %for.inc190
  %true_i_depth.0428 = phi i32 [ 0, %for.body62.lr.ph ], [ %true_i_depth.3, %for.inc190 ]
  %temp_score.0427 = phi i32 [ 0, %for.body62.lr.ph ], [ %temp_score.3, %for.inc190 ]
  %comp_move.0426 = phi i32 [ 0, %for.body62.lr.ph ], [ %comp_move.3, %for.inc190 ]
  %storemerge425 = phi i32 [ 1, %for.body62.lr.ph ], [ %inc191, %for.inc190 ]
  %26 = load i32, ptr @uci_mode, align 4, !tbaa !18
  %tobool63.not = icmp eq i32 %26, 0
  br i1 %tobool63.not, label %if.end66, label %if.then64

if.then64:                                        ; preds = %for.body62
  %27 = load <2 x i32>, ptr %TTStores, align 4, !tbaa !18
  %28 = lshr <2 x i32> %27, <i32 1, i32 1>
  store <2 x i32> %28, ptr %TTStores, align 4, !tbaa !18
  br label %if.end66

if.end66:                                         ; preds = %if.then64, %for.body62
  %29 = load i32, ptr %maxply, align 8, !tbaa !13
  %cmp67 = icmp sgt i32 %storemerge425, %29
  br i1 %cmp67, label %if.then68, label %if.end70

if.then68:                                        ; preds = %if.end66
  store i32 %storemerge425, ptr %maxply, align 8, !tbaa !13
  br label %if.end70

if.end70:                                         ; preds = %if.then68, %if.end66
  %call71 = call noundef i32 @_Z5rtimev()
  %30 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 19), align 8, !tbaa !29
  %call72 = call noundef i32 @_Z9rdifftimeii(i32 noundef %call71, i32 noundef %30)
  %31 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 22), align 4, !tbaa !31
  %tobool73 = icmp ne i32 %31, 0
  %32 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 23), align 8
  %tobool75 = icmp ne i32 %32, 0
  %or.cond280 = select i1 %tobool73, i1 true, i1 %tobool75
  br i1 %or.cond280, label %if.end88, label %land.lhs.true76

land.lhs.true76:                                  ; preds = %if.end70
  %33 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 24), align 4, !tbaa !33
  %tobool77.not = icmp eq i32 %33, 0
  br i1 %tobool77.not, label %lor.lhs.false, label %land.lhs.true85

lor.lhs.false:                                    ; preds = %land.lhs.true76
  %34 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 13), align 4, !tbaa !67
  %tobool78.not = icmp eq i32 %34, 0
  br i1 %tobool78.not, label %land.lhs.true79, label %if.end88

land.lhs.true79:                                  ; preds = %lor.lhs.false
  %conv80 = sitofp i32 %call72 to double
  %35 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8, !tbaa !30
  %conv81 = sitofp i32 %35 to double
  %div83 = fdiv double %conv81, 2.500000e+00
  %cmp84 = fcmp olt double %div83, %conv80
  %36 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4
  %cmp86 = icmp sgt i32 %36, 2
  %or.cond281 = select i1 %cmp84, i1 %cmp86, i1 false
  br i1 %or.cond281, label %for.end192, label %if.end88

land.lhs.true85:                                  ; preds = %land.lhs.true76
  %.old = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4, !tbaa !19
  %cmp86.old = icmp sgt i32 %.old, 2
  br i1 %cmp86.old, label %for.end192, label %if.end88

if.end88:                                         ; preds = %land.lhs.true85, %land.lhs.true79, %lor.lhs.false, %if.end70
  %sub = add nsw i32 %temp_score.0427, -30
  %add = add nsw i32 %temp_score.0427, 30
  %37 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4, !tbaa !19
  %mul89 = shl nsw i32 %37, 2
  %call90 = call noundef i32 @_Z11search_rootP7state_tiii(ptr noundef nonnull %s, i32 noundef %sub, i32 noundef %add, i32 noundef %mul89)
  %38 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 4), align 8, !tbaa !64
  %tobool91.not = icmp eq i32 %38, 0
  br i1 %tobool91.not, label %if.end93, label %for.end192

if.end93:                                         ; preds = %if.end88
  %39 = load i32, ptr @gamestate, align 8, !tbaa !59
  %cmp94 = icmp sgt i32 %39, %sub
  %40 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8
  %tobool96 = icmp ne i32 %40, 0
  %or.cond283 = select i1 %cmp94, i1 true, i1 %tobool96
  %not.or.cond283 = xor i1 %or.cond283, true
  %. = zext i1 %not.or.cond283 to i32
  store i32 %., ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 22), align 4, !tbaa !31
  br i1 %or.cond283, label %if.else120, label %if.then103

if.then103:                                       ; preds = %if.end93
  %41 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4, !tbaa !19
  %mul105 = shl nsw i32 %41, 2
  %call106 = call noundef i32 @_Z11search_rootP7state_tiii(ptr noundef nonnull %s, i32 noundef -1000000, i32 noundef %add, i32 noundef %mul105)
  %42 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %tobool107.not = icmp eq i32 %42, 0
  br i1 %tobool107.not, label %if.end109, label %if.end139

if.end109:                                        ; preds = %if.then103
  store i32 0, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 22), align 4, !tbaa !31
  %43 = load i32, ptr @gamestate, align 8, !tbaa !59
  %cmp110 = icmp slt i32 %43, %add
  br i1 %cmp110, label %if.end139, label %if.then113

if.then113:                                       ; preds = %if.end109
  %44 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4, !tbaa !19
  %mul114 = shl nsw i32 %44, 2
  %call115 = call noundef i32 @_Z11search_rootP7state_tiii(ptr noundef nonnull %s, i32 noundef -1000000, i32 noundef 1000000, i32 noundef %mul114)
  %45 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %tobool116.not = icmp eq i32 %45, 0
  br i1 %tobool116.not, label %if.end139.sink.split, label %if.end139

if.else120:                                       ; preds = %if.end93
  %cmp121 = icmp slt i32 %39, %add
  %or.cond289 = select i1 %cmp121, i1 true, i1 %tobool96
  br i1 %or.cond289, label %if.end139, label %if.then124

if.then124:                                       ; preds = %if.else120
  %46 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4, !tbaa !19
  %mul126 = shl nsw i32 %46, 2
  %call127 = call noundef i32 @_Z11search_rootP7state_tiii(ptr noundef nonnull %s, i32 noundef %sub, i32 noundef 1000000, i32 noundef %mul126)
  %47 = load i32, ptr @gamestate, align 8, !tbaa !59
  %cmp128 = icmp sgt i32 %47, %sub
  %48 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8
  %tobool130 = icmp ne i32 %48, 0
  %or.cond291 = select i1 %cmp128, i1 true, i1 %tobool130
  br i1 %or.cond291, label %if.end139, label %if.then131

if.then131:                                       ; preds = %if.then124
  store i32 1, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 22), align 4, !tbaa !31
  %49 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4, !tbaa !19
  %mul132 = shl nsw i32 %49, 2
  %call133 = call noundef i32 @_Z11search_rootP7state_tiii(ptr noundef nonnull %s, i32 noundef -1000000, i32 noundef 1000000, i32 noundef %mul132)
  %50 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %tobool134.not = icmp eq i32 %50, 0
  br i1 %tobool134.not, label %if.end139.sink.split, label %if.end139

if.end139.sink.split:                             ; preds = %if.then131, %if.then113
  %comp_move.1.ph = phi i32 [ %comp_move.0426, %if.then113 ], [ %call90, %if.then131 ]
  %temp_move.0.ph = phi i32 [ %call115, %if.then113 ], [ %call133, %if.then131 ]
  %temp_score.1.ph = phi i32 [ %temp_score.0427, %if.then113 ], [ %39, %if.then131 ]
  store i32 0, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 22), align 4, !tbaa !31
  br label %if.end139

if.end139:                                        ; preds = %if.end139.sink.split, %if.then103, %if.else120, %if.then131, %if.then124, %if.end109, %if.then113
  %comp_move.1 = phi i32 [ %comp_move.0426, %if.else120 ], [ %call90, %if.then124 ], [ %call90, %if.then131 ], [ %comp_move.0426, %if.end109 ], [ %comp_move.0426, %if.then113 ], [ %comp_move.0426, %if.then103 ], [ %comp_move.1.ph, %if.end139.sink.split ]
  %temp_move.0 = phi i32 [ %call90, %if.else120 ], [ %call127, %if.then124 ], [ %call133, %if.then131 ], [ %call106, %if.end109 ], [ %call115, %if.then113 ], [ %call106, %if.then103 ], [ %temp_move.0.ph, %if.end139.sink.split ]
  %temp_score.1 = phi i32 [ %temp_score.0427, %if.else120 ], [ %39, %if.then124 ], [ %39, %if.then131 ], [ %temp_score.0427, %if.end109 ], [ %temp_score.0427, %if.then113 ], [ %temp_score.0427, %if.then103 ], [ %temp_score.1.ph, %if.end139.sink.split ]
  %51 = load i32, ptr @uci_mode, align 4, !tbaa !18
  %tobool140.not = icmp eq i32 %51, 0
  br i1 %tobool140.not, label %if.then141, label %if.end149

if.then141:                                       ; preds = %if.end139
  %call142 = call noundef i32 @_Z9interruptv()
  %tobool143 = icmp ne i32 %call142, 0
  %52 = load i32, ptr @is_pondering, align 4
  %tobool145 = icmp ne i32 %52, 0
  %or.cond331 = select i1 %tobool143, i1 %tobool145, i1 false
  br i1 %or.cond331, label %for.end192, label %if.end149

if.end149:                                        ; preds = %if.then141, %if.end139
  %53 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 26), align 4, !tbaa !61
  %54 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 22), align 4
  %tobool152 = icmp eq i32 %54, 0
  %55 = or i32 %53, %temp_move.0
  %56 = icmp eq i32 %55, 0
  %or.cond295 = select i1 %56, i1 %tobool152, i1 false
  br i1 %or.cond295, label %if.then155, label %if.end156

if.then155:                                       ; preds = %if.end149
  %57 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %58 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 4), align 8, !tbaa !64
  call void (ptr, ...) @_Z8myprintfPKcz(ptr noundef nonnull @.str.8, i32 noundef %53, i32 noundef 0, i32 noundef %57, i32 noundef %58)
  call void (ptr, ...) @_Z8myprintfPKcz(ptr noundef nonnull @.str.9)
  br label %cleanup

if.end156:                                        ; preds = %if.end149
  %tobool150 = icmp eq i32 %53, 0
  br i1 %tobool150, label %if.then158, label %if.end186

if.then158:                                       ; preds = %if.end156
  %59 = load i32, ptr @uci_mode, align 4, !tbaa !18
  %tobool159 = icmp eq i32 %59, 0
  %60 = load i32, ptr @gamestate, align 8
  %cmp161 = icmp eq i32 %60, -32000
  %or.cond297 = select i1 %tobool159, i1 %cmp161, i1 false
  br i1 %or.cond297, label %cleanup, label %if.end163

if.end163:                                        ; preds = %if.then158
  %61 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %tobool164.not = icmp eq i32 %61, 0
  %62 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4
  %spec.select410 = select i1 %tobool164.not, i32 %62, i32 %true_i_depth.0428
  %or.cond299.not408 = select i1 %tobool164.not, i1 %tobool159, i1 false
  %63 = load i32, ptr @uci_multipv, align 4
  %cmp171 = icmp eq i32 %63, 1
  %or.cond301 = select i1 %or.cond299.not408, i1 %cmp171, i1 false
  br i1 %or.cond301, label %if.then172, label %if.end173

if.then172:                                       ; preds = %if.end163
  call void @_Z13post_thinkingP7state_tiiPci(ptr noundef nonnull %s, i32 noundef %60, i32 noundef %temp_move.0, ptr noundef null, i32 noundef 0)
  %.pre = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4, !tbaa !19
  br label %if.end173

if.end173:                                        ; preds = %if.then172, %if.end163
  %64 = phi i32 [ %.pre, %if.then172 ], [ %62, %if.end163 ]
  %cmp174 = icmp sgt i32 %64, 2
  %cmp176 = icmp sgt i32 %60, 31500
  %or.cond303 = select i1 %cmp174, i1 %cmp176, i1 false
  br i1 %or.cond303, label %land.lhs.true177, label %if.end186

land.lhs.true177:                                 ; preds = %if.end173
  %65 = load i32, ptr @gamestate, align 8, !tbaa !59
  %sub178 = sub nsw i32 32000, %65
  %cmp179 = icmp slt i32 %sub178, %64
  br i1 %cmp179, label %land.lhs.true180, label %if.end186

land.lhs.true180:                                 ; preds = %land.lhs.true177
  %66 = load i32, ptr @is_pondering, align 4, !tbaa !18
  %tobool181 = icmp eq i32 %66, 0
  %67 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8
  %cmp183 = icmp ne i32 %67, 99999999
  %or.cond305 = select i1 %tobool181, i1 true, i1 %cmp183
  %68 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8
  %tobool187 = icmp ne i32 %68, 0
  %or.cond333 = select i1 %or.cond305, i1 true, i1 %tobool187
  br i1 %or.cond333, label %for.end192, label %for.inc190

if.end186:                                        ; preds = %if.end173, %land.lhs.true177, %if.end156
  %comp_move.2 = phi i32 [ %comp_move.1, %if.end156 ], [ %temp_move.0, %land.lhs.true177 ], [ %temp_move.0, %if.end173 ]
  %temp_score.2 = phi i32 [ %temp_score.1, %if.end156 ], [ %60, %land.lhs.true177 ], [ %60, %if.end173 ]
  %true_i_depth.2 = phi i32 [ %true_i_depth.0428, %if.end156 ], [ %spec.select410, %land.lhs.true177 ], [ %spec.select410, %if.end173 ]
  %.old332 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 25), align 8, !tbaa !28
  %tobool187.old.not = icmp eq i32 %.old332, 0
  br i1 %tobool187.old.not, label %if.end186.for.inc190_crit_edge, label %for.end192

if.end186.for.inc190_crit_edge:                   ; preds = %if.end186
  %.pre466 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4, !tbaa !19
  br label %for.inc190

for.inc190:                                       ; preds = %if.end186.for.inc190_crit_edge, %land.lhs.true180
  %69 = phi i32 [ %.pre466, %if.end186.for.inc190_crit_edge ], [ %64, %land.lhs.true180 ]
  %comp_move.3 = phi i32 [ %comp_move.2, %if.end186.for.inc190_crit_edge ], [ %temp_move.0, %land.lhs.true180 ]
  %temp_score.3 = phi i32 [ %temp_score.2, %if.end186.for.inc190_crit_edge ], [ %60, %land.lhs.true180 ]
  %true_i_depth.3 = phi i32 [ %true_i_depth.2, %if.end186.for.inc190_crit_edge ], [ %spec.select410, %land.lhs.true180 ]
  %inc191 = add nsw i32 %69, 1
  store i32 %inc191, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4, !tbaa !19
  %70 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 14), align 8, !tbaa !18
  %.sroa.speculated = call i32 @llvm.smin.i32(i32 %70, i32 40)
  %cmp61.not.not = icmp slt i32 %69, %.sroa.speculated
  br i1 %cmp61.not.not, label %for.body62, label %for.end192, !llvm.loop !68

for.end192:                                       ; preds = %for.inc190, %land.lhs.true79, %land.lhs.true85, %if.end88, %land.lhs.true180, %if.end186, %if.then141
  %comp_move.4 = phi i32 [ %comp_move.3, %for.inc190 ], [ %comp_move.0426, %land.lhs.true79 ], [ %comp_move.0426, %land.lhs.true85 ], [ %comp_move.0426, %if.end88 ], [ %temp_move.0, %land.lhs.true180 ], [ %comp_move.2, %if.end186 ], [ %comp_move.1, %if.then141 ]
  %temp_score.4 = phi i32 [ %temp_score.3, %for.inc190 ], [ %temp_score.0427, %land.lhs.true79 ], [ %temp_score.0427, %land.lhs.true85 ], [ %temp_score.0427, %if.end88 ], [ %60, %land.lhs.true180 ], [ %temp_score.2, %if.end186 ], [ %temp_score.1, %if.then141 ]
  %true_i_depth.4 = phi i32 [ %true_i_depth.3, %for.inc190 ], [ %true_i_depth.0428, %land.lhs.true79 ], [ %true_i_depth.0428, %land.lhs.true85 ], [ %true_i_depth.0428, %if.end88 ], [ %spec.select410, %land.lhs.true180 ], [ %true_i_depth.2, %if.end186 ], [ %true_i_depth.0428, %if.then141 ]
  %cmp193 = icmp eq i32 %comp_move.4, 0
  br i1 %cmp193, label %if.then194, label %if.end195

if.then194:                                       ; preds = %if.end58, %for.end192
  call void (ptr, ...) @_Z8myprintfPKcz(ptr noundef nonnull @.str.9)
  br label %cleanup

if.end195:                                        ; preds = %for.end192
  %71 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 5), align 4, !tbaa !19
  %cmp196 = icmp sgt i32 %71, 31
  %72 = load i32, ptr @is_pondering, align 4
  %tobool198 = icmp ne i32 %72, 0
  %or.cond307.not.not = select i1 %cmp196, i1 %tobool198, i1 false
  %73 = load i32, ptr @uci_mode, align 4
  %tobool200 = icmp ne i32 %73, 0
  %or.cond309 = select i1 %or.cond307.not.not, i1 %tobool200, i1 false
  %74 = load i32, ptr @buffered_count, align 4
  %tobool202 = icmp eq i32 %74, 0
  %or.cond311 = select i1 %or.cond309, i1 %tobool202, i1 false
  %.old457 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8
  %cmp204.old = icmp eq i32 %.old457, 99999999
  %or.cond459 = select i1 %or.cond311, i1 %cmp204.old, i1 false
  br i1 %or.cond459, label %land.rhs, label %if.end207

land.rhs:                                         ; preds = %if.end195, %land.rhs
  %call205 = call noundef i32 @_Z9interruptv()
  %tobool206.not = icmp eq i32 %call205, 0
  %75 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 12), align 8
  %cmp204 = icmp eq i32 %75, 99999999
  %or.cond458 = select i1 %tobool206.not, i1 %cmp204, i1 false
  br i1 %or.cond458, label %land.rhs, label %if.end207.loopexit, !llvm.loop !69

if.end207.loopexit:                               ; preds = %land.rhs
  %.pre467 = load i32, ptr @uci_mode, align 4, !tbaa !18
  br label %if.end207

if.end207:                                        ; preds = %if.end207.loopexit, %if.end195
  %76 = phi i32 [ %.pre467, %if.end207.loopexit ], [ %73, %if.end195 ]
  %tobool208.not = icmp eq i32 %76, 0
  br i1 %tobool208.not, label %if.end220, label %if.then209

if.then209:                                       ; preds = %if.end207
  call void @_Z4makeP7state_ti(ptr noundef %s, i32 noundef %comp_move.4)
  %call210 = call noundef i32 @_Z19extract_ponder_moveP7state_t(ptr noundef %s)
  call void @_Z6unmakeP7state_ti(ptr noundef %s, i32 noundef %comp_move.4)
  call void @_Z13comp_to_coordP7state_tiPc(ptr noundef %s, i32 noundef %comp_move.4, ptr noundef nonnull %output)
  %cmp212.not = icmp eq i32 %call210, 0
  br i1 %cmp212.not, label %if.else217, label %if.then213

if.then213:                                       ; preds = %if.then209
  call void @_Z13comp_to_coordP7state_tiPc(ptr noundef %s, i32 noundef %call210, ptr noundef nonnull %output2)
  call void (ptr, ...) @_Z8myprintfPKcz(ptr noundef nonnull @.str.10, ptr noundef nonnull %output, ptr noundef nonnull %output2)
  br label %if.end220

if.else217:                                       ; preds = %if.then209
  call void (ptr, ...) @_Z8myprintfPKcz(ptr noundef nonnull @.str.2, ptr noundef nonnull %output)
  br label %if.end220

if.end220:                                        ; preds = %if.then213, %if.else217, %if.end207
  %cmp221 = icmp ne i32 %temp_score.4, 31998
  %77 = load i32, ptr @is_pondering, align 4
  %tobool223 = icmp ne i32 %77, 0
  %or.cond313 = select i1 %cmp221, i1 true, i1 %tobool223
  br i1 %or.cond313, label %if.end229, label %if.then224

if.then224:                                       ; preds = %if.end220
  %white_to_move = getelementptr inbounds %struct.state_t, ptr %s, i64 0, i32 11
  %78 = load i32, ptr %white_to_move, align 4, !tbaa !17
  %tobool225.not = icmp eq i32 %78, 0
  %.484 = select i1 %tobool225.not, i32 2, i32 3
  store i32 %.484, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 4), align 8, !tbaa !64
  br label %if.end229

if.end229:                                        ; preds = %if.then224, %if.end220
  %cmp230 = icmp sgt i32 %call72, 20
  br i1 %cmp230, label %if.then231, label %if.end245

if.then231:                                       ; preds = %if.end229
  %79 = load i64, ptr %nodes.i, align 8, !tbaa !42
  %conv232 = uitofp i64 %79 to float
  %conv233 = sitofp i32 %call72 to float
  %div234 = fdiv float %conv233, 1.000000e+02
  %div235 = fdiv float %conv232, %div234
  %div236 = fdiv float %div235, 2.000000e+01
  %conv237 = fptosi float %div236 to i32
  %call238 = call noundef i32 @_Z4logLi(i32 noundef %conv237)
  %spec.store.select = call i32 @llvm.smax.i32(i32 %call238, i32 8)
  %spec.store.select334 = call i32 @llvm.umin.i32(i32 %spec.store.select, i32 17)
  store i32 %spec.store.select334, ptr @time_check_log, align 4
  br label %if.end245

if.end245:                                        ; preds = %if.then231, %if.end229
  %call246 = call noundef i32 @_Z8in_checkP7state_t(ptr noundef %s)
  %80 = load i32, ptr @is_pondering, align 4
  %.fr491 = freeze i32 %80
  %81 = load i32, ptr getelementptr inbounds (%struct.gamestate_t, ptr @gamestate, i64 0, i32 4), align 8
  %.fr492 = freeze i32 %81
  %82 = insertelement <4 x i32> poison, i32 %.fr492, i64 0
  %83 = shufflevector <4 x i32> %82, <4 x i32> poison, <4 x i32> zeroinitializer
  %.fr = freeze <4 x i32> %83
  %84 = icmp eq <4 x i32> %.fr, <i32 2, i32 3, i32 1, i32 4>
  %cmp259 = icmp ne i32 %.fr492, 5
  %cmp261 = icmp sgt i32 %true_i_depth.4, 4
  %85 = bitcast <4 x i1> %84 to i4
  %86 = icmp eq i4 %85, 0
  %87 = or i32 %.fr491, %call246
  %88 = icmp eq i32 %87, 0
  %89 = and i1 %86, %88
  %op.rdx489 = and i1 %89, %cmp259
  %op.rdx490 = select i1 %op.rdx489, i1 %cmp261, i1 false
  br i1 %op.rdx490, label %if.then262, label %cleanup

if.then262:                                       ; preds = %if.end245
  store i32 %temp_score.4, ptr @_ZZ5thinkP11gamestate_tP7state_tE15lastsearchscore, align 4, !tbaa !18
  br label %cleanup

cleanup:                                          ; preds = %if.then158, %if.end245, %if.then262, %if.then194, %if.then155, %if.end29
  %retval.0 = phi i32 [ %14, %if.end29 ], [ 0, %if.then194 ], [ 0, %if.then155 ], [ %comp_move.4, %if.then262 ], [ %comp_move.4, %if.end245 ], [ 0, %if.then158 ]
  call void @llvm.lifetime.end.p0(i64 960, ptr nonnull %moves) #9
  call void @llvm.lifetime.end.p0(i64 512, ptr nonnull %output2) #9
  call void @llvm.lifetime.end.p0(i64 512, ptr nonnull %output) #9
  ret i32 %retval.0
}

declare void @_Z11clear_dp_ttv() local_unnamed_addr #2

declare void @_Z11check_phaseP11gamestate_tP7state_t(ptr noundef, ptr noundef) local_unnamed_addr #2

declare noundef i32 @_Z13allocate_timeP11gamestate_ti(ptr noundef, i32 noundef) local_unnamed_addr #2

declare noundef i32 @_Z9interruptv() local_unnamed_addr #2

declare noundef i32 @_Z19extract_ponder_moveP7state_t(ptr noundef) local_unnamed_addr #2

declare noundef i32 @_Z4logLi(i32 noundef) local_unnamed_addr #2

declare noundef i32 @_Z14elo_to_blunderi(i32 noundef) local_unnamed_addr #2

declare noundef i32 @_Z8myrandomv() local_unnamed_addr #2

declare noundef i32 @_Z12taxicab_distii(i32 noundef, i32 noundef) local_unnamed_addr #2

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.abs.i32(i32, i1 immarg) #7

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.smin.i32(i32, i32) #7

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.smax.i32(i32, i32) #7

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.umin.i32(i32, i32) #7

declare void @mcount()

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: read)
declare ptr @llvm.load.relative.i64(ptr, i64) #8

attributes #0 = { mustprogress uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+avx,+crc32,+cx8,+fxsr,+mmx,+popcnt,+sse,+sse2,+sse3,+sse4.1,+sse4.2,+ssse3,+x87,+xsave" "tune-cpu"="generic" }
attributes #1 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #2 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+avx,+crc32,+cx8,+fxsr,+mmx,+popcnt,+sse,+sse2,+sse3,+sse4.1,+sse4.2,+ssse3,+x87,+xsave" "tune-cpu"="generic" }
attributes #3 = { mustprogress nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+avx,+crc32,+cx8,+fxsr,+mmx,+popcnt,+sse,+sse2,+sse3,+sse4.1,+sse4.2,+ssse3,+x87,+xsave" "tune-cpu"="generic" }
attributes #4 = { mustprogress nofree norecurse nosync nounwind memory(readwrite, inaccessiblemem: none) uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+avx,+crc32,+cx8,+fxsr,+mmx,+popcnt,+sse,+sse2,+sse3,+sse4.1,+sse4.2,+ssse3,+x87,+xsave" "tune-cpu"="generic" }
attributes #5 = { mustprogress nofree norecurse nosync nounwind memory(write, inaccessiblemem: none) uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+avx,+crc32,+cx8,+fxsr,+mmx,+popcnt,+sse,+sse2,+sse3,+sse4.1,+sse4.2,+ssse3,+x87,+xsave" "tune-cpu"="generic" }
attributes #6 = { mustprogress nocallback nofree nounwind willreturn memory(argmem: write) }
attributes #7 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #8 = { nocallback nofree nosync nounwind willreturn memory(argmem: read) }
attributes #9 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 16.0.6 (git@github.com:karouzakisp/llvm-project.git db8a4223b313363bfbacc46b5bd9452fa8bdcc28)"}
!6 = !{!7, !7, i64 0}
!7 = !{!"long long", !8, i64 0}
!8 = !{!"omnipotent char", !9, i64 0}
!9 = !{!"Simple C++ TBAA"}
!10 = !{!11, !12, i64 472}
!11 = !{!"_ZTS7state_t", !12, i64 0, !8, i64 4, !7, i64 264, !7, i64 272, !7, i64 280, !8, i64 288, !12, i64 392, !12, i64 396, !8, i64 400, !12, i64 452, !12, i64 456, !12, i64 460, !12, i64 464, !12, i64 468, !12, i64 472, !12, i64 476, !7, i64 480, !7, i64 488, !8, i64 496, !8, i64 2544, !8, i64 2800, !8, i64 3056, !7, i64 4080, !7, i64 4088, !12, i64 4096, !8, i64 4100, !12, i64 4356, !12, i64 4360, !12, i64 4364, !12, i64 4368, !12, i64 4372, !12, i64 4376, !12, i64 4380, !12, i64 4384, !12, i64 4388, !12, i64 4392, !8, i64 4400}
!12 = !{!"int", !8, i64 0}
!13 = !{!11, !12, i64 4096}
!14 = !{!11, !12, i64 476}
!15 = !{!16, !12, i64 12}
!16 = !{!"_ZTS11gamestate_t", !12, i64 0, !12, i64 4, !12, i64 8, !12, i64 12, !12, i64 16, !12, i64 20, !12, i64 24, !12, i64 28, !12, i64 32, !12, i64 36, !12, i64 40, !12, i64 44, !12, i64 48, !12, i64 52, !12, i64 56, !12, i64 60, !8, i64 64, !8, i64 4064, !7, i64 36064, !12, i64 36072, !12, i64 36076, !12, i64 36080, !12, i64 36084, !12, i64 36088, !12, i64 36092, !12, i64 36096, !12, i64 36100}
!17 = !{!11, !12, i64 460}
!18 = !{!12, !12, i64 0}
!19 = !{!16, !12, i64 20}
!20 = distinct !{!20, !21}
!21 = !{!"llvm.loop.mustprogress"}
!22 = distinct !{!22, !23}
!23 = !{!"llvm.loop.unroll.disable"}
!24 = distinct !{!24, !21}
!25 = !{!11, !12, i64 0}
!26 = !{!11, !7, i64 480}
!27 = !{!16, !12, i64 60}
!28 = !{!16, !12, i64 36096}
!29 = !{!16, !12, i64 36072}
!30 = !{!16, !12, i64 48}
!31 = !{!16, !12, i64 36084}
!32 = !{!16, !12, i64 40}
!33 = !{!16, !12, i64 36092}
!34 = !{!16, !12, i64 36088}
!35 = !{!36, !12, i64 0}
!36 = !{!"_ZTSN7state_tUt_E", !12, i64 0, !12, i64 4, !12, i64 8, !12, i64 12}
!37 = !{!36, !12, i64 4}
!38 = !{!36, !12, i64 8}
!39 = !{!36, !12, i64 12}
!40 = distinct !{!40, !21}
!41 = distinct !{!41, !23}
!42 = !{!11, !7, i64 4080}
!43 = !{!16, !12, i64 4}
!44 = !{!11, !12, i64 456}
!45 = distinct !{!45, !21}
!46 = distinct !{!46, !21}
!47 = !{i32 0, i32 2}
!48 = distinct !{!48, !21}
!49 = distinct !{!49, !21}
!50 = distinct !{!50, !21, !51}
!51 = !{!"llvm.loop.peeled.count", i32 1}
!52 = distinct !{!52, !21}
!53 = distinct !{!53, !21}
!54 = distinct !{!54, !21}
!55 = distinct !{!55, !23}
!56 = distinct !{!56, !21}
!57 = distinct !{!57, !21}
!58 = distinct !{!58, !21}
!59 = !{!16, !12, i64 0}
!60 = !{!16, !12, i64 36076}
!61 = !{!16, !12, i64 36100}
!62 = distinct !{!62, !21}
!63 = !{!16, !7, i64 36064}
!64 = !{!16, !12, i64 16}
!65 = !{!16, !12, i64 56}
!66 = distinct !{!66, !21}
!67 = !{!16, !12, i64 52}
!68 = distinct !{!68, !21}
!69 = distinct !{!69, !21}
