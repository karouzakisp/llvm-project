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

// WIA information that later classes need
struct WIAInfo {
  // Check if the given Value has the request Solution
  // with a particular width and fillType
  llvm::function_ref<bool(Value *, unsigned, IntegerFillType)>
      HasAppropriateSolution;

  // This function is of high importance
  // defined in the WIA class as chooseFillTuple
  // Try to find matching fill types when we have a
  // an Instructions with a result filltype
  llvm::function_ref<bool(Instruction *, IntegerFillType,
                          SmallVectorImpl<IntegerFillType> &,
                          SmallVectorImpl<FillTuple> *)>
      ChooseOpsFills;

  // Get the extension TTI cost to go from SrcTy to DstTy
  llvm::function_ref<InstructionCost(Type *, Type *, IntegerFillType)>
      GetExtTTICost;

  // Check if the zero extend from SrcTy to DstTy is free.
  llvm::function_ref<bool(Type *, Type *)> IsZExtFree;

  // Check if the Ext is free
  llvm::function_ref<bool(Instruction *)> IsExtFree;

  // Get the known bits from the cache, do not try to recompute them.
  llvm::function_ref<void(Value *, KnownBits &)> GetKnownBitsCached;

  // Check if Value V is known to be non negative.
  llvm::function_ref<bool(Value *)> IsKnownNonNeg;

};

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

// What to do with an extension?
enum class ExtStrategy {
  KeepExt,            // Keep the extension
  SinkToOneWideUser,  // Sink to one specific user,
  PromoteSource,      // Promote the op of that extension and
                      // all relevant insts.
  NarrowGraph,        // Narrow the relevant instructions to a narrower width.
  PromoteTruncSource, // Promote trunc source to drop the trunc and promote
                      // The op of the trunc to a bigger ty, it can remove exts
                      // too.
  PromoteICmpRoot,    // Promote the expression around together with icmp,
                      // to avoid extensions that are introduced later on
                      // by the narrow expression.
  SplitExtUses        // Split extension uses when some accept SIGN and
                      // others require ZEROS.
};

enum BoundaryRewriteKind {
  ReplaceWithSource, // trunc(zext i8 %x to i32) to i8) --> %x
  TruncSource,       // trunc(zext i8 to i32) to i4 ) --> trunc i8 %x to i4
  ExtSource,         // trunc(sext i8 %x to i32) to i16) --> sext i8 %x to i16
  RepairNarrowValue, // Truncate a promoted value for a use
                     // that needs to stay narrow.
  NarrowICmp         // icmp i32 %a, %b --> to icmp i8 %a, %b
};

// Treat some instructions as boundaries, stop examining users further.
// This can simplify cases but we might lose some opportunities.
struct BoundaryRewrite {
  // The specific operand use to repair
  Use *ExtUse = nullptr;

  // The instruction containing ExtUse
  Instruction *User = nullptr;

  // The action selected by the planner.
  BoundaryRewriteKind Kind;

  // The required type for the replacement.
  Type *ResultTy = nullptr; // for TruncSource/ExtSource.

  // The operand value captured when planning a wide-graph boundary repair.
  Value *OriginalValue =
      nullptr; // The original value we see at the plan phase.

  // As example:
  //
  // Suppose we managed somehow to promote the sum
  // sum = add i16 %x, %y
  // store i16 %sum, ptr %p
  //
  // Then we need to insert a repair to the store.
  //
  // ExtUse: %sum
  // User: The store Instruction.
  // Kind: RepairNarrowValue
  // ResultTy : i16, what the store requires.
  // Original Value: %Sum
};

// when merging fills we can either have a conflict or changed or unchanged
enum class MergeFillResult { Conflict, Unchanged, Changed };

// A structure to hold all specific info about an extension plan.
// This holds analysis data and more
struct ExtRewritePlan {
  ExtStrategy Kind = ExtStrategy::KeepExt;
  // The FillType of op0 of the cast needed for legality purposes.
  IntegerFillType SourcePromoteFill = UNDEFINED;
  unsigned NarrowWidth = 0;
  // The promoted width.
  unsigned WideWidth = 0;

  // Which instructions we narrow to a smaller type.
  SmallPtrSet<Instruction *, 8> InstsToNarrow;

  // Preserve discovery order when inserting promoted operands into the IR.
  SmallSetVector<Instruction *, 8> InstsToPromote;

  // Save instructions that can save wrap here.
  SmallPtrSet<Instruction *, 4> SafeWrap;

  // Replace cast instructions with their operand, track them.
  SmallPtrSet<Instruction *, 8> InstsToReplaceWithOperand;

  // Similar for some ext cases.
  SmallPtrSet<Instruction *, 8> ExtsToReplaceWithOperand;

  // What FillType each value requires.
  DenseMap<Value *, IntegerFillType> RequiredFill;

  // Only for constants, they need different fill per use.
  DenseMap<const Use *, IntegerFillType> RequiredOperandFill;

  // Treat some instructions as boundaries
  SmallVector<Use *, 8> BoundaryUses;

  // Uses that demanded the high bits
  SmallVector<Use *, 4> WideUses;

  // Uses that have already DstWidth
  SmallPtrSet<Value *, 8> WideLeaves;

  // Mark loads to be extended.
  DenseMap<Value *, IntegerFillType> LoadsToExtend;


  // Track integer values outside of the promoted graph such as args.
  DenseMap<Value *, IntegerFillType> ExternalOpsToExtend;

  // Mark instructions eligible from being removed.
  SmallVector<Instruction *, 4> InstsToRemove;

  // Treat some instructions as boundaries, we can rewrite them
  // but we don't rewrite their users
  SmallVector<BoundaryRewrite, 4> BoundaryRewrites;

  // Track extensions that begin with a trunc and later
  // on get re-extended.
  SmallVector<CastInst *, 4> TruncRootExits;

  // Treat the promote value as sign or zero extended
  IntegerFillType TruncRootFill = UNDEFINED;

  // Some zext instructions that are removed may need repair
  // with an end mask to mask the promoted value.
  bool NeedsSourcePromoteZExtRepair = false;

  // OP0 of the cast.
  Value *Source = nullptr;

  // The cast instruction.
  Instruction *Ext = nullptr;

  // Replace trunc(lshr X, K) with a proven equivalent ashr X, K.
  TruncInst *TruncToReplaceWithAShr = nullptr;
};

struct SourcePromoteRejectInfo {
  StringRef Reason = "BuildGraphFailed";
  Value *V = nullptr;
  Instruction *User = nullptr;
  IntegerFillType Fill = UNDEFINED;
  IntegerFillType ExistingFill = UNDEFINED;
  std::optional<unsigned> OperandIndex;
};

struct SourcePromoteGraph {

  SourcePromoteGraph() = default;

  // The Cast Instruction
  CastInst *Ext = nullptr;

  // Op0 of the Cast Instruction.
  Value *Source = nullptr;

  // The src width of the cast.
  unsigned SrcWidth;

  // The dst width of the cast.
  unsigned DstWidth;

  // Required fill for Source at DstWidth.
  // Individual operands may require different fills.
  IntegerFillType Fill;

  // Save fillType together with Value or Node Id, this is faster
  // than holding them in different data structures.
  struct NodeState {
    unsigned NodeId;
    IntegerFillType Required : 3;
    IntegerFillType LastProcessedFill : 3;
    bool CompletedOperands : 1;

    constexpr NodeState(unsigned Id = 0, IntegerFillType Fill = ANYTHING)
        : NodeId(Id), Required(Fill), LastProcessedFill(UNDEFINED),
          CompletedOperands(false) {}
  };
  static_assert(sizeof(NodeState) == 8, "keep the original state size");

  using NodeList = SmallVector<std::pair<Value *, unsigned>, 8>;
  NodeList Nodes;

  SmallDenseMap<Value *, NodeState, 16> States;

  bool containsNode(Value *V) const {
    auto It = States.find(V);
    // NodeIds start from 1.
    return It != States.end() && It->second.NodeId != 0;
  }

  NodeList::iterator findNode(Value *V) {
    auto It = States.find(V);
    return It == States.end() || It->second.NodeId == 0
               ? Nodes.end()
               : Nodes.begin() + (It->second.NodeId - 1);
  }

  NodeList::const_iterator findNode(Value *V) const {
    auto It = States.find(V);
    return It == States.end() || It->second.NodeId == 0
               ? Nodes.end()
               : Nodes.begin() + (It->second.NodeId - 1);
  }

  void addNode(Value *V) {
    auto &State = States[V];
    if (State.NodeId != 0)
      return;
    State.NodeId = Nodes.size() + 1;
    Nodes.emplace_back(V, State.NodeId);
  }

  IntegerFillType getRequiredFill(Value *V,
                                  IntegerFillType Default = UNDEFINED) const {
    auto It = States.find(V);
    return It == States.end() || It->second.Required == UNDEFINED
               ? Default
               : It->second.Required;
  }

  void setRequiredFill(Value *V, IntegerFillType Fill) {
    States[V].Required = Fill;
  }

  // Non promoted values
  SmallPtrSet<Value *, 8> ExternalLeaves;
  SmallPtrSet<Value *, 8> WideLeaves;

  // Which add/sub instruction can safely wrap?
  SmallPtrSet<Instruction *, 4> SafeWrap;

  // Do we start from an narrow ICmp?
  bool IsICmpRoot = false;

  // To prevent cycles in the graph we use a Visiting set
  SmallPtrSet<Instruction *, 8> Visiting;

  // casts and truncs
  SmallVector<Instruction *, 4> InstsToRemove;


  // Capture casts inside the graph that are eligible to be removed
  // when we are obtaining a wide value.
  // For example, when promoting to i64,
  // zext i8 %x to i32 is being removed and
  // %x is safely extended to i64.
  SmallPtrSet<Instruction *, 8> InstsToReplaceWithOperand;

  // Capture extensions whose result already have the destination width
  // zext i32 %v to i64, is being redundant when %v is safely widened
  // to i64.
  SmallPtrSet<Instruction *, 8> ExtsToReplaceWithOperand;


  // Mark loads to be extended.
  DenseMap<Value *, IntegerFillType> LoadsToExtend;

  // Track integer values outside of the promoted graph such as args.
  DenseMap<Value *, IntegerFillType> ExternalOpsToExtend;
  bool NeedsRootZExtRepair = false;

  // Treat some instructions as boundaries, we can rewrite them
  // but we don't rewrite their users
  SmallVector<BoundaryRewrite, 4> BoundaryRewrites;

  // Used for opt remarks.
  SourcePromoteRejectInfo Reject;
};

// Separate operations when promoting to different types.
// Each type requires different actions.
enum class SourcePromoteNodeKind {
  Unsupported,
  NormalOp,           // add/sub/mul/and/or/xor/select/phi
  ReplaceWithOperand, // trunc
  Compare,            // icmp
};

// Behavior for users in the source promote plan
enum class UserStrategy { Reject, AcceptAndPush, AcceptNoPush };

struct ExtRewriteGraph {

  // The cast Instruction.
  CastInst *Ext = nullptr;

  // Op0 of the cast
  Value *Source = nullptr;

  unsigned NarrowWidth = 0;
  unsigned WideWidth = 0;

  MapVector<Value *, unsigned> Nodes;

  // Casts where we replace with their ops during the rewrite.
  SmallPtrSet<Instruction *, 8> InstsToReplaceWithOperand;

  // Mark nodes that we are visiting to prevent cycles.
  SmallPtrSet<Instruction *, 8> Visiting;

  // Uses that possibly cannot be rewritten to NarrowWidth but
  // they can be repaired with a different rewrite.
  SmallVector<BoundaryRewrite, 4> BoundaryRewrites;

  // Uses that demanded the high bits
  SmallVector<Use *, 4> WideUses;

  // Instructions that can be removed if the RewriteGraph is valid
  SmallVector<Instruction *, 4> InstsToRemove;
};

// A trunc-root plan widens the narrow computation following a trunc back to
// the trunc's source width.
//
// The graph ends at sext/zext users. Each exit reconstructs the original
// extension of the narrow result, using a mask or sign-extension repair when
// needed. Repairs are omitted when the required upper bits are already known.
//
// Before:
// %t = trunc i64 %x to i32
// %a = add i32 %t, 1
// %r = zext i32 %a to i64
//

// After, if profitable:
// %a.wide = add i64 %x, 1
// %r = and i64 %a.wide, 4294967295
//
struct TruncRootGraph {
  // We start with a trunc as the root of the graph
  TruncInst *Root = nullptr;

  // What is the FillType of the graph?
  IntegerFillType Fill = ZEROS;
  unsigned NarrowWidth = 0;
  unsigned WideWidth = 0;

  SmallPtrSet<LoadInst *, 8> LoadsToExtend;

  // Preserve discovery order when transferring nodes to the rewrite plan.
  SmallSetVector<Instruction *, 8> Nodes;

  // We treat cast Instructions as exits, those can re extend the expression
  // to a new width.
  SmallVector<CastInst *, 4> Exits;

  SmallVector<Instruction *, 8> InstsToRemove;
};

class PromoteConstraintSolver;
struct SourcePromoteNewlyExtsInfo;

// For %v = and i16 %x, 255
// zext(v) and sext(v) produce the same bits.
// Try to capture that.
static IntegerFillType mergeValueFills(Value *V, IntegerFillType Curr,
                                       IntegerFillType Candidate,
                                       const WIAInfo &Info) {
  if (((Curr == SIGN && Candidate == ZEROS) ||
       (Curr == ZEROS && Candidate == SIGN)) &&
      Info.IsKnownNonNeg(V))
    return Curr;

  return chooseBestFill(Curr, Candidate);
}

struct WideProfit;
struct SourcePromoteProfitDecision;

/// @brief
/// This class performs an additional analysis for each extension CI.
/// If the extended bits are used downstream it tries to promote all
/// the relevant instructions of CI to a greater width with an
/// appropriate fillType for each instruction.
/// This allows us to legally drop the extension.
/// If the extended bits are not used downstream we can simply build a narrow
/// graph while trying to remove the extension.
class ExtRewritePlanner {
  DemandedBits &DB;
  const TargetTransformInfo *TTI = nullptr;
  const TargetLowering *TLI = nullptr;
  const DataLayout *DL;
  LoopInfo *LI = nullptr;
  WIAInfo WiaInfo;
  bool OptimizeForSize = false;
  OptimizationRemarkEmitter *ORE = nullptr;
  WIATuplesMap SharedLegalTuples;

  SmallPtrSet<CastInst *, 16> CoveredExts;
  SmallPtrSet<Instruction *, 32> NonDecreasingCostNodes;
  DenseMap<Instruction *, uint8_t> ICmpSourceFacts;

  // Try to find a reason to explore a promotion for an ICmp graph.
  uint8_t getICmpSourceFacts(Instruction *I, unsigned &Budget,
                             SmallPtrSetImpl<Instruction *> &Active);

  static constexpr unsigned MaxGraphNodes = 96;
  static constexpr unsigned MaxGraphUses = 16;
  static constexpr unsigned MaxICmpGraphNodes = 15;
  static constexpr unsigned MaxICmpGraphPopval = 96;
  unsigned ICmpGraphPopvalCount = 0;

public:
  ExtRewritePlanner(DemandedBits &DB, const TargetTransformInfo *TTI,
                    const TargetLowering *TLI, const DataLayout *DL,
                    LoopInfo *LI, WIAInfo WiaInfo, LLVMContext &Ctx,
                    bool OptimizeForSize, OptimizationRemarkEmitter *ORE)
      : DB(DB), TTI(TTI), TLI(TLI), DL(DL), LI(LI), WiaInfo(WiaInfo),
        OptimizeForSize(OptimizeForSize), ORE(ORE), Ctx(Ctx) {}

  // Analyze functions to determine if we can drop the cast or not.
  // They also provide us a plan on how to drop if we can drop it.
  std::optional<ExtRewritePlan> analyze(Instruction *Ext);
  std::optional<ExtRewritePlan> analyzeExt(CastInst *Ext);
  std::optional<ExtRewritePlan> analyzeTrunc(TruncInst *Trunc);
  std::optional<ExtRewritePlan> analyzeICmpRoot(ICmpInst *ICmp);

private:
  LLVMContext &Ctx;

  // To capture already extended loads not re extend them if one has multiple
  // uses.
  struct ExistingLoadExtInfo {
    CastInst *Ext;
    unsigned DstWidth;
    IntegerFillType Fill;
  };

  // Count extensions that are folding into a load
  // This matters for some improvements.
  struct ExtLoadMats {
    unsigned Count = 0;
    bool AllInLoadBlock = true;
  };

  // Capture extended loads to not extended them again.
  DenseMap<LoadInst *, SmallVector<ExistingLoadExtInfo>> AlreadyExtLoads;

  // Checks if this load has another recorded extension with the requested
  // fill and width not including the CurrentExt.
  bool hasExistingLoadExt(LoadInst *LI, unsigned DstWidth, IntegerFillType Fill,
                          CastInst *CurrentExt);

  // Checks if the extended bits are used by the Use U.
  bool extBitsUsedByUse(Use &U, unsigned SrcBits, unsigned DstBits) const;
  // Checks if V is zero above NarrowWidth.
  bool isKnownZeroAbove(Value *V, unsigned NarrowWidth) const;
  // Checks if V is sign extended above NarrowWidth.
  bool isKnownSignExtended(Value *V, unsigned NarrowWidth) const;

  // Find all available integer filltypes for the operands of I
  // having an appropriate ResultFill as a filltype
  bool chooseOpsFills(Instruction *I, IntegerFillType ResultFill,
                      SmallVectorImpl<IntegerFillType> &OpFills);

  // Try to combine the IntegerFillType result from V if it has an existing Fill
  // With what is required at this expression (NewFill).
  MergeFillResult mergeFill(SourcePromoteGraph &G, Value *V,
                            IntegerFillType NewFill);

  // Check if this Fill is not PENDING or UNDEFINED
  bool isResolvedFill(IntegerFillType Fill);

  // Check if an ExtLoad can be free
  bool isExtLoadFree(const LoadInst *Load, Type *ExtTy, IntegerFillType ExtFill,
                     const DataLayout &DL) const;

  // Recognize small, load trees under a low-bit mask.
  // The DAGcombiner can narrow the loads by pushing the mask
  // into them, making WIA source promotion redundant.
  bool isMaskedLoadTree(const CastInst *Ext) const;

  // Wide graph functions use when extended bits are used downstream
  SourcePromoteNodeKind getSupportedSourcePromoteKind(Instruction *I);

