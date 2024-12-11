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
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Use.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
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
#include <memory>
#include <queue>
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
  typedef struct GlobalSolData {
    unsigned TotalCost;

  } GlobalSolData_t;
  using GlobalSols = SmallVector<WideningIntegerSolutionInfo *, 16>;
  using isSolvedMap = DenseMap<Value *, bool>;
  using SolutionSet = SmallVector<const WideningIntegerSolutionInfo *>;
  using SolutionSetParam = SmallVectorImpl<WideningIntegerSolutionInfo *>;
  using AvailableSolutionsMap =
      DenseMap<Value *, SmallVector<WideningIntegerSolutionInfo *, 8>>;

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

  bool visit_widening(Value *VI, std::queue<Value *> &Worklist);

  FillTypeSet getOperandFillTypes(Instruction *Instr);

  void setFillType(Type *IType);
  inline bool isSolved(Value *V);
  inline bool isInWorklist(Value *V);
  inline bool isLegalToPromote(Instruction *Instr);

  bool addNonRedudant(SmallVector<WideningIntegerSolutionInfo *, 8> &Solutions,
                      WideningIntegerSolutionInfo *GeneratedSol);
  inline bool hasTypeGarbage(IntegerFillType Fill);
  inline bool hasTypeT(IntegerFillType Fill);
  inline bool hasTypeS(IntegerFillType Fill);
  bool closure(Instruction *Instr, std::queue<Value *> &Worklist);
  bool tryClosure(Instruction *Instr, std::queue<Value *> &Worklist);

  bool visitInstruction(Value *VI, std::queue<Value *> &Worklist);
  bool visitBINOP(Instruction *Binop, std::queue<Value *> &Worklist);
  bool combineSols(Instruction *Binop, std::queue<Value *> &Worklist);
  bool visitICMP(Instruction *Icmp, std::queue<Value *> &Worklist);
  bool visitLOAD(Instruction *Instr);
  bool visitSTORE(Instruction *Instr);
  bool visitUNOP(Instruction *Instr);
  // TODO: currently constant labels of switch do not have any solutions.
  // FIXME:
  bool visitSWITCH(Instruction *Instr, std::queue<Value *> &Worklist);
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

  struct InstSolutions {
    DenseMap<Instruction *, SolutionSet> InstSols;
    DenseMap<Value *, SolutionSet> OperandSols;
    DenseMap<Instruction *, SolutionSet> UserSols;
  };
  // Returns true if it finds matching Solutions for every User and Operand
  // for every Instruction in F.
  bool generateOpsAndUserSols(Function &F);
  // Every Instruction may have multipe global Solutions.
  // Each global Solution has the totalCost, the solution for every value
  // that is related to the instruction, operands and users
  DenseMap<Instruction *, InstSolutions> SolsPerInst;

  // A map that holds for every instruction the best solution
  // for each of the users and operands of that instruction.
  DenseMap<Instruction *,
           DenseMap<Value *, const WideningIntegerSolutionInfo *>>
      BestUserOpsSolsPerInst;
  DenseMap<Value *, const WideningIntegerSolutionInfo *> BestSolPerInst;
  SetVector<Value *> NewInsts;
  SetVector<Value *> AppliedSols;
  SmallVector<Instruction *> ICmpInsts;
  // For every instruction **I** inside the Function
  // it calculates the best combination among the legal
  // solutions of every User of **I**
  void calcBestUserSols(Function &F);

  void calcBestUserSols_v2(Function &F);

  inline bool checkUserSols(Instruction *User, Instruction *Instr,
                            const WideningIntegerSolutionInfo *InstSol,
                            bool &ChangedWidth, SolutionSet &LegalUserSols,
                            WideningIntegerSolutionInfo *&NewUserSol);

  inline bool updateOperands(Instruction *User,
                             const WideningIntegerSolutionInfo *UserSol,
                             DenseMap<Value *, SolutionSet> &CandidateSols);

  void replaceAllUsersOfWith(Value *From, Value *To);

  // Applies this *Sol* to the Value  *V*
  bool applySingleSol(Value *V, const WideningIntegerSolutionInfo *Sol);
  // iterates all the uses of an Instruction that is solved
  // and applies the updated Instruction to all the uses.
  // Note that it might need to update the uses and ops of the Instruction also.
  bool applyChain(Instruction *Instr);

  bool applyPHI(Instruction *I, WideningIntegerSolutionInfo *Sol);

  void updateSwitchLabels(Instruction *I, Type *OldType, Type *NewType);
  Value *getPhiSuccessor(Value *V);

  std::vector<unsigned short> IntegerSizes = {8, 16, 32, 64};

  unsigned RegisterBitWidth = 0;

  BinOpWidth createWidth(unsigned char op1, unsigned char op2, unsigned dst);

  inline bool IsBinop(unsigned Opcode);
  inline bool IsICMP(unsigned Opcode);
  inline bool IsUnop(unsigned Opcode);
  inline bool IsTruncate(unsigned Opcode);
  inline bool IsExtension(unsigned Opcode);
  inline bool IsLit(unsigned Opcode);
  inline bool IsLoad(unsigned Opcode);
  inline bool IsStore(unsigned Opcode);
  inline bool IsPHI(unsigned Opcode);
  inline bool IsSwitch(unsigned Opcode);
  // Helper functions
  inline unsigned int getScalarSize(const Type *Typ) const;
  inline unsigned getExtensionChoice(enum IntegerFillType ExtChoice);
  inline unsigned getExtCost(Instruction *Instr,
                             const WideningIntegerSolutionInfo *Sol,
                             unsigned short IntegerSize);
  void initOperatorsFillTypes();
  void printInstrSols(Value *V);
  void printInstrSolsDetailed(Value *V);
  inline Type *getTypeFromInteger(int Integer);
  inline int getIntegerFromType(Type *ty);
  inline short int getKnownFillTypeWidth(Value *V);

  bool inline isLegalAndMatching(const WideningIntegerSolutionInfo *Sol1,
                                 const WideningIntegerSolutionInfo *Sol2) const;
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
  auto SolsIt = AvailableSolutions.find(V);
  if (SolsIt == AvailableSolutions.end()) {
    assert(0 && "Didn't find sols to print..\n");
  }
  auto Sols = SolsIt->second;

  if (auto *I = dyn_cast<Instruction>(V)) {
    dbgs() << "Printing Sols of Inst: " << *I << "\n";
  } else if (isa<ConstantInt>(V)) {
    dbgs() << "Printing Sols of ConstantInt: " << *V << "\n";
  }
  int i = 0;
  for (const WideningIntegerSolutionInfo *Sol : Sols) {
    dbgs() << "----- Solution " << ++i << "\n" << (*Sol) << "\n";
    // dbgs() << "------- Solution oldWidth: " << Sol->getWidth()
    //        << " newWidth: " << Sol->getUpdatedWidth() << "\n";
  }
}

void WideningIntegerArithmetic::printInstrSolsDetailed(Value *V) {
  auto SolsIt = AvailableSolutions.find(V);
  if (SolsIt == AvailableSolutions.end()) {
    assert(0 && "Didn't find sols to print..\n");
  }
  auto Sols = SolsIt->second;
  if (auto *I = dyn_cast<Instruction>(V)) {
    dbgs() << "Printing Sols of Inst: " << *I << "\n";
  } else if (isa<ConstantInt>(V)) {
    dbgs() << "Printing Sols of ConstantInt: " << *V << "\n";
  }
  int i = 0;
  for (const WideningIntegerSolutionInfo *Sol : Sols) {
    dbgs() << "Sol kind is --> " << Sol->getKind() << "\n";
    dbgs() << "Sol fillType is --> " << Sol->getFillType() << "\n";
    dbgs() << "Sol fillTypeWidth is --> " << Sol->getFillType() << "\n";
    dbgs() << "Sol oldWidth is --> " << Sol->getWidth() << "\n";
    dbgs() << "Sol updatedWidth is --> " << Sol->getUpdatedWidth() << "\n";
    dbgs() << "Sol cost is --> " << Sol->getCost() << "\n";
    dbgs() << "----- Solution " << ++i << "\n" << (*Sol) << "\n";
  }
}

inline unsigned
WideningIntegerArithmetic::getExtensionChoice(enum IntegerFillType ExtChoice) {
  return ExtChoice == SIGN ? Instruction::SExt : Instruction::ZExt;
}

inline bool WideningIntegerArithmetic::IsICMP(unsigned Opcode) {
  switch (Opcode) {
  case Instruction::ICmp:
    return true;
  default:
    return false;
  }
  return false;
}

bool inline WideningIntegerArithmetic::IsBinop(unsigned Opcode) {
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
    return true;
  default:
    return false;
  }
  return false;
}

// TODO all UNOPS are Intrinsics check IntrinsicInst
inline bool WideningIntegerArithmetic::IsUnop(unsigned Opcode) {
  switch (Opcode) {
  default:
    return false;
  }
  return false;
}

inline bool WideningIntegerArithmetic::IsTruncate(unsigned Opcode) {
  switch (Opcode) {
  case Instruction::Trunc:
    return true;
  default:
    break;
  }
  return false;
}

inline bool WideningIntegerArithmetic::IsPHI(unsigned Opcode) {
  switch (Opcode) {
  case Instruction::PHI:
    return true;
  default:
    return false;
  }
}

inline bool WideningIntegerArithmetic::IsSwitch(unsigned Opcode) {
  switch (Opcode) {
  case Instruction::Switch:
    return true;
  default:
    return false;
  }
}

inline bool WideningIntegerArithmetic::IsExtension(unsigned Opcode) {
  switch (Opcode) {
  case Instruction::ZExt:
  case Instruction::SExt:
    return true;
  default:
    return false;
  }
  return false;
}

inline bool WideningIntegerArithmetic::IsStore(unsigned Opcode) {
  switch (Opcode) {
  case Instruction::Store:
    return true;
  default:
    break;
  }
  return false;
}

inline bool WideningIntegerArithmetic::IsLoad(unsigned Opcode) {
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
    dbgs() << "Opcode is " << OpcodesToStr[Opcode] << "\n";
    if (IsBinop(Opcode)) {
      dbgs() << " and Visiting Binop..\n";
      return visitBINOP(Instr, Worklist);
    } else if (IsICMP(Opcode)) {
      dbgs() << " and Visiting ICMP..\n";
      return visitICMP(Instr, Worklist);
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
    } else if (IsSwitch(Opcode)) {
      return visitSWITCH(Instr, Worklist);
    } else {
      dbgs() << "Could not found a visit function for Opcode " << Opcode
             << "\n";
      ////dbgs() << "Defaut sol for Opcode " << OpcodesToStr[Opcode] << "\n";
      // default solution so we can initialize all the nodes with some solution
      // set.
      auto *Sol = new WideningIntegerSolutionInfo(
          Opcode, Opcode, ANYTHING, RegisterBitWidth, RegisterBitWidth,
          /* TODO: CHECK */ RegisterBitWidth, 0, WIAK_UNKNOWN, Instr);
      dbgs() << "v123Created Sol --> " << *Sol << "\n";
      Changed = addNonRedudant(AvailableSolutions[VI], Sol);
    }
  } else if (auto *CI = dyn_cast<ConstantInt>(VI)) {
    // dbgs() << " and Visiting Constant.." << "\n";
    return visitCONSTANT(CI);
  } else {

    dbgs() << "Careful here Adding Default value" << "\n";
    int FillTypeWidth = getKnownFillTypeWidth(VI);
    auto *Sol = new WideningIntegerSolutionInfo(
        0, 0, ANYTHING, FillTypeWidth, RegisterBitWidth,
        /* TODO CHECK */ RegisterBitWidth, 0, WIAK_UNKNOWN, Instr);
    dbgs() << "v123Created Sol --> " << *Sol << "\n";
    Changed = addNonRedudant(AvailableSolutions[VI], Sol);
    // dbgs() << "Adding Default value Succedded is " << Changed << "\n";
  }

  // TODO check if we are missing opcodes here, need to add them.
  return Changed;
}

