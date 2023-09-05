///===- WideningIntegerArithmetic ------------------------------------------===
//
//



#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/CodeGen/WideningIntegerArithmeticInfo.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/IR/Module.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/Support/TypeSize.h"
#include "llvm/CodeGen/ValueTypes.h"


using namespace llvm;


class WideningIntegerArithmetic {
  SelectionDAG &DAG;
  const TargetLowering &TLI; 

  enum OperatorKind{
    OK_ADD,
    OK_SUB,
    OK_XOR,
    OK_OR,
    OK_AND,
    OK_DIVU,
    OK_DIV,
    OK_GE,
    OK_GEU,
    OK_GT,
    OK_GTU,
    OK_LE,
    OK_LEU,
    OK_LT,
    OK_LTU,
    OK_MOD,
    OK_MODU,
    OK_MUL,
    OK_MULU,
    OK_NE,
    OK_SHL,
    OK_SHRA,
    OK_SHRL,
  };

  public:
    WideningIntegerArithmetic(SelectionDAG &D):
      DAG(D), TLI(D.getTargetLoweringInfo()) {
    } 
  
    void solve();
    ///                         NodeId 
    using isSolvedMap = DenseMap<int, bool>;
    using SolutionSet = SmallVector<WideningIntegerSolutionInfo *>;
    using SolutionSetParam = SmallVectorImpl<WideningIntegerSolutionInfo * >;    
    using AvailableSolutionsMap = DenseMap<int, SolutionSet>;
    
    using BinOpWidth = std::tuple<unsigned char, unsigned char, unsigned char>;
    using WidthsSet = SmallVector<BinOpWidth>;
    using OperatorWidthsMap = DenseMap<unsigned, WidthsSet>;
    using TargetWidthsMap = DenseMap<Triple::ArchType, OperatorWidthsMap>;
    
    using FillTypeSet = SmallSet<std::tuple<IntegerFillType, IntegerFillType, IntegerFillType>, 4>;
    using UnaryFillTypeSet = SmallSet<std::tuple<IntegerFillType,
                                      IntegerFillType, 4>;
    
    using OperatorFillTypesMap = DenseMap<unsigned, FillTypeSet>;
    typedef WideningIntegerSolutionInfo* SolutionType;
  private:
    enum IntegerFillType ExtensionChoice;   
 
    // checks whether a SDNode in the DAG is visited and solved` 
    isSolvedMap solvedNodes; 
    bool IsSolved(SDNode *Node);
    // Holds all the available solutions 
    AvailableSolutionsMap AvailableSolutions;

  
    TargetWidthsMap TargetWidths;
    
    unsigned char getBinOpWidth(unsigned opCode, unsigned char w1, unsigned char w2);
    void initTargetWidthTables();
    
                                        
    OperatorFillTypesMap FillTypesMap; 
    inline FillTypeSet getFillTypes(unsigned Opcode);
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

    SolutionSet closure(SolutionSet Sols);

    SolutionSet visitXOR(SDNode *Node);
    // TODO refactor them to SolutionSet visit****(SDNode *Node);
    SolutionSet visitInstruction(SDNode *Node);
    SolutionSet visitBINOP(SDNode *Node);
    SolutionSet visitLOAD(LoadSDNode *Node);
    SolutionSet visitAssign(SDNode *Node);
    SolutionSet visitUNOP(SDNode *Node);
    SolutionSet visitFILL(SDNode *Sol);
    SolutionSet visitWIDEN(SDNode *Sol);
    SolutionSet visitWIDEN_GARBAGE(SDNode *Sol);
    SolutionSet visitNARROW(SDNode *Sol);
    SolutionSet visitDROP_EXT(SDNode *Sol);
    SolutionSet visitDROP_LO_COPY(SDNode *Sol);
    SolutionSet visitDROP_LO_IGNORE(SDNode *Sol);
    SolutionSet visitEXTLO( SDNode *LeftSol, SDNode *RightSol);
    SolutionSet visitSUBSUME_FILL(SDNode *Sol);
    SolutionSet visitSUBSUME_INDEX(SDNode *Sol);
    SolutionSet visitNATURAL(SDNode *Sol);


    std::vector<unsigned char> IntegerSizes = {8, 16, 32, 64};

    unsigned char getTargetWidth();

    BinOpWidth createWidth(unsigned char op1, 
                   unsigned char op2, unsigned dst);

    WIAKind getNodeKind(SDNode *Node);
    bool IsBINOP(unsigned Opcode);
    bool IsUNOP(unsigned Opcode);
    bool IsTruncate(unsigned Opcode);
    bool IsExtension(unsigned Opcode);
    bool IsAssign(unsigned Opcode);
    bool IsLit(unsigned Opcode);
    bool IsLoad(unsigned Opcode);
  