  // Try to build the promote graph around ext, this allows us to possibly
  // drop the extension if are successful
  bool buildSourcePromoteGraph(CastInst *Ext, SourcePromoteGraph &G);

  // Choose the root fill using extension facts, users, and target costs.
  IntegerFillType selectBestFill(CastInst *Ext);

  // Check if we can set Sign Fill from this Ext.
  bool canSetSignFill(Instruction *Ext);

  // Check if it's profitable to perform the promotion.
  SourcePromoteProfitDecision
  getSourcePromoteProfitDecision(const SourcePromoteGraph &G,
                                 const PromoteConstraintSolver &Solver);

  void noteSourcePromoteReject(SourcePromoteGraph &G, StringRef Reason,
                               Value *V = nullptr, Instruction *User = nullptr,
                               IntegerFillType Fill = UNDEFINED) const;

  // Get the profit from the possible WideGraph promotion
  WideProfit getWideGraphProf(const SourcePromoteGraph &G,
                              const PromoteConstraintSolver &Solver);

  // Try to build the graph to be promoted from the Value Source.
  bool tryToBuildSourcePromoteGraphFromValue(Value *Source,
                                             SourcePromoteGraph &G);
  // Helper functions to build the source promote graph.
  // Widening all the relevant instructions to the destination width
  // and updating all the uses.
  bool tryToBuildSourcePromoteGraphFromProducers(CastInst *Ext,
                                                 SourcePromoteGraph &G);
  // For Instruction I, try to add users of I into the worklist
  // to build the source promote graph.
  bool
  tryToBuildSourcePromoteGraphUsers(Instruction *I,
                                    SmallVectorImpl<Instruction *> &Worklist,
                                    SourcePromoteGraph &G);

  // Try to add the Instruction I into the graph
  // We need to be sure that I can be promoted
  // to the G.DstWidth with a fillType of I being ResultFill.
  bool tryToAddSourcePromoteNode(Instruction *I, SourcePromoteGraph &G,
                                 IntegerFillType ResultFill);
  // Try to add a trunc repair if we cannot add this user to the
  // promoted graph.
  bool tryToAddSourcePromoteBoundary(Use &U, SourcePromoteGraph &G);

  // Narrow graph functions use when extended bits are not used downstream
  bool buildNarrowGraph(CastInst *Ext, ExtRewriteGraph &G);

  // Check if the narrow graph is profitable
  bool isNarrowGraphProfitable(const ExtRewriteGraph &G);

  // Try to build the narrow graph.
  bool tryToBuildNarrowGraphFromUsers(CastInst *Ext, ExtRewriteGraph &G);

  // Try to add I and all reachable ops from I to the worklist.
  bool tryToAddTruncRootNode(Instruction *I, TruncRootGraph &G,
                             SmallVectorImpl<Value *> &Worklist, int Budget);
  // Check if CI is a cast Exit from the trunc expression
  // The dst cast ty must be of greater width than the truncated width.
  bool isTruncRootExit(CastInst *CI, const TruncRootGraph &G);
  // Check if the trunc root graph can safely use a SIGN fill when
  // it is widened.
  bool isTruncSignExtended(const TruncRootGraph &G);

  // Check if it's profitable to build the truncRootGraph.
  bool isTruncRootProfitable(const TruncRootGraph &G);

  // Try to add into the narrow graph Instruction I.
  bool tryToAddNarrowGraphNode(Instruction *I, ExtRewriteGraph &G,
                               SmallVectorImpl<Value *> &Worklist);

  // Some direct users of Ext appear to demanded the extended bits
  // but can still be kept in the narrow graph when the extension
  // matches the op
  bool tryToKeepDemandedExtUseNarrowed(Use &U, ExtRewriteGraph &G,
                                       SmallVectorImpl<Value *> &Worklist);
  // Treat Trunc and ICmp as boundaries for now.
  // This is simpler.
  bool handleTruncBoundary(Use &U, TruncInst *TR, ExtRewriteGraph &G);
  bool handleICmpBoundary(Use &U, ICmpInst *ICmp, ExtRewriteGraph &G);

  // Try to build the truncated graph.
  static constexpr int maxTruncGraphAttempts = 128;
  bool tryToBuildTruncGraphFromUsers(TruncInst *Trunc, TruncRootGraph &G);
  bool isSupportedNarrowTruncOp(Instruction *I, unsigned NarrowWidth);

  // When we have a narrow signed icmp instruction of the following form
  // %add = add i8 %a, C1
  // %cmp = icmp sge i8 %add, C2
  //
  // We can handle wrapping comparisons and maybe we can
  // promote too, if this proves to be profitable.
  // We use a similar trick to typePromotion's logic
  // but we need to convert the signed comparison
  // to an unsigned one, because this allows
  // to turn the constants to unsigned forms
  // with some adjucements,
  // so the ICmp can remain legal.
  bool isSafeICmpWrap(Instruction *I, ICmpInst *ICmp, unsigned NarrowWidth,
                      unsigned PromotedWidth);

  // Try to build a small ICmp Plan to bypass some expensive functions
  // that are needed otherwise.
  std::optional<ExtRewritePlan> buildSmallICmpPlan(ICmpInst *ICmp,
                                                   Instruction *Source,
                                                   unsigned int PromotedWidth);

  // Find the best width for when we promote the ICmp instruction
  // to a greater width.
  unsigned int chooseBestWidth(Instruction *I, unsigned int InitialWidth);

  // Check if can we add this User to the Worklist.
  UserStrategy getSourcePromoteUserStrategy(Instruction *VUser,
                                            SourcePromoteGraph &G);

  // Return the number of inserted extensions on regular and icmp instructions.
  SourcePromoteNewlyExtsInfo
  getSourcePromoteNewlyExtsInfo(const SourcePromoteGraph &G,
                                const PromoteConstraintSolver &Solver);

  // Create the source promotion plan
  ExtRewritePlan makeSourcePromotePlan(SourcePromoteGraph &G,
                                       PromoteConstraintSolver &Solver);

  // Build an ExtREwritePlan from a SourcePromoteGraph.
  std::optional<ExtRewritePlan> buildSequencePlan(SourcePromoteGraph &PromG,
                                                  Instruction *Root);

  // Check if this ICmp is candidate to be promoted.
  bool isICmpCandidate(ICmpInst *ICmp, Instruction *Source);

  // Recognize loop counters with a constant start and step, used only by
  // their update and a branch comparison against a constant.
  bool isPureLoopCounter(ICmpInst *ICmp, Instruction *Source);

  // Try to build the icmp root graph for narrow ICmps
  // DAGCombiner inserts extensions to promote those.
  bool buildICmpRootGraph(ICmpInst *ICmp, Instruction *Source,
                          SourcePromoteGraph &G, IntegerFillType Fill,
                          unsigned PromotedWidth);

  // We can't drop the extension here, simple plan keeps it.
  ExtRewritePlan makeSimplePlan(CastInst *Ext);
  // Make an ExtRewritePlan from a Graph.
  ExtRewritePlan makeGraphPlan(ExtRewriteGraph &&G, ExtStrategy Strategy);
  // Make a plan to drop a trunc and possible an extension.
  ExtRewritePlan makeTruncRootPlan(TruncRootGraph &&G);

  // Plan a proven signed clamp following a logical shift and truncation.
  std::optional<ExtRewritePlan> tryToBuildSignedClampPlan(TruncInst *Trunc);

  // Split extension uses when some accept SIGN and others require ZEROS.
  std::optional<ExtRewritePlan> tryToBuildSplitExtUsesPlan(CastInst *Ext);
};

// if the demanded mask has zero across all newly-extended high bits,
// then this use does not need the extension.
bool ExtRewritePlanner::extBitsUsedByUse(Use &U, unsigned SrcBits,
                                         unsigned DstBits) const {
  auto *UI = dyn_cast<Instruction>(U.getUser());
  if (!UI || !UI->getType()->isIntOrIntVectorTy()) {
    return true;
  }

  // if we have a mask and the mask clears
  // all the upper bits, then the DstBits are not demanded.
  if (UI->getOpcode() == Instruction::And)
    if (auto *C = dyn_cast<ConstantInt>(UI->getOperand(1 - U.getOperandNo())))
      if (C->getValue().isIntN(SrcBits))
        return false;

  APInt DemandedMask = DB.getDemandedBits(&U);
  if (DemandedMask.countl_zero() >= DstBits - SrcBits) {
    return false;
  }
  return true;
}

static bool proofClampNoWrap(Value *X, TruncInst *Trunc, ConstantInt *C,
                             ConstantInt *ShiftAmnt, const DataLayout &DL) {
  unsigned W = Trunc->getSrcTy()->getIntegerBitWidth();
  unsigned N = Trunc->getDestTy()->getIntegerBitWidth();
  unsigned K = ShiftAmnt->getValue().getLimitedValue(W);

  // Require N + K <= W, so the low N bits contain no shift bits.
  // So, lshr and ashr agree in those bits.
  if (N >= W || K >= W || K > W - N)
    return false;

  SimplifyQuery SQ(DL);
  ConstantRange XRange = computeConstantRange(X, /*Signed*/ true, SQ);
  if (XRange.isEmptySet())
    return false;

  // Convert the known sign-bit count into a signed interval
  unsigned RequiredSignedWidth = W - ComputeNumSignBits(X, DL) + 1;
  APInt SignMin = APInt::getSignedMinValue(RequiredSignedWidth).sext(W);
  APInt SignMax = APInt::getSignedMaxValue(RequiredSignedWidth).sext(W);

  APInt XMin = APIntOps::smax(XRange.getSignedMin(), SignMin);
  APInt XMax = APIntOps::smin(XRange.getSignedMax(), SignMax);

  if (XMin.sgt(XMax))
    return false;

  // Use extra bits so the addition of C cannot overflow.
  unsigned CalcW = W + 2;
  APInt Min = XMin.ashr(K).sext(CalcW);
  APInt Max = XMax.ashr(K).sext(CalcW);

  APInt NarrowMin = APInt::getSignedMinValue(N).sext(CalcW);
  APInt NarrowMax = APInt::getSignedMaxValue(N).sext(CalcW);

  // Y must also fit in signed N bits.
  if (Min.slt(NarrowMin) || Max.sgt(NarrowMax))
    return false;

  APInt Const = C->getValue().sext(CalcW);

  // Prove the narrow addition cannot wrap in signed arithmetic
  // Otherwise widening would change its sign and the result.
  if ((Min + Const).slt(NarrowMin) || (Max + Const).sgt(NarrowMax))
    return false;

  return true;
}

struct SignedClampInfo {
  BinaryOperator *Shift;
  BinaryOperator *Add;
  IntrinsicInst *Low;
  IntrinsicInst *High;
};

// Match a sequence of the following form
// t = trunc to iN (lshr iW X, K)
// a = add iN t, C
// low = smax(a, 0)
// result = umin(low, L)
//
// check if trunc_N(lshr(X, K)) == trunc_N(ashr(X, K))
// This happens iff N + K <= W, meaning both shifts give the
// same low N bits.
//
// N bits: the bits where lshr and ashr may differ are discarded.
//
// Next, we prove that ashr(X, K) and ashr(X, K) + C
// both fit in signed N bits.
// This makes the truncation and narrow addition preserve their signed values.
// With noneg L, the widened smax/umin then computes the same result.
static std::optional<SignedClampInfo>
matchAndProveSignedClamp(TruncInst *Trunc, const DataLayout &DL) {
  Value *X = nullptr;
  ConstantInt *ShiftAmnt = nullptr;
  if (!match(Trunc->getOperand(0),
             m_LShr(m_Value(X), m_ConstantInt(ShiftAmnt))) ||
      !Trunc->hasOneUse())
    return std::nullopt;

  if (!Trunc->getOperand(0)->hasOneUse())
    return std::nullopt;

  auto *Add = dyn_cast<BinaryOperator>(*Trunc->user_begin());
  ConstantInt *C = nullptr;

  // trunc --> add constant
  if (!Add || !Add->hasOneUse() ||
      !match(Add, m_c_Add(m_Specific(Trunc), m_ConstantInt(C))))
    return std::nullopt;

  // add --> smax(add, 0)

  auto *Low = dyn_cast<IntrinsicInst>(*Add->user_begin());
  if (!Low || !Low->hasOneUse() ||
      !match(Low, m_SMax(m_Specific(Add), m_Zero())))
    return std::nullopt;

  // smax -> umin(smax, constant)
  auto *High = dyn_cast<IntrinsicInst>(*Low->user_begin());
  ConstantInt *Limit = nullptr;
  if (!High || !match(High, m_UMin(m_Specific(Low), m_ConstantInt(Limit))) ||
      Limit->isNegative())
    return std::nullopt;

  auto *ShiftI = dyn_cast<BinaryOperator>(Trunc->getOperand(0));
  if (!ShiftI)
    return std::nullopt;

  if (!proofClampNoWrap(X, Trunc, C, ShiftAmnt, DL))
    return std::nullopt;

  return SignedClampInfo{ShiftI, Add, Low, High};
}

std::optional<ExtRewritePlan>
ExtRewritePlanner::tryToBuildSignedClampPlan(TruncInst *Trunc) {

  auto ClampInfo = matchAndProveSignedClamp(Trunc, *DL);
  if (!ClampInfo)
    return std::nullopt;

  auto [ShiftI, Add, Low, High] = *ClampInfo;

  ExtRewritePlan Plan;
  // Preserver the original type for other users.
  for (Use &U : High->uses()) {
    auto *UI = dyn_cast<Instruction>(U.getUser());
    if (!UI)
      return std::nullopt;

    BoundaryRewrite Rewrite;
    Rewrite.ExtUse = &U;
    Rewrite.User = UI;
    Rewrite.Kind = RepairNarrowValue;
    Rewrite.ResultTy = High->getType();
    Rewrite.OriginalValue = High;
    Plan.BoundaryRewrites.push_back(Rewrite);
  }

  Plan.Kind = ExtStrategy::PromoteSource;
  // Use the upper limit as the source.
  Plan.Ext = High;
  Plan.Source = High;

  Plan.NarrowWidth = Trunc->getDestTy()->getIntegerBitWidth();
  Plan.WideWidth = Trunc->getSrcTy()->getIntegerBitWidth();
  Plan.SourcePromoteFill = SIGN;
  Plan.TruncToReplaceWithAShr = Trunc;
  Plan.RequiredFill[Trunc] = SIGN;

  Instruction *ToPromoteNodes[] = {Add, Low, High};
  for (Instruction *I : ToPromoteNodes) {
    Plan.InstsToPromote.insert(I);
    Plan.RequiredFill[I] = SIGN;
  }

  Plan.InstsToRemove.push_back(Trunc);
  Plan.InstsToRemove.push_back(ShiftI);

  return Plan;
}

ExtRewritePlan ExtRewritePlanner::makeTruncRootPlan(TruncRootGraph &&G) {
  ExtRewritePlan Plan;
  Plan.Kind = ExtStrategy::PromoteTruncSource;
  Plan.Ext = G.Root;
  Plan.Source = G.Root->getOperand(0);
  Plan.NarrowWidth = G.NarrowWidth;
  Plan.WideWidth = G.WideWidth;

  IntegerFillType Fill = isTruncSignExtended(G) ? SIGN : ZEROS;
  Plan.TruncRootFill = Fill;

  Plan.InstsToPromote = std::move(G.Nodes);
  Plan.InstsToRemove = std::move(G.InstsToRemove);
  Plan.TruncRootExits = std::move(G.Exits);

  for (LoadInst *LI : G.LoadsToExtend)
    Plan.LoadsToExtend[LI] = Fill;

  return Plan;
}

bool ExtRewritePlanner::isKnownSignExtended(Value *V,
                                            unsigned NarrowWidth) const {
  KnownBits Known;
  WiaInfo.GetKnownBitsCached(V, Known);
  unsigned Width = Known.getBitWidth();
  if (NarrowWidth >= Width) {
    return true;
  }
  if (NarrowWidth == 0)
    return false;

  // if the minimum sign bits can cover the upper bits we are done.
  if (Known.countMinSignBits() >= Width - NarrowWidth + 1) {
    return true;
  }

  return false;
}

bool ExtRewritePlanner::isKnownZeroAbove(Value *V, unsigned NarrowWidth) const {

  KnownBits Known;
  WiaInfo.GetKnownBitsCached(V, Known);

  unsigned Width = Known.getBitWidth();
  APInt RequiredZeros = APInt::getBitsSet(Width, NarrowWidth, Width);
  return (RequiredZeros & ~Known.Zero).isZero();
}

bool ExtRewritePlanner::handleICmpBoundary(Use &U, ICmpInst *ICmp,
                                           ExtRewriteGraph &G) {
  auto CanPossiblyNarrowICmpOp = [&](Value *V) {
    if (isa<ConstantInt>(V))
      return true;
    if (V == G.Source || V == G.Ext)
      return true;
    if (auto *I = dyn_cast<Instruction>(V))
      return G.Nodes.contains(I);
    return V->getType()->getScalarSizeInBits() == G.NarrowWidth;
  };
  auto NarrowWidth = G.NarrowWidth;
  // icmp eq i64 %ext, other : we get the other
  Value *ICmpOp = ICmp->getOperand(U.getOperandNo() ^ 1);
  if (!CanPossiblyNarrowICmpOp(ICmpOp)) {
    // TODO: Later on we can insert a truncation
    // if the upper bits are known zero and the truncation is basically free.
    return false;
  }

  auto CastFill = G.Ext->getOpcode() == Instruction::ZExt ? ZEROS : SIGN;

  if (!ICmp->isIntPredicate() || (ICmp->isSigned() && CastFill == ZEROS))
    return false;

  // If the fillType changes, reject here early.
  if (U.get() != G.Ext) {
    bool HasMatchingFill = CastFill == ZEROS
                               ? isKnownZeroAbove(U.get(), NarrowWidth)
                               : isKnownSignExtended(U.get(), NarrowWidth);
    if (!HasMatchingFill)
      return false;
  }

  bool OpIsValid = false;
  // FillType must match the constant or known bits
  if (auto *CI = dyn_cast<ConstantInt>(ICmpOp)) {
    OpIsValid = CastFill == ZEROS ? CI->getValue().isIntN(NarrowWidth)
                                  : CI->getValue().isSignedIntN(NarrowWidth);
  } else {
    OpIsValid = CastFill == ZEROS ? isKnownZeroAbove(ICmpOp, NarrowWidth)
                                  : isKnownSignExtended(ICmpOp, NarrowWidth);
  }
  if (!OpIsValid)
    return false;

  auto Kind = BoundaryRewriteKind::NarrowICmp;
  G.BoundaryRewrites.push_back(
      {&U, ICmp, Kind, ICmp->getOperand(0)->getType()});
  return true;
}

