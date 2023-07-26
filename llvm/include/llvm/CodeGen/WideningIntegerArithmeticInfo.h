#ifndef LLVM_CODEGEN_WIDENINGINTEGERARITHMETICINFO_H
#define LLVM_CODEGEN_WIDENINGINTEGERARITHMETICINFO_H


#include "llvm/IR/Instruction.h"

namespace llvm {


class Instruction;

enum IntegerFillType{
  SIGN = 0,
  ZEROS,
  ANYTHING,
};


// This class is used to specify all the arguments that a solution
// of WideningIntegerArithmetic class requires.
class WideningIntegerSolutionInfo{
public:

  WideningIntegerSolutionInfo(){}  
  WideningIntegerSolutionInfo(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
  Width(Width_), FillType(FillType_), UpdatedWidth(UpdatedWidth_),
  Cost(Cost_), Opcode(Opcode_) {} 
  
  virtual bool operator==(const WideningIntegerSolutionInfo &a) = 0; 


  unsigned getOpcode(void) const {
    return Opcode;
  }
  void setOpcode(unsigned Opcode_){
    Opcode = Opcode_;
  }  

  unsigned getUpdatedOpcode(void) const {
    return UpdatedOpcode;
  }
  void setUpdatedOpcode(unsigned UpdatedOpcode_){
    UpdatedOpcode = UpdatedOpcode_;
  }  
  
  short int getCost(void) const {
    return Cost;
  }
  void setCost(short int Cost_){
    Cost = Cost_;
  }
  IntegerFillType getFillType(void) const {
    return FillType;
  }
  void setFillType(IntegerFillType FillType_){
    FillType = FillType_; 
  }

  unsigned char getWidth(void) const {
    return Width;
  }
  void setWidth(unsigned char Width_){
    Width = Width_;
  }
  
  unsigned char getUpdatedWidth(void) const {
    return UpdatedWidth;
  }

  void setUpdatedWidth(unsigned char UpdatedWidth_){
    UpdatedWidth = UpdatedWidth_;
  }
  

protected:
  

  // need reference to other solutions
  // left and right 

  // We care only about integer arithmetic instructions
  unsigned       Opcode;
  
  // Fill type for the upper bits of the expression
  // can be zero, one or garbage
  IntegerFillType   FillType;



  // The current width of the instruction's destination operand
  unsigned char     Width;

  // The updated width of the instruction's destination operand
  unsigned char     UpdatedWidth;
  
  // The cost of the solution
  short int         Cost;

  
  // The updated instruction if we add an extension
  unsigned       UpdatedOpcode;
  

}; // WideningIntegerSolutionInfo


class WIA_BINOP : public WideningIntegerSolutionInfo
{
  public:
  WIA_BINOP() {}
  ~WIA_BINOP() {}
  WIA_BINOP(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_) {}

  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return  Width == a.getWidth() && 
            FillType == a.getFillType() &&
            Cost == a.getCost() &&
            UpdatedOpcode == a.getUpdatedOpcode() &&
            Opcode == a.getOpcode();
  }
};


class WIA_FILL : public WideningIntegerSolutionInfo
{
  public:
  WIA_FILL() {}
  ~WIA_FILL() {}
  WIA_FILL(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_) {}

  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return  Width == a.getWidth() && 
            FillType == a.getFillType() &&
            Cost == a.getCost() &&
            UpdatedOpcode == a.getUpdatedOpcode() &&
            Opcode == a.getOpcode();
  }
};

class WIA_WIDEN_ZERO : public WideningIntegerSolutionInfo
{
  public:
  WIA_WIDEN_ZERO() {}
  ~WIA_WIDEN_ZERO() {}
  WIA_WIDEN_ZERO(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_) {}

  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return  Width == a.getWidth() && 
            FillType == a.getFillType() &&
            Cost == a.getCost() &&
            UpdatedOpcode == a.getUpdatedOpcode() &&
            Opcode == a.getOpcode();
  }
};

