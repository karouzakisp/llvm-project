///===- WideningIntegerArithmetic ------------------------------------------===
//
//



#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/CodeGen/WideningIntegerArithmeticInfo.h"
#include "llvm/ADT/SmallSet.h"

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
    OK_MOD,
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
  }
  public:
    WideningIntegerArithmetic(SelectionDAG &D):
      DAG(D), TLI(D.getTargetLoweringInfo()) {
    } 
  
    void solve();
    
  private:
    enum IntegerFillType ExtensionChoice;   
 
    using isSolvedMap = DenseMap<int, bool>;
    // checks whether a SDNode in the DAG is visited and solved` 
    isSolvedMap solvedNodes;
    
    using SolutionSet = SmallVector<WideningIntegerSolutionInfo *>;
    
    using AvailableSolutionsMap = DenseMap<int, SolutionSet>;
    // Holds all the available solutions 
    AvailableSolutionsMap AvailableSolutions;

    using FillTypeSet = SmallSet<IntegerFillType, 4>;
  
    using BinOpWidth = tuple<unsigned char, unsigned char, unsigned char>;
    using WidthsSet = SmallVector<BinOpWidth>
    using OperatorWidthsMap = DenseMap<OperatorKind, WidthsSet>;
    using TargetWidthsMap = DenseMap<String, OperatorWidthsMap>
    TargetWidthsMap TargetWidths;
    void initTargetWidthTables();
 
    SDValue visit_widening(SDNode *Node);
    SDValue visitInstruction(SDNode *Node);
    SDValue visitANY_EXT_OR_TRUNC(SDNode *Node);
    SDValue visitXOR(SDNode *Node);
    SDValue visitADD(SDNode *Node);
    SDValue visitADDO(SDNode *Node);
  
    void setFillType(EVT SrcVt, EVT DstVT);  
    bool isInteger(SDNode *Node);
    bool isSolved(SDNode *Node);
    void combineSolutions(int NodeId, unsigned Opcode, SolutionSet leftSols,
SolutionSet rightSols, FillTypeSet operandFillTypes);
    bool canCombineSolutions(WideningIntegerSolutionInfo *leftSol, 
                             WideningIntegerSolutionInfo *rightSol, 
                             FillTypeSet OperandFillTypes);

    inline bool hasTypeGarbage(IntegerFillType fill);
    inline bool hasTypeT(IntegerFillType fill);
    inline bool hasTypeS(IntegerFillType fill);
    
    WideningIntegerSolutionInfo *findAllSolutions(
                      WideningIntegerSolutionInfo *Sol);    
    WideningIntegerSolutionInfo *visitBINOP(
                          WideningIntegerSolutionInfo *LeftSol,
                          WideningIntegerSolutionInfo *RightSol);
    WideningIntegerSolutionInfo *visitFILL(
                          WideningIntegerSolutionInfo *Sol);
    WideningIntegerSolutionInfo *visitWIDEN(
                          WideningIntegerSolutionInfo *Sol);
    WideningIntegerSolutionInfo *visitWIDEN_GARBAGE(
                          WideningIntegerSolutionInfo *Sol);
    WideningIntegerSolutionInfo *visitNARROW(
                          WideningIntegerSolutionInfo *Sol);
    WideningIntegerSolutionInfo *visitDROP_EXT(
                          WideningIntegerSolutionInfo *Sol);
    WideningIntegerSolutionInfo *visitDROP_LO_COPY(
                          WideningIntegerSolutionInfo *Sol);
    WideningIntegerSolutionInfo *visitDROP_LO_IGNORE(
                          WideningIntegerSolutionInfo *Sol);
    WideningIntegerSolutionInfo *visitEXTLO(
                          WideningIntegerSolutionInfo *LeftSol,
                          WideningIntegerSolutionInfo *RightSol);
    WideningIntegerSolutionInfo *visitSUBSUME_FILL(
                          WideningIntegerSolutionInfo *Sol);
    WideningIntegerSolutionInfo *visitSUBSUME_INDEX(
                          WideningIntegerSolutionInfo *Sol);
    WideningIntegerSolutionInfo *visitNATURAL(
                          WideningIntegerSolutionInfo *Sol);
     
};

SDValue WideningIntegerArithmetic::visitInstruction(SDNode *Node){
  assert(Node->getNumOperands() == 0 && "Not zero children");
  switch(Node->getOpcode()){
    default: break;
    case ISD::SIGN_EXTEND:
    case ISD::ZERO_EXTEND:    
    case ISD::TRUNCATE:       return visitANY_EXT_OR_TRUNC(Node);
    case ISD::XOR:            return visitXOR(Node); 
    case ISD::ADD:            return visitADD(Node);
    case ISD::UADDO:          return visitADDO(Node);
  }
  return SDValue(Node, 0);
}


SDValue WideningIntegerArithmetic::visit_widening(SDNode *Node){
  int NumOperands = Node->getNumOperands();
  if(NumOperands == 0 ){ // TODO how to check leaf is probably correct need to verify 
                         // TODO check if not solved 
                        // 2 paths can lead to the same node
                        // is already checked
    return visitInstruction(Node);
  }
  else {
    for (const SDValue &value : Node->op_values() ){
        
      SDNode *OperandNode = value.getNode(); // #TODO check value size is 1?
      SDValue N = visit_widening(OperandNode);
      return N;
    }
  }
  return SDValue(Node, 0);
}


