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







