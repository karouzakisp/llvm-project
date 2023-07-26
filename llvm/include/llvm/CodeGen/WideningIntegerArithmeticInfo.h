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


enum WIADerivedId{
  WIA_BINOP_ID = 0,
  WIA_FILL_ID,
  WIA_WIDEN_ZERO_ID,
  WIA_WIDEN_SIGN_ID,
  WIA_WIDEN_GARBAGE_ID,
  WIA_NARROW_ID,
  WIA_DROP_EXT_ID,
  WIA_DROP_LOCOPY_ID,
  WIA_DROP_LOIGNORE_ID,
  WIA_EXTLO_ID,
  WIA_SUBSUME_FILL_ID,
  WIA_SUBSUME_INDEX_ID,
  WIA_NATURAL_ID
};

// This class is used to specify all the arguments that a solution
// of WideningIntegerArithmetic class requires.
class WideningIntegerSolutionInfo{
public:

  WideningIntegerSolutionInfo(){}
  WideningIntegerSolutionInfo(WIADerivedId id): Id(id) {} 
  WideningIntegerSolutionInfo(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, WIADerivedId id): 
  Width(Width_), FillType(FillType_), UpdatedWidth(UpdatedWidth_),
  Cost(Cost_), Opcode(Opcode_), Id(id) {} 
  
  virtual inline bool operator==(const WideningIntegerSolutionInfo &a) = 0; 


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
  
  void getValueID() const {
    return Id;
  }

protected:
  
  WIADerivedId Id;
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
  ~WIA_BINOP() {}
  WIA_BINOP() {}
  WIA_BINOP(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_BINOP(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, WIADerivedId id = WIA_BINOP_ID): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_, id) {}
  
  static inline bool classof(WIA_BINOP const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getValueID()){
      case WIA_BINOP_ID: return true;
      default: return false;
    } 
  }
  
  
  virtual inline bool operator==(const WideningIntegerSolutionInfo& a) override
  {
    return (isa<WIA_BINOP>(&a)); 
  }
  private:
    WideningIntegerSolutionInfo *S;
};


class WIA_FILL : public WideningIntegerSolutionInfo
{
  public:
  ~WIA_FILL() {};
  WIA_FILL() {;};
  WIA_FILL(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_FILL(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, WIADerivedId id = WIA_FILL_ID): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_, id) {}: 

  static inline bool classof(WIA_FILL const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getValueID()){
      case WIA_FILL_ID: return true;
      default: return false;
    } 
  }
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_FILL>(&a));
  }
  private:
    WideningIntegerSolutionInfo *S;
};

class WIA_WIDEN_ZERO : public WideningIntegerSolutionInfo
{
  public:
  WIA_WIDEN_ZERO() {}
  ~WIA_WIDEN_ZERO() {}
  WIA_WIDEN_ZERO(WideningIntegerSolutionInfo *S_): S(S_) {;};
  WIA_WIDEN_ZERO(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, WIADerivedId id = WIA_WIDEN_ZERO_ID ): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_, id) {}

  static inline bool classof(WIA_WIDEN_ZERO const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getValueID()){
      case WIA_WIDEN_ZERO_ID: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_WIDEN_ZERO>(&a));
  }
  private:
    WideningIntegerSolutionInfo *S;
};

class WIA_WIDEN_SIGN : public WideningIntegerSolutionInfo
{
  public:
  WIA_WIDEN_SIGN() {}
  ~WIA_WIDEN_SIGN() {}
  WIA_WIDEN_SIGN(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_WIDEN_SIGN(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, WIADerivedId id = WIA_WIDEN_SIGN_ID): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_, id) {}
  
  static inline bool classof(WIA_WIDEN_SIGN const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getValueID()){
      case WIA_WIDEN_SIGN_ID: return true;
      default: return false;
    } 
  }

  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_WIDEN_SIGN>(&a));
  }
  private:
    WideningIntegerSolutionInfo *S;
};

class WIA_WIDEN_GARBAGE : public WideningIntegerSolutionInfo
{
  public:
  WIA_WIDEN_GARBAGE() {}
  ~WIA_WIDEN_GARBAGE() {}
  WIA_WIDEN_GARBAGE(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_WIDEN_GARBAGE(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, WIADerivedId id = WIA_WIDEN_GARBAGE_ID): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_, id) {}

  static inline bool classof(WIA_WIDEN_GARBAGE const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getValueID()){
      case WIA_WIDEN_GARBAGE_ID: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_WIDEN_GARBAGE>(&a));
  }
  private:
    WideningIntegerSolutionInfo *S;
};

class WIA_NARROW : public WideningIntegerSolutionInfo
{
  public:
  WIA_NARROW() {}
  ~WIA_NARROW() {}
  WIA_NARROW(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_NARROW(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, WIADerivedId id = WIA_NARROW_ID): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_, id) {}

