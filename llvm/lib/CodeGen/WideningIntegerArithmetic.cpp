/* Sign Extension Optimizations for the LLVM Compiler.
 * Proposed by Panagiotis Karouzakis (karouzakis@ics.forth.gr)
 *
 * */

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/CodeGen/WideningIntegerArithmeticInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Use.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Support/TypeSize.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <algorithm>
#include <climits>
#include <iterator>
#include <vector>

using namespace llvm;
#define PASS_NAME "WideningIntegerArithmetic"

#define DEBUG_TYPE "widening-integer-arithmetic"

#define ICMP_CONSTANT 2000
STATISTIC(NumSExtsDropped,
          "Number of ISD::SIGN_EXTEND nodes that were dropped");
STATISTIC(NumZExtsDropped,
          "Number of ISD::ZERO_EXTEND nodes that were dropped");
STATISTIC(NumAnyExtsDropped,
          "Number of ISD::ANY_EXTEND nodes that were dropped");
STATISTIC(NumTruncatesDropped,
          "Number of ISD::TRUNCATE nodes that were dropped");

namespace {

class WideningIntegerArithmetic : public FunctionPass {
  const TargetMachine *TM = nullptr;
  const TargetLowering *TLI = nullptr;
  const TargetTransformInfo *TTI = nullptr;
  const DataLayout *DL = nullptr;
  LLVMContext *Ctx = nullptr;

public:
  static char ID;
  WideningIntegerArithmetic() : FunctionPass(ID) {
    initializeWideningIntegerArithmeticPass(*PassRegistry::getPassRegistry());
  }

  StringRef getPassName() const override { return PASS_NAME; }
  bool runOnFunction(Function &F) override;

  using isSolvedMap = DenseMap<Value *, bool>;
  using SolutionSet = SmallVector<WideningIntegerSolutionInfo *>;
  using SolutionSetParam = SmallVectorImpl<WideningIntegerSolutionInfo *>;
  using AvailableSolutionsMap = DenseMap<Value *, SolutionSet>;

  using BinOpWidth = std::tuple<unsigned char, unsigned char, unsigned char>;
  using WidthsSet = SmallVector<BinOpWidth>;
  using OperatorWidthsMap = DenseMap<unsigned, WidthsSet>;
  using TargetWidthsMap = DenseMap<unsigned, OperatorWidthsMap>;

  using FillTypeSet =
      SmallSet<std::tuple<IntegerFillType, IntegerFillType, IntegerFillType>,
               4>;
  using UnaryFillTypeSet =
      SmallSet<std::tuple<IntegerFillType, IntegerFillType>, 2>;

  using OperatorFillTypesMap = DenseMap<unsigned, FillTypeSet>;
  typedef WideningIntegerSolutionInfo *SolutionType;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<TargetPassConfig>();
    AU.addRequired<TargetTransformInfoWrapperPass>();
  }

private:
  enum IntegerFillType ExtensionChoice;

  // checks whether an Instruction in the Function F is visited and solved`
  isSolvedMap SolvedInstructions;
  DenseMap<Value *, bool> InsideWorklist;
  // Holds all the available solutions
  AvailableSolutionsMap AvailableSolutions;

  TargetWidthsMap TargetWidths;

  DenseMap<unsigned, UnaryFillTypeSet> UnaryFillTypesMap;
  DenseMap<unsigned, FillTypeSet> FillTypesMap;
  SmallPtrSet<Instruction *, 8> InstsToRemove;
  SmallPtrSet<Value *, 8> Promoted;

  UnaryFillTypeSet getUnaryFillTypes(unsigned Opcode);
  FillTypeSet getFillTypes(Instruction *Instr);
  inline IntegerFillType getOrNullFillType(Instruction *Binop,
                                           FillTypeSet availableFillTypes,
                                           IntegerFillType Left,
                                           IntegerFillType Right);

  SmallVector<WideningIntegerSolutionInfo *>
  visit_widening(Value *VI, std::queue<Value *> &Worklist);

  FillTypeSet getOperandFillTypes(Instruction *Instr);

  void setFillType(Type *IType);
  inline bool isSolved(Value *V);
  inline bool isInWorklist(Value *V);
  inline bool isLegalToPromote(Instruction *Instr);

  bool addNonRedudant(SolutionSet &Solutions,
                      WideningIntegerSolutionInfo *GeneratedSol);
  inline bool hasTypeGarbage(IntegerFillType Fill);
  inline bool hasTypeT(IntegerFillType Fill);
  inline bool hasTypeS(IntegerFillType Fill);
  bool closure(Instruction *Instr, std::queue<Value *> &Worklist);
  bool tryClosure(Instruction *Instr, std::queue<Value *> &Worklist);

  bool visitInstruction(Value *VI, std::queue<Value *> &Worklist);
  bool visitBINOP(Instruction *Binop, std::queue<Value *> &Worklist);
  bool combineBINOPSols(Instruction *Binop, std::queue<Value *> &Worklist);
  bool visitLOAD(Instruction *Instr);
  bool visitSTORE(Instruction *Instr);
  bool visitUNOP(Instruction *Instr);
  std::list<WideningIntegerSolutionInfo *>
  visitFILL(Instruction *Instr, std::queue<Value *> &Worklist);
  std::list<WideningIntegerSolutionInfo *>
  visitWIDEN(Instruction *Instr, std::queue<Value *> &Worklist);
  std::list<WideningIntegerSolutionInfo *>
  visitWIDEN_GARBAGE(Instruction *Instr, std::queue<Value *> &Worklist);
  std::list<WideningIntegerSolutionInfo *>
  visitNARROW(Instruction *Instr, std::queue<Value *> &Worklist);
  bool visitDROP_EXT(Instruction *Instr, std::queue<Value *> &Worklist);
  bool visitDROP_TRUNC(Instruction *Instr, std::queue<Value *> &Worklist);
  bool visitEXTLO(Instruction *Instr, SmallVector<unsigned short> LowWidths,
                  std::queue<Value *> &Worklist);
  bool visitSUBSUME_FILL(Instruction *Instr);
  bool visitSUBSUME_INDEX(Instruction *Instr);
  bool visitNATURAL(Instruction *Instr);
  bool visitCONSTANT(ConstantInt *CI);
  bool visitPHI(Instruction *Instr, std::queue<Value *> &Worklist);
  bool solveSimplePhis(Instruction *Instr,
                       SmallVector<Value *, 32> IncomingValues,
                       std::queue<Value *> &Worklist);

  // A map that holds for every instruction the best solution
  // for each of the users of that instruction.
  DenseMap<Instruction *, DenseMap<Value *, WideningIntegerSolutionInfo *>>
      BestUserSolsPerInst;
  DenseMap<Instruction *, WideningIntegerSolutionInfo *> BestSolPerInst;
  // For every instruction **I** inside the Function
  // it calculates the best combination among the legal
  // solutions of every User of **I**
  void calcBestUserSols(Function &F);

  void replaceAllUsersOfWith(Value *From, Value *To);

  // Applies this *Sol* to the Instruction *Instr*
  bool applySingleSol(Instruction *Instr, WideningIntegerSolutionInfo *Sol);
  // iterates all the uses of an Instruction that is solved
  // and applies the updated Instruction to all the uses.
  // Note that it might need to update the uses of the Instruction also.
  bool applyChain(Instruction *Instr,
                  DenseMap<Value *, WideningIntegerSolutionInfo *>);

  std::vector<unsigned short> IntegerSizes = {8, 16, 32, 64};

  unsigned RegisterBitWidth = 0;
  Value *getPhiSuccessor(Value *V);

  BinOpWidth createWidth(unsigned char op1, unsigned char op2, unsigned dst);

  bool IsBinop(unsigned Opcode);
  bool IsUnop(unsigned Opcode);
  bool IsTruncate(unsigned Opcode);
  bool IsExtension(unsigned Opcode);
  bool IsLit(unsigned Opcode);
  bool IsLoad(unsigned Opcode);
  bool IsStore(unsigned Opcode);
  bool IsPHI(unsigned Opcode);
  // Helper functions
  inline unsigned int getScalarSize(const Type *Typ) const;
  inline unsigned getExtensionChoice(enum IntegerFillType ExtChoice);
  inline unsigned getExtCost(Instruction *Instr,
                             WideningIntegerSolutionInfo *Sol,
                             unsigned short IntegerSize);
  void initOperatorsFillTypes();
  void printInstrSols(Value *V);
  inline Type *getTypeFromInteger(int Integer);
  inline short int getKnownFillTypeWidth(Value *V);

  bool inline isLegalAndMatching(WideningIntegerSolutionInfo *Sol1,
                                 WideningIntegerSolutionInfo *Sol2);
  bool mayOverflow(Instruction *Instr);
  bool createDefaultSol(Value *V);
};
} // end anonymous namespace

char WideningIntegerArithmetic::ID = 0;

struct SolutionInfo {
  bool Changed; // indicates if the sol is updated
  SmallVector<WideningIntegerSolutionInfo *> Solutions;
};

void WideningIntegerArithmetic::printInstrSols(Value *V) {
  auto Sols = AvailableSolutions[V];
  if (auto *I = dyn_cast<Instruction>(V)) {
    dbgs() << "Printing Sols of Inst: " << *I << "\n";
  } else if (isa<ConstantInt>(V)) {
    dbgs() << "Printing Sols of ConstantInt: " << *V << "\n";
  }
  for (WideningIntegerSolutionInfo *Sol : Sols) {
    // dbgs() << "----- Solution " << ++i << "\n" << (*Solution) << "\n";
    dbgs() << "------- Solution oldWidth: " << Sol->getWidth()
           << " newWidth: " << Sol->getUpdatedWidth() << "\n";
  }
}

inline unsigned
WideningIntegerArithmetic::getExtensionChoice(enum IntegerFillType ExtChoice) {
  return ExtChoice == SIGN ? Instruction::SExt : Instruction::ZExt;
}

bool WideningIntegerArithmetic::IsBinop(unsigned Opcode) {
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
  case Instruction::ICmp:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor:
    return true;
  default:
    return false;
  }
  return false;
}

// TODO all UNOPS are Intrinsics check IntrinsicInst
bool WideningIntegerArithmetic::IsUnop(unsigned Opcode) {
  switch (Opcode) {
  default:
    return false;
  }
  return false;
}

bool WideningIntegerArithmetic::IsTruncate(unsigned Opcode) {
  switch (Opcode) {
  case Instruction::Trunc:
    return true;
  default:
    break;
  }
  return false;
}

bool WideningIntegerArithmetic::IsPHI(unsigned Opcode) {
  switch (Opcode) {
  case Instruction::PHI:
    return true;
  default:
    return false;
  }
}

bool WideningIntegerArithmetic::IsExtension(unsigned Opcode) {
  switch (Opcode) {
  case Instruction::ZExt:
  case Instruction::SExt:
    return true;
  default:
    return false;
  }
  return false;
}

bool WideningIntegerArithmetic::IsStore(unsigned Opcode) {
  switch (Opcode) {
  case Instruction::Store:
    return true;
  default:
    break;
  }
  return false;
}

bool WideningIntegerArithmetic::IsLoad(unsigned Opcode) {
  switch (Opcode) {
  case Instruction::Load:
    return true;
  default:
    break;
  }
  return false;
}

long int TruncCounter = 0;
long int ExtCounter = 0;

