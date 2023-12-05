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
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/Support/TypeSize.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "WideningIntegerArithmetic"
STATISTIC(NumSExtsDropped, "Number of ISD::SIGN_EXTEND nodes that were dropped"); 
STATISTIC(NumZExtsDropped, "Number of ISD::ZERO_EXTEND nodes that were dropped");
STATISTIC(NumAnyExtsDropped, "Number of ISD::ANY_EXTEND nodes that were dropped");
STATISTIC(NumTruncatesDropped, "Number of ISD::TRUNCATE nodes that were dropped"); 
using namespace llvm;

namespace {


class WideningIntegerArithmetic {
  SelectionDAG &DAG;
  const TargetLowering &TLI; 

  public:
    WideningIntegerArithmetic(SelectionDAG &D):
      DAG(D), TLI(D.getTargetLoweringInfo())  {
    }
    WideningIntegerArithmetic(const WideningIntegerArithmetic &)= delete;
    WideningIntegerArithmetic& operator=(const WideningIntegerArithmetic&) = delete; 
    
    void solve(CodeGenOpt::Level OL);
  
    ///                         NodeId 
    using isSolvedMap = DenseMap<SDNode*, bool>;
    using SolutionSet = SmallVector<WideningIntegerSolutionInfo *>;
    using SolutionSetParam = SmallVectorImpl<WideningIntegerSolutionInfo * >;    
    using AvailableSolutionsMap = DenseMap<SDNode* , SolutionSet>;
    
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
    
 
    // checks whether a SDNode in the DAG is visited and solved` 
    isSolvedMap solvedNodes; 
    bool IsSolved(SDNode *Node);
    SolutionSet PossibleSolutions;
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

    SmallVector<WideningIntegerSolutionInfo *> visit_widening(SDNode *Node);


    FillTypeSet getOperandFillTypes(SDNode *Node);
  
    void setFillType(EVT SrcVt, EVT DstVT);  
    bool isInteger(SDNode *Node);
    bool isSolved(SDNode *Node);
    void combineBinOp(SDNode *N, SolutionSet leftSols, 
                          SolutionSet rightSols);

    bool addNonRedudant(SolutionSet &Solutions, 
                        WideningIntegerSolutionInfo* GeneratedSol);
    inline bool hasTypeGarbage(IntegerFillType fill);
    inline bool hasTypeT(IntegerFillType fill);
    inline bool hasTypeS(IntegerFillType fill);

    SolutionSet closure(SDNode *Node);
    SolutionSet tryClosure(SDNode *Node, bool changed);

    SolutionSet visitXOR(SDNode *Node);
    SolutionSet visitInstruction(SDNode *Node);
    SolutionSet visitBINOP(SDNode *Node);
    SolutionSet visitLOAD(SDNode *Node);
    SolutionSet visitSTORE(SDNode *Node);
    SolutionSet visitUNOP(SDNode *Node);
    void visitFILL(SDNode *Node);
    void visitWIDEN(SDNode *Node);
    void visitWIDEN_GARBAGE(SDNode *Node);
    void visitNARROW(SDNode *Node);
    SolutionSet visitDROP_EXT(SDNode *Node);
    SolutionSet visitDROP_TRUNC(SDNode *Node);
    SolutionSet visitEXTLO( SDNode *Node);
    SolutionSet visitSUBSUME_FILL(SDNode *Node);
    SolutionSet visitSUBSUME_INDEX(SDNode *Node);
    SolutionSet visitNATURAL(SDNode *Node);
    SolutionSet visitCONSTANT(SDNode *Node);


    std::vector<int> IntegerSizes = {8, 16, 32, 64};

    unsigned int getTargetWidth();

    BinOpWidth createWidth(unsigned char op1, 
                   unsigned char op2, unsigned dst);

    WIAKind getNodeKind(SDNode *Node);
    bool IsBinop(unsigned Opcode);
    bool IsUnop(unsigned Opcode);
    bool IsTruncate(unsigned Opcode);
    bool IsExtension(unsigned Opcode);
    bool IsLit(unsigned Opcode);
    bool IsLoad(unsigned Opcode);
    bool IsStore(unsigned Opcode);
 
    // Helper functions 
    SolutionType NodeToSolutionType(SDNode *Node, int cost);
    inline unsigned char getScalarSize(const EVT &VT) const;
    inline unsigned  getExtensionChoice(enum IntegerFillType ExtChoice);   
    inline unsigned int getExtCost(SDNode *Node, 
                WideningIntegerSolutionInfo* Sol, unsigned char IntegerSize);
    void initOperatorsFillTypes();
    void printNodeSols(SolutionSet Sols, SDNode *Node);
    inline Type* getTypeFromInteger(unsigned char Integer);
    bool mayOverflow(SDNode *Node);
};

} // end anonymous namespace

