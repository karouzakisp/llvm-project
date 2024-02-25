///===- WideningIntegerArithmetic ------------------------------------------===
//
//



#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/CodeGen/WideningIntegerArithmeticInfo.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Use.h"
#include "llvm/IR/User.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/Support/TypeSize.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Pass.h"
#include "llvm/InitializePasses.h"
#include "llvm/Analysis/ValueTracking.h"


#define DEBUG_TYPE "WideningIntegerArithmetic"
STATISTIC(NumSExtsDropped, "Number of ISD::SIGN_EXTEND nodes that were dropped"); 
STATISTIC(NumZExtsDropped, "Number of ISD::ZERO_EXTEND nodes that were dropped");
STATISTIC(NumAnyExtsDropped, "Number of ISD::ANY_EXTEND nodes that were dropped");
STATISTIC(NumTruncatesDropped, "Number of ISD::TRUNCATE nodes that were dropped"); 
using namespace llvm;

namespace {


class WideningIntegerArithmetic : public FunctionPass {
	const TargetMachine *TM = nullptr;
  const TargetLowering *TLI = nullptr; 
	const TargetTransformInfo *TTI = nullptr;
	const DataLayout *DL = nullptr;
  const LLVMContext &Ctx;

  public:
    WideningIntegerArithmetic(): FunctionPass(ID) {}
    WideningIntegerArithmetic(const WideningIntegerArithmetic &)= delete;
    WideningIntegerArithmetic& operator=(const WideningIntegerArithmetic&) = delete; 
    
    void solve(CodeGenOpt::Level OL);
  
    using isSolvedMap = DenseMap<Instruction*, bool>;
    using SolutionSet = SmallVector<WideningIntegerSolutionInfo *>;
    using SolutionSetParam = SmallVectorImpl<WideningIntegerSolutionInfo * >;    
    using AvailableSolutionsMap = DenseMap<Instruction* , SolutionSet>;
    
    using BinOpWidth = std::tuple<unsigned char, unsigned char, unsigned char>;
    using WidthsSet = SmallVector<BinOpWidth>;
    using OperatorWidthsMap = DenseMap<unsigned, WidthsSet>;
    using TargetWidthsMap = DenseMap<unsigned, OperatorWidthsMap>;
    
    using FillTypeSet = SmallSet<std::tuple<IntegerFillType, IntegerFillType, IntegerFillType>, 4>;
    using UnaryFillTypeSet = SmallSet<std::tuple<IntegerFillType, IntegerFillType> , 2>;
    
    using OperatorFillTypesMap = DenseMap<unsigned, FillTypeSet>;
    typedef WideningIntegerSolutionInfo* SolutionType;
  private:
    enum IntegerFillType ExtensionChoice;   
    
 
    // checks whether an Instruction in the Function F is visited and solved` 
    isSolvedMap solvedNodes; 
    bool IsSolved(Instruction *Instr);
    // Holds all the available solutions 
    AvailableSolutionsMap AvailableSolutions;


    TargetWidthsMap TargetWidths;
				    
   
 
    DenseMap<unsigned, UnaryFillTypeSet>  UnaryFillTypesMap;
    DenseMap<unsigned, FillTypeSet>       FillTypesMap;


    UnaryFillTypeSet getUnaryFillTypes(unsigned Opcode);
    FillTypeSet getFillTypes(unsigned Opcode);
    inline IntegerFillType getOrNullFillType(
            FillTypeSet availableFillTypes, IntegerFillType Left, 
            IntegerFillType Right);

    SmallVector<WideningIntegerSolutionInfo *> visit_widening(Instruction *Instr);


    FillTypeSet getOperandFillTypes(Instruction *Instr);
  
    void setFillType(Type* SrcTy, Type* DstTy);  
    bool isSolved(Instruction *Instr);

    bool addNonRedudant(SolutionSet &Solutions, 
                        WideningIntegerSolutionInfo* GeneratedSol);
    inline bool hasTypeGarbage(IntegerFillType fill);
    inline bool hasTypeT(IntegerFillType fill);
    inline bool hasTypeS(IntegerFillType fill);

    SolutionSet closure(Instruction *Instr);
    SolutionSet tryClosure(Instruction *Instr, bool changed);

    SolutionSet visitXOR(Instruction *Instr);
    SolutionSet visitInstruction(Instruction *Instr);
    SolutionSet visitBINOP(BinaryOperator *Binop);
    SolutionSet visitLOAD(Instruction *Instr);
    SolutionSet visitSTORE(Instruction *Instr);
    SolutionSet visitUNOP(Instruction *Instr);
		std::list<WideningIntegerSolutionInfo*> visitFILL(Instruction *Instr);
		std::list<WideningIntegerSolutionInfo*> visitWIDEN(Instruction *Instr);
		std::list<WideningIntegerSolutionInfo*> 
		  visitWIDEN_GARBAGE(Instruction *Instr);
		std::list<WideningIntegerSolutionInfo*> visitNARROW(Instruction *Instr);
    SolutionSet visitDROP_EXT(Instruction *Instr);
    SolutionSet visitDROP_TRUNC(Instruction *Instr);
    SolutionSet visitEXTLO(Instruction *Instr);
    SolutionSet visitSUBSUME_FILL(Instruction *Instr);
    SolutionSet visitSUBSUME_INDEX(Instruction *Instr);
    SolutionSet visitNATURAL(Instruction *Instr);
    SolutionSet visitCONSTANT(ConstantInt *CI);


    std::vector<unsigned short> IntegerSizes = {8, 16, 32, 64};

    unsigned int RegisterBitwidth = 0; 

    BinOpWidth createWidth(unsigned char op1, 
                   unsigned char op2, unsigned dst);

    bool IsBinop(unsigned Opcode);
    bool IsUnop(unsigned Opcode);
    bool IsTruncate(unsigned Opcode);
    bool IsExtension(unsigned Opcode);
    bool IsLit(unsigned Opcode);
    bool IsLoad(unsigned Opcode);
    bool IsStore(unsigned Opcode);
 
    // Helper functions 
    inline IntegerFillType getIntFillTypeFromLoad(ISD::LoadExtType ExtType); 
    inline IntegerFillType getLoadFillType(Instruction *Instr);
    inline unsigned int getScalarSize(const Type *Typ) const;
    inline unsigned  getExtensionChoice(enum IntegerFillType ExtChoice);   
    inline unsigned int getExtCost(Instruction *Instr, 
                WideningIntegerSolutionInfo* Sol, unsigned short IntegerSize);
    void initOperatorsFillTypes();
    void printInstrSols(SolutionSet Sols, Instruction *Instr);
    inline Type* getTypeFromInteger(unsigned char Integer);
    inline short int WideningIntegerArithmetic::getKnownFillTypeWidth(Instruction *Instr);

    bool mayOverflow(Instruction *Instr);
};

} // end anonymous namespace

class WideningIntegerArithmetic::ID = 0; 
static RegisterPass<WideningIntegerArithmetic> X("WideningIntegerArithmetic",
                            "WideningIntegerArithmeticPass", false, false);


  