bool WideningIntegerArithmetic::visitInstruction(
    Value *VI, std::queue<Value *> &Worklist) {
  bool Changed = false;
  // We can cast VI to instruction because we know that VI is an Instruction.
  if (auto *Instr = dyn_cast<Instruction>(VI)) {
    unsigned Opcode = Instr->getOpcode();
    if (IsBinop(Opcode)) {
      // dbgs() << " and Visiting Binop..\n";
      return visitBINOP(Instr, Worklist);
    }
    // else if(IsUnop(Opcode)){
    //   dbgs() << " and Visiting Unop...\n";
    // return visitUNOP(Instr);
    //}
    else if (IsLoad(Opcode)) {
      // dbgs() << " and Visiting Load...\n";
      return visitLOAD(Instr);
    } else if (IsStore(Opcode)) {
      // dbgs() << " and Visting Store...\n";
      return visitSTORE(Instr);
    } else if (IsExtension(Opcode)) {
      ExtCounter++;
      // dbgs() << " and Visiting Extension...\n";
      return visitDROP_EXT(Instr, Worklist);
    } else if (IsTruncate(Opcode)) {
      TruncCounter++;
      // dbgs() << " and Visiting Truncation ..\n";
      return visitDROP_TRUNC(Instr, Worklist);
    } else if (IsPHI(Opcode)) {
      // dbgs() << " and Visiting PHI ..\n";
      return visitPHI(Instr, Worklist);
    } else {
      // dbgs() << "Could not found an Opcode is " << Opcode << "\n";
      ////dbgs() << "Defaut sol for Opcode " << OpcodesToStr[Opcode] << "\n";
      // default solution so we can initialize all the nodes with some solution
      // set.
      auto *Sol = new WideningIntegerSolutionInfo(
          Opcode, Opcode, ANYTHING, RegisterBitWidth, RegisterBitWidth,
          /* TODO CHECK */ RegisterBitWidth, 0, WIAK_UNKNOWN, Instr);
      Changed = addNonRedudant(AvailableSolutions[VI], Sol);
    }
  } else if (auto *CI = dyn_cast<ConstantInt>(VI)) {
    // dbgs() << " and Visiting Constant.." << "\n";
    return visitCONSTANT(CI);
  } else {

    // dbgs() << "Careful here Adding Default value" << "\n";
    int FillTypeWidth = getKnownFillTypeWidth(VI);
    auto *Sol = new WideningIntegerSolutionInfo(
        0, 0, ANYTHING, FillTypeWidth, RegisterBitWidth,
        /* TODO CHECK */ RegisterBitWidth, 0, WIAK_UNKNOWN, Instr);
    Changed = addNonRedudant(AvailableSolutions[VI], Sol);
    // dbgs() << "Adding Default value Succedded is " << Changed << "\n";
  }

  // TODO check if we are missing opcodes here, need to add them.
  return Changed;
}

bool WideningIntegerArithmetic::addNonRedudant(
    SolutionSet &Solutions, WideningIntegerSolutionInfo *GeneratedSol) {
  bool WasRedudant = false;
  int RedudantNodeToDeleteCost = INT_MAX;
  LLVM_DEBUG(dbgs() << "Begin Add Non Redudant -> " << '\n');
  for (auto *It = Solutions.begin(); It != Solutions.end();) {
    int ret = (*It)->IsRedudant((*GeneratedSol));
    if (ret == -1) { // GeneratedSol is redudant
      WasRedudant = true;
      It++;
      // dbgs() << "GeneratedSol is redudant! ret = -1" << '\n';
    } else if (ret == 1) { // Sol is redudant
      assert(GeneratedSol->getCost() < RedudantNodeToDeleteCost);
      // dbgs() << "ret = 1 Current Sol is Redudant .... Deleting it --> "
      //       << (**It) << '\n';
      It = Solutions.erase(It);
      // TODO consider change data structure for Possible small optimization
    } else { // ret == 0 no redudant found
      It++;
    }
  }
  if (!WasRedudant) {
    LLVM_DEBUG(dbgs() << "!!!!!!!!!!!!!!Adding Solution --> "
                      << OpcodesToStr[GeneratedSol->getOpcode()] << "\n");
    //<< *GeneratedSol << '\n';
    Solutions.push_back(GeneratedSol);
    return true;
  }
  LLVM_DEBUG(dbgs() << "Finished and Returning False --> " << '\n');
  return false;
}

WideningIntegerArithmetic::FillTypeSet
WideningIntegerArithmetic::getFillTypes(Instruction *Instr) {
  unsigned Opcode = Instr->getOpcode();
  // dbgs() << "Opcode is " << Opcode << "    " << OpcodesToStr[Opcode] << "\n";
  unsigned IsdOpc = TLI->InstructionOpcodeToISD(Opcode);
  assert(IsBinop(Opcode) && "Not a binary operator.");
  if (auto *CI = dyn_cast<ICmpInst>(Instr)) {
    unsigned Pred = CI->getPredicate();
    // dbgs() << "Searching in " << ICMP_CONSTANT + Pred << "\n";
    return FillTypesMap[ICMP_CONSTANT + Pred];
  }

  return FillTypesMap[IsdOpc];
}

WideningIntegerArithmetic::UnaryFillTypeSet
WideningIntegerArithmetic::getUnaryFillTypes(unsigned Opcode) {

  assert(IsUnop(Opcode) == true && "Not a unary operator to get the fillTypes");
  return UnaryFillTypesMap[TLI->InstructionOpcodeToISD(Opcode)];
}

inline IntegerFillType WideningIntegerArithmetic::getOrNullFillType(
    Instruction *Binop, FillTypeSet availableFillTypes,
    IntegerFillType LeftFillType, IntegerFillType RightFillType) {

  for (auto FillType : availableFillTypes) {
    if ((std::get<0>(FillType) == LeftFillType &&
         std::get<1>(FillType) == RightFillType) ||
        (std::get<0>(FillType) == ANYTHING &&
         std::get<1>(FillType) == RightFillType))
      return std::get<2>(FillType);
    if (auto *OBO = dyn_cast<OverflowingBinaryOperator>(Binop)) {
      if (OBO->hasNoUnsignedWrap() && LeftFillType == ZEROS &&
          RightFillType == ZEROS)
        return ZEROS;
      if (OBO->hasNoSignedWrap() && LeftFillType == SIGN &&
          RightFillType == SIGN)
        return SIGN;
    }
  }
  return UNDEFINED;
}

void WideningIntegerArithmetic::setFillType(Type *IType) {
  /*	EVT VT = TLI->getValueType(*DL, IType);
        EVT NewVT = VT;

        if(TLI->isSExtCheaperThanZExt(VT, NewVT)){
    ExtensionChoice = SIGN;
        }else{
                ExtensionChoice = ZEROS;
        }*/
  ExtensionChoice = ZEROS;
}

bool WideningIntegerArithmetic::runOnFunction(Function &F) {

  initOperatorsFillTypes();

  DL = &F.getParent()->getDataLayout();
  TM = &getAnalysis<TargetPassConfig>().getTM<TargetMachine>();
  const TargetSubtargetInfo *SubtargetInfo = TM->getSubtargetImpl(F);
  TLI = SubtargetInfo->getTargetLowering();
  TTI = &getAnalysis<TargetTransformInfoWrapperPass>().getTTI(F);
  Ctx = &F.getParent()->getContext();
  DL = &F.getParent()->getDataLayout();
  auto &FBB = F.getEntryBlock();
  if (FBB.empty()) {
    return false;
  }
  setFillType((&*(FBB.begin())->getType()));
  RegisterBitWidth =
      TTI->getRegisterBitWidth(TargetTransformInfo::RGK_Scalar).getFixedValue();

  std::queue<Value *> Worklist;
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      Value *VInstr = dyn_cast<Value>(&I);
      if (isSolved(VInstr))
        continue;

      if (isa<IntegerType>(I.getType())) {
        createDefaultSol(VInstr);
        visit_widening(VInstr, Worklist);
      } else if (isa<SwitchInst>(I)) {
        createDefaultSol(VInstr);
      }
      // dbgs() << "---------------------------------------------------------"
      //      << '\n';
    }
  }
  dbgs() << "Trying to calculate best Solutions..\n";
  calcBestUserSols(F);

  bool Changed = false;
  dbgs() << "Applying Solutions ...\n";
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (isa<IntegerType>(I.getType()) || isa<SwitchInst>(I)) {
        bool ChangedOnce = applyChain(&I, BestUserSolsPerInst[&I]);
        if (ChangedOnce) {
          Changed = true;
        }
      }
    }
  }
  return Changed;
}
INITIALIZE_PASS_BEGIN(WideningIntegerArithmetic, DEBUG_TYPE, PASS_NAME, false,
                      false)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
INITIALIZE_PASS_END(WideningIntegerArithmetic, DEBUG_TYPE, PASS_NAME, false,
                    false)

inline bool WideningIntegerArithmetic::isInWorklist(Value *V) {
  auto IsVisited = InsideWorklist.find(V);
  if (IsVisited != InsideWorklist.end())
    return true;

  return false;
}

inline bool WideningIntegerArithmetic::isSolved(Value *V) {
  auto IsSolved = SolvedInstructions.find(V);
  if (IsSolved != SolvedInstructions.end())
    return true;

  return false;
}

bool WideningIntegerArithmetic::tryClosure(Instruction *Instr,
                                           std::queue<Value *> &Worklist) {
  auto FillsSize = 0;
  unsigned Opcode = Instr->getOpcode();
  // dbgs() << "Inside try closure. " << "\n";
  if (IsBinop(Opcode)) {
    FillTypeSet Fills = getFillTypes(Instr);
    FillsSize = Fills.size();
  } else if (IsUnop(Opcode)) {
    UnaryFillTypeSet Fills = getUnaryFillTypes(Opcode);
    FillsSize = Fills.size();
  }

  if (FillsSize > 1)
    return closure(Instr, Worklist);

  return /* Changed */ false;
}

bool WideningIntegerArithmetic::closure(Instruction *Instr,
                                        std::queue<Value *> &Worklist) {
  Value *IV = dyn_cast<Value>(Instr);
  int i = 0;
  bool Changed;

  do {
    Changed = false;
    auto FillsList = visitFILL(Instr, Worklist);
    auto WidensList = visitWIDEN(Instr, Worklist);
    // auto GarbageWidenList = visitWIDEN_GARBAGE(Instr, Worklist);
    // dbgs() << "Visit Widen Garbage Done\n";
    auto NarrowList = visitNARROW(Instr, Worklist);
    FillsList.splice(FillsList.end(), WidensList);
    // FillsList.splice(FillsList.end(), GarbageWidenList);
    FillsList.splice(FillsList.end(), NarrowList);
    LLVM_DEBUG(dbgs() << "Iteration " << ++i
                      << "---------------------------------------Adding "
                         "Possible Solutions size is  "
                      << FillsList.size() << '\n'
                      << "Opcode to Str is --> "
                      << OpcodesToStr[Instr->getOpcode()] << '\n');
    for (auto *PossibleSol : FillsList) {

      bool Added = addNonRedudant(AvailableSolutions[IV], PossibleSol);
      if (Added) {
        Changed = true;
        // dbgs() << "Added Sol with updated width "
        //        << PossibleSol->getUpdatedWidth() << " and kind "
        //        << WIAK_NAMES_VEC[PossibleSol->getKind()] << " and cost "
        //        << PossibleSol->getCost() << '\n'
        //        << " and Opc " << PossibleSol->getOpcode() << "and kind"
        //        << WIAK_NAMES_VEC[PossibleSol->getKind()] << "\n";
        // printInstrSols(IV);
        // dbgs() << "Exiting printInstrSols..\n";
      }
    }
    // dbgs() << "Trying again..\n";
    LLVM_DEBUG(dbgs() << "Possible Solution size is --> " << FillsList.size()
                      << '\n');
  } while (Changed == true);
  LLVM_DEBUG(dbgs() << "Returning all the Solutions after non redudant"
                    << '\n');
  // TODO: change is always false here.
  return Changed;
}

inline WideningIntegerArithmetic::FillTypeSet
WideningIntegerArithmetic::getOperandFillTypes(Instruction *Instr) {

  return getFillTypes(Instr);

  // TODO handle the case below.
  // TODO write an overflow analysis.
  /*  switch(Opcode){
      default: break;
      case (ISD::ADD):
      case (ISD::SADDO):{
        Set.insert(std::make_tuple(ANYTHING, ANYTHING, ANYTHING));
        bool IsSigned = (ISD::SADDO == Opcode);


        // TODO check if it overflows based on ISD::ADD
        // and if it does not for sure we can add SIGN AND ZEROS ?
        if(!IsSigned){
          auto N0 = Node->getOperand(0);
          auto N1 = Node->getOperand(1);
          if(DAG.computeOverflowKind(N0, N1 ) == SelectionDAG::OFK_Never){
            Set.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));
          }
        }
        return Set;
      }
    }*/
}

