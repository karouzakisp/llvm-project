///===- WideningIntegerArithmetic ------------------------------------------===
//
//



#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/CodeGen/WideningIntegerArithmeticInfo.h"

using namespace llvm;

enum fillType{
  ZEROS = 0,
  SIGN_BIT = 1,
  GARBAGE = 2
};

class WideningIntegerArithmetic {
  SelectionDAG &DAG;
  const TargetLowering &TLI; 

  public:
    WideningIntegerArithmetic(SelectionDAG &D):
      DAG(D), TLI(D.getTargetLoweringInfo()) {
    } 
  
    void solve();

  private:
    enum fillType FillType;
    
    using SolutionMap = DenseMap<int, WideningIntegerSolutionInfo *>; 
    SolutionMap Solutions;
    
    using AvailableSolutionsMap = DenseMap<int, 
                    SmallVector<WideningIntegerSolutionInfo *>>;  
    AvailableSolutionsMap AvailableSolutions;
 
    SDValue visit_widening(SDNode *Node);
    SDValue visitInstruction(SDNode *Node);
    SDValue visitANY_EXT_OR_TRUNC(SDNode *Node);
    SDValue visitXOR(SDNode *Node);
  
    void setFillType(EVT SrcVt, EVT DstVT);  
    bool isInteger(SDNode *Node); 
};

SDValue WideningIntegerArithmetic::visitInstruction(SDNode *Node){
  assert(Node->getNumOperands() == 0 && "Not zero children");
  switch(Node->getOpcode()){
    default: break;
    case ISD::SIGN_EXTEND:
    case ISD::ZERO_EXTEND:    
    case ISD::TRUNCATE:       return visitANY_EXT_OR_TRUNC(Node );
    case ISD::XOR:            return visitXOR(Node ); 
  }
  return SDValue(Node, 0);
}


SDValue WideningIntegerArithmetic::visit_widening(SDNode *Node){
  int NumOperands = Node->getNumOperands();
  if(NumOperands == 0 ){
    return visitInstruction(Node);
  }
  else {
    for (const SDValue &value : Node->op_values() ){
      SDNode *OperandNode = value.getNode();
      SDValue N = visit_widening(OperandNode);
      return N;
    }
  }
  return SDValue(Node, 0);
}

SDValue WideningIntegerArithmetic::visitXOR(SDNode *Node ){
  SDValue N0 = Node->getOperand(0);
  SDValue N1 = Node->getOperand(1);
  auto LeftSolution = Solutions.find((N0.getNode())->getNodeId());
  auto RightSolution = Solutions.find((N1.getNode())->getNodeId());
  assert(LeftSolution != Solutions.end() && "Xor Dag Child not solved yet"); 
  assert(RightSolution != Solutions.end() && "Xor Dag Child not solved yet");
  int left_cost = LeftSolution->second->getExtensionCost();
  int right_cost = RightSolution->second->getExtensionCost();
  
  auto leftAvailableSolutions = 
      AvailableSolutions.find((N0.getNode())->getNodeId());
  auto rightAvailableSolutions = 
      AvailableSolutions.find((N1.getNode())->getNodeId());
  // add left and right to Node available Solutions 
  
  return SDValue(Node, 0);
}

SDValue WideningIntegerArithmetic::visitANY_EXT_OR_TRUNC(SDNode *Node){
  SDValue N0 = Node->getOperand(0);
  SDValue N1 = Node->getOperand(1);
  
  // TODO check this
  setFillType(N0->getValueType(0), N1->getValueType(0));
  
  // just drop the extension or truncation 
  //we will measure later if this produces incorrect results
  return N0; 
  
}

void WideningIntegerArithmetic::setFillType(EVT SrcVT, EVT DstVT){
  if(TLI.isSExtCheaperThanZExt(SrcVT, DstVT)){
    FillType = SIGN_BIT;
  }else{
    FillType = ZEROS;
  }
}

void WideningIntegerArithmetic::solve(){
  
  
  for (SDNode &Node : DAG.allnodes()){
      auto Solution = Solutions.find(Node.getNodeId());  
      if(Solution == Solutions.end() && 
                     isInteger(&Node) ){ // If not solved visit it
        visit_widening(&Node);  
      }
    
  }
} 

bool isInteger(SDNode *Node){
  return Node->getValueType(0).isInteger();
}