bool ExtRewritePlanner::handleTruncBoundary(Use &U, TruncInst *TR,
                                            ExtRewriteGraph &G) {
  // newly extended bits from Ext are not used by all users,
  // we have a trunc i32 %ext to i8 or something.
  auto CI = cast<CastInst>(TR);
  auto DstTy = CI->getDestTy()->getScalarSizeInBits();
  auto NarrowWidth = G.NarrowWidth;

  BoundaryRewriteKind Kind;
  if (DstTy == NarrowWidth) {
    // if we manage to build the graph it can be dropped,
    // otherwise not.
    Kind = BoundaryRewriteKind::ReplaceWithSource;
    G.InstsToRemove.push_back(TR);
  } else if (DstTy < NarrowWidth) {
    // Since the truncated bits are not demanded,
    // otherwise this will be a WideUse,
    // we can replace the old trunc with a new narrower trunc.
    Kind = BoundaryRewriteKind::TruncSource;
  } else if (U.get() == G.Ext) { // DstTy > NarrowWidth
    Kind = BoundaryRewriteKind::ExtSource;
  } else {
    // TODO: maybe consider extra cases here when DstTy > NarrowWidth
    // This can be somewhat complex for the Rebuilder.
    return false;
  }
  G.BoundaryRewrites.push_back({&U, TR, Kind, TR->getType()});
  // do not keep examining the users of trunc, treat as a boundary
  return true;
}

bool ExtRewritePlanner::tryToAddNarrowGraphNode(
    Instruction *I, ExtRewriteGraph &G, SmallVectorImpl<Value *> &Worklist) {

  if (G.Nodes.contains(I))
    return true;

  // Everything else gets rejected below.
  if (!I->isBinaryOp() && !isa<SelectInst, PHINode>(I))
    return false;

  // This is in progress do not visit it again.
  if (G.Visiting.contains(I))
    return true;

  // Mark the current visited, to prevent cycles this is needed for PHIs.
  G.Visiting.insert(I);

  // Use a guard to remove the current visited instead of doing it
  // in every return
  auto Guard = llvm::scope_exit([&] { G.Visiting.erase(I); });

  // G Visiting is needed for deep chains.
  if ((G.Nodes.size() + G.Visiting.size()) >= MaxGraphNodes)
    return false;

  auto NarrowWidth = G.NarrowWidth;
  auto WideWidth = G.WideWidth;

  switch (I->getOpcode()) {
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Mul:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor:
  case Instruction::Select:
  case Instruction::PHI:
    break;
  case Instruction::URem:
  case Instruction::UDiv: {
    if (WiaInfo.HasAppropriateSolution(I->getOperand(0), NarrowWidth, ZEROS) &&
        WiaInfo.HasAppropriateSolution(I->getOperand(1), NarrowWidth, ZEROS))
      break;

    if (isKnownZeroAbove(I->getOperand(0), NarrowWidth) &&
        isKnownZeroAbove(I->getOperand(1), NarrowWidth))
      break;

    return false;
  }
  case Instruction::SRem:
  case Instruction::SDiv: {
    // we need SIGN here
    if (G.Ext->getOpcode() != Instruction::SExt)
      return false;

    if (I->getOperand(0) != G.Ext)
      return false;

    auto *CI = dyn_cast<ConstantInt>(I->getOperand(1));
    // Only constants are supported for now.
    // Reject zero and -1 to avoid division by zero
    // or signed overflow at the narrower width.
    if (!CI)
      return false;

    if (CI->isZero() || CI->isMinusOne()) {
      return false;
    }
    if (!CI->getValue().isSignedIntN(NarrowWidth)) {
      return false;
    }
    // add more cases
    break;
  }

  case Instruction::Shl: {
    // We need every possible shifts amount to be below NarrowWidth.
    if (auto *C = dyn_cast<ConstantInt>(I->getOperand(1))) {
      if (!C->getValue().ult(NarrowWidth)) {
        return false;
      }
    } else {
      KnownBits Known;
      WiaInfo.GetKnownBitsCached(I->getOperand(1), Known);
      if (Known.getMaxValue().getLimitedValue(WideWidth) >= NarrowWidth) {
        return false;
      }
    }
    break;
  }
  case Instruction::LShr: {
    // TODO: to avoid computing KnownBits, we might already have cached
    // those from the forward analysis, make a function to get it here.

    // ShiftAmnt must be less than NarrowWidth, otherwise we might get poison.
    // Any discarded high bits that can possibly shift into the retained low
    // bits must be known zeros.
    if (auto ShiftAmnt = dyn_cast<ConstantInt>(I->getOperand(1))) {
      auto Amnt = ShiftAmnt->getValue().getLimitedValue(WideWidth);
      if (Amnt >= NarrowWidth) {
        return false;
      }
      unsigned HiBitPos = std::min<unsigned>(WideWidth, NarrowWidth + Amnt);
      APInt RequiredZeros = APInt::getBitsSet(WideWidth, NarrowWidth, HiBitPos);
      KnownBits KnownOp0;
      WiaInfo.GetKnownBitsCached(I->getOperand(0), KnownOp0);
      if (!(RequiredZeros & ~KnownOp0.Zero).isZero())
        return false;

    } else {
      KnownBits Amnt;
      WiaInfo.GetKnownBitsCached(I->getOperand(1), Amnt);
      auto Max = Amnt.getMaxValue().getLimitedValue(WideWidth);
      if (Max >= NarrowWidth) {
        return false;
      }
      unsigned HiBitPos = std::min<unsigned>(WideWidth, NarrowWidth + Max);
      APInt RequiredZeros = APInt::getBitsSet(WideWidth, NarrowWidth, HiBitPos);
      KnownBits Known0;
      WiaInfo.GetKnownBitsCached(I->getOperand(0), Known0);
      if (!(RequiredZeros & ~Known0.Zero).isZero())
        return false;
    }
  } break;
  default: {
    // TODO: add more here like ashr/icmp and others. (sext/zext?)
    // ++NumExtNarrowGraphRejectedUnsupportedOpcode;
    return false;
  }
  }

  // Check if every operand can participate in the narrow graph.
  for (auto [OpIdx, OpUse] : enumerate(I->operands())) {
    Value *Op = OpUse.get();
    if (I->getOpcode() == Instruction::Select && OpIdx == 0)
      continue;

    if (!Op->getType()->isIntegerTy())
      return false;

    if (Op == G.Source || Op == G.Ext)
      continue;

    if (G.Nodes.contains(Op))
      continue;

    if (isa<ConstantInt>(Op))
      continue;

    if (Op->getType()->getScalarSizeInBits() == G.NarrowWidth)
      continue;

    if (auto CI = dyn_cast<CastInst>(Op);
        CI && isExtOpcode(CI) &&
        (CI->getSrcTy()->isIntegerTy(G.NarrowWidth) &&
         CI->getDestTy()->isIntegerTy(G.WideWidth) && G.Ext &&
         CI->getOpcode() == G.Ext->getOpcode())) {
      G.InstsToReplaceWithOperand.insert(CI);
      continue;
    }

    if (auto *OpI = dyn_cast<Instruction>(Op)) {
      if (!tryToAddNarrowGraphNode(OpI, G, Worklist))
        return false;
      continue;
    }
    return false;
  }

  G.Nodes.insert({I, G.Nodes.size()});
  Worklist.push_back(I);

  return true;
}

bool ExtRewritePlanner::isSupportedNarrowTruncOp(Instruction *I,
                                                 unsigned NarrowWidth) {

  if (!I->getType()->isIntegerTy(NarrowWidth))
    return false;

  switch (I->getOpcode()) {
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Mul:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor:
  case Instruction::Select:
    return true;
  case Instruction::Shl: {
    auto *ShiftAmnt = I->getOperand(1);
    if (auto *CI = dyn_cast<ConstantInt>(ShiftAmnt)) {
      return CI->getValue().getLimitedValue() < NarrowWidth;
    }
    // TODO: add support for non constant shift amount.
    return false;
  }
  default:
    // TODO: add support for more opcodes here
    return false;
  }
}

bool ExtRewritePlanner::isTruncSignExtended(const TruncRootGraph &G) {
  if (!G.Root->hasNoSignedWrap())
    return false;

  for (CastInst *Exit : G.Exits) {
    if (!isa<SExtInst>(Exit) &&
        !(isa<ZExtInst>(Exit) && cast<ZExtInst>(Exit)->hasNonNeg()))
      return false;
  }

  for (Instruction *I : G.Nodes) {
    switch (I->getOpcode()) {
    case Instruction::And:
    case Instruction::Or:
    case Instruction::Xor:
      continue;
    case Instruction::Add:
    case Instruction::Sub:
    case Instruction::Mul:
      if (cast<BinaryOperator>(I)->hasNoSignedWrap())
        continue;
      return false;
    default:
      // TODO: add more opcodes here: lshr/shr/ashr/phis/divs/rems
      return false;
    }
  }
  return true;
}

bool ExtRewritePlanner::isTruncRootExit(CastInst *CI, const TruncRootGraph &G) {

  if (!isa<ZExtInst>(CI) && !isa<SExtInst>(CI))
    return false;

  if (!CI->getSrcTy()->isIntegerTy(G.NarrowWidth))
    return false;

  if (!CI->getDestTy()->isIntegerTy())
    return false;

  unsigned ExitWidth = CI->getDestTy()->getScalarSizeInBits();
  return ExitWidth > G.NarrowWidth && ExitWidth <= G.WideWidth;
}

bool ExtRewritePlanner::tryToAddTruncRootNode(
    Instruction *I, TruncRootGraph &G, SmallVectorImpl<Value *> &Worklist,
    int Budget) {

  if (Budget <= 0)
    return false;

  if (isa<TruncInst>(I))
    return false;

  if (!isSupportedNarrowTruncOp(I, G.NarrowWidth))
    return false;

  for (auto [OpIdx, OpUse] : enumerate(I->operands())) {

    if (isa<SelectInst>(I) && OpIdx == 0)
      continue;

    Value *Op = OpUse.get();

    if (Op == G.Root || isa<ConstantInt>(Op))
      continue;

    if (auto *OpI = dyn_cast<Instruction>(Op)) {
      if (G.Nodes.contains(OpI))
        continue;

      if (auto *LI = dyn_cast<LoadInst>(OpI)) {
        if (!LI->getType()->isIntegerTy(G.NarrowWidth))
          return false;
        if (!LI->isSimple() || !LI->hasOneUse())
          return false;

        G.LoadsToExtend.insert(LI);
        continue;
      }

      if (!tryToAddTruncRootNode(OpI, G, Worklist, --Budget))
        return false;

      continue;
    }

    if (Op->getType()->isIntegerTy(G.NarrowWidth))
      continue;

    return false;
  }

  if (G.Nodes.insert(I))
    Worklist.push_back(I);

  return true;
}

bool ExtRewritePlanner::tryToBuildTruncGraphFromUsers(TruncInst *Trunc,
                                                      TruncRootGraph &G) {

  SmallVector<Value *, 128> Worklist;
  int Budget = maxTruncGraphAttempts;

  Worklist.push_back(Trunc);
  while (!Worklist.empty()) {
    Value *V = Worklist.pop_back_val();

    for (const auto &U : V->uses()) {
      auto *User = U.getUser();
      auto *UserI = dyn_cast<Instruction>(User);
      if (!UserI)
        return false;

      if (isa<SExtInst>(User) || isa<ZExtInst>(User)) {
        auto *CI = cast<CastInst>(User);
        if (isTruncRootExit(CI, G)) {
          G.Exits.push_back(cast<CastInst>(User));
          continue;
        }

        // can we relax this?
        return false;
      }
      if (!tryToAddTruncRootNode(UserI, G, Worklist, --Budget))
        return false;
    }
  }
  if (G.Exits.empty())
    return false;

  G.InstsToRemove.push_back(Trunc);

  for (CastInst *Exit : G.Exits)
    G.InstsToRemove.push_back(Exit);

  return true;
}

// A user can enter the narrow graph iff:
// 1. The user of Ext doesn't demand the high bits
// 2. The user has appropriate solution matching at the narrow Width, for the
// narrow graph
// 3. The user opcode is low-bit closed (low bits of result depend only on the
// low N bits of the operands)
// 4. every operand of the user can live at NarrowWidth
bool ExtRewritePlanner::tryToBuildNarrowGraphFromUsers(CastInst *Ext,
                                                       ExtRewriteGraph &G) {

  G.Source = Ext->getOperand(0);
  SmallVector<Value *, 128> Worklist;
  Worklist.push_back(Ext);
  auto NarrowWidth = G.NarrowWidth;
  auto WideWidth = G.WideWidth;
  unsigned NumUses = 0;
  while (!Worklist.empty()) {
    Value *I = Worklist.pop_back_val();
    for (Use &U : I->uses()) {
      if (++NumUses > MaxGraphUses) {
        return false;
      }
      User *Usr = U.getUser();
      auto *UI = dyn_cast<Instruction>(Usr);
      if (!UI)
        return false;

      if (auto *Tr = dyn_cast<TruncInst>(UI)) {
        if (!handleTruncBoundary(U, Tr, G)) {
          return false;
        }
        // skip operands of the trunc.
        continue;
      }

      if (auto *Icmp = dyn_cast<ICmpInst>(UI)) {
        if (!handleICmpBoundary(U, Icmp, G)) {
          return false;
        }
        continue;
      }

      if (!tryToAddNarrowGraphNode(UI, G, Worklist))
        return false;
    }
  }

  if (!isNarrowGraphProfitable(G))
    return false;

  auto CheckUses = [&](Value *I) {
    for (Use &U : I->uses()) {
      if (isa<TruncInst, ICmpInst>(U.getUser()))
        continue;

      if (extBitsUsedByUse(U, NarrowWidth, WideWidth)) {
        if (I == G.Ext && tryToKeepDemandedExtUseNarrowed(U, G, Worklist))
          continue;
        G.WideUses.push_back(&U);
        return false;
      }
    }
    return true;
  };

  for (const auto &[I, _] : G.Nodes)
    if (I != Ext && !CheckUses(I))
      return false;

  assert(Worklist.empty() && "Graph must already contain each user");

  return true;
}

bool ExtRewritePlanner::hasExistingLoadExt(LoadInst *LI, unsigned DstWidth,
                                           IntegerFillType Fill,
                                           CastInst *CurrentExt) {
  auto It = AlreadyExtLoads.find(LI);
  if (It == AlreadyExtLoads.end())
    return false;

  for (const auto &Entry : It->second) {
    if (Entry.Ext == CurrentExt)
      continue;

    if (Entry.DstWidth != DstWidth)
      continue;

    if (Entry.Fill == Fill)
      return true;
  }
  return false;
}

bool ExtRewritePlanner::canSetSignFill(Instruction *Ext) {
  auto *SrcI = dyn_cast<Instruction>(Ext->getOperand(0));
  if (!SrcI)
    return false;

  if (auto *OBO = dyn_cast<OverflowingBinaryOperator>(SrcI))
    return OBO->hasNoSignedWrap();

  if (auto *TR = dyn_cast<TruncInst>(SrcI))
    return TR->hasNoSignedWrap();

  return false;
}

SourcePromoteNodeKind
ExtRewritePlanner::getSupportedSourcePromoteKind(Instruction *I) {
  Type *Ty = I->getType();
  if (I->getOpcode() == Instruction::ICmp)
    return SourcePromoteNodeKind::Compare;

  if (!isSupportedScalarIntType(Ty))
    return SourcePromoteNodeKind::Unsupported;

  switch (I->getOpcode()) {
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Mul:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor:
  case Instruction::PHI:
  case Instruction::Shl:
  case Instruction::LShr:
  case Instruction::AShr:
  case Instruction::UDiv:
  case Instruction::SDiv:
  case Instruction::SRem:
  case Instruction::URem:
    return SourcePromoteNodeKind::NormalOp;
  case Instruction::Select:
    if (isSupportedScalarIntType(I->getOperand(1)->getType()))
      return SourcePromoteNodeKind::NormalOp;
    return SourcePromoteNodeKind::Unsupported;
  case Instruction::Trunc:
    return SourcePromoteNodeKind::ReplaceWithOperand;
  case Instruction::Call:
  case Instruction::Invoke: {
    auto *II = dyn_cast<IntrinsicInst>(I);
    if (!II)
      return SourcePromoteNodeKind::Unsupported;

    switch (II->getIntrinsicID()) {
    case Intrinsic::umin:
    case Intrinsic::umax:
    case Intrinsic::smin:
    case Intrinsic::smax:
    case Intrinsic::abs:
    case Intrinsic::ctpop:
    case Intrinsic::cttz:
      return SourcePromoteNodeKind::NormalOp;
    default:
      return SourcePromoteNodeKind::Unsupported;
    }
  }
  }
  return SourcePromoteNodeKind::Unsupported;
}

bool ExtRewritePlanner::isResolvedFill(IntegerFillType Fill) {
  return Fill == SIGN || Fill == ZEROS || Fill == ANYTHING;
}

bool ExtRewritePlanner::chooseOpsFills(
    Instruction *I, IntegerFillType Fill,
    SmallVectorImpl<IntegerFillType> &OpFills) {
  if (isa<PHINode>(I)) {
    OpFills.assign(I->getNumOperands(), Fill);
    return true;
  }

  const unsigned Opc = I->getOpcode();
  if ((Fill == ZEROS || Fill == ANYTHING) &&
      (Opc == Instruction::LShr || Opc == Instruction::UDiv ||
       Opc == Instruction::URem)) {
    assignOpFills(I, ZEROS, ZEROS, OpFills);
    return true;
  }

  if ((Fill == SIGN || Fill == ANYTHING) &&
      (Opc == Instruction::AShr || Opc == Instruction::SDiv ||
       Opc == Instruction::SRem)) {
    assignOpFills(I, SIGN, Opc == Instruction::AShr ? ZEROS : SIGN, OpFills);
    return true;
  }

  if (auto *II = dyn_cast<IntrinsicInst>(I);
      II && (Fill == ZEROS || Fill == ANYTHING)) {
    auto ID = II->getIntrinsicID();

    if (ID == Intrinsic::abs || ID == Intrinsic::ctpop) {
      OpFills.assign(I->getNumOperands(), ANYTHING);
      OpFills[0] = ID == Intrinsic::abs ? SIGN : ZEROS;
      return true;
    }
  }

  auto Key = std::make_pair(I, Fill);
  auto It = SharedLegalTuples.find(Key);

  if (It == SharedLegalTuples.end()) {
    SmallVector<FillTuple, 4> Tuples;
    bool Valid = WiaInfo.ChooseOpsFills(I, Fill, OpFills, &Tuples);
    if ((Valid && !Tuples.empty()) ||
        (!Valid && (I->getOpcode() != Instruction::Shl || Fill != ZEROS)))
      SharedLegalTuples.try_emplace(Key, std::move(Tuples));
    return Valid;
  }

  const auto &Tuples = It->second;
  if (Tuples.empty())
    return false;

  OpFills.assign(I->getNumOperands(), ANYTHING);
  if (Tuples.size() == 1) {
    const auto &[L, R, Produced] = Tuples.front();
    assignOpFills(I, L, R, OpFills);
  } else {
    assignOpFills(I, PENDING, PENDING, OpFills);
  }
  return true;
}

