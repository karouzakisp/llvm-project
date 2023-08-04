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
    using isSolvedMap = DenseMap<int, bool>;
    using SolutionSet = SmallVector<WideningIntegerSolutionInfo *>;
    using SolutionSetParam = SmallVectorImpl<WideningIntegerSolutionInfo * >;    
    using AvailableSolutionsMap = DenseMap<int, SolutionSet>;
    
    using BinOpWidth = std::tuple<unsigned char, unsigned char, unsigned char>;
    using WidthsSet = SmallVector<BinOpWidth>;
    using OperatorWidthsMap = DenseMap<unsigned, WidthsSet>;
    using TargetWidthsMap = DenseMap<ArchType, OperatorWidthsMap>;
    
    using FillTypeSet = SmallSet<std::tuple<IntegerFillType, IntegerFillType, IntegerFillType>, 4>;
    
    using OperatorFillTypesMap = DenseMap<unsigned, FillTypeSet>;
  private:
    enum IntegerFillType ExtensionChoice;   
 
    // checks whether a SDNode in the DAG is visited and solved` 
    isSolvedMap solvedNodes;
    
    // Holds all the available solutions 
    AvailableSolutionsMap AvailableSolutions;

  
    TargetWidthsMap TargetWidths;
    
    unsigned char getBinOpWidth(unsigned opCode, unsigned char w1, unsigned char w2);
    void initTargetWidthTables();
    
                                        
                                        // opcode , FillTypeSet 
    OperatorFillTypesMap FillTypesMap; 
    inline FillTypeSet getFillTypes(unsigned Opcode);
    inline IntegerFillType WideningIntegerArithmetic::getOrNullFillType(
            FillTypeSet availableFillTypes, IntegerFillType Left, 
            IntegerFillType Right);

    SmallVector<WideningIntegerSolutionInfo *> visit_widening(SDNode *Node);
    SolutionSet visitInstruction(SDNode *Node);
    SolutionSet visitANY_EXT_OR_TRUNC(SDNode *Node);
    SolutionSet visitXOR(SDNode *Node);
    SolutionSet visitADD(SDNode *Node);
    SolutionSet visitADDO(SDNode *Node);
  
    void setFillType(EVT SrcVt, EVT DstVT);  
    bool isInteger(SDNode *Node);
    bool isSolved(SDNode *Node);
    void combineBinOp(SDNode *N, SolutionSet leftSols, 
                          SolutionSet rightSols);

    bool AddNonRedudant(SolutionSet &Sol, Solution GeneratedSol);
    inline bool hasTypeGarbage(IntegerFillType fill);
    inline bool hasTypeT(IntegerFillType fill);
    inline bool hasTypeS(IntegerFillType fill);

    
    
    SolutionSet findAllSolutions(SDNode *N);    
    SolutionSet visitBINOP(SDNode BinOp, SDNode *N0, SDNode *N1);
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
     
};


SmallVector<WideningIntegerSolutionInfo *> WideningIntegerArithmetic::visitInstruction(SDNode *Node){
  SolutionSet Solutions;
  switch(Node->getOpcode()){
    default: break;
    case ISD::SIGN_EXTEND:
    case ISD::ZERO_EXTEND:    
    case ISD::TRUNCATE:       return visitANY_EXT_OR_TRUNC(Node);
    case ISD::XOR:            return visitXOR(Node); 
    case ISD::ADD:            return visitADD(Node);
    case ISD::UADDO:          return visitADDO(Node);
  }
  return Solutions;
}

SmallVector<WideningIntegerSolutionInfo *> WideningIntegerArithmetic::visit_widening(SDNode *Node){

  for (const SDValue &value : Node->op_values() ){    
    SDNode *OperandNode = value.getNode(); // #TODO check value size is 1?
    SolutionSet Sols = visit_widening(OperandNode);
  }
  
  return visitInstruction(Node);
}



inline FillTypeSet  WideningIntegerArithmetic::getFillTypes(unsigned Opcode){

  auto It = FillTypesMap.find(Opcode);
  assert(It != FillTypesMap.end() && "Opcode does not have fillTypes" );   
  return It->second;
}

inline IntegerFillType getOrNullFillType(FillTypeSet availableFillTypes,
            IntegerFillType Left, IntegerFillType Right){
 
  for (auto FillType : availableFillTypes ){
    if(std::get<0>(FillType) == Left && std::get<1>(FillType) == Right)
      return std::get<2>(FillType);
  }
  return UNDEFINED; 
}