bool WideningIntegerArithmetic::addNonRedudant(
    SmallVector<WideningIntegerSolutionInfo *, 8> &Solutions,
    WideningIntegerSolutionInfo *GeneratedSol) {
  bool WasRedudant = false;
  int RedudantNodeToDeleteCost = INT_MAX;
  LLVM_DEBUG(dbgs() << "Begin Add Non Redudant -> " << '\n');
  for (auto *It = Solutions.begin(); It != Solutions.end();) {
    int ret = (*It)->IsRedudant((*GeneratedSol));
    if (ret == -1) { // GeneratedSol is redudant
      WasRedudant = true;
      dbgs() << "Generated sol is redudant\n";
      It++;
      // dbgs() << "GeneratedSol is redudant! ret = -1" << '\n';
    } else if (ret == 1) { // Sol is redudant
      assert(GeneratedSol->getCost() < RedudantNodeToDeleteCost);
      dbgs() << "ret = 1 Current Sol is Redudant .... Deleting it --> "
             << (**It) << '\n';
      It = Solutions.erase(It);
      // TODO consider change data structure for Possible small optimization
    } else { // ret == 0 no redudant found
      It++;
    }
  }
  if (!WasRedudant) {
    LLVM_DEBUG(dbgs() << "!!!!!!!!!!!!!!Adding Solution --> "
                      << OpcodesToStr[GeneratedSol->getOpcode()] << "\n");
    dbgs() << "Adding sol " << *GeneratedSol << '\n';
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
  // assert(IsBinop(Opcode) && "Not a binary operator.");
  if (auto *CI = dyn_cast<ICmpInst>(Instr)) {
    unsigned Pred = CI->getPredicate();
    dbgs() << "Searching in " << ICMP_CONSTANT + Pred << "\n";
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

      if (isa<IntegerType>(I.getType()) || isa<SwitchInst>(I)) {
        createDefaultSol(VInstr);
        visit_widening(VInstr, Worklist);
      } else if (isa<BranchInst>(I)) {
        createDefaultSol(VInstr);
      }
      // dbgs() << "---------------------------------------------------------"
      //      << '\n';
    }
  }
  dbgs() << "Before calculating best sols-->\n";
  for (auto *Inst : ICmpInsts) {
    dbgs() << "Printing Sols for Inst " << *Inst << "\n";
    for (auto *Sol : AvailableSolutions[Inst]) {
      dbgs() << "Icmp Sol is " << *Sol << "\n";
    }
  }
  dbgs() << "Trying to calculate best Solutions..\n";

  calcBestUserSols(F);

  dbgs() << "after calc best sols..!\n";
  for (auto *Inst : ICmpInsts) {
    dbgs() << "Printing ICMP Sols for Inst " << *Inst << "\n";
    for (auto *Sol : AvailableSolutions[Inst]) {
      dbgs() << "Icmp Sol is " << *Sol << "\n";
    }
  }

  /*
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (isa<IntegerType>(I.getType()) || isa<SwitchInst>(I)) {
        printInstrSolsDetailed(&I);
      }
    }
  }*/

  bool Changed = false;
  dbgs() << "Applying Solutions ...\n";
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (isa<IntegerType>(I.getType()) || isa<SwitchInst>(I)) {
        if (AvailableSolutions[&I].empty() == true) {
          dbgs() << "45adidn't find solutions for Inst --> " << I << "\n";
        }
        dbgs() << "Calling apply chain for Instruction --> " << I << "\n";
        bool ChangedOnce = applyChain(&I);
        if (ChangedOnce) {
          Changed = true;
        }
      }
    }
  }
  if (!InstsToRemove.empty()) {
    for (auto *I : InstsToRemove)
      I->eraseFromParent();
    InstsToRemove.clear();
  }
  dbgs() << "Final Function after all modifications is --> " << "\n";
  dbgs() << F << "\n";

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
    dbgs() << "visiting fill->\n";
    auto FillsList = visitFILL(Instr, Worklist);
    dbgs() << "visiting widen->\n";
    auto WidensList = visitWIDEN(Instr, Worklist);
    // auto GarbageWidenList = visitWIDEN_GARBAGE(Instr, Worklist);
    // dbgs() << "Visit Widen Garbage Done\n";
    dbgs() << "visiting narrow->\n";
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
  dbgs() << "Visiting constant..!\n";
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
      bool Added = addNonRedudant(AvailableSolutions[VCI], Sol);
      if (Added) {
        Changed = true;
      }
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
  for (const WideningIntegerSolutionInfo *Sol : N0Sols) {
    auto *StoreSol = new WIA_STORE(Instr->getOpcode(), Instr->getOpcode(),
                                   Sol->getFillType(), N0Bits, N0Bits, N0Bits,
                                   Sol->getCost(), VI);
    bool Added = addNonRedudant(AvailableSolutions[VI], StoreSol);
    if (Added) {
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
    dbgs() << "Created widen inst --> " << *Widen
           << "\ninside visitLOAD for Opc --> " << OpcodesToStr[Opc] << "\n";
    Widen->addOperand(std::make_unique<WIA_LOAD>(*WIALoad));
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
    return true;
  default:
    return false;
  }
  return false;
}

bool inline WideningIntegerArithmetic::isLegalAndMatching(
    const WideningIntegerSolutionInfo *Sol1,
    const WideningIntegerSolutionInfo *Sol2) const {
  return Sol1->getUpdatedWidth() == Sol2->getUpdatedWidth();
}

// TODO: add extension if it's needed to an operand, check how it is safe.
inline bool WideningIntegerArithmetic::updateOperands(
    Instruction *I, const WideningIntegerSolutionInfo *InstSol,
    DenseMap<Value *, SolutionSet> &CandidateSols) {

  if (isa<SwitchInst>(I)) {
    dbgs() << "Found Switch User.." << "\n";
  }
  dbgs() << "Entering updateOperands..\n";
  bool MatchingForAllOps = true;
  DenseMap<Value *, SolutionSet> OperandSols;
  bool processed_cond = true;
  for (unsigned i = 0U, e = I->getNumOperands(); i < e && processed_cond;) {
    Value *Op;
    if (auto *SI = dyn_cast<SwitchInst>(I)) {
      Op = SI->getCondition();
      dbgs() << "142Found Switch Cond " << *Op << "\n";
      processed_cond = false;
    } else {
      Op = I->getOperand(i);
      ++i;
    }
    if (auto *PHI = dyn_cast<PHINode>(I)) {
      dbgs() << "WOWWW first operand for " << *I << "is -----> " << *Op << "\n";
    }

    Type *OpTy = Op->getType();
    // if (isa<SwitchInst>(I) && !isa<ConstantInt>(Op)) {
    //   dbgs() << "CHECK if correct Ignoring Operand --> " << *Op << "\n";
    //   continue;
    // }
    //  TODO: what to do with operands that are not Int?
    //  We do not have any solutions for them.
    if (!OpTy->isIntOrIntVectorTy()) {
      dbgs() << "Didn't find int-type for op " << *Op << "\n";
      continue;
    }
    // auto OpSols = AvailableSolutions[Op];
    bool FoundMatchingSol = false;
    auto InstrUpdatedWidth = InstSol->getUpdatedWidth();
    if (auto *Icmp = dyn_cast<WIA_ICMP>(InstSol)) {
      auto Op0Width = getIntegerFromType(I->getOperand(0)->getType());
      dbgs() << "Update operands.. Found icmp sol.. address is "
             << Icmp->getValue() << "\n";
      dbgs() << "Update operands.. Found icmp sol.. value is "
             << *Icmp->getValue() << "\n";
      InstrUpdatedWidth = Op0Width;
      dbgs() << "Found icmp and updated to icmp width " << InstrUpdatedWidth
             << "From width --> " << InstSol->getUpdatedWidth() << "\n";
    }
    dbgs() << "Inside updateOperands Checking Operand " << *Op << "\n";
    dbgs() << "Inside updateOperands inst updated width is --> "
           << InstrUpdatedWidth << "\n";
    for (auto *OpSol : AvailableSolutions[Op]) {
      WIAKind SolKind = OpSol->getKind();
      dbgs() << "Inside update Operands Checking Sol " << *OpSol << "\n";
      dbgs() << "Inside update Operands Checking Sol " << OpSol << "\n";
      if (OpSol->getUpdatedWidth() == InstrUpdatedWidth) {
        FoundMatchingSol = true;
      } else {
        if (auto *OpI = dyn_cast<Instruction>(Op)) {
          if ((isLegalToPromote(OpI) &&
               getIntegerFromType(OpI->getType()) < InstrUpdatedWidth) &&
              // TODO: only for binops for now.
              SolKind == WIAK_BINOP) {
            auto *NewOpSol = new WideningIntegerSolutionInfo(*OpSol);
            dbgs() << "Update Operands Inst that got updated width is --> "
                   << *InstSol << "\n";
            NewOpSol->setUpdatedWidth(InstrUpdatedWidth);
            dbgs() << "v177 Created new OpSol --> " << *NewOpSol
                   << "for Instruction --> " << *OpI << "\n";
            FoundMatchingSol = true;
          }
        } else if (isa<ConstantInt>(Op)) {
          // TODO: what if the UpdatedWidth of Instr is lower than before?
          // maybe the constant doesn't fit in that specific bitWidth?
          auto *NewOpSol = new WideningIntegerSolutionInfo(*OpSol);
          NewOpSol->setUpdatedWidth(InstrUpdatedWidth);
          dbgs() << "v123 Created new OpSol --> " << *NewOpSol << "\n";
          FoundMatchingSol = true;
        }
      }
      // TODO: Is there a case where we can insert an extension
      // and increase the cost? Maybe isLegalToPromote covers all of them.
      //
      // Type *OpType = Op->getType();
      // auto UpdatedWidth = InstSol->getUpdatedWidth();
      // we increment the cost because we need to insert a ext or
      // trunc here.
      if (FoundMatchingSol) {
        CandidateSols[Op].push_back(OpSol);
        OperandSols[Op].push_back(OpSol);
      } else {
        dbgs() << "Inside Update Operands Didn't find matching sol for "
               << *OpSol << "\n";
      }
    }

    if (!FoundMatchingSol) {
      dbgs()
          << "UPDATE OPERANDS INSIDE .. Didn't find matching sol for Operand "
          << *Op << "\n";
      dbgs() << "Trying to match with Inst with width--> " << InstrUpdatedWidth
             << "\n of Inst " << *I << "\n";
      dbgs() << "Solutions are for operand are ..->\n";
      printInstrSols(Op);

      MatchingForAllOps = false;
    } else {
      SolsPerInst[I].OperandSols = OperandSols;
      dbgs() << ""
                " FOUND MATCHING SOL.\n";
      dbgs() << "matching forAllOps is  --> " << MatchingForAllOps << "\n";
      if (isa<ICmpInst>(I)) {
        dbgs() << "After found matching sol inside update operands printing "
                  "sols detailed for icmp"
               << *I << "\n";
        printInstrSolsDetailed(I);
      }
    }
  }
  dbgs() << "Exiting updateOperands..\n";
  return MatchingForAllOps;
}