/* Does llvm have an Unary Operator ?
WideningIntegerArithmetic::SolutionSet
WideningIntegerArithmetic::visitUNOP(Instruction *Instr){
  bool Changed = false;
  SDValue N0 = Instr->getOperand(0);
  unsigned Opcode = Instr->getOpcode();
  UnaryFillTypeSet AvailableFillTypes = getUnaryFillTypes(Instr->getOpcode());
  // Available Solutions of child
  SolutionSet Sols = AvailableSolutions[N0.getNode()];
  for(WideningIntegerSolutionInfo *Sol : Sols){
    IntegerFillType FillType = UNDEFINED;
    for(auto PossibleFillType : AvailableFillTypes){
      if(Sol->getFillType() == std::get<0>(PossibleFillType))
        FillType = Sol->getFillType();
    }
    if(FillType == UNDEFINED)
      continue;
    unsigned char w1 = Sol->getUpdatedWidth();
    EVT NewVT = EVT::getIntegerVT(*DAG.getContext(), w1);
    if(!TLI.isOperationLegal(Opcode, NewVT))
      continue;
    unsigned char FillTypeWidth = getScalarSize(N0->getValueType(0));
    auto Unop = new WIA_UNOP(Node->getOpcode(), FillType, FillTypeWidth,
                w1, w1 , Sol->getCost(), Node);
    Unop->addOperand(Sol);
    AvailableSolutions[Node].push_back(Unop);
    Changed = true;
  }
  // call closure if multiple FillTypes or Multiple Unop Widths
  return tryClosure(Node, Changed);
}

*/

/* Add an extension based on the target before Value V */

bool WideningIntegerArithmetic::visitCONSTANT(ConstantInt *CI) {
  SolutionSet Sols;
  unsigned BitWidth = CI->getBitWidth();
  Value *VCI = dyn_cast<Value>(CI);
  std::vector<unsigned int> PossibleWidths = {8, 16, 32, 64};
  bool Changed = false;
  // dbgs() << "Constant Sols are " << "\n";
  // printInstrSols(VCI);
  for (auto Width : PossibleWidths) {
    if (Width >= BitWidth) {
      auto *Sol =
          new WIA_CONSTANT(CONSTANT_INT_OPC, CONSTANT_INT_OPC, ExtensionChoice,
                           BitWidth, Width, Width, 0, VCI);
      // dbgs() << "Generated Sol " << *Sol << "\n";
      /*for(auto AvSol : AvailableSolutions[VCI]){
        dbgs() << "AvSol " << *AvSol << "Is redudant to gen Sol " << *Sol << "
      is " << AvSol->IsRedudant(*Sol) << "\n";

      } */
      Changed = addNonRedudant(AvailableSolutions[VCI], Sol);
      // dbgs() << "Adding Sol " << *Sol << "is " << Changed << "\n";
    }
  }
  return Changed;
}

bool WideningIntegerArithmetic::visitSTORE(Instruction *Instr) {
  SolutionSet Sols;
  Value *VI = dyn_cast<Value>(Instr);
  unsigned char N0Bits = Instr->getType()->getScalarSizeInBits();
  auto N0Sols = AvailableSolutions[Instr->getOperand(0)];
  bool Changed = false;
  for (WideningIntegerSolutionInfo *Sol : N0Sols) {
    auto *StoreSol = new WIA_STORE(Instr->getOpcode(), Instr->getOpcode(),
                                   Sol->getFillType(), N0Bits, N0Bits, N0Bits,
                                   Sol->getCost(), VI);
    if (addNonRedudant(AvailableSolutions[VI], StoreSol)) {
      Changed = true;
    }
  }
  return Changed;
}

bool WideningIntegerArithmetic::visitLOAD(Instruction *Instr) {

  SolutionSet Sols;

  IntegerFillType FillType = IntegerFillType::ANYTHING;
  Value *VInstr = dyn_cast<Value>(Instr);
  unsigned int Width = Instr->getType()->getScalarSizeInBits();
  int FillTypeWidth = getKnownFillTypeWidth(Instr);
  unsigned Opc = Instr->getOpcode();
  auto *WIALoad = new WIA_LOAD(Opc, Opc, FillType, FillTypeWidth, Width,
                               FillTypeWidth, 0, VInstr);
  bool Changed = false;
  if (addNonRedudant(AvailableSolutions[VInstr], WIALoad)) {
    Changed = true;
  }
  unsigned ExtensionOpc = getExtensionChoice(FillType);
  FillType = ExtensionChoice;
  for (int IntegerSize : IntegerSizes) {
    if (IntegerSize <= FillTypeWidth) // TODO check
      continue;
    EVT NewVT = EVT::getIntegerVT(*Ctx, IntegerSize);
    if (!TLI->isOperationLegal(ExtensionOpc, NewVT))
      continue;
    // Results to a widened expr based
    WideningIntegerSolutionInfo *Widen =
        new WIA_WIDEN(Opc, ExtensionOpc, FillType, FillTypeWidth, Width,
                      IntegerSize, 1, VInstr);
    Widen->addOperand(WIALoad);
    if (addNonRedudant(AvailableSolutions[VInstr], Widen)) {
      Changed = true;
    }
  }
  return Changed;
}

inline bool WideningIntegerArithmetic::isLegalToPromote(Instruction *Instr) {
  if (!isa<OverflowingBinaryOperator>(Instr))
    return true;
  if (ExtensionChoice == SIGN)
    return Instr->hasNoSignedWrap();
  if (ExtensionChoice == ZEROS)
    return Instr->hasNoUnsignedWrap();
  return false;
}

// TODO not done yet
inline bool WideningIntegerArithmetic::mayOverflow(Instruction *Instr) {

  unsigned Opcode;
  Opcode = Instr->getOpcode();
  switch (Opcode) {
  case Instruction::Add:
  case Instruction::SDiv:
  case Instruction::UDiv:
  case Instruction::Mul:
  case Instruction::URem:
  case Instruction::SRem:
  default:
    return false;
  }
  return false;
}

bool inline WideningIntegerArithmetic::isLegalAndMatching(
    WideningIntegerSolutionInfo *Sol1, WideningIntegerSolutionInfo *Sol2) {
  return Sol1->getUpdatedWidth() == Sol2->getUpdatedWidth();
}

// TODO: check if the current version of this function calculates the
// best solutions that are possible.
// TODO: modify the algorithm to check the operands of the Users and
// Instruction.
void WideningIntegerArithmetic::calcBestUserSols(Function &F) {
  std::queue<Instruction *> workList;
  // int TotalCost = INT_MAX;
  for (BasicBlock &BB : F) {
    for (Instruction &Instr : BB) {
      workList.push(&Instr);
    }
  }
  while (!workList.empty()) {
    Instruction *Instr = workList.front();
    workList.pop();
    int MinCost = INT_MAX;
    WideningIntegerSolutionInfo *BestInstSol;
    SmallVector<Value *, 4> VUsers;
    DenseMap<Instruction *, SmallVector<WideningIntegerSolutionInfo *>>
        CurrentInstSol;
    std::vector<DenseMap<Value *, WideningIntegerSolutionInfo *>> Combinations;
    DenseMap<Value *, WideningIntegerSolutionInfo *> BestCombination;
    if (Instr->getNumUses() == 0) {
      continue;
      // TODO: Instruction is dead erase it ??
    }
    auto *VInstr = dyn_cast<Value>(Instr);
    for (WideningIntegerSolutionInfo *InstSol : AvailableSolutions[VInstr]) {
      unsigned short InstrUpdatedWidth = InstSol->getUpdatedWidth();
      unsigned short ExtraCost = 0;
      bool DiscardSolution = false;
      for (unsigned i = 0U, e = Instr->getNumOperands(); i < e; ++i) {
        Value *InstOp = Instr->getOperand(i);
        auto InstOpSols = AvailableSolutions[InstOp];
        SmallVector<unsigned short> AvailableOpWidths(InstOpSols.size());
        for (auto *Sol : AvailableSolutions[InstOp]) {
          AvailableOpWidths.push_back(Sol->getUpdatedWidth());
          auto *It = std::find(AvailableOpWidths.begin(),
                               AvailableOpWidths.end(), InstrUpdatedWidth);
          if (It != AvailableOpWidths.end()) {
            ExtraCost += Sol->getCost();
          } else {
            DiscardSolution = true;
          }
        }
      }
      if (DiscardSolution) {
        dbgs() << "Discarding Sol didn't found op width..\n";
        continue;
      }
      InstSol->setCost(InstSol->getCost() + ExtraCost);

      dbgs() << "Checking Instr Width " << InstrUpdatedWidth << "\n";

      DenseMap<Value *, WideningIntegerSolutionInfo *> CandidateUserSols;
      std::queue<Instruction *> userWorklist;
      bool AllMatching = true;
      bool MatchingForAllOps = true;

      for (auto *User : Instr->users()) {
        if (auto *UserI = dyn_cast<Instruction>(User))
          userWorklist.push(UserI);
      }

      while (!userWorklist.empty()) {
        Instruction *User = userWorklist.front();
        userWorklist.pop();
        bool ChangedWidth = false;
        for (WideningIntegerSolutionInfo *UserSol : AvailableSolutions[User]) {
          unsigned short UserUpdatedWidth = UserSol->getUpdatedWidth();
          dbgs() << "Checking User Width " << UserUpdatedWidth << "\n";
          // 2, possible 3 cases
          // 1) same Widths cost of Combination is zero
          // 2) different widths two cases
          // 3) can we promote one type to another? if yes
          // cost + 1 if we add an extension.
          // We may can promote for free.
          // promot)ed type if we cannot no possible combination
          // 4) How to handle the truncate?
          auto *UserI = dyn_cast<Instruction>(User);
          WideningIntegerSolutionInfo *NewUserSol;
          bool UserSolCreated = false;
          if (UserUpdatedWidth == InstrUpdatedWidth) {
            CandidateUserSols[UserI] = UserSol;
            if (UserUpdatedWidth != UserSol->getWidth())
              ChangedWidth = true;
          } else if (isLegalToPromote(UserI) &&
                     UserSol->getUpdatedWidth() < InstSol->getUpdatedWidth()) {
            NewUserSol = UserSol;
            bool UserSolCreated = true;
            NewUserSol->setUpdatedWidth(InstSol->getUpdatedWidth());
            CandidateUserSols[UserI] = NewUserSol;
            if (NewUserSol->getUpdatedWidth() != NewUserSol->getWidth())
              ChangedWidth = true;
          } else {
            AllMatching = false;
            dbgs() << "Didnt found Match for InstrWidth " << InstrUpdatedWidth
                   << "\n";
          }
          if (ChangedWidth) {
            for (unsigned i = 0U, e = UserI->getNumOperands(); i < e; ++i) {
              Value *Op = UserI->getOperand(i);
              auto OpSols = AvailableSolutions[Op];
              SmallVector<unsigned short> AvailableOpWidths(OpSols.size());
              bool FoundMatchingSol = false;
              for (auto *Sol : AvailableSolutions[Op]) {
                AvailableOpWidths.push_back(Sol->getUpdatedWidth());
                Type *OpType = Op->getType();
                if (!UserSolCreated) {
                  auto UpdatedWidth = UserSol->getUpdatedWidth();
                  if (OpType != getTypeFromInteger(UpdatedWidth)) {
                    // we increment the cost because we need to insert a ext or
                    // trunc here.
                    auto *It = std::find(AvailableOpWidths.begin(),
                                         AvailableOpWidths.end(),
                                         UserSol->getUpdatedWidth());
                    if (It == AvailableOpWidths.end())
                      dbgs() << "Didn't find suitable user Sol..\n";
                    else {
                      dbgs() << "Found UserSol!\n";
                      FoundMatchingSol = true;
                      UserSol->incrementCost();
                      CandidateUserSols[UserI] = UserSol;
                    }
                  }
                } else {
                  if (OpType !=
                      getTypeFromInteger(NewUserSol->getUpdatedWidth())) {
                    // we increment the cost because we need to insert a ext or
                    // trunc here.
                    auto *It = std::find(AvailableOpWidths.begin(),
                                         AvailableOpWidths.end(),
                                         UserSol->getUpdatedWidth());
                    if (It == AvailableOpWidths.end())
                      dbgs() << "Didn't find suitable user Sol..\n";
                    else {
                      dbgs() << "Found Matching UserSol!\n";
                      FoundMatchingSol = true;
                      NewUserSol->incrementCost();
                      CandidateUserSols[UserI] = NewUserSol;
                    }
                  }
                }
              }
              if (!FoundMatchingSol) {
                dbgs() << "!!! Didn't found match for all operands of UserI"
                       << *UserI << "\n";
                MatchingForAllOps = false;
              }
            }
          }
        }
        if (ChangedWidth && MatchingForAllOps) {
          // TODO: need to see what to do with the operands. here or elsewhere
          for (auto *UserInstr : User->users()) {
            if (auto *UserofUserInstr = dyn_cast<Instruction>(UserInstr)) {
              userWorklist.push(UserofUserInstr);
            }
          }
        }

      } // end userWorklist
      dbgs() << "All Matching is " << AllMatching << "Ops Matching "
             << AllMatching << "\n";
      if (AllMatching && MatchingForAllOps) {
        dbgs() << "Found a Combination..!\n";
        Combinations.push_back(CandidateUserSols);
        CurrentInstSol[Instr].push_back(InstSol);
      }
    } // End Inst current Sol
    if (Combinations.size() == 0) {
      dbgs() << "!!! Couldn't find user combination for Inst " << *Instr
             << "\n";
      dbgs() << "Inst Solutions are --> " << '\n';
      printInstrSols(Instr);
      dbgs()
          << "-------------------------------------------------------------\n";
      dbgs()
          << "-------------------------------------------------------------\n";
      dbgs() << "Users are " << "\n";
      for (auto *User : Instr->users()) {
        if (auto *UserI = dyn_cast<Instruction>(User)) {
          UserI->dump();
          dbgs() << "Printing User Sols now. -->\n";
          printInstrSols(UserI);
          dbgs() << "----------------------------------------------------------"
                    "-----\n";
        }
      }
    }

    for (auto i = 0U; i < Combinations.size(); i++) {
      auto ValuesSolsMap = Combinations[i];
      int Sum = 0;
      for (const auto &It : ValuesSolsMap) {
        Sum += It.second->getCost();
      }
      if (Sum < MinCost) {
        MinCost = Sum;
        BestCombination = ValuesSolsMap;
        BestInstSol = CurrentInstSol[Instr][i];
      }
    }
    BestUserSolsPerInst[Instr] = BestCombination;
    dbgs() << "For Instr " << *Instr << " best Sol is " << BestInstSol << "\n";
    BestSolPerInst[Instr] = BestInstSol;
  } // end Inst worklist
}