void WideningIntegerArithmetic::printInstrSols(SolutionSet Sols, 
		Instruction *Instr){
  int i = 0;
	LLVM_DEBUG(dbgs() << "AvailableSolutions Size. --> " << Sols.size() << "\n");
	for(WideningIntegerSolutionInfo *Solution : Sols){
    LLVM_DEBUG(dbgs() << "----- Solution " << ++i << "\n" << (*Solution) << "\n");
  }
	LLVM_DEBUG(dbgs() << "=======================================================" << "\n");
}


inline unsigned WideningIntegerArithmetic::getExtensionChoice(
		enum IntegerFillType ExtChoice){
  return ExtChoice == SIGN ? ISD::SIGN_EXTEND:
                             ISD::ZERO_EXTEND;
} 

bool WideningIntegerArithmetic::IsBinop(unsigned Opcode){
  switch(Opcode){
		Instruction::Add:
		Instruction::Sub:
		Instruction::Mul:
		Instruction::UDiv:
		Instruction::SDiv:
		Instruction::URem:
		Instruction::SRem:
		Instruction::LShr:
		Instruction::AShr:
		Instruction::Shl:
		Instruction::ICmp:
		Instruction::And:
		Instruction::Or:
		Instruction::Xor: 				return true;
		default : 								return false; 
  }
  return false;
}


// TODO check UNOPS
bool WideningIntegerArithmetic::IsUnop(unsigned Opcode){
  switch(Opcode){
  	// TODO where is Instruction::Neg???????
		default: return false; 
	}
  return false;
}

bool WideningIntegerArithmetic::IsTruncate(unsigned Opcode){
  switch(Opcode){
		case Instruction::Trunc: return true;
    default: break;
  }
  return false;
}

bool WideningIntegerArithmetic::IsPHI(unsigned Opcode){
	switch(Opcode){
		case Instruction::PHI: return true;
		default: return false;
	}
}

bool WideningIntegerArithmetic::IsExtension(unsigned Opcode){
  switch(Opcode){
		case Instruction::ZExt: 		
		case Instruction::SExt: return true;
		default: return false;
  }
  return false;  
}

bool WideningIntegerArithmetic::IsStore(unsigned Opcode){
  switch(Opcode){
		Instruction::Store: return true;
    default: break;
  }
  return false;
}

bool WideningIntegerArithmetic::IsLoad(unsigned Opcode){
  switch(Opcode){
		case Instruction::Load:
      return true;   
    default: break;
  }
  return false;
}



long int TruncCounter = 0;
long int ExtCounter = 0;

SmallVector<WideningIntegerSolutionInfo *> 
WideningIntegerArithmetic::visitInstruction(Instruction *Instr){
  SolutionSet Solutions;
  unsigned Opcode = Instr->getOpcode();
  
  if(IsBinop(Opcode)){
    //dbgs() << " and Visiting Binop..\n"; 
    return visitBINOP(Instr)
  }
  else if(IsUnop(Opcode)){
    //dbgs() << " and Visiting Unop...\n"; 
    return visitUNOP(Instr);
  }
  else if(IsLoad(Opcode)){
    //dbgs() << " and Visiting Load...\n"; 
    return visitLOAD(Instr);
  }
  else if(IsStore(Opcode)){
    //dbgs() << " and Visting Store...\n"; 
    return visitSTORE(Instr);
  }
  else if(IsExtension(Opcode)){
    ExtCounter++;
    //dbgs() << " and Visiting Extension...\n"; 
    return visitDROP_EXT(Instr);
  }
  else if(IsTruncate(Opcode)){
    TruncCounter++; 
    //dbgs() << " and Visiting Truncation ..\n"; 
    return visitDROP_TRUNC(Instr);
  }
  else{
    dbgs() << "Could not found a solutionOpcode is " << Opcode << "\n";
    LLVM_DEBUG(dbgs() << "Opcode str is" << OpcodesToStr[Opcode] << "\n");
    // default solution so we can initialize all the nodes with some solution set.
    auto Sol = new WideningIntegerSolutionInfo(Opcode, ANYTHING, 
                RegisterBitWidth, 
                RegisterBitWidth, /* TODO CHECK */RegisterBitWidth , 
                0, WIAK_UNKNOWN, Instr);
    Solutions.push_back(Sol); 
  }
    
  // TODO we are missing many opcodes here, need to add them. 
  LLVM_DEBUG(dbgs() << "Returning default solution take care here.\n"); 
  return Solutions;
}


long long int counter = 0;

SmallVector<WideningIntegerSolutionInfo *> 
WideningIntegerArithmetic::visit_widening(Instruction *Instr){
 	SolutionSet EmptySols;

  if(IsSolved(Inst) ){
    LLVM_DEBUG(dbgs() << "Opcode str is " << OpcodesToStr[Instr->getOpcode()] << "\n"); 
    return AvailableSolutions[Instr]; 
  } 
  for (Value* V : Instr->operand_values() ){    
    if(auto *I = dyn_cast<Instruction>(V)){ // #TODO 1 check value size is 1?
   	 SolutionSet Sols = visit_widening(I);
		}
		else if(auto *CI = dyn_cast<ConstantInt>(V)){
			visitCONSTANT(CI);
		}
  }
	if(!Instr){
		//dbgs() << "empty Sols check........!!!!!!!!!!!!!!!!" << '\n';
		return EmptySols;
	}
  //dbgs() << " Trying to solve Node with Opc Number !! : " << Node->getOpcode() << "str -->  "  << OpcodesToStr[Node->getOpcode()] << "\n";
	auto CalcSolutions = visitInstruction(Instr);
	// #TODO to get the opcode need to check and cast to a Instruction
	//LLVM_DEBUG(dbgs() << " Solved Instruction !! : " << U->getOpcode() << "\n");
  //dbgs() << "Solved Instruction number !!: " << counter << "\n";
  solvedNodes[U] = true; 
	counter++;
  if(CalcSolutions.size() > 0){
    //printNodeSols(CalcSolutions, Node);
		if(auto search = AvailableSolutions.find(Instr); search != AvailableSolutions.end()){
      //dbgs() << "ERROR --!!!!!!!!!-===============" << "\n";
		}else{
    	AvailableSolutions[U] = CalcSolutions;
		}
  }else{
    LLVM_DEBUG(dbgs() << "This node does not have any solutions" << "\n");
  }
  return CalcSolutions;
  
}


