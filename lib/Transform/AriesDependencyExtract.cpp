#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "llvm/Support/Debug.h"
#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/Affine/Analysis/LoopAnalysis.h"

using namespace mlir;
using namespace aries;
using namespace mlir::memref;

namespace {

struct AriesDependencyExtract : public AriesDependencyExtractBase<AriesDependencyExtract> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";
  
    if (!DependencyExtract(mod,topFuncName))
      signalPassFailure();
  }

private:
  // Collect Copyop that access the external mem
  void CopyCollect(FuncOp topFunc, SmallVector<CopyOp, 4>& copyOps){
    topFunc.walk([&](CopyOp copy){
      auto subviewSrc = dyn_cast<SubViewOp>(copy.getSource().getDefiningOp());
      auto subviewDst = dyn_cast<SubViewOp>(copy.getTarget().getDefiningOp());
      SubViewOp subview;
      if(subviewSrc || subviewDst){
        if(subviewSrc)
          subview = subviewSrc;
        else
          subview = subviewDst;
        auto srcMem = subview.getSource();
        for (auto arg : topFunc.getArguments()) {
          if (srcMem == arg)
            copyOps.push_back(copy);
        }
      }
    });
  }

  //Check if the copy write to the external mem
  bool IsWrite(CopyOp copy){
    if(auto dst = dyn_cast<SubViewOp>(copy.getTarget().getDefiningOp()))
      return true;
    else
      return false;
  }

  // This function extracts the loop-carried flow dependencies 
  // There is a dependency if: 1) two accesses are to the same memory
  //                           2) one of the accesses is a write
  //                           3) precedence constraints
  // This is an experimental pass which should be replaced by mathmatical
  // formulation e.g. polyhedral model                      
  bool DependencyExtract (ModuleOp mod, StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    FuncOp topFunc;
    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }

    SmallVector<CopyOp, 4> copyOps;
    CopyCollect(topFunc, copyOps);
    SmallVector<AffineForOp, 6> bands;
    getLoopBands(topFunc, bands);

    for (auto copyWrite : copyOps){
      unsigned opIndex=0;
      if (IsWrite(copyWrite)){
        auto dst = dyn_cast<SubViewOp>(copyWrite.getTarget().getDefiningOp());
        for (auto copyRead : copyOps){
          if (!IsWrite(copyRead)){
            auto src = dyn_cast<SubViewOp>(copyRead.getSource().getDefiningOp());
            if(src == dst){
              auto offset = src.getOffsets();
              unsigned bandIndex = 0;
              for(auto band: bands){
                auto vi = band.getInductionVar();
                if(std::find(offset.begin(), offset.end(), vi) == offset.end()){
                  if(getConstantTripCount(band)>1){
                    if(bandIndex>=1){
                      llvm::outs() << "Currently doesn't support more than 1 loop-carried flow dependencies";
                      return false;
                    }
                    auto bandAttr = builder.getIntegerAttr(builder.getIndexType(), bandIndex++);
                    band->setAttr("flow", bandAttr);
                    auto opAttr = builder.getIntegerAttr(builder.getIndexType(), opIndex++);
                    copyWrite->setAttr("write", opAttr);
                    copyRead->setAttr("read", opAttr);
                  }
                }
              }
            }
          }
        }
      }
    }

    
    for (auto copyWrite : copyOps){
      unsigned opIndex=0;
      if (IsWrite(copyWrite)){
        auto dst = dyn_cast<SubViewOp>(copyWrite.getTarget().getDefiningOp());
        for (auto copyRead : copyOps){
          if (!IsWrite(copyRead)){
            auto src = dyn_cast<SubViewOp>(copyRead.getSource().getDefiningOp());
            if(src == dst){
              auto offset = src.getOffsets();
              unsigned bandIndex=0;
              for(auto band: bands){
                auto vi = band.getInductionVar();
                if(std::find(offset.begin(), offset.end(), vi) == offset.end()){
                  if(getConstantTripCount(band)>1){
                    if(opIndex>0){
                      llvm::outs() << "More than one flow dependencies found\n" ;
                      return false;
                    }
                    if(bandIndex>0){
                      llvm::outs() << "More than one loops leads to flow dependency\n" ;
                      return false;
                    }  
                    band->setAttr("flow", builder.getUnitAttr());
                    copyWrite->setAttr("write", builder.getUnitAttr());
                    copyRead->setAttr("read", builder.getUnitAttr());
                    opIndex++;
                    bandIndex++;
                  }
                }
              }
            }
          }
        }
      }
    }
    

    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesDependencyExtractPass() {
  return std::make_unique<AriesDependencyExtract>();
}

} // namespace aries
} // namespace mlir