inline short int WideningIntegerArithmetic::getKnownFillTypeWidth(Value *V) {
  KnownBits Known = computeKnownBits(V, *DL);
  if (Known.isUnknown()) {
    // Assume that the data is equal to the instruction width
    // We must guarantee that the Instr does not overflow
    // otherwise this is wrong.
    // The overflow checks is done for all the Operators that
    // can overflow such as add, sub.
    return V->getType()->getScalarSizeInBits();
  }
  return Known.getBitWidth();
}

long long int counter = 0;

SmallVector<WideningIntegerSolutionInfo *>
WideningIntegerArithmetic::visit_widening(Value *VInstr,
                                          std::queue<Value *> &Worklist) {
  SolutionSet EmptySols;
  Instruction *Instr = dyn_cast<Instruction>(VInstr);
  if (isSolved(VInstr)) {
    return AvailableSolutions[VInstr];
  }
  if (!Instr) {
    return EmptySols;
  }
  // dbgs() << "Pushing to worklist " << *Instr << "\n";
  Worklist.push(VInstr);
  InsideWorklist[VInstr] = true;
  for (Value *V : Instr->operand_values()) {
    if (isSolved(V)) {
    } else if (!isInWorklist(V)) {
      visit_widening(V, Worklist);
    }
  }
  if (!Instr) {
    return EmptySols;
  }
  // is ConstantInt visited here?

  while (!Worklist.empty()) {
    Value *PopVal = Worklist.front();
    Worklist.pop();

    // if (auto *I = dyn_cast<Instruction>(PopVal))
    //   // dbgs() << " Opc is " << OpcodesToStr[I->getOpcode()] << "\n";
    //   else {
    //     // dbgs() << "Not an instruction to print opc..\n";
    //   }
    bool Changed = visitInstruction(PopVal, Worklist);
    Value *SuccessorPhi = getPhiSuccessor(PopVal);
    if ((SuccessorPhi != nullptr && SuccessorPhi != PopVal &&
         !isSolved(SuccessorPhi)) ||
        Changed) {
      // dbgs() << "Adding PopVal " << *PopVal << " back to worklist.." << "\n";
      // dbgs() << "Succesor Phi Sols Size is "
      //        << AvailableSolutions[SuccessorPhi].size() << "\n";
      Worklist.push(PopVal);
      // dbgs() << "Worklist size is " << Worklist.size() << "\n";
      //  visitPHI(SuccessorPhi); // TODO if PHI is not solved solve it here?
    } else {
      // dbgs() << "Solved Instruction " << *PopVal << "\n";
      SolvedInstructions[PopVal] = true;
      counter++;
      // dbgs() << "Solutions Size is --> " << AvailableSolutions[PopVal].size()
      //<< '\n';
      assert(AvailableSolutions[PopVal].size() > 0 &&
             "Empty Solutions on visit_widening");
      // printInstrSols(PopVal);
    }
  }
  return AvailableSolutions[VInstr];
}

Value *WideningIntegerArithmetic::getPhiSuccessor(Value *V) {
  if (!isa<Instruction>(V)) {
    return nullptr;
  }
  auto *Instr = dyn_cast<Instruction>(V);
  if (isa<PHINode>(Instr)) {
    return V;
  }
  for (Value *VI : Instr->operand_values()) {
    if (isa<PHINode>(VI)) {
      return VI;
    }
    if (auto *I = dyn_cast<Instruction>(VI)) {
      if (isa<BinaryOperator>(I) || isa<ICmpInst>(I) || isa<TruncInst>(I) ||
          isa<ZExtInst>(I) || isa<SExtInst>(I)) {
        return getPhiSuccessor(VI);
      } else {
        return nullptr;
      }
    }
  }
  return nullptr;
}

bool WideningIntegerArithmetic::solveSimplePhis(
    Instruction *Instr, SmallVector<Value *, 32> IncomingValues,
    std::queue<Value *> &Worklist) {

  SolutionSet Solutions;
  auto *VInstr = dyn_cast<Value>(Instr);
  auto *PhiInst = dyn_cast<PHINode>(Instr);
  unsigned int InstrWidth = PhiInst->getType()->getScalarSizeInBits();
  if (IncomingValues.size() <= 0 || isSolved(Instr)) {
    return false;
  }
  for (auto *Inc : IncomingValues) {
    if (isSolved(Inc) || isInWorklist(Inc))
      continue;
    Worklist.push(Inc);
  }
  bool Changed = false;
  Value *SelectedValue = IncomingValues[0];
  SmallVector<Value *, 32> ValuesWithout = IncomingValues;
  ValuesWithout.erase(ValuesWithout.begin());
  auto *Vphi = dyn_cast<Value>(PhiInst);
  for (WideningIntegerSolutionInfo *Sol : AvailableSolutions[SelectedValue]) {
    for (Value *Val2 : ValuesWithout) {
      for (WideningIntegerSolutionInfo *Sol2 : AvailableSolutions[Val2]) {
        if (isLegalAndMatching(Sol, Sol2)) {
          IntegerFillType NewFillType =
              Sol->getFillType() == Sol2->getFillType() ? Sol->getFillType()
                                                        : ANYTHING;
          unsigned NewFillTypeWidth =
              Sol->getFillTypeWidth() >= Sol2->getFillTypeWidth()
                  ? Sol->getFillTypeWidth()
                  : Sol2->getFillTypeWidth();
          unsigned NewCost = Sol->getCost() > Sol2->getCost() ? Sol->getCost()
                                                              : Sol2->getCost();
          auto *PhiSol = new WIA_PHI(PhiInst->getOpcode(), PhiInst->getOpcode(),
                                     NewFillType, NewFillTypeWidth, InstrWidth,
                                     Sol->getUpdatedWidth(), NewCost, Vphi);

          if (addNonRedudant(AvailableSolutions[VInstr], PhiSol)) {
            Changed = true;
            // dbgs() << "Inside phiSimple Added new Sol PHI " << *PhiSol <<
            // "\n";
          }
        }
      }
    }
  }
  // We didn't find any new Solutions we are done.
  if (!Changed) {
    SolvedInstructions[VInstr] = true;
  }
  return Changed;
}

bool WideningIntegerArithmetic::createDefaultSol(Value *VI) {

  enum WIAKind Kind;
  if (VI == nullptr) {
    // dbgs() << "Cannot create Solution for a Null Value." << "\n";
    return false;
  }
  if (auto *CI = dyn_cast<ConstantInt>(VI)) {
    return visitCONSTANT(CI);
  }
  unsigned Opcode;
  if (auto *Instr = dyn_cast<Instruction>(VI)) {
    Opcode = Instr->getOpcode();
    if (IsBinop(Opcode)) {
      Kind = WIAK_BINOP;
    } else if (IsUnop(Opcode)) {
      Kind = WIAK_UNOP;
    } else if (IsLoad(Opcode)) {
      Kind = WIAK_LOAD;
    } else if (IsStore(Opcode)) {
      Kind = WIAK_STORE;
    } else if (IsExtension(Opcode)) {
      Kind = WIAK_DROP_EXT;
      return false; // TODO refactor
    } else if (IsTruncate(Opcode)) {
      return false;
      Kind = WIAK_DROP_LOCOPY;
      // TODO handle DROPLOIGNORE
    } else if (IsPHI(Opcode)) {
      Kind = WIAK_PHI;
    }
  } else {
    Kind = WIAK_UNKNOWN;
  }
  unsigned short FillTypeWidth, InstrWidth;
  if (auto *SI = dyn_cast<SwitchInst>(VI)) {
    FillTypeWidth = getKnownFillTypeWidth(SI->getCondition());
    InstrWidth = SI->getType()->getScalarSizeInBits();
  } else {
    FillTypeWidth = getKnownFillTypeWidth(VI);
    InstrWidth = VI->getType()->getScalarSizeInBits();
  }
  dbgs() << "Creating Default Sol for Opc " << OpcodesToStr[Opcode] << "\n";
  if (Kind == WIAK_UNKNOWN) {
    Opcode = UNKNOWN_OPC;
  }
  auto *DefaultSol = new WideningIntegerSolutionInfo(
      Opcode, Opcode, ExtensionChoice, FillTypeWidth, InstrWidth, InstrWidth, 0,
      Kind, VI);
  // createDefaultSol is called only when we have emptySols so we don't need
  // to call addNonRedundant.
  dbgs() << "Created Default Sol ..\n";
  assert(AvailableSolutions[VI].size() == 0 && "Sols are not empty..\n");
  addNonRedudant(AvailableSolutions[VI], DefaultSol);
  assert(AvailableSolutions[VI].size() != 0 && "Default Sol not added..\n");
  return true;
}

