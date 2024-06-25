#ifndef ARIES_TRANSFORM_UTILS_H
#define ARIES_TRANSFORM_UTILS_H


#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"

using namespace mlir;
using namespace affine;
using namespace func;

namespace mlir {
namespace aries {

// Modified UnrollFull: Use function_ref to anotote operations during unroll.
LogicalResult loopUnrollFull(AffineForOp forOp, function_ref<void(unsigned, Operation *, OpBuilder)> annotateFn);

// Get all the affine.for loops within the FuncOp and return them in the band
void getLoopBands(FuncOp f, SmallVector<AffineForOp, 6> &band, bool reverse = false);

/// Built-in Function: Checks whether a loop nest is hyper-rectangular or not.
LogicalResult checkIfHyperRectangular(MutableArrayRef<AffineForOp> input);

// Find topFunc and collect the Callee functions
bool topFind(ModuleOp mod, FuncOp& topFunc, StringRef topFuncName);

// Find the callee function called in the topFunc
bool calleeFind(ModuleOp mod, FuncOp topFunc, FuncOp &calleeFuncOp);

SmallVector<int64_t, 8> getIntArrayAttrValue(Operation *op, StringRef name);

}   // namespace aries
}   // namespace mlir


#endif // ARIES_TRANSFORM_UTILS_H