bool WideningIntegerArithmetic::addNonRedudant(SolutionSet &Solutions, 
                    WideningIntegerSolutionInfo* GeneratedSol){
  bool WasRedudant = false;
  int RedudantNodeToDeleteCost = INT_MAX;
  LLVM_DEBUG(dbgs() << "Begin Add Non Redudant -> " << '\n');
  for(auto It = Solutions.begin(); It != Solutions.end(); ){
    int ret = (*It)->IsRedudant((*GeneratedSol));
    if(ret == -1 ){ // GeneratedSol is redudant
      WasRedudant = true; 
      It++; 
		 	LLVM_DEBUG(dbgs() << "GeneratedSol is redudant!" << '\n');	
    }else if(ret == 1){ // Sol is redudant
      assert(GeneratedSol->getCost() < RedudantNodeToDeleteCost);
      LLVM_DEBUG(dbgs() << "Current Sol is Redudant .... Deleting it --> " << (**It) << '\n');
      It = Solutions.erase(It);
      // TODO consider change data structure for Possible small optimization
    }else{ // ret == 0 no redudant found
      It++;
    }
  }
  if(!WasRedudant){
    LLVM_DEBUG(dbgs() << "!!!!!!!!!!!!!!Adding Solution --> " << OpcodesToStr[GeneratedSol->getOpcode()] << "\n"); 
		//<< *GeneratedSol << '\n';
    Solutions.push_back(GeneratedSol);
    return true;
  }
  LLVM_DEBUG(dbgs() << "Finished and Returning False --> " << '\n');
  return false;
}


WideningIntegerArithmetic::FillTypeSet 
WideningIntegerArithmetic::getFillTypes(unsigned Opcode){
  assert(IsBinop(Opcode) == true && "Not a binary operator to get the fillTypes");
  return FillTypesMap[Opcode];
}

WideningIntegerArithmetic::UnaryFillTypeSet
WideningIntegerArithmetic::getUnaryFillTypes(unsigned Opcode){

  assert(IsUnop(Opcode) == true && "Not a unary operator to get the fillTypes");
  return UnaryFillTypesMap[Opcode];
}


inline IntegerFillType 
  WideningIntegerArithmetic::getOrNullFillType(
            FillTypeSet availableFillTypes,
            IntegerFillType LeftFillType, IntegerFillType RightFillType){
 
  for (auto FillType : availableFillTypes ){
    if((std::get<0>(FillType) == LeftFillType && 
       std::get<1>(FillType) == RightFillType) || 
        (std::get<0>(FillType) == ANYTHING &&
         std::get<1>(FillType) == RightFillType)) 
      return std::get<2>(FillType);
  }
  return UNDEFINED; 
}


WideningIntegerArithmetic::SolutionSet 
WideningIntegerArithmetic::visitXOR(Instruction *Instr ){

  SolutionSet Set; 
  return Set;
  // add A function that combines the solution
  // based on below 
  // add with empty set
  // based on opcode operand fillType for XOR
  // can be either g, s, z
  // TODO FOR MY operand what is the fill type? 
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
  // . check legal values for XOR can
  // xor legal types
  // { s -> s x s }, {g-> g x g}, { z -> z x z } 
  // We can  choose 
  // Solution 1 s[32]:64 cost 4 + 3, Solution 2 g[32]:64 cost 3 needs sext or zext depends on the fill type of target so total 3 + 1 }
  // Instruction can decide what solution is appropriate
  // for example print requires sext to use Solution 2 because upper  // upper bits
  // {i32, i32, i32 } 
  // can we Truncate? 
}




void WideningIntegerArithmetic::setFillType(EVT SrcVT, EVT DstVT){
  if(TLI.isSExtCheaperThanZExt(SrcVT, DstVT)){
    ExtensionChoice = SIGN;
  }else{
    ExtensionChoice = ZEROS;
  }
}

void WideningIntegerArithmetic::solve(Function &F){

 	// TODO how to set filltype of Target Machine??
  //setFillType(Root.getValueType(), Root.getValueType());
  initOperatorsFillTypes();
	DL = F.getParent()->getDataLayout();
	TM = &getAnalysis<TargetPassConfig>().getTM<TargetMachine>();
	const TargetSubtargetInfo *SubtargetInfo = TM->getSubtargetImpl(F);
	TLI = SubtargetInfo->getTargetLowering();
 	TTI = &getAnalysis<TargetLibraryInfoWrapperPass>().getTTI(F);	
  Ctx = F.getContext();

  for (BasicBlock &BB : F){
		for(Instruction &I : BB){
			if(!IsSolved(&I))
				continue;

			if(isa<IntegerType>(I.getType()) ){
				visit_widening(&Node);  
			}
		} 
  }
} 


// Checks whether a SDNode is visited 
// so we don't visit and solve the same Node again
inline bool WideningIntegerArithmetic::IsSolved(Instruction *Instr){
  auto isVisited = solvedNodes.find(Instr);
  if(isVisited != solvedNodes.end())
    return true;
  
  return false;
}

WideningIntegerArithmetic::SolutionSet
WideningIntegerArithmetic::tryClosure(Instruction *Instr, bool Changed){
  auto FillsSize = 0;
  unsigned Opcode = Instr->getOpcode();
  if(IsBinop(Opcode)){
    FillTypeSet Fills = getFillTypes(Opcode);
    FillsSize = Fills.size();
  }
  else if(IsUnop(Opcode)){
    UnaryFillTypeSet Fills = getUnaryFillTypes(Opcode);
    FillsSize = Fills.size();
  }
  
  if(FillsSize  > 1 && Changed )
    return closure(Node);
    
  
  return AvailableSolutions[Node];
  
}


WideningIntegerArithmetic::SolutionSet 
WideningIntegerArithmetic::closure(Instruction *Instr){
  auto Sols = AvailableSolutions[Instr];
  bool Changed;
  int i = 0;
  do{
    Changed = false;
    auto FillsList = visitFILL(Instr);
    auto WidensList = visitWIDEN(Instr);
    auto GarbageWidenList = visitWIDEN_GARBAGE(Instr); 
    auto NarrowList = visitNARROW(Instr);
		FillsList.splice(FillsList.end(), WidensList);
		FillsList.splice(FillsList.end(), GarbageWidenList);
		FillsList.splice(FillsList.end(), NarrowList);
    LLVM_DEBUG(dbgs() << "Iteration " << ++i << 
				"---------------------------------------Adding Possible Solutions size is  " << 
				FillsList.size() << '\n' << "Opcode to Str is --> " << OpcodesToStr[Instr->getOpcode()] << '\n');
    for(auto PossibleSol : FillsList){
      LLVM_DEBUG(dbgs() << "Trying to Add Possible Sol --> " << *PossibleSol << '\n');

      bool Added = addNonRedudant(AvailableSolutions[Instr], PossibleSol);
      //printNodeSols(AvailableSolutions[Node], Node); 
      if(Added){
        Changed = true;
        LLVM_DEBUG(dbgs() << "Added Possible Solution " << '\n');
        printNodeSols(AvailableSolutions[Node], Node);
      } 
    }
    LLVM_DEBUG(dbgs() << "Possible Solution size is --> " << FillsList.size() << '\n'); 
  }while(Changed == true );
  LLVM_DEBUG(dbgs() << "Returning all the Solutions after non redudant" << '\n');
  return Sols;
}