MergeFillResult ExtRewritePlanner::mergeFill(SourcePromoteGraph &G, Value *V,
                                             IntegerFillType NewFill) {

  if (NewFill == UNDEFINED)
    return MergeFillResult::Conflict;

  if (NewFill == ANYTHING)
    return MergeFillResult::Unchanged;

  auto It = G.States.try_emplace(V).first;
  if (It->second.Required == UNDEFINED) {
    It->second.Required = NewFill;
    return MergeFillResult::Changed;
  }
  auto OldFill = It->second.Required;
  IntegerFillType Merged = mergeValueFills(V, OldFill, NewFill, WiaInfo);
  if (Merged == UNDEFINED)
    return MergeFillResult::Conflict;

  if (Merged == OldFill)
    return MergeFillResult::Unchanged;

  It->second.Required = Merged;
  return MergeFillResult::Changed;
}

bool ExtRewritePlanner::isMaskedLoadTree(const CastInst *Ext) const {
  Value *Tree;
  const APInt *Mask;
  if (!isa<ZExtInst, SExtInst>(Ext) || !Ext->getType()->isIntegerTy() ||
      !match(Ext->getOperand(0), m_c_And(m_Value(Tree), m_APInt(Mask))) ||
      !Mask->isMask())
    return false;

  unsigned Bits = Mask->getActiveBits();
  if (Bits < 8 || !isPowerOf2_32(Bits) || Bits >= Mask->getBitWidth())
    return false;

  SmallVector<Value *, 8> Worklist{Ext->getOperand(0)};
  unsigned Visited = 0;
  bool FoundLoad = false;
  while (!Worklist.empty()) {
    Value *V = Worklist.pop_back_val();
    // DAGcombiner handles constants either inside or outside the mask.
    if (isa<ConstantInt>(V))
      continue;

    auto *I = dyn_cast<Instruction>(V);
    if (!I || ++Visited > 8 || !I->hasOneUse() ||
        I->getParent() != Ext->getParent())
      return false;

    if (auto *LI = dyn_cast<LoadInst>(I)) {
      if (!LI->isSimple())
        return false;
      // We may need more checks here.
      FoundLoad = true;
      continue;
    }

    auto *BO = dyn_cast<BinaryOperator>(I);
    if (!BO || !BO->isBitwiseLogicOp())
      return false;
    Worklist.push_back(BO->getOperand(0));
    Worklist.push_back(BO->getOperand(1));
  }
  return FoundLoad;
}

/// Return true if \p Load and \p ExtTy can form an ExtLoad.
/// using extension type \p ExtFill.
/// For example, in AArch64
///   %L = load i8, i8* %ptr
///   %E = zext i8 %L to i32
/// can be lowered into one load instruction
///   ldrb w0, [x0]
bool ExtRewritePlanner::isExtLoadFree(const LoadInst *Load, Type *ExtTy,
                                      IntegerFillType ExtFill,
                                      const DataLayout &DL) const {

  EVT VT = TLI->getValueType(DL, ExtTy);
  EVT LoadVT = TLI->getValueType(DL, Load->getType());

  // If the load has other users and the truncate is not free, the ext
  // probably isn't free.
  if (!Load->hasOneUse() &&
      (TLI->isTypeLegal(LoadVT) || !TLI->isTypeLegal(VT)) &&
      !TLI->isTruncateFree(ExtTy, Load->getType()))
    return false;

  // Check whether the target supports casts folded into loads.
  unsigned LType;
  if (ExtFill == ZEROS || ExtFill == ANYTHING)
    LType = ISD::ZEXTLOAD;
  else if (ExtFill == SIGN) {
    LType = ISD::SEXTLOAD;
  }

  return TLI->isLoadLegal(VT, LoadVT, Load->getAlign(),
                          Load->getPointerAddressSpace(), LType, false);
}

bool ExtRewritePlanner::tryToAddSourcePromoteNode(Instruction *I,
                                                  SourcePromoteGraph &G,
                                                  IntegerFillType ResultFill) {
  IntegerFillType RequiredFill = ResultFill;
  unsigned RequiredWidth = G.DstWidth;

  unsigned Limit = G.IsICmpRoot ? MaxICmpGraphNodes : MaxGraphNodes;
  if (G.Nodes.size() >= Limit) {
    noteSourcePromoteReject(G, "GraphLimit", I, I);
    return false;
  }

  if (auto *ExtNode = dyn_cast<CastInst>(I);
      ExtNode && (isa<ZExtInst>(ExtNode) || isa<SExtInst>(ExtNode))) {
    if (!isSupportedScalarIntType(ExtNode->getSrcTy()) ||
        !isSupportedScalarIntType(ExtNode->getDestTy())) {
      return false;
    }

    auto ExtDstWidth = ExtNode->getDestTy()->getScalarSizeInBits();
    if (ExtDstWidth > RequiredWidth) {
      noteSourcePromoteReject(G, "NestedExtTooWide", ExtNode, ExtNode,
                              ResultFill);
      return false;
    }

    // A ZExt result is no negative, so sext or zext is fine here.
    IntegerFillType OperandFill = isa<ZExtInst>(ExtNode) ? ZEROS : SIGN;
    bool IsMatchingResultFill = ResultFill == ANYTHING ||
                                ResultFill == OperandFill ||
                                (ResultFill == SIGN && isa<ZExtInst>(ExtNode));

    if (!IsMatchingResultFill) {
      noteSourcePromoteReject(G, "NonMatchingResultFill", ExtNode, ExtNode,
                              ResultFill);
      return false;
    }

    if (ExtDstWidth == RequiredWidth) {
      G.WideLeaves.insert(I);
      return true;
    }
    Value *Op = ExtNode->getOperand(0);
    // If the Op isn't in the graph yet, try to extend it
    // if we decide to promote.
    bool IsSupportedOp =
        !isa<ConstantInt>(Op) && !isa<FreezeInst>(Op) && !isa<UndefValue>(Op);
    if (!G.containsNode(Op) && IsSupportedOp) {
      auto [It, Inserted] = G.ExternalOpsToExtend.try_emplace(Op, OperandFill);
      if (!Inserted && It->second != OperandFill)
        return false;
    }
    G.addNode(I);
    G.InstsToReplaceWithOperand.insert(ExtNode);
    return true;
  }

  unsigned int IWidth = I->getType()->getScalarSizeInBits();
  unsigned int OpWidth = IWidth;

  if (auto *ICmp = dyn_cast<ICmpInst>(I)) {
    auto Op0 = ICmp->getOperand(0);
    if (!isSupportedScalarIntType(Op0->getType()))
      return false;

    OpWidth = ICmp->getOperand(0)->getType()->getIntegerBitWidth();
  }

  if (OpWidth > G.DstWidth) {
    noteSourcePromoteReject(G, "NodeTooWide", I, I, ResultFill);
    return false;
  }

  if (IWidth == G.DstWidth) {
    G.WideLeaves.insert(I);
    return true;
  }

  switch (getSupportedSourcePromoteKind(I)) {
  case SourcePromoteNodeKind::NormalOp:
  case SourcePromoteNodeKind::Compare: {
    // We let PromoteConstraintSolver check for those
    G.addNode(I);
    return true;
  }
  case SourcePromoteNodeKind::ReplaceWithOperand: {
    auto *Trunc = dyn_cast<TruncInst>(I);
    if (!Trunc) {
      // same region can be planned twice, check for any changes here
      return false;
    }
    Value *Op = Trunc->getOperand(0);
    if (!Op->getType()->isIntegerTy()) {
      break;
    }

    if (!isSupportedScalarIntType(Trunc->getType()) ||
        !isSupportedScalarIntType(Op->getType()) ||
        !isSupportedScalarIntType(Type::getIntNTy(Ctx, G.DstWidth))) {
      return false;
    }

    unsigned OpWidth = Op->getType()->getScalarSizeInBits();
    unsigned TruncatedWidth = Trunc->getType()->getScalarSizeInBits();

    if (OpWidth != RequiredWidth) {
      noteSourcePromoteReject(G, "TruncOperandWidthMismatch", Op, Trunc,
                              ResultFill);
      break;
    }

    if (RequiredFill == PENDING) {
      // Let nodes enter the graph, in promoteConstraintSolver
      // we can check for legality, here it's very complex to check.
      G.addNode(I);
      G.InstsToReplaceWithOperand.insert(Trunc);
      return true;
    }

    bool CanReplaceWithOp = false;
    if (RequiredFill == ZEROS)
      CanReplaceWithOp =
          Trunc->hasNoUnsignedWrap() || isKnownZeroAbove(Op, TruncatedWidth);
    else if (RequiredFill == SIGN)
      CanReplaceWithOp =
          Trunc->hasNoSignedWrap() || isKnownSignExtended(Op, TruncatedWidth);

    if (!CanReplaceWithOp) {
      StringRef Msg = RequiredFill == SIGN ? "TruncOpNotKnownSignExtended"
                                           : "TruncOpNotKnownZeroExtended";
      noteSourcePromoteReject(G, Msg, I, I, ANYTHING);
      return false;
    }
    G.addNode(I);
    G.InstsToReplaceWithOperand.insert(Trunc);
    return true;
  }
  case SourcePromoteNodeKind::Unsupported: {
    noteSourcePromoteReject(G, "UnsupportedOpcode", I, I, UNDEFINED);
    // TODO: add support for other opcodes
    LLVM_DEBUG(dbgs() << "WIA source-promote unsupported opcode: "
                      << I->getOpcodeName() << " required-fill=" << RequiredFill
                      << " required-width=" << RequiredWidth << " in "
                      << I->getFunction()->getName() << " : " << *I << "\n");

    return false;
  }
  }
  return false;
}

UserStrategy
ExtRewritePlanner::getSourcePromoteUserStrategy(Instruction *VUser,
                                                SourcePromoteGraph &G) {
  if (G.containsNode(VUser)) {
    return UserStrategy::AcceptNoPush;
  }
  if (VUser == G.Ext) {
    return UserStrategy::AcceptNoPush;
  }

  if (auto *TI = dyn_cast<TruncInst>(VUser)) {
    Type *SrcTy = TI->getSrcTy();
    if (isSupportedScalarIntType(SrcTy) &&
        SrcTy->getScalarSizeInBits() < G.DstWidth)
      return UserStrategy::Reject;
  }

  if (isa<PHINode>(VUser))
    return UserStrategy::AcceptAndPush;

  if (auto *UserI = dyn_cast<Instruction>(VUser)) {
    switch (getSupportedSourcePromoteKind(UserI)) {
    case SourcePromoteNodeKind::NormalOp:
    case SourcePromoteNodeKind::Compare:
      return UserStrategy::AcceptAndPush;
    default:
      break;
    }
  }
  // can't promote stores, maybe we can truncate.
  if (isa<StoreInst>(VUser) || isa<GetElementPtrInst>(VUser) ||
      isa<CallInst>(VUser)) {
    return UserStrategy::Reject;
  }

  auto *SourceBO = dyn_cast<BinaryOperator>(G.Source);
  bool IsBitwiseSource = SourceBO && SourceBO->isBitwiseLogicOp();

  if (auto *TI = dyn_cast<TruncInst>(VUser);
      TI && !IsBitwiseSource &&
      TI->getType()->getScalarSizeInBits() != G.DstWidth &&
      TI->getSrcTy()->getScalarSizeInBits() > G.DstWidth)
    return UserStrategy::Reject;

  if (auto *Ext = dyn_cast<CastInst>(VUser);
      Ext && isa<SExtInst, ZExtInst>(Ext)) {
    unsigned ExtWidth = Ext->getDestTy()->getScalarSizeInBits();
    // Such a cast is rejected by node admission,
    // so reject it early here.
    if (ExtWidth > G.DstWidth)
      return UserStrategy::Reject;

    if (!isSupportedScalarIntType(Ext->getDestTy()))
      return UserStrategy::Reject;

    if (!IsBitwiseSource && ExtWidth < G.DstWidth &&
        Ext->getOperand(0) == G.Source) {
      Value *Op = Ext->getOperand(0);
      IntegerFillType OperandFill = isa<ZExtInst>(Ext) ? ZEROS : SIGN;
      IntegerFillType Required =
          G.getRequiredFill(Op, ANYTHING); // if we cannot prove compatibility
                                           // with this cast we reject it,
      // it might be handled as a boundary.
      if (mergeValueFills(Op, Required, OperandFill, WiaInfo) == UNDEFINED)
        return UserStrategy::Reject;
    }
  }

  // An unsupported user cannot enter the graph unless it can be kept narrow
  // for a bitwise source, admitted as a cast, or reused as a wide leaf.
  if (auto *UserI = dyn_cast<Instruction>(VUser)) {
    if (!isa<CastInst>(UserI) &&
        UserI->getType()->getScalarSizeInBits() != G.DstWidth &&
        !IsBitwiseSource)
      return UserStrategy::Reject;
  }

  if (isa<CastInst>(VUser) &&
      VUser->getType()->getScalarSizeInBits() > G.DstWidth)
    return UserStrategy::Reject;

  auto DstWidth = G.DstWidth;

  // We start with ANYTHING here.
  // Record it as weak requirement because we found a suitable DstWidth
  // If later someone wants different FillType from VUser it will be
  // become verified by the Constraint Solver.
  if (WiaInfo.HasAppropriateSolution(VUser, DstWidth, ANYTHING)) {
    return UserStrategy::AcceptAndPush;
  }
  return UserStrategy::Reject;
}

bool ExtRewritePlanner::tryToAddSourcePromoteBoundary(Use &U,
                                                      SourcePromoteGraph &G) {
  auto *UI = dyn_cast<Instruction>(U.getUser());
  if (!UI)
    return false;

  Type *UTy = U.get()->getType();
  if (!isSupportedScalarIntType(UTy))
    return false;

  // Only insert a boundary repair
  // when the oldTy is narrower than the promoted width.
  if (UTy->getScalarSizeInBits() >= G.DstWidth)
    return false;

  auto AddBoundaryRepair = [&]() {
    // look if we already have this repair
    for (const BoundaryRewrite &R : G.BoundaryRewrites)
      if (R.ExtUse == &U)
        return true;

    BoundaryRewrite Rewrite;
    Rewrite.ExtUse = &U;
    Rewrite.User = UI;
    Rewrite.Kind = RepairNarrowValue;
    Rewrite.ResultTy = UTy;
    Rewrite.OriginalValue = U.get();
    G.BoundaryRewrites.push_back(Rewrite);

    return true;
  };

  if (isa<TruncInst>(UI))
    return U.getOperandNo() == 0 && AddBoundaryRepair();

  // Instead of rejecting the whole graph,
  // try to accept casts as boundaries if they have a DestTy different
  // than the updated graph width.
  if (isExtOpcode(UI) &&
      cast<CastInst>(UI)->getDestTy()->getScalarSizeInBits() != G.DstWidth)
    return AddBoundaryRepair();

  // Do not steal uses that the graph can remove or promote.
  switch (getSupportedSourcePromoteKind(UI)) {
  case SourcePromoteNodeKind::NormalOp:
  case SourcePromoteNodeKind::Compare:
  case SourcePromoteNodeKind::ReplaceWithOperand:
    return false;
  case SourcePromoteNodeKind::Unsupported:
    break;
  }

  if (isa<StoreInst>(UI))
    return U.getOperandNo() == 0 && AddBoundaryRepair();

  if (isa<ReturnInst>(UI))
    return AddBoundaryRepair();

  if (auto *Cb = dyn_cast<CallBase>(UI)) {
    if (!isa<CallInst>(Cb))
      return false;
    if (Cb->isCallee(&U) || U.getOperandNo() >= Cb->arg_size())
      return false;
    if (Cb->isInlineAsm())
      return false;
    if (auto *CI = dyn_cast<CallInst>(Cb)) {
      if (CI->isMustTailCall())
        return false;
    }
    return AddBoundaryRepair();
  }
  return false;
}

void ExtRewritePlanner::noteSourcePromoteReject(SourcePromoteGraph &G,
                                                StringRef Reason, Value *V,
                                                Instruction *User,
                                                IntegerFillType Fill) const {
  if (G.Reject.Reason != "BuildGraphFailed")
    return;

  G.Reject.Reason = Reason;
  G.Reject.V = V;
  G.Reject.User = User;
  G.Reject.Fill = Fill;
}