void WideningIntegerArithmetic::printNodeSols(SolutionSet Sols, SDNode *Node){
  int i = 0;
  dbgs() << "Found many Solutions ..-->\n";
  for(WideningIntegerSolutionInfo *Solution : Sols){
    dbgs() << "----- Solution " << ++i << "\n" << (*Solution) << "\n";
  }
}

inline unsigned char WideningIntegerArithmetic::getScalarSize(const EVT &VT) const {
  return VT.getScalarSizeInBits(); // TODO check
}

inline unsigned WideningIntegerArithmetic::getExtensionChoice(enum IntegerFillType ExtChoice){
  return ExtChoice == SIGN ? ISD::SIGN_EXTEND:
                             ISD::ZERO_EXTEND;
} 

bool WideningIntegerArithmetic::IsBinop(unsigned Opcode){
  switch(Opcode){
    case ISD::ADD:                
    case ISD::SUB:                
    case ISD::SADDSAT:
    case ISD::UADDSAT:            
    case ISD::SSUBSAT:
    case ISD::USUBSAT:            
    case ISD::ADDC:               
    case ISD::SADDO:
    case ISD::UADDO:             
    case ISD::SUBC:             
    case ISD::SSUBO:
    case ISD::USUBO:              
    case ISD::ADDE:               
    case ISD::ADDCARRY:           
    case ISD::SADDO_CARRY:        
    case ISD::SUBE:              
    case ISD::SUBCARRY:          
    case ISD::SSUBO_CARRY:       
    case ISD::SMULFIX:
    case ISD::SMULFIXSAT:
    case ISD::UMULFIX:
    case ISD::UMULFIXSAT:         
    case ISD::MUL:                
    case ISD::SDIV:               
    case ISD::UDIV:               
    case ISD::SREM:
    case ISD::UREM:               
    case ISD::MULHU:              
    case ISD::MULHS:             
    case ISD::AND:
    case ISD::OR:
    case ISD::XOR:      return true;
    default : return false;
    
  }
  return false;
}


// TODO check UNOPS
bool WideningIntegerArithmetic::IsUnop(unsigned Opcode){
  switch(Opcode){
    default: return false;
    case ISD::ABS: 
    case ISD::BSWAP:
    case ISD::CTTZ:
    case ISD::CTLZ:
    case ISD::BITREVERSE:
    case ISD::PARITY:
    case ISD::FREEZE:       return true;
  }
  return false;
}

bool WideningIntegerArithmetic::IsTruncate(unsigned Opcode){
  switch(Opcode){
    case ISD::TRUNCATE: return true;
    default: break;
  }
  return false;
}

bool WideningIntegerArithmetic::IsExtension(unsigned Opcode){
  switch(Opcode){
    default: break;
    case ISD::SIGN_EXTEND:
    case ISD::ZERO_EXTEND:   
    case ISD::ANY_EXTEND:   
    case ISD::AssertSext:        // We need to drop them only the first time
    case ISD::AssertZext:       return true;
  }
  return false;  
}

bool WideningIntegerArithmetic::IsStore(unsigned Opcode){
  switch(Opcode){
    default: break;
    case ISD::STORE:                      // TODO we need more? add masked store and others not vectors yet
    case ISD::MSTORE:
      return true;
  }
  return false;
}

bool WideningIntegerArithmetic::IsLoad(unsigned Opcode){
  switch(Opcode){
    default: break;
    case ISD::LOAD:
    case ISD::MLOAD:
      return true; //  
  }
  return false;
}

bool WideningIntegerArithmetic::IsLit(unsigned Opcode){
  switch(Opcode){
    default: break;
    case ISD::Constant: return true; 
  }
  return false;
}

WIAKind 
WideningIntegerArithmetic::getNodeKind(SDNode *Node){
  unsigned Opcode = Node->getOpcode();
  if(IsBinop(Opcode))
    return WIAK_BINOP;
  else if(IsUnop(Opcode))
    return WIAK_UNOP;
  else if(IsExtension(Opcode))
    return WIAK_DROP_EXT;
  else if(IsTruncate(Opcode))
    return WIAK_DROP_LOCOPY;
  else if(IsLit(Opcode))
    return WIAK_LIT;
  else if(IsLoad(Opcode) )
    return WIAK_LOAD;
  else if(IsStore(Opcode) )
    return WIAK_STORE;
  else // IsVar
    return WIAK_UNKNOWN;
   
}