// TODO: add extension if it's needed to a user, check how it is safe.
inline bool WideningIntegerArithmetic::checkUserSols(
    Instruction *User, Instruction *Instr,
    const WideningIntegerSolutionInfo *InstSol, bool &ChangedWidth,
    SolutionSet &LegalUserSols, WideningIntegerSolutionInfo *&NewUserSol) {

  auto CurrentInstWidth = InstSol->getUpdatedWidth();
  dbgs() << "For inst --> " << *Instr << "\n";
  dbgs() << "For current inst width --> " << CurrentInstWidth << "\n";
  dbgs() << "For user --> " << *User << "\n";
  dbgs() << "Printing inst sols here --> " << "\n";
  // printInstrSols(Instr);
  dbgs() << "Printing user sols here --> " << "\n";
  // printInstrSols(User);

  // TODO: some widths are in the sols of the instruction but are not checked..
  // why?
  bool FoundMatch = false;
  auto UserIt = AvailableSolutions.find(User);
  if (UserIt != AvailableSolutions.end() && AvailableSolutions[User].empty()) {
    dbgs() << "Encountered User --> " << *User << "With no solutions  " << "\n";
    Type *Ty;
    if (auto *BrInst = dyn_cast<BranchInst>(User)) {
      auto *Condition = BrInst->getCondition();
      Ty = Condition->getType();
    } else {
      Ty = User->getType();
    }
    dbgs() << "1599 Type is " << *Ty << "\n";
    auto TypeWidth = getIntegerFromType(Ty);
    dbgs() << "Type width is --> " << TypeWidth << "\n";
    auto *UnknownSol =
        new WIA_UNKNOWN(User->getOpcode(), User->getOpcode(), ANYTHING,
                        TypeWidth, TypeWidth, TypeWidth, 0, User);
    dbgs() << "v173 Encountered User --> " << *User
           << "With no solutions and created Unknown sol --> \n"
           << *UnknownSol << "\n";
    AvailableSolutions[User].push_back(UnknownSol);
  } else if (UserIt == AvailableSolutions.end()) {
    dbgs() << "17890 User --> " << *User << "doesn't exist.\n";
  }

  dbgs() << "CheckUserSols --> Checking user address -----> " << User << "\n";
  dbgs() << "CheckUserSols --> Checking user value -----> " << *User << "\n";
  for (auto *UserSol : AvailableSolutions[User]) {
    unsigned short UserUpdatedWidth = UserSol->getUpdatedWidth();
    dbgs() << "Checking UserSol " << UserSol << "\n";
    // 2, possible 3 cases
    // 1) same Widths cost of Combination is zero
    // 2) different widths two cases
    // 3) can we promote one type to another? if yes
    // cost + 1 if we add an extension.
    // We may can promote for free.
    // promoted type if we cannot no possible combination
    // 4) How to handle the truncate?
    dbgs() << "User updated width is --> " << UserUpdatedWidth << "\n";
    dbgs() << "Instr updated width is --> " << CurrentInstWidth << "\n";
    if (UserUpdatedWidth == CurrentInstWidth) {
      if (UserUpdatedWidth != UserSol->getWidth()) {
        ChangedWidth = true;
      }
      LegalUserSols.push_back(UserSol);
      FoundMatch = true;
    }
  }
  if (!FoundMatch) {
    dbgs() << "Didn't find match iterating sols for user --> " << *User << "\n";
    dbgs() << "Didn't find match iterating sols for user --> " << User << "\n";
    for (auto *UserSol : AvailableSolutions[User]) {
      if (isLegalToPromote(User) &&
          UserSol->getUpdatedWidth() < CurrentInstWidth &&
          UserSol->getKind() == llvm::WIAK_BINOP) {
        dbgs() << "Promoting but Checking UserSol " << UserSol << "\n";
        dbgs() << "CheckUserSols Creating new width for User --> " << *User
               << "\n";
        dbgs() << "CheckUserSols Creating new width for User address --> "
               << User << "\n";
        auto *NewUSol = new WideningIntegerSolutionInfo(*UserSol);
        dbgs() << "CheckUserSols UserSol after creating new sol is --> "
               << *UserSol << "\n";
        dbgs() << "CheckUserSols UserSol after creating new sol is --> "
               << UserSol << "\n";
        NewUSol->setUpdatedWidth(CurrentInstWidth);
        dbgs() << "v133 Created new User sol --> " << *NewUSol << "\n";
        dbgs() << "CHECK USERS... Created new sol from user --> " << *UserSol
               << "\n";
        FoundMatch = true;
        // dbgs() << "Found All MatchingWidth!!! AllMatchingWidth -->"
        //     << AllMatchingWidth << "\n";
        if (NewUSol->getUpdatedWidth() != NewUSol->getWidth())
          ChangedWidth = true;
        // AvailableSolutions[User].push_back(NewUSol);
        LegalUserSols.push_back(NewUSol);
        NewUserSol = NewUSol;
        // we don't need more than one for this Instr.
        break;
      }
    }
  }
  return FoundMatch;
}

void WideningIntegerArithmetic::calcBestUserSols(Function &F) {
  std::queue<Instruction *> workList;
  // int TotalCost = INT_MAX;
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    if (isa<IntegerType>((*I).getType()) || isa<SwitchInst>(*I)) {
      workList.push(&*I);
      printInstrSolsDetailed(&*I);
    }
  }

  SetVector<Instruction *> InstsVisited;
  // for debugging purposes
  SmallVector<Instruction *> usersEncountered;
  while (!workList.empty()) {
    Instruction *Instr = workList.front();
    workList.pop();
    if (InstsVisited.contains(Instr)) {
      continue;
    }
    int MinCost = INT_MAX;
    const WideningIntegerSolutionInfo *BestInstSol;
    SmallVector<Value *, 4> VUsers;
    DenseMap<Instruction *, SolutionSet> CurrentInstSol;
    // Every Instruction may have multipe global Solutions.
    // Each global Solution has the totalCost, the solution for every value
    // that is related to the instruction, operands and users
    std::vector<DenseMap<Value *, SolutionSet>> Combinations;
    DenseMap<Value *, const WideningIntegerSolutionInfo *> BestCombination;
    // New Solutions that cannot be inserted to AvailableSolutions at the
    // moment they are created
    DenseMap<Value *, SmallVector<WideningIntegerSolutionInfo *, 8>>
        PendingSols;
    if (Instr->getNumUses() == 0 && !isa<SwitchInst>(Instr)) {
      dbgs() << "Didn't find any uses..\n";
      continue;
      // TODO: Instruction is dead erase it ??
    }
    auto *VInstr = dyn_cast<Value>(Instr);
    // NOTE: MatchingWidthForAllUsers only for debugging
    bool MatchingWidthForAllUsers = false;
    dbgs() << "Checking Instr Solutions for Inst --> " << *Instr << "\n";
    dbgs() << "====== Printing and ITerating solutions.."
              "--------------------------------------------------\n";
    // printInstrSolsDetailed(Instr);
    for (const WideningIntegerSolutionInfo *InstSol :
         AvailableSolutions[Instr]) {
      if (InstSol == nullptr) {
        dbgs() << "Inst sol is nullptr\n";
      }
      dbgs() << "Checking Inst solution199 ------------> \n";
      dbgs() << InstSol << "\n";
      dbgs() << "Inst is --> " << *Instr << "\n";
      dbgs() << "Inst address is --> " << Instr << "\n";
      if (isa<ICmpInst>(Instr)) {
        printInstrSolsDetailed(Instr);
      }
      dbgs() << "-----------------!!!!!!!!!!534445----------> width is "
             << InstSol->getUpdatedWidth() << "\n";
      dbgs() << "InstSol is --> " << *InstSol << "\n";
      unsigned short InstrUpdatedWidth = InstSol->getUpdatedWidth();
      dbgs() << "Inst updated width is --> " << InstrUpdatedWidth << "\n";

      bool DiscardSolution = false;
      // TODO: if an Operand is an Instruction with uses, probably we need to
      // check their solutions and the users and so on..?
      DenseMap<Value *, SolutionSet> CandidateSols;
      dbgs() << "================================= MATCHING OPS FOR INST "
             << *Instr << "\n";
      bool MatchingForOps = updateOperands(Instr, InstSol, CandidateSols);
      if (!MatchingForOps) {
        dbgs() << "UPDATE OPERANDS FAILED Discarding Sol didn't found op "
                  "width..size is "
               << AvailableSolutions[VInstr].size() << "\n";
        dbgs() << "Instr is --> " << *VInstr << "\n";
        dbgs() << "And width is --> " << InstrUpdatedWidth << "\n";
        dbgs() << "Ignoring width --> " << InstrUpdatedWidth << "\n";
        continue;
      }
      bool MatchingPhis = false;
      bool isPhi = false;
      if (auto *PHI = dyn_cast<PHINode>(Instr)) {
        // MatchingPhis = checkPHISols(Instr, InstSol, CandidateSols);
        // isPhi = true;
      }

      dbgs() << "Checking Instr Width " << InstrUpdatedWidth << "\n";

      SetVector<Instruction *> userWorklist;
      bool AllMatching = true;
      bool MatchingForAllOps = true;

      for (auto *User : Instr->users()) {
        if (auto *UserI = dyn_cast<Instruction>(User))
          userWorklist.insert(UserI);
      }
      DenseMap<Instruction *, SolutionSet> UserSols;
      SetVector<Instruction *> UsersVisited;
      // current inst Sol width
      while (!userWorklist.empty()) {
        SolutionSet LegalUserSols;
        Instruction *User = userWorklist.pop_back_val();
        if (UsersVisited.count(User)) {
          continue;
        }
        usersEncountered.push_back(User);
        bool ChangedWidth = false;
        bool MatchingWidth = false;
        // TODO: maybe we can check if at least one solution was legal
        // check if the AllMatchingWidth is equivalent.
        dbgs() << "====================== Checking User Sols for Inst --> "
               << *Instr << "\n";
        WideningIntegerSolutionInfo *NewUSol = nullptr;
        bool isLegal = checkUserSols(User, Instr, InstSol, ChangedWidth,
                                     LegalUserSols, NewUSol);
        if (isLegal) {
          MatchingWidth = true;
          dbgs() << "Found matching width and isLegal width is "
                 << InstSol->getUpdatedWidth() << "\n";
          dbgs() << "InstSol is " << *InstSol << "\n";
          CandidateSols[User] = LegalUserSols;
          if (NewUSol != nullptr) {
            PendingSols[User].push_back(NewUSol);
          }
        }
        if (ChangedWidth) {
          bool MatchingOps = false;
          // TODO: refactor this to be done inside updateOperands
          for (auto *UserSol : LegalUserSols) {
            dbgs() << "Checking Operands of User--> " << *User << "\n";
            dbgs() << "With LEgalUser sol --> " << *UserSol << "\n";
            MatchingOps = updateOperands(User, UserSol, CandidateSols);
            if (MatchingOps) {
              MatchingForAllOps = true;
            }
          }
        }
        if (MatchingWidth == false) {
          dbgs() << "Exiting...!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Didn't found "
                    "suitable solution for user --> "
                 << *User << "\n";
          dbgs() << "Inst is --> " << *Instr << "\n";
          dbgs() << "Inst sols are --> \n";
          printInstrSols(Instr);
          dbgs() << "User Sols are --> \n";
          printInstrSols(User);
          if (isa<IntegerType>((*User).getType()) || isa<SwitchInst>(*User)) {
            dbgs() << "User should be solved is int type or switch.\n";
          } else {
            dbgs() << "User isn't int type or switch.\n";
            dbgs() << "User is --> " << *User << "\n";
          }

          if (AvailableSolutions[User].empty() == true &&
              PendingSols[User].empty() == true) {
            dbgs() << "This user has no "
                      "Solutions..........................!!!\n";
          }
          AllMatching = false;
          break;
          // NOTE: we didn't find any solution for that user.
        }
        // dbgs() << "After all calculations for user " << *User << "\nSols
        // are\n"; printInstrSolsDetailed(User);
        UsersVisited.insert(User);

      } // end userWorklist
      dbgs() << "--------!----!!---!--------All Matching is " << AllMatching
             << "Ops Matching " << MatchingForAllOps << "\n";
      if (AllMatching) {
        MatchingWidthForAllUsers = true;
      }
      if (AllMatching && MatchingForAllOps) {
        dbgs() << "Found a Combination..!\n";
        dbgs() << "For Inst --> " << *Instr << "\n";
        // TODO: some of the insertions are redudant.
        Combinations.push_back(CandidateSols);
        CurrentInstSol[Instr].push_back(InstSol);
        dbgs() << "Pushing Back Sol --> " << *InstSol << "\n";
        dbgs() << "Pushed inst sol now .\n";
        SolsPerInst[Instr].UserSols = UserSols;
        SolsPerInst[Instr].InstSols[Instr].push_back(InstSol);
      } else if (AllMatching == 0 || MatchingForAllOps == 0) {
        DiscardSolution = true;
        dbgs() << "AllMatching for Users is " << AllMatching << "\n";
        dbgs() << "MatchingForAllOps for Operands is " << MatchingForAllOps
               << "\n";
      }

      dbgs() << "v2Inst Sol kind Now is --> " << InstSol->getKind() << "\n";

    } // End Inst Solutions
    for (const auto &UserOrSols : PendingSols) {
      Value *U = UserOrSols.first;

      for (WideningIntegerSolutionInfo *Sol : UserOrSols.second) {
        AvailableSolutions[U].push_back(Sol);
      };
    }
    if (!MatchingWidthForAllUsers) {
      dbgs() << "Didn't find Solutions for all users for Instruction --> "
             << *Instr << "\n";
      dbgs() << "\nInst Solutions are --> \n";
      printInstrSols(Instr);
      dbgs() << "\nPrinting Solutions for all users now..\n";
      for (auto *User : usersEncountered) {
        dbgs() << "\nPrinting Solutions for User" << *User << "\n";
        printInstrSols(User);
      }
      assert(0);
    }
    if (Combinations.size() == 0) {
      dbgs() << "!!! IMPORTANT!!!!!!!!!!!!! Couldn't find user combination for "
                "Inst "
             << *Instr << "\n";
      dbgs() << "Inst Solutions are --> " << '\n';
      printInstrSols(Instr);
      dbgs() << "------------------------------------------------------------"
                "-\n";
      dbgs() << "------------------------------------------------------------"
                "-\n";
      dbgs() << "Users are " << "\n";
      dbgs() << "\nPrinting Solutions for all users now..\n";
      for (auto *User : usersEncountered) {
        dbgs() << "\nPrinting Solutions for User" << *User << "\n";
        printInstrSols(User);
      }
    }
    // std::vector<DenseMap<Value *, SolutionSet>> Combinations;
    // DenseMap<Value *, WideningIntegerSolutionInfo *> BestCombination;
    dbgs() << "Accessing current InstSol\n";
    assert(Combinations.size() >= 1 &&
           "Didn't find combination size bigger_eq than 1");
    auto BestInstSolIt = CurrentInstSol.find(Instr);
    if (BestInstSolIt == CurrentInstSol.end()) {
      assert(0 && "Didn't find best Inst Solution.\n");
    }
    SolutionSet BestInstSolVec = BestInstSolIt->second;
    assert(BestInstSolVec.empty() == false && "Found Empty best sols");

    BestInstSol = BestInstSolVec[0];
    dbgs() << "Completed current InstSol\n";
    for (auto i = 0U; i < Combinations.size(); i++) {
      const auto &denseMap = Combinations[i];
      for (const auto &entry : denseMap) {
        Value *value = entry.first;
        SolutionSet Solutions = entry.second;
        if (Solutions.size() == 0) {
          dbgs() << "Didn't find solutions for value " << *value
                 << "exiting..\n";
          assert(0);
        }
        const WideningIntegerSolutionInfo *bestSol = Solutions[0];
        for (const WideningIntegerSolutionInfo *Sol : Solutions) {
          if (bestSol->getCost() > Sol->getCost()) {
            bestSol = Sol;
            if (i >= BestInstSolVec.size()) {
              assert(0 && "Trying to access out of bounds BestInstSolVec");
            }
            BestInstSol = BestInstSolVec[i];
          }
        }
        dbgs() << "14For " << value << " best sol is " << bestSol << "\n";
        dbgs() << "14For " << *value << " best sol is " << *bestSol << "\n";
        dbgs() << "15 Inst that those combinations is " << Instr << "\n";
        dbgs() << "15 Inst that those combinations is " << *Instr << "\n";
        BestCombination.insert({value, bestSol});
        auto It = BestCombination.find(value);
        if (It == BestCombination.end()) {
          dbgs() << "Found empty Sol for --> " << value << "\n";
        }
        auto *gettedSol = It->second;
        if (gettedSol == nullptr) {
          dbgs() << "Found null Sol for --> " << value << "\n";
        }
        assert(gettedSol != nullptr);
        assert(It != BestCombination.end());
      }
    }
    BestUserOpsSolsPerInst.insert({Instr, BestCombination});
    dbgs() << "133For Instr " << Instr << " best Sol is " << BestInstSol
           << "\n";
    dbgs() << "47aPushed best sol for inst " << *Instr << "best sol "
           << *BestInstSol << "\n";
    BestSolPerInst.insert({Instr, BestInstSol});
    dbgs() << "------------------------------------------------------\n";
    usersEncountered.clear();
    InstsVisited.insert(Instr);
  }
}