SolutionSet WideningIntegerArithmetic::visitXOR(SDNode *Node ){
  SDValue N0 = Node->getOperand(0);
  SDValue N1 = Node->getOperand(1);
  FillTypeSet OperandFillTypes;
  OperandFillTypes.insert(SIGN);
  OperandFillTypes.insert(ZEROS);
  OperandFillTypes.insert(ANYTHING);

  // get All the available solutions from nodes 
  SolutionSet LeftSols = 
      AvailableSolutions.find((N0.getNode())->getNodeId())->second;
  SolutionSet  RightSols = 
      AvailableSolutions.find((N1.getNode())->getNodeId())->second;
  // add A function that combines the solution
  combineSolutions(Node->getNodeId(), Node->getOpcode(),  LeftSols,
                   RightSols,   OperandFillTypes);
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
  return SDValue(Node, 0);
}


SDValue WideningIntegerArithmetic::visitANY_EXT_OR_TRUNC(SDNode *Node){
  SDValue N0 = Node->getOperand(0);
 
  // TODO convert node to Set of WideningSolutionInfo's 
  // and generate new solutions based on operands
  // just drop the extension or truncation 
  // we will measure later if this produces incorrect results
    
  return N0; 
  
}

SDValue WideningIntegerArithmetic::visitADD(SDNode *Node ){
  SDValue N0 = Node->getOperand(0);
  SDValue N1 = Node->getOperand(1);
  bool IsSigned = (ISD::SADDO == Node->getOpcode());
  FillTypeSet OperandFillTypes;
  
  OperandFillTypes.insert(ANYTHING);

  // TODO check if it overflows based on ISD::ADD
  // and if it does not for sure we can add SIGN AND ZEROS ?
  if(!IsSigned){
    if(DAG.computeOverflowKind(N0, N1 ) == SelectionDAG::OFK_Never){
      OperandFillTypes.insert(SIGN);
      OperandFillTypes.insert(ZEROS);
    }
  }
    
  // get All the available solutions from nodes 
  SolutionSet LeftSols = 
      AvailableSolutions.find((N0.getNode())->getNodeId())->second;
  SolutionSet  RightSols = 
      AvailableSolutions.find((N1.getNode())->getNodeId())->second;
  
  combineSolutions(Node->getNodeId(), Node->getOpcode(),  LeftSols,
                   RightSols,   OperandFillTypes);
   
  return SDValue(Node, 0); 

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
      if(!isSolved(&Node)  && isInteger(&Node) ){ 
        visit_widening(&Node);  
      }
    
  }
} 

bool WideningIntegerArithmetic::isInteger(SDNode *Node){
  return Node->getValueType(0).isInteger();
}

// Checks whether a SDNode is visited 
// so we don't visit and solve the same Node again
bool WideningIntegerArithmetic::isSolved(SDNode *Node){
  auto isVisited = solvedNodes.find(Node->getNodeId());
  if(isVisited == solvedNodes.end())
    return false;
  else
    return true; 
  return true;
}



SolutionSet WideningIntegerArithmetic::findAllSolutions(
                      WideningIntegerSolutionInfo *Sol){
  return NULL;
}

SolutionSet WideningIntegerArithmetic::closure(SolutionSet Sols){
  SolutionSet NewSolutions = Sols; // TODO check
  do
    for(auto Solution : Sols){
      FillSolutions = visitFILL(Solution);
      // Append all solutions from VISIT*** into the vector FillSolutions
      for(auto NewSolution : FillSolutions){
        addNonRedudant(NewSolutions, NewSolution); 
      }
    }
    
  while(NewSolutions != Sols) 
  return Sols;
} 
    