  static inline bool classof(WIA_NARROW const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getValueID()){
      case WIA_NARROW_ID: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_NARROW>(&a));
  }
  private:
    WideningIntegerSolutionInfo *S;
};


class WIA_DROP_EXT : public WideningIntegerSolutionInfo
{
  public:
  WIA_DROP_EXT() {}
  ~WIA_DROP_EXT() {}
  WIA_DROP_EXT(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_DROP_EXT(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, WIADerivedId id = WIA_DROP_EXT_ID): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_, id) {}

  static inline bool classof(WIA_DROP_EXT const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getValueID()){
      case WIA_DROP_EXT_ID: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_DROP_EXT>(&a));
  }
  private:
    WideningIntegerSolutionInfo *S;
};


class WIA_DROP_LOCOPY : public WideningIntegerSolutionInfo
{
  public:
  WIA_DROP_LOCOPY() {}
  ~WIA_DROP_LOCOPY() {}
  WIA_DROP_LOCOPY(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_DROP_LOCOPY(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, WIADerivedId id = WIA_DROP_LOCOPY_ID): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_, id) {}

  static inline bool classof(WIA_DROP_LOCOPY const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getValueID()){
      case WIA_DROP_LOCOPY: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_DROP_LOCOPY>(&a));
  }
  private:
    WideningIntegerSolutionInfo *S;
};


class WIA_DROP_LOIGNORE : public WideningIntegerSolutionInfo
{
  public:
  WIA_DROP_LOIGNORE() {}
  ~WIA_DROP_LOIGNORE() {}
  WIA_DROP_LOIGNORE(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_DROP_LOIGNORE(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, WIADerivedId id = WIA_DROP_LOIGNORE_ID): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_, id) {}

  static inline bool classof(WIA_DROP_LOIGNORE const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getValueID()){
      case WIA_DROP_LOIGNORE_ID: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_DROP_LOIGNORE>(&a));
  }
  private:
    WideningIntegerSolutionInfo *S;
};


class WIA_EXTLO : public WideningIntegerSolutionInfo
{
  public:
  WIA_EXTLO() {}
  ~WIA_EXTLO() {}
  WIA_EXTLO(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_EXTLO(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, WIADerived id = WIA_EXTLO_ID): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_, id) {}

  static inline bool classof(WIA_EXTLO const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getValueID()){
      case WIA_EXTLO_ID: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_EXTLO>(&a));
  }
  private:
    WideningIntegerSolutionInfo *S;
};


class WIA_SUBSUME_FILL : public WideningIntegerSolutionInfo
{
  public:
  WIA_SUBSUME_FILL() {}
  ~WIA_SUBSUME_FILL() {}
  WIA_SUBSUME_FILL(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_SUBSUME_FILL(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, WIADerivedId id = WIA_SUBSUME_FILL_ID): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_, id) {}

  static inline bool classof(WIA_SUBSUME_FILL const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getValueID()){
      case WIA_SUBSUME_FILL_ID: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return  Width == a.getWidth() && 
            FillType == a.getFillType() &&
            Cost == a.getCost() &&
            UpdatedOpcode == a.getUpdatedOpcode() &&
            Opcode == a.getOpcode();
  }
  private:
    WideningIntegerSolutionInfo *S;
};


class WIA_SUBSUME_INDEX : public WideningIntegerSolutionInfo
{
  public:
  WIA_SUBSUME_INDEX() {}
  ~WIA_SUBSUME_INDEX() {}
  WIA_SUBSUME_INDEX(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_SUBSUME_INDEX(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, WIADerivedId id = WIA_SUBSUME_INDEX_ID): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_, id) {}

  static inline bool classof(WIA_SUBSUME_INDEX const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getValueID()){
      case WIA_SUBSUME_INDEX_ID: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return  Width == a.getWidth() && 
            FillType == a.getFillType() &&
            Cost == a.getCost() &&
            UpdatedOpcode == a.getUpdatedOpcode() &&
            Opcode == a.getOpcode();
  }
  private:
    WideningIntegerSolutionInfo *S;
};


class WIA_NATURAL : public WideningIntegerSolutionInfo
{
  public:
  WIA_NATURAL() {}
  ~WIA_NATURAL() {}
  WIA_NATURAL(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_NATURAL(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, WIADerivedId id = WIA_NATURAL_ID): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_, id) {}

  static inline bool classof(WIA_NATURAL const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getValueID()){
      case WIA_NATURAL_ID: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return  Width == a.getWidth() && 
            FillType == a.getFillType() &&
            Cost == a.getCost() &&
            UpdatedOpcode == a.getUpdatedOpcode() &&
            Opcode == a.getOpcode();
  }
  private:
    WideningIntegerSolutionInfo *S;
};






} // namespace llvm;


#endif // LLVM_ANALYSIS_UTILS_WIDENINGINTEGERARITHMETIC_H
