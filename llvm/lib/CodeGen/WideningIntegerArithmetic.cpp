//===- WideningIntegerArithmetic.cpp - Widening Integer Arithmetic Pass
//----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
/// \file
/// This file implements the new Sign Extension Elimination pass.
/// First, widening integer solutions are computed for each Value of interest
/// They hold the fillType and other relevant fields.
/// The first part of the algorithm implemented here is described in the paper
/// "Widening Integer Arithmetic, Kevin Redwine and Norman Ramsey, CC 2004"
///
///
/// A brief overview of the whole algorithm:
/// First, we compute various solutions for a given value V
/// that represent the possible widths of V for example, (i16, i32, i64)
/// and for a given width W, we have a matching fillType
///
/// A fillType indicates for an operand what it produces and accepts in its
/// upper bits
///
/// An operand has a fillType only if it is widenable, i.e,
/// if by applying the operator to a wider value,
/// we can simulate the operator applied to narrow values.
///
/// wide value: [ fillType bits ][ original bits ]
///
/// SIGN: [ b b b b b b b ][ b . . . . . . . ]
/// ZEROS: [ 0 0 0 0 0 0 0 ][ . . . . . . . . ]
/// ANYTHING: [ ? ? ? ? ? ? ? ? ][ . . . . . . . . ]
///
/// For example since the OR instruction can produce a ZERO when operands are
/// zero-filled, 8-bit OR can be implemented as a 16-bit OR.
//
/// x narrow: 1010 0101
/// y narrow: 0001 1000
/// result narrow: 1011 1101

/// zext(x) to i16: 0000 0000 1010 0101
/// zext(y) to i16: 0000 0000 0001 1000
/// -------------------
/// or result i16: 0000 0000 1011 1101

/// This clearly explains why the low bits do not change, if we have a fillType.
/// Then we can simulate the narrow operator with a wider one.

/// In terms of using a different simpler definition for the fillType,
/// we know that operators represent the truth of mathematics.
/// So, if we use the KnownBits of LLVM and see that the upper bits of the
/// operands of the OR are ZERO, then we can infer that OR will produce zero in
/// its upper bits.
///
/// Secondly, after we are done finding solutions for every value V of interest
/// we create extension plans that express how are we going to
/// rewrite a particular graph.
/// The algorithm uses a simple worklist algorithm
/// to find a suitable fillType for each value in the worklist
/// for the given lattice we are examining.
///
/// An example lattice is given below
///
///
///             g[n+1]
///            /  |   \
///      z[n+1]   |   s[n+1]
///        |     g[n]    |
///        |    / .   \  |
///        |  /   .    \ |
///       z[n]    .     s[n]
///        .      .      .
///        .      .      .
///        .      .      .
///       z[1]   g[1]   s[1]
///
/// g: represents Anything
/// s: represents Sign
/// z: represents Zeros
///
/// If we have found more than one legal fillType during the graph build.
/// We aren't sure before building the graph which one will be legal at the end.
/// So we delay the choice for later. This is handled by the PromotionFillSolver
/// and the PromoteConstraintSolver.
///
/// In order to handle graphs with cycles ie, removing extensions
/// inside loops, the algorithm tries to propagate an available fillType
/// to the operands of an instruction, that checks if we have violation
/// on operands and users constraints.
///
///
/// Finally, we use the Rebuilder to rewrite the IR
/// and remove any extensions we have proved that are legal.
///
///

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/DemandedBits.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/CodeGen/WideningIntegerArithmeticInfo.h"
#include "llvm/CodeGenTypes/MachineValueType.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Use.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/InstructionCost.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/TypeName.h"
#include "llvm/Support/TypeSize.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/ARMTargetParser.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

using namespace llvm;
using namespace llvm::PatternMatch;

#define DEBUG_TYPE "widening-integer-arithmetic"
STATISTIC(ICmpEmptyRejects, "ICmp empty-graph rejections");
STATISTIC(ICmpSolverRejects, "ICmp solver rejections");
STATISTIC(ICmpProfitRejects, "ICmp profitability rejections");

static cl::opt<bool>
    DisableWIA("disable-wia", cl::Hidden,
               cl::desc("Disable Widening Integer Arithmetic"));

#define ICMP_CONSTANT 2000
STATISTIC(NumSExtsDropped, "Number of sext instructions that were dropped");
STATISTIC(NumZExtsDropped, "Number of zext instructions that were dropped");
STATISTIC(NumTruncatesDropped,
          "Number of ISD::TRUNCATE nodes that were dropped");
STATISTIC(NumInstsRemoved,
          "Number of removed Instructions");

namespace {

TTI::TargetCostKind TTICostKind = llvm::TargetTransformInfo::TCK_CodeSize;
using FillTuple = std::tuple<IntegerFillType, IntegerFillType, IntegerFillType>;
using WIATuplesMap = DenseMap<std::pair<Instruction *, IntegerFillType>,
                              SmallVector<FillTuple, 4>>;

/// @brief
/// Holds the pending cleanups of the Allocator
/// otherwise we can get leaks.
class SolutionArena {
  struct PendingClean {
    void *Ptr;
    // function ref is not owning, so we can't use it here
    void (*Destroy)(void *);
  };

  BumpPtrAllocator Allocator;
  SmallVector<PendingClean, 16> PendingCleanups;

public:
  template <typename T, typename... Args> T *allocateSolution(Args &&...args) {
    void *Mem = Allocator.Allocate(sizeof(T), llvm::Align(alignof(T)));
    T *Sol = new (Mem) T(std::forward<Args>(args)...);
    // Save constructed items here otherwise we can get leaks
    PendingCleanups.push_back(PendingClean{
        Sol, [](void *Ptr) { std::destroy_at(static_cast<T *>(Ptr)); }});
    return Sol;
  }
  void Reset() {
    for (const PendingClean &Cleanup : llvm::reverse(PendingCleanups))
      Cleanup.Destroy(Cleanup.Ptr);

    PendingCleanups.clear();
    Allocator.Reset();
  }

  ~SolutionArena() { Reset(); }
};

template <typename T, typename... Args>
static T *allocateSolution(SolutionArena &Arena, Args &&...args) {
  return Arena.allocateSolution<T>(std::forward<Args>(args)...);
}

/// @brief This WIA class implements the type rules from the paper
/// Widening Integer Arithmetic, CC 2004, Kevin Redwine and Norman Ramsey
/// At the end we have for every instruction some solutions containing some
/// metadata The most useful ones are those two : (UpdatedWidth, fillType)
/// UpdatedWidth : the new width of the Instruction I.
/// FillType: Denotes how are the upper bits filled with that UpdatedWidth.
class WideningIntegerArithmetic {
  const TargetMachine *TM = nullptr;
  const TargetLowering *TLI = nullptr;
  const TargetTransformInfo *TTI = nullptr;
  const DataLayout *DL = nullptr;
  DemandedBits *DB = nullptr;
  LLVMContext *Ctx = nullptr;
  LoopInfo *LI = nullptr;
  DominatorTree *DT = nullptr;
  AssumptionCache *AC = nullptr;
  OptimizationRemarkEmitter *ORE = nullptr;

public:
  using FillTypeSet = ArrayRef<FillTuple>;
  static FillTypeSet getFillRules(unsigned Opcode);

private:
  // Prefer SExt if the current target prefers it,
  // Otherwise prefer ZExt.
  IntegerFillType ExtensionChoice;

  // This contains a mapping from Values to Numbers.
  // An instruction with 0 Number returns
  // !hasAppropriateType || hasNonIntegerOps --> true.
  DenseMap<const Value *, unsigned> ValueNum;

  // Which instructions we might change, meaning we can
  // change their type or remove them entierely.
  SmallPtrSet<Instruction *, 32> TouchedInsts;
  // Checks whether an Instruction in the Function F is visited and solved`
  BitVector SolvedInstructions;
  // Checks if a value is inside the worklist.
  BitVector InsideWorklist;

  // For every Value *V, holds all the available solutions
  // that we calculate with the WIA class.
  DenseMap<Value *, SmallVector<WideningIntegerSolutionInfo *, 8>>
      AvailableSolutions;

  // Run the WIA fill analyses only once, this function achieves that.
  llvm::function_ref<void()> EnsureForwardAnalysis = nullptr;

  // In some cases, we can avoid running the whole fill analysis.
  bool DeferredWiaAnalysis = false;

  // Get all the filltypes for the given Instruction.
  FillTypeSet getFillTypes(Instruction *Instr);

  // For a given Instruction cache the fillTypes so we can avoid
  // repeated queries.
  DenseMap<uint32_t, IntegerFillType> FillTypeCache;
  // Used to compute the key for the FillTypeCache.
  uint32_t makeFillKey(IntegerFillType LeftF, IntegerFillType RightF,
                       unsigned int Opc, uint8_t Flags);
  // Get the ISD opcode for this Instruction to access the fillTypes of it.
  unsigned getFillRuleKey(Instruction *I);

  // Try to find the FillType of Binop by traversing all the calculated
  // fillTypes of Binop named AvailableFillTypes. The Left and Right fillTypes
  // are what the ops of the binop have as fillTypes. Flags represent the
  // wrapFlags(nsw, nuw), that can help us choose a fillType if they are
  // present.
  IntegerFillType getOrNullFillType(Instruction *Binop,
                                    FillTypeSet AvailableFillTypes,
                                    IntegerFillType Left, IntegerFillType Right,
                                    uint8_t Flags);

  // Use a BumpPtr Allocator for efficiency when allocating WIA solutions.
  SolutionArena SolutionAllocator;

  // Use a forward analysis using the WIA paper to get all the FillTypes of
  // all the relevant instructions for the function F.
  [[maybe_unused]] bool visitWidening(Function &F);

  // Number operands of Instruction I, so we can use BitVectors to query
  // all the relevant instructions, this can help with compile time.
  void numberOperands(Instruction *I, unsigned int &ValuesSize);

  // Identify Values or Instructions that are not of interest for this
  // pass. Their type should stay the same.
  bool shouldIgnoreValue(Value *V);

  // implement the forward analysis to calculate all the FillType information
  // for all the relevant instructions.
  void processWorklist(SmallVectorImpl<Value *> &Worklist);

  // visit an Instruction from the worklist and try to calculate
  // all the relevant WIA solutions for that Instruction.
  bool solveInst(Value *PopVal, SmallVectorImpl<Value *> &Worklist);

  // Try to push Value V into the Worklist.
  void pushToWorklist(Value *V, SmallVectorImpl<Value *> &Worklist);

  // What extension the target prefers? RISC-V prefers sext over zext.
  // Most of the other targets prefer zext over sext.
  [[maybe_unused]] void setFillType(Type *IType);

  // Check if we have at least one solution for the Value V.
  bool isPartiallySolved(Value *V);

  // Checks if we are done solving Value V.
  bool isSolved(Value *V);

  // Checks if we have a solution for V for a given width.
  bool isSolvedForWidth(Value *V, unsigned int Width);

  // get the width of a value or the icmp compare width.
  unsigned short getWidthOrICmpWidth(const WideningIntegerSolutionInfo *Sol);

  // Checks if V is inside the worklist.
  bool isInWorklist(Value *V);

  // Check if we can promote Instr by existing flags and simpler rules
  // More sophisticated methos can perform better.
  bool isLegalToPromote(Instruction *Instr);

  // Try to add the GeneratedSol of V into the list of all the Solutions for V.
  // For this to happen the GeneratedSol must not be redundant given
  // the IsRedundant method defined in the WIA header file.
  bool addNonRedundant(Value *V, WideningIntegerSolutionInfo *GeneratedSol);

  // Try to add widen and narrow rules to that Instruction.
  bool closure(Instruction *Instr, SmallVectorImpl<Value *> &Worklist);

  // Solve that particular Instruction getting all available Widths
  // and all available filltypes.
  bool visitInstruction(Instruction *I, SmallVectorImpl<Value *> &Worklist);
  // Combine sols used for binops and icmps to take their operands
  // into account when solving that binop or icmp.
  bool combineSols(Instruction *Binop, SmallVectorImpl<Value *> &Worklist);

  // Try to prove no wrap flags if perform calculations with
  // a constant as op1.
  IntegerFillType tryToProveNoWrapFlagsWithConstant(Instruction *I,
                                                    IntegerFillType ResultFill);

  // Try to register the solutions of Instr and push them to the
  // Solutions vector cause other methods might need them.
  bool tryToRegisterSolutions(
      Instruction *Instr, ArrayRef<WideningIntegerSolutionInfo *> PossibleSols,
      SmallVectorImpl<WideningIntegerSolutionInfo *> &Solutions);

  // Solve a store instruction.
  bool visitStore(Instruction *Instr, SmallVectorImpl<Value *> &Worklist);

  // Solve a switch instruction.
  bool visitSwitch(Instruction *Instr, SmallVectorImpl<Value *> &Worklist);

  // Try to create sext_inreg sols for Instr at various widths.
  bool visitFill(Instruction *Instr,
                 SmallVector<WideningIntegerSolutionInfo *, 8> &Solutions);

  // Try to create Instr sols at various widths with a preferred extension.
  bool visitWiden(Instruction *Instr,
                  SmallVector<WideningIntegerSolutionInfo *, 8> &Solutions);

  // Try to create trunc sols for Instr at various narrow widths.
  bool visitNarrow(Instruction *Instr,
                   SmallVector<WideningIntegerSolutionInfo *, 8> &Solutions);

  // Try to create drop ext sols for Instr at various widths.
  bool visitDropExt(Instruction *Instr, SmallVectorImpl<Value *> &Worklist);

  // Try to create drop trunc sols for Instr at various widths.
  bool visitDropTrunc(Instruction *Instr, SmallVectorImpl<Value *> &Worklist);

  // From the paper, not implemented yet.
  bool visitSubsumeFill(Instruction *Instr) { return false; }

  // Implemented implicitly.
  bool visitSubsumeIndex(Instruction *Instr) {
    // We implement this rule implicitly
    // On every solution we consider the index n to stand
    // for all indices from n to w.
    return false;
  }

  // Create constant solutions at various legal widths.
  bool visitConstant(ConstantInt *CI);

  // Create select solutions at various legal widths.
  bool visitSelect(Instruction *Instr);

  // Create undef sols at all widths.
  bool visitUndef(Value *V);

  // Create return solution at the legal width.
  bool visitReturn(Instruction *Instr, SmallVectorImpl<Value *> &Worklist);

  // Create PHi solution only at the default width.
  // For other widths we prove legality at the other classes.
  bool visitPhi(Instruction *Instr);

  // check if N0 has a drop trunc solution then a drop ext from a zext
  // we can drop both the trunc and the zext of the load only
  // if it has range metadata.
  bool canDropTruncZext(Instruction *Instr, Value *N0, unsigned short NewWidth);

  // Clear all relevant tables.
  [[maybe_unused]] void clearTables();

  // Push all users of V into the worklist and mark them as unsolved.
  void pushUsersToWorklist(Value *V, SmallVectorImpl<Value *> &Worklist);

  // Check if the operands of PopVal are solved, this is used
  // for the forward analysis when solving to get all legal widths
  // and all the legal FillTypes for all the instructions.
  bool checkOperands(Value *PopVal, SmallVectorImpl<Value *> &Worklist);

  // Get a solution with a match at the requested width.
  // If no width is found, it creates a default sol.
  // Legality is checked at a later stage.
  // The function can return a nullptr if we didn't find a
  // solution at the requested width.
  WideningIntegerSolutionInfo *getMatchingWidthSol(
      Value *V,
      const DenseMap<Value *, SmallVector<WideningIntegerSolutionInfo *, 8>>
          &AvailableSolutions,
      unsigned short Width);

  // Check if V has no integer ops, so that this value can be rejected
  // by joining a relevant region of interest.
  bool hasNonIntegerOps(Value *V);

  // If we find an Inst that is already extended we delete our saved
  // extensions for that Inst.
  void deleteSolsForKind(Instruction *I, WIAKind Kind);

  // Check if this Instr is a Extension or a Trunc.
  bool isExtOrTrunc(Value *Instr);

  // Check if this solution is of Trunc ty.
  bool isTruncKind(WideningIntegerSolutionInfo *Solution);

  // Check if Ty is of Ptr type.
  bool isPtrType(Type *Ty);

  // Get the width of invo's return type.
  int getCallInstWidth(CallBase *CB);

  // Get the width of VI and it's fillType too.
  unsigned short getInstWidth(Value *VI, unsigned short &FillTypeWidth);

  // Get the scalar bits of that type.
  unsigned short getTypeScalarBits(Type *Ty);

  // Identify values that are sinks. These values consume a value of interest
  // But usually they don't produce a value of interest. So the users
  // of that value should not be pulled past the inspected region.
  bool isSink(Value *V);

  // Identify values that are sources. Thse values can produce
  // a value of interest. But their operands are not of interest.
  bool isSource(Value *V);