class WIA_WIDEN_SIGN : public WideningIntegerSolutionInfo
{
  public:
  WIA_WIDEN_SIGN() {}
  ~WIA_WIDEN_SIGN() {}
  WIA_WIDEN_SIGN(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_) {}

  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return  Width == a.getWidth() && 
            FillType == a.getFillType() &&
            Cost == a.getCost() &&
            UpdatedOpcode == a.getUpdatedOpcode() &&
            Opcode == a.getOpcode();
  }
};


class WIA_NARROW : public WideningIntegerSolutionInfo
{
  public:
  WIA_NARROW() {}
  ~WIA_NARROW() {}
  WIA_NARROW(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_) {}

  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return  Width == a.getWidth() && 
            FillType == a.getFillType() &&
            Cost == a.getCost() &&
            UpdatedOpcode == a.getUpdatedOpcode() &&
            Opcode == a.getOpcode();
  }
};


class WIA_DROP_EXT : public WideningIntegerSolutionInfo
{
  public:
  WIA_DROP_EXT() {}
  ~WIA_DROP_EXT() {}
  WIA_DROP_EXT(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_) {}

  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return  Width == a.getWidth() && 
            FillType == a.getFillType() &&
            Cost == a.getCost() &&
            UpdatedOpcode == a.getUpdatedOpcode() &&
            Opcode == a.getOpcode();
  }
};


class WIA_DROP_LOCOPY : public WideningIntegerSolutionInfo
{
  public:
  WIA_DROP_LOCOPY() {}
  ~WIA_DROP_LOCOPY() {}
  WIA_DROP_LOCOPY(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_) {}

  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return  Width == a.getWidth() && 
            FillType == a.getFillType() &&
            Cost == a.getCost() &&
            UpdatedOpcode == a.getUpdatedOpcode() &&
            Opcode == a.getOpcode();
  }
};


class WIA_DROP_LOIGNORE : public WideningIntegerSolutionInfo
{
  public:
  WIA_DROP_LOIGNORE() {}
  ~WIA_DROP_LOIGNORE() {}
  WIA_DROP_LOIGNORE(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_) {}

  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return  Width == a.getWidth() && 
            FillType == a.getFillType() &&
            Cost == a.getCost() &&
            UpdatedOpcode == a.getUpdatedOpcode() &&
            Opcode == a.getOpcode();
  }
};


class WIA_EXTLO : public WideningIntegerSolutionInfo
{
  public:
  WIA_EXTLO() {}
  ~WIA_EXTLO() {}
  WIA_EXTLO(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_) {}

  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return  Width == a.getWidth() && 
            FillType == a.getFillType() &&
            Cost == a.getCost() &&
            UpdatedOpcode == a.getUpdatedOpcode() &&
            Opcode == a.getOpcode();
  }
};


class WIA_SUBSUME_FILL : public WideningIntegerSolutionInfo
{
  public:
  WIA_SUBSUME_FILL() {}
  ~WIA_SUBSUME_FILL() {}
  WIA_SUBSUME_FILL(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_) {}

  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return  Width == a.getWidth() && 
            FillType == a.getFillType() &&
            Cost == a.getCost() &&
            UpdatedOpcode == a.getUpdatedOpcode() &&
            Opcode == a.getOpcode();
  }
};


class WIA_SUBSUME_INDEX : public WideningIntegerSolutionInfo
{
  public:
  WIA_SUBSUME_INDEX() {}
  ~WIA_SUBSUME_INDEX() {}
  WIA_SUBSUME_INDEX(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_) {}

  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return  Width == a.getWidth() && 
            FillType == a.getFillType() &&
            Cost == a.getCost() &&
            UpdatedOpcode == a.getUpdatedOpcode() &&
            Opcode == a.getOpcode();
  }
};


class WIA_NATURAL : public WideningIntegerSolutionInfo
{
  public:
  WIA_NATURAL() {}
  ~WIA_NATURAL() {}
  WIA_NATURAL(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_) {}

  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return  Width == a.getWidth() && 
            FillType == a.getFillType() &&
            Cost == a.getCost() &&
            UpdatedOpcode == a.getUpdatedOpcode() &&
            Opcode == a.getOpcode();
  }
};






} // namespace llvm;


#endif // LLVM_ANALYSIS_UTILS_WIDENINGINTEGERARITHMETIC_H
