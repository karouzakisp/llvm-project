#ifndef LLVM_CODEGEN_WIDENINGINTEGERARITHMETICINFO_H
#define LLVM_CODEGEN_WIDENINGINTEGERARITHMETICINFO_H


#include "llvm/IR/Instruction.h"
#include "llvm/ADT/SmallPtrSet.h"


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
  
  void setOperands(SmallPtrSetImpl<WideningIntegerSolutionInfo *>  Operands_){
    this.Operands = Operands_;
  }
  SmallPtrSetImpl<WideningIntegerSolutionInfo *>  getOperands(void) const {
    return Operands;
  }
  
  // Returns -1 if "this" is not redundant given solution b
  // Returns 1 if "this" is redudant given solution b
  // Returns 0 if b is redudant given solution "this"
  int isRedudant(const WideningIntegerSolutionInfo &b){
    if(this.Width > b.getWidth() && this.Cost < b.getCost() )
      return -1;
    else if(this.Width < b.getWidth() && this.Cost > b.getCost() )
      return -1;
    else if(this.Width >= b.getWidth() && this.Cost >= b.getCost() )
      return 1;
    else if(this.Width < b.getWidth() )
      return -1
    else if(b.getWidth() >= this.Width() && b.getCost() >= this.Cost )
      return 0;
    else if(b.getWidth() < this.Width() )
      return -1;
    return -1; 
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
  
private:  
  WIADerivedId Id;
  // TODO can a SDNode have more than 4 Operands? 
  SmallPtrSet<WideningIntegerSolutionInfo *, 4> Operands;
}; // WideningIntegerSolutionInfo


class WIA_BINOP : public WideningIntegerSolutionInfo
{
  public:
  ~WIA_BINOP() {}
  WIA_BINOP() {}
  WIA_BINOP(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_BINOP(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_, 
        WIA_BINOP_ID) {}
  
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
  
  
};


class WIA_FILL : public WideningIntegerSolutionInfo
{
  public:
  ~WIA_FILL() {};
  WIA_FILL() {;};
  WIA_FILL(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_FILL(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_,
        WIA_FILL_ID) {}: 

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
};

class WIA_WIDEN_ZERO : public WideningIntegerSolutionInfo
{
  public:
  WIA_WIDEN_ZERO() {}
  ~WIA_WIDEN_ZERO() {}
  WIA_WIDEN_ZERO(WideningIntegerSolutionInfo *S_): S(S_) {;};
  WIA_WIDEN_ZERO(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_
        WIA_WIDEN_ZERO_ID) {}

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
};

class WIA_WIDEN_SIGN : public WideningIntegerSolutionInfo
{
  public:
  WIA_WIDEN_SIGN() {}
  ~WIA_WIDEN_SIGN() {}
  WIA_WIDEN_SIGN(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_WIDEN_SIGN(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_, 
        WIA_WIDEN_SIGN_ID) {}
  
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
};

class WIA_WIDEN_GARBAGE : public WideningIntegerSolutionInfo
{
  public:
  WIA_WIDEN_GARBAGE() {}
  ~WIA_WIDEN_GARBAGE() {}
  WIA_WIDEN_GARBAGE(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_WIDEN_GARBAGE(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_,
        WIA_WIDEN_GARBAGE_ID) {}

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
};

class WIA_NARROW : public WideningIntegerSolutionInfo
{
  public:
  WIA_NARROW() {}
  ~WIA_NARROW() {}
  WIA_NARROW(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_NARROW(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_,
        WIA_NARROW_ID) {}

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
};


class WIA_DROP_EXT : public WideningIntegerSolutionInfo
{
  public:
  WIA_DROP_EXT() {}
  ~WIA_DROP_EXT() {}
  WIA_DROP_EXT(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_DROP_EXT(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_,
        WIA_DROP_EXT_ID) {}

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
};


class WIA_DROP_LOCOPY : public WideningIntegerSolutionInfo
{
  public:
  WIA_DROP_LOCOPY() {}
  ~WIA_DROP_LOCOPY() {}
  WIA_DROP_LOCOPY(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_DROP_LOCOPY(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_,
        WIA_DROP_LOCOPY_ID) {}

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
};


class WIA_DROP_LOIGNORE : public WideningIntegerSolutionInfo
{
  public:
  WIA_DROP_LOIGNORE() {}
  ~WIA_DROP_LOIGNORE() {}
  WIA_DROP_LOIGNORE(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_DROP_LOIGNORE(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_,
        WIA_DROP_LOIGNORE_ID) {}

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
};


class WIA_EXTLO : public WideningIntegerSolutionInfo
{
  public:
  WIA_EXTLO() {}
  ~WIA_EXTLO() {}
  WIA_EXTLO(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_EXTLO(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_,
        WIA_EXTLO_ID) {}

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
};


class WIA_SUBSUME_FILL : public WideningIntegerSolutionInfo
{
  public:
  WIA_SUBSUME_FILL() {}
  ~WIA_SUBSUME_FILL() {}
  WIA_SUBSUME_FILL(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_SUBSUME_FILL(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, Width_, UpdatedWidth_, Cost_,
        WIA_SUBSUME_FILL_ID) {}

  static inline bool classof(WIA_SUBSUME_FILL const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getValueID()){
      case WIA_SUBSUME_FILL_ID: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_SUBSUME_FILL>(&a));
  }
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
    return (isa<WIA_SUBSUME_INDEX>(&a));
  }
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
    return (isa<WIA_SUBSUME_INDEX>(&a));
  }
};






} // namespace llvm;


#endif // LLVM_ANALYSIS_UTILS_WIDENINGINTEGERARITHMETIC_H