  // Look for Narrow ICmps that have constant as op1
  // and Add/Sub as Op0, those can generate redundant cast instruction.
  // On signed ICmps we try to promote those by proving no wrap on the add/sub.
  bool LookICmpRegion(Value *V);

  // Check if this region is appropriate to run the WIA analysis.
  bool isAppropriateRegionValue(Value *V);

  enum class RegionResult { Rejected, Accepted, Unknown };

  // Collect a region around the extension to possibly promote it later on.
  RegionResult
  CollectRelevantRegion(Instruction *Ext,
                        SmallPtrSetImpl<Instruction *> *Region,
                        const SmallPtrSetImpl<Instruction *> &Relevant,
                        const SmallPtrSetImpl<Instruction *> &FailedValues,
                        std::optional<unsigned> Budget = std::nullopt);

  // Put into the worklist all of the relevant regions that are of
  // interest to be solved by the WIA function.
  void buildWorklist(Function &F, SmallVectorImpl<Value *> &Worklist);

  std::vector<unsigned short> IntegerSizes = {8, 16, 32, 64};

  unsigned RegisterBitWidth = 0;

  // ZEROS to zext opcode and SIGN to sext opcode.
  unsigned getExtensionChoice(enum IntegerFillType ExtChoice);

  // Get the extension cost using TTI
  InstructionCost getExtCost(Instruction *Instr,
                             const WideningIntegerSolutionInfo *Sol,
                             unsigned short IntegerSize);

  // used to speed up the worklist loop.
  unsigned ValueToNum(const Value *V) const { return ValueNum.lookup(V); }

  // Return a default fillType for V, most of the time ANYTHING or Top.
  // This can later be refined to SIGN or ZERO.
  IntegerFillType getDefaultFillType(Value *V);

  // Print all the sols of V
  void printInstrSols(Value *V);

  // Cache KnownBits to avoid repeated queries.
  llvm::DenseMap<Value *, KnownBits> KnownBitsCache;

  // Cache known bits to avoid repeated queries on the same Value.
  void getKnownBitsCached(Value *V, KnownBits &KB);

  // Try to get the payload bits for the Value V, the bits that are
  // considered to be the data bits, not the extended or upper bits.
  short int getKnownFillTypeWidth(Value *V, KnownBits &Known);
  unsigned short getKnownFillTypeWidthCached(Value *V);

  // use KnownBits to determine a FillType
  IntegerFillType determineFill(KnownBits &K, unsigned short PayloadW);

  // Checks if V has a appropriate type for this pass
  // Or this value is not of any interest at all.
  bool hasAppropriateType(Value *V);

  // Checks if the fillType that is Produced by instruction I,
  // if it matches with what we have incoming from the IR
  // referenced as Needed.
  bool satisfiesResultFill(Instruction *I, IntegerFillType Produced,
                           IntegerFillType Needed);

  // Prefer Sign or Zeros over Anything.
  bool satisfiesFill(IntegerFillType Actual, IntegerFillType Needed);

  // Create a solution for V with width as the current bitwidth of V.
  bool createDefaultSol(Value *V);

  // Return the WIA kind for a specific opcode.
  WIAKind getKindForOpcode(unsigned Opcode);

  bool isNegativeCost(InstructionCost Cost);

  // Sometimes we can avoid the whole wia analysis,
  // try to prove that V can have an appropriate solution (width, fill)
  [[maybe_unused]] std::optional<bool> tryToGetLocalPromotionSolution(
      Value *V, unsigned Width, IntegerFillType Fill,
      llvm::function_ref<RegionResult(Instruction *)> ClassifyRegion);

  // Check if we have a solution for V at a specific Width and Fill
  [[maybe_unused]] bool hasPromotionSolution(Value *V, unsigned Width, IntegerFillType Fill);

  // For a given Instruction I with a specific result with a FillType
  // named ResultFill, produce a fill requirement for each operand
  // if we have more than one choice defer the choice for later,
  // at the PromoteConstraintSolver.
  [[maybe_unused]] bool chooseFillTuple(Instruction *I, IntegerFillType ResultFill,
                       SmallVectorImpl<IntegerFillType> &,
                       SmallVectorImpl<FillTuple> *GeneratedTuples = nullptr);

  // Try to prove nsw or nuw on a recurrence, this sometimes is possible
  // due to icmp's proving bounds on both paths false and true.
  uint8_t inferRecurrenceWrapFlags(Instruction *I, IntegerFillType ResultFill);
  // For Instruction I with result having fill ResultFill,
  // collect all the legal fillType combinations for all the operands.
  void collectLegalFillTuples(Instruction *I, IntegerFillType ResultFill,
                              SmallVectorImpl<FillTuple> &AllOpFillTuples);

  // Check if I can produce a zero-upper bits or zero fill
  // from signed operands.
  bool canProduceZeroFillFromSignedOps(Instruction *I);

  // Check if V is no negative, this can be helpful in many places.
  bool isKnownNonNeg(Value *V);

  // Get the extension cost from SrcTy to DstTy.
  [[maybe_unused]] InstructionCost getExtTTICost(Type *SrcTy, Type *DstTy, IntegerFillType Fill);
};

} // end anonymous namespace

// Try to get a Loop Carried Phi.
static Loop *getLoopCarriedPhiLoop(PHINode *Phi, LoopInfo *LI) {
  if (!LI)
    return nullptr;

  Loop *L = LI->getLoopFor(Phi->getParent());
  if (!L || L->getHeader() != Phi->getParent())
    return nullptr;

  for (unsigned I = 0, Inc = Phi->getNumIncomingValues(); I != Inc; ++I)
    if (L->contains(Phi->getIncomingBlock(I)))
      return L;

  return nullptr;
}

static bool isSupportedScalarIntType(Type *Ty) {
  if (!Ty->isIntegerTy())
    return false;
  unsigned Width = Ty->getIntegerBitWidth();
  return Width <= 64 && llvm::isPowerOf2_32(Width);
}

// From those places we can possibly remove a cast.
[[maybe_unused]] static bool isWIARoot(const Instruction &I) {
  if (isa<SExtInst, ZExtInst, TruncInst>(I) &&
      (!isSupportedScalarIntType(I.getType()) ||
       !isSupportedScalarIntType(I.getOperand(0)->getType())))
    return false;

  // Those casts don't produce a plan so there not candidates.
  if (isa<SExtInst, ZExtInst>(I) && I.hasOneUse() &&
      isa<ReturnInst>(*I.user_begin()) &&
      isa<Argument, ICmpInst>(I.getOperand(0)))
    return false;

  // if the extended icmp is used as a wide value
  // reject it for narrow, currently we only optimize
  // for the compare width of the icmp.
  if (isa<SExtInst, ZExtInst>(I) && I.hasOneUse() &&
      isa<ICmpInst>(I.getOperand(0)) &&
      isa<SExtInst, ZExtInst>(*I.user_begin()))
    return false;

  // InstCombine already handles this pattern.
  if (auto *T = dyn_cast<TruncInst>(&I); T && T->hasOneUse())
    if (auto *E = dyn_cast<CastInst>(*T->user_begin());
        E && isa<SExtInst, ZExtInst>(E) &&
        E->getDestTy()->getScalarSizeInBits() >
            T->getSrcTy()->getScalarSizeInBits())
      return false;

  // Those truncations will get rejected in later plans.
  // Reject them early here.
  // Every user must be supported by the truncation graph.
  if (isa<TruncInst>(I) && llvm::any_of(I.users(), [](const User *U) {
        return !isa<BinaryOperator, SelectInst, SExtInst, ZExtInst>(U);
      }))
    return false;

  if (isa<SExtInst, ZExtInst, TruncInst>(I))
    return true;

  const auto *ICmp = dyn_cast<ICmpInst>(&I);
  if (!ICmp)
    return false;

  Instruction *Op0 = dyn_cast<Instruction>(ICmp->getOperand(0));
  Instruction *Op1 = dyn_cast<Instruction>(ICmp->getOperand(1));
  if (!Op0 && !Op1)
    return false;

  Type *Ty = ICmp->getOperand(0)->getType();
  if (!isSupportedScalarIntType(Ty))
    return false;

  unsigned Width = Ty->getScalarSizeInBits();
  return Width > 1 && Width <= 32;
}

// Assign operand fillTypes for Instruction I.
static void assignOpFills(const Instruction *I, IntegerFillType Left,
                          IntegerFillType Right,
                          SmallVectorImpl<IntegerFillType> &OpFills) {
  // Select's operands start at index 1
  unsigned FirstOp = isa<SelectInst>(I) ? 1 : 0;
  assert(I->getNumOperands() >= FirstOp + 2 && "Expected two data operands");

  OpFills.assign(I->getNumOperands(), ANYTHING);
  OpFills[FirstOp] = Left;
  OpFills[FirstOp + 1] = Right;
}


bool WideningIntegerArithmetic::addNonRedundant(
    Value *V, WideningIntegerSolutionInfo *GeneratedSol) {
  bool WasRedundant = false;
  SmallVector<WideningIntegerSolutionInfo *, 8> &Solutions =
      AvailableSolutions[V];
  if (Solutions.size() >= 8)
    return false;

  for (const auto *It = Solutions.begin(); It != Solutions.end();) {
    int IsRedundant = (*It)->IsRedudant((*GeneratedSol));
    if (IsRedundant == -1) { // GeneratedSol is redudant
      WasRedundant = true;
      ++It;
    } else if (IsRedundant == 1) { // Sol is redudant
      It = Solutions.erase(It);
    } else { // ret == 0 no redudant found
      ++It;
    }
  }
  if (!WasRedundant) {
    AvailableSolutions[V].push_back(GeneratedSol);
    return true;
  }
  return false;
}

bool WideningIntegerArithmetic::isPtrType(Type *Ty) {
  return Ty->isPointerTy();
}

bool WideningIntegerArithmetic::isPartiallySolved(Value *V) {
  return !AvailableSolutions[V].empty();
}

bool WideningIntegerArithmetic::visitConstant(ConstantInt *CI) {
  auto &Sols = AvailableSolutions[CI];
  if (!Sols.empty())
    return false;

  static constexpr unsigned PossibleWidths[] = {1, 8, 16, 32, 64};
  unsigned FillTypeWidth;
  IntegerFillType FillCI;
  if (CI->isNegative()) {
    FillCI = SIGN;
    FillTypeWidth = CI->getValue().getSignificantBits();
  } else {
    FillCI = ZEROS;
    FillTypeWidth = CI->getValue().getActiveBits();
  }
  InstructionCost ZeroCost(0);
  for (auto Width : PossibleWidths) {
    if ((FillCI == SIGN && CI->getValue().isSignedIntN(Width)) ||
        (FillCI == ZEROS && CI->getValue().isIntN(Width))) {
      auto *Sol = allocateSolution<WIA_CONSTANT>(
          SolutionAllocator, CONSTANT_INT_OPC, CONSTANT_INT_OPC, FillCI,
          FillTypeWidth, Width, Width, ZeroCost, CI);
      Sols.push_back(Sol);
    }
  }
  return false;
}

bool WideningIntegerArithmetic::isNegativeCost(InstructionCost Cost) {
  if (!Cost.isValid())
    return false;
  return Cost < 0;
}
bool WideningIntegerArithmetic::visitUndef(Value *V) {
  if (isPartiallySolved(V)) {
    return false;
  }
  for (auto W : IntegerSizes) {
    auto *Sol = allocateSolution<WideningIntegerSolutionInfo>(
        SolutionAllocator, CONSTANT_INT_OPC, CONSTANT_INT_OPC, ANYTHING, W, W,
        W, 0, llvm::WIAK_UNKNOWN, V);
    AvailableSolutions[V].push_back(Sol);
  }
  return false;
}

bool WideningIntegerArithmetic::hasAppropriateType(Value *V) {
  auto *Ty = V->getType();
  if (auto *I = dyn_cast<Instruction>(V)) {
    switch (I->getOpcode()) {
    case Instruction::PtrToInt:
    case Instruction::InsertValue:
    case Instruction::Select:
      return isSupportedScalarIntType(Ty);

    case Instruction::FCmp:
      return false;

    case Instruction::ICmp: {
      auto *CmpI = cast<ICmpInst>(I);
      return isSupportedScalarIntType(CmpI->getOperand(0)->getType());
    }

    case Instruction::Ret: {
      auto *RI = cast<ReturnInst>(I);
      if (Value *RetVal = RI->getReturnValue())
        return isSupportedScalarIntType(RetVal->getType());
      return false;
    }

    case Instruction::FPToSI:
    case Instruction::FPToUI:
      return isSupportedScalarIntType(Ty);

    case Instruction::SIToFP:
    case Instruction::UIToFP:
    case Instruction::IntToPtr:
    case Instruction::FPExt:
    case Instruction::FPTrunc:
    case Instruction::AddrSpaceCast:
      return false;

    case Instruction::BitCast: {
      auto *CI = cast<BitCastInst>(I);
      auto *Op0 = CI->getOperand(0);
      if (isa<ExtractValueInst>(Op0) || isa<ConstantInt>(Op0)) {
        return false;
      }
      return isSupportedScalarIntType(CI->getDestTy()) &&
             isSupportedScalarIntType(CI->getSrcTy());
    }

    case Instruction::Trunc:
    case Instruction::ZExt:
    case Instruction::SExt: {
      auto *CI = cast<CastInst>(I);
      auto *SrcTy = CI->getSrcTy();
      auto *DstTy = CI->getDestTy();
      auto *Op0 = CI->getOperand(0);
      if (isa<InsertElementInst>(Op0) || isa<ExtractValueInst>(Op0) ||
          isa<ConstantInt>(Op0) || isa<CallInst>(Op0) || isa<InvokeInst>(Op0)) {
        return false;
      }
      if (SrcTy->isVectorTy() || DstTy->isVectorTy()) {
        return false;
      }
      if (llvm::isPowerOf2_32(SrcTy->getIntegerBitWidth()) &&
          llvm::isPowerOf2_32(DstTy->getIntegerBitWidth())) {
        return isSupportedScalarIntType(SrcTy) &&
               isSupportedScalarIntType(DstTy) && hasAppropriateType(Op0);
      }
      return false;
    }

    case Instruction::Load:
      return isSupportedScalarIntType(Ty);

    case Instruction::Store: {
      auto *SI = cast<StoreInst>(I);
      return isSupportedScalarIntType(SI->getValueOperand()->getType());
    }

    case Instruction::Switch:
    case Instruction::UncondBr:
    case Instruction::CondBr:
      return true;

    default:
      break;
    }
  }

  if (Ty->isIntegerTy())
    return isSupportedScalarIntType(Ty);

  return false;
}

IntegerFillType WideningIntegerArithmetic::getDefaultFillType(Value *V) {
  if (auto *I = dyn_cast<Instruction>(V)) {
    unsigned Opcode = I->getOpcode();
    switch (Opcode) {
    case Instruction::SExt:
      return llvm::SIGN;
    case Instruction::ZExt:
      return llvm::ZEROS;
    }
  }
  return llvm::ANYTHING;
}

bool WideningIntegerArithmetic::hasNonIntegerOps(Value *V) {
  if (auto *I = dyn_cast<Instruction>(V)) {
    switch (I->getOpcode()) {
    case Instruction::Load:
    case Instruction::Select:
      return !V->getType()->isIntegerTy();

    case Instruction::Store:
      return !cast<StoreInst>(V)->getValueOperand()->getType()->isIntegerTy();

    case Instruction::ICmp:
    case Instruction::PtrToInt:
    case Instruction::FPToSI:
    case Instruction::FPToUI:
      return false;

    case Instruction::Call:
    case Instruction::Invoke:
      return !I->getType()->isIntegerTy();

    case Instruction::Switch: {
      auto *SI = cast<SwitchInst>(I);
      return !SI->getCondition()->getType()->isIntegerTy();
    }

    default:
      if (isExtOrTrunc(V)) {
        return false;
      }

      // Fallback: check operand types
      for (Value *Op : I->operand_values()) {
        if (!Op->getType()->isIntegerTy() ||
            Op->getType()->isFPOrFPVectorTy()) {
          return true;
        }
      }
      break;
    }
  }

  return false;
}

bool WideningIntegerArithmetic::shouldIgnoreValue(Value *V) {
  if (V->getType()->isFloatingPointTy()) {
    return true;
  }
  if (!hasAppropriateType(V)) {
    return true;
  }
  if (auto *I = dyn_cast<Instruction>(V)) {
    auto Opc = I->getOpcode();
    switch (Opc) {
    case Instruction::GetElementPtr:
      return true;
    case Instruction::UncondBr:
    case Instruction::CondBr:
      return false;
    default:
      return hasNonIntegerOps(I);
    }
  }
  return false;
}

int WideningIntegerArithmetic::getCallInstWidth(CallBase *CB) {
  Type *RetTy = CB->getFunctionType()->getReturnType();
  if (isPtrType(RetTy)) {
    return DL->getPointerTypeSizeInBits(RetTy);
  }
  if (!RetTy->isIntegerTy()) {
    return 0;
  }
  return RetTy->getIntegerBitWidth();
}