SmallVector<WideningIntegerSolutionInfo *> WideningIntegerArithmetic::visitInstruction(SDNode *Node){
  SolutionSet Solutions ;
  unsigned Opcode = Node->getOpcode();
  
  dbgs() << " and Opcode is " << Opcode;
  if(IsBinop(Opcode)){
    dbgs() << " Binop Opcode str is " << OpcodesToStr[Opcode];
    dbgs() << " and Visiting Binop..\n";  
    return visitBINOP(Node);
  }
  else if(IsUnop(Opcode)){
    dbgs() << " Unop Opcode str is " << OpcodesToStr[Opcode];
    dbgs() << " and Visiting Unop...\n"; 
    return visitUNOP(Node);
  }
  else if(IsLoad(Opcode)){
    dbgs() << " Load Opcode str is " << OpcodesToStr[Opcode];
    dbgs() << " and Visiting Load...\n"; 
    return visitLOAD(Node);
  }
  else if(IsStore(Opcode)){
    dbgs() << " Store Opcode str is " << OpcodesToStr[Opcode];
    dbgs() << " and Visting Store...\n"; 
    return visitSTORE(Node);
  }
  else if(IsExtension(Opcode)){
    dbgs() << " Is Extension str is " << OpcodesToStr[Opcode];
    dbgs() << " and Visiting Extension...\n"; 
    return visitDROP_EXT(Node);
  }
  else if(IsTruncate(Opcode)){ 
    dbgs() << " Is Truncate str is " << OpcodesToStr[Opcode];
    dbgs() << " and Visiting Truncation ..\n"; 
    return visitDROP_TRUNC(Node);
  }
  else if(Opcode == ISD::Constant){
    dbgs() << " Is Constant str is " << OpcodesToStr[Opcode];
    dbgs() << " and Visiting Constant..\n"; 
    return visitCONSTANT(Node);
  }
  else{
    dbgs() << "Could not found a solutionOpcode is " << Opcode << "\n";
    dbgs() << "Opcode str is " << OpcodesToStr[Opcode] << "\n"; 
    // default solution so we can initialize all the nodes with some solution set.
    auto Sol = new WideningIntegerSolutionInfo(Opcode, ANYTHING, 0, 
                getTargetWidth(), /* TODO CHECK */getTargetWidth() , 
                0, WIAK_UNKNOWN, Node);
    Solutions.push_back(Sol); 
  }
    
  // TODO we are missing many opcodes here, need to add them. 
  dbgs() << "Returning default solution take care here.\n"; 
  return Solutions;
}



SmallVector<WideningIntegerSolutionInfo *> 
WideningIntegerArithmetic::visit_widening(SDNode *Node){
  
  if(IsSolved(Node) ){
    dbgs() << "Node " << Node->getNodeId() << "is Solved";
    dbgs() << " and Opcode str is " << OpcodesToStr[Node->getOpcode()] << "\n"; 
    return AvailableSolutions[Node]; 
  } 
  for (const SDValue &value : Node->op_values() ){    
    SDNode *OperandNode = value.getNode(); // #TODO 1 check value size is 1?
    SolutionSet Sols = visit_widening(OperandNode);
  }
  dbgs() << " Trying to solve Node with Opc !! : " << OpcodesToStr[Node->getOpcode()] << " " << Node->getOpcode() << "\n";
  auto CalcSolutions = visitInstruction(Node);
  solvedNodes[Node] = true; 
  if(CalcSolutions.size() > 0){
    printNodeSols(CalcSolutions, Node);
    AvailableSolutions[Node] = CalcSolutions;
  }else{
    dbgs() << "This node does not have any solutions" << "\n";
  }
    return CalcSolutions;
  
}


