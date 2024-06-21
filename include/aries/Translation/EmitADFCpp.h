#ifndef ARIES_TRANSLATION_EMITADFCPP_H
#define ARIES_TRANSLATION_EMITADFCPP_H

#include "mlir/IR/BuiltinOps.h"

namespace mlir {
namespace aries {

LogicalResult emitADFCpp(ModuleOp module, llvm::raw_ostream &os);
void registerEmitADFCppTranslation();

//===----------------------------------------------------------------------===//
// Base Classes of emitters
//===----------------------------------------------------------------------===//

/// This class maintains the mutable state that cross-cuts and is shared by the
/// various emitters.
class ADFEmitterState {
public:
  explicit ADFEmitterState(raw_ostream &os) : os(os) {}

  // The stream to emit to.
  raw_ostream &os;

  bool encounteredError = false;
  unsigned currentIndent = 0;

  // This table contains all declared values.
  DenseMap<Value, SmallString<8>> nameTable;
  std::map<std::string, int> nameConflictCnt;

private:
  ADFEmitterState(const ADFEmitterState &) = delete;
  void operator=(const ADFEmitterState &) = delete;
};

/// This is the base class for all of the ADFCpp emitters.
class ADFEmitterBase {
public:
  explicit ADFEmitterBase(ADFEmitterState &state): state(state), os(state.os) {}

  raw_ostream &indent() { return os.indent(state.currentIndent); }

  void addIndent() { state.currentIndent += 2; }
  void reduceIndent() { state.currentIndent -= 2; }

  // All of the mutable state we are maintaining.
  ADFEmitterState &state;

  // The stream to emit to.
  raw_ostream &os;

private:
  ADFEmitterBase(const ADFEmitterBase &) = delete;
  void operator=(const ADFEmitterBase &) = delete;
};



} // namespace aries
} // namespace mlir

#endif // ARIES_TRANSLATION_EMITADFCPP_H