unsigned short WideningIntegerArithmetic::getTypeScalarBits(Type *Ty) {
  if (isPtrType(Ty)) {
    return DL->getPointerTypeSizeInBits(Ty);
  }
  if (Ty->isIntegerTy()) {
    return Ty->getScalarSizeInBits();
  }
  // for cmpxchg and others that have {i64, i1} type we just return the ptr size
  // in bits. since we do not touch those instructions.
  // TODO: check if can ignore them in the future.
  return DL->getPointerSizeInBits();
}

/// Return the solution kind for an IR opcode.
WIAKind WideningIntegerArithmetic::getKindForOpcode(unsigned Opcode) {
  switch (Opcode) {
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Mul:
  case Instruction::UDiv:
  case Instruction::SDiv:
  case Instruction::URem:
  case Instruction::SRem:
  case Instruction::LShr:
  case Instruction::AShr:
  case Instruction::Shl:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor:
    return WIAK_BINOP;
  case Instruction::ZExt:
  case Instruction::SExt:
    return WIAK_DROP_EXT;
  case Instruction::Trunc:
    return WIAK_NOOP;
  case Instruction::Load:
    return WIAK_LOAD;
  case Instruction::Store:
    return WIAK_STORE;
  case Instruction::PHI:
    return WIAK_PHI;
  case Instruction::Switch:
    return WIAK_SWITCH;
  case Instruction::Ret:
    return WIAK_RET;
  case Instruction::ICmp:
    return WIAK_ICMP;
  default:
    return WIAK_UNKNOWN;
  }
}

bool WideningIntegerArithmetic::isExtOrTrunc(Value *V) {
  if (auto *I = dyn_cast<Instruction>(V)) {
    unsigned Opc = I->getOpcode();
    return Opc == Instruction::ZExt || Opc == Instruction::SExt ||
           Opc == Instruction::Trunc;
  }
  return false;
}

bool WideningIntegerArithmetic::isTruncKind(
    WideningIntegerSolutionInfo *Solution) {
  if ((Solution->getKind() == llvm::WIAK_DROP_LOCOPY ||
       Solution->getKind() == llvm::WIAK_DROP_LOIGNORE))
    return true;

  return false;
}
WideningIntegerSolutionInfo *WideningIntegerArithmetic::getMatchingWidthSol(
    Value *V,
    const DenseMap<Value *, SmallVector<WideningIntegerSolutionInfo *, 8>>
        &AvailableSolutions,
    unsigned short Width) {
  auto SolsIt = AvailableSolutions.find(V);
  if (SolsIt == AvailableSolutions.end()) {
    createDefaultSol(V);
  }
  SolsIt = AvailableSolutions.find(V);

  assert(SolsIt != AvailableSolutions.end() && "Didnt find Value on map..");
  const auto &Sols = SolsIt->getSecond();
  auto *SolIt = std::find_if(Sols.begin(), Sols.end(),
                             [&](const WideningIntegerSolutionInfo *Sol) {
                               return Sol->getUpdatedWidth() == Width;
                             });
  if (SolIt == Sols.end())
    return nullptr;

  return *SolIt;
}

static bool isExtOpcode(Instruction *Instr) {
  switch (Instr->getOpcode()) {
  case Instruction::ZExt:
  case Instruction::SExt:
    return true;
  }
  return false;
}

bool WideningIntegerArithmetic::satisfiesFill(IntegerFillType Actual,
                                              IntegerFillType Needed) {

  if (Actual == UNDEFINED || Needed == UNDEFINED)
    return false;

  if (Actual == PENDING || Needed == PENDING)
    return false;

  // a rule requiring garbage accepts any valid actual fill.
  if (Needed == ANYTHING)
    return true;

  return Actual == Needed;
}

// prefer SIGN or ZEROS over Garbage
static IntegerFillType chooseBestFill(IntegerFillType Curr,
                                      IntegerFillType Candidate) {
  if (Curr == UNDEFINED)
    return Candidate;

  if (Candidate == UNDEFINED)
    return Curr;

  if (Curr == PENDING && Candidate == PENDING)
    return PENDING;

  if (Curr == PENDING)
    return Candidate == ANYTHING ? PENDING : Candidate;

  if (Candidate == PENDING)
    return Curr == ANYTHING ? PENDING : Curr;

  if (Curr == Candidate)
    return Curr;

  if (Curr == ANYTHING)
    return Candidate;

  if (Candidate == ANYTHING)
    return Curr;

  return UNDEFINED;
}

bool WideningIntegerArithmetic::isKnownNonNeg(Value *V) {

  if (isa<ZExtInst>(V))
    return true;

  if (auto *C = dyn_cast<ConstantInt>(V))
    return !C->isNegative();

  if (auto *BO = dyn_cast<BinaryOperator>(V)) {
    if (auto *C = dyn_cast<ConstantInt>(BO->getOperand(1))) {
      if (BO->getOpcode() == Instruction::And && !C->isNegative())
        return true;

      if (BO->getOpcode() == Instruction::LShr && !C->isZero() &&
          C->getValue().ult(C->getBitWidth()))
        return true;

      if (BO->getOpcode() == Instruction::UDiv && C->getValue().ugt(1))
        return true;

      if (BO->getOpcode() == Instruction::URem &&
          (C->getValue().isStrictlyPositive() ||
           C->getValue().isMinSignedValue()))
        return true;
    }
  }

  if (auto *BO = dyn_cast<BinaryOperator>(V);
      BO && BO->getOpcode() == Instruction::Mul && BO->hasNoSignedWrap() &&
      BO->getOperand(0) == BO->getOperand(1))
    return true;

  if (auto *II = dyn_cast<IntrinsicInst>(V)) {
    switch (II->getIntrinsicID()) {
    case Intrinsic::abs: {
      if (auto *C = dyn_cast<ConstantInt>(II->getArgOperand(1));
          C && C->isOne())
        return true;
      return false;
    }
    case Intrinsic::umin: {
      if (auto *C = dyn_cast<ConstantInt>(II->getArgOperand(1));
          C && !C->isNegative())
        return true;
    }
    }
  }

  BinaryOperator *Binop = nullptr;
  Value *Start = nullptr, *Step = nullptr;
  if (auto *Phi = dyn_cast<PHINode>(V)) {
    if (!matchSimpleRecurrence(Phi, Binop, Start, Step))
      return false;
  } else if (auto *BO = dyn_cast<BinaryOperator>(V);
             BO && BO->getOpcode() == Instruction::Add &&
             BO->hasNoSignedWrap()) {
    PHINode *Phi = nullptr;
    if (!matchSimpleRecurrence(BO, Phi, Start, Step))
      return false;
    Binop = BO;
  } else {
    return false;
  }

  if (Binop->getOpcode() != Instruction::Add || !Binop->hasNoSignedWrap())
    return false;

  auto *StartC = dyn_cast<ConstantInt>(Start);
  auto *StepC = dyn_cast<ConstantInt>(Step);
  // if we have found a recurrence and it started from non negative
  // number with a non negative step and it doesn't wrap
  // we can prove no negative.
  return StartC && StepC && !StartC->isNegative() && !StepC->isNegative();
}

bool WideningIntegerArithmetic::satisfiesResultFill(Instruction *I,
                                                    IntegerFillType Produced,
                                                    IntegerFillType Needed) {
  // Needed ANYTHING, accept
  // Otherwise Produced == Needed
  if (satisfiesFill(Produced, Needed))
    return true;

  if ((Produced == ZEROS && Needed == SIGN) ||
      (Produced == SIGN && Needed == ZEROS))
    return isKnownNonNeg(I);
  return false;
};

void WideningIntegerArithmetic::getKnownBitsCached(Value *V, KnownBits &KB) {
  auto It = KnownBitsCache.find(V);
  if (It != KnownBitsCache.end()) {
    KB = It->second;
    return;
  }
  KnownBits Known = computeKnownBits(V, *DL);
  KnownBitsCache[V] = Known;
  KB = Known;
  return;
}

unsigned short
WideningIntegerArithmetic::getKnownFillTypeWidthCached(Value *V) {
  KnownBits KB;
  getKnownBitsCached(V, KB);
  unsigned short Width = getKnownFillTypeWidth(V, KB);
  return Width;
}

short int WideningIntegerArithmetic::getKnownFillTypeWidth(Value *V,
                                                           KnownBits &Known) {

  if (Known.isUnknown()) {
    auto *Ty = V->getType();
    if (isPtrType(Ty)) {
      return DL->getPointerTypeSizeInBits(Ty);
    }
    return V->getType()->getScalarSizeInBits();
  }
  if (Known.isNonNegative()) {
    return Known.countMaxActiveBits();
  }
  return Known.countMaxSignificantBits();
}

IntegerFillType
WideningIntegerArithmetic::determineFill(KnownBits &Kb,
                                         unsigned short PayloadW) {
  unsigned int N = Kb.getBitWidth();
  // NOTE: no upper bits here payload n is the same as the bitwidth.
  if (N == PayloadW) {
    return ANYTHING;
  }

  // if the max active bits are less than n,
  // we can use ZEXT here
  if (Kb.countMaxActiveBits() <= PayloadW)
    return ZEROS;

  // Similarly for signed bits.
  if (Kb.countMaxSignificantBits() <= PayloadW)
    return SIGN;

  return ANYTHING;
}

unsigned short
WideningIntegerArithmetic::getInstWidth(Value *VI,
                                        unsigned short &FillTypeWidth) {

  unsigned short InstrWidth = 0;
  if (auto *I = dyn_cast<Instruction>(VI)) {
    auto Opcode = I->getOpcode();
    switch (Opcode) {
    case Instruction::Switch: {
      auto *SI = dyn_cast<SwitchInst>(VI);
      FillTypeWidth = getKnownFillTypeWidthCached(SI->getCondition());
      InstrWidth = SI->getType()->getScalarSizeInBits();
      return InstrWidth;
    }
    case Instruction::UncondBr:
    case Instruction::CondBr: {
      FillTypeWidth = 1;
      return 1;
    }
    case Instruction::Load: {
      Type *Ty = I->getType();
      if (isPtrType(Ty)) {
        InstrWidth = DL->getPointerTypeSizeInBits(Ty);
        FillTypeWidth = InstrWidth;
      } else {
        InstrWidth = Ty->getScalarSizeInBits();
        FillTypeWidth = getKnownFillTypeWidthCached(VI);
      }
      return InstrWidth;
    }
    case Instruction::Store: {
      StoreInst *SI = dyn_cast<StoreInst>(VI);
      Value *N0 = SI->getValueOperand();
      Type *N0Ty = N0->getType();
      InstrWidth = getTypeScalarBits(N0Ty);
      FillTypeWidth = InstrWidth;
      return InstrWidth;
    }
    case Instruction::Ret: {
      auto *RI = dyn_cast<ReturnInst>(I);
      if (!RI->getReturnValue()) {
        InstrWidth = 0;
        FillTypeWidth = 0;
      } else {
        Value *N0 = RI->getReturnValue();
        InstrWidth = N0->getType()->getScalarSizeInBits();
        FillTypeWidth = getKnownFillTypeWidthCached(N0);
      }
      return InstrWidth;
    }
    case Instruction::Call: {
      auto *CI = dyn_cast<CallInst>(I);
      auto InstrWidth = getCallInstWidth(CI);
      FillTypeWidth = InstrWidth;
      return InstrWidth;
    }
    case Instruction::Invoke: {
      auto *CI = dyn_cast<InvokeInst>(I);
      auto InstrWidth = getCallInstWidth(CI);
      FillTypeWidth = InstrWidth;
      return InstrWidth;
    }
    case Instruction::Select: {
      auto *SI = dyn_cast<SelectInst>(I);
      auto *Ty = SI->getTrueValue()->getType();
      InstrWidth = getTypeScalarBits(Ty);
      FillTypeWidth = InstrWidth;
      return InstrWidth;
    }
    case Instruction::ExtractValue: {
      auto *EV = dyn_cast<ExtractValueInst>(I);
      auto *AggValue = EV->getAggregateOperand();
      auto *AggType = AggValue->getType();
      auto *Ty = ExtractValueInst::getIndexedType(AggType, EV->getIndices());
      InstrWidth = getTypeScalarBits(Ty);
      FillTypeWidth = InstrWidth;
      return InstrWidth;
    }
    case Instruction::InsertValue: {
      auto *IV = dyn_cast<InsertValueInst>(I);
      auto *AggValue = IV->getAggregateOperand();
      auto *AggType = AggValue->getType();
      auto *Ty = ExtractValueInst::getIndexedType(AggType, IV->getIndices());
      InstrWidth = getTypeScalarBits(Ty);
      FillTypeWidth = InstrWidth;
      return InstrWidth;
    }
    default: {
      auto *Ty = VI->getType();
      if (!isSupportedScalarIntType(Ty)) {
        return RegisterBitWidth;
      }
      InstrWidth = getTypeScalarBits(Ty);
      FillTypeWidth = getKnownFillTypeWidthCached(VI);
      return InstrWidth;
    }
    }
  }
  auto *Ty = VI->getType();
  InstrWidth = getTypeScalarBits(Ty);
  FillTypeWidth = InstrWidth;
  return InstrWidth;
}

bool WideningIntegerArithmetic::createDefaultSol(Value *VI) {

  if (VI == nullptr) {
    return false;
  }
  if (auto *RI = dyn_cast<ReturnInst>(VI)) {
    if (!RI->getReturnValue())
      return false;
  }

  enum WIAKind Kind = WIAK_UNKNOWN;
  AvailableSolutions.try_emplace(VI);
  if (auto *CI = dyn_cast<ConstantInt>(VI)) {
    bool CChanged = visitConstant(CI);
    return CChanged;
  }
  if (isa<UndefValue>(VI)) {
    return visitUndef(VI);
  }
  auto FillType = getDefaultFillType(VI);
  bool Changed = false;
  unsigned Opcode = 0, NewOpcode = 0;
  unsigned OldWidth = 0, UpdatedWidth = 0;
  unsigned Cost = 0;
  Value *N0;
  if (auto *Instr = dyn_cast<Instruction>(VI)) {
    Opcode = Instr->getOpcode();
    N0 = Instr->getOperand(0);
    Kind = getKindForOpcode(Opcode);

    if (isExtOpcode(Instr)) {
      Kind = WIAK_DROP_EXT;
      OldWidth = Instr->getType()->getScalarSizeInBits();
      UpdatedWidth = N0->getType()->getScalarSizeInBits();
    } else if (Opcode == Instruction::Trunc) {
      UpdatedWidth = N0->getType()->getScalarSizeInBits();
      OldWidth = Instr->getType()->getScalarSizeInBits();
      Kind = WIAK_NOOP;
    }
  }
  unsigned short FillTypeWidth = VI->getType()->getScalarSizeInBits();
  unsigned short InstrWidth;
  if (NewOpcode == 0) {
    NewOpcode = Opcode;
  }
  if (auto *SI = dyn_cast<SwitchInst>(VI)) {
    InstrWidth = getTypeScalarBits(SI->getCondition()->getType());
  } else {
    InstrWidth = getTypeScalarBits(VI->getType());
  }
  OldWidth = InstrWidth;
  // assert(AvailableSolutions[VI].size() == 0 && "Sols are not empty..\n");
  UpdatedWidth = UpdatedWidth == 0 ? InstrWidth : UpdatedWidth;
  if (auto *Icmp = dyn_cast<ICmpInst>(VI)) {
    int Op0Width;
    Value *N0 = Icmp->getOperand(0);
    auto *N0Ty = N0->getType();
    if (isPtrType(N0Ty)) {
      Op0Width = DL->getPointerTypeSizeInBits(N0Ty);
      FillTypeWidth = InstrWidth;
    } else {
      Op0Width = N0Ty->getIntegerBitWidth();
      FillTypeWidth = Op0Width;
    }
    unsigned short IcmpWidth = Op0Width;
    auto *IcmpSol = allocateSolution<WIA_ICMP>(
        SolutionAllocator, Opcode, NewOpcode, ExtensionChoice, FillTypeWidth,
        OldWidth, UpdatedWidth, 0, VI);
    IcmpSol->setUpdatedIcmpWidth(IcmpWidth);
    AvailableSolutions[VI].push_back(IcmpSol);
    Changed = true;
  } else if (Kind != WIAK_NOOP) {
    if (Kind == WIAK_DROP_EXT) {
      if (isa<LoadInst>(N0)) {
        FillType = llvm::ANYTHING;
      }
      if (!llvm::isPowerOf2_32(UpdatedWidth)) {
        Kind = WIAK_NOOP;
        auto OldWidthTemp = OldWidth;
        OldWidth = UpdatedWidth;
        UpdatedWidth = OldWidthTemp;
      }
    }
    if (Kind == WIAK_LOAD) {
      FillType = llvm::ANYTHING;
      if (isPtrType(VI->getType())) {
        FillTypeWidth = InstrWidth;
      } else {
        FillTypeWidth = VI->getType()->getIntegerBitWidth();
      }
    }
    if (auto *RI = dyn_cast<ReturnInst>(VI)) {
      auto *RetVal = RI->getReturnValue();

      if (auto *CI = dyn_cast<CastInst>(RetVal)) {
        if (isExtOpcode(CI))
          OldWidth = CI->getDestTy()->getIntegerBitWidth();
        else if (CI->getOpcode() == Instruction::Trunc)
          OldWidth = CI->getType()->getIntegerBitWidth();
      } else if (RetVal->getType()->isIntegerTy()) {
        OldWidth = RetVal->getType()->getIntegerBitWidth();
      } else {
        unsigned short FillTypeWidth = 0U;
        OldWidth = getInstWidth(RetVal, FillTypeWidth);
      }
      UpdatedWidth = OldWidth;
    }
    if (Kind == WIAK_DROP_EXT)
      FillTypeWidth = N0->getType()->getScalarSizeInBits();

    auto *DefaultSol = allocateSolution<WideningIntegerSolutionInfo>(
        SolutionAllocator, Opcode, NewOpcode, FillType, FillTypeWidth, OldWidth,
        UpdatedWidth, Cost, Kind, VI);
    if (Kind == WIAK_DROP_EXT) {
      unsigned short MatchingWidth = UpdatedWidth;
      if (isa<LoadInst>(N0)) {
        MatchingWidth = N0->getType()->getScalarSizeInBits();
      }
      auto *MatchingSol =
          getMatchingWidthSol(N0, AvailableSolutions, MatchingWidth);
      if (MatchingSol != nullptr) {
        DefaultSol->addOperand(
            getMatchingWidthSol(N0, AvailableSolutions, MatchingWidth));
        AvailableSolutions[VI].push_back(DefaultSol);
      }
    } else {
      AvailableSolutions[VI].push_back(DefaultSol);
    }
  } else {
    WideningIntegerSolutionInfo *NoopSol = allocateSolution<WIA_NOOP>(
        SolutionAllocator, Opcode, Opcode, FillType, FillTypeWidth,
        UpdatedWidth, OldWidth, 0, VI);
    auto *MatchingSol = getMatchingWidthSol(N0, AvailableSolutions, OldWidth);
    if (MatchingSol != nullptr) {
      NoopSol->addOperand(
          getMatchingWidthSol(N0, AvailableSolutions, OldWidth));
      AvailableSolutions[VI].push_back(NoopSol);
      Changed = true;
    } else {
      Changed = false;
    }
  }
  return Changed;
}

