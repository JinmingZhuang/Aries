#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/Dialect/Affine/Analysis/LoopAnalysis.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "llvm/Support/Debug.h"

#include "mlir/IR/Builders.h"

using namespace mlir;
using namespace mlir::memref;
using namespace aries;
using namespace func;


namespace {

struct AriesMemHoist : public AriesMemHoistBase<AriesMemHoist> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";
  
    if (!MemHoist(mod,topFuncName))
      signalPassFailure();
  }

private:
  bool MemHoist (ModuleOp mod,StringRef topFuncName) {
    FuncOp topFunc;
    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }

    if(!subviewHoist(mod, topFunc)){
      return false;
    }

    funcUpdate(mod, topFunc);

    return true;
  }

  //eliminate the unused arguments in callee and caller functions
  void funcUpdate(ModuleOp mod, FuncOp topFunc){
    auto builder = OpBuilder(mod);

    
    topFunc.walk([&](CallOp caller){
      auto calleeFuncOp = mod.lookupSymbol<FuncOp>(caller.getCallee());

      SmallVector<Type, 8> newinTypes;
      auto outTypes = calleeFuncOp.getResultTypes();
      SmallVector<unsigned, 8> usedArgIndex;
      SmallVector<unsigned, 8> unusedArgIndex;

      //Record the unused and used arguments
      for (auto arg : calleeFuncOp.getArguments()) {
        if(!arg.use_empty()){
          newinTypes.push_back(arg.getType());
          usedArgIndex.push_back(arg.getArgNumber());
        }else{
          unusedArgIndex.push_back(arg.getArgNumber()); 
        }
      }
      if (unusedArgIndex.empty())
        return WalkResult::advance();

      // Update the callee function type.
      calleeFuncOp.setType(builder.getFunctionType(newinTypes, outTypes));

      //Erase the unsed block arguments in the callee
      BitVector bv(calleeFuncOp.getArguments().size());
      for (auto index:unusedArgIndex){
        bv.set(index);
      }
      auto& entryBlock = calleeFuncOp.front();
      entryBlock.eraseArguments(bv);

      SmallVector<Value, 8> newOperands;
      for (unsigned i = 0; i < usedArgIndex.size(); ++i) {
        newOperands.push_back(caller.getOperand(usedArgIndex[i]));
      }
      builder.setInsertionPoint(caller);
      auto newCallOp = builder.create<CallOp>(
      caller.getLoc(), caller.getCallee(), caller.getResultTypes(), newOperands);

      // Replace the old CallOp with the new one
      caller.replaceAllUsesWith(newCallOp);
      caller.erase();

      return WalkResult::advance();
    });
  }

  bool subviewHoist(ModuleOp mod, FuncOp topFunc){
    auto builder = OpBuilder(mod);
    
    topFunc.walk([&](CallOp caller){
      auto calleeFuncOp = mod.lookupSymbol<FuncOp>(caller.getCallee());

      //Change the offset of the subview to the caller arguments
      for (auto arg : calleeFuncOp.getArguments()) {
        auto firstUser = *arg.user_begin(); 
        if(dyn_cast<SubViewOp>(firstUser)&&!dyn_cast<MemRefType>(arg.getType())){
          arg.replaceAllUsesWith(caller.getOperand(arg.getArgNumber()));
        }
      }

      auto inTypes = SmallVector<Type, 8>(calleeFuncOp.getArgumentTypes().begin(),
                                           calleeFuncOp.getArgumentTypes().end());

      auto outTypes = calleeFuncOp.getResultTypes();

      for (auto subview : llvm::make_early_inc_range(calleeFuncOp.getOps<SubViewOp>())) {
        //check if the source of the subview is an argument of the callee
        auto arg = subview.getSource().dyn_cast<BlockArgument>();
        if (!arg)
          continue;
        
        //Record the corrsponding arguments in caller since the subview will be
        //replaced by the new callee arguments next
        auto callerMem = caller.getOperand(arg.getArgNumber());

        //Change the memref of callee to the type of corrsponding subview
        //And replace the use of subview by the arguments 
        inTypes[arg.getArgNumber()] = subview.getType();
        arg.setType(subview.getType());
        subview.replaceAllUsesWith(arg);
        
        //Move the subview before the caller function
        subview.getSourceMutable().assign(callerMem);
        subview->remove();
        builder.setInsertionPoint(caller);
        builder.insert(subview);

        //Change the arguments of the caller function
        caller.setOperand(arg.getArgNumber(), subview);
      }

      // Update the callee function type.
      calleeFuncOp.setType(builder.getFunctionType(inTypes, outTypes));

    });

    return true;
  }


};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesMemHoistPass() {
  return std::make_unique<AriesMemHoist>();
}

} // namespace aries
} // namespace mlir