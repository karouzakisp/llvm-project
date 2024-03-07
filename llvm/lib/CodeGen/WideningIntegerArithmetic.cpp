///===- WideningIntegerArithmetic ------------------------------------------===
//
//



#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/CodeGen/WideningIntegerArithmeticInfo.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Analysis/TargetTransformInfo.h"
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
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Value.h"
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

using namespace llvm;
#define PASS_NAME "WideningIntegerArithmetic"

#define DEBUG_TYPE "widening-integer-arithmetic"

#define ICMP_CONSTANT 2000
STATISTIC(NumSExtsDropped, "Number of ISD::SIGN_EXTEND nodes that were dropped"); 
STATISTIC(NumZExtsDropped, "Number of ISD::ZERO_EXTEND nodes that were dropped");
STATISTIC(NumAnyExtsDropped, "Number of ISD::ANY_EXTEND nodes that were dropped");
STATISTIC(NumTruncatesDropped, "Number of ISD::TRUNCATE nodes that were dropped"); 


namespace {


class WideningIntegerArithmetic : public FunctionPass {
	const TargetMachine *TM = nullptr;
  const TargetLowering *TLI = nullptr; 
	const TargetTransformInfo *TTI = nullptr;
	const DataLayout *DL = nullptr;
  LLVMContext *Ctx = nullptr;

  public:
		static char ID;
    WideningIntegerArithmetic(): FunctionPass(ID) {
      initializeWideningIntegerArithmeticPass(*PassRegistry::getPassRegistry());
    }
   
	 	StringRef getPassName() const override { return PASS_NAME; }	
    bool runOnFunction(Function &F) override;
  
    using isSolvedMap = DenseMap<Value*, bool>;
    using SolutionSet = SmallVector<WideningIntegerSolutionInfo *>;
    using SolutionSetParam = SmallVectorImpl<WideningIntegerSolutionInfo * >;    
    using AvailableSolutionsMap = DenseMap<Value* , SolutionSet>;
    
    using BinOpWidth = std::tuple<unsigned char, unsigned char, unsigned char>;
    using WidthsSet = SmallVector<BinOpWidth>;
    using OperatorWidthsMap = DenseMap<unsigned, WidthsSet>;
    using TargetWidthsMap = DenseMap<unsigned, OperatorWidthsMap>;
    
    using FillTypeSet = SmallSet<std::tuple<IntegerFillType, IntegerFillType, IntegerFillType>, 4>;
    using UnaryFillTypeSet = SmallSet<std::tuple<IntegerFillType, IntegerFillType> , 2>;
    
    using OperatorFillTypesMap = DenseMap<unsigned, FillTypeSet>;
    typedef WideningIntegerSolutionInfo* SolutionType;

		void getAnalysisUsage(AnalysisUsage &AU) const override{
			AU.setPreservesCFG();
			AU.addRequired<TargetPassConfig>();
			AU.addRequired<TargetTransformInfoWrapperPass>();
		}
  private:
    enum IntegerFillType ExtensionChoice;   
    
 
    // checks whether an Instruction in the Function F is visited and solved` 
    isSolvedMap SolvedInstructions;
	 	DenseMap<Value *, bool> VisitedInstructions;		
    // Holds all the available solutions 
    AvailableSolutionsMap AvailableSolutions;


    TargetWidthsMap TargetWidths;
				    
   
 
    DenseMap<unsigned, UnaryFillTypeSet>  UnaryFillTypesMap;
    DenseMap<unsigned, FillTypeSet>       FillTypesMap;


    UnaryFillTypeSet getUnaryFillTypes(unsigned Opcode);
    FillTypeSet getFillTypes(Instruction *Instr);
    inline IntegerFillType getOrNullFillType(
            FillTypeSet availableFillTypes, IntegerFillType Left, 
            IntegerFillType Right);

    SmallVector<WideningIntegerSolutionInfo *> visit_widening(Instruction *Instr);


    FillTypeSet getOperandFillTypes(Instruction *Instr);
  
    void setFillType(Type *IType);  
    inline bool IsSolved(Value *V);
    inline bool IsVisited(Value *V);

    bool addNonRedudant(SolutionSet &Solutions, 
                        WideningIntegerSolutionInfo* GeneratedSol);
    inline bool hasTypeGarbage(IntegerFillType fill);
    inline bool hasTypeT(IntegerFillType fill);
    inline bool hasTypeS(IntegerFillType fill);

    SolutionSet closure(Instruction *Instr);
    SolutionSet tryClosure(Instruction *Instr, bool changed);

    SolutionSet visitXOR(Instruction *Instr);
    SolutionSet visitInstruction(Instruction *Instr);
    SolutionSet visitBINOP(Instruction *Binop);
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
    void  			visitCONSTANT(ConstantInt *CI);
		SolutionSet visitPHI(Instruction *Instr);

		// Finds all the combinations of the legal 
		// solutions of all the Users of Instr 
		DenseMap<Value *, WideningIntegerSolutionInfo *> 
			getAllUsersLegalSolutions(Instruction *Instr);

		// iterates all the uses of an Instruction that is solved
    // and applies the updated Instruction to all the uses. 
		bool applyChain(DenseMap<Value *, WideningIntegerSolutionInfo*> );

    std::vector<unsigned short> IntegerSizes = {8, 16, 32, 64};

    unsigned int RegisterBitWidth = 0; 

    BinOpWidth createWidth(unsigned char op1, 
                   unsigned char op2, unsigned dst);

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
    inline unsigned  getExtensionChoice(enum IntegerFillType ExtChoice);   
    inline unsigned getExtCost(Instruction *Instr, 
                WideningIntegerSolutionInfo* Sol, unsigned short IntegerSize);
    void initOperatorsFillTypes();
    void printInstrSols(SolutionSet Sols, Instruction *Instr);
    inline Type* getTypeFromInteger(int Integer);
    inline short int getKnownFillTypeWidth(Instruction *Instr);

		bool inline isLegalAndMatching(WideningIntegerSolutionInfo *Sol1,
																	 WideningIntegerSolutionInfo *Sol2);
    bool mayOverflow(Instruction *Instr);
};
} // end anonymous namespace

char WideningIntegerArithmetic::ID = 0; 


  
void WideningIntegerArithmetic::printInstrSols(SolutionSet Sols, 
		Instruction *Instr){
  int i = 0;
	dbgs() << "AvailableSolutions Size. --> " << Sols.size() << "\n";
	for(WideningIntegerSolutionInfo *Solution : Sols){
    dbgs() << "----- Solution " << ++i << "\n" << (*Solution) << "\n";
  }
}