bool WideningIntegerArithmetic::addNonRedudant(SolutionSet &Solutions, 
                    WideningIntegerSolutionInfo* GeneratedSol){
  bool WasRedudant = false;
  int RedudantNodeToDeleteCost = INT_MAX;
  dbgs() << "Begin Add Non Redudant -> " << '\n';
  for(auto It = Solutions.begin(); It != Solutions.end(); ){
    int ret = (*It)->IsRedudant((*GeneratedSol));
    if(ret == -1 ){ // GeneratedSol is redudant
      WasRedudant = true; 
      It++;  
    }else if(ret == 1){ // Sol is redudant
      assert(GeneratedSol->getCost() < RedudantNodeToDeleteCost);
      dbgs() << "Check .... Deleting it --> " << (**It) << '\n';
      It = Solutions.erase(It);
      // TODO consider change data structure for Possible small optimization
    }else{ // ret == 0 no redudant found
      It++;
    }
  }
  dbgs() << "End Add Non Redudant -> " << '\n';
  if(!WasRedudant){
    dbgs() << "Adding Solution --> " << *GeneratedSol << '\n';
    Solutions.push_back(GeneratedSol);
    dbgs() << "Returning True --> " << '\n';
    return true;
  }
  dbgs() << "Returning False --> " << '\n';
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
WideningIntegerArithmetic::visitXOR(SDNode *Node ){

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
  // User can decide what solution is appropriate
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

void WideningIntegerArithmetic::solve(CodeGenOpt::Level OL){

  ///  if(OL.getLevel != Aggressive)
  ///  return;
 
  SDValue Root = DAG.getRoot();
  setFillType(Root.getValueType(), Root.getValueType());
  initOperatorsFillTypes();

  for (SDNode &Node : DAG.allnodes()){
    if(!IsSolved(&Node)  && isInteger(&Node) ){ 
      visit_widening(&Node);  
    }
    
  }
} 

bool WideningIntegerArithmetic::isInteger(SDNode *Node){
  return Node->getValueType(0).isInteger();
}

// Checks whether a SDNode is visited 
// so we don't visit and solve the same Node again
bool WideningIntegerArithmetic::IsSolved(SDNode *Node){
  auto isVisited = solvedNodes.find(Node);
  if(isVisited != solvedNodes.end())
    return true;
  
  return false;
}

WideningIntegerArithmetic::SolutionSet
WideningIntegerArithmetic::tryClosure(SDNode *Node, bool Changed){
  auto FillsSize = 0;
  unsigned Opcode = Node->getOpcode();
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
WideningIntegerArithmetic::closure(SDNode *Node){
  auto Sols = AvailableSolutions[Node];
  bool Changed;
  int i = 0;
  do{
    Changed = false;
    visitFILL(Node);
    visitWIDEN(Node);
    visitWIDEN_GARBAGE(Node); 
    visitNARROW(Node);
    dbgs() << "Iteration " << ++i << "---------------------------------------Adding Possible Solutions size is  " << PossibleSolutions.size() << '\n' << "Opcode to Str is --> " << OpcodesToStr[Node->getOpcode()] << '\n';
    for(auto PossibleSol : PossibleSolutions){
      dbgs() << "Before Adding Possible Sol --> " << '\n';
      printNodeSols(AvailableSolutions[Node], Node); 
      if(PossibleSol == NULL){
        dbgs() << "Error here.." << '\n';
      }
      else if(PossibleSol == NULL){
        dbgs() << "Error here.." << '\n';
      }
      dbgs() << "Trying to add pos sol if non redudant --> " << *PossibleSol << '\n';
      bool Added = addNonRedudant(AvailableSolutions[Node], PossibleSol);
      if(Added){
        Changed = true;
        dbgs() << "After Adding Possible Sol --> " << '\n';
        printNodeSols(AvailableSolutions[Node], Node);
      } 
    }
    dbgs() << "Clear all the possible solutions " << '\n';
    PossibleSolutions.clear();  // TODO how to optimize this ?
                                // Implement closure in each visit?
    dbgs() << "Possible Solution size is --> " << PossibleSolutions.size() << '\n';; 
  }while(Changed == true );
  dbgs() << "Returning all the Solutions after non redudant" << '\n';
  return Sols;
}



inline 
WideningIntegerArithmetic::FillTypeSet 
WideningIntegerArithmetic::getOperandFillTypes(SDNode *Node){

  FillTypeSet Set;
  unsigned Opcode = Node->getOpcode();
  switch(Opcode){
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
          Set.insert(std::make_tuple(SIGN, SIGN, SIGN));
          Set.insert(std::make_tuple(ZEROS, ZEROS, ZEROS));
        }
      }
      return Set;
    }
  }
  return getFillTypes(Opcode);
} 
   
WideningIntegerArithmetic::SolutionSet 
WideningIntegerArithmetic::visitUNOP(SDNode *Node){
  bool Changed = false;
  SDValue N0 = Node->getOperand(0);
  unsigned Opcode = Node->getOpcode();
  UnaryFillTypeSet AvailableFillTypes = getUnaryFillTypes(Node->getOpcode());
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
                w1, /* TODO CHECK */w1 , Sol->getCost(), Node); 
    Unop->addOperand(Sol); 
    AvailableSolutions[Node].push_back(Unop);
    Changed = true;
  }
  // call closure if multiple FillTypes or Multiple Unop Widths
  return tryClosure(Node, Changed);  
}