bool WideningIntegerArithmetic::canCombineSolutions(
                                WideningIntegerSolutionInfo *leftSolution, 
                                WideningIntegerSolutionInfo *rightSolution, 
                                FillTypeSet operandFillTypes){
  if(leftSolution->getFillType() != rightSolution->getFillType()){ 
    return false;
  }else if(!operandFillTypes.contains(leftSolution->getFillType())){
    return false;  
  }

  return true; 
  

}

void WideningIntegerArithmetic::combineSolutions(int NodeId, unsigned Opcode, SolutionSet LeftSols, 
        SolutionSet RightSols, FillTypeSet OperandFillTypes){
  for (auto leftSolution : LeftSols){
    for(auto rightSolution : RightSols){
      if(canCombineSolutions(leftSolution, rightSolution, OperandFillTypes)){
        unsigned char Cost = leftSolution->getCost() + rightSolution->getCost();
        unsigned char LeftWidth = leftSolution->getUpdatedWidth();
        unsigned char RightWidth = rightSolution->getUpdatedWidth();
        unsigned char NewWidth = LeftWidth > RightWidth ? LeftWidth : 
                                                          RightWidth;
        auto Sol = new WIA_BINOP(Opcode, 
          leftSolution->getFillType(), NewWidth, NewWidth, Cost);

        AvailableSolutions[NodeId].push_back(Sol); 
      }    
    }
  }
}

SDValue WideningIntegerArithmetic::visitXOR(SDNode *Node ){
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
  SDValue N1 = Node->getOperand(1);
  
  // TODO check this do we know that the result is in index 0?
  
  setFillType(N0->getValueType(0), N1->getValueType(0));
  
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



WideningIntegerSolutionInfo* WideningIntegerArithmetic::findAllSolutions(
                      WideningIntegerSolutionInfo *Sol){
  return NULL;
} 
    
// Return all the solution based on binop rules op1 x op2 -> W
WideningIntegerSolutionInfo* WideningIntegerArithmetic::visitBINOP(
                          WideningIntegerSolutionInfo *BinOperator,
                          WideningIntegerSolutionInfo *LeftSol,
                          WideningIntegerSolutionInfo *RightSol){
  unsigned opc = BinOperator->getOpcode();
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
    
  return Binop; 

}

WideningIntegerSolutionInfo* WideningIntegerArithmetic::visitFILL(
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
  return Fill; 
}
    
WideningIntegerSolutionInfo* WideningIntegerArithmetic::visitWIDEN(
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
  return Widen; 
}
    

WideningIntegerSolutionInfo* WideningIntegerArithmetic::visitWIDEN_GARBAGE(
      WideningIntegerSolutionInfo *Sol){
 
  if(!hasTypeGarbage(Sol->getFillType());

  unsigned ExtensionOpc = ISD::ANY_EXTEND  // Results to a garbage widened
  WideningIntegerSolutionInfo *GarbageWiden = new WIA_WIDEN(
    ExtensionOpc, ExtensionChoice, Sol->getFillTypeWidth(), 
    Sol->getWidth(),
    64/* TODO get TARGET WIDTH */, Sol->getCost() + 1 );
  GarbageWiden->addOperand(Sol);
  return GarbageWiden; 
}
    
WideningIntegerSolutionInfo* WideningIntegerArithmetic::visitNARROW(
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
  return Trunc; 
}
    
WideningIntegerSolutionInfo* WideningIntegerArithmetic::visitDROP_EXT(
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

}


  
WideningIntegerSolutionInfo* WideningIntegerArithmetic::visitDROP_LO_COPY(
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
  return Expr;
}
// TODO What's the difference with DROP_LOCOPY and DROP_LOIGNORE
WideningIntegerSolutionInfo* WideningIntegerArithmetic::visitDROP_LO_IGNORE(
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
  return Expr;
}

WideningIntegerSolutionInfo* WideningIntegerArithmetic::visitEXTLO(
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
  
  return Expr; 


}
    
WideningIntegerSolutionInfo* WideningIntegerArithmetic::visitSUBSUME_FILL(
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
    
WideningIntegerSolutionInfo* WideningIntegerArithmetic::visitNATURAL(
                          WideningIntegerSolutionInfo *Sol){
 
  if(!hasTypeGarbage(Sol->getFillType())
    return NULL; 
  unsigned ExtensionOpc = ExtensionChoice == SIGN ? ISD::SIGN_EXTEND :
                                                    ISD::ZERO_EXTEND;
  
  WideningIntegerSolutionInfo *Expr = new WIA_NATURAL(
    ExtensionOpc, ExtensionChoice , Sol->getFillTypeWidth(), 
    Sol->getWidth(), Sol->getUpdatedWidth(), Sol->getCost());
  return Expr;

}



void WideningIntegerArithmetic::initTargetWidthTables(){
    
  
  
    OperatorWidthsMap RISCVOpsMap, ARMOpsMap, X86OpsMap;
    WidthsSet RISCVAdd, ARMAdd, X86Add;
    
    RISCVAdd.insert(make_tuple(32, 32,32));
    RISCVAdd.insert(make_tuple(64, 64, 64));
    
    ARMAdd.insert(make_tuple(32, 32, 32));
    ARMAdd.insert(make_tuple(64, 64, 64));
    ARMAdd.insert(make_tuple(64, 32, 64)) // zero extends 32 bit internally
    X86Add.insert(make_tuple(64, 64, 64));
    X86Add.insert(make_tuple(32, 32, 32));
    X86Add.insert(make_tuple(16, 16, 16));
    X86Add.insert(make_tuple(8, 8, 8));
    
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