WideningIntegerArithmetic::FillTypeSet
WideningIntegerArithmetic::getFillTypes(Instruction *Instr) {
  return getFillRules(getFillRuleKey(Instr));
}

uint32_t WideningIntegerArithmetic::makeFillKey(IntegerFillType LeftF,
                                                IntegerFillType RightF,
                                                unsigned int Opc,
                                                uint8_t Flags) {
  uint32_t Key =
      (static_cast<uint32_t>(Opc) << 12) | (static_cast<uint32_t>(Flags) << 4) |
      (static_cast<uint32_t>(LeftF) << 2) | static_cast<uint32_t>(RightF);
  return Key;
}

unsigned WideningIntegerArithmetic::getFillRuleKey(Instruction *I) {
  if (auto *CI = dyn_cast<ICmpInst>(I))
    return ICMP_CONSTANT + CI->getPredicate();
  return TLI->InstructionOpcodeToISD(I->getOpcode());
}

WideningIntegerArithmetic::FillTypeSet
WideningIntegerArithmetic::getFillRules(unsigned Opcode) {
  static constexpr FillTuple Any[] = {{ANYTHING, ANYTHING, ANYTHING}};
  static constexpr FillTuple And[] = {{SIGN, SIGN, SIGN},
                                      {ZEROS, ANYTHING, ZEROS},
                                      {ANYTHING, ZEROS, ZEROS},
                                      {ANYTHING, ANYTHING, ANYTHING}};
  static constexpr FillTuple OrXor[] = {{SIGN, SIGN, SIGN},
                                        {ZEROS, ZEROS, ZEROS},
                                        {ANYTHING, ANYTHING, ANYTHING}};
  static constexpr FillTuple Signed[] = {{SIGN, SIGN, SIGN}};
  static constexpr FillTuple Unsigned[] = {{ZEROS, ZEROS, ZEROS}};
  static constexpr FillTuple Shl[] = {{ANYTHING, ZEROS, ANYTHING}};
  static constexpr FillTuple AShr[] = {{SIGN, ZEROS, SIGN}};
  static constexpr FillTuple Cmp[] = {{SIGN, SIGN, ZEROS},
                                      {ZEROS, ZEROS, ZEROS}};
  static constexpr FillTuple SignedCmp[] = {{SIGN, SIGN, ZEROS}};

  switch (Opcode) {
  case ISD::ADD:
  case ISD::SUB:
  case ISD::MUL:
    return Any;
  case ISD::AND:
    return And;
  case ISD::OR:
  case ISD::XOR:
    return OrXor;
  case ISD::SDIV:
  case ISD::SREM:
    return Signed;
  case ISD::SRA:
    return AShr;
  case ISD::UDIV:
  case ISD::UREM:
  case ISD::SRL:
    return Unsigned;
  case ISD::SHL:
    return Shl;
  case ICMP_CONSTANT + CmpInst::ICMP_EQ:
  case ICMP_CONSTANT + CmpInst::ICMP_NE:
  case ICMP_CONSTANT + CmpInst::ICMP_UGE:
  case ICMP_CONSTANT + CmpInst::ICMP_UGT:
  case ICMP_CONSTANT + CmpInst::ICMP_ULE:
  case ICMP_CONSTANT + CmpInst::ICMP_ULT:
    return Cmp;
  case ICMP_CONSTANT + CmpInst::ICMP_SGE:
  case ICMP_CONSTANT + CmpInst::ICMP_SGT:
  case ICMP_CONSTANT + CmpInst::ICMP_SLE:
  case ICMP_CONSTANT + CmpInst::ICMP_SLT:
    return SignedCmp;
  default:
    return {};
  }
}

IntegerFillType WideningIntegerArithmetic::getOrNullFillType(
    Instruction *Binop, FillTypeSet AvailableFillTypes,
    IntegerFillType LeftFillType, IntegerFillType RightFillType,
    uint8_t Flags) {
  unsigned int Opc = getFillRuleKey(Binop);

  const auto &Key = makeFillKey(LeftFillType, RightFillType, Opc, Flags);
  const auto &FillIt = FillTypeCache.find(Key);
  if (FillIt != FillTypeCache.end())
    return FillIt->second;

  IntegerFillType Result = UNDEFINED;

  for (const auto &[RequiredLeft, RequiredRight, Res] : AvailableFillTypes) {

    if (!satisfiesFill(LeftFillType, RequiredLeft))
      continue;

    if (!satisfiesFill(RightFillType, RequiredRight))
      continue;

    Result = chooseBestFill(Result, Res);
  }

  // check nowrap flags here
  IntegerFillType FlagResult = UNDEFINED;
  if (LeftFillType == RightFillType) {
    if ((Flags & 1) && LeftFillType == ZEROS)
      FlagResult = ZEROS;
    else if ((Flags & 2) && LeftFillType == SIGN)
      FlagResult = SIGN;
  }

  if (FlagResult != UNDEFINED)
    Result = chooseBestFill(Result, FlagResult);

  FillTypeCache[Key] = Result;
  return Result;
}

void WideningIntegerArithmetic::setFillType(Type *IType) {
  Type *Ty32 = IntegerType::get(*Ctx, 32);
  Type *Ty64 = IntegerType::get(*Ctx, 64);
  EVT VT32 = TLI->getValueType(*DL, Ty32);
  EVT VT64 = TLI->getValueType(*DL, Ty64);

  if (TLI->isSExtCheaperThanZExt(VT32, VT64)) {
    ExtensionChoice = SIGN;
  } else {
    ExtensionChoice = ZEROS;
  }
}

bool WideningIntegerArithmetic::visitSelect(Instruction *Instr) {
  auto *SI = dyn_cast<SelectInst>(Instr);
  auto *TrueVal = SI->getTrueValue();
  auto *FalseVal = SI->getFalseValue();
  auto &Sols0 = AvailableSolutions[TrueVal];
  auto &Sols1 = AvailableSolutions[FalseVal];

  if (Sols0.empty() || Sols1.empty())
    return false;

  int TrueValPayLoad = getKnownFillTypeWidthCached(TrueVal);
  int FalseValPayLoad = getKnownFillTypeWidthCached(FalseVal);
  int BiggestPayload = std::max(TrueValPayLoad, FalseValPayLoad);
  auto Opcode = SI->getOpcode();
  auto OldWidth = SI->getType()->getScalarSizeInBits();
  bool Changed = false;
  for (auto *LeftSol : Sols0) {
    auto LeftFill = LeftSol->getFillType();
    for (auto *RightSol : Sols1) {
      if (LeftFill != RightSol->getFillType())
        continue;

      auto RightSolWidth = RightSol->getUpdatedWidth();
      if (LeftSol->getUpdatedWidth() != RightSolWidth)
        continue;

      auto *SISol = allocateSolution<WIA_SELECT>(
          SolutionAllocator, Opcode, Opcode, LeftFill, BiggestPayload, OldWidth,
          RightSolWidth, std::max(LeftSol->getCost(), RightSol->getCost()),
          Instr);
      Changed |= addNonRedundant(Instr, SISol);
    }
  }
  return Changed;
}

unsigned short WideningIntegerArithmetic::getWidthOrICmpWidth(
    const WideningIntegerSolutionInfo *Sol) {
  unsigned short Width;
  if (const auto *ICmpSol = dyn_cast<WIA_ICMP>(Sol)) {
    Width = ICmpSol->getUpdatedIcmpWidth();
  } else
    Width = Sol->getUpdatedWidth();
  return Width;
}

bool WideningIntegerArithmetic::isSolvedForWidth(Value *V, unsigned int Width) {
  auto It = AvailableSolutions.find(V);
  if (It == AvailableSolutions.end())
    return false;

  bool IsICmpOp = isa<ICmpInst>(V);
  for (auto *Sol : It->second) {
    if (IsICmpOp && Sol->getUpdatedWidth() == Width)
      return true;
    if (getWidthOrICmpWidth(Sol) == Width)
      return true;
  }
  return false;
}

bool WideningIntegerArithmetic::visitStore(Instruction *Instr,
                                           SmallVectorImpl<Value *> &Worklist) {
  SmallVector<WideningIntegerSolutionInfo *, 8> Sols;
  Value *VI = Instr;
  StoreInst *SI = dyn_cast<StoreInst>(Instr);
  Value *N0 = SI->getValueOperand();
  if (!N0->getType()->isIntegerTy()) {
    // we don't care about those stores.
    return false;
  }
  unsigned short Width = N0->getType()->getIntegerBitWidth();
  if (!isSolvedForWidth(N0, Width)) {
    pushToWorklist(N0, Worklist);
    return false;
  }

  auto N0SolsIt = AvailableSolutions.find(N0);
  const auto &N0Sols = N0SolsIt->getSecond();
  bool Changed = false;
  unsigned short N0Bits = N0->getType()->getScalarSizeInBits();
  for (const WideningIntegerSolutionInfo *Sol : N0Sols) {
    if (Sol->getUpdatedWidth() != N0Bits) {
      continue;
    }
    InstructionCost Cost = Sol->getCost();
    if (isNegativeCost(Cost)) {
      Cost = InstructionCost(0);
    }
    auto *StoreSol = allocateSolution<WIA_STORE>(
        SolutionAllocator, Instr->getOpcode(), Instr->getOpcode(),
        Sol->getFillType(), Sol->getFillTypeWidth(), N0Bits, N0Bits, Cost, VI);
    Changed |= addNonRedundant(VI, StoreSol);
    if (Changed) {
      break;
    }
  }
  return Changed;
}

void WideningIntegerArithmetic::numberOperands(Instruction *I,
                                               unsigned int &ValuesSize) {
  auto Ops = I->operand_values();
  auto Opc = I->getOpcode();
  if (Opc == Instruction::CondBr || Opc == Instruction::UncondBr)
    return;

  for (Value *V : Ops) {
    if (isa<BasicBlock>(V)) {
      continue;
    }
    Type *VTy = V->getType();
    if (!VTy->isIntegerTy()) {
      continue;
    }
    if (isa<ExtractValueInst>(V)) {
      continue;
    }

    auto Num = ValueToNum(V);
    if (Num == 0) {
      if (auto *C = dyn_cast<ConstantInt>(V)) {
        visitConstant(C);
      }
      ++ValuesSize;
      ValueNum.insert({V, ValuesSize});
    }
  }
}

bool WideningIntegerArithmetic::LookICmpRegion(Value *V) {
  auto *ICmp = dyn_cast<ICmpInst>(V);
  if (!ICmp)
    return false;

  Type *OpTy = ICmp->getOperand(0)->getType();
  // Look for narrow integer comparisons.
  if (!isSupportedScalarIntType(OpTy))
    return false;

  unsigned Width = OpTy->getScalarSizeInBits();
  return Width > 1 && Width <= 32;
}

bool WideningIntegerArithmetic::isSource(Value *V) {
  if (isa<LoadInst>(V) || isa<Argument>(V) || isa<ConstantInt>(V) ||
      isa<UndefValue>(V) || isa<PoisonValue>(V)) {
    return true;
  }
  if (auto *CB = dyn_cast<CallBase>(V)) {
    return CB->getType()->isIntegerTy();
  }

  return false;
}

bool WideningIntegerArithmetic::isSink(Value *V) {
  return isa<StoreInst>(V) || isa<ReturnInst>(V) || isa<SwitchInst>(V) ||
         isa<CondBrInst>(V) || isa<CallBase>(V) || isa<GetElementPtrInst>(V) ||
         isa<IntToPtrInst>(V) || isa<ICmpInst>(V);
}

bool WideningIntegerArithmetic::isInWorklist(Value *V) {
  auto VNum = ValueToNum(V);
  auto IsVisited = InsideWorklist.test(VNum);
  return IsVisited;
}

void WideningIntegerArithmetic::pushToWorklist(
    Value *V, SmallVectorImpl<Value *> &Worklist) {
  if (auto *I = dyn_cast<Instruction>(V)) {
    if (!TouchedInsts.contains(I)) {
      return;
    }
  }
  auto VNum = ValueToNum(V);
  if (InsideWorklist.test(VNum) == false) {
    InsideWorklist.set(VNum);
    Worklist.push_back(V);
  }
}

bool WideningIntegerArithmetic::isAppropriateRegionValue(Value *V) {
  return (hasAppropriateType(V) && !hasNonIntegerOps(V)) ||
         isa<CondBrInst, UncondBrInst>(V);
}

WideningIntegerArithmetic::RegionResult
WideningIntegerArithmetic::CollectRelevantRegion(
    Instruction *Ext, SmallPtrSetImpl<Instruction *> *Region,
    const SmallPtrSetImpl<Instruction *> &Relevant,
    const SmallPtrSetImpl<Instruction *> &FailedValues,
    std::optional<unsigned> Budget) {

  SmallVector<Value *, 32> Worklist;
  SmallPtrSet<Value *, 32> Visited;

  auto AddAppropriateInst = [&](Value *V) {
    if (auto *I = dyn_cast<Instruction>(V); I && FailedValues.contains(I))
      return false;

    if (auto *I = dyn_cast<Instruction>(V); I && Relevant.contains(I))
      return true;

    if (Visited.count(V))
      return true;

    if (isAppropriateRegionValue(V)) {
      Worklist.push_back(V);
      Visited.insert(V);
      return true;
    }
    return false;
  };

  if (!isAppropriateRegionValue(Ext->getOperand(0)))
    return RegionResult::Rejected;

  if (!AddAppropriateInst(Ext))
    return RegionResult::Rejected;

  while (!Worklist.empty()) {

    if (Budget) {
      if (*Budget == 0)
        return RegionResult::Unknown;
      --*Budget;
    }
    Value *V = Worklist.pop_back_val();

    if (auto *I = dyn_cast<Instruction>(V)) {
      if (I->isAtomic())
        return RegionResult::Rejected;
      if (Region)
        Region->insert(I);
    }

    bool IsSource = isSource(V);
    bool IsSink = isSink(V);
    // if it's an inside node or an icmp inst inspect the ops.
    if ((!IsSource && !IsSink) || LookICmpRegion(V)) {
      if (auto *I = dyn_cast<Instruction>(V)) {
        for (auto &U : I->operands()) {
          if (!AddAppropriateInst(U)) {
            return RegionResult::Rejected;
          }
        }
      }
    }
    // Add the appropriate users of the source
    // to the worklist.
    if (!IsSink || IsSource) {
      if (!V->hasUseList())
        continue;

      for (Use &U : V->uses()) {
        if (!AddAppropriateInst(U.getUser())) {
          return RegionResult::Rejected;
        }
      }
    }
  }
  return RegionResult::Accepted;
}