// Return all the solution based on binop rules op1 x op2 -> W
SolutionSet visitBINOP(SDNode *Node){
  
  SDValue N0 = Node->getOperand(0);
  SDValue N1 = Node->getOperand(1);

  if(!isSolved(N0))
    visitInstruction(N0);
  if(!isSolved(N1))
    visitInstruction(N1);
  

  // get All the available solutions from nodes 
  SolutionSet LeftSols = 
      AvailableSolutions.find((N0.getNode())->getNodeId())->second;
  SolutionSet  RightSols = 
      AvailableSolutions.find((N1.getNode())->getNodeId())->second;
  // A function that combines solutions from operands left and right
  auto combineBinOp = [&](SDNode *Node, auto SolsLeft, auto SolsRight){    
    FillTypeSet OperandFillTypes = getOperandFillTypes(Node->getOpcode());
    for (auto leftSolution : SolsLeft){
      for(auto rightSolution : SolsRight){
        // Test whether current binary operator has the available
        // fill type provided by leftSolution and rightSolution
        auto FillType = getOrNullFillType(make_tuple(OperandFillTypes),
                                    leftSolution->getFillType(),
                                    rightSolution->getFillType());
        if(FillType == UNDEFINED)
          continue;
 
        unsigned char Cost = leftSolution->getCost() + rightSolution->getCost();
        unsigned char w1 = leftSolution->getUpdatedWidth();
        unsigned char w2 = rightSolution->getUpdatedWidth();
        unsigned char w = // get width from target 
        auto Sol = new WIA_BINOP(Opcode, 
          FillType, NewWidth, NewWidth, Cost);

        AvailableSolutions[NodeId].push_back(Sol); 
          
      }
    }
  }(Node, LeftSols, RightSols);
  
  
  unsigned opc = BinOp->getOpcode();
  SolutionSetParam 
  unsigned char newWidth = // TODO need to check what width is available for
                      // this Target of this Binary operator of the form
                      // width1 x width2 = newWidth
                      // for example on rv64 we have addw
                      // that is of the form i32 x i32 -> i32 
                      // and stored in a sign extended format to i64
  unsigned char FillTypeWidth; // TODO get it from target 
  WideningIntegerSolutionInfo *Binop = new WIA_BINOP(opc,
    ExtensionChoice, BinOperator->getWidth(), newWidth, 
    BinOperator->getCost() + 1);
  
  SolutionSet Sols;
  Sols.push_back(Binop) 
  return Sols;
    
}