// Compute safe signed wrap icmp constants
// helpful for the signed comparison trick
static bool computeSignedICmpWrapConstants(Instruction *I, ICmpInst *Cmp,
                                           unsigned NarrowWidth,
                                           unsigned PromotedWidth,
                                           APInt &NewOpConstant,
                                           APInt &CmpConstant) {

  unsigned Opc = I->getOpcode();
  if (Opc != Instruction::Add && Opc != Instruction::Sub)
    return false;

  if (!I->hasOneUse() || *I->user_begin() != Cmp)
    return false;

  if (!Cmp->isSigned())
    return false;

  if (Cmp->getOperand(0) != I)
    return false;

  auto *OpConst = dyn_cast<ConstantInt>(I->getOperand(1));
  if (!OpConst)
    return false;

  auto *ICmpConst = dyn_cast<ConstantInt>(Cmp->getOperand(1));
  if (!ICmpConst)
    return false;

  APInt ICmpC = ICmpConst->getValue();

  APInt NormalizedC = OpConst->getValue();
  // Treat the subtraction as addition
  // x - 1 = x + (-1)
  if (Opc == Instruction::Sub)
    NormalizedC = -NormalizedC;

  // NarrowWidth is N
  // PromotedWidth is W

  unsigned N = NarrowWidth;
  unsigned W = PromotedWidth;

  if (N == 0 || W <= N || W > 64)
    return false;

  APInt H = APInt::getOneBitSet(N, N - 1); // narrow sign bit
  APInt M = APInt::getOneBitSet(W, N);     // M = 2 ^ N

  APInt O = NormalizedC;

  APInt B = O + H; // unsigned add constant == (O + H) mod 2 ^ N

  // convert to an unsigned order
  APInt KPrime = ICmpC ^ H;
  // The first narrow result that is represented as a negative
  // wide value.
  APInt Cut = B ^ H;

  APInt AddConstant = B.sext(W);

  // We use zext here because KPrime is already converted to unsigned order
  CmpConstant = KPrime.zext(W);

  // Check if we are beyond the wrap, then we need to go back.
  if (KPrime.uge(Cut)) {
    CmpConstant -= M;
  }

  NewOpConstant = AddConstant;
  // Subtract again to keep the original sub operation.
  if (Opc == Instruction::Sub)
    NewOpConstant = -NewOpConstant;

  return true;
}

bool ExtRewritePlanner::isSafeICmpWrap(Instruction *I, ICmpInst *Cmp,
                                       unsigned NarrowWidth,
                                       unsigned PromotedWidth) {

  // We can support a potentially wrapping add/sub (Instruction I) iff:
  // - It is only used by a signed comparison
  // - The icmp uses a constant
  // - The wrap instruction (I) also uses a constant.
  // For example:
  //
  // %add = add i8 %a, C1
  // %cmp = icmp sge i8 %add, C2
  //
  // Original shape
  //
  // %t = add/sub iN %x, C
  // %r = icmp sPred iN, %t, K
  //
  // The narrow operation happens module 2 ^ N. So %t is really
  //
  // t = (x + O) mod 2 ^ N
  // where O = C for add, and O = -C for sub.
  //
  // For signed comparisons, we can convert the signed order into unsigned order
  // by flipping the sign bit
  //
  // t sPred K <=>  (t xor H) uPred(K xor H)
  // where:
  // H = 2 ^ (N - 1), is the Sign Bit.
  // But in a N-bit two's complement ring
  //
  // v xor H == (v + H) mod 2 ^ N
  //
  // So,
  // (t xor H)
  // = ((x + O) + H) mod 2 ^ N
  // = (x + O + H) mod 2 ^ N
  //
  // Therefore we can fold the signed order bias into the add/sub constant
  //
  // B = O + H mod 2 ^ N
  // K' = K xor H
  //
  // So, the signed comparison becomes an unsigned comparison over the biased
  // wrapped value
  //
  // This is the signed analogue of TypePromotion's unsigned safe wrap trick.
  //
  // G.Fill == SIGN is required
  // if G.Fill == ZEROS, then the values with are narrow sign bit bit are mapped
  // in a different way
  //
  // i8 -128
  // sext -> -128
  // zext -> 128
  //
  // Those are not equivalent in the promoted unsigned comparison, so the proof
  // is correct when the graph fill is SIGN.
  //
  // The promoted rewrite uses:
  // sext(x) + sext_N(B)
  // compared with an adjusted W-Bit unsigned constant.
  //
  // Cut is the point where the promoted representation crosses from the
  // nonnegative region into the negative unsigned region Cut = B ^ H

  APInt NewOpConstant;
  APInt ICmpConstant;

  if (!computeSignedICmpWrapConstants(I, Cmp, NarrowWidth, PromotedWidth,
                                      NewOpConstant, ICmpConstant))
    return false;

  if (!TLI->isLegalAddImmediate(NewOpConstant.getSExtValue()))
    return false;

  if (!TLI->isLegalICmpImmediate(ICmpConstant.getSExtValue()))
    return false;

  return true;
}

bool ExtRewritePlanner::tryToBuildSourcePromoteGraphUsers(
    Instruction *I, SmallVectorImpl<Instruction *> &Worklist,
    SourcePromoteGraph &G) {
  if (isa<ICmpInst>(I))
    return true;

  for (Use &U : I->uses()) {
    Instruction *VUser = cast<Instruction>(U.getUser());

    if (isa<SelectInst>(VUser) && U.getOperandNo() == 0) {
      noteSourcePromoteReject(G, "SelectConditionUser", I,
                              cast<Instruction>(VUser), ANYTHING);
      // ++NumExtSourcePromoteRejectedUser;
      return false;
    }

    if (auto *CI = dyn_cast<CastInst>(VUser)) {
      if (CI != G.Ext && (isa<ZExtInst>(CI) || isa<SExtInst>(CI)) &&
          CI->getDestTy()->isIntegerTy(G.DstWidth)) {
        G.ExtsToReplaceWithOperand.insert(CI);
        continue;
      }
    }

    if (auto *Cmp = dyn_cast<ICmpInst>(VUser)) {
      if (G.SafeWrap.contains(I) ||
          (G.Fill == SIGN && isSafeICmpWrap(I, Cmp, G.SrcWidth, G.DstWidth))) {
        G.SafeWrap.insert(I);
        continue;
      }
    }

    UserStrategy Strategy = getSourcePromoteUserStrategy(VUser, G);
    if (Strategy == UserStrategy::Reject && tryToAddSourcePromoteBoundary(U, G))
      continue;

    switch (Strategy) {
    case UserStrategy::Reject: {
      StringRef Reason = "UnsupportedUser";
      if (isa<GetElementPtrInst>(VUser))
        Reason = "UnsupportedGepUser";
      else if (isa<StoreInst>(VUser))
        Reason = "UnsupportedStoreUser";
      else if (isa<CallBase>(VUser))
        Reason = "UnsupportedCallUser";
      noteSourcePromoteReject(G, Reason, I, dyn_cast<Instruction>(VUser),
                              ANYTHING);
      if (auto *UI = dyn_cast<Instruction>(VUser)) {
        LLVM_DEBUG(dbgs() << "WIA source-promote unsupported user: "
                          << UI->getOpcodeName() << " in "
                          << UI->getFunction()->getName() << " value: " << *I
                          << " user: " << *UI << "\n");
      } else {
        LLVM_DEBUG(
            dbgs() << "WIA source-promote unsupported non-instruction user: "
                   << *VUser << " value: " << *I << "\n");
      }
      return false;
    }
    case UserStrategy::AcceptAndPush: {
      Worklist.push_back(VUser);
      break;
    }
    case UserStrategy::AcceptNoPush:
      continue;
    }
  }
  return true;
}

ExtRewritePlan ExtRewritePlanner::makeSimplePlan(CastInst *Ext) {
  ExtRewritePlan Plan;
  Plan.Ext = Ext;
  Plan.Source = Ext->getOperand(0);
  Plan.Kind = ExtStrategy::KeepExt;
  return Plan;
}

ExtRewritePlan ExtRewritePlanner::makeGraphPlan(ExtRewriteGraph &&G,
                                                ExtStrategy Strategy) {

  ExtRewritePlan Plan;
  Plan.Kind = Strategy;
  Plan.Ext = G.Ext;
  Plan.Source = G.Source;
  Plan.NarrowWidth = G.NarrowWidth;
  Plan.WideWidth = G.WideWidth;
  Plan.InstsToReplaceWithOperand = std::move(G.InstsToReplaceWithOperand);

  for (auto &Entry : G.Nodes) {
    auto *I = dyn_cast<Instruction>(Entry.first);
    if (!I)
      continue;
    Plan.InstsToNarrow.insert(I);
  }
  Plan.BoundaryRewrites = std::move(G.BoundaryRewrites);
  Plan.WideUses = std::move(G.WideUses);
  Plan.InstsToRemove = std::move(G.InstsToRemove);
  return Plan;
}

bool ExtRewritePlanner::buildNarrowGraph(CastInst *Ext, ExtRewriteGraph &G) {
  auto SrcWidth = Ext->getSrcTy()->getScalarSizeInBits();
  auto DstWidth = Ext->getDestTy()->getScalarSizeInBits();
  G.Ext = Ext;
  G.NarrowWidth = SrcWidth;
  G.WideWidth = DstWidth;
  return tryToBuildNarrowGraphFromUsers(Ext, G);
}

bool ExtRewritePlanner::tryToKeepDemandedExtUseNarrowed(
    Use &U, ExtRewriteGraph &G, SmallVectorImpl<Value *> &Worklist) {

  auto *UI = dyn_cast<Instruction>(U.getUser());
  if (!UI)
    return false;

  if (U.getOperandNo() != 0)
    return false;

  switch (UI->getOpcode()) {
  case Instruction::LShr:
  case Instruction::URem:
  case Instruction::UDiv:
    if (G.Ext->getOpcode() != Instruction::ZExt)
      return false;
    return tryToAddNarrowGraphNode(UI, G, Worklist);
  case Instruction::SRem:
  case Instruction::SDiv:
    if (G.Ext->getOpcode() != Instruction::SExt) {
      return false;
    }
    return tryToAddNarrowGraphNode(UI, G, Worklist);
  }
  return false;
}

bool ExtRewritePlanner::isNarrowGraphProfitable(const ExtRewriteGraph &G) {
  if (G.Nodes.empty())
    return false;

  bool HasPhi = llvm::any_of(
      G.Nodes, [](const auto &Entry) { return isa<PHINode>(Entry.first); });

  if (HasPhi && G.InstsToRemove.empty())
    return false;

  if (!G.InstsToRemove.empty()) {
    return true;
  }

  return llvm::any_of(G.BoundaryRewrites, [](const BoundaryRewrite &Rewrite) {
    return Rewrite.Kind == BoundaryRewriteKind::ReplaceWithSource ||
           Rewrite.Kind == BoundaryRewriteKind::TruncSource ||
           Rewrite.Kind == BoundaryRewriteKind::NarrowICmp;
  });
}

// Choose the op fills when we have already proved a safe wrap on I.
static bool
chooseSafeWrapOpFills(Instruction *I, IntegerFillType ResultFill,
                      SmallVectorImpl<IntegerFillType> &OpsFills,
                      const SmallPtrSetImpl<Instruction *> &SafeWrap) {
  if (!SafeWrap.contains(I))
    return false;

  if (I->getOpcode() != Instruction::Add && I->getOpcode() != Instruction::Sub)
    return false;

  if (ResultFill != ANYTHING)
    return false;

  // Assign ANYTHING to the safe wrap operand 2
  // We have already proved we are safe to promote to a bigger type
  OpsFills.assign(I->getNumOperands(), ANYTHING);
  // Operand 0 needs to stay SIGN, cause the trick is only valid
  // when the G.Fill of the graph is SIGN.
  OpsFills[0] = SIGN;
  return true;
}

// Check if we can widen Use U.
static bool isWidenedOp(const Use &U) {
  const auto *I = cast<Instruction>(U.getUser());

  if (isa<SelectInst>(I))
    return U.getOperandNo() != 0;

  const auto *CB = dyn_cast<CallBase>(I);

  if (!CB)
    return true;

  if (!CB->isArgOperand(&U))
    return false;

  switch (CB->getIntrinsicID()) {
  case Intrinsic::abs:
  case Intrinsic::ctpop:
  case Intrinsic::cttz:
    return CB->getArgOperandNo(&U) == 0;
  case Intrinsic::umin:
  case Intrinsic::umax:
  case Intrinsic::smin:
  case Intrinsic::smax:
    return true;

  default:
    return false;
  }
}

bool ExtRewritePlanner::tryToBuildSourcePromoteGraphFromValue(
    Value *Source, SourcePromoteGraph &G) {
  G.Source = Source;
  if (G.IsICmpRoot && ICmpGraphPopvalCount >= MaxICmpGraphPopval) {
    noteSourcePromoteReject(G, "ICmpFunctionWorkLimit", Source, nullptr,
                            G.Fill);
    return false;
  }

  auto *SourceI = dyn_cast<Instruction>(Source);
  if (!SourceI) {
    noteSourcePromoteReject(G, "SourceExternalLeaf", Source, nullptr, G.Fill);
    return false;
  }

  SmallVector<Instruction *, 128> Worklist{SourceI};
  G.setRequiredFill(Source, G.Fill);

  bool AllNodesBitwise = llvm::all_of(G.Nodes, [](const auto &Node) {
    auto *BO = dyn_cast<BinaryOperator>(Node.first);
    return BO && BO->isBitwiseLogicOp();
  });

  while (!Worklist.empty()) {
    if (G.IsICmpRoot)
      ++ICmpGraphPopvalCount;

    Instruction *V = Worklist.pop_back_val();
    Instruction *VI = V;

    if (!VI) {
      // constants and args can be extended to Wide Width.
      G.ExternalLeaves.insert(V);
      continue;
    }

    auto StateIt = G.States.find(V);
    unsigned CurrentNodeId =
        StateIt == G.States.end() ? 0 : StateIt->second.NodeId;
    bool InGraph = CurrentNodeId != 0;
    IntegerFillType ResultFill =
        StateIt != G.States.end() && StateIt->second.Required != UNDEFINED
            ? StateIt->second.Required
        : V == G.Source ? G.Fill
                        : ANYTHING;
    // Don't try to recompute something with the same ResultFill
    // If have already recomputed it.
    if (InGraph && StateIt->second.CompletedOperands &&
        StateIt->second.LastProcessedFill == ResultFill)
      continue;

    LLVM_DEBUG(dbgs() << "WIA source-promote visit node: fill=" << ResultFill
                      << " src=" << G.SrcWidth << " dst=" << G.DstWidth << " : "
                      << *VI << "\n");
    SmallVector<IntegerFillType, 4> OpNeededFills;
    bool HaveOpFills = false;
    if (!InGraph) {
      auto *SrcBO = dyn_cast<BinaryOperator>(G.Source);
      // If this operation cannot be widened, keep it narrow
      // and extends its result for the promote bitwise users instead.
      bool IsBitwiseG = SrcBO && SrcBO->isBitwiseLogicOp() && AllNodesBitwise;
      // Allow ops to remain narrow if they are inside the graph.
      auto CanKeepNarrowOp = [&] {
        return !isa<FreezeInst, CastInst, LoadInst>(VI) &&
               llvm::none_of(VI->operands(),
                             [&](Value *Op) { return G.containsNode(Op); });
      };

      if (isSupportedScalarIntType(VI->getType()) && VI != G.Source &&
          !isa<PHINode>(VI) &&
          VI->getType()->getScalarSizeInBits() < G.DstWidth &&
          (ResultFill == SIGN || ResultFill == ZEROS) &&
          (IsBitwiseG || CanKeepNarrowOp())) {
        if (!chooseOpsFills(VI, ResultFill, OpNeededFills)) {
          // Preserve narrow computation and extend the result.
          G.ExternalOpsToExtend[VI] = ResultFill;
          continue;
        }
        HaveOpFills = true;
      }

      if (auto *LI = dyn_cast<LoadInst>(VI)) {
        Type *LoadTy = LI->getType();
        if (!isSupportedScalarIntType(LoadTy)) {
          noteSourcePromoteReject(G, "UnsupportedLoadType", LI, nullptr,
                                  ResultFill);
          return false;
        }

        auto *ExtTy = Type::getIntNTy(Ctx, G.DstWidth);
        unsigned LoadWidth = LoadTy->getScalarSizeInBits();
        // If this load can be extended and it's not already extended, try to
        // extend it later.
        if (LoadWidth < G.DstWidth && isResolvedFill(ResultFill) &&
            !hasExistingLoadExt(LI, G.DstWidth, ResultFill, G.Ext) &&
            isExtLoadFree(LI, ExtTy, ResultFill, *DL)) {
          G.LoadsToExtend[LI] = ResultFill;
          continue;
        }
        if (LoadWidth == G.DstWidth)
          continue;
      }
      const SourcePromoteRejectInfo SavedReject = G.Reject;
      if (!tryToAddSourcePromoteNode(VI, G, ResultFill)) {
        if (auto *LI = dyn_cast<LoadInst>(VI);
            LI && VI != G.Source && isSupportedScalarIntType(LI->getType()) &&
            LI->getType()->getScalarSizeInBits() < G.DstWidth &&
            (ResultFill == SIGN || ResultFill == ZEROS)) {
          // Keep the load narrow and try to extend it,
          // rather than to reject the whole graph.
          // This may be more profitable down the line.
          G.ExternalOpsToExtend[LI] = ResultFill;
          G.Reject = SavedReject;
          continue;
        }
        noteSourcePromoteReject(G, "AddNodeFailed", VI, nullptr, ResultFill);
        return false;
      }

      if (G.WideLeaves.contains(VI))
        continue;

      if (AllNodesBitwise) {
        auto *BO = dyn_cast<BinaryOperator>(VI);
        AllNodesBitwise = BO && BO->isBitwiseLogicOp();
      }
    }

    if (!InGraph && !tryToBuildSourcePromoteGraphUsers(VI, Worklist, G))
      return false;

    // No need to check VIs operands, as this is the wide replacement.
    if (G.InstsToReplaceWithOperand.contains(VI) && ResultFill != PENDING)
      continue;

    // Prune early, avoid unfavorable graphs
    bool FillTuple =
        chooseSafeWrapOpFills(VI, ResultFill, OpNeededFills, G.SafeWrap) ||
        HaveOpFills || chooseOpsFills(VI, ResultFill, OpNeededFills);
    HaveOpFills |= FillTuple;

    if (!FillTuple) {
      noteSourcePromoteReject(G, "NoOperandFill", VI, nullptr, ResultFill);
      return false;
    }

    for (auto [OpIdx, OpUse] : llvm::enumerate(VI->operands())) {
      Value *Producer = OpUse.get();
      if (!isWidenedOp(OpUse))
        continue;

      if (isa<ConstantInt>(Producer))
        continue;

      IntegerFillType NeededOpFill = OpNeededFills[OpIdx];
      // if it's needed, change PENDING to ANYTHING here
      // Otherwise we might reject a legal graph
      // The PromoteConstraintSolver checks the ANYTHING
      // if it has specific fill requirements and whether
      // we can assign those.
      IntegerFillType GraphOpFill =
          NeededOpFill == PENDING ? ANYTHING : NeededOpFill;

      if (auto *LI = dyn_cast<LoadInst>(Producer)) {
        LLVM_DEBUG(dbgs() << "WIA source-promote load leaf candidate: needed="
                          << NeededOpFill << " known-zero-above-src="
                          << isKnownZeroAbove(LI, G.SrcWidth)
                          << " src=" << G.SrcWidth << " dst=" << G.DstWidth
                          << " : " << *LI << "\n");
        Type *LoadTy = LI->getType();
        if (!isSupportedScalarIntType(LoadTy)) {
          noteSourcePromoteReject(G, "UnsupportedOperandLoadType", LI, VI,
                                  NeededOpFill);
          return false;
        }

        unsigned LoadWidth = LoadTy->getScalarSizeInBits();

        if (LoadWidth < G.DstWidth &&
            (isResolvedFill(NeededOpFill) || NeededOpFill == PENDING)) {
          auto It = G.LoadsToExtend.find(LI);
          auto *ExtTy = Type::getIntNTy(Ctx, G.DstWidth);
          if (It != G.LoadsToExtend.end()) {
            IntegerFillType MergedFill =
                mergeValueFills(LI, It->second, NeededOpFill, WiaInfo);
            if (MergedFill == UNDEFINED) {
              noteSourcePromoteReject(G, "LoadFillConflict", LI, VI,
                                      NeededOpFill);
              // ++NumExtSourcePromoteRejectedFillConflict;
              return false;
            }
            It->second = MergedFill;
            continue;
          }

          if (NeededOpFill == PENDING) {
            G.LoadsToExtend[LI] = PENDING;
            continue;
          }
          if (!hasExistingLoadExt(LI, G.DstWidth, NeededOpFill, G.Ext) &&
              isExtLoadFree(LI, ExtTy, NeededOpFill, *DL)) {
            G.LoadsToExtend[LI] = NeededOpFill;
            continue;
          }
        }
      }

      // Prune early, if I have seen this user again and we cannot
      // match the fillTypes
      MergeFillResult Res = mergeFill(G, Producer, GraphOpFill);
      if (Res == MergeFillResult::Conflict) {
        noteSourcePromoteReject(G, "OperandFillConflict", Producer, VI,
                                NeededOpFill);
        //++NumExtSourcePromoteRejectedFillConflict;
        return false;
      }

      if (auto *ProducerI = dyn_cast<Instruction>(Producer)) {
        if ((!G.containsNode(ProducerI) && !G.WideLeaves.contains(ProducerI)) ||
            Res == MergeFillResult::Changed) {
          Worklist.push_back(ProducerI);
        }
      } else {
        // record external/leaf
        G.ExternalLeaves.insert(Producer);
      }
    }
    // Save the state so we don't need to visit VI again.
    auto &ProcessedState = G.States[VI];
    ProcessedState.LastProcessedFill = ResultFill;
    ProcessedState.CompletedOperands = true;
  }

  for (Value *Leaf : G.ExternalLeaves) {
    IntegerFillType Required = G.getRequiredFill(Leaf);
    if (auto *Arg = dyn_cast<Argument>(Leaf);
        Arg && isSupportedScalarIntType(Leaf->getType()) &&
        Leaf->getType()->getScalarSizeInBits() < G.DstWidth &&
        (Required == SIGN || Required == ZEROS)) {
      // Extend the arg, this may be cheaper.
      G.ExternalOpsToExtend[Leaf] = Required;
      continue;
    }

    if (!isa<ConstantInt>(Leaf)) {
      LLVM_DEBUG(dbgs() << "WIA source-promote rejected external leaf: "
                        << *Leaf << "\n");
      noteSourcePromoteReject(G, "ExternalLeafNoSol", Leaf, nullptr, ANYTHING);
      // ++NumExtSourcePromoteRejectedExternalLeaf;
      return false;
    }
  }
  // If it's decided to keep a trunc narrow,
  // do not try to remove it later on.
  for (const BoundaryRewrite &Rewrite : G.BoundaryRewrites) {
    if (Rewrite.Kind == RepairNarrowValue && G.containsNode(Rewrite.User)) {
      noteSourcePromoteReject(G, "RepairNarrowValue in G", Rewrite.User,
                              Rewrite.User);
      return false;
    }
  }
  // Do not make an external op in one case
  // A promoted node in another traversal.
  for (const auto &[V, _] : G.ExternalOpsToExtend) {
    if (G.containsNode(V))
      return false;
  }

  auto IsUserRewritten = [&](Use &U) {
    auto *UI = dyn_cast<Instruction>(U.getUser());
    if (!UI)
      return false;

    if (UI == G.Ext || G.ExtsToReplaceWithOperand.contains(UI))
      return true;

    if (G.containsNode(UI) && !G.WideLeaves.contains(UI))
      return true;

    for (const BoundaryRewrite &Rewrite : G.BoundaryRewrites)
      if (Rewrite.ExtUse == &U)
        return true;

    return false;
  };

  for (Instruction *I : G.InstsToReplaceWithOperand) {
    bool CanRemoveInst = true;
    // An intruction can be removed if we handle all of it's uses
    // otherwise prevent removal.
    for (Use &U : I->uses()) {
      if (!IsUserRewritten(U)) {
        CanRemoveInst = false;
        break;
      }
    }
    // Nested zext/sext is a must remove rewrite: all uses of the cast
    // must be rewritten by this graph.
    // If any outside user remains, keeping the old extension,
    // requires extra boundary repair, for now
    // only truncs keep the relaxed behavior.
    if (!CanRemoveInst) {
      if (isa<SExtInst>(I) || isa<ZExtInst>(I)) {
        noteSourcePromoteReject(G, "NestedExtHasOutsideUsers", I, nullptr,
                                ANYTHING);
        return false;
      }
      continue;
    }
    G.InstsToRemove.push_back(I);
  }
  // if we managed to build the wide graph,
  // we can safely remove the extension.
  if (G.Ext)
    G.InstsToRemove.push_back(G.Ext);
  return true;
}