WideningIntegerArithmetic::SolutionSet 
WideningIntegerArithmetic::visitCONSTANT(SDNode *Node){
  SolutionSet Sols;
  ConstantSDNode *CN = cast<ConstantSDNode>(Node);
  unsigned bitWidth = CN->getConstantIntValue()->getBitWidth();
  auto Sol = new WIA_CONSTANT(Node->getOpcode(), ExtensionChoice,
    bitWidth, getTargetWidth(), getTargetWidth(), 0, Node);
  Sols.push_back(Sol);
  return Sols;
}
  
WideningIntegerArithmetic::SolutionSet 
WideningIntegerArithmetic::visitSTORE(SDNode *Node){
  SolutionSet Sols;
  StoreSDNode *SD = cast<StoreSDNode>(Node);
  SDValue N0 = SD->getValue();
  
  unsigned char N0Bits = getScalarSize(N0->getValueType(0));
  unsigned char MemBits = getScalarSize(SD->getMemoryVT());
  auto N0Sols = AvailableSolutions[N0.getNode()];
  for(WideningIntegerSolutionInfo *Sol : N0Sols ){
    auto StoreSol = new WIA_STORE(Node->getOpcode(), Sol->getFillType(),
          MemBits, N0Bits, MemBits, Sol->getCost(), Node);
    Sols.push_back(StoreSol);
  }
  return Sols;   
}
WideningIntegerArithmetic::SolutionSet 
WideningIntegerArithmetic::visitLOAD(SDNode *Node){

  SolutionSet Sols;
  LoadSDNode *LD  = cast<LoadSDNode>(Node); 
  // TODO very far in the future distiguish load that the users of the load 
  // use only a smaller width and ignore the upper bits

  // TODO do we need to return multiple loads? 
  EVT MemoryVT = LD->getMemoryVT();
  IntegerFillType FillType; 
  // distiguish EXTLOAD and NON ext LOAD Non EXT needs to be extended if the upper bits are used
  switch(LD->getExtensionType()){
    case ISD::NON_EXTLOAD: 
      FillType = IntegerFillType::ANYTHING; 
      break; 
    case ISD::EXTLOAD: 
      FillType = IntegerFillType::ANYTHING;
      break;
    case ISD::SEXTLOAD: 
      FillType = IntegerFillType::SIGN;
      break;
    case ISD::ZEXTLOAD: 
      FillType = IntegerFillType::ZEROS;
      break;
    default:
      return Sols;
  }
  unsigned char Width = getScalarSize(MemoryVT); 
  unsigned char FillTypeWidth = getScalarSize(LD->getValueType(0)); // 
  dbgs() << "--------------------------------------------------------- getTargetWidth is " << getTargetWidth() << '\n'; 
  auto WIALoad = new WIA_LOAD(Node->getOpcode(), FillType, FillTypeWidth,
                Width, getTargetWidth(), 0, Node);  
  Sols.push_back(WIALoad);
  return Sols;
}
  