inline 
WideningIntegerArithmetic::FillTypeSet 
WideningIntegerArithmetic::getOperandFillTypes(Instruction *Instr){

  FillTypeSet Set;
  unsigned Opcode = Instruction->getOpcode();
  return getFillTypes(Opcode);

	// TODO handle the case below.
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


WideningIntegerArithmetic::SolutionSet 
WideningIntegerArithmetic::visitCONSTANT(ConstantInt *CI){
  SolutionSet Sols;
  unsigned bitWidth = CI->getBitWidth();
  auto Sol = new WIA_CONSTANT(Node->getOpcode(), ExtensionChoice,
    bitWidth, getTargetWidth(), getTargetWidth(), 0, Instr);
  Sols.push_back(Sol);
  return Sols;
}
  
WideningIntegerArithmetic::SolutionSet 
WideningIntegerArithmetic::visitSTORE(Instruction *Instr){
  SolutionSet Sols;
  
  unsigned char N0Bits = getScalarSize(Instr->getType());
  auto N0Sols = AvailableSolutions[Instr];
  for(WideningIntegerSolutionInfo *Sol : N0Sols ){
    auto StoreSol = new WIA_STORE(Node->getOpcode(), Sol->getFillType(),
          N0Bits, N0Bits, N0Bits, Sol->getCost(), Node);
    Sols.push_back(StoreSol);
  }
  return Sols;   
}

inline IntegerFillType WideningIntegerArithmetic::getIntFillTypeFromLoad(ISD::LoadExtType ExtType){
  switch(ExtType){
  case ISD::NON_EXTLOAD: 
    return IntegerFillType::ANYTHING; 
  case ISD::EXTLOAD: 
    return IntegerFillType::ANYTHING;
  case ISD::SEXTLOAD: 
    return  IntegerFillType::SIGN;
  case ISD::ZEXTLOAD: 
    return IntegerFillType::ZEROS;
  default:
    return IntegerFillType::UNDEFINED;
  }
  return IntegerFillType::UNDEFINED;
}

inline IntegerFillType WideningIntegerArithmetic::getLoadFillType(SDNode *Node){
  
  if(isa<LoadSDNode>(Node)){
    LoadSDNode *LD  = cast<LoadSDNode>(Node);
    return getIntFillTypeFromLoad(LD->getExtensionType());
  }else if(isa<MaskedLoadSDNode>(Node)){
    MaskedLoadSDNode *MLD  = cast<MaskedLoadSDNode>(Node);
    return getIntFillTypeFromLoad(MLD->getExtensionType());
  }
  return IntegerFillType::UNDEFINED;
}

WideningIntegerArithmetic::SolutionSet 
WideningIntegerArithmetic::visitLOAD(Instruction *Instr){

  SolutionSet Sols;


  IntegerFillType FillType = getLoadFillType(Instr); 
  
  unsigned int Width = getScalarSize(Instr->getValueType(0));
  int FillTypeWidth = Width; 
  auto WIALoad = new WIA_LOAD(Node->getOpcode(), FillType, FillTypeWidth,
                Width, FillTypeWidth, 0, Node);
  //unsigned ExtensionOpc = getExtensionChoice(FillType);
 	SolutionSet WidenSols;	
	/*for(int IntegerSize : IntegerSizes){
		EVT NewVT = EVT::getIntegerVT(*DAG.getContext(), IntegerSize); 
		if(IntegerSize <= FillTypeWidth) // TODO check
			continue;
		if(!TLI.isOperationLegal(ISD::SIGN_EXTEND_INREG, NewVT))
			continue;
		// Results to a widened expr based on ExtensionOpc
		WideningIntegerSolutionInfo *Widen = new WIA_WIDEN(
			ExtensionOpc, FillType, FillTypeWidth, Width,
			IntegerSize, 1, Node);
		Widen->addOperand(WIALoad); 
		WidenSols.push_back(Widen); 
	}	*/
 	WidenSols.push_back(WIALoad);	
 	return WidenSols; 
}
  


// TODO not done yet
inline bool
WideningIntegerArithmetic::mayOverflow(Instruction *Instr){
	
  unsigned Opcode;
  SDValue N0 = Node->getOperand(0);
  SDValue N1 = Node->getOperand(1);
  Opcode = Node->getOpcode();
  switch(Opcode){
		case Instruction::Add:
		case Instruction::SDiv:
		case Instruction::UDiv:
		case Instruction::Mul:
		case Instruction::URem:
		case Instruction::SRem: 
		default:
      return false; 
  }
} 


std::vector<WideningIntegerArithmetic::SolutionSet>
WideningIntegerArithmetic::getLegalSolutions(Instruction *Instr){

  SmallVector<Use *, 4> IUsers;
  SmallVector<SolutionSet, 4> UsersSolution;
  std::vector<SolutionSet> Combinations;
  if(Instruction->uses().size() == 0 ){
    return Combinations;
  }

  for(auto &Use : Instr->uses()){
    Instruction *IUser = cast<Instruction>(Use.getUser());
    UsersSolution.push_back(AvailableSolutions[IUser]);
    IUsers.push_back(IUser);
  }
  Instruction *User = Users[0];
  SmallVector<Instruction *, 3> Users_without = Users;
  auto UserPos = std::find(Users.begin(), Users.end(), User);
  Users_without.erase(UserPos);
  for(WideningIntegerSolutionInfo *Sol : AvailableSolutions[User]){
    SolutionSet OneCombination.push_back(Sol);
    bool all_matching = true;
    for(Instruction *IUser1 : Users_without){
      for(WideningIntegerSolutionInfo *Sol1 : AvailableSolutions[IUser1]){
        if(isMatching(Sol, Sol1)){
          OneCombination.push_back(Sol1);
        }else{
          all_matching = false;
        }
      }
    }
    if(all_matching){
      Combinations.push_back(OneCombination);
    }
    OneCombination.clear();
  }
  return Combinations;
}



inline short int WideningIntegerArithmetic::getKnownFillTypeWidth(Instruction *Instr){
	if(!isa<Value>(Instr)){
		return -1;
	}	
	Value *V = dyn_cast<Value>(Instr);
	KnownBits Known = computeKnownBits(V, 0, Instr);
	if(Known.isUnknown()){
    // Assume that the data is equal to the instruction width
    // We must guarantee that the Instr does not overflow
    // otherwise this is wrong. 
    // The overflow checks is done for all the Operators that
    // can overflow such as add, sub.
    return Instr->getType()->getScalarSizeInBits(); 
  }
  return Known.getBitWidth();	 
}

// check IsRedudant uses fillTypeWidth 
// Return all the solution based on binop rules op1 x op2 -> W
WideningIntegerArithmetic::SolutionSet 
WideningIntegerArithmetic::visitBINOP(BinaryOperator *Binop){
	
  SolutionSet newSolutions;
  
	Value *V0 = Binop->getOperand(0);
  Value *V1 = Binop->getOperand(1);
	if(!isa<Instruction>(V0) || !isa<ConstantInt>(V0)){
		dbgs() << "Error in binop operands not a constant or Instr" << '\n';
		return Sols;	
	}
	if(!isa<Instruction>(V1) || !isa<ConstantInt>(V1)){
		dbgs() << "Error in binop operands not a constant or Inst" << '\n';
		return Sols;
	}
  
  bool AddedSol = false; 
  // get All the available solutions from nodes 
  SmallVector<WideningIntegerSolutionInfo*> LeftSols = 
                                              AvailableSolutions[V0];
  SmallVector<WideningIntegerSolutionInfo*> RightSols = 
                                              AvailableSolutions[V1];
  
  unsigned Opcode = Instr->getOpcode();
	unsigned int InstrWidth = Instr->getType()->getScalarSizeInBits();
  unsigned BinopFillTypeWidth = getKnownFillTypeWidth(Instr);
	assert(BinopFillTypeWidth > Known.getBitWidth() && 
      "Error FillType cannot be bigger than BinopWidth" ); 
  WideningIntegerSolutionInfo *defaultSol = new WIA_BINOP(Opcode, 
			ExtensionChoice, 
			/*FillTypeWidth*/ BinopFillTypeWidth, /* OldWidth*/ InstrWidth , 
			InstrWidth/*NewWidth*/ , /* Cost */ 0, Node); 
  AvailableSolutions[Node].push_back(defaultSol);

  LLVM_DEBUG(dbgs() << "Operand 0 is --> " << OpcodesToStr[V0->getOpcode()]<< "\n");
  LLVM_DEBUG(dbgs() << "Operand 1 is --> " << OpcodesToStr[V1->getOpcode()]<< "\n");
  LLVM_DEBUG(dbgs() << "Operand 0 Solutions Size is --> " << LeftSols.size() << "\n");
  LLVM_DEBUG(dbgs() << "Operand 1 Solutions Size is --> " << RightSols.size() << "\n");
  FillTypeSet OperandFillTypes = getOperandFillTypes(Binop);
  // A function that combines solutions from operands left and right
  for (WideningIntegerSolutionInfo *leftSolution : LeftSols){
    for(WideningIntegerSolutionInfo *rightSolution : RightSols){
      // Test whether current binary operator has the available
      // fill type provided by leftSolution and rightSolution
      auto FillType = getOrNullFillType((OperandFillTypes),
                                  leftSolution->getFillType(),
                                  rightSolution->getFillType());
      if(FillType == UNDEFINED)
        continue;
      //dbgs() << "Passed FillType for combination --> " << FillType << "\n";
      unsigned int w1 = leftSolution->getUpdatedWidth();
      unsigned int w2 = rightSolution->getUpdatedWidth();
      if(w1 != w2){
       	LLVM_DEBUG(dbgs() << "Width " << w1 << "and Width " << w2 << " are not the same skipping solution.." << "\n");
			 	continue;
			}
      if(w1 == 0 || w2 == 0){
        //dbgs() << "Left Solution --> " << *leftSolution << '\n';
        //dbgs() << "Right Solution --> " << *rightSolution << '\n';
        auto Node11 = leftSolution->getNode();
        LoadSDNode *LD  = cast<LoadSDNode>(Node11);
        //dbgs() << "EVT get String " << LD->getMemoryVT().getEVTString() << '\n'; 
      }
      //dbgs() << "The widths are the same and we continue--> " << w1 << "\n"; 
      EVT NewVT = EVT::getIntegerVT(Ctx, w1); 
      if(!TLI.isOperationLegal(TLI.InstructionOpcodeToISD(Opcode), NewVT)){
				LLVM_DEBUG(dbgs() << "Width: " << w1 << " Is not legal for binop" << "\n");
        continue;
			}
      LLVM_DEBUG(dbgs() << "The Operation is legal for that newVT--> "<<  w1 << "\n");
      // w1 x w2 --> w all widths are the same at this point
      // and there is a LegalOperation for that Opcode
      unsigned char UpdatedWidth = w1;

      unsigned char Cost = leftSolution->getCost() + rightSolution->getCost();
      unsigned char FillTypeWidth = getScalarSize(Node->getValueType(0)); 
                    // this Target of this Binary operator of the form
                    // width1 x width2 = newWidth
                    // for example on rv64 we have addw
                    // that is of the form i32 x i32 -> i32 
                    // and stored in a sign extended format to i64

      auto Sol = new WIA_BINOP(Node->getOpcode(), FillType, 
          FillTypeWidth, w1, UpdatedWidth, Cost, Node);
      Sol->addOperand(leftSolution);
      Sol->addOperand(rightSolution);
      LLVM_DEBUG(dbgs() << "Adding Sol with cost " << Cost << " and width " << UpdatedWidth << "\n");
      AvailableSolutions[Node].push_back(Sol);
      AddedSol = true; 
    }
  }
  //dbgs() << "Calling closure here.\n"; 
  // call closure if multiple FillTypes or Multiple Binop Widths
  return tryClosure(Node, AddedSol); 
}

std::list<WideningIntegerSolutionInfo*> WideningIntegerArithmetic::visitFILL(
                          Instruction *Instr){
  
  unsigned Width = Instr->getType()->getScalarSizeInBits();
  unsigned ExtensionOpc = getExtensionChoice(ExtensionChoice);
  unsigned char FillTypeWidth = getKnownFillTypeWidth(Instr); 
  
  SolutionSet Sols = AvailableSolutions[Instr];
	std::list<WideningIntegerSolutionInfo*> Solutions;
  for(WideningIntegerSolutionInfo *Sol : Sols){
    if(Sol->getOpcode() == Instruction::SExt ||
       llvm::ISD::isExtOpcode(Sol->getOpcode()) ||
         Sol->getOpcode() == Instruction::Trunc ) 
     continue;    //FILL is a ISD::SIGN_EXTEND_INREG
    // WIA_FILL extends the least significant *Width* bits of Instr
    // to targetWidth
    for(auto IntegerSize : IntegerSizes ){
      EVT NewVT = EVT::getIntegerVT(Ctx, IntegerSize);
      // TODO should I add isTypeLegal for the NewVT
      // meaning is TLi.isOperationLegal enough for the NewVT? 
      if(IntegerSize < FillTypeWidth || IntegerSize < (int)Sol->getWidth() ||
         InstructionOpcodeToISD(Sol->getOpcode()) == ISD::TRUNCATE || 
         !TLI.isOperationLegal(ISD::SIGN_EXTEND_INREG, NewVT)){
        continue;
			} 
      WideningIntegerSolutionInfo *Fill = new WIA_FILL(
        ExtensionOpc, ExtensionChoice, FillTypeWidth, Width,
        IntegerSize , Sol->getCost() + 1, Instr ); 
      Fill->addOperand(Sol);
      Solutions.push_front(Fill);  
    }
  }
	return Solutions;	  
}

inline Type* WideningIntegerArithmetic::getTypeFromInteger(
                                unsigned char Integer){
  Type* Ty;
  switch(Integer){
    case 8: Ty = Type::getInt8Ty(Ctx); break;
    case 16: Ty = Type::getInt16Ty(Ctx); break;
    case 32: Ty = Type::getInt32Ty(Ctx); break;
    case 64: Ty = Type::getInt64Ty(Ctx); break; 
  }
  return Ty; 
}

inline unsigned WideningIntegerArithmetic::getExtCost(Instrution *Instr,
         WideningIntegerSolutionInfo* Sol, unsigned short IntegerSize){
  unsigned cost = Sol->getCost();
  Type* Ty1 = Instr->getType();;
  Type* Ty2 = getTypeFromInteger((int)IntegerSize);
  if(!TLI->isZExtFree(Ty1, Ty2) || !TLI->isSExtFree(Ty1, Ty2)){
    ++cost;
  }
  return cost;
}
  
std::list<WideningIntegerSolutionInfo*> WideningIntegerArithmetic::visitWIDEN(
                          Instruction *Instr){
 
  // TODO how to check for Type S? depends on target ? or on this operand
  // if(!hasTypeS(Sol->getFillType())
  //  return NULL; 
  
  unsigned Width = Instr->getType()->getScalarSizeInBits();
  unsigned ExtensionOpc = getExtensionChoice(ExtensionChoice);
  unsigned char FillTypeWidth = getKnownFillTypeWidth(Instr); 
  SolutionSet Sols = AvailableSolutions[Instr];
  LLVM_DEBUG(dbgs() << "SolutionsSize is --> " << Sols.size() << '\n');
	std::list<WideningIntegerSolutionInfo*> Solutions;
  for(WideningIntegerSolutionInfo *Sol : Sols){
    LLVM_DEBUG(dbgs() << "Inside visitWiden Iterating Sol --> " << Sol << '\n');
    unsigned IsdOpc = InstructionOpcodeToISD(Sol->getOpcode);
    if(llvm::ISD::isExtOpcode(Sol->getOpcode()) || 
       IsdOpc == ISD::SIGN_EXTEND_INREG || 
       IsdOpc == ISD::TRUNCATE ) 
      continue;
    LLVM_DEBUG(dbgs() << "Passed first if --> " << Sol << '\n');
    for(int IntegerSize : IntegerSizes){
      EVT NewVT = EVT::getIntegerVT(Ctx, IntegerSize); 
      if(IntegerSize < FillTypeWidth || IntegerSize < Sol->getWidth() ||
         !TLI.isOperationLegal(ExtensionOpc, NewVT)){
        continue;
			}
      LLVM_DEBUG(dbgs() << "Adding all Solution with IntegerSize --> " << IntegerSize << '\n');
      unsigned cost = getExtCost(Node, Sol, IntegerSize); 
      // Results to a widened expr based on ExtensionOpc
      WideningIntegerSolutionInfo *Widen = new WIA_WIDEN(
        ExtensionOpc, ExtensionChoice, FillTypeWidth, Width,
        IntegerSize, cost , Instr);
      Widen->addOperand(Sol); 
      Solutions.push_front(Widen);  
    } 
  }
	return Solutions;	
}
    

std::list<WideningIntegerSolutionInfo*> WideningIntegerArithmetic::visitWIDEN_GARBAGE(
      Instruction *Instr){

  unsigned Width = Instr->getType()->getScalarSizeInBits();
  unsigned ExtensionOpc = ISD::ANY_EXTEND;  // Results to a garbage widened
  unsigned char FillTypeWidth = getKnownFillTypeWidth(Instr); 
  auto Sols = AvailableSolutions[Instr];
  LLVM_DEBUG(dbgs() << "SolutionsSize is --> " << Sols.size() << '\n');
	std::list<WideningIntegerSolutionInfo*> Solutions;
  // TODO add isExtFree and modify cost accordingly 
  for(WideningIntegerSolutionInfo *Sol : Sols){
    LLVM_DEBUG(dbgs() << "Inside GarbageWiden Iterating Sol --> " << Sol << '\n');
    unsigned IsdOpc = InstructionOpcodeToISD(Sol->getOpcode);
    if(llvm::ISD::isExtOpcode(IsdOpc) || 
       // TODO how to check for this? Is it needed also ? Sol->getOpcode() == ISD::SIGN_EXTEND_INREG ) ||
       IsdOpc == Instruction::Truncate ) 
      continue;
    LLVM_DEBUG(dbgs() << "Passed first if --> " << Sol << '\n');
    for(int IntegerSize : IntegerSizes){
      EVT NewVT = EVT::getIntegerVT(Ctx, IntegerSize); 
      if(IntegerSize < FillTypeWidth || IntegerSize < Sol->getWidth() ||  
         !TLI->isOperationLegal(ISD::ANY_EXTEND, NewVT)){
        continue;
			}
      // TODO check do we need isTypeLegal? 
      unsigned cost = getExtCost(Instr, Sol, IntegerSize); 
      LLVM_DEBUG(dbgs() << "Adding GarbageWiden --> " << IntegerSize << '\n');
     
      WideningIntegerSolutionInfo *GarbageWiden = new WIA_WIDEN(
        ExtensionOpc, ANYTHING, FillTypeWidth,  Sol->getWidth(), IntegerSize,
        cost , Instr);
      GarbageWiden->addOperand(Sol);
      Solutions.push_front(GarbageWiden);    
    }
  }
	return Solutions;	
}
    
std::list<WideningIntegerSolutionInfo*> WideningIntegerArithmetic::visitNARROW(
    Instruction *Instr){
  
 
  unsigned Width = Instr->getType()->getScalarSizeInBits();
  unsigned ExtensionOpc = ISD::ANY_EXTEND;  // Results to a garbage widened
  unsigned char FillTypeWidth = getKnownFillTypeWidth(Instr); 
  // TODO check isTruncateFree on some targets and widths.
  // Not sure the kinds of truncate of the Target will determine it.
  unsigned ExtensionOpc = getExtensionChoice(ExtensionChoice);
 
  auto Sols = AvailableSolutions[Instr];
  LLVM_DEBUG(dbgs() << "SolutionsSize is --> " << Sols.size() << '\n');
	std::list<WideningIntegerSolutionInfo*> Solutions;
  for(auto Sol : Sols){ 
    LLVM_DEBUG(dbgs() << "Inside visitNarrow " << '\n');
    unsigned IsdOpc = InstructionOpcodeToISD(Sol->getOpcode);
    if(llvm::ISD::isExtOpcode(Sol->getOpcode()) || 
        IsdOpc == ISD::SIGN_EXTEND_INREG  ||
        IsdOpc == ISD::TRUNCATE ) 
      continue;
    for(int IntegerSize : IntegerSizes){
      EVT NewVT = EVT::getIntegerVT(Ctx, IntegerSize);
      if(!TLI.isOperationLegal(ISD::TRUNCATE, NewVT) ||
				 IntegerSize < FillTypeWidth || IntegerSize >= Sol->getWidth()) {
				continue;
			}
    
      LLVM_DEBUG(dbgs() << "Getting type for EVT for Left child " << '\n');
      unsigned cost = Sol->getCost();
      Type* Ty1 = Node->getValueType(0).getTypeForEVT(*DAG.getContext());
      Type* Ty2;
      switch(IntegerSize){
        case 8: Ty2 = Type::getInt8Ty(Ctx); break;
        case 16: Ty2 = Type::getInt16Ty(Ctx); break;
        case 32: Ty2 = Type::getInt32Ty(Ctx); break;
      }
      if(!TLI->isTruncateFree(Ty1, Ty2))
        cost = cost + 1;
      LLVM_DEBUG(dbgs() << "Adding new Wia Narrow" << IntegerSize <<  '\n');
      WideningIntegerSolutionInfo *Trunc = new WIA_NARROW(ExtensionOpc,
        ExtensionChoice, // Will depend on available Narrowing , 
        FillTypeWidth, Width, IntegerSize, Sol->getCost() + 1 , Node);
      Trunc->addOperand(Sol);
      Solutions.push_front(Trunc);
    }
  } 
	return Solutions;	
}
    
WideningIntegerArithmetic::SolutionSet WideningIntegerArithmetic::visitDROP_EXT(
                          Instruction *Instr){
  unsigned Width = Instr->getType()->getScalarSizeInBits();
  unsigned ExtensionOpc = ISD::ANY_EXTEND;  // Results to a garbage widened
  unsigned char FillTypeWidth = getKnownFillTypeWidth(Instr); 

  Value *N0 = Instr->getOperand(0);
  unsigned char ExtendedWidth = Instr->getType()->getScalarSizeInBits();
  unsigned char OldWidth = N0->getType()->getScalarSizeInBits();  
  unsigned Opc;
  if(auto *Iop = dyn_cast<Instruction>(N0)){
    Opc = N0->getOpcode();
  }else if(Iop = dyn_cast<ConstantInt>(N0)){
    Opc = 100; //  Indicates a ConstantIn
  }else{
    Opc = 333; // Indicates unknown opc 
  }
  auto ExprSolutions = AvailableSolutions[N0.getNode()];
  SolutionSet Sols;
  switch(Node->getOpcode()){
    case ISD::SIGN_EXTEND: ++NumSExtsDropped; break;
    case ISD::ZERO_EXTEND: ++NumZExtsDropped; break;
    case ISD::ANY_EXTEND: ++NumAnyExtsDropped; break;
    case ISD::AssertSext: ++NumSExtsDropped; break;      
    case ISD::AssertZext: ++NumZExtsDropped; break;
  }
  LLVM_DEBUG(dbgs() << "Trying to drop extension in Solutions" << '\n');
  LLVM_DEBUG(dbgs() << "Expr Solutions Size is " << ExprSolutions.size() << '\n');
  LLVM_DEBUG(dbgs() << "Opc of Node->Op0 is " << Opc << " Opc string is " << OpcodesToStr[Opc] << '\n');
  LLVM_DEBUG(dbgs() << "Opc of Node is " << Node->getOpcode() << "Opc of Node str is " << OpcodesToStr[Node->getOpcode()] << '\n');
  LLVM_DEBUG(dbgs() << "ExtendedWidth of Node is " << ExtendedWidth << '\n');
  LLVM_DEBUG(dbgs() << "NewWidth of Node is " << OldWidth << '\n');
  LLVM_DEBUG(dbgs() << "Opc of Node is " << Node->getOpcode() << "Opc of Node str is " << OpcodesToStr[Node->getOpcode()] << '\n');
  for(auto Solution : ExprSolutions){ 
  // We simply drop the extension and we will later see if it's needed.
    LLVM_DEBUG(dbgs() << "Drop extension in Solutions" << '\n'); 
  	unsigned char FillTypeWidth = Solution->getFillTypeWidth();  
    WideningIntegerSolutionInfo *Expr = new WIA_DROP_EXT(Solution->getOpcode(),
      ExtensionChoice, FillTypeWidth, ExtendedWidth /*OldWidth*/, 
      OldWidth/*NewWidth*/, Solution->getCost(), Instr);
    
    Expr->setOperands(Solution->getOperands());
    Sols.push_back(Expr); 
  }
  return Sols; 
}


  
WideningIntegerArithmetic::SolutionSet 
  WideningIntegerArithmetic::visitDROP_TRUNC(
                          SDNode *Node){
  SolutionSet Sols;
  assert(Node->getOpcode() == llvm::ISD::TRUNCATE && 
                              "Not an extension to drop here");  
  // TRUNCATE has only 1 operand
  SDValue N0 = Node->getOperand(0);
  SmallVector<WideningIntegerSolutionInfo*> ExprSolutions = 
                                              AvailableSolutions[N0.getNode()];
  
  if(ExprSolutions.size() == 0 ){
    LLVM_DEBUG(dbgs() << "Child of Truncate with opc " << N0.getNode()->getOpcode());
    LLVM_DEBUG(dbgs() << " has no Solutions\n");
    return Sols; 
  }
  unsigned Opc = N0->getOpcode();
	// NewWidth is the width of the value before the truncation
  unsigned char NewWidth = getScalarSize(N0.getValueType()); 
  unsigned char TruncatedWidth = getScalarSize(Node->getValueType(0)); 
  // We simply drop the truncation and we will later see if it's needed.
  NumTruncatesDropped++; 
  for(auto Sol : ExprSolutions){
    WideningIntegerSolutionInfo *Expr = new WIA_DROP_LOCOPY(Opc,
      Sol->getFillType(), Sol->getFillTypeWidth(), TruncatedWidth, 
      NewWidth, Sol->getCost(), Node);
    Expr->setOperands(Sol->getOperands());   
    Sols.push_back(Expr); 
  }
  // We simply drop the truncation and we will later see if it's needed.
  NumTruncatesDropped++; 
 
  // We simply drop the truncation and we will later see if it's needed.
  for(auto Sol : ExprSolutions){ 
    WideningIntegerSolutionInfo *Expr = new WIA_DROP_LOIGNORE(Opc,
      Sol->getFillType(), Sol->getFillTypeWidth(), TruncatedWidth, 
      NewWidth, Sol->getCost(), Node);
    Expr->setOperands(Sol->getOperands());
    Sols.push_back(Expr); 
  }
  return Sols;  
}


WideningIntegerArithmetic::SolutionSet WideningIntegerArithmetic::visitEXTLO(
                          SDNode *N){
  SolutionSet CalcSols;
  SDValue N0 = N->getOperand(0);
  SDValue N1 = N->getOperand(1);
  EVT VT = N->getValueType(0); // TYPE OF the result
  unsigned VTBits = getScalarSize(VT);
  SolutionSet LeftSols = AvailableSolutions[N0.getNode()];
  SolutionSet RightSols = AvailableSolutions[N1.getNode()];
  
  unsigned ExtensionOpc = getExtensionChoice(ExtensionChoice);
  for(auto LeftSol : LeftSols){
    for(auto RightSol : RightSols){
      // check that LeftSol->getWidth == RightSol->getWidth &&
      // check that Leftsol->fillTypeWidth == RightSol->fillTypeWidth
      // check that LeftSol->fillType = Zeros and LeftSol->fillType = Garbage
      if(LeftSol->getUpdatedWidth() != RightSol->getUpdatedWidth() || 
         LeftSol->getFillTypeWidth() != RightSol->getFillTypeWidth() ||
         (LeftSol->getFillType() != ZEROS && hasTypeGarbage(RightSol->getFillType()) )){
        return AvailableSolutions[N];
      }
      unsigned char LeftSolWidth = LeftSol->getUpdatedWidth();
      for(int IntegerSize : IntegerSizes){
        EVT NewVT = EVT::getIntegerVT(Ctx, IntegerSize); 
        if(IntegerSize <= LeftSolWidth && 
           TLI.isOperationLegal(ISD::SIGN_EXTEND_INREG, NewVT )){ 
          continue;
        }
        unsigned cost = LeftSol->getCost() + RightSol->getCost() + 1;
        // add all integers that are bigger to the solutions
        WideningIntegerSolutionInfo *Expr = new WIA_EXTLO(ExtensionOpc,
          ExtensionChoice, LeftSol->getFillTypeWidth(), VTBits, IntegerSize, cost, N);
        Expr->addOperand(LeftSol); // TODO CHECK a SIGN_EXTEND_INREG has 2 operands??
        Expr->addOperand(RightSol);  // TODO same as above
        
        AvailableSolutions[N].push_back(Expr);
      } 
    } 
  }
  return closure(N);  
}
    
WideningIntegerArithmetic::SolutionSet WideningIntegerArithmetic::visitSUBSUME_FILL(
                          SDNode *Node){
  SolutionSet Sols;
  return Sols;
}
    
WideningIntegerArithmetic::SolutionSet WideningIntegerArithmetic::visitSUBSUME_INDEX(
                          SDNode *Node){
  
  // We implement this rule implicitly 
  // On every solution we consider the index n to stand
  // for all indices from n to w.
  SolutionSet Set;
  return Set;

}
    
WideningIntegerArithmetic::SolutionSet WideningIntegerArithmetic::visitNATURAL(
                          SDNode *Node){
 
  SolutionSet Sols = AvailableSolutions[Node];
  unsigned TargetWidth = getTargetWidth();
  for(WideningIntegerSolutionInfo *Sol : Sols){ 
    if(Sol->getFillType() == ANYTHING && TargetWidth == Sol->getUpdatedWidth())
      Sol->setFillType(ExtensionChoice); 
  }
  return Sols;
}

unsigned int WideningIntegerArithmetic::getTargetWidth(){
  const auto &TargetTriple = DAG.getTarget().getTargetTriple();
  switch (TargetTriple.getArch()){
    default: break;
    case Triple::riscv64: return 64;
    case Triple::riscv32: return 32;
    

  }
  return 64;
}
 

inline WideningIntegerArithmetic::BinOpWidth 
WideningIntegerArithmetic::createWidth(unsigned char op1, 
               unsigned char op2, unsigned dst){
  return std::make_tuple<unsigned char, unsigned char, unsigned char>(
        std::move(op1), std::move(op2), dst);
}


void WideningIntegerArithmetic::initOperatorsFillTypes(){

    // FillTypes are of the form op1 x op2 -> result
    // Stored in a tuple <op1, op2, result>
    UnaryFillTypeSet Popcnt, Neg, Com;  // TODO maybe there are others too. 
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
    UDivFillTypes.insert(std::make_tuple(SIGN, SIGN, SIGN));
    SDivFillTypes.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));
    
    FillTypesMap[ISD::SDIV] = SDivFillTypes;
    FillTypesMap[ISD::UDIV] = UDivFillTypes;

    FillTypeSet Eq, Ge, Geu, Gt, Gtu, Le, Leu, Lt, Ltu, Mod, Modu,
                Mul, Mulu, Ne, Or, Rem, Remu,
                Quot, Shl, Shra, Shrl, Sub, Xor;
    Eq.insert(std::make_tuple(SIGN, SIGN, SIGN));
    Eq.insert(std::make_tuple(ZEROS, ZEROS, ZEROS)); 
    FillTypesMap[ISD::CondCode::SETEQ + ISD::BUILTIN_OP_END] =  Eq;

    Ge.insert(std::make_tuple(SIGN, SIGN, SIGN));
    FillTypesMap[ISD::CondCode::SETGE + ISD::BUILTIN_OP_END] =  Ge;

    Geu.insert(std::make_tuple(SIGN, SIGN, SIGN));
    Geu.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));
    
    Gtu.insert(std::make_tuple(SIGN, SIGN, ZEROS));
    Gtu.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));
   
    Leu.insert(std::make_tuple(SIGN, SIGN, SIGN));
    Leu.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));
    
    Ltu.insert(std::make_tuple(SIGN, SIGN, ZEROS));
    Ltu.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));
    
    Mulu.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));
    // TODO what to do with Geu, Gtu, Leu, Ltu and Mulu ??   
 
    Gt.insert(std::make_tuple(SIGN, SIGN, SIGN));
    FillTypesMap[ISD::CondCode::SETGT + ISD::BUILTIN_OP_END] =  Gt;
    
    Le.insert(std::make_tuple(SIGN, SIGN, SIGN));
    FillTypesMap[ISD::CondCode::SETLE + ISD::BUILTIN_OP_END] =  Le;

    Lt.insert(std::make_tuple(SIGN, SIGN, SIGN));
    FillTypesMap[ISD::CondCode::SETLT + ISD::BUILTIN_OP_END] =  Lt;

    Mod.insert(std::make_tuple(SIGN, SIGN, SIGN));
    FillTypesMap[ISD::SREM] = SDivFillTypes;
    
    Modu.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));
    FillTypesMap[ISD::UREM] = SDivFillTypes;
     
    // MOD -> REMAINDER
    // DIV -> QUOTIENT
    
    Quot.insert(std::make_tuple(SIGN, SIGN, SIGN));
    // TODO what is the ISD OF THE QUOTIENT??  FillTypesMap[ISD::] = SDivFillTypes;
     
    Mul.insert(std::make_tuple(SIGN, SIGN, SIGN)); // IF NOT OVERFLOW OCCURS
    FillTypesMap[ISD::MUL] = Mul;
    // TODO check ISD for MULU does not exist or it's a MULHU
    Ne.insert(std::make_tuple(SIGN, SIGN, ZEROS));
    FillTypesMap[ISD::CondCode::SETNE + ISD::BUILTIN_OP_END] =  Ne;

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
    
inline bool WideningIntegerArithmetic::hasTypeGarbage(IntegerFillType fill){
  
  return 1;
}
  
inline bool WideningIntegerArithmetic::hasTypeT(IntegerFillType fill){
  return hasTypeGarbage(fill);
}

inline bool WideningIntegerArithmetic::hasTypeS(IntegerFillType fill){
  if(fill != ANYTHING && fill != ZEROS && fill != SIGN){
    return 0;
  }
  return 1;

}

void SelectionDAG::OptExtensions(CodeGenOpt::Level OptLevel){
  WideningIntegerArithmetic(*this).solve(OptLevel);
  std::cout << "Exiting OptExtensions " << '\n';
}