void WideningIntegerArithmetic::buildWorklist(
    Function &F, SmallVectorImpl<Value *> &Worklist) {
  if (llvm::none_of(instructions(F), [&](Instruction &I) {
        return isExtOpcode(&I) || LookICmpRegion(&I);
      }))
    return;

  ReversePostOrderTraversal<Function *> RPOT(&F);

  unsigned ValuesSize = 0;
  SmallVector<Instruction *, 8> Seeds;
  for (auto &B : RPOT) {
    for (Instruction &I : *B) {
      if (isAppropriateRegionValue(&I)) {
        ValuesSize += ValueNum.try_emplace(&I, ValuesSize + 1).second;
        numberOperands(&I, ValuesSize);
      }
      if (isExtOpcode(&I) || LookICmpRegion(&I))
        Seeds.push_back(&I);
    }
  }

  if (Seeds.empty())
    return;

  SmallPtrSet<Instruction *, 32> Relevant;
  SmallPtrSet<Instruction *, 32> FailedValues;
  // Start from every extension and try to build the relevant region
  // that we need to solve for fillTypes.
  for (Instruction *Ext : Seeds) {
    if (Relevant.contains(Ext))
      continue;

    SmallPtrSet<Instruction *, 32> Region;
    if (CollectRelevantRegion(Ext, &Region, Relevant, FailedValues) !=
        RegionResult::Accepted) {
      FailedValues.insert(Ext);
      continue;
    }
    Relevant.insert_range(Region);
  }

  if (Relevant.empty())
    return;

  SmallVector<Instruction *, 32> RelevantVec;
  for (BasicBlock *BB : RPOT)
    for (Instruction &I : *BB)
      if (Relevant.contains(&I))
        RelevantVec.push_back(&I);

  if (RelevantVec.size() != Relevant.size()) {
    DenseMap<Instruction *, unsigned> RPOIndex;
    unsigned Rank = 0;
    for (BasicBlock *BB : RPOT)
      for (Instruction &I : *BB)
        RPOIndex[&I] = ++Rank;

    RelevantVec.assign(Relevant.begin(), Relevant.end());
    llvm::sort(RelevantVec, [&](Instruction *A, Instruction *B) {
      return RPOIndex[A] < RPOIndex[B];
    });
  }

  InsideWorklist.resize(ValuesSize + 1);
  SolvedInstructions.resize(ValuesSize + 1);
  // Worklist is LIFO, so we push in reverse RPO.
  // Ops are getting processed before their users.
  for (Instruction *I : llvm::reverse(RelevantVec)) {
    TouchedInsts.insert(I);
    pushToWorklist(I, Worklist);
  }

  AvailableSolutions.reserve(ValuesSize);
  KnownBitsCache.reserve(ValuesSize);
}

bool WideningIntegerArithmetic::isSolved(Value *V) {
  return SolvedInstructions.test(ValueToNum(V));
}

void WideningIntegerArithmetic::deleteSolsForKind(Instruction *I,
                                                  WIAKind Kind) {
  auto SolsIt = AvailableSolutions.find(I);
  if (SolsIt != AvailableSolutions.end()) {
    auto &Sols = SolsIt->getSecond();
    Sols.erase(std::remove_if(Sols.begin(), Sols.end(),
                              [&](const WideningIntegerSolutionInfo *Sol) {
                                return Sol->getKind() == Kind;
                              }),
               Sols.end());
  }
}

bool WideningIntegerArithmetic::checkOperands(
    Value *PopVal, SmallVectorImpl<Value *> &Worklist) {
  auto *I = dyn_cast<Instruction>(PopVal);
  if (!I || isa<CondBrInst, UncondBrInst>(I))
    return true;

  bool OpsSolved = true;
  if (isExtOpcode(I)) {
    auto *Op = I->getOperand(0);
    // Don't try to widen already widened loads
    if (auto *LI = dyn_cast<LoadInst>(Op)) {
      deleteSolsForKind(LI, WIAK_WIDEN);
    }
    // Similarly for truncs if they are already widened
    if (auto *TI = dyn_cast<TruncInst>(Op)) {
      auto *InnerOp = TI->getOperand(0);
      if (auto *LI = dyn_cast<LoadInst>(InnerOp)) {
        deleteSolsForKind(LI, WIAK_WIDEN);
      }
    }
  }

  for (Value *V : I->operand_values()) {
    if (!V->getType()->isIntegerTy() || isa<BasicBlock>(V))
      continue;

    if (isa<ExtractValueInst>(V))
      createDefaultSol(V);

    if (auto *Gep = dyn_cast<GetElementPtrInst>(V)) {
      for (auto &Gop : Gep->operands()) {
        Value *GopVal = Gop.get();
        if (hasAppropriateType(GopVal)) {
          pushToWorklist(GopVal, Worklist);
        }
      }
    }
    bool HasApprTy = hasAppropriateType(V);
    unsigned short ReqWidth = V->getType()->getScalarSizeInBits();

    bool WidthSolved =
        getMatchingWidthSol(V, AvailableSolutions, ReqWidth) != nullptr;
    bool PartiallySolved = isPartiallySolved(V);
    if (!HasApprTy && !PartiallySolved) {
      createDefaultSol(V);
      OpsSolved = false;
    } else if ((!PartiallySolved || !WidthSolved) && HasApprTy) {
      if (!PartiallySolved) {
        createDefaultSol(V);
      }

      if (!isInWorklist(V)) {
        pushToWorklist(V, Worklist);
      }
      OpsSolved = false;
    }
  }
  return OpsSolved;
}

void WideningIntegerArithmetic::pushUsersToWorklist(
    Value *V, SmallVectorImpl<Value *> &Worklist) {

  if (auto *I = dyn_cast<Instruction>(V)) {
    for (auto *U : I->users()) {
      if (U == I) {
        continue;
      }
      pushToWorklist(U, Worklist);
      // We need to reset here because we might need to resolve
      // the user we just pushed to the worklist.
      SolvedInstructions.reset(ValueToNum(U));
    }
  }
}

void WideningIntegerArithmetic::clearTables() {
  EnsureForwardAnalysis = nullptr;
  DeferredWiaAnalysis = false;
  AvailableSolutions.clear();
  SolvedInstructions.clear();
  InsideWorklist.clear();
  ValueNum.shrink_and_clear();
  KnownBitsCache.clear();
  SolutionAllocator.Reset();
  TouchedInsts.clear();
  FillTypeCache.clear();
}

IntegerFillType WideningIntegerArithmetic::tryToProveNoWrapFlagsWithConstant(
    Instruction *I, IntegerFillType ResultFill) {

  ConstantInt *CI = cast<ConstantInt>(I->getOperand(1));
  APInt C = CI->getValue();
  bool Overflows = false;

  switch (I->getOpcode()) {
  case Instruction::Add: {
    if (auto *ZExt = dyn_cast<ZExtInst>(I->getOperand(0))) {
      unsigned SrcBits = ZExt->getSrcTy()->getIntegerBitWidth();
      APInt Max = APInt::getLowBitsSet(C.getBitWidth(), SrcBits);
      if (Max.ule(~C))
        return ZEROS;
    }

    KnownBits Known0;
    getKnownBitsCached(I->getOperand(0), Known0);
    APInt MaxValue = Known0.getMaxValue();
    (void)MaxValue.uadd_ov(C, Overflows);
    if (!Overflows) {
      // TODO: this is not safe if the graph gets rejected later
      // Can we make it safe somehow?
      // I->setHasNoUnsignedWrap(true);
      return ZEROS;
    }
    bool MinSignedOverflow = false;
    bool MaxSignedOVerflow = false;
    APInt Known0SignedMin = Known0.getSignedMinValue();
    APInt Known0SignedMax = Known0.getSignedMaxValue();
    (void)Known0SignedMin.sadd_ov(C, MinSignedOverflow);
    (void)Known0SignedMax.sadd_ov(C, MaxSignedOVerflow);
    if (!MinSignedOverflow && !MaxSignedOVerflow) {
      // FIXME: this is not safe if the graph gets rejected later
      // Can we make it safe somehow?
      // I->setHasNoSignedWrap(true);
      return SIGN;
    }
    return UNDEFINED;
  }
  case Instruction::Sub: {

    KnownBits Known0;
    if (ResultFill == ZEROS) {
      getKnownBitsCached(I->getOperand(0), Known0);
      APInt MinOp0 = Known0.getMinValue();
      bool MinUOverflow = false;
      (void)MinOp0.usub_ov(C, MinUOverflow);
      if (!MinUOverflow)
        return ZEROS;
      return UNDEFINED;
    }
    // TODO: add support for SIGN.
    return UNDEFINED;
  }
  case Instruction::Mul: {
    return UNDEFINED;
  }
  case Instruction::Shl: {
    auto *Amnt = cast<ConstantInt>(I->getOperand(1));
    uint64_t ShiftAmnt = Amnt->getZExtValue();
    auto BitWidth = I->getType()->getScalarSizeInBits();
    if (ShiftAmnt >= BitWidth)
      return UNDEFINED;

    if (ResultFill != ZEROS && ResultFill != SIGN)
      return UNDEFINED;

    if ((ResultFill == ZEROS && I->hasNoUnsignedWrap()) ||
        (ResultFill == SIGN && I->hasNoSignedWrap()))
      return ResultFill;

    KnownBits KnownOp0;
    getKnownBitsCached(I->getOperand(0), KnownOp0);
    if (ResultFill == ZEROS) {
      if (ShiftAmnt <= KnownOp0.countMinLeadingZeros()) {
        return ZEROS;
      }
      if (DeferredWiaAnalysis && EnsureForwardAnalysis)
        EnsureForwardAnalysis();

      auto It = AvailableSolutions.find(I->getOperand(0));
      if (It == AvailableSolutions.end())
        return UNDEFINED;

      return llvm::any_of(It->second,
                          [&](const auto *Sol) {
                            auto Width = Sol->getWidth();
                            auto Payload = Sol->getFillTypeWidth();
                            return Sol->getFillType() == ZEROS &&
                                   Payload <= Width &&
                                   ShiftAmnt <=
                                       static_cast<uint64_t>(Width - Payload);
                          })
                 ? ZEROS
                 : UNDEFINED;
    }
    if (ResultFill == SIGN) {
      if (ShiftAmnt < KnownOp0.countMinSignBits()) {
        return SIGN;
      }
    }
    return UNDEFINED;
  }
  default:
    return UNDEFINED;
  }
  return UNDEFINED;
}

bool WideningIntegerArithmetic::canProduceZeroFillFromSignedOps(
    Instruction *I) {
  unsigned Opc = I->getOpcode();
  if ((Opc != Instruction::Add && Opc != Instruction::Sub) ||
      !I->getType()->isIntegerTy())
    return false;

  unsigned Width = I->getType()->getIntegerBitWidth();
  unsigned UpperWidth = Width + 2;

  SimplifyQuery SQ(*DL, DT, AC, I);
  ConstantRange L = computeConstantRange(I->getOperand(0), true, SQ);
  if (L.isEmptySet() ||
      (Opc == Instruction::Add && L.getSignedMin().isMinSignedValue()))
    return false;

  ConstantRange R = computeConstantRange(I->getOperand(1), true, SQ);

  if (R.isEmptySet())
    return false;

  APInt LMin = L.getSignedMin().sext(UpperWidth);
  APInt Min = Opc == Instruction::Add
                  ? LMin + R.getSignedMin().sext(UpperWidth)
                  : LMin - R.getSignedMax().sext(UpperWidth);

  if (Min.isNegative())
    return false;

  APInt LMax = L.getSignedMax().sext(UpperWidth);
  APInt Max = Opc == Instruction::Add
                  ? LMax + R.getSignedMax().sext(UpperWidth)
                  : LMax - R.getSignedMin().sext(UpperWidth);
  // i8 smax(x, -1) belongs to [-1, 127]
  // adding 1 and we have belongs to [0, 128] that fits within [0, 255]
  // so it can produce a zero fill.
  // Similarly for sub.
  APInt UnsignedMax = APInt::getMaxValue(Width).zext(UpperWidth);
  return Max.sle(UnsignedMax);
}

uint8_t WideningIntegerArithmetic::inferRecurrenceWrapFlags(
    Instruction *I, IntegerFillType ResultFill) {

  auto SimpleRange = [](Value *V) {
    if (auto *C = dyn_cast<ConstantInt>(V))
      return ConstantRange(C->getValue());

    unsigned Width = V->getType()->getIntegerBitWidth();

    if (auto *Ext = dyn_cast<CastInst>(V); Ext && isa<ZExtInst, SExtInst>(V)) {
      ConstantRange SrcRange =
          ConstantRange::getFull(Ext->getSrcTy()->getIntegerBitWidth());

      return isa<ZExtInst>(Ext) ? SrcRange.zeroExtend(Width)
                                : SrcRange.signExtend(Width);
    }
    return ConstantRange::getFull(Width);
  };
  auto *Phi = dyn_cast<PHINode>(I->getOperand(0));
  if (!Phi && I->getOpcode() == Instruction::Add)
    Phi = dyn_cast<PHINode>(I->getOperand(1));

  if (!Phi)
    return 0;

  Value *StepV = I->getOperand(I->getOperand(0) == Phi ? 1 : 0);
  ConstantInt *Step = dyn_cast<ConstantInt>(StepV);
  if (!Step)
    return 0;

  Loop *L = getLoopCarriedPhiLoop(Phi, LI);
  if (!L)
    return 0;

  if (Phi->getNumIncomingValues() != 2)
    return 0;

  unsigned BackIdx = Phi->getIncomingValue(0) == I ? 0 : 1;

  if (Phi->getIncomingValue(BackIdx) != I ||
      !L->contains(Phi->getIncomingBlock(BackIdx)) ||
      L->contains(Phi->getIncomingBlock(1 - BackIdx)) ||
      !isa<ConstantInt>(Phi->getIncomingValue(1 - BackIdx)))
    return 0;

  auto *Header = L->getHeader();
  auto *BI = dyn_cast<CondBrInst>(Header->getTerminator());
  if (!BI)
    return 0;

  auto *Cmp = dyn_cast<ICmpInst>(BI->getCondition());
  if (!Cmp || !Cmp->getOperand(0)->getType()->isIntegerTy())
    return 0;

  CmpInst::Predicate Pred = Cmp->getPredicate();
  Value *LHS = Cmp->getOperand(0);
  Value *RHS = Cmp->getOperand(1);

  // The Phi must be in that specific comparison.
  if (!match(LHS, m_ZExtOrSExtOrSelf(m_Specific(Phi)))) {
    if (!match(RHS, m_ZExtOrSExtOrSelf(m_Specific(Phi))))
      return 0;

    std::swap(LHS, RHS);
    Pred = CmpInst::getSwappedPredicate(Pred);
  }

  if (!DT || !L->contains(I->getParent()) ||
      BI->getSuccessor(0) == BI->getSuccessor(1))
    return 0;

  // Example CFG:
  //
  //   header: %cond = icmp ult i8 %phi, 100
  //           br i1 %cond, label %body, label %exit
  //                        / true             \ false
  //                       body                 exit
  //                        |
  //               for.inc: %next = add i8 %phi, 1
  //                 |
  //               br 1% cond, label %body, label %exit
  //
  // BI is the conditional branch in header.
  // BI->getParent()     == header
  // BI->getSuccessor(0) == body  (true edge)
  // BI->getSuccessor(1) == exit  (false edge)
  //
  // I is the arithmetic instruction (%next in this example).
  // I->getParent() == for.inc: the block containing I.
  //
  // Require the selected edge to dominate I's block: every path from entry
  // to I's block must cross that edge. The blocks need not be adjacent.
  // Here, header -> body establishes %phi < 100 before the add.
  // If the false edge dominates instead, invert Pred before computing
  // the allowed range.
  bool OnTrue = DT->dominates(BasicBlockEdge(Header, BI->getSuccessor(0)),
                              I->getParent());

  bool OnFalse = DT->dominates(BasicBlockEdge(Header, BI->getSuccessor(1)),
                               I->getParent());

  if (!OnTrue && !OnFalse)
    return 0;

  if (!OnTrue)
    Pred = CmpInst::getInversePredicate(Pred);

  ConstantRange AllowedRegion =
      ConstantRange::makeAllowedICmpRegion(Pred, SimpleRange(RHS));

  ConstantRange CorrectRegion = AllowedRegion.intersectWith(SimpleRange(LHS));

  unsigned PhiWidth = Phi->getType()->getIntegerBitWidth();
  if (CorrectRegion.getBitWidth() != PhiWidth)
    CorrectRegion = CorrectRegion.truncate(PhiWidth);

  unsigned RequiredFlag = ResultFill == SIGN
                              ? OverflowingBinaryOperator::NoSignedWrap
                              : OverflowingBinaryOperator::NoUnsignedWrap;

  ConstantRange SafeRange = ConstantRange::makeExactNoWrapRegion(
      cast<BinaryOperator>(I)->getOpcode(), Step->getValue(), RequiredFlag);
  if (SafeRange.contains(CorrectRegion))
    return RequiredFlag;

  return 0;
}