inline bool
WideningIntegerArithmetic::mayOverflow(SDNode *Node){
  unsigned Opcode;
  SDValue N0 = Node->getOperand(0);
  SDValue N1 = Node->getOperand(1);
  Opcode = Node->getOpcode();
  switch(Opcode){
    case (ISD::ADD):
    case (ISD::UADDO):
    case (ISD::SADDO):
    case (ISD::UADDSAT):
    case (ISD::ADDC):
    case (ISD::ADDCARRY): return DAG.computeOverflowKind(N0, N1) != 
                                 SelectionDAG::OFK_Never;
    default:
      return false; 
  }
} 
 
 
// Return all the solution based on binop rules op1 x op2 -> W
WideningIntegerArithmetic::SolutionSet 
WideningIntegerArithmetic::visitBINOP(SDNode* Node){
 
  bool AddedSol = false; 
  SDNode *N0 = Node->getOperand(0).getNode();
  SDNode *N1 = Node->getOperand(1).getNode();
  
  SolutionSet newSolutions;
  // get All the available solutions from nodes 
  SmallVector<WideningIntegerSolutionInfo*> LeftSols = 
                                              AvailableSolutions[N0];
  SmallVector<WideningIntegerSolutionInfo*> RightSols = 
                                              AvailableSolutions[N1];

  unsigned Opcode = Node->getOpcode();
  dbgs() << "Operand 0 is --> " << OpcodesToStr[N0->getOpcode()]<< "\n";
  dbgs() << "Operand 1 is --> " << OpcodesToStr[N1->getOpcode()]<< "\n";
  dbgs() << "Operand 0 Solutions Size is --> " << LeftSols.size() << "\n";
  dbgs() << "Operand 1 Solutions Size is --> " << RightSols.size() << "\n";
  FillTypeSet OperandFillTypes = getOperandFillTypes(Node);
  // A function that combines solutions from operands left and right
  for (WideningIntegerSolutionInfo *leftSolution : LeftSols){
    for(WideningIntegerSolutionInfo *rightSolution : RightSols){
      // Test whether current binary operator has the available
      // fill type provided by leftSolution and rightSolution
      auto FillType = getOrNullFillType((OperandFillTypes),
                                  leftSolution->getFillType(),
                                  rightSolution->getFillType());
      dbgs() << "Left FillType is --> " << leftSolution->getFillType() << "\n";
      dbgs() << "Right FillType is --> " << rightSolution->getFillType() << "\n";
      dbgs() << "Found FillType for combination --> " << FillType << "\n";
      if(FillType == UNDEFINED)
        continue;
      dbgs() << "Passed FillType for combination --> " << FillType << "\n";
      unsigned char w1 = leftSolution->getUpdatedWidth();
      unsigned char w2 = rightSolution->getUpdatedWidth();
      if(w1 != w2)
        continue;
      dbgs() << "The widths are the same and we continue--> " << w1 << "\n";
      EVT NewVT = EVT::getIntegerVT(*DAG.getContext(), w1); 
      if(!TLI.isOperationLegal(Opcode, NewVT))
        continue;
      dbgs() << "The Operation is legal for that newVT--> "<<  w1 << "\n";
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
      // TODO check overflow and then calculate the FillTypeWidth.

      auto Sol = new WIA_BINOP(Node->getOpcode(), FillType, 
          FillTypeWidth, w1, UpdatedWidth, Cost, Node);
      Sol->addOperand(leftSolution);
      Sol->addOperand(rightSolution);
      dbgs() << "Adding Sol with cost " << Cost << " and width " << UpdatedWidth << "\n";
      AvailableSolutions[Node].push_back(Sol);
      AddedSol = true; 
    }
  }
  dbgs() << "Calling closure here.\n"; 
  // call closure if multiple FillTypes or Multiple Binop Widths
  return tryClosure(Node, AddedSol); 
}

void WideningIntegerArithmetic::visitFILL(
                          SDNode *Node){
  
  EVT VT = Node->getValueType(0);
  unsigned VTBits = VT.getScalarSizeInBits();
  unsigned ExtensionOpc = getExtensionChoice(ExtensionChoice);
  unsigned char Width = getTargetWidth() - VTBits;  
  unsigned char FillTypeWidth = VTBits; 
  
  SolutionSet Sols = AvailableSolutions[Node];
  for(WideningIntegerSolutionInfo *Sol : Sols){
    if(Sol->getOpcode() == ISD::SIGN_EXTEND_INREG ||
       llvm::ISD::isExtOpcode(Sol->getOpcode()))
      continue;    // TODO CHECK is FILL a ISD::SIGN_EXTEND_INREG ??
    // WIA_FILL extends the least significant *Width* bits of SDNode
    // to targetWidth
    for(auto IntegerSize : IntegerSizes ){
      EVT NewVT = EVT::getIntegerVT(*DAG.getContext(), IntegerSize); 
      if(IntegerSize <= FillTypeWidth && 
         !TLI.isOperationLegal(ISD::SIGN_EXTEND_INREG, NewVT))
        continue;
      // get first legal IntegerSize.
      // in the inner loop
      // Solution with the biggest FillTypeWidth wins
      WideningIntegerSolutionInfo *Fill = new WIA_FILL(
        ExtensionOpc, ExtensionChoice, FillTypeWidth, Width,
        getTargetWidth() , Sol->getCost() + 1, Node ); 
      Fill->addOperand(Sol);
      PossibleSolutions.push_back(Fill);  
    }

  }
    
}

inline Type* WideningIntegerArithmetic::getTypeFromInteger(
                                unsigned char Integer){
  Type* Ty;
  switch(Integer){
    case 8: Ty = Type::getInt8Ty(*DAG.getContext()); break;
    case 16: Ty = Type::getInt16Ty(*DAG.getContext()); break;
    case 32: Ty = Type::getInt32Ty(*DAG.getContext()); break;
    case 64: Ty = Type::getInt64Ty(*DAG.getContext()); break; 
  }
  return Ty; 
}

inline unsigned WideningIntegerArithmetic::getExtCost(SDNode *Node,
         WideningIntegerSolutionInfo* Sol, unsigned char IntegerSize){
  unsigned cost = Sol->getCost();
  Type* Ty1 = Node->getValueType(0).getTypeForEVT(*DAG.getContext());
  Type* Ty2 = getTypeFromInteger(IntegerSize);
  if(!TLI.isZExtFree(Ty1, Ty2) || !TLI.isSExtFree(Ty1, Ty2)){
    ++cost;
  }
  return cost;
}
  