bool WideningIntegerArithmetic::visitPHI(Instruction *Instr,
                                         std::queue<Value *> &Worklist) {
  // we know that Instr is PHINode from isPHI method so we can cast it.
  auto *PhiInst = dyn_cast<PHINode>(Instr);
  int NumIncValues = PhiInst->getNumIncomingValues();
  SolutionSet Solutions;
  SmallVector<Value *, 32> IncomingValues;
  // dbgs() << "Inside VISIT_PHI : Number of incoming values --> " <<
  // NumIncValues
  //      << "\n";
  bool UpdatedWorklist = false, Cycle = false;
  for (int i = 0; i < NumIncValues; ++i) {
    Value *V = PhiInst->getIncomingValue(i);
    if (AvailableSolutions[V].size() == 0) {
      // dbgs() << "visitPHI: Calling create default sol,  ";
      if (auto *I = dyn_cast<Instruction>(V)) {
        // dbgs() << "Inst is is --> " << *I << "\n";
      }
      createDefaultSol(V);

      Worklist.push(V);
      UpdatedWorklist = true;
    }
    // If we have a possible cycle push it to the Worklist and continue,
    // because the solutions of V might not be completed
    Value *PhiSucc = getPhiSuccessor(V);
    if (PhiSucc != nullptr) {
      auto *PhiSuccCasted = dyn_cast<PHINode>(PhiSucc);
      if (PhiSuccCasted == PhiInst) {
        // dbgs() << "visitPHI : Found PossibleCycle..\n";
        Cycle = true;
      }
    }
    IncomingValues.push_back(V);
  }
  bool Changed = solveSimplePhis(Instr, IncomingValues, Worklist);
  // dbgs() << "Changed is " << Changed << "\n";
  if ((Changed && Cycle) || (Changed && UpdatedWorklist))
    return true;
  return false;
}

// based on opcode operand fillType for any binop
// can be either g, s, z
// g means GARBAGE , g means anything, we cannot trust the extended bits
// s means  that the fill type is signed bits
// z means that the fill type is zero bits
// τ means means that fill type can be anything s, z, or g
// . left operand
// . {2 solutions: σ[32]:64--cost:4 , g[32]:64--cost:1}
// σ[32]:64 means . sign extend (no garbage) upper unused bits zeros or one.
// g[32]:64 means you cannot trust the high 32 bits
// g[16]:64 means you cannot trust the high 48 bits
// . right operand
// . {1 solutions: σ[32]:i64 cost :3 }
// . check legal values for example XOR hass these legal types
// { s -> s x s }, {g-> g x g}, { z -> z x z }
// We can  choose
// Solution 1 s[32]:64 cost 4 + 3, Solution 2 g[32]:64 cost 3 needs sext or
// zext depends on the fill type of target so total 3 + 1 } Instruction can
// decide what solution is appropriate for example print requires sext to
// use Solution 2 because upper  // upper bits {i32, i32, i32 } can we
// Truncate?

// check IsRedudant uses fillTypeWidth
// Return all the solution based on binop rules op1 x op2 -> W
bool WideningIntegerArithmetic::combineBINOPSols(
    Instruction *Binop, std::queue<Value *> &Worklist) {

  Value *VBinop = dyn_cast<Value>(Binop);
  unsigned Opcode = Binop->getOpcode();
  FillTypeSet OperandFillTypes = getOperandFillTypes(Binop);
  bool Changed = false;
  // A function that combines solutions from operands left and right
  for (WideningIntegerSolutionInfo *leftSolution :
       AvailableSolutions[Binop->getOperand(0)]) {
    for (WideningIntegerSolutionInfo *rightSolution :
         AvailableSolutions[Binop->getOperand(1)]) {
      // Test whether current binary operator has the available
      // fill type provided by leftSolution and rightSolution
      auto LeftFill = leftSolution->getFillType();
      auto RightFill = rightSolution->getFillType();
      // dbgs() << "Left Fill type is " << LeftFill << "\n";
      // dbgs() << "right Fill type is " << RightFill << "\n";
      auto FillType = getOrNullFillType(Binop, OperandFillTypes,
                                        leftSolution->getFillType(),
                                        rightSolution->getFillType());
      if (FillType == UNDEFINED) {
        // dbgs() << "Fill type is undefined skipping solution" << "\n";
        //  dbgs() << "Left Solution --> " << *leftSolution << '\n';
        //  dbgs() << "Right Solution --> " << *rightSolution << '\n';
        continue;
      }
      // dbgs() << "Passed FillType for combination --> " << FillType << "\n";
      unsigned int w1 = leftSolution->getUpdatedWidth();
      unsigned int w2 = rightSolution->getUpdatedWidth();
      if (w1 != w2) {
        LLVM_DEBUG(dbgs() << "Width " << w1 << "and Width " << w2
                          << " are not the same skipping solution.." << "\n");
        // dbgs() << "Width " << w1 << "and Width " << w2
        //      << " are not the same skipping solution.." << "\n";
        // dbgs() << "Left Solution --> " << *leftSolution << '\n';
        // dbgs() << "Right Solution --> " << *rightSolution << '\n';
        continue;
      }
      if (w1 == 0 || w2 == 0) {
        // dbgs() << "Left Solution --> " << *leftSolution << '\n';
        // dbgs() << "Right Solution --> " << *rightSolution << '\n';
        // dbgs() << "Error here be aware. ------ " << '\n';
        continue;
        // dbgs() << "EVT get String " << LD->getMemoryVT().getEVTString()
        // <<
        // '\n';
      }
      // dbgs() << "The widths are the same and we continue--> " << w1 << "\n";
      EVT NewVT = EVT::getIntegerVT(*Ctx, w1);
      if (!TLI->isOperationLegal(TLI->InstructionOpcodeToISD(Opcode), NewVT)) {
        continue;
      }
      // dbgs() << "The Operation is legal for that newVT--> " << w1 << "\n";
      //  w1 x w2 --> w all widths are the same at this point
      // and there is a LegalOperation for that Opcode
      unsigned int UpdatedWidth = w1;

      unsigned int Cost = leftSolution->getCost() + rightSolution->getCost();
      unsigned int FillTypeWidth = getKnownFillTypeWidth(Binop);
      // this Target of this Binary operator of the form
      // width1 x width2 = newWidth
      // for example on rv64 we have addw
      // that is of the form i32 x i32 -> i32
      // and stored in a sign extended format to i64

      auto *Sol = new WIA_BINOP(Opcode, Opcode, FillType, FillTypeWidth, w1,
                                UpdatedWidth, Cost, VBinop);
      Sol->addOperand(leftSolution);
      Sol->addOperand(rightSolution);
      LLVM_DEBUG(dbgs() << "Adding Sol with cost " << Cost << " and width "
                        << UpdatedWidth << "\n");
      // dbgs() << "BINOP Adding Sol with cost " << Cost << " and width "
      //      << UpdatedWidth << "\n";

      if (addNonRedudant(AvailableSolutions[VBinop], Sol))
        Changed = true;
      else {
        // dbgs() << "Called add non redudant but solution wasn't added.\n";
      }
    }
  }
  assert(AvailableSolutions[Binop].size() > 0);
  return closure(Binop, Worklist);
}

bool WideningIntegerArithmetic::visitBINOP(Instruction *Binop,
                                           std::queue<Value *> &Worklist) {

  Value *V0 = Binop->getOperand(0);
  Value *V1 = Binop->getOperand(1);
  // shift left x0, x1, x2
  // arithmetic shift right x3, x0, x2
  //
  if (Binop->getOpcode() == Instruction::Shl) {
    if (auto *I = dyn_cast<Instruction>(V0)) {
      if (auto *CI = dyn_cast<ConstantInt>(V1)) {
        unsigned LeftShiftC = CI->getZExtValue();
        unsigned TotalBits = I->getType()->getScalarSizeInBits();
        if (I->getOpcode() == Instruction::AShr) {
          auto *ShlV1 = dyn_cast<Instruction>(I);
          if (auto *C2 = dyn_cast<ConstantInt>(ShlV1)) {
            unsigned ARightShiftC = C2->getZExtValue();
            if (ARightShiftC == LeftShiftC) {
              // call closure here to get all the LowWidths that are available.
              assert(AvailableSolutions[I].size() > 0);
              SmallVector<unsigned short> AvailableWidths(
                  AvailableSolutions[I].size());
              for (auto *Sol : AvailableSolutions[I]) {
                if (TotalBits - Sol->getUpdatedWidth() > 0) {
                  AvailableWidths.push_back(TotalBits - Sol->getUpdatedWidth());
                }
              }
              // AvailableWidths.resize()
              return visitEXTLO(Binop, AvailableWidths, Worklist);
            }
          }
        }
      }
    }
  }
  auto LeftSols = AvailableSolutions[V0];
  auto RightSols = AvailableSolutions[V1];
  // TODO how to handle carry and borrow?
  if (LeftSols.size() <= 0) {
    Worklist.push(V0);
    bool AddedLeft = createDefaultSol(V0);
    // dbgs() << "BINOP Warning!!!! Found empty left Sols on Binop and added "
    //         "default is "
    //<< AddedLeft << "\n";
  }
  if (RightSols.size() <= 0) {
    Worklist.push(V1);
    bool AddedRight = createDefaultSol(V1);
    // dbgs() << "BINOP Warning!!!! Found empty right Sols on Binop and added "
    //          "default is "
    //      << AddedRight << "\n";
  }
  // dbgs() << "Calling Combine Binops..\n";
  return combineBINOPSols(Binop, Worklist);
}

std::list<WideningIntegerSolutionInfo *>
WideningIntegerArithmetic::visitFILL(Instruction *Instr,
                                     std::queue<Value *> &Worklist) {

  unsigned char FillTypeWidth = getKnownFillTypeWidth(Instr);
  Value *VInstr = dyn_cast<Value>(Instr);

  SolutionSet Sols = AvailableSolutions[VInstr];
  if (Sols.size() <= 0) {
    Worklist.push(VInstr);
    createDefaultSol(VInstr);
  }
  std::list<WideningIntegerSolutionInfo *> Solutions;
  for (WideningIntegerSolutionInfo *Sol : Sols) {
    if (Sol->getOpcode() == Instruction::SExt ||
        llvm::ISD::isExtOpcode(Sol->getOpcode()) ||
        Sol->getOpcode() == Instruction::Trunc)
      continue; // FILL is a ISD::SIGN_EXTEND_INREG
    // WIA_FILL extends the least significant *Width* bits of Instr
    // to targetWidth
    for (auto IntegerSize : IntegerSizes) {
      EVT NewVT = EVT::getIntegerVT(*Ctx, IntegerSize);
      // TODO add isTypeLegal for the NewVT??
      // meaning is TLi.isOperationLegal enough for the NewVT?
      // FIXME: check correctness.
      if (IntegerSize < FillTypeWidth || IntegerSize < (int)Sol->getWidth() ||
          TLI->InstructionOpcodeToISD(Sol->getOpcode()) == ISD::TRUNCATE ||
          !TLI->isOperationLegal(ISD::SIGN_EXTEND_INREG, NewVT)) {
        continue;
      }
      WideningIntegerSolutionInfo *Fill = new WIA_FILL(
          FILL_INST_OPC, FILL_INST_OPC, ExtensionChoice, FillTypeWidth,
          FillTypeWidth, IntegerSize, Sol->getCost() + 1, VInstr);
      Fill->addOperand(Sol);
      Solutions.push_front(Fill);
    }
  }
  return Solutions;
}

inline Type *WideningIntegerArithmetic::getTypeFromInteger(int Integer) {
  Type *Ty;
  switch (Integer) {
  case 8:
    Ty = Type::getInt8Ty(*Ctx);
    break;
  case 16:
    Ty = Type::getInt16Ty(*Ctx);
    break;
  case 32:
    Ty = Type::getInt32Ty(*Ctx);
    break;
  case 64:
    Ty = Type::getInt64Ty(*Ctx);
    break;
  }
  return Ty;
}