void WideningIntegerArithmetic::collectLegalFillTuples(
    Instruction *I, IntegerFillType ResultFill,
    SmallVectorImpl<FillTuple> &AllOpFillTuples) {

  AllOpFillTuples.clear();
  std::optional<bool> IsINonNeg;

  auto AddTuple = [&](IntegerFillType L, IntegerFillType R,
                      IntegerFillType Produced) {
    if (!satisfiesFill(Produced, ResultFill)) {
      // SExt and ZExt can produce the same upper bits
      // if I is no negative.
      bool IsDiffFill = (Produced == ZEROS && ResultFill == SIGN) ||
                        (Produced == SIGN && ResultFill == ZEROS);
      if (!IsDiffFill)
        return;

      if (!IsINonNeg.has_value())
        IsINonNeg = isKnownNonNeg(I);

      if (!*IsINonNeg)
        return;
    }
    FillTuple T = {L, R, Produced};
    if (!llvm::is_contained(AllOpFillTuples, T))
      AllOpFillTuples.emplace_back(T);
  };

  // if an earlier ICmp proves that the add can stay
  // positive even when the constant is negative
  // we can add a FillType
  // because the resultFill can stay ZEROS.
  if (ResultFill == ZEROS && I->getOpcode() == Instruction::Add) {
    if (auto *C = dyn_cast<ConstantInt>(I->getOperand(1));
        C && C->getValue().isNegative() &&
        isImpliedByDomCondition(
            CmpInst::ICMP_UGE, I->getOperand(0),
            ConstantInt::get(I->getContext(), -C->getValue()), I, *DL)
            .value_or(false))
      AddTuple(ZEROS, SIGN, ZEROS);
  }

  if (I->getOpcode() == Instruction::Select) {
    AddTuple(SIGN, SIGN, SIGN);
    AddTuple(ZEROS, ZEROS, ZEROS);
    AddTuple(ANYTHING, ANYTHING, ANYTHING);
  }

  if (auto *II = dyn_cast<IntrinsicInst>(I)) {
    switch (II->getIntrinsicID()) {
    case Intrinsic::umin:
    case Intrinsic::umax:
      AddTuple(ZEROS, ZEROS, ZEROS);
      // Sext preserves unsigned ordering.
      AddTuple(SIGN, SIGN, SIGN);
      break;
    case Intrinsic::smin:
    case Intrinsic::smax:
      AddTuple(SIGN, SIGN, SIGN);
      break;
    }
  }

  switch (I->getOpcode()) {
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Mul: {
    if (I->hasNoUnsignedWrap() && ResultFill == ZEROS) {
      AddTuple(ZEROS, ZEROS, ZEROS);
    }
    if (I->hasNoSignedWrap() && ResultFill == SIGN) {
      AddTuple(SIGN, SIGN, SIGN);
    }
    break;
  }
  case Instruction::Shl: {
    if (ResultFill == ZEROS && I->hasNoUnsignedWrap()) {
      AddTuple(ZEROS, ZEROS, ZEROS);
    }
    if (ResultFill == SIGN && I->hasNoSignedWrap()) {
      AddTuple(SIGN, ZEROS, SIGN);
    }
    if (ResultFill == ZEROS && I->hasNoSignedWrap() &&
        isKnownNonNeg(I->getOperand(0))) {
      AddTuple(ZEROS, ZEROS, ZEROS);
    }
    break;
  }
  default:
    break;
  }

  const auto &Rules = getFillTypes(I);
  for (const auto &[L, R, Res] : Rules) {
    // Res comes from the Rules of the paper
    AddTuple(L, R, Res);
  }

  switch (I->getOpcode()) {
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Shl:
    break;
  case Instruction::Mul:
    if (ResultFill == SIGN)
      return;
    break;
  default:
    return;
  }

  uint8_t WrapFlags = 0;
  switch (I->getOpcode()) {
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Mul: {
    if (ResultFill == SIGN || ResultFill == ZEROS) {
      WrapFlags = cast<OverflowingBinaryOperator>(I)->getNoWrapKind();
      unsigned RequiredFlag = ResultFill == SIGN
                                  ? OverflowingBinaryOperator::NoSignedWrap
                                  : OverflowingBinaryOperator::NoUnsignedWrap;

      if (!(WrapFlags & RequiredFlag))
        WrapFlags |= inferRecurrenceWrapFlags(I, ResultFill);
    }

    if (ResultFill == ZEROS && (WrapFlags & 1)) // NUW
      AddTuple(ZEROS, ZEROS, ZEROS);

    if (ResultFill == SIGN && (WrapFlags & 2)) // NSW
      AddTuple(SIGN, SIGN, SIGN);

    break;
  }
  }
  unsigned Opcode = I->getOpcode();
  if (ResultFill == ZEROS &&
      (Opcode == Instruction::Add || Opcode == Instruction::Sub ||
       Opcode == Instruction::Mul) &&
      (I->hasNoSignedWrap() || (WrapFlags & 2)) && !IsINonNeg.has_value())
    IsINonNeg = isKnownNonNeg(I);

  // SIGN ops  + NSW add/sub/mul -> SIGN Result
  switch (I->getOpcode()) {
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Mul: {
    if (ResultFill == ZEROS && (I->hasNoSignedWrap() || (WrapFlags & 2)) &&
        *IsINonNeg)
      AddTuple(SIGN, SIGN, SIGN);
    break;
  }
  default:
    break;
  }
  switch (I->getOpcode()) {
  case Instruction::Sub: {
    // sub with nuw can have zero tuples if all ops are proven non negative.
    if (ResultFill == ZEROS && (I->hasNoSignedWrap() || (WrapFlags & 2)) &&
        !llvm::is_contained(AllOpFillTuples, FillTuple{ZEROS, ZEROS, ZEROS}) &&
        *IsINonNeg && isKnownNonNeg(I->getOperand(0)) &&
        isKnownNonNeg(I->getOperand(1))) {
      AddTuple(ZEROS, ZEROS, ZEROS);
    }
    break;
  }
  case Instruction::Add:
  case Instruction::Mul: {
    // Add and Sub with nsw and no negative ops can have a zero fillTuple.
    if (ResultFill == ZEROS && (I->hasNoSignedWrap() || (WrapFlags & 2)) &&
        !llvm::is_contained(AllOpFillTuples, FillTuple{ZEROS, ZEROS, ZEROS}) &&
        isKnownNonNeg(I->getOperand(0)) && isKnownNonNeg(I->getOperand(1)))
      AddTuple(ZEROS, ZEROS, ZEROS);
    break;
  }
  default:
    break;
  }

  if (ResultFill == ZEROS &&
      !llvm::is_contained(AllOpFillTuples, FillTuple{SIGN, SIGN, ZEROS}) &&
      canProduceZeroFillFromSignedOps(I))
    AddTuple(SIGN, SIGN, ZEROS);

  IntegerFillType Fill = UNDEFINED;
  if (isa<BinaryOperator>(I) && isa<ConstantInt>(I->getOperand(1))) {
    auto Has = [&](IntegerFillType L, IntegerFillType R, IntegerFillType F) {
      return llvm::is_contained(AllOpFillTuples, FillTuple{L, R, F});
    };

    // If the tuple we are searching already exist
    // avoid the known bits queries.
    if (Opcode == Instruction::Add &&
        (ResultFill == SIGN || ResultFill == ZEROS) &&
        Has(ResultFill, ResultFill, ResultFill)) {
      if (!IsINonNeg.has_value())
        IsINonNeg = isKnownNonNeg(I);

      if (!*IsINonNeg)
        return;
    }

    bool NeedProof = (Opcode == Instruction::Add &&
                      (!Has(ZEROS, ZEROS, ZEROS) || !Has(SIGN, SIGN, SIGN))) ||
                     (Opcode == Instruction::Sub && ResultFill == ZEROS &&
                      !Has(ZEROS, ZEROS, ZEROS)) ||
                     (Opcode == Instruction::Shl &&
                      (ResultFill == SIGN || ResultFill == ZEROS) &&
                      !Has(ResultFill, ZEROS, ResultFill));

    if (NeedProof)
      Fill = tryToProveNoWrapFlagsWithConstant(I, ResultFill);
  }

  if (Fill != UNDEFINED) {
    switch (I->getOpcode()) {
    case Instruction::Add:
    case Instruction::Sub:
    case Instruction::Mul:
      AddTuple(Fill, Fill, Fill);
      break;
    case Instruction::Shl:
      if (Fill == SIGN || Fill == ZEROS)
        AddTuple(Fill, ZEROS, Fill);
      break;
    default:
      break;
    }
  }
  return;
}

bool WideningIntegerArithmetic::chooseFillTuple(
    Instruction *I, IntegerFillType ResultFill,
    SmallVectorImpl<IntegerFillType> &OpsFills,
    SmallVectorImpl<FillTuple> *GeneratedTuples) {

  if (GeneratedTuples)
    GeneratedTuples->clear();

  OpsFills.assign(I->getNumOperands(), ANYTHING);
  if (I->getOpcode() == Instruction::PHI) {
    OpsFills.assign(I->getNumOperands(), ResultFill);
    return true;
  }
  if (I->getOpcode() == Instruction::Trunc) {
    OpsFills[0] = ResultFill;
    return true;
  }

  if (auto *II = dyn_cast<IntrinsicInst>(I)) {
    switch (II->getIntrinsicID()) {
    case Intrinsic::abs:
      // SIGN bits produce ZEROS cause of the abs
      OpsFills[0] = SIGN;
      return satisfiesResultFill(I, ZEROS, ResultFill);
    case Intrinsic::ctpop:
      // Adding ZEROS in the upper bits doesn't change the
      // population count, thus is safe.
      OpsFills[0] = ZEROS;
      return satisfiesResultFill(I, ZEROS, ResultFill);
    case Intrinsic::cttz: {
      Value *Inp = II->getArgOperand(0);
      bool IsZeroPoison = cast<ConstantInt>(II->getOperand(1))->isOne();
      // We can widen if zero is impossible (input is proven)
      // Or if zero is possible, but the original result would be poison.
      SimplifyQuery SQ(*DL, DT, AC, II);
      if (IsZeroPoison || isKnownNonZero(Inp, SQ)) {
        OpsFills[0] = ZEROS;
        return satisfiesResultFill(I, ZEROS, ResultFill);
      }
      return false;
    }
    case Intrinsic::ctlz:
      // We need to account the result change
      // if we widen ctlz, cause otherwise
      // we will return a wrong result.
      return false;
    }
  }
  SmallVector<FillTuple, 4> LocalTuples;
  auto &AllOpFillTuples = GeneratedTuples ? *GeneratedTuples : LocalTuples;
  collectLegalFillTuples(I, ResultFill, AllOpFillTuples);

  if (AllOpFillTuples.empty())
    return false;

  if (AllOpFillTuples.size() == 1) {
    const auto &[L, R, Produced] = AllOpFillTuples.front();
    assignOpFills(I, L, R, OpsFills);
  } else {
    // if we have more than one OpFills to select,
    // delay that choice to the PromoteConstraintSolver
    // as here it's very complex to check for this.
    // If we have the graph already built, it's easier
    // to find a suitable fillType.
    assignOpFills(I, PENDING, PENDING, OpsFills);
  }
  return true;
}

bool WideningIntegerArithmetic::visitReturn(
    Instruction *Instr, SmallVectorImpl<Value *> &Worklist) {

  if (isPartiallySolved(Instr)) {
    return false;
  }
  ReturnInst *RI = dyn_cast<ReturnInst>(Instr);
  Value *N0 = RI->getReturnValue();
  if (!N0)
    return false;

  if (!hasAppropriateType(N0) || isa<ExtractValueInst>(N0)) {
    createDefaultSol(Instr);
    return false;
  }
  auto N0Width = N0->getType()->getScalarSizeInBits();
  // We need to solve the Operand first.
  if (!isSolvedForWidth(N0, N0Width)) {
    pushToWorklist(N0, Worklist);
    return false;
  }
  auto FillTypeWidth = getKnownFillTypeWidthCached(N0);
  auto Opcode = RI->getOpcode();
  // Just create the return solution, as we cannot change the width.
  auto *RetSol = allocateSolution<WIA_RET>(SolutionAllocator, Opcode, Opcode,
                                           ExtensionChoice, FillTypeWidth,
                                           N0Width, N0Width, 0, RI);
  auto *Sol = getMatchingWidthSol(N0, AvailableSolutions, N0Width);
  assert(Sol != nullptr && "Didnt find op solution for return");
  RetSol->addOperand(Sol);
  AvailableSolutions[RI].push_back(RetSol);
  return false;
}

bool WideningIntegerArithmetic::visitSwitch(
    Instruction *Instr, SmallVectorImpl<Value *> &Worklist) {
  SwitchInst *SI = dyn_cast<SwitchInst>(Instr);
  Value *Condition = SI->getCondition();
  if (!isPartiallySolved(Condition))
    return false;

  unsigned Opcode = SI->getOpcode();
  SmallVector<WideningIntegerSolutionInfo *, 8> CondSols(
      AvailableSolutions[Condition]);
  for (auto *CondSol : CondSols) {
    // First, solve for the switch cases
    for (auto Case : SI->cases()) {
      auto *CI = Case.getCaseValue();
      if (AvailableSolutions[CI].size() >= 4)
        continue;
      // For SIGN keep only one extra bit as the sign bit.
      // For ZEROS ignore all the leading zeros.
      unsigned FillTypeWidth =
      ExtensionChoice == ZEROS ? CI->getValue().getActiveBits()
                               : CI->getValue().getSignificantBits();
      unsigned short CiBitWidth = CI->getBitWidth();
      auto *Sol = allocateSolution<WIA_CONSTANT>(
          SolutionAllocator, CONSTANT_INT_OPC, CONSTANT_INT_OPC,
          ExtensionChoice, FillTypeWidth, CiBitWidth,
          CondSol->getUpdatedWidth(), 0, CI);
      AvailableSolutions[CI].push_back(Sol);
    }
    auto Cost = CondSol->getCost();
    if (Cost < 0)
      Cost = 0;
    auto *SISol = allocateSolution<WIA_SWITCH>(
        SolutionAllocator, Opcode, Opcode, ExtensionChoice,
        CondSol->getFillTypeWidth(), CondSol->getWidth(),
        CondSol->getUpdatedWidth(), Cost, SI);
    SISol->addOperand(CondSol);
    AvailableSolutions[SI].push_back(SISol);
  }
  return false;
}


unsigned
WideningIntegerArithmetic::getExtensionChoice(enum IntegerFillType ExtChoice) {
  return ExtChoice == SIGN ? Instruction::SExt : Instruction::ZExt;
}

bool WideningIntegerArithmetic::isLegalToPromote(Instruction *Instr) {
  if (!isa<OverflowingBinaryOperator>(Instr))
    return true;
  if (ExtensionChoice == SIGN)
    return Instr->hasNoSignedWrap();
  if (ExtensionChoice == ZEROS)
    return Instr->hasNoUnsignedWrap();
  return false;
}

bool WideningIntegerArithmetic::tryToRegisterSolutions(
    Instruction *Instr, ArrayRef<WideningIntegerSolutionInfo *> PossibleSols,
    SmallVectorImpl<WideningIntegerSolutionInfo *> &Solutions) {
  bool Changed = false;
  for (auto *Sol : PossibleSols) {
    if (addNonRedundant(Instr, Sol)) {
      Solutions.push_back(Sol);
      Changed = true;
    }
  }
  return Changed;
}