inline unsigned WideningIntegerArithmetic::getExtensionChoice(
		enum IntegerFillType ExtChoice){
  return ExtChoice == SIGN ? Instruction::SExt:
                             Instruction::ZExt;
}


bool WideningIntegerArithmetic::IsBinop(unsigned Opcode){
  switch(Opcode){
		case 	Instruction::Add:
		case 	Instruction::Sub:
	  case 	Instruction::Mul:
		case	Instruction::UDiv:
		case  Instruction::SDiv:
		case  Instruction::URem:
		case  Instruction::SRem:
		case  Instruction::LShr:
		case  Instruction::AShr:
		case  Instruction::Shl:
		case  Instruction::ICmp:
		case  Instruction::And:
		case  Instruction::Or:
		case  Instruction::Xor: 				return true;
		default : 								return false; 
  }
  return false;
}


// TODO all UNOPS are Intrinsics check IntrinsicInst 
bool WideningIntegerArithmetic::IsUnop(unsigned Opcode){
  switch(Opcode){
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
		case Instruction::Store: return true;
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
    dbgs() << " and Visiting Binop..\n"; 
    return visitBINOP(Instr);
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
    dbgs() << " and Visiting Extension...\n"; 
    return visitDROP_EXT(Instr);
  }
  else if(IsTruncate(Opcode)){
    TruncCounter++; 
    //dbgs() << " and Visiting Truncation ..\n"; 
    return visitDROP_TRUNC(Instr);
  }else if(IsPHI(Opcode)){
		return visitPHI(Instr);
	}
  else{
    dbgs() << "Could not found a solutionOpcode is " << Opcode << "\n";
    LLVM_DEBUG(dbgs() << "Opcode str is" << OpcodesToStr[Opcode] << "\n");
    // default solution so we can initialize all the nodes with some solution set.
    auto Sol = new WideningIntegerSolutionInfo(Opcode, Opcode,ANYTHING, 
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
	Value *VInstr = dyn_cast<Value>(Instr);
	dbgs() << "Inside visit_widening Instr Opcode is " << OpcodesToStr[Instr->getOpcode()] << '\n';
	VisitedInstructions[VInstr] = true;
  if(IsSolved(VInstr) ){
    LLVM_DEBUG(dbgs() << "Opcode str is " << OpcodesToStr[Instr->getOpcode()] << "\n"); 
    return AvailableSolutions[Instr]; 
  } 
  for (Value* V : Instr->operand_values() ){
	 	if(IsSolved(V)){
			continue;
		}	
    else if(auto *I = dyn_cast<Instruction>(V)){
		 	if(!IsVisited(I)){	
   	 		visit_widening(I);
			}
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
	// is ConstantInt visited here?
	auto CalcSolutions = visitInstruction(Instr);	
	// #TODO to get the opcode need to check and cast to a Instruction
	//LLVM_DEBUG(dbgs() << " Solved Instruction !! : " << U->getOpcode() << "\n");
  dbgs() << "Solved Instruction number !!: " << counter << "\n";
  SolvedInstructions[VInstr] = true; 
	counter++;
  dbgs () << "Solutions Size is --> " << CalcSolutions.size() << '\n';
  if(CalcSolutions.size() > 0){
    printInstrSols(CalcSolutions, Instr);
		if(auto search = AvailableSolutions.find(Instr); search != AvailableSolutions.end()){
      //dbgs() << "ERROR --!!!!!!!!!-===============" << "\n";
		}else{
    	AvailableSolutions[VInstr] = CalcSolutions;
		}
  }else{
    dbgs() << "This Inst does not have any solutions" << "\n";
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
WideningIntegerArithmetic::getFillTypes(Instruction *Instr){
  unsigned Opcode = Instr->getOpcode();
  dbgs() << "Opcode is " << Opcode << "    " << OpcodesToStr[Opcode] << "\n";
  unsigned IsdOpc = TLI->InstructionOpcodeToISD(Opcode);
  assert(IsBinop(Opcode) && "Not a binary operator." ); 
  if(auto *CI = dyn_cast<ICmpInst>(Instr)){
    unsigned Pred = CI->getPredicate();
    dbgs() << "Searching in " << ICMP_CONSTANT + Pred << "\n";
    return FillTypesMap[ICMP_CONSTANT + Pred];
  }

  return FillTypesMap[IsdOpc];
}

WideningIntegerArithmetic::UnaryFillTypeSet
WideningIntegerArithmetic::getUnaryFillTypes(unsigned Opcode){

  assert(IsUnop(Opcode) == true && "Not a unary operator to get the fillTypes");
  return UnaryFillTypesMap[TLI->InstructionOpcodeToISD(Opcode)];
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





void WideningIntegerArithmetic::setFillType(Type *IType){
	EVT VT = TLI->getValueType(*DL, IType);
	EVT NewVT = VT;
 /*	
	if(TLI->isSExtCheaperThanZExt(VT, NewVT)){ 
    ExtensionChoice = SIGN;
	}else{
		ExtensionChoice = ZEROS;
	}*/
	ExtensionChoice = SIGN;
}

bool WideningIntegerArithmetic::runOnFunction(Function &F){

  initOperatorsFillTypes();
	
  
  DL = &F.getParent()->getDataLayout();
	TM = &getAnalysis<TargetPassConfig>().getTM<TargetMachine>();
	const TargetSubtargetInfo *SubtargetInfo = TM->getSubtargetImpl(F);
	TLI = SubtargetInfo->getTargetLowering();
 	TTI = &getAnalysis<TargetTransformInfoWrapperPass>().getTTI(F);	
  Ctx = &F.getParent()->getContext();
	DL = &F.getParent()->getDataLayout();
  auto &FBB = F.getEntryBlock();
	if(FBB.empty()){
		dbgs() << "Function has no Instructions" << '\n';
		return false;
	}
	setFillType((&*(FBB.begin())->getType()));
	dbgs() << "ExtensionChoice is " << ExtensionChoice << '\n';
	
  for (BasicBlock &BB : F){
		for(Instruction &I : BB){
      dbgs() << "InstructionOpcode is  " << OpcodesToStr[I.getOpcode()] << '\n';
			Value *VInstr = dyn_cast<Value>(&I);
			if(IsSolved(VInstr))
				continue;

			if(isa<IntegerType>(I.getType())) {
				visit_widening(&I);  
			}
      dbgs() << "---------------------------------------------------------" << '\n';
		} 
  }
	return false;
} 
INITIALIZE_PASS_BEGIN(WideningIntegerArithmetic, DEBUG_TYPE,
                      PASS_NAME, false, false)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
INITIALIZE_PASS_END(WideningIntegerArithmetic, DEBUG_TYPE,
                      PASS_NAME, false, false)

inline bool WideningIntegerArithmetic::IsVisited(Value *V){
  auto IsVisited = VisitedInstructions.find(V);
  if(IsVisited != VisitedInstructions.end())
    return true;
  
  return false;
}

inline bool WideningIntegerArithmetic::IsSolved(Value *V){
  auto IsSolved = SolvedInstructions.find(V);
  if(IsSolved != SolvedInstructions.end())
    return true;
  
  return false;
}


WideningIntegerArithmetic::SolutionSet
WideningIntegerArithmetic::tryClosure(Instruction *Instr, bool Changed){
  auto FillsSize = 0;
  unsigned Opcode = Instr->getOpcode();
  dbgs() << "Inside try closure. " << "\n";
  if(IsBinop(Opcode)){
    FillTypeSet Fills = getFillTypes(Instr);
    FillsSize = Fills.size();
  }
  else if(IsUnop(Opcode)){
    UnaryFillTypeSet Fills = getUnaryFillTypes(Opcode);
    FillsSize = Fills.size();
  }
  
  if(FillsSize  > 1 && Changed )
    return closure(Instr);
    
  
  return AvailableSolutions[dyn_cast<Value>(Instr)];
  
}


WideningIntegerArithmetic::SolutionSet 
WideningIntegerArithmetic::closure(Instruction *Instr){
	Value *IV = dyn_cast<Value>(Instr);
  auto Sols = AvailableSolutions[IV];
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

      bool Added = addNonRedudant(AvailableSolutions[IV], PossibleSol);
      //printInstrSols(AvailableSolutions[Node], Node); 
      if(Added){
        Changed = true;
        LLVM_DEBUG(dbgs() << "Added Possible Solution " << '\n');
        printInstrSols(AvailableSolutions[IV], Instr);
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

  return getFillTypes(Instr);

	// TODO handle the case below.
  // TODO right an overflow analysis.
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



void WideningIntegerArithmetic::visitCONSTANT(ConstantInt *CI){
  SolutionSet Sols;
  unsigned bitWidth = CI->getBitWidth();
	Value *VCI = dyn_cast<Value>(CI);
  auto Sol = new WIA_CONSTANT(CONSTANT_INT_OPC, CONSTANT_INT_OPC,
      ExtensionChoice, bitWidth, RegisterBitWidth, RegisterBitWidth, 0, VCI);
  AvailableSolutions[VCI].push_back(Sol);
}
  
WideningIntegerArithmetic::SolutionSet 
WideningIntegerArithmetic::visitSTORE(Instruction *Instr){
  SolutionSet Sols;
  Value *VI = dyn_cast<Value>(Instr);
  unsigned char N0Bits = Instr->getType()->getScalarSizeInBits();
  auto N0Sols = AvailableSolutions[Instr->getOperand(0)];
  for(WideningIntegerSolutionInfo *Sol : N0Sols ){
    auto StoreSol = new WIA_STORE(Instr->getOpcode(), Instr->getOpcode(), 
        Sol->getFillType(), N0Bits, N0Bits, N0Bits, Sol->getCost(), VI);
    Sols.push_back(StoreSol);
  }
  return Sols;   
}



WideningIntegerArithmetic::SolutionSet 
WideningIntegerArithmetic::visitLOAD(Instruction *Instr){

  SolutionSet Sols;


	// TODO check
  //IntegerFillType FillType = IntegerFillType::UNDEFINED; 
  IntegerFillType FillType = ExtensionChoice;
  Value *VInstr = dyn_cast<Value>(Instr);
  unsigned int Width = Instr->getType()->getScalarSizeInBits();
  int FillTypeWidth = getKnownFillTypeWidth(Instr);
  unsigned Opc = Instr->getOpcode(); 
  auto WIALoad = new WIA_LOAD(Opc, Opc, FillType, FillTypeWidth,
                Width, FillTypeWidth, 0, VInstr);

  SolutionSet WidenSols;
	WidenSols.push_back(WIALoad);
  unsigned ExtensionOpc = getExtensionChoice(FillType);
	for(int IntegerSize : IntegerSizes){
		EVT NewVT = EVT::getIntegerVT(*Ctx, IntegerSize); 
		if(IntegerSize <= FillTypeWidth) // TODO check
			continue;
		if(!TLI->isOperationLegal(ExtensionOpc, NewVT))
			continue;
		// Results to a widened expr based on ExtensionOpc
		WideningIntegerSolutionInfo *Widen = new WIA_WIDEN(
			Opc, ExtensionOpc, FillType, FillTypeWidth, Width,
			IntegerSize, 1, VInstr);
		Widen->addOperand(WIALoad); 
		WidenSols.push_back(Widen); 
	}	
 	return WidenSols; 
}
  


// TODO not done yet
inline bool
WideningIntegerArithmetic::mayOverflow(Instruction *Instr){
	
  unsigned Opcode;
  Opcode = Instr->getOpcode();
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
	return false;
} 


bool inline WideningIntegerArithmetic::isLegalAndMatching(
		WideningIntegerSolutionInfo *Sol1, WideningIntegerSolutionInfo *Sol2){
	return Sol1->getUpdatedWidth() == Sol2->getUpdatedWidth();
}

DenseMap<Value *, WideningIntegerSolutionInfo *>
WideningIntegerArithmetic::getAllUsersLegalSolutions(Instruction *Instr){

  SmallVector<Value *, 4> VUsers;
	std::vector<DenseMap<Value *, WideningIntegerSolutionInfo *>> Combinations;
	DenseMap<Value *, WideningIntegerSolutionInfo *> BestCombination;	
  if(Instr->getNumUses() == 0 ){
    return BestCombination;
  }
  for(auto &Use : Instr->uses()){
    Value *VUser1 = Use.getUser();
    VUsers.push_back(VUser1);
  }
  Value *VUser = VUsers[0];
  SmallVector<Value *, 4> Users_without = VUsers;
  auto UserPos = std::find(VUsers.begin(), VUsers.end(), VUser);
  Users_without.erase(UserPos);
  for(WideningIntegerSolutionInfo *Sol : AvailableSolutions[VUser]){
    bool all_matching = true;
		DenseMap<Value *, WideningIntegerSolutionInfo *> OneCombination;
		OneCombination[VUser] = Sol;
    for(Value *VUser2 : Users_without){
      for(WideningIntegerSolutionInfo *Sol2 : AvailableSolutions[VUser2]){
        if(isLegalAndMatching(Sol, Sol2)){
					OneCombination[VUser2] = Sol2;	
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
	int min_cost = INT_MAX;
	for(auto ValuesSolsMap : Combinations){
		int sum = 0;
  	for(const auto& it : ValuesSolsMap){
			sum += it.second->getCost();	
		}
		if(sum < min_cost){
			min_cost = sum;
			BestCombination = ValuesSolsMap;
		}
	}	
	return BestCombination;
}



inline short int WideningIntegerArithmetic::getKnownFillTypeWidth(
		Instruction *Instr){
	if(!isa<Value>(Instr)){
		return -1;
	}	
	Value *V = dyn_cast<Value>(Instr);
	KnownBits Known = computeKnownBits(V, *DL);
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

WideningIntegerArithmetic::SolutionSet
WideningIntegerArithmetic::visitPHI(Instruction *Instr){
	// we know that Instr is PHINode from isPHI method so we can cast it.
	auto *PhiInst = dyn_cast<PHINode>(Instr);
	int NumIncValues = PhiInst->getNumIncomingValues();
  SolutionSet Combinations;
	SmallVector<Value*, 4> IncomingValues;
	
	for(int i = 0; i < NumIncValues; i++){
		Value *V = PhiInst->getIncomingValue(i);
		IncomingValues.push_back(V);
		if(IsSolved(V)){
			continue;
		}
    // TODO check visit_widening here? or visitInstruction is enough?
		if(auto *VI = dyn_cast<Instruction>(V)){
			visitInstruction(VI);
		}
		if(auto *CI = dyn_cast<ConstantInt>(V)){
			visitCONSTANT(CI);
		}
	}
  Value *SelectedValue = IncomingValues[0];
  SmallVector<Value *, 4> ValuesWithout = IncomingValues;
  auto ValuePos = std::find(IncomingValues.begin(), IncomingValues.end(), 
			IncomingValues[0]);
	dbgs() << "Incoming Values [0] " << IsSolved(IncomingValues[0]) << "\n";
	dbgs() << "incoming values size is " << IncomingValues.size() <<'\n';
	dbgs() << "Values Without size is " << ValuesWithout.size() <<'\n';
  ValuesWithout.erase(ValuesWithout.begin());
	dbgs() << "After erase" << '\n';
  for(WideningIntegerSolutionInfo *Sol : AvailableSolutions[SelectedValue]){
		Combinations.push_back(Sol);
    for(Value *Val2 : IncomingValues){
      for(WideningIntegerSolutionInfo *Sol2 : AvailableSolutions[Val2]){
				// TODO check can the combination have different fillType? Probably not.
        if(isLegalAndMatching(Sol, Sol2)){
          Combinations.push_back(Sol2);
				}      
			}
    }
  }
	AvailableSolutions[dyn_cast<Value>(Instr)] = Combinations;
  return Combinations;
		
}

	
	// check IsRedudant uses fillTypeWidth 
// Return all the solution based on binop rules op1 x op2 -> W
WideningIntegerArithmetic::SolutionSet 
WideningIntegerArithmetic::visitBINOP(Instruction *Binop){
	
  SolutionSet Sols;
 
 	Value *VBinop = dyn_cast<Value>(Binop);	
	Value *V0 = Binop->getOperand(0);
  Value *V1 = Binop->getOperand(1);
	
  if(Binop->getOpcode() == Instruction::Shl){
    if(auto *I = dyn_cast<Instruction>(V0)){
		 	if(auto *CI = dyn_cast<ConstantInt>(V1)){
				unsigned LeftShiftC = CI->getZExtValue(); 
				if(I->getOpcode() == Instruction::AShr ){
					auto ShlV1 = dyn_cast<Instruction>(I);
					if(auto *C2 = dyn_cast<ConstantInt>(ShlV1)){
						unsigned ARightShiftC = C2->getZExtValue();
						if(ARightShiftC == LeftShiftC){
							return visitEXTLO(Binop);
						}
					}
				} 
			} 
		}
	}
  
  bool AddedSol = false; 
  // get All the available solutions from the operands // 
	// TODO how to handle carry and borrow? 
  SmallVector<WideningIntegerSolutionInfo*> LeftSols = 
                                              AvailableSolutions[V0];
  SmallVector<WideningIntegerSolutionInfo*> RightSols = 
                                              AvailableSolutions[V1];
  
  unsigned Opcode = Binop->getOpcode();
	unsigned int InstrWidth = Binop->getType()->getScalarSizeInBits();
  unsigned BinopFillTypeWidth = getKnownFillTypeWidth(Binop);
  dbgs() << "Instr Width is " << InstrWidth << '\n';
  dbgs() << "FillTypeWidth is " << BinopFillTypeWidth << '\n';
  WideningIntegerSolutionInfo *defaultSol = new WIA_BINOP(Opcode, Opcode, 
			ExtensionChoice, 
			/*FillTypeWidth*/ BinopFillTypeWidth, /* OldWidth*/ InstrWidth , 
			InstrWidth/*NewWidth*/ , /* Cost */ 0, VBinop); 
  AvailableSolutions[VBinop].push_back(defaultSol);

  LLVM_DEBUG(dbgs() << "Operand 0 is --> " << V0->getName() << "\n");
  LLVM_DEBUG(dbgs() << "Operand 1 is --> " << V1->getName() << "\n");
  LLVM_DEBUG(dbgs() << "Operand 0 Solutions Size is --> " << LeftSols.size() << "\n");
  LLVM_DEBUG(dbgs() << "Operand 1 Solutions Size is --> " << RightSols.size() << "\n");
  dbgs() << "Operand 0 Solutions Size is --> " << LeftSols.size() << "\n";
  dbgs() << "Operand 1 Solutions Size is --> " << RightSols.size() << "\n";
  FillTypeSet OperandFillTypes = getOperandFillTypes(Binop);
  // A function that combines solutions from operands left and right
  for (WideningIntegerSolutionInfo *leftSolution : LeftSols){
    for(WideningIntegerSolutionInfo *rightSolution : RightSols){
      // Test whether current binary operator has the available
      // fill type provided by leftSolution and rightSolution
      auto leftFill = leftSolution->getFillType();
      auto rightFill = rightSolution->getFillType();
      dbgs() << "Left Fill type is " << leftFill << "\n";
      dbgs() << "right Fill type is " << rightFill << "\n";
      auto FillType = getOrNullFillType((OperandFillTypes),
                                  leftSolution->getFillType(),
                                  rightSolution->getFillType());
      if(FillType == UNDEFINED){
        dbgs() << "Fill type is undefined skipping solution" << "\n";
        continue;
      }
      //dbgs() << "Passed FillType for combination --> " << FillType << "\n";
      unsigned int w1 = leftSolution->getUpdatedWidth();
      unsigned int w2 = rightSolution->getUpdatedWidth();
      if(w1 != w2){
       	LLVM_DEBUG(dbgs() << "Width " << w1 << "and Width " << w2 << " are not the same skipping solution.." << "\n");
       	dbgs() << "Width " << w1 << "and Width " << w2 << " are not the same skipping solution.." << "\n";
        dbgs() << "Left Solution --> " << *leftSolution << '\n';
        dbgs() << "Right Solution --> " << *rightSolution << '\n';
			 	continue;
			}
      if(w1 == 0 || w2 == 0){
        dbgs() << "Left Solution --> " << *leftSolution << '\n';
        dbgs() << "Right Solution --> " << *rightSolution << '\n';
        dbgs() << "Error here be aware. ------ " << '\n';
        continue;
        //dbgs() << "EVT get String " << LD->getMemoryVT().getEVTString() << '\n'; 
      }
      //dbgs() << "The widths are the same and we continue--> " << w1 << "\n"; 
      EVT NewVT = EVT::getIntegerVT(*Ctx, w1); 
      if(!TLI->isOperationLegal(TLI->InstructionOpcodeToISD(Opcode), NewVT)){
				LLVM_DEBUG(dbgs() << "Width: " << w1 << " Is not legal for binop" << "\n");
				dbgs() << "Width: " << w1 << " Is not legal for binop " << OpcodesToStr[Opcode] << "\n";
        continue;
			}
      LLVM_DEBUG(dbgs() << "The Operation is legal for that newVT--> "<<  w1 << "\n");
      dbgs() << "The Operation is legal for that newVT--> "<<  w1 << "\n";
      // w1 x w2 --> w all widths are the same at this point
      // and there is a LegalOperation for that Opcode
      unsigned char UpdatedWidth = w1;

      unsigned char Cost = leftSolution->getCost() + rightSolution->getCost();
      unsigned char FillTypeWidth = getKnownFillTypeWidth(Binop); 
                    // this Target of this Binary operator of the form
                    // width1 x width2 = newWidth
                    // for example on rv64 we have addw
                    // that is of the form i32 x i32 -> i32 
                    // and stored in a sign extended format to i64

      auto Sol = new WIA_BINOP(Opcode, Opcode, FillType, 
          FillTypeWidth, w1, UpdatedWidth, Cost, VBinop);
      Sol->addOperand(leftSolution);
      Sol->addOperand(rightSolution);
      LLVM_DEBUG(dbgs() << "Adding Sol with cost " << Cost << " and width " << UpdatedWidth << "\n");
      dbgs() << "Adding Sol with cost " << Cost << " and width " << UpdatedWidth << "\n";
      AvailableSolutions[VBinop].push_back(Sol);
      
			AddedSol = true; 
    }
  }
  //dbgs() << "Calling closure here.\n"; 
  // call closure if multiple FillTypes or Multiple Binop Widths
  return tryClosure(Binop, AddedSol); 
}

std::list<WideningIntegerSolutionInfo*> WideningIntegerArithmetic::visitFILL(
                          Instruction *Instr){
  
  unsigned Width = Instr->getType()->getScalarSizeInBits();
  unsigned char FillTypeWidth = getKnownFillTypeWidth(Instr); 
 	Value *VInstr = dyn_cast<Value>(Instr);

  SolutionSet Sols = AvailableSolutions[VInstr];
	std::list<WideningIntegerSolutionInfo*> Solutions;
  for(WideningIntegerSolutionInfo *Sol : Sols){
    if(Sol->getOpcode() == Instruction::SExt ||
       llvm::ISD::isExtOpcode(Sol->getOpcode()) ||
         Sol->getOpcode() == Instruction::Trunc ) 
     continue;    //FILL is a ISD::SIGN_EXTEND_INREG
    // WIA_FILL extends the least significant *Width* bits of Instr
    // to targetWidth
    for(auto IntegerSize : IntegerSizes ){
      EVT NewVT = EVT::getIntegerVT(*Ctx, IntegerSize);
      // TODO should I add isTypeLegal for the NewVT
      // meaning is TLi.isOperationLegal enough for the NewVT? 
      if(IntegerSize < FillTypeWidth || IntegerSize < (int)Sol->getWidth() ||
         TLI->InstructionOpcodeToISD(Sol->getOpcode()) == ISD::TRUNCATE || 
         !TLI->isOperationLegal(ISD::SIGN_EXTEND_INREG, NewVT)){
        continue;
			} 
      WideningIntegerSolutionInfo *Fill = new WIA_FILL(
       FILL_INST_OPC , FILL_INST_OPC, ExtensionChoice, FillTypeWidth, 
       Width, IntegerSize , Sol->getCost() + 1, VInstr ); 
      Fill->addOperand(Sol);
      Solutions.push_front(Fill);  
    }
  }
	return Solutions;	  
}

inline Type* WideningIntegerArithmetic::getTypeFromInteger(
                                int Integer){
  Type* Ty;
  switch(Integer){
    case 8: Ty = Type::getInt8Ty(*Ctx); break;
    case 16: Ty = Type::getInt16Ty(*Ctx); break;
    case 32: Ty = Type::getInt32Ty(*Ctx); break;
    case 64: Ty = Type::getInt64Ty(*Ctx); break; 
  }
  return Ty; 
}

inline unsigned WideningIntegerArithmetic::getExtCost(Instruction *Instr,
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
 	Value *VInstr = dyn_cast<Value>(Instr);
  SolutionSet Sols = AvailableSolutions[VInstr];
  LLVM_DEBUG(dbgs() << "SolutionsSize is --> " << Sols.size() << '\n');
	std::list<WideningIntegerSolutionInfo*> Solutions;
  for(WideningIntegerSolutionInfo *Sol : Sols){
    LLVM_DEBUG(dbgs() << "Inside visitWiden Iterating Sol --> " << Sol << '\n');
    unsigned IsdOpc = TLI->InstructionOpcodeToISD(Sol->getOpcode());
    if(llvm::ISD::isExtOpcode(Sol->getOpcode()) || 
       IsdOpc == ISD::SIGN_EXTEND_INREG || 
       IsdOpc == ISD::TRUNCATE ) 
      continue;
    LLVM_DEBUG(dbgs() << "Passed first if --> " << Sol << '\n');
    for(int IntegerSize : IntegerSizes){
      EVT NewVT = EVT::getIntegerVT(*Ctx, IntegerSize); 
      if(IntegerSize < FillTypeWidth || IntegerSize < Sol->getWidth() ||
         !TLI->isOperationLegal(ExtensionOpc, NewVT)){
        continue;
			}
      LLVM_DEBUG(dbgs() << "Adding all Solution with IntegerSize --> " << IntegerSize << '\n');
      unsigned cost = getExtCost(Instr, Sol, IntegerSize); 
      // Results to a widened expr based on ExtensionOpc
      WideningIntegerSolutionInfo *Widen = new WIA_WIDEN(
        ExtensionOpc, ExtensionOpc, ExtensionChoice, FillTypeWidth, Width,
        IntegerSize, cost , VInstr);
      Widen->addOperand(Sol); 
      Solutions.push_front(Widen);  
    } 
  }
	return Solutions;	
}
    

std::list<WideningIntegerSolutionInfo*> WideningIntegerArithmetic::visitWIDEN_GARBAGE(
      Instruction *Instr){

  unsigned ExtensionOpc = ISD::ANY_EXTEND;  // Results to a garbage widened
  unsigned char FillTypeWidth = getKnownFillTypeWidth(Instr); 
 	Value *VInstr = dyn_cast<Value>(Instr);
  auto Sols = AvailableSolutions[VInstr];
  LLVM_DEBUG(dbgs() << "SolutionsSize is --> " << Sols.size() << '\n');
	std::list<WideningIntegerSolutionInfo*> Solutions;
  // TODO add isExtFree and modify cost accordingly 
  for(WideningIntegerSolutionInfo *Sol : Sols){
    LLVM_DEBUG(dbgs() << "Inside GarbageWiden Iterating Sol --> " << Sol << '\n');
    unsigned IsdOpc = TLI->InstructionOpcodeToISD(Sol->getOpcode());
    if(llvm::ISD::isExtOpcode(IsdOpc) || 
       // TODO how to check for this? Is it needed also ? Sol->getOpcode() == ISD::SIGN_EXTEND_INREG ) ||
       IsdOpc == Instruction::Trunc) 
      continue;
    LLVM_DEBUG(dbgs() << "Passed first if --> " << Sol << '\n');
    for(int IntegerSize : IntegerSizes){
      EVT NewVT = EVT::getIntegerVT(*Ctx, IntegerSize); 
      if(IntegerSize < FillTypeWidth || IntegerSize < Sol->getWidth() ||  
         !TLI->isOperationLegal(ISD::ANY_EXTEND, NewVT)){
        continue;
			}
      // TODO check do we need isTypeLegal? 
      unsigned cost = getExtCost(Instr, Sol, IntegerSize); 
      LLVM_DEBUG(dbgs() << "Adding GarbageWiden --> " << IntegerSize << '\n');
     
      WideningIntegerSolutionInfo *GarbageWiden = new WIA_WIDEN(
        ExtensionOpc, ExtensionOpc, ANYTHING, FillTypeWidth,  Sol->getWidth(), IntegerSize,
        cost , VInstr);
      GarbageWiden->addOperand(Sol);
      Solutions.push_front(GarbageWiden);    
    }
  }
	return Solutions;	
}
    
std::list<WideningIntegerSolutionInfo*> WideningIntegerArithmetic::visitNARROW(
    Instruction *Instr){
  
 
  unsigned Width = Instr->getType()->getScalarSizeInBits();
  unsigned char FillTypeWidth = getKnownFillTypeWidth(Instr); 
 	Value *VInstr = dyn_cast<Value>(Instr);
  // TODO check isTruncateFree on some targets and widths.
  // Not sure the kinds of truncate of the Target will determine it.
  unsigned ExtensionOpc = getExtensionChoice(ExtensionChoice);
  std::vector<short int> TruncSizes = {8, 16, 32};
 
  auto Sols = AvailableSolutions[VInstr];
  LLVM_DEBUG(dbgs() << "SolutionsSize is --> " << Sols.size() << '\n');
	std::list<WideningIntegerSolutionInfo*> Solutions;

  for(auto Sol : Sols){ 
    LLVM_DEBUG(dbgs() << "Inside visitNarrow " << '\n');
    unsigned IsdOpc = TLI->InstructionOpcodeToISD(Sol->getOpcode());
    if(llvm::ISD::isExtOpcode(Sol->getOpcode()) || 
        IsdOpc == ISD::SIGN_EXTEND_INREG  ||
        IsdOpc == ISD::TRUNCATE ) 
      continue;
    for(int IntegerSize : TruncSizes){
      EVT NewVT = EVT::getIntegerVT(*Ctx, IntegerSize);
      if(!TLI->isOperationLegal(ISD::TRUNCATE, NewVT) ||
				 IntegerSize < FillTypeWidth || IntegerSize >= Sol->getWidth()) {
				continue;
			}
    
      LLVM_DEBUG(dbgs() << "Getting type for EVT for Left child " << '\n');
      unsigned cost = Sol->getCost();
      Type* Ty1 = Instr->getType(); 
      Type* Ty2 = getTypeFromInteger(IntegerSize);
      if(!TLI->isTruncateFree(Ty1, Ty2))
        cost = cost + 1;
      LLVM_DEBUG(dbgs() << "Adding new Wia Narrow" << IntegerSize <<  '\n');
      WideningIntegerSolutionInfo *Trunc = new WIA_NARROW(ExtensionOpc, 
        ExtensionOpc, ExtensionChoice, // Will depend on available Narrowing , 
        FillTypeWidth, Width, IntegerSize, Sol->getCost() + 1 , VInstr);
      Trunc->addOperand(Sol);
      Solutions.push_front(Trunc);
    }
  } 
	return Solutions;	
}
    
WideningIntegerArithmetic::SolutionSet WideningIntegerArithmetic::visitDROP_EXT(
                          Instruction *Instr){
  Value *N0 = Instr->getOperand(0);
	// TODO check can an operand of constantInt be extended?
	// does it need to be extended? or we can be sure that N0 is never a ConstantInt? 
  unsigned char ExtendedWidth = Instr->getType()->getScalarSizeInBits();
  unsigned char OldWidth = N0->getType()->getScalarSizeInBits();  
  SolutionSet ExprSolutions = AvailableSolutions[N0];
  int Opc;
  if(auto Iop = dyn_cast<Instruction>(N0)){
    Opc = TLI->InstructionOpcodeToISD(Iop->getOpcode());
  }else if(isa<ConstantInt>(N0)){
    Opc = -1; //  Indicates a ConstantIn
  }else{
    Opc = -2; // Indicates unknown opc 
  }
  SolutionSet Sols;
  switch(Instr->getOpcode()){
    case Instruction::SExt: ++NumSExtsDropped; break;
    case Instruction::ZExt: ++NumZExtsDropped; break;
    default: dbgs() << "Check this is not an Instruction!" << '\n';
  }
  LLVM_DEBUG(dbgs() << "Trying to drop extension in Solutions" << '\n');
  LLVM_DEBUG(dbgs() << "Expr Solutions Size is " << ExprSolutions.size() << '\n');
  if(Opc >= 0){
    LLVM_DEBUG(dbgs() << "Opc of Instr->Op0 is " << Opc << " Opc string is " << OpcodesToStr[Opc] << '\n');
  }
  LLVM_DEBUG(dbgs() << "Opc of Instr is " << Instr->getOpcode() << "Opc of Instr str is " << OpcodesToStr[Instr->getOpcode()] << '\n');
  LLVM_DEBUG(dbgs() << "ExtendedWidth of Node is " << ExtendedWidth << '\n');
  LLVM_DEBUG(dbgs() << "NewWidth of Node is " << OldWidth << '\n');
  LLVM_DEBUG(dbgs() << "Opc of Node is " << Instr->getOpcode() << "Opc of Node str is " << OpcodesToStr[Instr->getOpcode()] << '\n');
  for(auto Solution : ExprSolutions){ 
  // We simply drop the extension and we will later see if it's needed.
    LLVM_DEBUG(dbgs() << "Drop extension in Solutions" << '\n'); 
  	unsigned char FillTypeWidth = Solution->getFillTypeWidth();  
    WideningIntegerSolutionInfo *Expr = new WIA_DROP_EXT(
        Instr->getOpcode(), Solution->getNewOpcode(), ExtensionChoice, 
        FillTypeWidth, ExtendedWidth /*OldWidth*/, 
      OldWidth/*NewWidth*/, Solution->getCost(), Instr);
    // TODO check OldWidth must come from The operand of the Extension
    // or from The Solutions? PRobably thes solutions. 
    Expr->setOperands(Solution->getOperands());
    Sols.push_back(Expr); 
  }
  return Sols; 
}

bool WideningIntegerArithmetic::applyChain(
		DenseMap<Value *, WideningIntegerSolutionInfo*> BestSolsUsersCombination){
	bool Changed = false;
	
	for(auto &Combination : BestSolsUsersCombination){
		// All users are solved at this point
		Value *VComb = Combination.first;
		WideningIntegerSolutionInfo *Sol = Combination.second;
		if(Sol->getKind() == WIAK_UNKNOWN){
			dbgs() << "Best solution has Unknown kind..cannot apply it yet, ";
			if(auto *I = dyn_cast<Instruction>(VComb)){
				dbgs() << "Opcode is " << I->getOpcode() << '\n';
			}
			if(isa<ConstantInt>(VComb)){
				dbgs() << "Opcode is Constant" << '\n';
			}
			return Changed;
		}
	}
	for(auto &Combination : BestSolsUsersCombination){
		// All users are solved at this point
		Value *VComb = Combination.first;
		WideningIntegerSolutionInfo *Sol = Combination.second;
		Value *NewV = VComb;
		assert(isa<Instruction>(NewV) || isa<ConstantInt>(NewV));
		NewV->mutateType(getTypeFromInteger(Sol->getUpdatedWidth()));
		// TODO check do we need to mutate the type of the operands?
		VComb->replaceAllUsesWith(NewV);
		// we need to have all the Combinations of the LegalUsersSolutions..
    Changed = true;	
				
	}
	return Changed;
}
  
WideningIntegerArithmetic::SolutionSet 
  WideningIntegerArithmetic::visitDROP_TRUNC(Instruction *Instr){
  SolutionSet Sols;

  assert(Instr->getOpcode() == Instruction::Trunc && 
                              "Not an extension to drop here");  
  // TRUNCATE has only 1 operand
	
	// TODO check can an operand of Truncation be a ConstantInt?
	// does it need to be extended? or we can be sure that N0 is never a ConstantInt? 
  Value *N0 = Instr->getOperand(0);
  if(auto *I = dyn_cast<Instruction>(N0)){
    if(I->getOpcode() == Instruction::SExt){
      return visitEXTLO(Instr);
    }
  }
  SmallVector<WideningIntegerSolutionInfo*> ExprSolutions = 
                                              AvailableSolutions[N0];
  
  if(ExprSolutions.size() == 0 ){
    if(auto I = dyn_cast<Instruction>(N0)){
      LLVM_DEBUG(dbgs() << "Child of Truncate with opc " << I->getOpcode());
    }
    LLVM_DEBUG(dbgs() << " has no Solutions\n");
    return Sols; 
  }

  // NewWidth is the width of the value before the truncation
  unsigned char NewWidth = N0->getType()->getScalarSizeInBits(); 
  unsigned char TruncatedWidth = Instr->getType()->getScalarSizeInBits(); 
  // We simply drop the truncation and we will later see if it's needed.
  NumTruncatesDropped++; 
  for(auto Sol : ExprSolutions){
    WideningIntegerSolutionInfo *Expr = new WIA_DROP_LOCOPY(
        Instr->getOpcode(), Sol->getOpcode(),
      Sol->getFillType(), Sol->getFillTypeWidth(), TruncatedWidth, 
      NewWidth, Sol->getCost(), dyn_cast<Value>(Instr));
    Expr->setOperands(Sol->getOperands());   
    Sols.push_back(Expr); 
  }
  // We simply drop the truncation and we will later see if it's needed.
  NumTruncatesDropped++; 
 
  // We simply drop the truncation and we will later see if it's needed.
  for(auto Sol : ExprSolutions){ 
    WideningIntegerSolutionInfo *Expr = new WIA_DROP_LOIGNORE(
        Instr->getOpcode(), Sol->getOpcode(),
      Sol->getFillType(), Sol->getFillTypeWidth(), TruncatedWidth, 
      NewWidth, Sol->getCost(), Instr);
    Expr->setOperands(Sol->getOperands());
    Sols.push_back(Expr); 
  }
  return Sols;  
}


WideningIntegerArithmetic::SolutionSet WideningIntegerArithmetic::visitEXTLO(
                          Instruction *Instr){
  SolutionSet CalcSols;
  Value *N0 = Instr->getOperand(0);
  Value *N1 = Instr->getOperand(1);
  Value *VI = dyn_cast<Value>(Instr);
  // TODO check! is VTBits correct?
  unsigned VTBits = N0->getType()->getScalarSizeInBits(); 
  SolutionSet LeftSols = AvailableSolutions[N0];
  SolutionSet RightSols = AvailableSolutions[N1];
  
  unsigned ExtensionOpc = getExtensionChoice(ExtensionChoice);
  for(auto LeftSol : LeftSols){
    for(auto RightSol : RightSols){
      // check that LeftSol->getWidth == RightSol->getWidth &&
      // check that Leftsol->fillTypeWidth == RightSol->fillTypeWidth
      // check that LeftSol->fillType = Zeros and LeftSol->fillType = Garbage
      if(LeftSol->getUpdatedWidth() != RightSol->getUpdatedWidth() || 
         LeftSol->getFillTypeWidth() != RightSol->getFillTypeWidth() ||
         (LeftSol->getFillType() != ZEROS && hasTypeGarbage(RightSol->getFillType()) )){
        return AvailableSolutions[VI];
      }
      unsigned char LeftSolWidth = LeftSol->getUpdatedWidth();
      for(int IntegerSize : IntegerSizes){
        EVT NewVT = EVT::getIntegerVT(*Ctx, IntegerSize); 
        if(IntegerSize <= LeftSolWidth && 
           TLI->isOperationLegal(ISD::SIGN_EXTEND_INREG, NewVT )){ 
          continue;
        }
        unsigned cost = LeftSol->getCost() + RightSol->getCost() + 1;
        // add all integers that are bigger to the solutions
        WideningIntegerSolutionInfo *Expr = new WIA_EXTLO(Instr->getOpcode(), 
            ExtensionOpc, ExtensionChoice, 
            LeftSol->getFillTypeWidth(), VTBits, 
          IntegerSize, cost, VI);
        Expr->addOperand(LeftSol);         
        AvailableSolutions[VI].push_back(Expr);
      } 
    } 
  }
  return closure(Instr);  
}
    
WideningIntegerArithmetic::SolutionSet WideningIntegerArithmetic::visitSUBSUME_FILL(
                          Instruction *Instr){
  SolutionSet Sols;
  return Sols;
}
    
WideningIntegerArithmetic::SolutionSet WideningIntegerArithmetic::visitSUBSUME_INDEX(
                          Instruction *Instr){
  
  // We implement this rule implicitly 
  // On every solution we consider the index n to stand
  // for all indices from n to w.
  SolutionSet Set;
  return Set;

}
    
WideningIntegerArithmetic::SolutionSet WideningIntegerArithmetic::visitNATURAL(
                          Instruction *Instr){
 
  Value *VI = dyn_cast<Value>(Instr);
  SolutionSet Sols = AvailableSolutions[VI];

  for(WideningIntegerSolutionInfo *Sol : Sols){
	 	Type *NewType = getTypeFromInteger(Sol->getUpdatedWidth());
    if(Sol->getFillType() == ANYTHING && TTI->isTypeLegal(NewType) && 
				Sol->getFillTypeWidth() == Sol->getUpdatedWidth())
      Sol->setFillType(ExtensionChoice); 
  }
  return Sols;
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
    UDivFillTypes.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));
    SDivFillTypes.insert(std::make_tuple(SIGN, SIGN, SIGN));
    
    FillTypesMap[ISD::SDIV] = SDivFillTypes;
    FillTypesMap[ISD::UDIV] = UDivFillTypes;

    FillTypeSet Eq, Ge, Geu, Gt, Gtu, Le, Leu, Lt, Ltu, Mod, Modu,
                Mul, Mulu, Ne, Or, Rem, Remu,
                Quot, Shl, Shra, Shrl, Sub, Xor;
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
    // TODO what to do with Geu, Gtu, Leu, Ltu and Mulu ??   
 
    Gt.insert(std::make_tuple(SIGN, SIGN, ZEROS));
    FillTypesMap[CmpInst::ICMP_UGE + ICMP_CONSTANT] =  Geu;
    FillTypesMap[CmpInst::ICMP_UGT + ICMP_CONSTANT] =  Gtu;
    FillTypesMap[CmpInst::ICMP_ULE + ICMP_CONSTANT] =  Leu;
    FillTypesMap[CmpInst::ICMP_ULT + ICMP_CONSTANT] =  Ltu;
    FillTypesMap[CmpInst::ICMP_EQ + ICMP_CONSTANT] =  Eq;
    FillTypesMap[CmpInst::ICMP_SGE + ICMP_CONSTANT] =  Ge;
    FillTypesMap[CmpInst::ICMP_SGT + ICMP_CONSTANT] = Gt;
    
    Le.insert(std::make_tuple(SIGN, SIGN, SIGN));
    FillTypesMap[CmpInst::ICMP_SLE + ICMP_CONSTANT] =  Le;

    Lt.insert(std::make_tuple(SIGN, SIGN, SIGN));
    FillTypesMap[CmpInst::ICMP_SLT + ICMP_CONSTANT] =  Lt;

    Mod.insert(std::make_tuple(SIGN, SIGN, SIGN));
    FillTypesMap[ISD::SREM] = SDivFillTypes;
    
    Modu.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));
    FillTypesMap[ISD::UREM] = SDivFillTypes;
    
    Ne.insert(std::make_tuple(SIGN, SIGN, ZEROS));
    FillTypesMap[CmpInst::ICMP_NE + ICMP_CONSTANT] =  Ne;
     
    // MOD -> REMAINDER
    // DIV -> QUOTIENT
    
    Quot.insert(std::make_tuple(SIGN, SIGN, SIGN));
    // TODO what is the ISD OF THE QUOTIENT??  FillTypesMap[ISD::] = SDivFillTypes;
     
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