void WideningIntegerArithmetic::visitWIDEN(
                          SDNode *Node){
 
  // TODO how to check for Type S? depends on target ? or on this operand
  // if(!hasTypeS(Sol->getFillType())
  //  return NULL; 
  
  unsigned char FillTypeWidth = getScalarSize(Node->getOperand(0).getValueType()); 
  unsigned char Width = getTargetWidth() - FillTypeWidth; 
  unsigned ExtensionOpc = getExtensionChoice(ExtensionChoice);
  SolutionSet Sols = AvailableSolutions[Node];
  dbgs() << "SolutionsSize is --> " << Sols.size() << '\n';
  for(WideningIntegerSolutionInfo *Sol : Sols){
    dbgs() << "Inside visitWiden Iterating Sol --> " << Sol << '\n';
    if(llvm::ISD::isExtOpcode(Sol->getOpcode()) || 
       Sol->getOpcode() == ISD::SIGN_EXTEND_INREG)
      continue;
    dbgs() << "Passed first if --> " << Sol << '\n';
    for(int IntegerSize : IntegerSizes){
      EVT NewVT = EVT::getIntegerVT(*DAG.getContext(), IntegerSize); 
      if(IntegerSize <= FillTypeWidth) // TODO check
        continue;
      dbgs() << "Passed first check on integerSize--> " << Sol << '\n';
      if(!TLI.isOperationLegal(ISD::SIGN_EXTEND_INREG, NewVT))
        continue;
      dbgs() << "Adding all Solution with IntegerSize --> " << IntegerSize << '\n';
      unsigned cost = getExtCost(Node, Sol, IntegerSize); 
      // Results to a widened expr based on ExtensionOpc
      WideningIntegerSolutionInfo *Widen = new WIA_WIDEN(
        ExtensionOpc, ExtensionChoice, FillTypeWidth, Width,
        getTargetWidth() , cost , Node);
      Widen->addOperand(Sol); 
      PossibleSolutions.push_back(Widen);
    } 
  }

}
    

void WideningIntegerArithmetic::visitWIDEN_GARBAGE(
      SDNode *Node){

  // #TODO get width of expr must be in the same node or on the operand
  unsigned char FillTypeWidth = getScalarSize(Node->getValueType(0));  
  unsigned char Width = getTargetWidth() - FillTypeWidth;
  unsigned ExtensionOpc = ISD::ANY_EXTEND;  // Results to a garbage widened
  auto Sols = AvailableSolutions[Node];
  dbgs() << "SolutionsSize is --> " << Sols.size() << '\n';
  // TODO add isExtFree and modify cost accordingly 
  for(WideningIntegerSolutionInfo *Sol : Sols){
    dbgs() << "Inside GarbageWiden Iterating Sol --> " << Sol << '\n';
    if(llvm::ISD::isExtOpcode(Sol->getOpcode() || 
       Sol->getOpcode() == ISD::SIGN_EXTEND_INREG))
      continue;
    dbgs() << "Passed first if --> " << Sol << '\n';
    for(int IntegerSize : IntegerSizes){
      EVT NewVT = EVT::getIntegerVT(*DAG.getContext(), IntegerSize); 
      if(IntegerSize <= FillTypeWidth || 
         !TLI.isOperationLegal(ISD::ANY_EXTEND, NewVT))
        continue;
      
      unsigned cost = getExtCost(Node, Sol, IntegerSize); 
      dbgs() << "Adding GarbageWiden --> " << IntegerSize << '\n';
     
      WideningIntegerSolutionInfo *GarbageWiden = new WIA_WIDEN(
        ExtensionOpc, ANYTHING, FillTypeWidth,  Width, getTargetWidth(),
        cost , Node);
      GarbageWiden->addOperand(Sol);
      PossibleSolutions.push_back(GarbageWiden);    
    }
  }  
}
    
