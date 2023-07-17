#ifndef LLVM_CODEGEN_WIDENINGINTEGERARITHMETICINFO_H
#define LLVM_CODEGEN_WIDENINGINTEGERARITHMETICINFO_H


#include "llvm/IR/Instruction.h"

namespace llvm {


class Instruction;

enum IntegerFillType{
  sign_bits = 0,
  zeroes,
  anything 
};


// This class is used to specify all the arguments that a solution
// of WideningIntegerArithmetic class requires.
class WideningIntegerSolutionInfo{
public:
 
  ~WideningIntegerSolutionInfo();
  WideningIntegerSolutionInfo() = default; 
  WideningIntegerSolutionInfo(Instruction *Instr, IntegerFillType fillType_,
                      unsigned char width_)
    : IntegerInstr(Instr), FillType(fillType_), Width(width_){} 
  

private:
  

  // We care only about integer arithmetic instructions
  Instruction       *IntegerInstr=nullptr;

  // The updated instruction if we add an extension
  Instruction       *UpdatedInstr=nullptr;

  // The cost of the extension 
  short int         ExtensionCost=0;

  // Fill type for the upper bits of the register
  // can be zero, one or garbage
  IntegerFillType   FillType=anything;

  // The updated width of the instruction's destination operand
  unsigned char     UpdatedWidth=0;

  // The current width of the instruction's destination operand
  unsigned char     Width=0;

public:
  Instruction *getInstruction(void){
    return IntegerInstr;
  }
  void setIntegerInstruction(Instruction *IntegerInstr_){
    IntegerInstr = IntegerInstr_;
  }  

  Instruction *getUpdatedInstruction(void){
    return UpdatedInstr;
  }
  void setUpdatedInstruction(Instruction *UpdatedInstr_){
    UpdatedInstr = UpdatedInstr_;
  }
  
  short int getExtensionCost(void){
    return ExtensionCost;
  }
  void setExtensionCost(short int ExtensionCost_){
    ExtensionCost = ExtensionCost_;
  }
  IntegerFillType getIntegerFillType(void){
    return FillType;
  }
  void setIntegerFillType(IntegerFillType FillType_){
    FillType = FillType_; 
  }

  unsigned char getWidth(void){
    return Width;
  }
  void setWidth(unsigned char Width_){
    Width = Width_;
  }
  
  unsigned char getUpdatedWidth(void){
    return UpdatedWidth;
  }

  void setUpdatedWidth(unsigned char UpdatedWidth_){
    UpdatedWidth = UpdatedWidth_;
  }
  bool operator==(const WideningIntegerSolutionInfo& a){
    return  this->Width == a.Width && 
            this->FillType == a.FillType &&
            this->ExtensionCost == a.ExtensionCost &&
            this->UpdatedInstr == a.UpdatedInstr &&
            this->IntegerInstr == a.IntegerInstr;
  }
  bool operator==(const WideningIntegerSolutionInfo& a){
    return  this->Width == a.Width && 
            this->FillType == a.FillType &&
            this->ExtensionCost == a.ExtensionCost &&
            this->UpdatedInstr == a.UpdatedInstr &&
            this->IntegerInstr == a.IntegerInstr;
  }
  

  

}; // WideningIntegerSolutionInfo




} // namespace llvm;


#endif // LLVM_ANALYSIS_UTILS_WIDENINGINTEGERARITHMETIC_H