    SolutionType NodeToSolutionType(SDNode *Node, int cost);
    inline unsigned char getScalarSize(const Node &Node) const;    
};

inline unsigned char getScalarSize(EVT VT){
  return = VT.getScalarSizeInBits(); // TODO check
}  

bool WideningIntegerArithmetic::IsBINOP(unsigned Opcode){
  switch(Opcode){
    default : return false;
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
    
  }
}


// TODO check UNOPS
bool WideningIntegerArithmetic::IsUNOP(unsigned Opcode){
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

}

bool WideningIntegerArithmetic::IsTruncate(unsigned Opcode){
  switch(Opcode){
    default: break;
    case ISD::TRUNCATE: return true;
  }
  return false;
}

bool WideningIntegerArithmetic::IsExtension(unsigned Opcode){
  switch(Opcode){
    default: break;
    case ISD::SIGN_EXTEND:
    case ISD::ZERO_EXTEND:      
    case ISD::AssertSext:        // TODO check we need to drop those extensions or not?
    case ISD::AssertZext:       return true;
  }
  
}

bool WideningIntegerArithmetic::IsAssign(unsigned Opcode){
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
  if(IsBINOP(Opcode))
    return WIAK_BINOP;
  else if(IsExtension(Opcode))
    return WIAK_DROP_EXT;
  else if(IsTruncate(Opcode))
    return WIAK_DROP_LOCOPY;
  else if(IsLit(Opcode))
    return WIAK_LIT;
  else if(IsAssign(Opcode))
    return WIAK_ASSIGN;
  else if(IsLoad )
    return WIAK_LOAD;
  else if(IsStore )
    return WIAK_STORE;
  else // IsVar
    return WIAK_VAR;
   
}


SmallVector<WideningIntegerSolutionInfo *> WideningIntegerArithmetic::visitInstruction(SDNode *Node){
  SolutionSet Solutions;
  unsigned Opcode = Node->getOpcode();
   
  if(IsBINOP(Opcode))
    return visitBINOP(Node);
  else if(IsUNOP(Opcode))
    return visitUNOP(Node);
  else if(IsLOAD(Opcode))
    return visitLOAD(Node);
  else if(IsExtension(Opcode))
    return visitDROP_EXT(Node);
  else if(IsTruncate(Opcode)) // TODO when to call visitDROP_LOIGNORE
    return visitDROP_LO_COPY(Node);
  else{
    WideningIntegerSolutionInfo *NewNode(Node);
    NewNode->setFillTypeWidth(0);
    
    NewNode->setWidth(getTargetWidth());
    Solutions.push_back(NewNode);
  }
    
  // TODO we are missing many opcodes here, need to add them. 
   
  return Solutions;
}



// TODO check correctness
SmallVector<WideningIntegerSolutionInfo *> 
WideningIntegerArithmetic::visit_widening(SDNode *Node){
  
  if(IsSolved(Node)) 
    return AvailableSolutions[Node->getNodeId()]; 

  SmallVector<int> Costs; // One WideningSolutionInfo has many costs and one get's chosen
  // NodeId to FillTypeSet
  DenseMap<unsigned, FillTypeSet> ChildrenFillTypes;
  
  for (const SDValue &value : Node->op_values() ){    
    SDNode *OperandNode = value.getNode(); // #TODO 1 check value size is 1?
    ChildrenFillTypes[OperandNode->getNodeId()] = 
                                    getOperandFillTypes(OperandNode);
    SolutionSet Sols = visit_widening(OperandNode);
  }
  
  auto CalcSolutions = visitInstruction(Node);
  solvedNodes[Node->getNodeId()] = true; 
  return CalcSolutions;
}


bool WideningIntegerArithmetic::addNonRedudant(SolutionSet &Solutions, 
                    WideningIntegerSolutionInfo* GeneratedSol){
  bool WasRedudant = false;
  unsigned RedudantNodeToDeleteId = 0;
  int RedudantNodeToDeleteCost = INT_MAX;
  for(auto Sol : Solutions ){
    int ret = Sol->IsRedudant(GeneratedSol);
    if(ret == -1 ){
      WasRedudant = true; 
    }else if(ret == 1){
      assert(GeneratedSol->getCost() < RedudantNodeToDeleteCost);
      auto ItToDelete = std::find(Solutions::begin(), Solutions::end(), 
                      RedudantNodeToDeleteId); 
      // TODO consider change data structure for Possible small optimization
      // for std::find
      std::erase(ItToDelete);
      RedudantNodeToDeleteId = Sol->getNode()->getNodeId();
    }  
  }
  if(!WasRedudant){
    Solutions.insert(GeneratedSol)
    return true;
  }
  return false;
}