// FIXME: v2 is only to calculate the best solution globally
// after we had finished finding all the solutions, and it is way more
// complex.
void WideningIntegerArithmetic::calcBestUserSols_v2(Function &F) {

  // struct InstSolutions {
  //   DenseMap<Instruction *, SolutionSet> InstSols;
  //   DenseMap<Value*, SolutionSet> OperandSols;
  //   DenseMap<Instruction*, SolutionSet> UserSols;
  // };

  DenseMap<Instruction *,
           DenseMap<const WideningIntegerSolutionInfo *, SolutionSet>>
      InstLegalSols;
  for (auto entry : SolsPerInst) {
    int cost = INT_MAX;
    struct InstSolutions InstructionSols = entry.second;
    Instruction *Inst = entry.first;

    for (const WideningIntegerSolutionInfo *InstSol :
         InstructionSols.InstSols[Inst]) {
      // for every operand
      // for every user
      for (auto userSolEntries : InstructionSols.UserSols) {
        SolutionSet UserSols = userSolEntries.second;
        for (auto UserSol : UserSols) {
          if (isLegalAndMatching(UserSol, InstSol)) {
            InstLegalSols[Inst][InstSol].push_back(UserSol);
          }
        }
      }
      for (auto opSolEntries : InstructionSols.OperandSols) {
        SolutionSet OpSols = opSolEntries.second;
        for (auto OpSol : OpSols) {
          if (isLegalAndMatching(OpSol, InstSol)) {
            InstLegalSols[Inst][InstSol].push_back(OpSol);
          }
        }
      }
    }
  }
  // TODO: apply DP recurrence DP[I][w] = ? + min(Sum(DP[User_i, w] for every
  // User of I ))
  for (auto entry : InstLegalSols) {
    Instruction *I = entry.first;
    int min_cost = INT_MAX;
    const WideningIntegerSolutionInfo *SelectedSol;
    DenseMap<Value *, const WideningIntegerSolutionInfo *> SelectedOpUserSols;
    for (auto LegalSolsEntry : entry.second) {
      SolutionSet OpAndUserSols = LegalSolsEntry.second;
      int total_cost = 0;
      const WideningIntegerSolutionInfo *InstSol = LegalSolsEntry.first;
      for (auto Sol : OpAndUserSols) {
        total_cost += Sol->getCost();
      }
      total_cost += InstSol->getCost();
      if (total_cost < min_cost) {
        SelectedSol = InstSol;
        for (auto OpOrUserSol : OpAndUserSols) {
          Value *V = OpOrUserSol->getValue();
          SelectedOpUserSols[V] = OpOrUserSol;
        }
        // TODO: do we need to keep all the users and ops sols?
        // They might need an extension but check if is already done.
      }
    }
  }
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

bool WideningIntegerArithmetic::visit_widening(Value *VInstr,
                                               std::queue<Value *> &Worklist) {
  SmallVector<WideningIntegerSolutionInfo *> EmptySols;
  Instruction *Instr = dyn_cast<Instruction>(VInstr);
  if (isSolved(VInstr)) {
    return true;
  }
  if (!Instr) {
    return false;
  }
  dbgs() << "Pushing to worklist " << *Instr << "\n";
  Worklist.push(VInstr);
  InsideWorklist[VInstr] = true;
  for (Value *V : Instr->operand_values()) {
    if (isSolved(V)) {
    } else if (!isInWorklist(V)) {
      dbgs() << "Visiting --> " << *V << "\n";
      visit_widening(V, Worklist);
    }
  }
  if (!Instr) {
    return false;
  }
  // is ConstantInt visited here?

  while (!Worklist.empty()) {
    Value *PopVal = Worklist.front();
    Worklist.pop();
    dbgs() << "visit_widening worklist size is " << Worklist.size() << "\n";

    // if (auto *I = dyn_cast<Instruction>(PopVal))
    //   // dbgs() << " Opc is " << OpcodesToStr[I->getOpcode()] << "\n";
    //   else {
    //     // dbgs() << "Not an instruction to print opc..\n";
    //   }
    bool Changed = visitInstruction(PopVal, Worklist);
    dbgs() << "Solved Inst --> " << *PopVal << "\n";
    dbgs() << "AvailableSols size is --> " << AvailableSolutions[PopVal].size()
           << "\n";
    Value *SuccessorPhi = getPhiSuccessor(PopVal);
    if ((SuccessorPhi != nullptr && SuccessorPhi != PopVal &&
         !isSolved(SuccessorPhi)) ||
        Changed) {
      // dbgs() << "Adding PopVal " << *PopVal << " back to worklist.." <<
      // "\n"; dbgs() << "Succesor Phi Sols Size is "
      //        << AvailableSolutions[SuccessorPhi].size() << "\n";
      Worklist.push(PopVal);
      // dbgs() << "Worklist size is " << Worklist.size() << "\n";
      //  visitPHI(SuccessorPhi); // TODO if PHI is not solved solve it here?
    } else {
      // dbgs() << "Solved Instruction " << *PopVal << "\n";
      SolvedInstructions[PopVal] = true;
      counter++;
      // dbgs() << "Solutions Size is --> " <<
      // AvailableSolutions[PopVal].size()
      //<< '\n';
      assert(AvailableSolutions[PopVal].size() > 0 &&
             "Empty Solutions on visit_widening");
      // printInstrSols(PopVal);
    }
  }
  return true;
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
  bool AddExtensions = true;
  for (auto *Inc : IncomingValues) {
    if (auto *IncI = dyn_cast<Instruction>(Inc)) {
      if (!isLegalToPromote(IncI) && !isa<ConstantInt>(Inc)) {
        AddExtensions = false;
      }
    }
    if (isSolved(Inc) || isInWorklist(Inc))
      continue;
    Worklist.push(Inc);
  }
  bool Changed = false;
  Value *SelectedValue = IncomingValues[0];
  SmallVector<Value *, 32> ValuesWithout = IncomingValues;
  ValuesWithout.erase(ValuesWithout.begin());
  auto *Vphi = dyn_cast<Value>(PhiInst);
  for (const WideningIntegerSolutionInfo *Sol :
       AvailableSolutions[SelectedValue]) {
    for (Value *Val2 : ValuesWithout) {
      for (const WideningIntegerSolutionInfo *Sol2 : AvailableSolutions[Val2]) {
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
  if (AddExtensions) {
    // TODO: we may come to this place many times. Add something
    // so we don't call addNonRedudant every single time if
    // we have already added the extensions.
    unsigned FillTypeWidth = getKnownFillTypeWidth(Instr);
    unsigned Width = Instr->getType()->getScalarSizeInBits();
    unsigned ExtensionOpc = getExtensionChoice(ExtensionChoice);
    for (auto IntegerSize : IntegerSizes) {
      if (IntegerSize > Width) {
        auto *Ext =
            new WIA_WIDEN(Instr->getOpcode(), ExtensionOpc, ExtensionChoice,
                          FillTypeWidth, Width, IntegerSize, 1, VInstr);
        dbgs() << "1444Created widen inst --> " << *Ext
               << "\ninside solveSimplePhis\n";
        for (const WideningIntegerSolutionInfo *Sol :
             AvailableSolutions[Instr]) {
          if (Sol->getKind() == WIAK_WIDEN)
            continue;
          dbgs() << "1443Adding operand --> " << *Sol << "For this extension "
                 << "n";
          Ext->addOperand(std::make_unique<WideningIntegerSolutionInfo>(*Sol));
        }
        if (addNonRedudant(AvailableSolutions[Instr], Ext)) {
          Changed = true;
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

  enum WIAKind Kind = WIAK_UNKNOWN;
  if (VI == nullptr) {
    dbgs() << "Cannot create Solution for a Null Value." << "\n";
    return false;
  }
  if (auto *CI = dyn_cast<ConstantInt>(VI)) {
    return visitCONSTANT(CI);
  }
  unsigned Opcode = 0, NewOpcode = 0;
  unsigned OldWidth = 0, UpdatedWidth = 0;
  if (auto *Instr = dyn_cast<Instruction>(VI)) {
    Opcode = Instr->getOpcode();
    if (IsBinop(Opcode)) {
      Kind = WIAK_BINOP;
    } else if (IsICMP(Opcode)) {
      Kind = WIAK_ICMP;
    } else if (IsUnop(Opcode)) {
      Kind = WIAK_UNOP;
    } else if (IsLoad(Opcode)) {
      Kind = WIAK_LOAD;
    } else if (IsStore(Opcode)) {
      Kind = WIAK_STORE;
    } else if (IsExtension(Opcode)) {
      Kind = WIAK_DROP_EXT;
      OldWidth = Instr->getType()->getScalarSizeInBits();
      Value *N0 = Instr->getOperand(0);
      if (auto *I0 = dyn_cast<Instruction>(N0)) {
        NewOpcode = I0->getOpcode();
      }
      UpdatedWidth = N0->getType()->getScalarSizeInBits();
    } else if (IsTruncate(Opcode)) {
      Value *N0 = Instr->getOperand(0);
      UpdatedWidth = N0->getType()->getScalarSizeInBits();
      OldWidth = Instr->getType()->getScalarSizeInBits();
      if (auto *I0 = dyn_cast<Instruction>(N0)) {
        NewOpcode = I0->getOpcode();
      }
      Kind = WIAK_DROP_LOCOPY;
    } else if (IsPHI(Opcode)) {
      Kind = WIAK_PHI;
    } else if (IsStore(Opcode)) {
      Kind = WIAK_STORE;
    } else if (IsSwitch(Opcode)) {
      Kind = WIAK_SWITCH;
    } else { // TODO: ADD others too.
      Kind = WIAK_UNKNOWN;
    }
  } else {
    Kind = WIAK_UNKNOWN;
  }
  unsigned short FillTypeWidth, InstrWidth;
  if (auto *SI = dyn_cast<SwitchInst>(VI)) {
    FillTypeWidth = getKnownFillTypeWidth(SI->getCondition());
    InstrWidth = SI->getType()->getScalarSizeInBits();
  } else if (auto *BRI = dyn_cast<BranchInst>(VI)) {
    if (BRI->isConditional()) {
      Value *Cond = BRI->getCondition();
      InstrWidth = Cond->getType()->getScalarSizeInBits();
      FillTypeWidth = InstrWidth;
    }
    FillTypeWidth = 1;
    InstrWidth = 1;
  } else {
    FillTypeWidth = getKnownFillTypeWidth(VI);
    InstrWidth = VI->getType()->getScalarSizeInBits();
  }
  if (Kind == WIAK_UNKNOWN) {
    Opcode = UNKNOWN_OPC;
  }
  dbgs() << "Creating Default Sol for Opc " << OpcodesToStr[Opcode] << "\n";
  OldWidth = InstrWidth;
  // Can we truncate a constant?
  if (NewOpcode == 0) {
    NewOpcode = Opcode;
  } else {
    dbgs() << "Found new Opcode --> " << OpcodesToStr[NewOpcode] << "n";
  }
  // assert(AvailableSolutions[VI].size() == 0 && "Sols are not empty..\n");
  UpdatedWidth = UpdatedWidth == 0 ? InstrWidth : UpdatedWidth;
  if (auto *Icmp = dyn_cast<ICmpInst>(VI)) {
    auto Op0Width = getIntegerFromType(Icmp->getOperand(0)->getType());
    unsigned IcmpWidth = Op0Width;
    WIA_ICMP *IcmpSol;
    IcmpSol = new WIA_ICMP(Opcode, NewOpcode, ExtensionChoice, FillTypeWidth,
                           OldWidth, UpdatedWidth, 0, VI);
    dbgs() << "174Setting IcmpWidth to --> " << IcmpWidth << "\n";
    IcmpSol->setUpdatedIcmpWidth(IcmpWidth);
    dbgs() << "Created Icmp Default Sol .. " << *IcmpSol << "\n";
    AvailableSolutions[VI].push_back(IcmpSol);
  } else {
    dbgs() << "Trying to create default sol with kind " << Kind << "\n";
    dbgs() << "Trying to create default sol with opc " << OpcodesToStr[Opcode]
           << "\n";
    dbgs() << "Trying to create default sol with new opc "
           << OpcodesToStr[NewOpcode] << "\n";
    dbgs() << "Trying to create default sol with ext choice " << ExtensionChoice
           << "\n";
    dbgs() << "Trying to create default sol with filltypewidth "
           << FillTypeWidth << "\n";
    dbgs() << "Trying to create default sol with oldWidth " << OldWidth << "\n";
    dbgs() << "Trying to create default sol with UpdatedWidth " << UpdatedWidth
           << "\n";
    dbgs() << "Trying to create default sol with Val " << *VI << "\n";
    auto *DefaultSol = new WideningIntegerSolutionInfo(
        Opcode, NewOpcode, ExtensionChoice, FillTypeWidth, OldWidth,
        UpdatedWidth, 0, Kind, VI);
    AvailableSolutions[VI].push_back(DefaultSol);
    dbgs() << "Created Default Sol .. " << *DefaultSol << "\n";
  }
  dbgs() << "Kind is --> " << Kind << "\n";
  // createDefaultSol is called only when we have emptySols so we don't need
  // to call addNonRedundant.
  if (Kind == WIAK_DROP_LOCOPY) {
    auto *DropLoIgnore =
        new WIA_DROP_LOIGNORE(Opcode, Opcode, ExtensionChoice, FillTypeWidth,
                              OldWidth, UpdatedWidth, 0, VI);
    AvailableSolutions[VI].push_back(DropLoIgnore);
  }
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
// TODO: refactor to handle ICmp Insts too.
// ICMP insts have two widths the return width and the icmp width.
bool WideningIntegerArithmetic::combineSols(Instruction *Instr,
                                            std::queue<Value *> &Worklist) {

  dbgs() << "Getting value\n";
  Value *VInstr = dyn_cast<Value>(Instr);
  dbgs() << "Getting Opcode\n";
  unsigned Opcode = Instr->getOpcode();
  dbgs() << "Getting FillTypes\n";
  FillTypeSet OperandFillTypes = getOperandFillTypes(Instr);
  dbgs() << "Getting operand 0\n";
  Value *N0 = Instr->getOperand(0);
  dbgs() << "Getting operand 1\n";
  Value *N1 = Instr->getOperand(1);
  dbgs() << "Op0 is --> " << *N0 << "\n";
  dbgs() << "Op1 is --> " << *N1 << "\n";
  if (isa<ICmpInst>(Instr)) {
    dbgs() << "Dumping ICmpInst --> " << *Instr << "\n";

    dbgs() << "\nfor icmp Int type is " << getIntegerFromType(Instr->getType())
           << "\n";
  }
  dbgs() << "OperandFillTypes size is --> " << OperandFillTypes.size() << "\n";
  if (OperandFillTypes.size() > 0) {
    IntegerFillType icmpFill = std::get<2>(*OperandFillTypes.begin());
    dbgs() << "Icmp fill is --> " << icmpFill << "\n";
  }
  bool Changed = false;
  // dbgs() << "Checking Binop for Opcode " << OpcodesToStr[Opcode] << "\n";
  // A function that combines solutions from operands left and right
  for (const WideningIntegerSolutionInfo *leftSolution :
       AvailableSolutions[Instr->getOperand(0)]) {
    for (const WideningIntegerSolutionInfo *rightSolution :
         AvailableSolutions[Instr->getOperand(1)]) {
      // Test whether current binary operator has the available
      // fill type provided by leftSolution and rightSolution
      auto LeftFill = leftSolution->getFillType();
      auto RightFill = rightSolution->getFillType();
      // dbgs() << "Left Fill type is " << LeftFill << "\n";
      // dbgs() << "right Fill type is " << RightFill << "\n";
      auto FillType = getOrNullFillType(Instr, OperandFillTypes,
                                        leftSolution->getFillType(),
                                        rightSolution->getFillType());
      if (FillType == UNDEFINED) {
        //  dbgs() << "Fill type is undefined skipping solution" << "\n";
        //  dbgs() << "Left Solution --> " << *leftSolution << '\n';
        //  dbgs() << "Right Solution --> " << *rightSolution << '\n';
        continue;
      }
      dbgs() << "fillType is " << FillType << "\n";
      // dbgs() << "Passed FillType for combination --> " << FillType << "\n";
      unsigned int w1 = leftSolution->getUpdatedWidth();
      unsigned int w2 = rightSolution->getUpdatedWidth();
      if (w1 != w2) {
        LLVM_DEBUG(dbgs() << "Width " << w1 << "and Width " << w2
                          << " are not the same skipping solution.." << "\n");
        dbgs() << "Width " << w1 << "and Width " << w2
               << " are not the same skipping solution.." << "\n";
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
      // dbgs() << "The widths are the same and we continue--> " << w1 <<
      // "\n";
      unsigned int UpdatedWidth = w1;
      if (isa<ICmpInst>(Instr)) {
        // TODO: check if we must anything else for ICMP.
      }
      EVT NewVT = EVT::getIntegerVT(*Ctx, UpdatedWidth);
      // TODO: with ICMP inst this always fails because the NewVT should be
      // always 1. In our case we need to change the type of the ICMP not the
      // result type.
      if (!TLI->isOperationLegal(TLI->InstructionOpcodeToISD(Opcode), NewVT) &&
          !isa<ICmpInst>(Instr)) {
        dbgs() << "Operation is not legal.." << "with updatedWidth is "
               << UpdatedWidth << "\n";
        continue;
      }
      // dbgs() << "The Operation is legal for that newVT--> " << w1 << "\n";
      //  w1 x w2 --> w all widths are the same at this point
      // and there is a LegalOperation for that Opcode

      unsigned int Cost = leftSolution->getCost() + rightSolution->getCost();
      unsigned int FillTypeWidth = getKnownFillTypeWidth(Instr);
      // if this is a Target of this Binary operator of the form
      // width1 x width2 = newWidth
      // for example on rv64 we have addw
      // that is of the form i32 x i32 -> i32
      // and stored in a sign extended format to i64
      if (!isa<ICmpInst>(Instr)) {
        auto *Sol = new WIA_BINOP(Opcode, Opcode, FillType, FillTypeWidth, w1,
                                  UpdatedWidth, Cost, Instr);
        Sol->addOperand(
            std::make_unique<WideningIntegerSolutionInfo>(*leftSolution));
        Sol->addOperand(
            std::make_unique<WideningIntegerSolutionInfo>(*rightSolution));
        if (addNonRedudant(AvailableSolutions[Instr], Sol)) {
          Changed = true;
        }
      } else {
        auto *Sol = new WIA_ICMP(Opcode, Opcode, FillType, FillTypeWidth, 1, 1,
                                 Cost, Instr);
        dbgs() << "175Setting IcmpWidth to --> " << UpdatedWidth << "\n";
        Sol->setUpdatedIcmpWidth(UpdatedWidth);
        Sol->addOperand(
            std::make_unique<WideningIntegerSolutionInfo>(*leftSolution));
        Sol->addOperand(
            std::make_unique<WideningIntegerSolutionInfo>(*rightSolution));
        LLVM_DEBUG(dbgs() << "Adding Sol with cost " << Cost << " and width "
                          << UpdatedWidth << "\n");
        if (isa<ICmpInst>(Instr)) {
          dbgs() << "BINOP Adding Sol with cost " << Cost << " and width "
                 << UpdatedWidth << " and old width " << w1 << "\n";
        }
        // dbgs() << "Created Sol Binop for Opcode " << OpcodesToStr[Opcode]
        // <<
        // "\n";
        //  dbgs() << "BINOP Adding Sol with cost " << Cost << " and width "
        //       << UpdatedWidth << "\n";
      }
    }
  }
  assert(AvailableSolutions[Instr].size() > 0);
  // if (isa<ICmpInst>(Binop)) {
  //   dbgs() << "Printing inst sols for icmp\n";
  //   printInstrSols(Binop);
  // }
  dbgs() << "Calling closure..\n";
  Changed = closure(Instr, Worklist);
  if (isa<ICmpInst>(Instr)) {
    ICmpInsts.push_back(Instr);
    dbgs() << "printing sols for " << *Instr << "\n";
    for (auto *Sol : AvailableSolutions[Instr]) {
      dbgs() << "Printing ICMP Sol --> " << Sol << "\n" << *Sol << "\n";
    }
  }
  return Changed;
}

bool WideningIntegerArithmetic::visitSWITCH(Instruction *Instr,
                                            std::queue<Value *> &Worklist) {
  SwitchInst *SI = dyn_cast<SwitchInst>(Instr);
  Value *Condition = SI->getCondition();
  if (AvailableSolutions[Condition].size() <= 0) {
    Worklist.push(Condition);
    // We need to solve the Condition first and then the Switch
    return true;
  }
  unsigned Opcode = SI->getOpcode();
  for (auto *CondSol : AvailableSolutions[Condition]) {
    for (auto Case : SI->cases()) {
      auto *CI = Case.getCaseValue();
      auto *Sol =
          new WIA_CONSTANT(CONSTANT_INT_OPC, CONSTANT_INT_OPC, ExtensionChoice,
                           getKnownFillTypeWidth(CI), CI->getBitWidth(),
                           CondSol->getUpdatedWidth(), 0, CI);
      dbgs() << "Created Constant Sol for switch --> " << *Sol << "\n";
      AvailableSolutions[CI].push_back(Sol);
    }
    auto *SISol =
        new WIA_SWITCH(Opcode, Opcode, ExtensionChoice,
                       CondSol->getFillTypeWidth(), CondSol->getWidth(),
                       CondSol->getUpdatedWidth(), CondSol->getCost(), SI);
    SISol->addOperand(std::make_unique<WideningIntegerSolutionInfo>(*CondSol));

    AvailableSolutions[SI].push_back(SISol);
  }
  return false;
}

bool WideningIntegerArithmetic::visitICMP(Instruction *Icmp,
                                          std::queue<Value *> &Worklist) {

  Value *V0 = Icmp->getOperand(0);
  Value *V1 = Icmp->getOperand(1);

  auto LeftSols = AvailableSolutions[V0];
  auto RightSols = AvailableSolutions[V1];
  if (LeftSols.size() <= 0) {
    Worklist.push(V0);
    bool AddedLeft = createDefaultSol(V0);
    dbgs() << "Icmp Warning!!!! Found empty left Sols on ICMP and added "
              "default is "
           << AddedLeft << "\n";
    return true;
  }
  if (RightSols.size() <= 0) {
    Worklist.push(V1);
    dbgs() << "Trying to create default sol for " << *V1 << "\n";
    dbgs() << "Inst is " << *Icmp << "\n";
    bool AddedRight = createDefaultSol(V1);
    dbgs() << "ICMP Warning!!!! Found empty right Sols on Binop and added "
              "default is "
           << AddedRight << "\n";
    return true;
  }
  dbgs() << "Combining Sols for ICMP..\n";
  return combineSols(Icmp, Worklist);
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
              // call closure here to get all the LowWidths that are
              // available.
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
    dbgs() << "BINOP Warning!!!! Found empty left Sols on Binop and added "
              "default is "
           << AddedLeft << "\n";
    return true;
  }
  if (RightSols.size() <= 0) {
    Worklist.push(V1);
    dbgs() << "Trying to create default sol for " << *V1 << "\n";
    dbgs() << "Inst is " << *Binop << "\n";
    bool AddedRight = createDefaultSol(V1);
    dbgs() << "BINOP Warning!!!! Found empty right Sols on Binop and added "
              "default is "
           << AddedRight << "\n";
  }
  dbgs() << "Calling Combine Binops.. for Opc "
         << OpcodesToStr[Binop->getOpcode()] << "\n";
  return combineSols(Binop, Worklist);
}

std::list<WideningIntegerSolutionInfo *>
WideningIntegerArithmetic::visitFILL(Instruction *Instr,
                                     std::queue<Value *> &Worklist) {

  dbgs() << "getting fill width...\n";
  unsigned char FillTypeWidth = getKnownFillTypeWidth(Instr);
  Value *VInstr = dyn_cast<Value>(Instr);

  dbgs() << "getting sols ...\n";
  SmallVector<WideningIntegerSolutionInfo *, 8> Sols =
      AvailableSolutions[VInstr];
  std::list<WideningIntegerSolutionInfo *> Solutions;
  if (Sols.size() <= 0) {
    Worklist.push(VInstr);
    createDefaultSol(VInstr);
  }
  for (const WideningIntegerSolutionInfo *Sol : Sols) {
    unsigned SolOpc = Sol->getOpcode();
    if (SolOpc == Instruction::SExt || llvm::ISD::isExtOpcode(SolOpc) ||
        SolOpc == Instruction::Trunc || SolOpc == Instruction::ZExt ||
        Sol->getKind() == WIAK_DROP_EXT ||
        Sol->getNewOpcode() == FILL_INST_OPC || Sol->getKind() == WIAK_WIDEN)
      continue; // FILL is a ISD::SIGN_EXTEND_INREG
    // WIA_FILL extends the least significant *Width* bits of Instr
    // to targetWidth
    for (auto IntegerSize : IntegerSizes) {
      EVT NewVT = EVT::getIntegerVT(*Ctx, IntegerSize);
      // TODO add isTypeLegal for the NewVT??
      // meaning is TLi.isOperationLegal enough for the NewVT?
      // FIXME: check correctness.
      dbgs() << "checking isd opcodes  from solopc is --> " << SolOpc
             << "...\n";
      dbgs() << "Sol is --> " << *Sol << "\n";
      dbgs() << "Instr is --> " << *Instr << "\n";
      if (IntegerSize < FillTypeWidth || IntegerSize < (int)Sol->getWidth() ||
          TLI->InstructionOpcodeToISD(SolOpc) == ISD::TRUNCATE ||
          !TLI->isOperationLegal(ISD::SIGN_EXTEND_INREG, NewVT)) {
        continue;
      }
      WideningIntegerSolutionInfo *Fill =
          new WIA_FILL(SolOpc, FILL_INST_OPC, ExtensionChoice, FillTypeWidth,
                       FillTypeWidth, IntegerSize, Sol->getCost() + 1, VInstr);
      dbgs() << "visitFILL--> Created Fill --> " << *Fill << "\n";
      Fill->addOperand(std::make_unique<WideningIntegerSolutionInfo>(*Sol));
      dbgs() << "Added operand --> " << *Sol << "\n";

      Solutions.push_front(Fill);
    }
  }
  return Solutions;
}

inline Type *WideningIntegerArithmetic::getTypeFromInteger(int Integer) {
  Type *Ty;
  switch (Integer) {
  case 1:
    Ty = Type::getInt1Ty(*Ctx);
    break;
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

inline int WideningIntegerArithmetic::getIntegerFromType(Type *Ty) {
  if (auto *IntTy = dyn_cast<IntegerType>(Ty)) {
    switch (IntTy->getBitWidth()) {
    case 1:
      return 1;
    case 2:
      return 2;
    case 3:
      return 3;
    case 4:
      return 4;
    case 5:
      return 5;
    case 6:
      return 6;
    case 7:
      return 7;
    case 8:
      return 8;
    case 16:
      return 16;
    case 32:
      return 32;
    case 64:
      return 64;
    default:
      return 0;
    }
  } else {
    assert(0 && "Called getIntegerFromType that is not IntTy");
  }
}

inline unsigned
WideningIntegerArithmetic::getExtCost(Instruction *Instr,
                                      const WideningIntegerSolutionInfo *Sol,
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
  SmallVector<WideningIntegerSolutionInfo *, 8> Sols =
      AvailableSolutions[VInstr];
  if (Sols.size() <= 0) {
    Worklist.push(VInstr);
    createDefaultSol(VInstr);
  }
  LLVM_DEBUG(dbgs() << "SolutionsSize is --> " << Sols.size() << '\n');
  std::list<WideningIntegerSolutionInfo *> Solutions;
  if (!isLegalToPromote(Instr)) {
    return Solutions;
  }
  for (const WideningIntegerSolutionInfo *Sol : Sols) {
    LLVM_DEBUG(dbgs() << "Inside visitWiden Iterating Sol --> " << Sol << '\n');
    unsigned SolOpc = Sol->getOpcode();
    if (SolOpc == FILL_INST_OPC || Sol->getNewOpcode() == FILL_INST_OPC)
      continue;

    unsigned IsdOpc = TLI->InstructionOpcodeToISD(SolOpc);
    if (llvm::ISD::isExtOpcode(Sol->getOpcode()) ||
        IsdOpc == ISD::SIGN_EXTEND_INREG || IsdOpc == ISD::TRUNCATE ||
        Sol->getKind() == WIAK_WIDEN)
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
          new WIA_WIDEN(SolOpc, ExtensionOpc, ExtensionChoice, FillTypeWidth,
                        Width, IntegerSize, cost, VInstr);
      Widen->addOperand(std::make_unique<WideningIntegerSolutionInfo>(*Sol));
      dbgs() << "Created widen inst --> " << *Widen
             << "\ninside visitWiden for Opc --> " << OpcodesToStr[SolOpc]
             << "\n";
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
  for (const WideningIntegerSolutionInfo *Sol : Sols) {
    unsigned SolOpc = Sol->getOpcode();
    if (SolOpc == FILL_INST_OPC || Sol->getNewOpcode() == FILL_INST_OPC)
      continue;
    unsigned IsdOpc = TLI->InstructionOpcodeToISD(SolOpc);
    if (llvm::ISD::isExtOpcode(Sol->getOpcode()) ||
        IsdOpc == ISD::SIGN_EXTEND_INREG || IsdOpc == ISD::TRUNCATE ||
        Sol->getKind() == WIAK_WIDEN)
      continue;
    for (int IntegerSize : IntegerSizes) {
      EVT NewVT = EVT::getIntegerVT(*Ctx, IntegerSize);
      if (IntegerSize < FillTypeWidth || IntegerSize < Sol->getWidth() ||
          !TLI->isOperationLegal(ISD::ANY_EXTEND, NewVT)) {
        continue;
      }
      unsigned cost = getExtCost(Instr, Sol, IntegerSize);

      WideningIntegerSolutionInfo *GarbageWiden =
          new WIA_WIDEN(SolOpc, ExtensionOpc, ANYTHING, FillTypeWidth,
                        Sol->getWidth(), IntegerSize, cost, VInstr);
      GarbageWiden->addOperand(
          std::make_unique<WideningIntegerSolutionInfo>(*Sol));
      dbgs() << "Created garbage widen inst --> " << *GarbageWiden
             << "\ninside visitWiden for Opc --> " << OpcodesToStr[SolOpc]
             << "\n";
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
    if (SolOpc == FILL_INST_OPC || Sol->getNewOpcode() == FILL_INST_OPC)
      continue;
    unsigned IsdOpc = TLI->InstructionOpcodeToISD(SolOpc);
    if (llvm::ISD::isExtOpcode(IsdOpc) || IsdOpc == ISD::SIGN_EXTEND_INREG ||
        IsdOpc == ISD::TRUNCATE)
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
          SolOpc, ExtensionOpc,
          ExtensionChoice, // Will depend on available Narrowing ,
          FillTypeWidth, Width, IntegerSize, Sol->getCost() + 1, VInstr);
      Trunc->addOperand(std::make_unique<WideningIntegerSolutionInfo>(*Sol));
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
  // LLVM_DEBUG(dbgs() << "ExtendedWidth of Node is " << ExtendedWidth <<
  // '\n'); LLVM_DEBUG(dbgs() << "Opc of Node is " << Instr->getOpcode()
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
  SmallVector<WideningIntegerSolutionInfo *, 8> ExprSolutions =
      AvailableSolutions[N0];
  for (auto *Solution : ExprSolutions) {
    if (Solution->getKind() == WIAK_DROP_EXT ||
        Solution->getKind() == WIAK_WIDEN ||
        Solution->getNewOpcode() == FILL_INST_OPC) {
      continue;
    }
    // We simply drop the extension and we will later see if it's needed.
    LLVM_DEBUG(dbgs() << "Drop extension in Solutions" << '\n');
    unsigned char FillTypeWidth = Solution->getFillTypeWidth();
    if (FillTypeWidth > OldWidth) {
      // TODO: check this. should be >= or > ??
      continue;
    }
    dbgs() << "For123 instr --> " << *Instr << "\n";
    WideningIntegerSolutionInfo *Expr = new WIA_DROP_EXT(
        Instr->getOpcode(), Solution->getNewOpcode(), ExtensionChoice,
        FillTypeWidth, ExtendedWidth /*OldWidth*/, OldWidth /*NewWidth*/,
        Solution->getCost(), Instr);
    dbgs() << "DROP_EXT Created Sol --> " << *Expr << "\n";
    dbgs() << "kind is --> " << Expr->getKind() << "\n";
    dbgs() << "And Op Sol is " << *Solution << "\n";
    // we must retain the widen sol so we add a no op.
    // TODO check OldWidth must come from The operand of the Extension
    // or from The Solutions? PRobably thes solutions.
    Expr->setOperands(Solution->getOperands());
    dbgs() << "Trying to add Sol " << *Expr << "\n";
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
  unsigned char FillTypeWidth = getKnownFillTypeWidth(Instr);
  WideningIntegerSolutionInfo *WidenSol =
      new WIA_NOOP(Instr->getOpcode(), Instr->getOpcode(), ExtensionChoice,
                   FillTypeWidth, ExtendedWidth, ExtendedWidth, 0, Instr);
  dbgs() << "v123 Created no op for zext or sext --> " << *WidenSol << "\n";
  AvailableSolutions[VInstr].push_back(WidenSol);
  return Changed;
}

void WideningIntegerArithmetic::updateSwitchLabels(Instruction *I,
                                                   Type *OldType,
                                                   Type *NewType) {
  if (auto *SI = dyn_cast<SwitchInst>(I)) {
    if (NewType != OldType) {
      for (auto &Case : SI->cases()) {
        ConstantInt *CaseValue = Case.getCaseValue();
        auto *IntegerTy = IntegerType::get(*Ctx, NewType->getIntegerBitWidth());
        ConstantInt *NewCaseValue =
            ConstantInt::get(IntegerTy, CaseValue->getZExtValue());
        Case.setValue(NewCaseValue);
      }
    }
  }
}

bool WideningIntegerArithmetic::applyPHI(Instruction *I,
                                         WideningIntegerSolutionInfo *Sol) {
  auto OldWidth = getIntegerFromType(I->getType());
  auto NewWidth = Sol->getUpdatedWidth();
  Type *NewTy = getTypeFromInteger(NewWidth);
  dbgs() << "12Printing Sol -->>> " << Sol << "\n";
  dbgs() << "12Printing Sol -->>> " << *Sol << "\n";
  dbgs() << "Mutating Type on --> " << *I << "\n";
  dbgs() << "From " << *(I->getType()) << "to " << *NewTy << "\n";
  // TODO: If we mutate the type here the operands have the old type.
  // We need to update them too to the type of the parent and so on.
  dbgs() << "Checking phis NOW!\n";
  I->mutateType(getTypeFromInteger(Sol->getUpdatedWidth()));
  auto *PhiInst = dyn_cast<PHINode>(I);
  int NumIncValues = PhiInst->getNumIncomingValues();
  for (int i = 0; i < NumIncValues; ++i) {
    Value *V = PhiInst->getIncomingValue(i);
    if (auto *I = dyn_cast<Instruction>(V)) {
      auto SolIt = BestSolPerInst.find(I);
      if (SolIt != BestSolPerInst.end()) {
        const WideningIntegerSolutionInfo *IncSol = SolIt->second;
        // assert(IncSol->getUpdatedWidth() == NewWidth);
        I->mutateType(getTypeFromInteger(NewWidth));
        // TODO: is it enough?
        // What about uses and ops?
      }
      // applyChain(I);
    } else if (auto *CI = dyn_cast<ConstantInt>(V)) {
      int FoundWidth = 0;
      for (auto *CSol : AvailableSolutions[CI]) {
        if (CSol->getUpdatedWidth() == NewWidth) {
          CI->mutateType(getTypeFromInteger(NewWidth));
        }
      }
      // assert(FoundWidth &&
      //      "Didn't found matching width on constant on PHiNode..\n");
    }
  }

  AppliedSols.insert(I);
  return false;
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
  Type *NewType = To->getType();
  Type *OldType = From->getType();
  int newWidth = getIntegerFromType(NewType);
  for (auto *U : Users) {
    bool DoReplace = true;
    updateSwitchLabels(U, OldType, NewType);
    if (isa<PHINode>(U)) {
      auto SolsIt = AvailableSolutions.find(U);
      if (SolsIt == AvailableSolutions.end()) {
        assert(0 && "User should have solutions.\n");
      }
      auto Sols = SolsIt->getSecond();
      auto *SolIt = std::find_if(Sols.begin(), Sols.end(),
                                 [&](const WideningIntegerSolutionInfo *Sol) {
                                   return Sol->getUpdatedWidth() == newWidth;
                                 });
      if (SolIt == Sols.end()) {
        DoReplace = false;
      }
      WideningIntegerSolutionInfo *NewSol = *SolIt;
      applyPHI(U, NewSol);
    }
    if (DoReplace)
      U->replaceUsesOfWith(From, To);
    else
      ReplacedAll = false;
  }

  if (ReplacedAll)
    if (auto *I = dyn_cast<Instruction>(From))
      InstsToRemove.insert(I);
}

bool WideningIntegerArithmetic::applySingleSol(
    Value *V, const WideningIntegerSolutionInfo *Sol) {

  // TODO: for icmp find how to change the icmp ne i16 to icmp ne i8 and so
  // on.
  // FIXME: when applying WIAK_DROP_EXT and then we apply WIAK_WIDEN
  // the ext type might change from zext to sext or vice verca
  dbgs() << "123456 Applying Sol for Value " << *V << "\n";
  if (AppliedSols.contains(V) || isa<BranchInst>(V)) {
    dbgs() << "We have already applied a Solution for this Value " << *V
           << "\n";
    return false;
  }
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
        NewInsts.insert(I);
      }
      assert((getIntegerFromType(ZExt->getType()) >
              getIntegerFromType(V->getType())) &&
             "calling SExt but the type of Sext is not bigger than the Value "
             "applied.\n");
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
        NewInsts.insert(I);
      }
      assert((getIntegerFromType(SExt->getType()) >
              getIntegerFromType(V->getType())) &&
             "calling SExt but the type of Sext is not bigger than the Value "
             "applied.\n");
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
    if (Trunc)
      NewInsts.insert(Trunc);
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
      if (isa<Argument>(V)) {
        I->moveBefore(InsertPt);
      } else {
        dbgs() << "Moving after..!\n";
        I->moveAfter(InsertPt);
        dbgs() << "Moved after..!\n";
      }
      NewInsts.insert(I);
    }
    dbgs() << "Replacing users..\n";
    assert(SExt != nullptr);
    dbgs() << "Truncated is " << *Truncated << "\n";
    dbgs() << "Truncated Ty is " << *(Truncated->getType()) << "\n";
    dbgs() << "B Ty is " << *(B) << "\n";
    dbgs() << "SExtType is --> " << getIntegerFromType(SExt->getType()) << "\n";
    dbgs() << "Value is " << *V << "\n";
    dbgs() << "SExt is " << *SExt << "\n";
    dbgs() << "SExtType is --> " << getIntegerFromType(SExt->getType()) << "\n";
    dbgs() << "V Type is " << getIntegerFromType(V->getType()) << "\n";
    dbgs() << "V type str is " << *(V->getType()) << "\n";
    assert((getIntegerFromType(SExt->getType()) >
            getIntegerFromType(Truncated->getType())) &&
           "calling SExt but the type of Sext is not bigger than the Truncated "
           "applied.\n");
    replaceAllUsersOfWith(V, SExt);
  };

  WIAKind SolKind = Sol->getKind();
  if (SolKind == llvm::WIAK_UNKNOWN) {
    dbgs() << "Found unknown sol not applying it for " << *V << ".\n";
    dbgs() << "Sol is --> " << *Sol << "\n";
    dbgs() << "Printing All Sols for " << *V << "\n";
    printInstrSols(V);
    return false;
  }
  Type *NewTy = getTypeFromInteger(Sol->getUpdatedWidth());
  switch (SolKind) {
  default: {
    if (auto *SI = dyn_cast<SwitchInst>(V)) {
      Value *Cond = SI->getCondition();
      dbgs() << "Found Switch!! Cond is --> " << *Cond << "\n";
      if (NewInsts.contains(Cond)) {
        dbgs() << "Cond " << *Cond << "is a new Instruction..\n";
        return false;
      }
      auto BestSolIt = BestSolPerInst.find(Cond);
      if (BestSolIt == BestSolPerInst.end()) {
        return false;
      }
      auto *BestSol = BestSolIt->second;
      Type *NewType = getTypeFromInteger(BestSol->getUpdatedWidth());
      dbgs() << "Old Type is --> " << *(Cond->getType())
             << " While new type is " << *NewType << "\n";
      if (Cond->getType() != NewType) {
        Cond->mutateType(NewType);
        SI->setCondition(Cond);
        dbgs() << "Setting Switch Cond..\n";
        // TODO: If we mutate the cond type we need to mutate all the labels
        // to the same type
        for (auto &Case : SI->cases()) {
          ConstantInt *CaseValue = Case.getCaseValue();
          auto *IntegerTy =
              IntegerType::get(*Ctx, NewType->getIntegerBitWidth());
          dbgs() << "IntegerType for switch is --> " << *IntegerTy << "\n";
          ConstantInt *NewCaseValue =
              ConstantInt::get(IntegerTy, CaseValue->getZExtValue());
          Case.setValue(NewCaseValue);
        }
      }
    } else {
      auto OldWidth = getIntegerFromType(V->getType());
      auto NewWidth = Sol->getUpdatedWidth();
      if (OldWidth == NewWidth) {
        break;
      }
      Type *NewTy = getTypeFromInteger(NewWidth);
      dbgs() << "Printing sol !!->>>" << Sol << "\n";
      dbgs() << "Printing sol !!->>>" << *Sol << "\n";
      dbgs() << "Mutating Type on --> " << *V << "\n";
      dbgs() << "From " << *(V->getType()) << "to " << *NewTy << "\n";
      dbgs() << "V now is " << *V << "\n";

      // TODO: If we mutate the type here the operands have the old type.
      // We need to update them too to the type of the parent and so on.

      V->mutateType(getTypeFromInteger(Sol->getUpdatedWidth()));
      AppliedSols.insert(V);
      dbgs() << "After mutating type.. Printing users now! " << "\n";
      for (auto &U : V->uses()) {
        auto User = U.getUser();
        if (auto *InstU = dyn_cast<Instruction>(User)) {
          dbgs() << "User is now --> " << *InstU << "\n";
        }
      }
      break;
    }
  }
  case WIAK_NOOP:
    return false;
    break;
  case WIAK_WIDEN:
  case WIAK_WIDEN_GARBAGE: {
    if (auto *Instr = dyn_cast<Instruction>(V)) {
      Instruction *InsertPt = Instr;
      if (auto *Arg = dyn_cast<Argument>(V)) {
        BasicBlock &BB = Arg->getParent()->front();
        InsertPt = &*BB.getFirstInsertionPt();
      }
      InsertExt(V, InsertPt, NewTy);
      Promoted.insert(Instr);
      AppliedSols.insert(V);
    } else {
      dbgs() << "Called WIAK_WIDEN on something that is not an Instruction\n";
    }
    break;
  }
  case WIAK_NARROW: {
    // TODO: needs fixes not complete, switch is diff stores are diff
    // and maybe more.
    Instruction *Instr;
    if (isa<Instruction>(V))
      Instr = dyn_cast<Instruction>(V);
    else {
      dbgs() << "Called WIAK_NARROW on something that is not an Instruction\n";
    }
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
          dbgs() << "Switch Setting Cond..\n";
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
    AppliedSols.insert(V);
    break;
  }
  case WIAK_EXTLO: {
    if (auto *Instr = dyn_cast<Instruction>(V)) {
      Type *TruncTy = getTypeFromInteger(Sol->getWidth());
      Type *ExtTy = getTypeFromInteger(Sol->getUpdatedWidth());
      dbgs() << "Calling InsertSextInReg..\n";
      dbgs() << "TruncTy is --> " << *TruncTy << "\n";
      dbgs() << "ExtTy is --> " << *ExtTy << "\n";
      InsertSExtInReg(V, Instr, TruncTy, ExtTy);
      AppliedSols.insert(V);
    } else {
      dbgs() << "Called WIAK_EXTLO on something that is not an Instruction";
    }
    break;
  }
  case WIAK_FILL: {
    if (auto *Instr = dyn_cast<Instruction>(V)) {
      Type *TruncTy = getTypeFromInteger(Sol->getFillTypeWidth());
      Type *ExtTy = getTypeFromInteger(Sol->getUpdatedWidth());
      dbgs() << "Sol is " << *Sol << "\n";
      dbgs() << "Calling InsertSextInReg..\n";
      dbgs() << "TruncTy is --> " << *TruncTy << "\n";
      dbgs() << "ExtTy is --> " << *ExtTy << "\n";
      InsertSExtInReg(Instr, Instr, TruncTy, ExtTy);
      AppliedSols.insert(V);
    } else {
      dbgs() << "Called WIAK_EXTLO on something that is not an Instruction";
    }
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
          auto UseIt = BestSolPerInst.find(UseI);
          if (UseIt == BestSolPerInst.end()) {
            // assert(0 && "Cannot have a no sol here");
            continue;
          }
          auto *BestUseSol = UseIt->second;
          unsigned short NewUserWidth = BestUseSol->getUpdatedWidth();
          unsigned short UseFillTypeWidth = getKnownFillTypeWidth(UseI);
          if ((!isLegalToPromote(UseI) ||
               UseFillTypeWidth > Sol->getUpdatedWidth()) &&
              NewUserWidth != Sol->getUpdatedWidth()) {
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
    dbgs() << "Applying wiak_drop something..\n";
    Instruction *Instr = dyn_cast<Instruction>(V);
    Value *N0 = Instr->getOperand(0);
    const WideningIntegerSolutionInfo *BestSol = nullptr;
    if (auto *OP0 = dyn_cast<Instruction>(Instr->getOperand(0))) {
      auto It = BestSolPerInst.find(OP0);
      if (It == BestSolPerInst.end()) {
        dbgs() << "Couldn't find best sol for " << *OP0 << "\n";
        if (NewInsts.contains(OP0)) {
          dbgs() << *OP0 << "is a new inst\n";
          BestSol = nullptr;
        }
      } else {
        auto *Sol = It->second;
        BestSol = Sol;
      }
    }
    bool DropFromAllUsers = CanDropExtFromAllUsers(Instr);
    if (BestSol != nullptr)
      dbgs() << "Best sol is --> " << *BestSol << "\n";

    dbgs() << "Drop from all Users is --> " << DropFromAllUsers << "\n";
    if (CanDropExtFromAllUsers(Instr)) {
      dbgs() << "YAYY Droping Extension!\n";
      dbgs() << "Replacing users with --> " << *N0 << "\n";
      replaceAllUsersOfWith(Instr, N0);
      if (BestSol != nullptr) {
        Type *NewTy = getTypeFromInteger(BestSol->getUpdatedWidth());
        dbgs() << "Mutating Type on --> " << *N0 << "\n";
        dbgs() << "From " << *(N0->getType()) << "to " << *NewTy << "\n";
        N0->mutateType(NewTy);
        dbgs() << "N0 now is " << *N0 << "\n";
      }
      InstsToRemove.insert(Instr);
      AppliedSols.insert(V);
    } else {
      dbgs() << "SAD Trying to Drop Ext " << *Instr
             << " but cannot drop cause of the users..\n";
    }

    break;
  }
  case WIAK_PHI: {
    auto OldWidth = getIntegerFromType(V->getType());
    auto NewWidth = Sol->getUpdatedWidth();
    Type *NewTy = getTypeFromInteger(NewWidth);
    dbgs() << "Mutating Type on --> " << *V << "\n";
    dbgs() << "From " << *(V->getType()) << "to " << *NewTy << "\n";
    dbgs() << "V now is " << *V << "\n";
    // TODO: If we mutate the type here the operands have the old type.
    // We need to update them too to the type of the parent and so on.
    dbgs() << "Checking phis NOW!\n";
    V->mutateType(getTypeFromInteger(Sol->getUpdatedWidth()));
    auto *PhiInst = dyn_cast<PHINode>(V);
    int NumIncValues = PhiInst->getNumIncomingValues();
    for (int i = 0; i < NumIncValues; ++i) {
      Value *V = PhiInst->getIncomingValue(i);
      if (auto *I = dyn_cast<Instruction>(V)) {
        auto SolIt = BestSolPerInst.find(I);
        if (SolIt != BestSolPerInst.end()) {
          const WideningIntegerSolutionInfo *IncSol = SolIt->second;
          // assert(IncSol->getUpdatedWidth() == NewWidth);
          I->mutateType(getTypeFromInteger(NewWidth));
          // TODO: is it enough?
          // What about uses and ops?
        }
        // applyChain(I);
      } else if (auto *CI = dyn_cast<ConstantInt>(V)) {
        int FoundWidth = 0;
        for (auto *CSol : AvailableSolutions[CI]) {
          if (CSol->getUpdatedWidth() == NewWidth) {
            CI->mutateType(getTypeFromInteger(NewWidth));
          }
        }
        // TODO: check
        // FIXME: this fails
        // assert(FoundWidth &&
        //       "Didn't found matching width on constant on PHiNode..\n");
      }
    }

    AppliedSols.insert(V);
  }
  }
  // we need to have all the Combinations of the LegalUsersSolutions..
  return true;
}

bool WideningIntegerArithmetic::applyChain(Instruction *Instr) {
  bool Changed = false;
  SmallSetVector<Instruction *, 16> Worklist;
  // TODO: when applying a Sol for an Instruction
  // First we must rewrite all the operands
  // and all the Users with the operands for each user
  // and so on
  dbgs() << "Inside apply chain for Inst --> " << *Instr << "\n";
  dbgs() << "before transformation Func is " << "\n";
  Function *F = Instr->getFunction();
  dbgs() << *F << "\n";
  Worklist.insert(Instr);
  SetVector<Instruction *> InstsVisited;
  while (!Worklist.empty()) {
    dbgs() << "Poping from 12worklist size before pop" << Worklist.size()
           << "\n";
    Instruction *I = Worklist.pop_back_val();
    if (InstsVisited.contains(I)) {
      continue;
    }
    if (isa<CallInst>(I)) {
      continue;
    }
    dbgs() << "11worklist size is --> " << Worklist.size() << "\n";
    auto BestSolsUserOpsCombIt = BestUserOpsSolsPerInst.find(I);
    if (isa<BranchInst>(I) || NewInsts.contains(I)) {
      continue;
    }
    if (BestSolsUserOpsCombIt == BestUserOpsSolsPerInst.end() &&
        !isa<BranchInst>(I)) {
      dbgs() << "Didn't find best UserOps Sols for Inst " << I << "\n";
      dbgs() << "Didn't find best UserOps Sols for Inst " << *I << "\n";
      assert(0);
    }

    auto BestSolsUsersOpsCombination = BestSolsUserOpsCombIt->second;
    dbgs() << "Size of best sols users combs is "
           << BestSolsUsersOpsCombination.size() << "\n";
    if (BestSolsUsersOpsCombination.empty()) {
      dbgs() << "Found empty userOps combinations for " << *I << "\n";
    }
    dbgs() << "Instruction that we are iterating users and ops is " << *I
           << "\n";
    dbgs() << "=============================================================\n";
    for (auto &Combination : BestSolsUsersOpsCombination) {
      // All users are solved at this point
      Value *VComb = Combination.first;
      const WideningIntegerSolutionInfo *Sol = Combination.second;
      if (Sol == nullptr) {
        dbgs() << "Sol is null..! for --> " << VComb << "\n";
        dbgs() << "Sol is null..! for --> " << *VComb << "\n";

        if (AvailableSolutions[VComb].empty()) {
          dbgs() << "Available solutions are empty..\n";
        } else {
          dbgs() << "AvailableSolutions are NOT empty..\n";
        }
        assert(Sol != nullptr && "Sol is null..\n");
      }
      dbgs() << "Sol is " << Sol << "\n";
      if (NewInsts.contains(VComb)) {
        continue;
      }
      if (Sol->getKind() == WIAK_UNKNOWN) {
        dbgs() << "!!!!!!Best solution has Unknown kind..cannot apply it yet, ";
        if (auto *I = dyn_cast<Instruction>(VComb)) {
          dbgs() << "Opcode is " << OpcodesToStr[I->getOpcode()] << '\n';
        } else if (isa<ConstantInt>(VComb)) {
          dbgs() << "Opcode is Constant" << '\n';
        } else {
          dbgs() << "We don't know the opcode .. name is " << VComb->getName()
                 << "\n";
        }
        continue;
      }
    }
    dbgs() << "Going to apply best Sols to users and operands\n";
    for (auto &Combination : BestSolsUsersOpsCombination) {
      Value *UserOrOp = Combination.first;
      if (NewInsts.contains(UserOrOp) || isa<CallInst>(UserOrOp)) {
        continue;
      }
      dbgs() << "Getting Sol!! for User --> " << *UserOrOp << "\n";
      if (!UserOrOp) {
        dbgs() << "Failed to find UserOrOp..\n";
        assert(0);
      }
      if (auto *UserInst = dyn_cast<Instruction>(UserOrOp)) {
        auto BestSolIt = BestUserOpsSolsPerInst.find(UserInst);
        assert(BestSolIt != BestUserOpsSolsPerInst.end() &&
               "Didn't find user entry best sols");
        // UserOrOp and value BestSol.
        DenseMap<Value *, const WideningIntegerSolutionInfo *>
            BestUserOpSolsForInst = BestSolIt->second;
        for (Use &Op : UserInst->operands()) {
          Value *Operand = Op.get();
          if (isa<CallInst>(Operand) || isa<BranchInst>(Operand)) {
            continue;
          }
          if (AvailableSolutions[Operand].empty()) {
            dbgs() << "EMPTY sols for operand " << *Operand << "\n";
            continue;
          }
          auto BestSolOpIt = BestUserOpSolsForInst.find(Operand);
          if (BestSolOpIt == BestUserOpSolsForInst.end()) {
            dbgs() << "Didn't find best Sol for operand or user " << *Operand
                   << "\n";
            dbgs() << "Didn't find best Sol for operand or user " << Operand
                   << "\n";
            dbgs() << "For instruction --> " << *UserInst << "\n";
            dbgs() << "For instruction --> " << UserInst << "\n";
            if (NewInsts.contains(UserInst)) {
              dbgs() << "UserInst is a new inst.\n";
            }
            if (NewInsts.contains(Operand)) {
              dbgs() << "Operand is a new inst.\n";
            }
            dbgs() << "Printing All solutions..\n";
            printInstrSols(Operand);
            assert(0);
          }
          const WideningIntegerSolutionInfo *BestSol = BestSolOpIt->second;
          if (NewInsts.contains(Operand)) {
            dbgs() << "Found newly instruction..! " << *Operand << "\n";
            continue;
          }
          dbgs() << "Best sol is " << BestSol << "\n";
          if (BestSol == nullptr) {
            dbgs() << "Best sol is NULL for " << *Operand << "\n";
            continue;
          }
          dbgs() << "Operand is --> " << *Operand << "\n";
          printInstrSols(Operand);
          dbgs() << "for Operand --> " << *Operand << "from User-- > "
                 << *UserInst << " Applying Sol-- >" << *BestSol << "\n";
          auto UIt = BestSolPerInst.find(UserInst);
          if (UIt != BestSolPerInst.end()) {
            auto *bestUserInstSol = UIt->second;
            auto UserWidth = bestUserInstSol->getUpdatedWidth();
            // FIXME: THIS crashes
            // assert(BestSol->getUpdatedWidth() == UserWidth);
          }
          Changed |= applySingleSol(Operand, BestSol);
          if (auto *OpI = dyn_cast<Instruction>(Operand)) {
            dbgs() << "Pushing to the workList for apply Sol Inst --> " << *OpI
                   << "\n";
            Worklist.insert(OpI);
          }
        }
      }
      if (NewInsts.contains(UserOrOp)) {
        continue;
      }
      if (!NewInsts.contains(UserOrOp)) {
        dbgs() << *UserOrOp << "isn't a new value.\n";
      }
      const WideningIntegerSolutionInfo *BestUserSol = Combination.second;
      dbgs() << "1443Applying Single Sol for Value " << *UserOrOp << "\n";
      dbgs() << "With Solution " << *BestUserSol << "\n";
      auto ItS = BestSolPerInst.find(I);
      if (ItS != BestSolPerInst.end()) {
        const WideningIntegerSolutionInfo *BestSol = ItS->second;
        // FIXME: this crashes.
        // assert(BestUserSol->getUpdatedWidth() ==
        // BestSol->getUpdatedWidth());
      } else {
        // assert(0 && "didn't find BestSol for Instruction\n");
      }
      Changed |=
          applySingleSol(UserOrOp,
                         BestUserSol); // All users are solved at this point
      if (Changed) {
        if (auto *UserI = dyn_cast<Instruction>(UserOrOp)) {
          dbgs() << "Pushing to the workList for apply Sol Inst --> " << *UserI
                 << "\n";
          Worklist.insert(UserI);
        }
      }
    }
    if (NewInsts.contains(I)) {
      continue;
    }
    dbgs() << "Searching Sor for Inst! " << *I << "\n";
    auto SolIt = BestSolPerInst.find(I);
    if (!(isa<IntegerType>(I->getType()) || isa<SwitchInst>(I))) {
      dbgs() << "Inst --> " << *I << "isn't int type \n";
      dbgs() << "Careful! we don't apply SOL here \n";
      continue;
    }
    if (SolIt == BestSolPerInst.end()) {
      dbgs() << "No sol is found.\n";
      dbgs() << "Inst is --> " << *I << "\n";
      dbgs() << "we cannot apply anything\n";
      continue;
    }
    // assert(SolIt != BestSolPerInst.end());
    dbgs() << "Applying Sol!! " << *(SolIt->second) << "\n";
    Changed |= applySingleSol(I, SolIt->second);
    InstsVisited.insert(I);
  }
  dbgs() << "Returning changed..-> " << Changed << "\n";
  dbgs() << "---------After Transforming function for Instr --> " << *Instr
         << "\n";
  dbgs() << *F << "\n";
  dbgs() << "=============================================================\n";
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
  SmallVector<WideningIntegerSolutionInfo *, 8> ExprSolutions =
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
    WideningIntegerSolutionInfo *Expr =
        new WIA_DROP_LOIGNORE(Instr->getOpcode(), Sol->getOpcode(),
                              Sol->getFillType(), Sol->getFillTypeWidth(),
                              TruncatedWidth, NewWidth, Sol->getCost(), Instr);
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
  SmallVector<WideningIntegerSolutionInfo *, 8> LeftSols =
      AvailableSolutions[N0];
  SmallVector<WideningIntegerSolutionInfo *, 8> RightSols =
      AvailableSolutions[N1];
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
          Expr->addOperand(
              std::make_unique<WideningIntegerSolutionInfo>(*LeftSol));
          if (addNonRedudant(AvailableSolutions[VI], Expr)) {
            Changed = true;
          }
        }
      }
    }
  }
  return Changed;
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
  SmallVector<WideningIntegerSolutionInfo *, 8> Sols = AvailableSolutions[VI];
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