inline unsigned
WideningIntegerArithmetic::getExtCost(Instruction *Instr,
                                      WideningIntegerSolutionInfo *Sol,
                                      unsigned short IntegerSize) {
  unsigned Cost = Sol->getCost();
  Type *Ty1 = Instr->getType();
  ;
  Type *Ty2 = getTypeFromInteger((int)IntegerSize);
  if (!TLI->isZExtFree(Ty1, Ty2) || !TLI->isSExtFree(Ty1, Ty2)) {
    ++Cost;
  }
  return Cost;
}

std::list<WideningIntegerSolutionInfo *>
WideningIntegerArithmetic::visitWIDEN(Instruction *Instr,
                                      std::queue<Value *> &Worklist) {

  // TODO how to check for Type S? depends on target ? or on this operand
  // if(!hasTypeS(Sol->getFillType())
  //  return NULL;

  unsigned Width = Instr->getType()->getScalarSizeInBits();
  unsigned ExtensionOpc = getExtensionChoice(ExtensionChoice);
  unsigned char FillTypeWidth = getKnownFillTypeWidth(Instr);
  Value *VInstr = dyn_cast<Value>(Instr);
  SolutionSet Sols = AvailableSolutions[VInstr];
  if (Sols.size() <= 0) {
    Worklist.push(VInstr);
    createDefaultSol(VInstr);
  }
  LLVM_DEBUG(dbgs() << "SolutionsSize is --> " << Sols.size() << '\n');
  std::list<WideningIntegerSolutionInfo *> Solutions;
  if (!isLegalToPromote(Instr)) {
    return Solutions;
  }
  for (WideningIntegerSolutionInfo *Sol : Sols) {
    LLVM_DEBUG(dbgs() << "Inside visitWiden Iterating Sol --> " << Sol << '\n');
    unsigned SolOpc = Sol->getOpcode();
    if (SolOpc == FILL_INST_OPC)
      continue;

    unsigned IsdOpc = TLI->InstructionOpcodeToISD(SolOpc);
    if (llvm::ISD::isExtOpcode(Sol->getOpcode()) ||
        IsdOpc == ISD::SIGN_EXTEND_INREG || IsdOpc == ISD::TRUNCATE)
      continue;
    for (int IntegerSize : IntegerSizes) {
      EVT NewVT = EVT::getIntegerVT(*Ctx, IntegerSize);
      if (IntegerSize < FillTypeWidth || IntegerSize < Sol->getWidth() ||
          !TLI->isOperationLegal(ExtensionOpc, NewVT)) {
        continue;
      }
      unsigned cost = getExtCost(Instr, Sol, IntegerSize);
      // Results to a widened expr based on ExtensionOpc
      WideningIntegerSolutionInfo *Widen =
          new WIA_WIDEN(ExtensionOpc, ExtensionOpc, ExtensionChoice,
                        FillTypeWidth, Width, IntegerSize, cost, VInstr);
      Widen->addOperand(Sol);
      Solutions.push_front(Widen);
    }
  }
  return Solutions;
}

std::list<WideningIntegerSolutionInfo *>
WideningIntegerArithmetic::visitWIDEN_GARBAGE(Instruction *Instr,
                                              std::queue<Value *> &Worklist) {

  unsigned ExtensionOpc = ISD::ANY_EXTEND; // Results to a garbage widened
  unsigned char FillTypeWidth = getKnownFillTypeWidth(Instr);
  Value *VInstr = dyn_cast<Value>(Instr);
  auto Sols = AvailableSolutions[VInstr];
  if (Sols.size() <= 0) {
    Worklist.push(VInstr);
    createDefaultSol(VInstr);
  }
  std::list<WideningIntegerSolutionInfo *> Solutions;
  if (!isLegalToPromote(Instr)) {
    return Solutions;
  }
  for (WideningIntegerSolutionInfo *Sol : Sols) {
    unsigned SolOpc = Sol->getOpcode();
    if (SolOpc == FILL_INST_OPC)
      continue;
    unsigned IsdOpc = TLI->InstructionOpcodeToISD(SolOpc);
    if (llvm::ISD::isExtOpcode(IsdOpc) ||
        // TODO how to check for this? Is it needed also ? Sol->getOpcode()
        // == ISD::SIGN_EXTEND_INREG ) ||
        IsdOpc == Instruction::Trunc)
      continue;
    for (int IntegerSize : IntegerSizes) {
      EVT NewVT = EVT::getIntegerVT(*Ctx, IntegerSize);
      if (IntegerSize < FillTypeWidth || IntegerSize < Sol->getWidth() ||
          !TLI->isOperationLegal(ISD::ANY_EXTEND, NewVT)) {
        continue;
      }
      unsigned cost = getExtCost(Instr, Sol, IntegerSize);

      WideningIntegerSolutionInfo *GarbageWiden =
          new WIA_WIDEN(ExtensionOpc, ExtensionOpc, ANYTHING, FillTypeWidth,
                        Sol->getWidth(), IntegerSize, cost, VInstr);
      GarbageWiden->addOperand(Sol);
      Solutions.push_front(GarbageWiden);
    }
  }
  return Solutions;
}

std::list<WideningIntegerSolutionInfo *>
WideningIntegerArithmetic::visitNARROW(Instruction *Instr,
                                       std::queue<Value *> &Worklist) {

  unsigned Width = Instr->getType()->getScalarSizeInBits();
  unsigned char FillTypeWidth = getKnownFillTypeWidth(Instr);
  Value *VInstr = dyn_cast<Value>(Instr);
  // TODO check isTruncateFree on some targets and widths.
  // Not sure the kinds of truncate of the Target will determine it.
  unsigned ExtensionOpc = getExtensionChoice(ExtensionChoice);
  std::vector<short int> TruncSizes = {8, 16, 32};

  auto Sols = AvailableSolutions[VInstr];
  if (Sols.size() <= 0) {
    Worklist.push(VInstr);
    createDefaultSol(VInstr);
  }
  std::list<WideningIntegerSolutionInfo *> Solutions;
  for (auto *Sol : Sols) {
    unsigned SolOpc = Sol->getOpcode();
    if (SolOpc == FILL_INST_OPC)
      continue;
    unsigned IsdOpc = TLI->InstructionOpcodeToISD(Sol->getOpcode());
    if (llvm::ISD::isExtOpcode(Sol->getOpcode()) ||
        IsdOpc == ISD::SIGN_EXTEND_INREG || IsdOpc == ISD::TRUNCATE)
      continue;
    for (int IntegerSize : TruncSizes) {
      EVT NewVT = EVT::getIntegerVT(*Ctx, IntegerSize);
      if (!TLI->isOperationLegal(ISD::TRUNCATE, NewVT) ||
          IntegerSize < FillTypeWidth || IntegerSize >= Sol->getWidth()) {
        continue;
      }

      unsigned Cost = Sol->getCost();
      Type *Ty1 = Instr->getType();
      Type *Ty2 = getTypeFromInteger(IntegerSize);
      if (!TLI->isTruncateFree(Ty1, Ty2))
        Cost = Cost + 1;
      WideningIntegerSolutionInfo *Trunc = new WIA_NARROW(
          ExtensionOpc, ExtensionOpc,
          ExtensionChoice, // Will depend on available Narrowing ,
          FillTypeWidth, Width, IntegerSize, Sol->getCost() + 1, VInstr);
      Trunc->addOperand(Sol);
      Solutions.push_front(Trunc);
    }
  }
  return Solutions;
}

bool WideningIntegerArithmetic::visitDROP_EXT(Instruction *Instr,
                                              std::queue<Value *> &Worklist) {
  Value *N0 = Instr->getOperand(0);
  // TODO check can an operand of constantInt be extended?
  // does it need to be extended? or we can be sure that N0 is never a
  // ConstantInt?
  if (auto *Binop = dyn_cast<OverflowingBinaryOperator>(N0)) {
    if (Instr->getOpcode() == Instruction::SExt) {
      if (!Binop->hasNoSignedWrap()) {
        return false;
      }
    } else if (Instr->getOpcode() == Instruction::ZExt) {
      if (!Binop->hasNoUnsignedWrap()) {
        return false;
      }
    }
  }
  unsigned int ExtendedWidth = Instr->getType()->getScalarSizeInBits();
  unsigned int OldWidth = N0->getType()->getScalarSizeInBits();
  // TODO: We need to check that the N0 fillTypeWidth is less than
  // the width after the extension is dropped. If it isn't probably
  // we cannot drop the extension??
  // dbgs() << "NewWidth of Node is " << OldWidth << '\n';
  // dbgs() << "N0 name is " << N0->getName() << "\n";
  int Opc;
  if (auto *Iop = dyn_cast<Instruction>(N0)) {
    Opc = TLI->InstructionOpcodeToISD(Iop->getOpcode());
  } else if (isa<ConstantInt>(N0)) {
    Opc = -1; //  Indicates a ConstantIn
  } else {
    Opc = -2; // Indicates unknown opc
  }
  SolutionSet Sols;
  switch (Instr->getOpcode()) {
  case Instruction::SExt:
    ++NumSExtsDropped;
    break;
  case Instruction::ZExt:
    ++NumZExtsDropped;
    break;
  default:
    return false;
  }
  if (Opc >= 0) {
    LLVM_DEBUG(dbgs() << "Opc of Instr->Op0 is " << Opc << " Opc string is "
                      << OpcodesToStr[Opc] << '\n');
  }
  // LLVM_DEBUG(dbgs() << "Opc of Instr is " << Instr->getOpcode()
  //                   << "Opc of Instr str is "
  //                   << OpcodesToStr[Instr->getOpcode()] << '\n');
  // LLVM_DEBUG(dbgs() << "ExtendedWidth of Node is " << ExtendedWidth << '\n');
  // LLVM_DEBUG(dbgs() << "Opc of Node is " << Instr->getOpcode()
  //                   << "Opc of Node str is " <<
  //                   OpcodesToStr[Instr->getOpcode()]
  //                  << '\n');
  if (AvailableSolutions[N0].size() <= 0) {
    createDefaultSol(N0);
    Worklist.push(N0);
    dbgs() << "Warning!!!! Drop EXt tCreating default sol .." << "\n";
  }
  bool Changed = false;
  Value *VInstr = dyn_cast<Value>(Instr);
  SolutionSet ExprSolutions = AvailableSolutions[N0];
  for (auto *Solution : ExprSolutions) {
    if (Solution->getKind() == WIAK_DROP_EXT) {
      continue;
    }
    // We simply drop the extension and we will later see if it's needed.
    LLVM_DEBUG(dbgs() << "Drop extension in Solutions" << '\n');
    unsigned char FillTypeWidth = Solution->getFillTypeWidth();
    if (FillTypeWidth > OldWidth) {
      // TODO: check this. should be >= or > ??
      continue;
    }
    WideningIntegerSolutionInfo *Expr = new WIA_DROP_EXT(
        Instr->getOpcode(), Solution->getNewOpcode(), ExtensionChoice,
        FillTypeWidth, ExtendedWidth /*OldWidth*/, OldWidth /*NewWidth*/,
        Solution->getCost(), Instr);
    // TODO check OldWidth must come from The operand of the Extension
    // or from The Solutions? PRobably thes solutions.
    Expr->setOperands(Solution->getOperands());
    // dbgs() << "Trying to add Sol " << *Expr << "\n";
    // dbgs() << "Size is " << AvailableSolutions[VInstr].size() << "\n";
    // dbgs() << "Solutions before \n";
    // printInstrSols(VInstr);
    int Added = addNonRedudant(AvailableSolutions[VInstr], Expr);
    // dbgs() << "Solutions after \n";
    // printInstrSols(VInstr);
    // dbgs() << "Updated Size is " << AvailableSolutions[VInstr].size() <<
    // "\n"; dbgs() << "Ret add non Red is " << Added << "\n";
    if (Added) {
      Changed = true;
    }
  }
  return Changed;
}