bool ExtRewritePlanner::tryToBuildSourcePromoteGraphFromProducers(
    CastInst *Ext, SourcePromoteGraph &G) {
  return tryToBuildSourcePromoteGraphFromValue(Ext->getOperand(0), G);
}

// Sometimes a sext and zext produce the same index in some
// accesses, try to capture that
static bool canUseSignFillForGEP(ZExtInst *Ext, unsigned SrcWidth,
                                 const DataLayout &DL) {

  if (Ext->use_empty())
    return false;

  for (auto &U : Ext->uses()) {
    auto User = U.getUser();
    auto *Gep = dyn_cast<GetElementPtrInst>(User);
    if (!Gep)
      return false;

    if (!Gep->isInBounds())
      return false;

    if (U.getOperandNo() != Gep->getNumOperands() - 1)
      return false;

    for (auto &U : Gep->indices()) {
      Value *VU = U.get();

      if (U.getOperandNo() == Gep->getNumOperands() - 1)
        continue;

      auto *C = dyn_cast<ConstantInt>(VU);
      if (!C)
        return false;

      if (!C->isZero())
        return false;
    }

    // Simplify by using byte indexing, not a hard requirement.
    if (!Gep->getResultElementType()->isIntegerTy(8))
      return false;

    auto Base = Gep->getPointerOperand();
    auto *GlobalV = dyn_cast<GlobalVariable>(Base);
    if (!GlobalV)
      return false;

    // To rely on the object's size we need a local definition.
    if (!GlobalV->hasLocalLinkage() || !GlobalV->hasInitializer())
      return false;

    auto GlobalVTy = GlobalV->getValueType();
    if (!GlobalVTy->isSized())
      return false;

    auto TySize = DL.getTypeAllocSize(GlobalVTy);
    if (TySize.isScalable())
      return false;

    auto ObjectSize = TySize.getFixedValue();
    auto Max = APInt::getSignedMaxValue(SrcWidth).getZExtValue();
    if (ObjectSize > Max)
      return false;

    auto AddrSpace = Gep->getPointerAddressSpace();
    unsigned DstWidth = Ext->getDestTy()->getScalarSizeInBits();
    if (DL.getIndexSizeInBits(AddrSpace) != DstWidth)
      return false;
  }
  return true;
}

// If the constant fits in the source's non negative signed range
// sext and zext give same the same equality comparison.
static bool canUseSignFillForICmpUse(const Use &U, unsigned NarrowWidth) {
  auto *ICmp = dyn_cast<ICmpInst>(U.getUser());
  if (!ICmp)
    return false;

  // We cannot accept signed comparisons.
  if (!(ICmp->isUnsigned() || ICmp->isEquality()))
    return false;

  auto *C = dyn_cast<ConstantInt>(ICmp->getOperand(1 - U.getOperandNo()));
  if (!C)
    return false;

  const APInt &Val = C->getValue();
  // Find out the largest nonnegative signed value of the source type
  // int the same width as the comparison constant.
  //
  // %wide = zext i8 %x to i32
  // %icmp = icmp ult i32 %wide, 100
  //
  // We can use a sext here, if it's cheaper.
  APInt Max = APInt::getSignedMaxValue(NarrowWidth).zext(Val.getBitWidth());
  return Val.ule(Max);
}

static ICmpInst *getSignFillICmp(ZExtInst *Ext, unsigned SrcWidth) {
  if (!Ext->hasOneUse())
    return nullptr;

  Use &U = *Ext->use_begin();
  return canUseSignFillForICmpUse(U, SrcWidth) ? cast<ICmpInst>(U.getUser())
                                               : nullptr;
}

IntegerFillType ExtRewritePlanner::selectBestFill(CastInst *Ext) {
  IntegerFillType SelectedFill = UNDEFINED;

  SelectedFill = Ext->getOpcode() == Instruction::SExt ? SIGN : ZEROS;
  if (auto *ZExt = dyn_cast<ZExtInst>(Ext)) {
    if (ZExt->hasNonNeg() || canSetSignFill(Ext) ||
        canUseSignFillForGEP(ZExt, ZExt->getSrcTy()->getScalarSizeInBits(),
                             *DL))
      SelectedFill = SIGN;
  }

  if (!Ext->hasOneUse())
    return SelectedFill;

  Use &U = *Ext->use_begin();
  auto ICmp = dyn_cast<ICmpInst>(U.getUser());
  if (!ICmp)
    return SelectedFill;

  if (auto *ZExt = dyn_cast<ZExtInst>(Ext);
      ZExt && getSignFillICmp(ZExt, Ext->getSrcTy()->getScalarSizeInBits())) {
    auto SrcTy = Ext->getSrcTy();
    auto DstTy = Ext->getDestTy();
    EVT SrcVt = TLI->getValueType(*DL, SrcTy);
    EVT DstVt = TLI->getValueType(*DL, DstTy);
    if (TLI->isSExtCheaperThanZExt(SrcVt, DstVt))
      return SIGN;
  }
  return SelectedFill;
}

bool ExtRewritePlanner::buildSourcePromoteGraph(CastInst *Ext,
                                                SourcePromoteGraph &G) {
  if (isa<ICmpInst>(Ext->getOperand(0)))
    return false;

  // Currently, only power of 2 types up to 64 bits are supported.
  if (!isSupportedScalarIntType(Ext->getSrcTy()) ||
      !isSupportedScalarIntType(Ext->getDestTy())) {
    return false;
  }

  auto SrcWidth = Ext->getSrcTy()->getScalarSizeInBits();
  auto DstWidth = Ext->getDestTy()->getScalarSizeInBits();
  G.Ext = Ext;
  G.SrcWidth = SrcWidth;
  G.DstWidth = DstWidth;

  G.Fill = selectBestFill(Ext);

  // No reason to build a graph with a Load with same Cast Fill.
  if (isa<LoadInst>(Ext->getOperand(0)) &&
      G.Fill == (isa<SExtInst>(Ext) ? SIGN : ZEROS))
    return false;

  if (tryToBuildSourcePromoteGraphFromProducers(Ext, G))
    return true;

  return false;
}

unsigned int ExtRewritePlanner::chooseBestWidth(Instruction *Source,
                                                unsigned RegisterWidth) {
  SmallVector<Instruction *, 8> Worklist;
  SmallPtrSet<Instruction *, 8> Visited;

  unsigned int BestWidth = RegisterWidth;
  Type *NarrowTy = Source->getType();
  unsigned NarrowWidth = NarrowTy->getScalarSizeInBits();
  Worklist.push_back(Source);
  int Count = 0;
  constexpr int Limit = 8;
  while (!Worklist.empty()) {
    Instruction *I = Worklist.pop_back_val();
    ++Count;
    if (Count > Limit)
      return BestWidth;

    if (auto *Tr = dyn_cast<TruncInst>(I)) {
      Type *SrcTy = Tr->getSrcTy();
      if (isSupportedScalarIntType(SrcTy)) {
        unsigned Width = SrcTy->getIntegerBitWidth();
        // Choose a width that allows us to possibly drop the truncation
        // too, if this is supported by the CPU as a native type.
        if (Width > NarrowWidth && Width < RegisterWidth &&
            DL->isLegalInteger(Width))
          BestWidth = Width;
      }
      continue;
    }
    if (!isa<BinaryOperator>(I) && !isa<SelectInst>(I))
      continue;

    for (const auto &Op : I->operand_values()) {
      if (Worklist.size() == Limit)
        break;

      auto *OpI = dyn_cast<Instruction>(Op);
      if (!OpI || OpI->getType() != NarrowTy)
        continue;

      if (Visited.insert(OpI).second)
        Worklist.push_back(OpI);
    }
  }
  return BestWidth;
}

bool ExtRewritePlanner::isPureLoopCounter(ICmpInst *ICmp, Instruction *Source) {
  if (!ICmp->hasOneUse())
    return false;

  auto *Phi = dyn_cast<PHINode>(Source);
  if (!Phi || !Phi->getType()->isIntegerTy() ||
      Phi->getNumIncomingValues() != 2)
    return false;

  if (Phi->hasNUsesOrMore(3))
    return false;

  unsigned OtherOp = ICmp->getOperand(0) == Phi ? 1 : 0;
  if (!isa<ConstantInt>(ICmp->getOperand(OtherOp)))
    return false;

  auto *L = getLoopCarriedPhiLoop(Phi, LI);
  if (!L || ICmp->getParent() != L->getHeader())
    return false;

  auto *Preheader = L->getLoopPreheader();
  auto *Latch = L->getLoopLatch();
  if (!Preheader || !Latch ||
      !isa<ConstantInt>(Phi->getIncomingValueForBlock(Preheader)))
    return false;

  auto *Update = dyn_cast<BinaryOperator>(Phi->getIncomingValueForBlock(Latch));
  if (!Update || Update->getParent() != Latch || !Update->hasOneUse())
    return false;

  if (!Update->hasNoUnsignedWrap())
    return false;

  // Require phi + constant, constant + phi, or phi - constant.
  if (!match(Update, m_c_Add(m_Specific(Phi), m_ConstantInt())) &&
      !match(Update, m_Sub(m_Specific(Phi), m_ConstantInt())))
    return false;

  // The counter is used only by its update and this comparison.
  return llvm::all_of(Phi->users(),
                      [&](User *U) { return U == Update || U == ICmp; });
}

bool ExtRewritePlanner::isICmpCandidate(ICmpInst *ICmp, Instruction *Source) {

  if (!Source)
    return false;

  unsigned N = Source->getType()->getIntegerBitWidth();
  unsigned W =
      TTI->getRegisterBitWidth(TargetTransformInfo::RGK_Scalar).getFixedValue();

  // Narrow icmps are always candidates as they will promoted by the
  // type legalizer.
  if (N > 1 && N < 32 && ICmp->isSigned() && isSafeICmpWrap(Source, ICmp, N, W))
    return true;

  // Select instruction usually presents opportunities.
  for (User *U : ICmp->users())
    if (isa<SelectInst>(U) && U->getType()->isIntegerTy(1))
      return true;

  unsigned Budget = 64;
  SmallPtrSet<Instruction *, 16> Active;
  if (getICmpSourceFacts(Source, Budget, Active) == 0)
    return false;

  // Those usually present opportunities too.
  if (isa<PHINode, CastInst>(Source))
    return true;

  SmallVector<Instruction *, 8> Worklist{Source};
  SmallPtrSet<Instruction *, 8> Seen{Source};
  unsigned Count = 0;
  constexpr unsigned Limit = 8;
  while (!Worklist.empty()) {
    Instruction *I = Worklist.pop_back_val();
    if (isa<PHINode, CastInst>(I))
      return true;
    if (++Count > Limit)
      return true;
    for (Value *V : I->operand_values()) {
      auto *OpI = dyn_cast<Instruction>(V);
      if (OpI && OpI->getType() == Source->getType() && Seen.insert(OpI).second)
        Worklist.push_back(OpI);
    }
  }
  auto SrcWidth = Source->getType()->getIntegerBitWidth();
  // This ICmp will be promoted DAGCombiner,
  // so try to promote it to avoid casts.
  return SrcWidth > 1 && SrcWidth < 32;
}

uint8_t
ExtRewritePlanner::getICmpSourceFacts(Instruction *I, unsigned &Budget,
                                      SmallPtrSetImpl<Instruction *> &Active) {
  if (ICmpSourceFacts.contains(I))
    return ICmpSourceFacts[I];

  uint8_t Facts = 0;
  // Truncations Phis and Select insts they may signal
  // useful promotion graphs for the icmp case.
  // If we find one of those we keep investigating the graph.
  // Save as facts for simplicitly.
  if (isa<PHINode, TruncInst>(I))
    return ICmpSourceFacts[I] = 1;
  if (isa<SelectInst>(I) && I->getType()->isIntegerTy(1))
    return ICmpSourceFacts[I] = 2;

  if (!Budget || !Active.insert(I).second)
    return 8; // Not known.

  --Budget;

  // For the recursion do not visit the same I in the same path twice.
  auto PopActive = llvm::scope_exit([&] { Active.erase(I); });

  for (Use &U : I->operands()) {
    if (!isWidenedOp(U))
      continue;

    auto *Producer = dyn_cast<Instruction>(U.get());
    if (!Producer || !Producer->getType()->isIntegerTy())
      continue;

    // Record if an operand comes from another basic block.
    if (Producer->getParent() != I->getParent())
      Facts |= 4;

    Facts |= getICmpSourceFacts(Producer, Budget, Active);
    if (Facts & (1 | 2 | 16)) {
      // if its 1 or 2, ie an interesting value
      // clear any unknown.
      Facts &= ~uint8_t(8);
      break;
    }
  }

  if (!(Facts & 8))
    ICmpSourceFacts[I] = Facts;

  return Facts;
}