WideningIntegerArithmetic::FillTypeSet
WideningIntegerArithmetic::getFillTypes(unsigned Opcode){

  auto It = FillTypesMap.find(Opcode);
  assert(It != FillTypesMap.end() && "Opcode does not have fillTypes" );   
  FillTypeSet FillTypes = It->second;
  return FillTypes;
}

WideningIntegerArithmetic::UnaryFillTypeSet
WideningIntegerArithmetic::getUnaryFillTypes(unsigned Opcode){
  auto It = UnaryFillTypesMap.find(Opcode);
  assert(It != UnaryFillTypesMap.end() && "Opcode does not have fillTypes");
  return It->second;  

}

inline IntegerFillType 
  WideningIntegerArithmetic::getOrNullFillType(
            FillTypeSet availableFillTypes,
            IntegerFillType LeftFillType, IntegerFillType RightFillType){
 
  for (auto FillType : availableFillTypes ){
    if(std::get<0>(FillType) == LeftFillType && 
       std::get<1>(FillType) == RightFillType)
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

void WideningIntegerArithmetic::solve(){
 
  SDValue Root = DAG.getRoot();
  setFillType(Root.getValueType(), Root.getValueType());
  
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
  auto isVisited = solvedNodes.find(Node->getNodeId());
  if(isVisited != solvedNodes.end())
    return true;
  
  return false;
}




WideningIntegerArithmetic::SolutionSet 
WideningIntegerArithmetic::closure(SolutionSet Sols){
  SolutionSet NewSolutions = Sols; // TODO check
  do{
    for(auto Solution : Sols){
      SDNode *Node = Solution->getNode();
      auto FillSols = visitFILL(Node);
      auto WidenSSols = visitWIDEN(Node);
      auto NarrowSols = visitNARROW(Node);
      auto WidenGarbageSols = visitWIDEN_GARBAGE(Node);
      auto DropedExt = visitDROP_EXT(Node);
      auto DropedTruncCopy = visitDROP_LO_COPY(Node);
      auto DropedTruncIgnore = visitDROP_LO_IGNORE(Node);
      auto Naturals = visitNATURAL(Node);
      
      // Append all solutions from VISIT*** into the vector FillSolutions
      FillSols.insert(Fills.end(), std::make_move_iterator(WidenSSols.begin())
                                 , std::make_move_iterator(WidenSSols.end())); 
      FillSols.insert(Fills.end(), std::make_move_iterator(NarrowSols.begin())
                                 , std::make_move_iterator(NarrowSols.end())); 
      FillSols.insert(Fills.end(), std::make_move_iterator(WidenGarbageSols.begin())
                                 , std::make_move_iterator(WidenGarbageSols.end())); 
      FillSols.insert(Fills.end(), std::make_move_iterator(DropedExt.begin())
                                 , std::make_move_iterator(DropedExt.end())); 
      FillSols.insert(Fills.end(), std::make_move_iterator(DropedTruncCopy.begin())
                                 , std::make_move_iterator(DropedTruncCopy.end())); 
      FillSols.insert(Fills.end(), std::make_move_iterator(DropedTruncIgnore.begin())
                                 , std::make_move_iterator(DropedTruncIgnore.end())); 
      FillSols.insert(Fills.end(), std::make_move_iterator(Naturals.begin())
                                 , std::make_move_iterator(Naturals.end())); 
      
      for(auto NewSolution : FillSolutions){
        addNonRedudant(NewSolutions, NewSolution); 
      }
    }
    
  }while(NewSolutions != Sols);
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
   
SolutionSet visitUNOP(SDNode *Node){
  unsigned char FillTypeWidth;
  unsigned char w1 = getScalarSize(Node->getOperand(0).getValueType(0));
  unsigned char w = getScalarSize(Node->getValueType(0)); 
  SolutionSet NewSols;
  FillTypeSet Fills = getFillTypes(Node->getOpcode());
  bool TryClosure = false;
 
  FillTypeWidth = getTargetWidth() - w;
  // Available Solutions of child 
  auto Sols = AvailableSolutions[Node->getOperand(0)->getNode()->getNodeId()];
  for(auto Sol : Sols){
     
    auto Unop = new WIA_UNOP();
    Unop->setFillType();  
    Unop->setFillTypeWidth(FillTypeWidth);
    Unop->setWidth(Width);
    Unop->setUpdatedWidth(getTargetWidth());
    Unop->setCost(Sol->getCost());
    Unop->setNode(Node);
    Unop.getOperands().push_back(Sol);  // TODO implement push_back operand in WI sol Info 
    Unop->setOperands(Unop.getOperands());
    NewSols.push_back(Sol)
  }
  // call closure if multiple FillTypes or Multiple Unop Widths
  if(Fills.size () > 1 )
    return closure(NewSols());
 
  unsigned Size = Node->getValueType(0).getSizeInBits();
  for(auto IntegerSize : IntegerSizes){
    if(IntegerSize == Size)
      continue; 
    EVT NewVT = EVT::getIntegerVT(*DAG.getContext(), IntegerSize);
    if(TLI.isTypeDesirableForOp(Node->getOpcode(),newVT))
     return closure(NewSols); 
  return NewSols;  
}
  
WideningIntegerArithmetic::SolutionSet 
WideningIntegerArithmetic::visitLOAD(LoadSDNode *Node){
 
  // TODO very far in the future distiguish load that the users of the load 
  // use only a smaller width and ignore the upper bits


  // TODO do we need to return multiple loads? 
  unsigned char Width = getScalarSize(Node->getOperand(0).getValueType(0));  
  unsigned char FillTypeWidth = getTargetWidth() - Width; 
  auto Sol = new WIA_LOAD();
  IntegerFillType FillType; 
  // distiguish EXTLOAD and NON ext LOAD Non EXT needs to be extended if the upper bits are used
  switch(Node->getExtensionType()){
    case ISD::NON_EXTLOAD: FillType = IntegerFillType::ANYTHING; 
    case ISD::EXTLOAD: FillType = IntegerFillType::ANYTHING; 
    case ISD::SEXTLOAD: FillType = IntegerFillType::SIGN;
    case ISD::ZEXTLOAD: FillType = IntegerFillType::ZEROS;
  }
  Sol->setFillType(FillType);  
  Sol->setFillTypeWidth(FillTypeWidth);
  Sol->setWidth(Width);
  Sol->setUpdatedWidth(0);
  Sol->setCost(0);
  Sol->setNode(Node);
  SolutionSet Sols;
  Sols.push_back(Sol);
  return Sols;
}
    
// Return all the solution based on binop rules op1 x op2 -> W
WideningIntegerArithmetic::SolutionSet 
WideningIntegerArithmetic::visitBINOP(SolutionType Node){
  
  SDNode *N0 = Node->getOperand(0);
  SDNode *N1 = Node->getOperand(1);

  SolusionSet newSolutions;
  // get All the available solutions from nodes 
  SolutionSet LeftSols = 
      AvailableSolutions.find(N0->getNodeId())->second;
  SolutionSet  RightSols = 
      AvailableSolutions.find(N1->getNodeId())->second;

  unsigned NodeId = Node->getNodeId(); 
  FillTypeSet OperandFillTypes = getOperandFillTypes(Node);
  // A function that combines solutions from operands left and right
  auto combineBinOp = [&](SolutionType Node, auto SolsLeft, auto SolsRight){    
    for (auto leftSolution : SolsLeft){
      for(auto rightSolution : SolsRight){
        // Test whether current binary operator has the available
        // fill type provided by leftSolution and rightSolution
        auto FillType = getOrNullFillType((OperandFillTypes),
                                    leftSolution->getFillType(),
                                    rightSolution->getFillType());
        if(FillType == UNDEFINED)
          continue;
        unsigned char Cost = leftSolution->getCost() + rightSolution->getCost();
        unsigned char w1 = leftSolution->getUpdatedWidth();
        unsigned char w2 = rightSolution->getUpdatedWidth();
        unsigned char UpdatedWidth = getBinOpTargetWidth(Node->getOpcode(), 
                                                         w1, w2);
        unsigned char Width = getScalarSize(Node->getValueType(0));  
        unsigned char FillTypeWidth = getTargetWidth() - UpdatedWidth(); 
                      // this Target of this Binary operator of the form
                      // width1 x width2 = newWidth
                      // for example on rv64 we have addw
                      // that is of the form i32 x i32 -> i32 
                      // and stored in a sign extended format to i64

        auto Sol = new WIA_BINOP(Node->getOpcode(), FillType, 
            FillTypeWidth, Node->getWidth(), UpdatedWidth, Cost, Node->getNode());

        AvailableSolutions[NodeId].push_back(Sol); 
        newSolutions.push_back(Sol); 
      }
    }
  }(Node, LeftSols, RightSols);
 
  // We need to call closure if we have multiple fillTypes or when
  // we have more than one target instr for the same operand 
  // TODO define newVT
  if(OperandFillTypes.size() > 1 || TLI.isTypeDesirableForOp(Node->getOpcode(),newVT))
  newSolutions = closure(newSolutions);
  // TODO append all the newSolutions in AvailableSolutions[NodeId]
  return AvailableSolutions[NodeId];
    
}

WideningIntegerArithmetic::SolutionSet  WideningIntegerArithmetic::visitFILL(
                          SDNode *Node){
  
  SolutionSet Sols;
  unsigned ExtensionOpc = ExtensionChoice == SIGN ? ISD::SIGN_EXTEND :
                                                    ISD::ZERO_EXTEND;
  // TODO check if targets support this how to check??
  // target must have sxlo(w->w') or just add appropriate instructions
  EVT VT = Node->getValueType(0);
  if(!TLI.isOperationLegalOrCustom(ISD::SIGN_EXTEND_INREG, VT))
    return NULL;
   
  unsigned char Width = getScalarSize(Node->getValueType(0));  
  unsigned char FillTypeWidth = getTargetWidth() - Width; 
  
  auto Sols = AvailableSolutions[Node->getNodeId()];
  for(auto Sol : Sols){
    if(Sol->getOpcode() == ISD::SIGN_EXTEND_INREG)
      continue;    // TODO CHECK is FILL a ISD::SIGN_EXTEND_INREG ??
    // WIA_FILL extends the least significant *Width* bits of SDNode
    // to targetWidth
    WideningIntegerSolutionInfo *Fill = new WIA_FILL(
      ExtensionOpc, ExtensionChoice, FillTypeWidth, Width,
      getTargetWidth() , Sol->getCost() + 1 );
    
    Fill->addOperand(Sol);
    Sols.push_back(Fill);  
  }
   
  
  return Sols;
}
  
WideningIntegerArithmetic::SolutionSet WideningIntegerArithmetic::visitWIDEN(
                          SDNode *Node){
 
  // TODO how to check for Type S? depends on target ? or on this operand
  // if(!hasTypeS(Sol->getFillType())
  //  return NULL; 
  
  SolutionSet Sols;
  unsigned char Width = getScalarSize(Node->getOperand(0).getValueType(0));  
  unsigned char FillTypeWidth = getTargetWidth() - Width; 
 
  unsigned ExtensionOpc = ExtensionChoice == SIGN ? ISD::SIGN_EXTEND :
                                                    ISD::ZERO_EXTEND;
  auto Sols = AvailableSolutions[Node->getNodeId()];
  for(auto Sol : Sols){
    if(isExtOpcode(Sol->getOpcode())
      continue;
    // Results to a widened expr
    WideningIntegerSolutionInfo *Widen = new WIA_WIDEN(
      ExtensionOpc, ExtensionChoice, FillTypeWidth, Width,
      getTargetWidth() , Sol->getCost() + 1 );
    Widen->addOperand(Sol);
    
    Widen->addOperand(Sol);
    Sols.push_back(GarbageWiden); 
    
  }

  Sols.push_back(Widen); 
  return Sols;
}
    

WideningIntegerArithmetic::SolutionSet WideningIntegerArithmetic::visitWIDEN_GARBAGE(
      SDNode *Node){

  SolutionSet Sols;
  unsigned char Width = getScalarSize(Node->getOperand(0).getValueType(0));  
  unsigned char FillTypeWidth = getTargetWidth() - Width;
  unsigned ExtensionOpc = ISD::ANY_EXTEND  // Results to a garbage widened
  auto Sols = AvailableSolutions[Node->getNodeId()];
  for(auto Sol : Sols){
    if(isExtOpcode(Sol->getOpcode())
      continue;
    WideningIntegerSolutionInfo *GarbageWiden = new WIA_WIDEN(
      ExtensionOpc, ANYTHING, FillTypeWidth,  Width, getTargetWidth(), 
      Sol->getCost() + 1 );
    GarbageWiden->addOperand(Sol);
    Sols.push_back(GarbageWiden); 
    
  }
  
  return Sols;
}
    
WideningIntegerArithmetic::SolutionSet WideningIntegerArithmetic::visitNARROW(
                          SDNode *Node){
  

  if(!hasTypeT(Sol->getFillType())
    return NULL;  
  
  unsigned char Width = getScalarSize(Node->getOperand(0).getValueType(0));  
  unsigned char FillTypeWidth = getTargetWidth() - Width;

  // TODO check isTruncateFree on some targets and widths.
  // Not sure the kinds of truncate of the Target will determine it.
  unsigned ExtensionOpc = ExtensionChoice == SIGN ? ISD::SIGN_EXTEND :
                                                    ISD::ZERO_EXTEND;
  
  unsigned char UpdatedWidth = 32; // TODO get info from targets. 
  // Check truncate size for Machine
  // for example rv64 has zext for truncate
  // or store 32 
  // Narrowing on targets how they are implemented?? 
  WideningIntegerSolutionInfo *Trunc = new WIA_NARROW(ExtensionOpc,
    // Will depend on available Narrowing , 
    FillTypeWidth, Width, UpdatedWidth, Sol->getCost() + 1 );
  Trunc->addOperand(Sol);
  
  SolutionSet Sols;
  Sols.push_back(Trunc); 
  return Sols;
}
    
WideningIntegerArithmetic::SolutionSet WideningIntegerArithmetic::visitDROP_EXT(
                          SDNode *Node){
  
  if(!hasTypeS(Sol->getFillType()))
    return NULL;
  
  SDNode *N0 = Node->getOperand(0)->getNode();
  unsigned char Width = getScalarSize(Node->getOperand(0).getValueType(0));  
  unsigned char FillTypeWidth = getTargetWidth() - Width;
 
  WideningIntegerSolutionInfo *FIXME = 
                      *(Sol->getOperands().begin());
  unsigned ExprOpc = N0->getOpcode();
  // We simply drop the extension and we will later see if it's needed.
  WideningIntegerSolutionInfo *Expr = new WIA_DROP_EXT(ExtOpc,
    N0->getFillType(), FillTypeWidth, Width, 
    Node, N0->getCost());
  Expr->setOperands(N0->getOperands());
  
  SolutionSet Sols;
  Sols.push_back(expr) 
  return Sols;

}


  
WideningIntegerArithmetic::SolutionSet 
  WideningIntegerArithmetic::visitDROP_LO_COPY(
                          SDNode *Node){
  // TRUNCATE has only 1 operand
  WideningIntegerSolutionInfo *N0 = *(Sol->getOperands().begin());

  if(!hasTypeT(Sol->getFillType()))
    return NULL;

  unsigned char NewWidth = getScalarSize(Node->getOperand(0).getValueType(0));  
  // We simply drop the truncation and we will later see if it's needed.
  unsigned ExtensionOpc = ExtensionChoice == SIGN ? ISD::SIGN_EXTEND :
                                                    ISD::ZERO_EXTEND;
  WideningIntegerSolutionInfo *Expr = new WIA_DROP_LOCOPY(ExtensionOpc,
    N0->getFillType(), N0->getFillTypeWidth(), N0->getWidth(), 
    NewWidth, N0->getCost());
  Expr->setOperands(N0->getOperands());
  SolutionSet Sols;
  Sols.push_back(expr) 
  return Expr;
}


WideningIntegerArithmetic::SolutionSet 
WideningIntegerArithmetic::visitDROP_LO_IGNORE(
                          WideningIntegerSolutionInfo *Sol){
  
  if(!hasTypeT(Sol->getFillType()))
    return NULL;
  
  // TRUNCATE has only 1 operand
  WideningIntegerSolutionInfo *N0 = 
                Sol->getOperands().begin()->second;
  
  unsigned char FillTypeWidth = getTargetWidth() - Sol->getWidth();
 
  // We simply drop the truncation and we will later see if it's needed.
  unsigned ExtensionOpc = ExtensionChoice == SIGN ? ISD::SIGN_EXTEND :
                                                    ISD::ZERO_EXTEND;
  WideningIntegerSolutionInfo *Expr = new WIA_DROP_LOIGNORE(ExtensionOpc,
    N0->getFillType(), FillTypeWidth, N0->getWidth(), 
    N0->getUpdatedWidth(), N0->getCost());
  Expr->setOperands(N0->getOperands());
  
  SolutionSet Sols;
  Sols.push_back(expr) 
  return Expr;
}

WideningIntegerArithmetic::SolutionSet WideningIntegerArithmetic::visitEXTLO(
                          WideningIntegerSolutionInfo *LeftSol,
                          WideningIntegerSolutionInfo *RightSol){
  unsigned ExtensionOpc = ExtensionChoice == SIGN ? ISD::SIGN_EXTEND :
                                                    ISD::ZERO_EXTEND;
  unsigned cost = LeftSol->getCost() + RightSol->getCost() + 1;
  unsigned n = LeftSol->getFillTypeWidth();
  newWidth = getTargetWidth(); // TODO check for x86 that has multiple width registers
  // check that LeftSol->getWidth == RightSol->getWidth &&
  // check that Leftsol->fillTypeWidth == RightSol->fillTypeWidth
  // check that LeftSol->fillType = Zeros and LeftSol->fillType = Garbage
  if(LeftSol->getWidth() != RightSol->getWidth() || 
     LeftSol->getFillTypeWidth() != RightSol->getFillTypeWidth() ||
     (LeftSol->getFillType() != ZEROS && !hasTypeGarbage(RightSol->getFillType()) ){
    return NULL;
  }
  
  WideningIntegerSolutionInfo *Expr = new WIA_EXTLO(ExtensionOpc,k
    ExtensionChoice, LeftSol->getFillTypeWidth(), OldWidth, NewWidth, cost); 
  
  SolutionSet Sols;
  Sols.push_back(expr) 
  
  return Expr; 
}
    
WideningIntegerArithmetic::SolutionSet WideningIntegerArithmetic::visitSUBSUME_FILL(
                          WideningIntegerSolutionInfo *Sol){
  return NULL;
}
    
WideningIntegerArithmetic::SolutionSet WideningIntegerArithmetic::visitSUBSUME_INDEX(
                          WideningIntegerSolutionInfo *Sol){
  
  // We implement this rule implicitly 
  // On every solution we consider the index n to stand
  // for all indices from n to w.
  SolutionSet Set;
  return Set;

}
    
WideningIntegerArithmetic::SolutionSet WideningIntegerArithmetic::visitNATURAL(
                          WideningIntegerSolutionInfo *Sol){
 
  if(!hasTypeGarbage(Sol->getFillType())
    return NULL; 
  unsigned ExtensionOpc = ExtensionChoice == SIGN ? ISD::SIGN_EXTEND :
                                                    ISD::ZERO_EXTEND;
  
  WideningIntegerSolutionInfo *Expr = new WIA_NATURAL(
    ExtensionOpc, ExtensionChoice , Sol->getFillTypeWidth(), 
    Sol->getWidth(), Sol->getUpdatedWidth(), Sol->getCost());
 
  SolutionSet Sols;
  Sols.push_back(expr) 
  return Sols;

}

unsigned char WideningIntegerArithmetic::getTargetWidth(){
  const auto &Triple = DAG.getTarget().getTriple();
  switch (Triple.getArchType()){
    default: break;
    case Triple::riscv64: return 64;
    case Triple::riscv32: return 32;
    

  }
  return 0;
}

unsigned char WideningIntegerArithmetic::getBinOpTargetWidth(
                        unsigned opc, unsigned char w1, unsigned char w2){
    
    
  const auto &Triple = DAG.getTarget().getTriple();
  switch (Triple.getArchType()){
    default: break;
    case Triple::riscv64:{

      OperatorWidthsMap WidthsMap = TargetWidths[Triple.getArchType()];
      WidthsSet Widths = WidthsMap[opc];
      auto TupleIt = std::find_if(Widths.begin(), Widths.end(), 
          [](const std::tuple<unsigned char , unsigned char , unsigned char>& e ){
            return (std::get<0>(e) == w1 && std::get<1>(e) == w2);
          });
      if(TupleIt != Widths.end() ){
        return std::get<2>(*TupleIt);
      }else {
        return 0;
      }
    }
  }
  return 0;
}
                          

inline WideningIntegerArithmetic::BinOpWidth 
WideningIntegerArithmetic createWidth(unsigned char op1, 
               unsigned char op2, unsigned dst){
  return std::make_tuple<unsigned char, unsigned char, unsigned char>(op1, op2, dst);
}



void WideningIntegerArithmetic::initTargetWidthTables(){
    
  
  
    OperatorWidthsMap RISCVOpsMap, ARMOpsMap, X86OpsMap;
    WidthsSet RISCVAdd, RISCVSUB, RISCVShiftLeft, RISCVShiftRight,
              RISCVLoad, RISCVStore, RISCVMul, RISCVDiv, RISCVRem 
              RISCVAnd, RISCVSub, RISCVOr, RISCVXor, RISCVARMAdd, X86Add;
    // for RVI64  
    RISCVAdd.insert(createwidth(32, 32,32));
    RISCVAdd.insert(createWidth(64, 64, 64));

    RISCVShiftLeft.insert(createWidth(32, 32, 32));
    RISCVShiftLeft.insert(createWidth(64, 64, 64));
    
    RISCVShiftRight.insert(createWidth(32, 32, 32));
    RISCVShiftRight.insert(createWidth(64, 64, 64));

    RISCVSub.insert(createWidth(32, 32,32));
    RISCVSub.insert(createWidth(64, 64, 64));

    RISCVLoad.insert(createWidth(64, 64, 64);
    
    // TODO Distiguish LW AND LWU
    // LW loads a 32-bit value from memory and sign-extends this to 
    // 64 bits before storing it in register rd
    // The LWU instruction, on the other hand, zero-extends 
    // the 32-bit value from memory
    RISCVLoad.insert(createWidth(32, 32, 32); 

    RISCVMul.insert(createwidth(64, 64, 64));
    
    RISCVDiv.insert(createWidth(64, 64, 64));
    RISCVDiv.insert(createWidth(32, 32, 32));
    
    RISCVRem.insert(createWidth(64, 64, 64));
    RISCVRem.insert(createWidth(32, 32, 32));
       

    RISCVStore.insert(createWidth(8, 8, 8));
    RISCVStore.insert(createWidth(16, 16, 16));
    RISCVStore.insert(createWidth(32, 32, 32));
    RISCVStore.insert(createWidth(64, 64, 64));
    
    RISCVOpsMap[ISD::ADD] = RISCVAdd;
    RISCVOpsMap[ISD::SUB] = RISCVMul;
    RISCVOpsMap[ISD::SUB] = RISCVSub;
    RISCVOpsMap[ISD::SHL] = RISCVShiftLeft; 
    RISCVOpsMap[ISD::SRL] = RISCVShiftRight; 
    RISCVOpsMap[ISD::STORE] = RISCVStore;
    RISCVOpsMap[ISD::LOAD] = RISCVLoad;
    RISCVOpsMap[ISD::MUL] = RISCVMul;
    RISCVOpsMap[ISD::SDIV] = RISCVDiv;
    RISCVOpsMap[ISD::UDIV] = RISCVDiv; // TODO check is UDIV and SDIV the same in RV64?
    RISCVOpsMap[ISD::SREM] = RISCVRem;
    RISCVOpsMap[ISD::UREM] = RISCVRem; // TODO check is UREM and SREM the same in RV64?
    
 
    TargetWidths[Triple::riscv64] = RISCVOpsMap; 
 
    // ARM  
    ARMAdd.insert(createWidth(32, 32, 32));
    ARMAdd.insert(createWidth(64, 64, 64));
    ARMAdd.insert(createWidth(64, 32, 64)) // zero extends 32 bit internally
    
    X86Add.insert(createWidth(64, 64, 64));
    X86Add.insert(createWidth(32, 32, 32));
    X86Add.insert(createWidth(16, 16, 16));
    X86Add.insert(createWidth(8, 8, 8));
   
    
    ARMOpsMap[ISD::ADD].push_back(ARMAdd);
    
    X86OpsMap[ISD::ADD].push_back(X86Add); 
}


void WideningIntegerArithmetic::initOperatorsFillTypes(){

    // FillTypes are of the form op1 x op2 -> result
    // Stored in a tuple <op1, op2, result>
    using FillTypeSet = SmallSet<tuple<IntegerFillType,
                                       IntegerFillType,
                                       IntegerFillType> 4>;
    FillTypeSet AddFillTypes;
    AddFillTypes.insert(make_tuple(ANYTHING, ANYTHING, ANYTHING));
    FillTypesMap.push_back(ISD::ADD, AddFillTypes);
    
    FillTypeSet AndFillTypes;
    AndFillTypes.insert(make_tuple(SIGN, SIGN, SIGN));
    AndFillTypes.insert(make_tuple(ZEROS, ANYTHING, ZEROS));
    AndFillTypes.insert(make_tuple(ANYTHING, ZEROS, ZEROS));
    AndFillTypes.insert(make_tuple(ANYTHING, ANYTHING, ANYTHING));
    
    FillTypesMap.push_back(ISD::AND, AndFillTypes);

    FillTypeSet UDivFillTypes, SDivFillTypes;
    UDivFillTypes.insert(make_tuple(SIGN, SING, SIGN));
    SDivFillTypes.insert(make_tuple(ZEROS, ZEROS, ZEROS));
    
    FillTypesMap.push_back(ISD::SDIV, SDivFillTypes);
    FillTypesMap.push_back(ISD::UDIV, UDivFillTypes);

    FillTypeSet Eq, Ge, Geu, Gt, Gtu, Le, Leu, Lt, Ltu, Mod, Modu,
                Mod, Modu, Mul, Mulu, Ne, Neg, Or, Popcnt, Rem,
                Shl, Shra, Shrl, Sub;
    Eq.insert(make_tuple(SIGN, SIGN, SIGN));
    Eq.insert(make_tuple(ZEROS, ZEROS, ZEROS));
    Ge.insert(make_tuple(SIGN, SING, SIGN));
    Geu.insert(make_tuple(SIGN, SIGN, SIGN));
    Geu.insert(make_tuple(ZEROS, ZEROS, ZEROS));
    Gt.insert(make_tuple(SIGN, SIGN, SIGN));
    Gtu.insert(make_tuple(SIGN, SIGN, ZEROS);
    Gtu.insert(make_tuple(ZEROS, ZEROS, ZEROS);
    Le.insert(make_tuple(SIGN, SIGN, SIGN));
    Leu.insert(make_tuple(SIGN, SIGN, SIGN));
    Leu.insert(make_tuple(ZEROS, ZEROS, ZEROS));
    Lt.insert(make_tuple(SIGN, SIGN, SIGN));
    Ltu.insert(make_tuple(SIGN, SIGN, ZEROS);
    Ltu.insert(make_tuple(ZEROS, ZEROS, ZEROS));
    
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