SolutionSet  WideningIntegerArithmetic::visitFILL(
                          WideningIntegerSolutionInfo *Sol){
  unsigned ExtensionOpc = ExtensionChoice == SIGN ? ISD::SIGN_EXTEND :
                                                    ISD::ZERO_EXTEND;
  // TODO check if targets support this how to check??
  // target must have sxlo(w->w') or just add appropriate instructions
  if(!hasTypeGarbage(Sol->getFillType()){
    return NULL;
  }
   
  // Results to a truncate
  WideningIntegerSolutionInfo *Fill = new WIA_FILL(
    ExtensionOpc, ExtensionChoice, Sol->getFillTypeWidth(), Sol->getWidth(),
    64/* TODO get TARGET WIDTH */, Sol->getCost() + 1 );
  Fill->addOperand(Sol);
  
  SolutionSet Sols;
  Sols.push_back(Fill) 
  return Sols;
}
    
SolutionSet WideningIntegerArithmetic::visitWIDEN(
                          WideningIntegerSolutionInfo *Sol){
  
  if(!hasTypeS(Sol->getFillType())
    return NULL; 
 
  unsigned ExtensionOpc = ExtensionChoice == SIGN ? ISD::SIGN_EXTEND :
                                                    ISD::ZERO_EXTEND;
  // Results to a widened expr
  WideningIntegerSolutionInfo *Widen = new WIA_WIDEN(
    ExtensionOpc, ExtensionChoice, Sol->getFillTypeWidth(), Sol->getWidth(),
    64/* TODO get TARGET WIDTH */, Sol->getCost() + 1 );
  Widen->addOperand(Sol);

  SolutionSet Sols;
  Sols.push_back(Widen) 
  return Sols;
}
    

SolutionSet WideningIntegerArithmetic::visitWIDEN_GARBAGE(
      WideningIntegerSolutionInfo *Sol){
 
  if(!hasTypeGarbage(Sol->getFillType());

  unsigned ExtensionOpc = ISD::ANY_EXTEND  // Results to a garbage widened
  WideningIntegerSolutionInfo *GarbageWiden = new WIA_WIDEN(
    ExtensionOpc, ExtensionChoice, Sol->getFillTypeWidth(), 
    Sol->getWidth(),
    64/* TODO get TARGET WIDTH */, Sol->getCost() + 1 );
  GarbageWiden->addOperand(Sol);
  
  SolutionSet Sols;
  Sols.push_back(GarbageWiden) 
  return Sols;
}
    
SolutionSet WideningIntegerArithmetic::visitNARROW(
                          WideningIntegerSolutionInfo *Sol){
  

  if(!hasTypeT(Sol->getFillType())
    return NULL;  

  // TODO check isTruncateFree on some targets and widths.
  // Not sure the kinds of truncate of the Target will determine it.
  unsigned ExtensionOpc = ExtensionChoice == SIGN ? ISD::SIGN_EXTEND :
                                                    ISD::ZERO_EXTEND;
    
  // Check truncate size for Machine
  // for example rv64 has zext for truncate
  // or store 32 
  // Narrowing on targets how they are implemented?? 
  WideningIntegerSolutionInfo *Trunc = new WIA_NARROW(ExtensionOpc,
    // Will depend on available Narrowing , 
    Sol->getFillTypeWidth(), Sol->getWidth(),
    64/* TODO get Trunc Size */, Sol->getCost() + 1 );
  Trunc->addOperand(Sol);
  
  SolutionSet Sols;
  Sols.push_back(Trunc) 
  return Sols;
}
    
SolutionSet WideningIntegerArithmetic::visitDROP_EXT(
                          WideningIntegerSolutionInfo *Sol){
  
  if(!hasTypeS(Sol->getFillType()))
    return NULL;
  
  // SExt or ZExt have only 1 operand
  WideningIntegerSolutionInfo *N0 = 
                      *(Sol->getOperands().begin());
  unsigned ExprOpc = N0->getOpcode();
  // We simply drop the extension and we will later see if it's needed.
  WideningIntegerSolutionInfo *Expr = new WIA_DROP_EXT(ExtOpc,
    N0->getFillType(), N0->getFillTypeWidth(), N0->getWidth(), 
    N0->getUpdatedWidth(), N0->getCost());
  Expr->setOperands(N0->getOperands());
  
  SolutionSet Sols;
  Sols.push_back(expr) 
  return Sols;

}


  
SolutionSet WideningIntegerArithmetic::visitDROP_LO_COPY(
                          WideningIntegerSolutionInfo *Sol){
  // TRUNCATE has only 1 operand
  WideningIntegerSolutionInfo *N0 = *(Sol->getOperands().begin());

  if(!hasTypeT(Sol->getFillType()))
    return NULL;

  // We simply drop the truncation and we will later see if it's needed.
  unsigned ExtensionOpc = ExtensionChoice == SIGN ? ISD::SIGN_EXTEND :
                                                    ISD::ZERO_EXTEND;
  WideningIntegerSolutionInfo *Expr = new WIA_DROP_LOCOPY(ExtensionOpc,
    N0->getFillType(), N0->getFillTypeWidth(), N0->getWidth(), 
    N0->getUpdatedWidth(), N0->getCost());
  Expr->setOperands(N0->getOperands());
  SolutionSet Sols;
  Sols.push_back(expr) 
  return Expr;
}


SolutionSet WideningIntegerArithmetic::visitDROP_LO_IGNORE(
                          WideningIntegerSolutionInfo *Sol){
  
  if(!hasTypeT(Sol->getFillType()))
    return NULL;
  
  // TRUNCATE has only 1 operand
  WideningIntegerSolutionInfo *N0 = 
                Sol->getOperands().begin()->second;
  
  unsigned char FillTypeWidth = // TODO we find it? 
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

SolutionSet WideningIntegerArithmetic::visitEXTLO(
                          WideningIntegerSolutionInfo *LeftSol,
                          WideningIntegerSolutionInfo *RightSol){
  unsigned ExtensionOpc = ExtensionChoice == SIGN ? ISD::SIGN_EXTEND :
                                                    ISD::ZERO_EXTEND;
  unsigned cost = LeftSol->getCost() + RightSol->getCost() + 1;
  unsigned newOldWidth = 
  unsigned OldWidth, NewWidth; // TODO how to calculate OldWidth and NewWidth??
  OldWidth = LeftSol->getWidth();
  newWidth = 0; // TODO check
  // check that LeftSol->getWidth == RightSol->getWidth &&
  // check that Leftsol->fillTypeWidth == RightSol->fillTypeWidth
  // check that LeftSol->fillType = Zeros and LeftSol->fillType = Garbage
  if(LeftSol->getWidth() != RightSol->getWidth() || 
     LeftSol->getFillTypeWidth() != RightSol->getFillTypeWidth() ||
     (LeftSol->getFillType() != ZEROS && hasTypeGarbage(RightSol->getFillType()) ){
    return NULL;
  }
  
  WideningIntegerSolutionInfo *Expr = new WIA_EXTLO(ExtensionOpc,k
    ExtensionChoice, LeftSol->getFillTypeWidth(), OldWidth, NewWidth, cost); 
  
  SolutionSet Sols;
  Sols.push_back(expr) 
  
  return Expr; 
}
    
SolutionSet WideningIntegerArithmetic::visitSUBSUME_FILL(
                          WideningIntegerSolutionInfo *Sol){
  return NULL;
}
    
WideningIntegerSolutionInfo* WideningIntegerArithmetic::visitSUBSUME_INDEX(
                          WideningIntegerSolutionInfo *Sol){
  
  // We implement this rule implicitly 
  // On every solution we consider the index n to stand
  // for all indices from n to w.
  return NULL;


}
    
SolutionSet WideningIntegerArithmetic::visitNATURAL(
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

unsigned char WideningIntegerArithmetic::getBinOpTargetWidth(
                        unsigned opc, unsigned char w1, unsigned char w2){
    
    using BinOpWidth = std::tuple<unsigned char, unsigned char, unsigned char>;
    using WidthsSet = SmallVector<BinOpWidth>;
    using OperatorWidthsMap = DenseMap<unsigned, WidthsSet>;
    using TargetWidthsMap = DenseMap<ArchType, OperatorWidthsMap>;
    TargetWidthsMap TargetWidths;
  auto findSuitableArchType = [&](unsigned opc, unsigned
  const auto &Triple = DAG.getTarget().getTriple();
  switch (Triple.getArchType())
    case Triple::riscv64:
      return TargetWidths[Triple.getArchType()] 
}
                          

void WideningIntegerArithmetic::initTargetWidthTables(){
    
  
  
    OperatorWidthsMap RISCVOpsMap, ARMOpsMap, X86OpsMap;
    WidthsSet RISCVAdd, RISCVSUB, RISCVShiftLeft, RISCVShiftRight,
              RISCVLoad, RISCVStore, RISCVMul, RISCVDiv, RISCVRem 
              RISCVAnd, RISCVSub, RISCVOr, RISCVXor, RISCVARMAdd, X86Add;
    // for RVI64  
    RISCVAdd.insert(std::make_tuple(32, 32,32));
    RISCVAdd.insert(std::make_tuple(64, 64, 64));

    RISCVShiftLeft.insert(std::make_tuple(32, 32, 32));
    RISCVShiftLeft.insert(std::make_tuple(64, 64, 64));
    
    RISCVShiftRight.insert(std::make_tuple(32, 32, 32));
    RISCVShiftRight.insert(std::make_tuple(64, 64, 64));

    RISCVSub.insert(std::make_tuple(32, 32,32));
    RISCVSub.insert(std::make_tuple(64, 64, 64));

    // TODO How to add LUI ??
    RISCVLoad.insert(std::make_tuple(64, 64, 64);
    
    // TODO Distiguish LW AND LWU
    // LW loads a 32-bit value from memory and sign-extends this to 
    // 64 bits before storing it in register rd
    // The LWU instruction, on the other hand, zero-extends 
    // the 32-bit value from memory
    RISCVLoad.insert(std::make_tuple(32, 32, 32); 

    RISCVMul.insert(std::make_tuple(64, 64, 64));
    
    RISCVDiv.insert(std::make_tuple(64, 64, 64));
    RISCVDiv.insert(std::make_tuple(32, 32, 32));
    
    RISCVRem.insert(std::make_tuple(64, 64, 64));
    RISCVRem.insert(std::make_tuple(32, 32, 32));
       

    RISCVStore.insert(std::make_tuple(8, 8, 8));
    RISCVStore.insert(std::make_tuple(16, 16, 16));
    RISCVStore.insert(std::make_tuple(32, 32, 32));
    RISCVStore.insert(std::make_tuple(64, 64, 64));
 
    // ARM  
    ARMAdd.insert(make_tuple(32, 32, 32));
    ARMAdd.insert(make_tuple(64, 64, 64));
    ARMAdd.insert(make_tuple(64, 32, 64)) // zero extends 32 bit internally
    X86Add.insert(make_tuple(64, 64, 64));
    X86Add.insert(make_tuple(32, 32, 32));
    X86Add.insert(make_tuple(16, 16, 16));
    X86Add.insert(make_tuple(8, 8, 8));
   
    RISCVOpsMap[ISD::ADD].push_back(RISCVAdd);
    
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

    FillTypeSet Eq, Ge, Geu, Gt, Gtu, Le, Leu, Lt, Ltu, Mod, Modu;
    
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
  
  if(fill != ANYTHING && fill != ZEROS && fill != ZEROS){
    return 0;
  }
  return 1;


}
  
inline bool WideningIntegerArithmetic::hasTypeT(IntegerFillType fill){
  return hasTypeGarbage(fill);
}

inline bool WideningIntegerArithmetic::hasTypeS(IntegerFillType fill){
  if(fill != ANYTHING && fill != ZEROS && fill != ZEROS){
    return 0;
  }
  return 1;

}