std::optional<ExtRewritePlan>
ExtRewritePlanner::buildSmallICmpPlan(ICmpInst *ICmp, Instruction *Source,
                                      unsigned PromotedWidth) {
  auto SrcOpc = Source->getOpcode();
  if (SrcOpc != Instruction::Add && SrcOpc != Instruction::Sub)
    return std::nullopt;

  auto *Load = dyn_cast<LoadInst>(Source->getOperand(0));
  if (!Load || !Load->isSimple() || !Load->hasOneUse() ||
      !isa<ConstantInt>(Source->getOperand(1)))
    return std::nullopt;

  unsigned Narrow = Source->getType()->getIntegerBitWidth();
  // Construct a plan in place here to by pass all the expensive functions.
  if (isSafeICmpWrap(Source, ICmp, Narrow, PromotedWidth)) {
    ExtRewritePlan Plan;
    Plan.Kind = ExtStrategy::PromoteICmpRoot;
    Plan.Ext = ICmp;
    Plan.Source = Source;
    Plan.NarrowWidth = Narrow;
    Plan.WideWidth = PromotedWidth;
    Plan.SourcePromoteFill = ANYTHING;
    Plan.InstsToPromote.insert(Source);
    Plan.SafeWrap.insert(Source);
    Plan.LoadsToExtend[Load] = SIGN;
    Plan.RequiredFill[Load] = SIGN;
    Plan.RequiredFill[Source] = ANYTHING;
    return Plan;
  }
  return std::nullopt;
}

/// @brief
/// Try to prove that the graph G, is legal and we can actually perform
/// the promotion. If we find a PENDING filltype, this means we were unable
/// to select a suitable fillType because we had more than one choice.
/// We solve this by using an UndoLog and when we try a new choice of
/// that fillType we take a checkpoint or snapshot, so we can go back at it if
/// we didn't manage to prove legality.
class PromoteConstraintSolver {
public:
  enum class FillChoice { Concrete, MultipleChoices, Invalid };

  PromoteConstraintSolver(
      const SourcePromoteGraph &G, const WIAInfo &WiaInfo, const DataLayout &DL,
      LLVMContext &Ctx,
      function_ref<bool(Value *, unsigned)> IsKnownSignExtended,
      WIATuplesMap &SharedTuples)
      : G(G), WiaInfo(WiaInfo), DL(DL), Ctx(Ctx),
        IsKnownSignExtended(IsKnownSignExtended), LegalTuples(SharedTuples) {}
  // Solve the graph, try to prove it's legal to promote
  bool solve();

  // Try to get matching Operand fillTypes of I with a result fillType given as
  // ResultFill.
  FillChoice chooseOpsFills(Instruction *I, IntegerFillType ResultFill,
                            SmallVectorImpl<IntegerFillType> &OpsFills) const;

  // Transfer the final solution after all solver queries are complete.
  void moveSolutionTo(ExtRewritePlan &Plan) {
    Plan.RequiredFill = std::move(RequiredFill);
    Plan.RequiredOperandFill = std::move(RequiredOperandFill);
    Plan.LoadsToExtend = std::move(LoadsToExtend);
  }

  // Those populate maps that are needed by the Rebuilder when the widening
  // is happening.
  const DenseMap<Value *, IntegerFillType> &getRequiredFill() const;
  const DenseMap<const Use *, IntegerFillType> &getRequiredOperandFill() const;

  const DenseMap<Value *, IntegerFillType> &getLoadsToExtend() const;
  const DenseMap<Value *, IntegerFillType> &getExternalOpsToExtend() const;

  const SourcePromoteRejectInfo &getRejectInfo() const;

private:
  const SourcePromoteGraph &G;

  const WIAInfo &WiaInfo;
  const DataLayout &DL;
  LLVMContext &Ctx;

  function_ref<bool(Value *, unsigned)> IsKnownSignExtended;
  using TupleKey = std::pair<Instruction *, IntegerFillType>;
  WIATuplesMap &LegalTuples;
  // The promote state is of type (width, FillType)
  struct PromoteState {
    unsigned Width;
    IntegerFillType Fill;
  };

  // What FillType each value requires.
  DenseMap<Value *, IntegerFillType> RequiredFill;
  DenseMap<const Use *, IntegerFillType> RequiredOperandFill;

  // Store instructions that will need extensions in the rebuilder phase.
  DenseMap<Value *, IntegerFillType> LoadsToExtend;

  enum : uint8_t {
    SignState = 1u << 0,
    ZeroState = 1u << 1,
    AnyState = 1u << 2,
    AllStates = SignState | ZeroState | AnyState
  };

  // All the possible solutions before we reach a fixpoint.
  SmallVector<uint8_t, 32> Possible;
  mutable SourcePromoteRejectInfo Reject;

  // Try to find the fillType requirements for the given expression
  bool computeFillRequirements();

  static constexpr unsigned MaxPendingResolveDepth = 3;
  static constexpr unsigned MaxPendingResolveAttempts = 2;

  // Explore alternative fill tuples within depth and attempts limits.
  // If have more than one legal fill tuple, this can be useful.
  // Because we don't know when we build the graph what to choose.
  bool propagateFillRequirements(
      DenseMap<Value *, IntegerFillType> &ReqFill,
      DenseMap<const Use *, IntegerFillType> &ReqOperandFill,
      SmallSetVector<Value *, 32> &Worklist, unsigned Depth,
      unsigned &Attempts) const;

  enum class PropagationStatus { Done, Conflict, Continue };

  // Used as a return value in the function
  // propagateUntil.
  struct PropagationResult {
    PropagationStatus Status;
    Instruction *I = nullptr;
    IntegerFillType Fill = UNDEFINED;
  };

  // The Undo log is needed when we are trying
  // to use a particular fillType on an expression
  // but that doesn't work anymore and we might have
  // more choices, so we need to go back.
  template <typename T> struct UndoLog {

    struct FillEntry {
      FillEntry(DenseMap<T, IntegerFillType> *Map, T Key,
                IntegerFillType OldFill, bool HadEntry)
          : Map(Map), Key(Key), OldFill(OldFill), Exists(HadEntry) {}

      DenseMap<T, IntegerFillType> *Map;
      T Key;
      IntegerFillType OldFill;
      bool Exists;
    };

    SmallVector<FillEntry, 32> FillEntries;
    bool Recording = false;

    size_t snapshot() {
      Recording = true;
      return FillEntries.size();
    }

    void set(DenseMap<T, IntegerFillType> &FillMap, T Key,
             IntegerFillType NewFill) {
      auto [It, Inserted] = FillMap.try_emplace(Key, NewFill);
      if (!Inserted && It->second == NewFill)
        return;

      recordOldState(FillMap, Key, Inserted ? UNDEFINED : It->second,
                     !Inserted);

      if (!Inserted)
        It->second = NewFill;
    }

    void rollback(size_t Snapshot) {
      assert(Snapshot <= FillEntries.size() && "Invalid undo checkpoint");

      while (FillEntries.size() > Snapshot) {
        auto FillEntry = FillEntries.pop_back_val();
        if (FillEntry.Exists)
          (*FillEntry.Map)[FillEntry.Key] = FillEntry.OldFill;
        else
          FillEntry.Map->erase(FillEntry.Key);
      }
    }

    void recordOldState(DenseMap<T, IntegerFillType> &FillMap, T Key,
                        IntegerFillType OldFill, bool Exists) {
      if (Recording)
        FillEntries.emplace_back(&FillMap, Key, OldFill, Exists);
    }
  };

  // Assume with PENDING that we have a matching Fill but
  // but don't know exactly what, use a Pending decision
  // to help examine things further with a FillType
  // and a restore method if things don't work out.
  struct PendingDecision {
    Instruction *I = nullptr;
    IntegerFillType Fill = UNDEFINED;
    unsigned NextTuple = 0;

    size_t ValueCheckpoint;
    size_t OperandCheckpoint;
    SmallSetVector<Value *, 32> SavedWorklist;

    PendingDecision(Instruction *Inst, IntegerFillType ResultFill,
                    size_t ValueCP, size_t OperandCP,
                    const SmallSetVector<Value *, 32> &Worklist)
        : I(Inst), Fill(ResultFill), ValueCheckpoint(ValueCP),
          OperandCheckpoint(OperandCP), SavedWorklist(Worklist) {}
  };

  // Try to push fillTypes to the operands to see if we can find a legal
  // match for the whole expression.
  PropagationResult
  propagateUntil(DenseMap<Value *, IntegerFillType> &ReqFill,
                 DenseMap<const Use *, IntegerFillType> &ReqOperandFill,
                 UndoLog<Value *> &ValueLog, UndoLog<const Use *> &OperandLog,
                 SmallSetVector<Value *, 32> &Worklist) const;

  // Apply fillTypes to the operands of I and return true if it's legal,
  // false otherwise.
  bool applyOpFills(Instruction *I, ArrayRef<IntegerFillType> OpFills,
                    DenseMap<Value *, IntegerFillType> &ReqFill,
                    DenseMap<const Use *, IntegerFillType> &ReqOperandFill,
                    UndoLog<Value *> &ValueUndo,
                    UndoLog<const Use *> &OperandUndo,
                    SmallSetVector<Value *, 32> &Worklist) const;

  // Checks if we form Value V with a specific Width a suitable FillType Fill.
  bool isMatchingOp(Value *V, unsigned Width, IntegerFillType Fill) const;

  // Prefer Zeros and Sign over Anything
  bool stateSatisfies(PromoteState Actual, PromoteState Needed) const;

  // For optimization remarks.
  void noteReject(StringRef Reason, Value *V = nullptr,
                  Instruction *User = nullptr, IntegerFillType Fill = UNDEFINED,
                  IntegerFillType ExistingFill = UNDEFINED,
                  std::optional<unsigned> OperandIndex = std::nullopt) const;

  // Check if we have the required Fill for Value
  MergeFillResult requireFill(DenseMap<Value *, IntegerFillType> &ReqFill,
                              Value *V, IntegerFillType Fill,
                              UndoLog<Value *> *ValueUndo = nullptr) const;

};


MergeFillResult PromoteConstraintSolver::requireFill(
    DenseMap<Value *, IntegerFillType> &ReqFill, Value *V, IntegerFillType Fill,
    UndoLog<Value *> *ValueUndo) const {

  assert(Fill != PENDING && "Pending is'not a suitable requirement");

  if (Fill == ANYTHING && &ReqFill == &RequiredFill && G.containsNode(V))
    return MergeFillResult::Unchanged;

  auto [It, Inserted] = ReqFill.try_emplace(V, Fill);
  if (Inserted) {
    if (ValueUndo)
      ValueUndo->recordOldState(ReqFill, V, UNDEFINED, false);
    return MergeFillResult::Changed;
  }

  IntegerFillType OldFill = It->second;
  // If we have already found a fill for V need to merge them together
  // to find a match
  IntegerFillType Merged = mergeValueFills(V, OldFill, Fill, WiaInfo);
  if (Merged == UNDEFINED)
    return MergeFillResult::Conflict;

  if (Merged == OldFill)
    return MergeFillResult::Unchanged;

  if (ValueUndo)
    ValueUndo->recordOldState(ReqFill, V, OldFill, true);

  It->second = Merged;
  return MergeFillResult::Changed;
}

bool PromoteConstraintSolver::stateSatisfies(PromoteState Actual,
                                             PromoteState Needed) const {
  if (Actual.Width != Needed.Width)
    return false;

  if (Actual.Fill == UNDEFINED || Needed.Fill == UNDEFINED ||
      Actual.Fill == PENDING || Needed.Fill == PENDING)
    return false;

  if (Needed.Fill == ANYTHING)
    return true;

  return Actual.Fill == Needed.Fill;
}

PromoteConstraintSolver::FillChoice PromoteConstraintSolver::chooseOpsFills(
    Instruction *I, IntegerFillType ResultFill,
    SmallVectorImpl<IntegerFillType> &OpsFills) const {
  if (chooseSafeWrapOpFills(I, ResultFill, OpsFills, G.SafeWrap))
    return FillChoice::Concrete;

  // For PHis and Truncs we can return straight away.
  if (isa<PHINode, TruncInst>(I))
    return WiaInfo.ChooseOpsFills(I, ResultFill, OpsFills, nullptr)
               ? FillChoice::Concrete
               : FillChoice::Invalid;

  auto [It, Inserted] = LegalTuples.try_emplace(TupleKey{I, ResultFill});
  auto &Tuples = It->second;

  if (Inserted) {
    if (!WiaInfo.ChooseOpsFills(I, ResultFill, OpsFills, &Tuples)) {
      // Avoid repeated tuple generation on those.
      if (!I->isBinaryOp() || !I->getType()->isIntegerTy() || I->isShift() ||
          I->isIntDivRem())
        LegalTuples.erase(It);
      return FillChoice::Invalid;
    }

    // intrinsics can return concrete fills without generating tuples.
    if (Tuples.empty()) {
      LegalTuples.erase(It);
      return FillChoice::Concrete;
    }
  } else {
    if (Tuples.empty())
      return FillChoice::Invalid;

    if (Tuples.size() == 1) {
      const auto &[L, R, Produced] = Tuples.front();
      assignOpFills(I, L, R, OpsFills);
    }
  }

  if (Tuples.size() > 1) {
    bool HasNonConstraintTuple =
        llvm::any_of(Tuples, [](const FillTuple &Tuple) {
          return std::get<0>(Tuple) == ANYTHING &&
                 std::get<1>(Tuple) == ANYTHING;
        });

    if (HasNonConstraintTuple) {
      OpsFills.assign(I->getNumOperands(), ANYTHING);
      return FillChoice::Concrete;
    }
    OpsFills.clear();
    return FillChoice::MultipleChoices;
  }

  return FillChoice::Concrete;
}

const DenseMap<Value *, IntegerFillType> &
PromoteConstraintSolver::getRequiredFill() const {
  return RequiredFill;
}

const DenseMap<const Use *, IntegerFillType> &
PromoteConstraintSolver::getRequiredOperandFill() const {
  return RequiredOperandFill;
}

const DenseMap<Value *, IntegerFillType> &
PromoteConstraintSolver::getLoadsToExtend() const {
  return LoadsToExtend;
}

const DenseMap<Value *, IntegerFillType> &
PromoteConstraintSolver::getExternalOpsToExtend() const {
  return G.ExternalOpsToExtend;
}

const SourcePromoteRejectInfo &PromoteConstraintSolver::getRejectInfo() const {
  return Reject;
}

void PromoteConstraintSolver::noteReject(
    StringRef Reason, Value *V, Instruction *User, IntegerFillType Fill,
    IntegerFillType ExistingFill, std::optional<unsigned> OperandIndex) const {
  if (Reject.Reason != "BuildGraphFailed")
    return;

  Reject.Reason = Reason;
  Reject.V = V;
  Reject.User = User;
  Reject.Fill = Fill;
  Reject.ExistingFill = ExistingFill;
  Reject.OperandIndex = OperandIndex;
}

bool PromoteConstraintSolver::applyOpFills(
    Instruction *I, ArrayRef<IntegerFillType> OpFills,
    DenseMap<Value *, IntegerFillType> &ReqFill,
    DenseMap<const Use *, IntegerFillType> &ReqOperandFill,
    UndoLog<Value *> &ValueUndo, UndoLog<const Use *> &OperandUndo,
    SmallSetVector<Value *, 32> &Worklist) const {

  if (OpFills.size() != I->getNumOperands())
    return false;

  for (const auto &[OpIdx, U] : enumerate(I->operands())) {
    Value *Producer = U.get();

    if (!isWidenedOp(U))
      continue;

    IntegerFillType NeededFill = OpFills[OpIdx];
    // applyOpFills should only apply concrete requirements.
    if (NeededFill == UNDEFINED || NeededFill == PENDING)
      return false;

    // Constants are per use.
    // They don't necessarily share fill requirements
    // for every use even if it's the same constant value.
    if (isa<ConstantInt>(Producer)) {
      OperandUndo.set(ReqOperandFill, &U, NeededFill);
      continue;
    }

    bool IsGraphNode = G.containsNode(Producer);
    if (IsGraphNode && NeededFill == ANYTHING && &ReqFill == &RequiredFill)
      continue;

    if (!IsGraphNode && !G.LoadsToExtend.contains(Producer) &&
        !G.ExternalOpsToExtend.contains(Producer) &&
        !(Producer->getType()->isIntegerTy(G.DstWidth) &&
          WiaInfo.HasAppropriateSolution(Producer, G.DstWidth, NeededFill))) {
      noteReject("BoundaryFillConflict", Producer, I, NeededFill, UNDEFINED,
                 static_cast<unsigned>(OpIdx));
      return false;
    }

    MergeFillResult R = requireFill(ReqFill, Producer, NeededFill, &ValueUndo);
    if (R == MergeFillResult::Conflict) {
      noteReject("OperandFillConflict", Producer, I, NeededFill,
                 ReqFill.lookup_or(Producer, UNDEFINED),
                 static_cast<unsigned>(OpIdx));
      return false;
    }

    if (R == MergeFillResult::Unchanged)
      continue;

    if (!IsGraphNode && isa<LoadInst>(Producer)) {
      auto It = G.LoadsToExtend.find(Producer);
      if (It != G.LoadsToExtend.end() &&
          (It->second == ANYTHING || It->second == PENDING))
        continue;
    }

    // We need to revisit if we have a change in the requireFill.
    if (auto *ProducerI = dyn_cast<Instruction>(Producer)) {
      if (R == MergeFillResult::Changed &&
          (G.containsNode(ProducerI) || isa<LoadInst>(ProducerI) ||
           G.InstsToReplaceWithOperand.contains(ProducerI)))
        Worklist.insert(ProducerI);
    }
  }
  return true;
}