bool WideningIntegerArithmetic::visitFill(
    Instruction *Instr,
    SmallVector<WideningIntegerSolutionInfo *, 8> &Solutions) {

  unsigned short FillTypeWidth = getKnownFillTypeWidthCached(Instr);
  if (FillTypeWidth == 0) {
    return false;
  }
  const SmallVector<WideningIntegerSolutionInfo *, 8> &Sols =
      AvailableSolutions[Instr];

  SmallVector<WideningIntegerSolutionInfo *, 8> NewFillSols;
  for (WideningIntegerSolutionInfo *Sol : Sols) {
    unsigned SolOpc = Sol->getOpcode();
    if (SolOpc == Instruction::SExt || llvm::ISD::isExtOpcode(SolOpc) ||
        SolOpc == Instruction::Trunc || SolOpc == Instruction::ZExt ||
        Sol->getKind() == WIAK_DROP_EXT ||
        Sol->getNewOpcode() == FILL_INST_OPC || Sol->getKind() == WIAK_WIDEN ||
        Sol->getNewOpcode() == Instruction::ICmp)
      continue;
    // WIA_FILL extends the least significant *Width* bits of Instr
    // to targetWidth
    if (Sol->getFillTypeWidth() <= 1)
      continue;

    for (auto IntegerSize : IntegerSizes) {
      EVT NewVT = EVT::getIntegerVT(*Ctx, IntegerSize);
      if (IntegerSize < FillTypeWidth || IntegerSize < Sol->getWidth() ||
          TLI->InstructionOpcodeToISD(SolOpc) == ISD::TRUNCATE ||
          !TLI->isOperationLegal(ISD::SIGN_EXTEND_INREG, NewVT)) {
        continue;
      }
      auto Cost = Sol->getCost();
      if (Cost < 0)
        Cost = 0;
      Type *TruncTy = Type::getIntNTy(*Ctx, FillTypeWidth);
      Type *ExtTy = Type::getIntNTy(*Ctx, IntegerSize);
      Type *SrcTy = Type::getIntNTy(*Ctx, Sol->getUpdatedWidth());
      auto TruncCost =
          TTI->getCastInstrCost(Instruction::Trunc, TruncTy, SrcTy,
                                TTI::CastContextHint::None, TTICostKind);
      auto SextCost =
          TTI->getCastInstrCost(Instruction::SExt, ExtTy, TruncTy,
                                TTI::CastContextHint::None, TTICostKind);
      WideningIntegerSolutionInfo *Fill = allocateSolution<WIA_FILL>(
          SolutionAllocator, SolOpc, FILL_INST_OPC, ExtensionChoice,
          FillTypeWidth, FillTypeWidth, IntegerSize,
          TruncCost + SextCost + Cost, Instr);
      Fill->addOperand(Sol);
      NewFillSols.push_back(Fill);
    }
  }
  return tryToRegisterSolutions(Instr, NewFillSols, Solutions);
}

bool WideningIntegerArithmetic::visitNarrow(
    Instruction *Instr,
    SmallVector<WideningIntegerSolutionInfo *, 8> &Solutions) {

  unsigned short Width = Instr->getType()->getScalarSizeInBits();
  unsigned short FillTypeWidth = getKnownFillTypeWidthCached(Instr);
  if (FillTypeWidth == 0) {
    return false;
  }
  std::vector<short int> TruncSizes = {8, 16, 32};
  const SmallVector<WideningIntegerSolutionInfo *, 8> &Sols =
      AvailableSolutions[Instr];

  SmallVector<WideningIntegerSolutionInfo *, 8> NewNarrowSols;
  for (auto *Sol : Sols) {
    auto Kind = Sol->getKind();
    unsigned SolOpc = Sol->getOpcode();
    // ignore casts
    if (Kind == llvm::WIAK_DROP_EXT || Kind == WIAK_NOOP)
      continue;
    if (SolOpc == FILL_INST_OPC || Sol->getNewOpcode() == FILL_INST_OPC)
      continue;
    unsigned IsdOpc = TLI->InstructionOpcodeToISD(SolOpc);
    if (llvm::ISD::isExtOpcode(IsdOpc) || IsdOpc == ISD::SIGN_EXTEND_INREG ||
        IsdOpc == ISD::TRUNCATE)
      continue;
    // Try legal widths smaller than the original width, keeping
    // all fillType bits.
    for (int IntegerSize : TruncSizes) {
      EVT NewVT = EVT::getIntegerVT(*Ctx, IntegerSize);
      if (!TLI->isOperationLegal(ISD::TRUNCATE, NewVT) ||
          IntegerSize < FillTypeWidth || IntegerSize >= Sol->getWidth()) {
        continue;
      }
      auto Cost = Sol->getCost();
      Type *Ty1 = Instr->getType();
      Type *Ty2 = Type::getIntNTy(*Ctx, IntegerSize);
      if (!TLI->isTruncateFree(Ty1, Ty2)) {
        Cost += TTI->getCastInstrCost(Instruction::Trunc, Ty2, Ty1,
                                      TTI::CastContextHint::None, TTICostKind);
      }
      WideningIntegerSolutionInfo *Trunc = allocateSolution<WIA_NARROW>(
          SolutionAllocator, SolOpc, Instruction::Trunc,
          ExtensionChoice, // Will depend on available Narrowing ,
          FillTypeWidth, Width, IntegerSize, Cost, Instr);
      Trunc->addOperand(Sol);
      NewNarrowSols.push_back(Trunc);
    }
  }
  return tryToRegisterSolutions(Instr, NewNarrowSols, Solutions);
}

InstructionCost
WideningIntegerArithmetic::getExtCost(Instruction *Instr,
                                      const WideningIntegerSolutionInfo *Sol,
                                      unsigned short DstSize) {
  auto Cost = Sol->getCost();
  Type *Ty1 = Instr->getType();
  Type *Ty2 = Type::getIntNTy(*Ctx, static_cast<int>(DstSize));
  if (ExtensionChoice == ZEROS && !TLI->isZExtFree(Ty1, Ty2))
    Cost += TTI->getCastInstrCost(Instruction::ZExt, Ty2, Ty1,
                                  TTI::CastContextHint::None, TTICostKind);
  if (ExtensionChoice == SIGN && !TLI->isSExtFree(Ty1, Ty2))
    Cost += TTI->getCastInstrCost(Instruction::SExt, Ty2, Ty1,
                                  TTI::CastContextHint::None, TTICostKind);
  return Cost;
}

bool WideningIntegerArithmetic::visitWiden(
    Instruction *Instr,
    SmallVector<WideningIntegerSolutionInfo *, 8> &Solutions) {

  unsigned short Width = Instr->getType()->getScalarSizeInBits();
  unsigned ExtensionOpc = getExtensionChoice(ExtensionChoice);
  unsigned ExtensionISDOpc = TLI->InstructionOpcodeToISD(ExtensionOpc);
  unsigned short FillTypeWidth = getKnownFillTypeWidthCached(Instr);
  const SmallVector<WideningIntegerSolutionInfo *, 8> &Sols =
      AvailableSolutions[Instr];
  if (!isLegalToPromote(Instr)) {
    return false;
  }

  SmallVector<WideningIntegerSolutionInfo *, 8> NewWidenSols;
  for (WideningIntegerSolutionInfo *Sol : Sols) {
    unsigned SolOpc = Sol->getOpcode();
    if (SolOpc == FILL_INST_OPC || Sol->getNewOpcode() == FILL_INST_OPC)
      continue;

    unsigned IsdOpc = TLI->InstructionOpcodeToISD(SolOpc);
    // ignore existing casts
    if (llvm::ISD::isExtOpcode(IsdOpc) || IsdOpc == ISD::SIGN_EXTEND_INREG ||
        IsdOpc == ISD::TRUNCATE || Sol->getKind() == WIAK_WIDEN)
      continue;
    for (int IntegerSize : IntegerSizes) {
      EVT NewVT = EVT::getIntegerVT(*Ctx, IntegerSize);
      if (IntegerSize < FillTypeWidth || IntegerSize <= Sol->getWidth() ||
          !TLI->isOperationLegal(ExtensionISDOpc, NewVT)) {
        continue;
      }
      auto Cost = getExtCost(Instr, Sol, IntegerSize);
      // Results to a widened expr based on ExtensionOpc
      WideningIntegerSolutionInfo *Widen = allocateSolution<WIA_WIDEN>(
          SolutionAllocator, SolOpc, ExtensionOpc, ExtensionChoice,
          FillTypeWidth, Width, IntegerSize, Cost, Instr);
      Widen->addOperand(Sol);
      NewWidenSols.push_back(Widen);
    }
  }
  return tryToRegisterSolutions(Instr, NewWidenSols, Solutions);
}

bool WideningIntegerArithmetic::closure(Instruction *Instr,
                                        SmallVectorImpl<Value *> &Worklist) {
  bool Changed = false;
  bool ItChange = false;
  SmallVector<WideningIntegerSolutionInfo *, 8> Fills;
  SmallVector<WideningIntegerSolutionInfo *, 8> Widens;
  SmallVector<WideningIntegerSolutionInfo *, 8> Narrows;
  while (true) {
    ItChange = false;
    ItChange |= visitFill(Instr, Fills);
    ItChange |= visitWiden(Instr, Widens);
    ItChange |= visitNarrow(Instr, Narrows);

    if (!ItChange)
      break;

    Changed = true;
  }
  return Changed;
}

bool WideningIntegerArithmetic::visitPhi(Instruction *Instr) {
  if (AvailableSolutions[Instr].empty()) {
    createDefaultSol(Instr);
    return true;
  }
  return false;
}

bool WideningIntegerArithmetic::combineSols(
    Instruction *Instr, SmallVectorImpl<Value *> &Worklist) {
  bool Changed = false;
  Value *OP0 = Instr->getOperand(0);
  Value *OP1 = Instr->getOperand(1);
  auto &Sols0 = AvailableSolutions[OP0];
  auto &Sols1 = AvailableSolutions[OP1];
  unsigned Opcode = Instr->getOpcode();

  if (Sols0.empty() || Sols1.empty())
    return false;

  ICmpInst *ICmpInstr = nullptr;
  Type *I1Ty = nullptr;
  if (Opcode == Instruction::ICmp) {
    ICmpInstr = dyn_cast<ICmpInst>(Instr);
    I1Ty = Type::getInt1Ty(*Ctx);
  }

  uint8_t WrapFlags = 0;
  if (auto *OBO = dyn_cast<OverflowingBinaryOperator>(Instr)) {
    if (OBO->hasNoUnsignedWrap())
      WrapFlags |= 1;
    if (OBO->hasNoSignedWrap())
      WrapFlags |= 2;
  }
  const auto &OperandFillTypes = getFillTypes(Instr);
  unsigned short FillTypeWidth = getKnownFillTypeWidthCached(Instr);
  KnownBits KB;
  getKnownBitsCached(Instr, KB);

  IntegerFillType InstFill = determineFill(KB, FillTypeWidth);

  for (WideningIntegerSolutionInfo *LeftSolution : Sols0) {
    unsigned short LeftUpdWidth = LeftSolution->getUpdatedWidth();
    if (LeftUpdWidth == 0 || LeftUpdWidth < FillTypeWidth)
      continue;
    for (WideningIntegerSolutionInfo *RightSolution : Sols1) {
      unsigned short RightUpdWidth = RightSolution->getUpdatedWidth();
      if ((LeftUpdWidth != RightUpdWidth) || RightUpdWidth == 0)
        continue;
      IntegerFillType Fill = InstFill;
      if (InstFill == ANYTHING) {
        auto LeftFill = LeftSolution->getFillType();
        auto RightFill = RightSolution->getFillType();
        // Infer the ResultFill from the stored
        // (leftFill, RightFill, ResultFill) rules,
        // using the operand fills and the NoWrapFlags.
        Fill = getOrNullFillType(Instr, OperandFillTypes, LeftFill, RightFill,
                                 WrapFlags);
      }

      if (Fill == UNDEFINED) {
        continue;
      }
      EVT NewVT = EVT::getIntegerVT(*Ctx, LeftUpdWidth);
      auto InstOpc = Instr->getOpcode();
      auto Opc = TLI->InstructionOpcodeToISD(Opcode);
      auto TLIOpLegal =
          TLI->isOperationLegalOrCustomOrPromote(Opc, NewVT) ||
          TLI->getTypeAction(*Ctx, NewVT) == TargetLowering::TypePromoteInteger;
      if (!TLIOpLegal && InstOpc != Instruction::ICmp) {
        continue;
      }
      if (InstOpc == Instruction::ICmp) {
        auto Op0VT =
            TLI->getValueType(*DL, Type::getIntNTy(*Ctx, LeftUpdWidth));
        if (!TLI->isTypeLegal(Op0VT)) {
          continue;
        }
      }

      InstructionCost Cost = 0;
      unsigned short UpdatedWidth = LeftUpdWidth;
      auto *UpdatedWidthTy = Type::getIntNTy(*Ctx, UpdatedWidth);
      if (!isa<ICmpInst>(Instr)) {
        Cost = TTI->getArithmeticInstrCost(Opcode, UpdatedWidthTy, TTICostKind);
        unsigned short OldWidth = Instr->getType()->getIntegerBitWidth();
        auto *Sol = allocateSolution<WIA_BINOP>(
            SolutionAllocator, Opcode, Opcode, Fill, FillTypeWidth, OldWidth,
            UpdatedWidth, Cost, Instr);
        Sol->addOperand(LeftSolution);
        Sol->addOperand(RightSolution);
        if (addNonRedundant(Instr, Sol)) {
          Changed = true;
        }
      } else {
        Value *VI = dyn_cast<Value>(Instr);
        Cost = TTI->getCmpSelInstrCost(Instruction::ICmp, UpdatedWidthTy, I1Ty,
                                       ICmpInstr->getPredicate(), TTICostKind);
        auto *Sol =
            allocateSolution<WIA_ICMP>(SolutionAllocator, Opcode, Opcode, Fill,
                                       FillTypeWidth, 1, 1, Cost, VI);
        Sol->setUpdatedIcmpWidth(LeftUpdWidth);
        Sol->addOperand(LeftSolution);
        Sol->addOperand(RightSolution);
        if (addNonRedundant(Instr, Sol))
          Changed = true;
      }
    }
  }
  bool WasEmpty = false;
  if (AvailableSolutions[Instr].empty()) {
    createDefaultSol(Instr);
    Changed = true;
    WasEmpty = true;
  }
  if (!isa<ICmpInst>(Instr) && Changed && !WasEmpty) {
    Changed |= closure(Instr, Worklist);
  }
  return Changed;
}

bool WideningIntegerArithmetic::visitDropExt(
    Instruction *Instr, SmallVectorImpl<Value *> &Worklist) {
  Value *N0 = Instr->getOperand(0);
  unsigned int OldWidth = N0->getType()->getScalarSizeInBits();
  if (!isSolvedForWidth(N0, OldWidth)) {
    pushToWorklist(N0, Worklist);
    return false;
  }
  unsigned ExtensionOpc = getExtensionChoice(ExtensionChoice);
  unsigned int ExtendedWidth = Instr->getType()->getScalarSizeInBits();
  auto *OldTy = Type::getIntNTy(*Ctx, OldWidth);
  auto *ExtTy = Type::getIntNTy(*Ctx, ExtendedWidth);
  if (!llvm::isPowerOf2_32(ExtendedWidth) || !llvm::isPowerOf2_32(OldWidth)) {
    return false;
  }

  bool Added = false;
  // For Overflowing operators we cannot drop an extension
  // if we can't prove a no-wrap.
  if (auto *Binop = dyn_cast<OverflowingBinaryOperator>(N0)) {
    bool MissingNoWrap = (isa<SExtInst>(Instr) && !Binop->hasNoSignedWrap()) ||
                         (isa<ZExtInst>(Instr) && !Binop->hasNoUnsignedWrap());
    if (MissingNoWrap) {
      auto *Sol = allocateSolution<WIA_NOOP>(
          SolutionAllocator, Instr->getOpcode(), ExtensionOpc, ExtensionChoice,
          ExtendedWidth, OldWidth, ExtendedWidth, 0, Instr);
      auto *MatchingSol = getMatchingWidthSol(N0, AvailableSolutions, OldWidth);
      if (MatchingSol != nullptr) {
        Sol->addOperand(MatchingSol);
        Added = addNonRedundant(Instr, Sol);
      }
      return false;
    }
  }
  // Add a no-op sol, meaning we just keep the extension
  // In a case we can't drop it later on.
  unsigned short FillTypeWidth = getKnownFillTypeWidthCached(N0);
  WideningIntegerSolutionInfo *WidenSol = allocateSolution<WIA_NOOP>(
      SolutionAllocator, Instr->getOpcode(), Instr->getOpcode(),
      ExtensionChoice, FillTypeWidth, OldWidth, ExtendedWidth, 0, Instr);
  auto *MatchingSol = getMatchingWidthSol(N0, AvailableSolutions, OldWidth);
  if (MatchingSol != nullptr) {
    WidenSol->addOperand(MatchingSol);
    Added = addNonRedundant(Instr, WidenSol);
  }
  auto ExtCost = TTI->getCastInstrCost(
      Instr->getOpcode() == Instruction::SExt ? Instruction::SExt
                                              : Instruction::ZExt,
      ExtTy, OldTy, TTI::CastContextHint::None, TTICostKind);
  if (isa<LoadInst>(N0))
    ExtCost = 0;

  bool Changed = Added;
  Value *VInstr = dyn_cast<Value>(Instr);
  SmallVector<WideningIntegerSolutionInfo *, 8> &ExprSolutions =
      AvailableSolutions[N0];
  for (WideningIntegerSolutionInfo *Solution : ExprSolutions) {
    if (Solution->getKind() == WIAK_DROP_EXT ||
        Solution->getKind() == WIAK_WIDEN ||
        Solution->getNewOpcode() == FILL_INST_OPC) {
      continue;
    }
    if (Solution->getUpdatedWidth() == ExtendedWidth &&
        !isTruncKind(Solution)) {
      continue;
    }
    // We assume we can drop the extension and we will later see if it's needed.
    unsigned short FillTypeWidth = Solution->getFillTypeWidth();
    if (FillTypeWidth > OldWidth && !isTruncKind(Solution)) {
      continue;
    }
    auto Cost = Solution->getCost();
    if (Cost < 0) {
      Cost = 0;
    }
    WideningIntegerSolutionInfo *Expr = allocateSolution<WIA_DROP_EXT>(
        SolutionAllocator, Instr->getOpcode(), Solution->getNewOpcode(),
        Solution->getFillType(), FillTypeWidth, ExtendedWidth /*OldWidth*/,
        Solution->getUpdatedWidth() /*NewWidth*/, Cost - ExtCost, Instr);
    Expr->addOperand(Solution);
    Changed |= addNonRedundant(VInstr, Expr);
    for (auto NewWidth : IntegerSizes) {
      // try to find extra casts to a new width
      if (NewWidth >= FillTypeWidth && NewWidth != OldWidth &&
          NewWidth != ExtendedWidth) {
        auto NoopOldWidth = Solution->getWidth();
        if (isa<ICmpInst>(N0)) {
          NoopOldWidth = 1;
        }
        auto *Sol = allocateSolution<WIA_NOOP>(
            SolutionAllocator, Instr->getOpcode(), Instr->getOpcode(),
            ExtensionChoice, FillTypeWidth, NoopOldWidth, NewWidth, Cost,
            VInstr);
        Sol->addOperand(Solution);
        Changed |= addNonRedundant(VInstr, Sol);
      }
    }
  }
  return Changed;
}