void WideningIntegerArithmetic::replaceAllUsersOfWith(Value *From, Value *To) {
  SmallVector<Instruction *, 4> Users;
  Instruction *InstTo = dyn_cast<Instruction>(To);
  bool ReplacedAll = true;

  LLVM_DEBUG(dbgs() << "IR Promotion: Replacing " << *From << " with " << *To
                    << "\n");

  for (Use &U : From->uses()) {
    auto *User = cast<Instruction>(U.getUser());
    if (InstTo && User->isIdenticalTo(InstTo)) {
      ReplacedAll = false;
      continue;
    }
    Users.push_back(User);
  }

  for (auto *U : Users)
    U->replaceUsesOfWith(From, To);

  if (ReplacedAll)
    if (auto *I = dyn_cast<Instruction>(From))
      InstsToRemove.insert(I);
}

bool WideningIntegerArithmetic::applySingleSol(
    Instruction *Instr, WideningIntegerSolutionInfo *Sol) {

  IRBuilder<> Builder{*Ctx};

  auto InsertExt = [&](Value *V, Instruction *InsertPt, Type *ExtTy) {
    Builder.SetInsertPoint(InsertPt);
    if (auto *I = dyn_cast<Instruction>(V))
      Builder.SetCurrentDebugLocation(I->getDebugLoc());
    if (ExtensionChoice == ZEROS) {
      LLVM_DEBUG(dbgs() << "WideningIntegerArithmetic: Inserting SExt for "
                        << *V << "\n");
      Value *ZExt = Builder.CreateZExt(V, ExtTy);
      if (auto *I = dyn_cast<Instruction>(ZExt)) {
        if (isa<Argument>(V))
          I->moveBefore(InsertPt);
        else
          I->moveAfter(InsertPt);
        // NewInsts.insert(I);
      }
      replaceAllUsersOfWith(V, ZExt);
    } else { // ExtensionChoice == SIGN
      LLVM_DEBUG(dbgs() << "WideningIntegerArithmetic: Inserting SExt for "
                        << *V << "\n");
      Value *SExt = Builder.CreateZExt(V, ExtTy);
      if (auto *I = dyn_cast<Instruction>(SExt)) {
        if (isa<Argument>(V))
          I->moveBefore(InsertPt);
        else
          I->moveAfter(InsertPt);
        // NewInsts.insert(I);
      }
      replaceAllUsersOfWith(V, SExt);
    }
  };

  auto InsertTrunc = [&](Value *V, Type *TruncTy) -> Instruction * {
    if (!isa<Instruction>(V) || !isa<IntegerType>(V->getType()))
      return nullptr;

    if (Promoted.count(V))
      return nullptr;

    LLVM_DEBUG(dbgs() << "Widening Integer Arithmetic : Creating " << *TruncTy
                      << " Trunc for " << *V << "\n");
    Builder.SetInsertPoint(cast<Instruction>(V));
    auto *Trunc = dyn_cast<Instruction>(Builder.CreateTrunc(V, TruncTy));
    // if (Trunc)
    //   NewInsts.insert(Trunc);
    return Trunc;
  };

  auto InsertSExtInReg = [&](Value *V, Instruction *InsertPt, Type *B,
                             Type *ExtTy) {
    LLVM_DEBUG(dbgs() << "WideningIntegerArithmetic: Inserting SExt for " << *V
                      << "\n");
    Builder.SetInsertPoint(InsertPt);
    if (auto *I = dyn_cast<Instruction>(V))
      Builder.SetCurrentDebugLocation(I->getDebugLoc());
    Value *Truncated = InsertTrunc(V, B);
    Value *SExt = Builder.CreateSExt(Truncated, ExtTy);
    if (auto *I = dyn_cast<Instruction>(SExt)) {
      if (isa<Argument>(V))
        I->moveBefore(InsertPt);
      else
        I->moveAfter(InsertPt);
      // NewInsts.insert(I);
    }

    replaceAllUsersOfWith(V, SExt);
  };

  Value *VInstr = dyn_cast<Value>(Instr);
  WIAKind SolKind = Sol->getKind();
  Type *NewTy = getTypeFromInteger(Sol->getUpdatedWidth());
  switch (SolKind) {
  default:
    VInstr->mutateType(getTypeFromInteger(Sol->getUpdatedWidth()));
    break;
  case WIAK_WIDEN:
  case WIAK_WIDEN_GARBAGE: {
    Instruction *InsertPt = Instr;
    if (auto *Arg = dyn_cast<Argument>(Instr)) {
      BasicBlock &BB = Arg->getParent()->front();
      InsertPt = &*BB.getFirstInsertionPt();
    }
    InsertExt(Instr, InsertPt, NewTy);
    Promoted.insert(Instr);
    break;
  }
  case WIAK_NARROW: {
    // TODO: needs fixes not complete, switch is diff stores are diff
    // and maybe more.
    Value *Trunc = InsertTrunc(Instr, NewTy);
    // FIXME: handle truncate not done.
    // Fix up any stores or returns that use the results of the promoted
    // chain.
    for (auto *User : Instr->users()) {
      auto *UserI = dyn_cast<Instruction>(User);
      LLVM_DEBUG(dbgs() << "IR Truncate WIA: For User: " << *UserI << "\n");

      // Handle calls separately as we need to iterate over arg operands.
      if (auto *Call = dyn_cast<CallInst>(UserI)) {
        for (unsigned i = 0; i < Call->arg_size(); ++i) {
          Value *Arg = Call->getArgOperand(i);
          Type *Ty = NewTy;
          if (Instruction *Trunc = InsertTrunc(Arg, Ty)) {
            Trunc->moveBefore(Call);
            Call->setArgOperand(i, Trunc);
          }
        }
        continue;
      }

      // Special case switches because we need to truncate the condition.
      if (auto *Switch = dyn_cast<SwitchInst>(UserI)) {
        Type *Ty = NewTy;
        if (Instruction *Trunc = InsertTrunc(Switch->getCondition(), Ty)) {
          Trunc->moveBefore(Switch);
          Switch->setCondition(Trunc);
        }
        continue;
      }

      // Now handle the others.
      for (unsigned i = 0; i < UserI->getNumOperands(); ++i) {
        Type *Ty = NewTy;
        if (Instruction *Trunc = InsertTrunc(UserI->getOperand(i), Ty)) {
          Trunc->moveBefore(UserI);
          UserI->setOperand(i, Trunc);
        }
      }

      // VInstr->replaceAllUsesWith(Trunc);
    }
    break;
  }
  case WIAK_EXTLO: {
    Type *TruncTy = getTypeFromInteger(Sol->getWidth());
    Type *ExtTy = getTypeFromInteger(Sol->getUpdatedWidth());
    InsertSExtInReg(Instr, Instr, TruncTy, ExtTy);
    break;
  }
  case WIAK_FILL: {
    Type *TruncTy = getTypeFromInteger(Sol->getFillTypeWidth());
    Type *ExtTy = getTypeFromInteger(Sol->getUpdatedWidth());
    InsertSExtInReg(Instr, Instr, TruncTy, ExtTy);
    break;
  }
  case WIAK_DROP_EXT:
  case WIAK_DROP_LOCOPY:
  case WIAK_DROP_LOIGNORE: {
    auto CanDropExtFromAllUsers = [&](Value *Inst) {
      for (auto *U : Inst->users()) {
        if (auto *UseI = dyn_cast<Instruction>(U)) {
          // TODO: how to get the BestUserSol of the Inst
          // that is user to the Inst we need to update?
          // TODO: isLegalToPromote is suited for this case?
          unsigned short UseFillTypeWidth = getKnownFillTypeWidth(UseI);
          if (!isLegalToPromote(UseI) ||
              UseFillTypeWidth > Sol->getUpdatedWidth()) {
            // TODO: Maybe count users that we cannot promote or drop
            // the extension and use that information? If we can drop
            // the extension to most users maybe we can create another
            // extension for the remaining users??
            return false;
          }
        }
      }
      return true;
    };
    Value *N0 = Instr->getOperand(0);
    // TODO: BestSol maybe not be the AvailableSolutions[Op0][0]
    WideningIntegerSolutionInfo *BestSol = nullptr;
    if (auto *OP0 = dyn_cast<Instruction>(Instr->getOperand(0))) {
      auto It = BestSolPerInst.find(OP0);
      auto *Sol = It->second;
      BestSol = Sol;
    }
    if (CanDropExtFromAllUsers(Instr)) {
      replaceAllUsersOfWith(Instr, N0);
      if (BestSol != nullptr)
        N0->mutateType(getTypeFromInteger(BestSol->getUpdatedWidth()));
      InstsToRemove.insert(Instr);
    }
  } break;
  }
  // we need to have all the Combinations of the LegalUsersSolutions..
  return true;
}

bool WideningIntegerArithmetic::applyChain(
    Instruction *Instr,
    DenseMap<Value *, WideningIntegerSolutionInfo *> BestSolsUsersCombination) {
  bool Changed = false;

  for (auto &Combination : BestSolsUsersCombination) {
    // All users are solved at this point
    Value *VComb = Combination.first;
    WideningIntegerSolutionInfo *Sol = Combination.second;
    if (Sol->getKind() == WIAK_UNKNOWN) {
      dbgs() << "!!!!!!Best solution has Unknown kind..cannot apply it yet, ";
      if (auto *I = dyn_cast<Instruction>(VComb)) {
        dbgs() << "Opcode is " << I->getOpcode() << '\n';
      } else if (isa<ConstantInt>(VComb)) {
        dbgs() << "Opcode is Constant" << '\n';
      } else {
        dbgs() << "We don't know the opcode .. name is " << VComb->getName()
               << "\n";
      }
      return Changed;
    }
  }
  dbgs() << "Going to apply best Sols to users.";
  for (auto &Combination : BestSolsUsersCombination) {
    dbgs() << "Getting Sol!!\n";
    Instruction *UserI = dyn_cast<Instruction>(Combination.first);
    WideningIntegerSolutionInfo *BestUserSol = Combination.second;
    dbgs() << "Applying Single Sol for Inst " << *UserI << "\n";
    dbgs() << "With Solution " << *BestUserSol << "\n";
    applySingleSol(UserI,
                   BestUserSol); // All users are solved at this point
  }
  dbgs() << "Searching Sor for Inst! " << *Instr << "\n";
  auto SolIt = BestSolPerInst.find(Instr);
  dbgs() << "Found Sol or not??\n";
  if (SolIt == BestSolPerInst.end()) {
    dbgs() << "Printing inst..\n";
    dbgs() << "Inst is " << *Instr << "\n";
  }
  assert(SolIt != BestSolPerInst.end());
  dbgs() << "Applying Sol!! " << SolIt->second << "\n";
  applySingleSol(Instr, SolIt->second);
  return Changed;
}