PromoteConstraintSolver::PropagationResult
PromoteConstraintSolver::propagateUntil(
    DenseMap<Value *, IntegerFillType> &ReqFill,
    DenseMap<const Use *, IntegerFillType> &ReqOperandFill,
    UndoLog<Value *> &ValueLog, UndoLog<const Use *> &OperandLog,
    SmallSetVector<Value *, 32> &Worklist) const {

  while (!Worklist.empty()) {
    Value *V = Worklist.pop_back_val();

    if (G.ExternalOpsToExtend.contains(V))
      continue;

    bool IsGraphNode = G.containsNode(V);
    if (!IsGraphNode && !G.LoadsToExtend.contains(V)) {
      IntegerFillType Needed = ReqFill.lookup_or(V, UNDEFINED);
      if (!isMatchingOp(V, G.DstWidth, Needed)) {
        noteReject("BoundaryFillConflict", V, nullptr, Needed);
        return {PropagationStatus::Conflict};
      }
      continue;
    }

    auto *VI = dyn_cast<Instruction>(V);
    if (!VI)
      continue;

    auto LoadIt =
        isa<LoadInst>(VI) ? G.LoadsToExtend.find(VI) : G.LoadsToExtend.end();
    if (LoadIt != G.LoadsToExtend.end()) {
      IntegerFillType GraphLFill = LoadIt->second;
      // Try to find the match with the planned load ext
      // and the fill required by its users.
      auto ReqIt = ReqFill.find(VI);
      IntegerFillType RequiredFill =
          ReqIt == ReqFill.end() ? ANYTHING : ReqIt->second;

      // If we cannot find a fill for the load, just reject it.
      if (GraphLFill == UNDEFINED || RequiredFill == UNDEFINED ||
          RequiredFill == PENDING) {
        noteReject("Unresolved Load Fill", VI, nullptr, RequiredFill);
        return {PropagationStatus::Conflict};
      }

      // GraphLFill and RequiredFill must agree
      IntegerFillType LoadFill =
          mergeValueFills(VI, GraphLFill, RequiredFill, WiaInfo);

      if (LoadFill == UNDEFINED) {
        noteReject("LoadFillConflict", VI, nullptr, RequiredFill);
        return {PropagationStatus::Conflict};
      }

      if (LoadFill == PENDING) {
        if (RequiredFill != ANYTHING &&
            (RequiredFill != SIGN && RequiredFill != ZEROS)) {
          noteReject("PendingLoadFill", VI, nullptr, LoadFill);
          return {PropagationStatus::Conflict};
        }
        LoadFill = RequiredFill;
      }
      continue;
    }

    auto ReqIt = ReqFill.find(V);
    if (ReqIt == ReqFill.end() && !IsGraphNode) {
      noteReject("MissingFillRequirement", V);
      return {PropagationStatus::Conflict};
    }
    IntegerFillType ResultFill =
        ReqIt == ReqFill.end() ? ANYTHING : ReqIt->second;
    // The Cast will be removed, so it's operand must supply
    // the required fill.
    if (isa<CastInst>(VI) && G.InstsToReplaceWithOperand.contains(VI)) {
      Value *Op0 = VI->getOperand(0);
      IntegerFillType OpFill = ResultFill;
      if (isa<ZExtInst>(VI))
        OpFill = ZEROS;
      else if (isa<SExtInst>(VI))
        OpFill = SIGN;

      if (isa<SExtInst, ZExtInst>(VI)) {
        bool ResultMatches = ResultFill == ANYTHING || ResultFill == OpFill ||
                             (ResultFill == SIGN && isa<ZExtInst>(VI));
        if (!ResultMatches) {
          noteReject("ReplacementResultFillConflict", VI, nullptr, ResultFill);
          return {PropagationStatus::Conflict};
        }

        if (!G.containsNode(Op0) && !G.LoadsToExtend.contains(Op0) &&
            !G.ExternalOpsToExtend.contains(Op0) &&
            !isMatchingOp(Op0, G.DstWidth, OpFill)) {
          noteReject("ReplacementOperandUnavailable", Op0, VI, OpFill);
          return {PropagationStatus::Conflict};
        }
      }
      if (auto *Trunc = dyn_cast<TruncInst>(VI)) {
        bool PreservesFill =
            ResultFill == ANYTHING ||
            (ResultFill == SIGN && Trunc->hasNoSignedWrap()) ||
            (ResultFill == ZEROS && Trunc->hasNoUnsignedWrap());

        if (PreservesFill && Op0->getType()->isIntegerTy(G.DstWidth))
          continue;

        if (Op0->getType()->isIntegerTy(G.DstWidth)) {
          unsigned NarrowWidth = Trunc->getType()->getIntegerBitWidth();
          if (ResultFill == SIGN && IsKnownSignExtended(Op0, NarrowWidth))
            continue;

          if (ResultFill == ZEROS) {
            KnownBits Known;
            WiaInfo.GetKnownBitsCached(Op0, Known);
            unsigned ExtraBits = Known.getBitWidth() - NarrowWidth;
            if (Known.countMinLeadingZeros() >= ExtraBits)
              continue;
          }
        }
        noteReject("ReplacementTruncFillConflict", VI, nullptr, ResultFill);
        return {PropagationStatus::Conflict};
      }

      MergeFillResult R = requireFill(ReqFill, Op0, OpFill, &ValueLog);
      if (R == MergeFillResult::Conflict) {
        noteReject("ReplacementOperandFillConflict", Op0, VI, OpFill);
        return {PropagationStatus::Conflict};
      }
      if (R == MergeFillResult::Changed)
        Worklist.insert(Op0);
      continue;
    }
    SmallVector<IntegerFillType, 6> OpFills;
    FillChoice Choice = chooseOpsFills(VI, ResultFill, OpFills);
    // If we didn't found a fill, try to find a concrete
    // requirement on an icmp with constant
    if (Choice == FillChoice::MultipleChoices) {
      if (auto *Cmp = dyn_cast<ICmpInst>(VI)) {
        Value *Src = Cmp->getOperand(0);
        auto *C = dyn_cast<ConstantInt>(Cmp->getOperand(1));
        if (!C) {
          C = dyn_cast<ConstantInt>(Cmp->getOperand(0));
          Src = Cmp->getOperand(1);
        }

        IntegerFillType Fill = ReqFill.lookup_or(Src, ANYTHING);
        if (G.containsNode(Src) && C && !C->isNegative() &&
            (Fill == SIGN || Fill == ZEROS) && WiaInfo.IsKnownNonNeg(Src) &&
            llvm::is_contained(LegalTuples.at(TupleKey{VI, ResultFill}),
                               FillTuple{Fill, Fill, ZEROS})) {
          assignOpFills(VI, Fill, Fill, OpFills);
          Choice = FillChoice::Concrete;
        }
      }
    }

    if (Choice == FillChoice::Invalid) {
      noteReject("NoOperandFill", VI, nullptr, ResultFill);
      return {PropagationStatus::Conflict};
    }

    // if we have PENDING, then the operand fills still have a choice
    // between legal fill requirement.
    if (Choice == FillChoice::MultipleChoices)
      return {PropagationStatus::Continue, VI, ResultFill};

    bool Applied = applyOpFills(VI, OpFills, ReqFill, ReqOperandFill, ValueLog,
                                OperandLog, Worklist);
    if (!Applied) {
      noteReject("OperandFillConflict", VI, nullptr, ResultFill);
      return {PropagationStatus::Conflict};
    }
  }
  return {PropagationStatus::Done};
}

bool PromoteConstraintSolver::propagateFillRequirements(
    DenseMap<Value *, IntegerFillType> &ReqFill,
    DenseMap<const Use *, IntegerFillType> &ReqOperandFill,
    SmallSetVector<Value *, 32> &Worklist, unsigned Depth,
    unsigned &Attempts) const {

  if (Depth > MaxPendingResolveDepth) {
    noteReject("PendingResolveDepthLimit", nullptr, nullptr, UNDEFINED);
    return false;
  }

  // UndoLogs are needed to remove expensive copy operations
  UndoLog<Value *> ValueLog;
  UndoLog<const Use *> OperandLog;

  SmallVector<PendingDecision, 4> Decisions;

  std::optional<SourcePromoteRejectInfo> LastFailure;
  std::optional<SourcePromoteRejectInfo> InitialReject;

  auto Restore = [&](PendingDecision &D, size_t NumTuples) {
    // Preserve a useful rejection reason before restoring saved state.
    if (Reject.Reason != "BuildGraphFailed" &&
        Reject.Reason != "NoPendingSelection")
      LastFailure = Reject;

    if (D.NextTuple < NumTuples) {
      ValueLog.rollback(D.ValueCheckpoint);
      OperandLog.rollback(D.OperandCheckpoint);
      if (D.NextTuple + 1 == NumTuples)
        Worklist.swap(D.SavedWorklist);
      else
        Worklist = D.SavedWorklist;
    }

    Reject = *InitialReject;
  };

  while (true) {

    PropagationResult R = {PropagationStatus::Conflict};

    // continue propagating if we have more room.
    if (Depth + Decisions.size() <= MaxPendingResolveDepth)
      R = propagateUntil(ReqFill, ReqOperandFill, ValueLog, OperandLog,
                         Worklist);
    else
      noteReject("PendingResolveDepthLimit", nullptr, nullptr, UNDEFINED);

    // Currently we accept the first consistent fill assignment
    // Later on when the cost model is good enough
    // We can compare to find the cheaper assignment.
    // This will have better result in some cases.
    if (R.Status == PropagationStatus::Done) {
      for (const auto &[V, PlannedFill] : G.ExternalOpsToExtend) {
        IntegerFillType Needed = ReqFill.lookup_or(V, ANYTHING);
        if (mergeValueFills(V, PlannedFill, Needed, WiaInfo) == UNDEFINED) {
          noteReject("ExternalDemandMisMatch", V, nullptr, Needed, PlannedFill);
          R.Status = PropagationStatus::Conflict;
          break;
        }
      }
    }
    if (R.Status == PropagationStatus::Done) {
      if (!Decisions.empty())
        Reject = *InitialReject;
      return true;
    }

    if (R.Status == PropagationStatus::Continue) {
      ++Attempts;
      if (Attempts > MaxPendingResolveAttempts) {
        noteReject("PendingResolveAttemptLimit", R.I, nullptr, R.Fill);
        return false;
      }
      // Get the operands legal tuples for propagation result
      // that came from the worklist.
      const auto &Tuples = LegalTuples.at(TupleKey{R.I, R.Fill});
      if (Tuples.empty())
        noteReject("NoLegalFillTuple", R.I, nullptr, R.Fill);
      else if (Depth + Decisions.size() >= MaxPendingResolveDepth)
        noteReject("PendingResolveDepthLimit", R.I, nullptr, R.Fill);
      else {
        if (!InitialReject)
          InitialReject = Reject;
        // Record it as a decision but take a snapshot so we can rollback.
        Decisions.emplace_back(R.I, R.Fill, ValueLog.snapshot(),
                               OperandLog.snapshot(), Worklist);
      }
    }
    // Traverse the decisions so we can propagate the assignment of OpFills.
    while (!Decisions.empty()) {
      PendingDecision &D = Decisions.back();
      const auto &Tuples = LegalTuples.at(TupleKey{D.I, D.Fill});
      size_t TuplesSize = Tuples.size();
      // Undo previous tuple's changes before trying something else.
      if (D.NextTuple != 0 &&
          (D.NextTuple < TuplesSize || Decisions.size() == 1))
        Restore(D, Tuples.size());

      if (D.NextTuple == TuplesSize || D.I->getNumOperands() < 2) {
        noteReject("NoPendingSelection", D.I, nullptr, D.Fill);
        Decisions.pop_back();
        continue;
      }

      const auto &[LeftF, RightF, ResF] = Tuples[D.NextTuple];
      ++D.NextTuple;
      // We can stop recording now, no parent left.
      if (Decisions.size() == 1 && D.NextTuple == TuplesSize) {
        ValueLog.Recording = false;
        OperandLog.Recording = false;
      }

      SmallVector<IntegerFillType, 6> OpFills;
      assignOpFills(D.I, LeftF, RightF, OpFills);
      // Impose those tuple's operands requirements throught the Undo Logs.
      // A successful assignment resumes propagation.
      // Otherwise, we try the next tuple.
      bool LegalOps = applyOpFills(D.I, OpFills, ReqFill, ReqOperandFill,
                                   ValueLog, OperandLog, Worklist);
      if (LegalOps) {
        // Propagate this assignment of OpFills before trying something else.
        break;
      }
    }
    if (Decisions.empty()) {
      if (Reject.Reason == "NoPendingSelection" && LastFailure)
        Reject = *LastFailure;
      return false;
    }
  }
}

bool PromoteConstraintSolver::computeFillRequirements() {
  SmallSetVector<Value *, 32> Worklist;
  // Get the Required fill for every node in the graph
  // If we have a conflict reject the whole graph
  // If we have a change push it to the worklist.
  MergeFillResult R = requireFill(RequiredFill, G.Source, G.Fill);
  if (R == MergeFillResult::Conflict) {
    noteReject("SourceFillConflict", G.Source, nullptr, G.Fill);
    return false;
  }

  for (const auto &[V, _] : G.Nodes)
    if (V != G.Source)
      Worklist.insert(V);

  // Try to find the fill for the operand that will replace the cast.
  for (Instruction *I : G.ExtsToReplaceWithOperand) {
    auto *CI = cast<CastInst>(I);
    IntegerFillType CastFill =
        I->getOpcode() == Instruction::SExt ? SIGN : ZEROS;
    if (auto *ZExt = dyn_cast<ZExtInst>(CI);
        ZExt && ZExt->getSrcTy()->getScalarSizeInBits() == G.SrcWidth &&
        ((G.Fill == SIGN && ZExt->hasNonNeg()) ||
         canUseSignFillForGEP(ZExt, G.SrcWidth, DL)))
      CastFill = SIGN;

    // CastFill and RequiredFill for the operand must agree.
    MergeFillResult Required =
        requireFill(RequiredFill, CI->getOperand(0), CastFill);
    if (Required == MergeFillResult::Conflict) {
      noteReject("ExtOperandFillConflict", CI->getOperand(0), I, CastFill);
      return false;
    }
    if (Required == MergeFillResult::Changed && CI->getOperand(0) != G.Source)
      Worklist.insert(CI->getOperand(0));
  }
  // Source has priority, otherwise we might revisit some nodes
  // if they started with anything.
  Worklist.insert(G.Source);

  unsigned Attempts = 0;
  // Propagate the fill requirements to try to remove any pending ones.
  if (!propagateFillRequirements(RequiredFill, RequiredOperandFill, Worklist, 0,
                                 Attempts)) {
    noteReject("FillPropagationFailed", G.Source, nullptr, G.Fill);
    return false;
  }

  for (auto &[V, GFill] : G.LoadsToExtend) {
    auto It = RequiredFill.find(V);
    if (It == RequiredFill.end() || G.ExternalOpsToExtend.contains(V))
      continue;

    IntegerFillType Fill = GFill;
    if (Fill == ANYTHING || Fill == PENDING)
      Fill = It->second;

    if (Fill == ANYTHING)
      Fill = ZEROS;

    if (Fill != SIGN && Fill != ZEROS) {
      noteReject("LoadsToExtendUnknownFill", G.Source, nullptr, G.Fill);
      return false;
    }
    LoadsToExtend.try_emplace(V, Fill);
  }

  return true;
}

bool PromoteConstraintSolver::solve() {
  if (!computeFillRequirements()) {
    noteReject("FillRequirementsFailed", G.Source, nullptr, G.Fill);
    return false;
  }
  return true;
}

bool PromoteConstraintSolver::isMatchingOp(Value *V, unsigned Width,
                                           IntegerFillType Fill) const {
  if (Fill == UNDEFINED || Fill == PENDING)
    return false;

  if (auto *CI = dyn_cast<ConstantInt>(V)) {
    return Width >= CI->getBitWidth() &&
           (Fill == SIGN || Fill == ZEROS || Fill == ANYTHING);
  }

  if (auto *LI = dyn_cast<LoadInst>(V)) {
    unsigned LoadWidth = LI->getType()->getScalarSizeInBits();
    if (Width == G.DstWidth && LoadWidth < Width &&
        (Fill == SIGN || Fill == ZEROS) &&
        (G.LoadsToExtend.contains(V) || G.ExternalOpsToExtend.contains(V)))
      return true;
  }

  PromoteState Needed = {Width, Fill};

  auto It = G.LoadsToExtend.find(V);
  if (It != G.LoadsToExtend.end()) {
    return stateSatisfies({G.DstWidth, It->second}, Needed);
  }

  auto ExtOpIt = G.ExternalOpsToExtend.find(V);
  if (ExtOpIt != G.ExternalOpsToExtend.end())
    return stateSatisfies({G.DstWidth, ExtOpIt->second}, {Width, Fill});

  if (auto It = G.findNode(V); It != G.Nodes.end()) {
    uint8_t States = Possible[It->second];
    return Width == G.DstWidth &&
           (Fill == ANYTHING ? States != 0 : (States & (1u << Fill)) != 0);
  }

  return V->getType()->isIntegerTy(Width) &&
         WiaInfo.HasAppropriateSolution(V, Width, Fill);
}

ExtRewritePlan
ExtRewritePlanner::makeSourcePromotePlan(SourcePromoteGraph &G,
                                         PromoteConstraintSolver &Solver) {
  ExtRewritePlan Plan;
  Plan.Kind = ExtStrategy::PromoteSource;
  Plan.Ext = G.Ext;
  Plan.Source = G.Source;
  Plan.NarrowWidth = G.SrcWidth;
  Plan.WideWidth = G.DstWidth;
  Solver.moveSolutionTo(Plan);
  Plan.WideLeaves = std::move(G.WideLeaves);
  Plan.ExternalOpsToExtend = std::move(G.ExternalOpsToExtend);

  assert(llvm::none_of(Plan.LoadsToExtend,
                       [](const auto &Entry) {
                         return Entry.second == PENDING ||
                                Entry.second == UNDEFINED;
                       }) &&
         "unresolved load fill reached source-promote plan");

  Plan.BoundaryRewrites = std::move(G.BoundaryRewrites);

  for (auto &Load : Plan.LoadsToExtend) {
    assert(
        (Load.second == SIGN || Load.second == ZEROS) &&
        "Found load supposed to be extended but doesn't have a matching fill");
  }
  Plan.ExtsToReplaceWithOperand = std::move(G.ExtsToReplaceWithOperand);

  for (auto &Entry : G.Nodes) {
    // For the rebuilder.
    Plan.RequiredFill.try_emplace(Entry.first, ANYTHING);

    auto *I = dyn_cast<Instruction>(Entry.first);
    if (!I)
      continue;

    if (G.InstsToReplaceWithOperand.contains(I))
      continue;

    Plan.InstsToPromote.insert(I);
  }

  Plan.SourcePromoteFill = G.Fill;
  Plan.NeedsSourcePromoteZExtRepair = G.NeedsRootZExtRepair;
  Plan.SafeWrap = std::move(G.SafeWrap);
  Plan.InstsToRemove.append(G.InstsToRemove.begin(), G.InstsToRemove.end());

  for (Instruction *I : Plan.InstsToRemove)
    Plan.InstsToPromote.remove(I);

  Plan.InstsToReplaceWithOperand = std::move(G.InstsToReplaceWithOperand);

  return Plan;
}