bool WideningIntegerArithmetic::canDropTruncZext(Instruction *Instr, Value *N0,
                                                 unsigned short NewWidth) {
  bool CanDrop = true;
  if (isa<LoadInst>(N0) && NewWidth == 8) {
    auto *SolIt = std::find_if(AvailableSolutions[N0].begin(),
                               AvailableSolutions[N0].end(),
                               [&](const WideningIntegerSolutionInfo *Sol) {
                                 return Sol->getUpdatedWidth() == 8 &&
                                        Sol->getKind() == WIAK_DROP_EXT &&
                                        Sol->getOpcode() == Instruction::ZExt;
                               });
    bool SolExists = SolIt != AvailableSolutions[N0].end();

    if (MDNode *RangeMD = Instr->getMetadata(LLVMContext::MD_range)) {
      unsigned NumOperands = RangeMD->getNumOperands();
      for (unsigned I = 0; I + 1 < NumOperands; I += 2) {
        ConstantInt *Low =
            mdconst::dyn_extract<ConstantInt>(RangeMD->getOperand(I));
        ConstantInt *High =
            mdconst::dyn_extract<ConstantInt>(RangeMD->getOperand(I + 1));
        if (Low && High) {
          auto HighNum = High->getValue().getZExtValue();
          if (HighNum > 2 && SolExists) {
            CanDrop = false;
            break;
          }
        }
      }
    }
  }
  return CanDrop;
}

bool WideningIntegerArithmetic::visitDropTrunc(
    Instruction *Instr, SmallVectorImpl<Value *> &Worklist) {

  Value *N0 = Instr->getOperand(0);
  Value *VInstr = dyn_cast<Value>(Instr);
  auto NarrowTy = Instr->getType();
  auto WideTy = N0->getType();
  unsigned short TruncatedWidth = NarrowTy->getScalarSizeInBits();
  if (!llvm::isPowerOf2_32(TruncatedWidth)) {
    return false;
  }
  unsigned int WideWidth = N0->getType()->getScalarSizeInBits();
  if (!isSolvedForWidth(N0, WideWidth)) {
    pushToWorklist(N0, Worklist);
    return false;
  }

  auto ExprSolutionsIt = AvailableSolutions.find(N0);
  bool Changed = false;
  // NewWidth is the width of the value before the truncation
  auto *DstTy = N0->getType();
  unsigned short NewWidth = DstTy->getScalarSizeInBits();
  unsigned short FillTypeWidth = getKnownFillTypeWidthCached(Instr);
  WideningIntegerSolutionInfo *NoTruncSol = allocateSolution<WIA_NOOP>(
      SolutionAllocator, Instr->getOpcode(), Instr->getOpcode(),
      ExtensionChoice, FillTypeWidth, NewWidth, TruncatedWidth, 0, Instr);
  auto *MatchingSol = getMatchingWidthSol(N0, AvailableSolutions, WideWidth);
  if (MatchingSol != nullptr) {
    NoTruncSol->addOperand(MatchingSol);
    Changed |= addNonRedundant(Instr, NoTruncSol);
  }

  bool CanDrop = canDropTruncZext(Instr, N0, NewWidth);
  if (!CanDrop)
    return Changed;

  SmallVector<WideningIntegerSolutionInfo *, 8> &ExprSolutions =
      ExprSolutionsIt->getSecond();
  auto TruncCost =
      TTI->getCastInstrCost(Instruction::Trunc, NarrowTy, WideTy,
                            TTI::CastContextHint::None, TTICostKind);
  for (auto *Sol : ExprSolutions) {
    auto Cost = Sol->getCost();
    if (Cost < 0)
      Cost = 0;
    if (Sol->getFillTypeWidth() > TruncatedWidth)
      continue;

    WideningIntegerSolutionInfo *Expr = allocateSolution<WIA_DROP_LOCOPY>(
        SolutionAllocator, Instr->getOpcode(), Sol->getNewOpcode(),
        Sol->getFillType(), Sol->getFillTypeWidth(), TruncatedWidth,
        Sol->getUpdatedWidth(), Cost - TruncCost, dyn_cast<Value>(Instr));
    Expr->setOperands(Sol->getOperands());
    bool Added = addNonRedundant(VInstr, Expr);
    if (Added) {
      Changed |= Added;
    }
  }
  for (auto *Sol : ExprSolutions) {
    auto Cost = Sol->getCost();
    if (Cost < 0)
      Cost = 0;
    if (Sol->getFillTypeWidth() > TruncatedWidth)
      continue;
    Type *FromTy = Type::getIntNTy(*Ctx, TruncatedWidth);
    Type *ToTy = Type::getIntNTy(*Ctx, NewWidth);
    if (TLI->isTruncateFree(ToTy, FromTy)) {
      Cost = 0;
    } else {
      Cost = Cost - TruncCost;
    }
    WideningIntegerSolutionInfo *Expr = allocateSolution<WIA_DROP_LOIGNORE>(
        SolutionAllocator, Instr->getOpcode(), Sol->getNewOpcode(),
        Sol->getFillType(), Sol->getFillTypeWidth(), TruncatedWidth,
        Sol->getUpdatedWidth(), Cost, Instr);
    Expr->setOperands(Sol->getOperands());
    Changed |= addNonRedundant(VInstr, Expr);
  }
  return Changed;
}

bool WideningIntegerArithmetic::visitInstruction(
    Instruction *Instr, SmallVectorImpl<Value *> &Worklist) {
  unsigned Opcode = Instr->getOpcode();
  if (Instr->isBinaryOp() && Instr->getType()->isIntOrIntVectorTy())
    return combineSols(Instr, Worklist);

  switch (Opcode) {
  case Instruction::ICmp:
    return combineSols(Instr, Worklist);
  case Instruction::Store:
    return visitStore(Instr, Worklist);
  case Instruction::ZExt:
  case Instruction::SExt:
    return visitDropExt(Instr, Worklist);
  case Instruction::Trunc:
    return visitDropTrunc(Instr, Worklist);
  case Instruction::PHI:
    return visitPhi(Instr);
  case Instruction::Ret:
    return visitReturn(Instr, Worklist);
  case Instruction::Switch:
    return visitSwitch(Instr, Worklist);
  case Instruction::Select:
    return visitSelect(Instr);
  default:
    break;
  }
  unsigned short Width = Instr->getType()->getScalarSizeInBits();
  auto *Sol = allocateSolution<WideningIntegerSolutionInfo>(
      SolutionAllocator, Opcode, Opcode, ANYTHING, 0, Width, Width,
      InstructionCost(0), WIAK_UNKNOWN, Instr);
  return addNonRedundant(Instr, Sol);
}

bool WideningIntegerArithmetic::solveInst(Value *PopVal,
                                          SmallVectorImpl<Value *> &Worklist) {
  bool Changed = false;

  if (auto *I = dyn_cast<Instruction>(PopVal)) {
    switch (I->getOpcode()) {
    // Instructions where we use visitInstruction
    case Instruction::Switch:
    case Instruction::Store:
    case Instruction::Ret:
      if (hasAppropriateType(PopVal)) {
        Changed |= visitInstruction(I, Worklist);
      }
      break;
    case Instruction::UncondBr:
    case Instruction::CondBr:
    case Instruction::Load:
    case Instruction::Call:
    case Instruction::Invoke:
    case Instruction::PtrToInt:
    case Instruction::FPToSI:
    case Instruction::FPToUI:
      // Create the Top solution for some insts we don't change
      Changed = createDefaultSol(PopVal);
      SolvedInstructions.set(ValueToNum(PopVal));
      break;
    default:
      if (isa<IntegerType>(PopVal->getType()) && hasAppropriateType(PopVal)) {
        Changed |= visitInstruction(I, Worklist);
      }
      break;
    }
  }
  if (isa<UndefValue>(PopVal)) {
    Changed |= visitUndef(PopVal);
  }
  if (Changed)
    pushUsersToWorklist(PopVal, Worklist);

  return Changed;
}

void WideningIntegerArithmetic::processWorklist(
    SmallVectorImpl<Value *> &Worklist) {

  while (!Worklist.empty()) {
    auto *PopVal = Worklist.pop_back_val();
    auto VNum = ValueToNum(PopVal);
    InsideWorklist.reset(VNum);
    auto &Sols = AvailableSolutions[PopVal];
    if (shouldIgnoreValue(PopVal)) {
      if (Sols.empty()) {
        createDefaultSol(PopVal);
      }
      SolvedInstructions.set(VNum);
      continue;
    }
    if (isSolved(PopVal)) {
      continue;
    }
    bool OpsSolved = checkOperands(PopVal, Worklist);
    bool Changed = solveInst(PopVal, Worklist);
    // if no new solutions and ops are solved we are done.
    if (!Changed && OpsSolved) {
      SolvedInstructions.set(VNum);
      InsideWorklist.reset(VNum);
    }
  }
}

bool WideningIntegerArithmetic::visitWidening(Function &F) {
  SmallVector<Value *, 128> Worklist;
  // We build the worklist by starting from each extension
  // and inspect only the relevant IR, so we don't use the
  // expensive width solver on unrelated instructions.
  buildWorklist(F, Worklist);
  if (Worklist.empty()) {
    Worklist.clear();
    return true;
  }
  processWorklist(Worklist);
  Worklist.clear();
  return true;
}

std::optional<bool> WideningIntegerArithmetic::tryToGetLocalPromotionSolution(
    Value *V, unsigned Width, IntegerFillType Fill,
    llvm::function_ref<RegionResult(Instruction *)> ClassifyRegion) {

  if (V->getType()->isIntegerTy() &&
      isa<Argument, LoadInst, CallInst, InvokeInst, PtrToIntInst, FPToSIInst,
          FPToUIInst>(V) &&
      Width > V->getType()->getIntegerBitWidth()) {
    return false;
  }

  if (isa<ICmpInst>(V) && V->getType()->isIntegerTy(1) && Width != 1) {
    return false;
  }

  // Try to predict visitDropExt's keep-extension solution
  // so we don't need to run the whole analysis.
  if (auto *Ext = dyn_cast<CastInst>(V); Ext && isa<SExtInst, ZExtInst>(Ext) &&
                                         Ext->getDestTy()->isIntegerTy(Width) &&
                                         Fill == ExtensionChoice) {
    Value *Src = Ext->getOperand(0);
    auto HasOriginalWidth = [](Value *Op) {
      return isa<Argument, LoadInst, PHINode>(Op);
    };
    bool OriginalWidth = HasOriginalWidth(Src);
    // if we have a select we can possible avoid the wia analysis too.
    if (auto *Sel = dyn_cast<SelectInst>(Src)) {
      Value *TrueVal = Sel->getTrueValue();
      Value *FalseVal = Sel->getFalseValue();
      OriginalWidth =
          (HasOriginalWidth(TrueVal) || isa<ConstantInt>(TrueVal)) &&
          (HasOriginalWidth(FalseVal) || isa<ConstantInt>(FalseVal)) &&
          (HasOriginalWidth(TrueVal) || HasOriginalWidth(FalseVal));
    }

    if (OriginalWidth && isSupportedScalarIntType(Src->getType()) &&
        isSupportedScalarIntType(Ext->getDestTy()) && DT &&
        DT->isReachableFromEntry(Ext->getParent()) &&
        ClassifyRegion(Ext) == RegionResult::Accepted)
      return true;
  }

  // visitDROPExt cannot drop extensions without nsw or nuw flags.
  // So, don't run the whole analysis on those instructions.
  if (auto *Ext = dyn_cast<CastInst>(V);
      Ext && isa<SExtInst, ZExtInst>(Ext) && Ext->getDestTy()->isIntegerTy() &&
      Width > Ext->getDestTy()->getIntegerBitWidth()) {

    auto *Op = dyn_cast<OverflowingBinaryOperator>(Ext->getOperand(0));
    if (Op && (isa<SExtInst>(Ext) ? !Op->hasNoSignedWrap()
                                  : !Op->hasNoUnsignedWrap())) {
      return false;
    }
    Value *Src = Ext->getOperand(0);

    // Try to predict visitDropExt's wider width solution
    // from an argument/load's original width solution.
    if (Fill == ANYTHING && isa<Argument, LoadInst, PHINode>(Src) &&
        isSupportedScalarIntType(Src->getType()) &&
        isSupportedScalarIntType(Ext->getDestTy()) &&
        llvm::is_contained(IntegerSizes, Width) && DT &&
        DT->isReachableFromEntry(Ext->getParent()) &&
        ClassifyRegion(Ext) == RegionResult::Accepted) {
      return true;
    }
  }

  if ((Fill == ZEROS || Fill == SIGN) && Width > 1 && isa<BinaryOperator>(V) &&
      V->getType()->isIntegerTy(Width)) {
    KnownBits Known;
    getKnownBitsCached(V, Known);

    if (Known.countMaxActiveBits() <= 1) {
      if (Fill == ZEROS)
        return true;

      // visitFILL requires legal sext_inreg
      // if it's not legal don't run the analysis.
      EVT VT = TLI->getValueType(*DL, V->getType());
      if (!TLI->isOperationLegal(ISD::SIGN_EXTEND_INREG, VT))
        return false;
    }
  }

  // If a value is inside a region and the region will get rejected
  // don't run the wia fill analysis.
  if (auto *I = dyn_cast<Instruction>(V); I && I->getType()->isIntegerTy()) {
    bool IsValueOfInterest =
        (isa<BinaryOperator>(I) && !isSource(I) && !isSink(I)) ||
        (isa<SExtInst, ZExtInst>(I) &&
         Width > I->getType()->getIntegerBitWidth());

    if (IsValueOfInterest && ClassifyRegion(I) == RegionResult::Rejected) {
      return false;
    }
  }
  return std::nullopt;
}

bool WideningIntegerArithmetic::hasPromotionSolution(Value *V, unsigned Width,
                                                     IntegerFillType Fill) {
  auto SolsIt = AvailableSolutions.find(V);
  if (SolsIt == AvailableSolutions.end())
    return false;

  const auto &Sols = SolsIt->getSecond();

  // ANYTHING is allowed because we check it at a later stage at
  // the PromoteConstraintSolver
  return llvm::any_of(Sols, [=](WideningIntegerSolutionInfo *Sol) {
    return Sol->getUpdatedWidth() == Width &&
           (Sol->getFillType() == Fill ||
            (Fill == ANYTHING && Sol->getFillType() != UNDEFINED));
  });
}

InstructionCost WideningIntegerArithmetic::getExtTTICost(Type *SrcTy,
                                                         Type *DstTy,
                                                         IntegerFillType Fill) {
  if (Fill == ZEROS && TLI && TLI->isZExtFree(SrcTy, DstTy)) {
    return 0;
  }

  unsigned CastOpc = Fill == SIGN ? Instruction::SExt : Instruction::ZExt;

  InstructionCost Cost = TTI->getCastInstrCost(
      CastOpc, DstTy, SrcTy, TTI::CastContextHint::Normal, TTICostKind);

  if (Fill == SIGN && SrcTy->isIntegerTy(32) && DstTy->isIntegerTy(64))
    Cost = std::max(Cost, InstructionCost(1));

  return std::max(Cost, InstructionCost(0));
}