bool WideningIntegerArithmetic::visitDROP_TRUNC(Instruction *Instr,
                                                std::queue<Value *> &Worklist) {

  assert(Instr->getOpcode() == Instruction::Trunc &&
         "Not an truncation to drop here");

  // TODO check can an operand of Truncation be a ConstantInt?
  // does it need to be extended? or we can be sure that N0 is never a
  // ConstantInt?
  Value *N0 = Instr->getOperand(0);
  Value *VInstr = dyn_cast<Value>(Instr);
  unsigned short TruncatedWidth = Instr->getType()->getScalarSizeInBits();
  if (auto *I = dyn_cast<Instruction>(N0)) {
    if (I->getOpcode() == Instruction::SExt) {
      // call closure on truncate width.
      assert(AvailableSolutions[I].size() > 0);
      SmallVector<unsigned short> AvailableWidths;
      for (auto *Sol : AvailableSolutions[I]) {
        AvailableWidths.push_back(Sol->getUpdatedWidth());
      }
      return visitEXTLO(Instr, AvailableWidths, Worklist);
    }
  }
  SmallVector<WideningIntegerSolutionInfo *> ExprSolutions =
      AvailableSolutions[N0];

  if (ExprSolutions.size() <= 0) {
    dbgs() << "!!!!Warning DropTrunc OP0 " << *N0 << "has no Solutions\n";
    createDefaultSol(N0);
    Worklist.push(N0);
  }

  bool Changed = false;
  // NewWidth is the width of the value before the truncation
  unsigned char NewWidth = N0->getType()->getScalarSizeInBits();
  // We simply drop the truncation and we will later see if it's needed.
  NumTruncatesDropped++;
  for (auto *Sol : ExprSolutions) {
    WideningIntegerSolutionInfo *Expr = new WIA_DROP_LOCOPY(
        Instr->getOpcode(), Sol->getOpcode(), Sol->getFillType(),
        Sol->getFillTypeWidth(), TruncatedWidth, NewWidth, Sol->getCost(),
        dyn_cast<Value>(Instr));
    Expr->setOperands(Sol->getOperands());
    if (addNonRedudant(AvailableSolutions[VInstr], Expr)) {
      Changed = true;
    }
  }
  // We simply drop the truncation and we will later see if it's needed.
  NumTruncatesDropped++;

  // We simply drop the truncation and we will later see if it's needed.
  for (auto *Sol : ExprSolutions) {
    WideningIntegerSolutionInfo *Expr = new WIA_DROP_LOIGNORE(
        Instr->getOpcode(), Sol->getOpcode(), Sol->getFillType(),
        Sol->getFillTypeWidth(), TruncatedWidth, TruncatedWidth, Sol->getCost(),
        Instr);
    Expr->setOperands(Sol->getOperands());
    if (addNonRedudant(AvailableSolutions[VInstr], Expr)) {
      Changed = true;
    }
  }
  return Changed;
}

bool WideningIntegerArithmetic::visitEXTLO(
    Instruction *Instr, SmallVector<unsigned short> TruncatedWidths,
    std::queue<Value *> &Worklist) {
  SolutionSet CalcSols;
  Value *N0 = Instr->getOperand(0);
  Value *N1 = Instr->getOperand(1);
  Value *VI = dyn_cast<Value>(Instr);
  // TODO check! is VTBits correct?
  unsigned VTBits = 32 - N0->getType()->getScalarSizeInBits();
  SolutionSet LeftSols = AvailableSolutions[N0];
  SolutionSet RightSols = AvailableSolutions[N1];
  if (LeftSols.size() <= 0) {
    Worklist.push(N0);
    createDefaultSol(N0);
  }
  if (RightSols.size() <= 0) {
    Worklist.push(N1);
    createDefaultSol(N1);
  }
  bool Changed = false;
  unsigned ExtensionOpc = getExtensionChoice(ExtensionChoice);
  for (auto *LeftSol : LeftSols) {
    for (auto *RightSol : RightSols) {
      // check that LeftSol->getWidth == RightSol->getWidth &&
      // check that Leftsol->fillTypeWidth == RightSol->fillTypeWidth
      // check that LeftSol->fillType = Zeros and LeftSol->fillType =
      // Garbage
      if (LeftSol->getUpdatedWidth() != RightSol->getUpdatedWidth() ||
          LeftSol->getFillTypeWidth() != RightSol->getFillTypeWidth() ||
          (LeftSol->getFillType() != ZEROS &&
           hasTypeGarbage(RightSol->getFillType()))) {
        continue;
      }
      for (int IntegerSize : IntegerSizes) {
        EVT NewVT = EVT::getIntegerVT(*Ctx, IntegerSize);
        for (auto TruncatedWidth : TruncatedWidths) {
          if (IntegerSize <= TruncatedWidth &&
              !TLI->isOperationLegal(ISD::SIGN_EXTEND_INREG, NewVT)) {
            continue;
          }
          unsigned Cost = LeftSol->getCost() + RightSol->getCost() + 1;
          // add all integers that are bigger to the solutions
          WideningIntegerSolutionInfo *Expr =
              new WIA_EXTLO(Instr->getOpcode(), ExtensionOpc, ExtensionChoice,
                            LeftSol->getFillTypeWidth(), TruncatedWidth,
                            IntegerSize, Cost, VI);
          Expr->addOperand(LeftSol);
          if (addNonRedudant(AvailableSolutions[VI], Expr)) {
            Changed = true;
          }
        }
      }
    }
  }
  return false;
}

bool WideningIntegerArithmetic::visitSUBSUME_FILL(Instruction *Instr) {
  return false;
}

bool WideningIntegerArithmetic::visitSUBSUME_INDEX(Instruction *Instr) {

  // We implement this rule implicitly
  // On every solution we consider the index n to stand
  // for all indices from n to w.
  return false;
}

bool WideningIntegerArithmetic::visitNATURAL(Instruction *Instr) {

  Value *VI = dyn_cast<Value>(Instr);
  SolutionSet Sols = AvailableSolutions[VI];
  bool Changed = false;
  for (WideningIntegerSolutionInfo *Sol : Sols) {
    Type *NewType = getTypeFromInteger(Sol->getUpdatedWidth());
    if (Sol->getFillType() == ANYTHING && TTI->isTypeLegal(NewType) &&
        Sol->getFillTypeWidth() == Sol->getUpdatedWidth())
      Sol->setFillType(ExtensionChoice);
    Changed = true;
  }
  return Changed;
}

inline WideningIntegerArithmetic::BinOpWidth
WideningIntegerArithmetic::createWidth(unsigned char op1, unsigned char op2,
                                       unsigned dst) {
  return std::make_tuple<unsigned char, unsigned char, unsigned char>(
      std::move(op1), std::move(op2), dst);
}

void WideningIntegerArithmetic::initOperatorsFillTypes() {

  // FillTypes are of the form op1 x op2 -> result
  // Stored in a tuple <op1, op2, result>
  UnaryFillTypeSet Popcnt, Neg, Com; // TODO maybe there are others too.
  FillTypeSet AddFillTypes;
  AddFillTypes.insert(std::make_tuple(ANYTHING, ANYTHING, ANYTHING));

  FillTypesMap[ISD::ADD] = AddFillTypes;

  FillTypeSet AndFillTypes;
  AndFillTypes.insert(std::make_tuple(SIGN, SIGN, SIGN));
  AndFillTypes.insert(std::make_tuple(ZEROS, ANYTHING, ZEROS));
  AndFillTypes.insert(std::make_tuple(ANYTHING, ZEROS, ZEROS));
  AndFillTypes.insert(std::make_tuple(ANYTHING, ANYTHING, ANYTHING));

  FillTypesMap[ISD::AND] = AndFillTypes;

  FillTypeSet UDivFillTypes, SDivFillTypes;
  UDivFillTypes.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));
  SDivFillTypes.insert(std::make_tuple(SIGN, SIGN, SIGN));

  FillTypesMap[ISD::SDIV] = SDivFillTypes;
  FillTypesMap[ISD::UDIV] = UDivFillTypes;

  FillTypeSet Eq, Ge, Geu, Gt, Gtu, Le, Leu, Lt, Ltu, Mod, Modu, Mul, Mulu, Ne,
      Or, Rem, Remu, Quot, Shl, Shra, Shrl, Sub, Xor;
  Eq.insert(std::make_tuple(SIGN, SIGN, SIGN));
  Eq.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));

  Ge.insert(std::make_tuple(SIGN, SIGN, SIGN));

  Geu.insert(std::make_tuple(SIGN, SIGN, SIGN));
  Geu.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));

  Gtu.insert(std::make_tuple(SIGN, SIGN, ZEROS));
  Gtu.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));

  Leu.insert(std::make_tuple(SIGN, SIGN, SIGN));
  Leu.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));

  Ltu.insert(std::make_tuple(SIGN, SIGN, ZEROS));
  Ltu.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));

  Mulu.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));

  Gt.insert(std::make_tuple(SIGN, SIGN, ZEROS));
  FillTypesMap[CmpInst::ICMP_UGE + ICMP_CONSTANT] = Geu;
  FillTypesMap[CmpInst::ICMP_UGT + ICMP_CONSTANT] = Gtu;
  FillTypesMap[CmpInst::ICMP_ULE + ICMP_CONSTANT] = Leu;
  FillTypesMap[CmpInst::ICMP_ULT + ICMP_CONSTANT] = Ltu;
  FillTypesMap[CmpInst::ICMP_EQ + ICMP_CONSTANT] = Eq;
  FillTypesMap[CmpInst::ICMP_SGE + ICMP_CONSTANT] = Ge;
  FillTypesMap[CmpInst::ICMP_SGT + ICMP_CONSTANT] = Gt;

  Le.insert(std::make_tuple(SIGN, SIGN, SIGN));
  FillTypesMap[CmpInst::ICMP_SLE + ICMP_CONSTANT] = Le;

  Lt.insert(std::make_tuple(SIGN, SIGN, SIGN));
  FillTypesMap[CmpInst::ICMP_SLT + ICMP_CONSTANT] = Lt;

  Mod.insert(std::make_tuple(SIGN, SIGN, SIGN));
  FillTypesMap[ISD::SREM] = SDivFillTypes;

  Modu.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));
  FillTypesMap[ISD::UREM] = SDivFillTypes;

  Ne.insert(std::make_tuple(SIGN, SIGN, ZEROS));
  FillTypesMap[CmpInst::ICMP_NE + ICMP_CONSTANT] = Ne;

  // MOD -> REMAINDER
  // DIV -> QUOTIENT

  Quot.insert(std::make_tuple(SIGN, SIGN, SIGN));
  // TODO what is the ISD OF THE QUOTIENT??  FillTypesMap[ISD::] =
  // SDivFillTypes;

  Mul.insert(std::make_tuple(SIGN, SIGN, SIGN)); // IF NOT OVERFLOW OCCURS
  FillTypesMap[ISD::MUL] = Mul;
  // TODO check ISD for MULU does not exist or it's a MULHU

  Or.insert(std::make_tuple(SIGN, SIGN, SIGN));
  Or.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));
  Or.insert(std::make_tuple(ANYTHING, ANYTHING, ANYTHING));
  FillTypesMap[ISD::OR] = Or;

  // Unary FillTypes
  // TODO Popcnt is intrinsic in llvm what to do here?
  // CHeck intrinsics sd nodes and get popcnt
  Popcnt.insert(std::make_tuple(ZEROS, ZEROS));

  // TODO what is Com??
  Com.insert(std::make_tuple(SIGN, SIGN));
  Com.insert(std::make_tuple(ANYTHING, ANYTHING));

  // TODO what is the ISD:Opcode of NEG ?
  Neg.insert(std::make_tuple(ANYTHING, ANYTHING));

  Rem.insert(std::make_tuple(SIGN, SIGN, SIGN));
  FillTypesMap[ISD::SREM] = Or;

  Remu.insert(std::make_tuple(ZEROS, ZEROS, ZEROS)); // TODO CHECK REMU
  FillTypesMap[ISD::UREM] = Remu;

  Shl.insert(std::make_tuple(ANYTHING, ZEROS, ANYTHING));
  FillTypesMap[ISD::SHL] = Shl;

  Shra.insert(std::make_tuple(SIGN, ZEROS, SIGN));
  FillTypesMap[ISD::SRA] = Shra;

  Shrl.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));
  FillTypesMap[ISD::SRL] = Shrl;

  Sub.insert(std::make_tuple(ANYTHING, ANYTHING, ANYTHING));
  FillTypesMap[ISD::SUB] = Sub;

  Xor.insert(std::make_tuple(SIGN, SIGN, SIGN));
  Xor.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));
  Xor.insert(std::make_tuple(ANYTHING, ANYTHING, ANYTHING));
  FillTypesMap[ISD::XOR] = Xor;
}

inline bool WideningIntegerArithmetic::hasTypeGarbage(IntegerFillType fill) {
  return 1;
}

inline bool WideningIntegerArithmetic::hasTypeT(IntegerFillType fill) {
  return hasTypeGarbage(fill);
}

inline bool WideningIntegerArithmetic::hasTypeS(IntegerFillType fill) {
  if (fill != ANYTHING && fill != ZEROS && fill != SIGN) {
    return 0;
  }
  return 1;
}