void WideningIntegerArithmetic::visitNARROW(SDNode *Node){
  
 
  unsigned char Width = getScalarSize(Node->getValueType(0));  
  unsigned char FillTypeWidth = getTargetWidth() - Width;

  // TODO check isTruncateFree on some targets and widths.
  // Not sure the kinds of truncate of the Target will determine it.
  unsigned ExtensionOpc = getExtensionChoice(ExtensionChoice);
 
  auto Sols = AvailableSolutions[Node];
  dbgs() << "SolutionsSize is --> " << Sols.size() << '\n';
  for(auto Sol : Sols){ 
    dbgs() << "Inside visitNarrow " << '\n';
    for(unsigned char k = 0; k < IntegerSizes.size() - 1; k++){
      if(llvm::ISD::isExtOpcode(Sol->getOpcode())) // do we need to keep solutions of the form truncate( anyext (x ) ) ??
        continue;
      EVT NewVT = EVT::getIntegerVT(*DAG.getContext(), IntegerSizes[k]);
      if(!TLI.isOperationLegal(ISD::TRUNCATE, NewVT) ||
         llvm::ISD::isExtOpcode(Sol->getOpcode()))         continue;
    
      dbgs() << "Getting type for EVT for Left child " << '\n';
      unsigned cost = Sol->getCost();
      Type* Ty1 = Node->getValueType(0).getTypeForEVT(*DAG.getContext());
      Type* Ty2;
      switch(IntegerSizes[k]){
        case 8: Ty2 = Type::getInt8Ty(*DAG.getContext()); break;
        case 16: Ty2 = Type::getInt16Ty(*DAG.getContext()); break;
        case 32: Ty2 = Type::getInt32Ty(*DAG.getContext()); break;
      }
      if(!TLI.isTruncateFree(Ty1, Ty2))
        cost = cost + 1;
      dbgs() << "Adding new Wia Narrow" << IntegerSizes[k] <<  '\n';
      WideningIntegerSolutionInfo *Trunc = new WIA_NARROW(ExtensionOpc,
        ExtensionChoice, // Will depend on available Narrowing , 
        FillTypeWidth, Width, IntegerSizes[k], Sol->getCost() + 1 , Node);
      Trunc->addOperand(Sol);
      PossibleSolutions.push_back(Trunc);
    }
  } 
}
    
WideningIntegerArithmetic::SolutionSet WideningIntegerArithmetic::visitDROP_EXT(
                          SDNode *Node){

  SDValue N0 = Node->getOperand(0);
  unsigned char ExtendedWidth = getScalarSize(Node->getValueType(0));
  unsigned char Width = getScalarSize(N0.getValueType());  
  unsigned char FillTypeWidth = getTargetWidth() - Width;  
  unsigned Opc = N0->getOpcode();
  auto ExprSolutions = AvailableSolutions[N0.getNode()];
  SolutionSet Sols;
  switch(Node->getOpcode()){
    case ISD::SIGN_EXTEND: ++NumSExtsDropped; break;
    case ISD::ZERO_EXTEND: ++NumZExtsDropped; break;
    case ISD::ANY_EXTEND: ++NumAnyExtsDropped; break;
    case ISD::AssertSext: ++NumSExtsDropped; break;      
    case ISD::AssertZext: ++NumZExtsDropped; break;
  }
  dbgs() << "Trying to drop extension in Solutions" << '\n';
  dbgs() << "Expr Solutions Size is " << ExprSolutions.size() << '\n';
  dbgs() << "Opc of Node->Op0 is " << Opc << " Opc string is " << OpcodesToStr[Opc] << '\n';
  dbgs() << "Opc of Node is " << Node->getOpcode() << "Opc of Node str is " << OpcodesToStr[Node->getOpcode()] << '\n';
  for(auto Solution : ExprSolutions){ 
  // We simply drop the extension and we will later see if it's needed.
    dbgs() << "Drop extension in Solutions" << '\n'; 
    WideningIntegerSolutionInfo *Expr = new WIA_DROP_EXT(Opc,
      ExtensionChoice, FillTypeWidth, ExtendedWidth /*OldWidth*/, 
      Width/*NewWidth*/, Solution->getCost(), Node);
    
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
  dbgs() << "Getting Child's Solutions\n";
  SmallVector<WideningIntegerSolutionInfo*> ExprSolutions = 
                                              AvailableSolutions[N0.getNode()];
  
  if(ExprSolutions.size() == 0 ){
    dbgs() << "Child of Truncate with opc " << N0.getNode()->getOpcode();
    dbgs() << " has no Solutions\n";
    return Sols; 
  }
  unsigned char TruncatedWidth = getScalarSize(Node->getValueType(0)); // TODO CHECK
  // We simply drop the truncation and we will later see if it's needed.
  unsigned Opc = N0->getOpcode();
  NumTruncatesDropped++; 
  for(auto Sol : ExprSolutions){
    unsigned char NewWidth = getTargetWidth() - Sol->getFillTypeWidth();   
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
    unsigned char NewWidth = getTargetWidth() - Sol->getFillTypeWidth();   
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
        EVT NewVT = EVT::getIntegerVT(*DAG.getContext(), IntegerSize); 
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
  return 0;
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
}






