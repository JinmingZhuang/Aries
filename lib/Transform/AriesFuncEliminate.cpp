#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/Support/Debug.h"
#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"

using namespace mlir;
using namespace aries;
using namespace mlir::aries::adf;

namespace {

struct AriesFuncEliminate : public AriesFuncEliminateBase<AriesFuncEliminate> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
  
    if (!FuncEliminate(mod))
      signalPassFailure();
  }

private:
  bool compareAttributes(Operation *op0, Operation *op1) {
    for (auto attr : op0->getAttrs()) {
      auto attr2 = op1->getAttr(attr.getName());
      if (!attr2 || attr.getValue() != attr2) {
        return false;
      }
    }
    return true;
  }
  bool generalCompareOp(Operation *op0, Operation *op1) {
    if (op0->getName() != op1->getName())
      return false;
    if (op0->getNumOperands() != op1->getNumOperands())
      return false;
    if (op0->getNumResults() != op1->getNumResults())
      return false;
    for (unsigned i = 0; i < op0->getNumOperands(); ++i) {
      if (op0->getOperand(i).getType() != op1->getOperand(i).getType())
        return false;
    }
    for (unsigned i = 0; i < op0->getNumResults(); ++i) {
      if (op0->getResult(i).getType() != op1->getResult(i).getType())
        return false;
    }
    if (!compareAttributes(op0, op1))
      return false;
    return true;
  }

  void getMem(Operation* op, Value& memref, AffineMap& map, 
              SmallVector<Value, 4>& operands){
    if (auto read = dyn_cast<AffineLoadOp>(op)) {
      memref = read.getMemRef();
      map = read.getAffineMap();
      operands = read.getMapOperands();
    }else if(auto write = dyn_cast<AffineStoreOp>(op)){
      memref = write.getMemRef();
      map = write.getAffineMap();
      operands = write.getMapOperands();
    }
  }

  bool compareLoadStore(Operation *op0, Operation *op1){
    SmallVector<AffineForOp, 6> srcBand;
    SmallVector<AffineForOp, 6> dstBand;
    getSurroundingLoops(*op0, srcBand);
    getSurroundingLoops(*op1, dstBand);
    auto srcSize = srcBand.size();
    auto dstSize = dstBand.size();
    if (srcSize != dstSize)
      return false;
    // Check if all the loops share the same lbs, ubs and steps
    for(unsigned i = 0; i < srcSize; i++){
      auto srcForOp = srcBand[i];
      auto dstForOp = dstBand[i];
      if(srcForOp.getConstantLowerBound()!=dstForOp.getConstantLowerBound() ||
         srcForOp.getConstantUpperBound()!=dstForOp.getConstantUpperBound() ||
         srcForOp.getStepAsInt()!=dstForOp.getStepAsInt())
         return false;
    }
    // Check if the access the memory with the same type and access pattern
    Value srcMemref, dstMemref;
    AffineMap srcMap, dstMap;
    SmallVector<Value, 4> srcOperands, dstOperands;
    getMem(op0, srcMemref, srcMap, srcOperands);
    getMem(op1, dstMemref, dstMap, dstOperands);
    auto srcMemrefType = dyn_cast<MemRefType>(srcMemref.getType());
    auto dstMemrefType = dyn_cast<MemRefType>(dstMemref.getType());
    // 1. Check if memref has same type
    if(srcMemrefType.getRank() != dstMemrefType.getRank() ||
       srcMemrefType.getElementType() != dstMemrefType.getElementType() ||
       srcMemrefType.getShape() != dstMemrefType.getShape())
       return false;
    // 2. Check if Map access is the same
    if(srcMap.getNumResults() != dstMap.getNumResults())
      return false;
    for (unsigned i = 0; i< srcMap.getNumResults(); i++) {
      auto srcExpr = srcMap.getResult(i);
      auto dstExpr = dstMap.getResult(i);
      if(srcExpr != dstExpr)
        return false;
      if(srcMap.getNumDims() !=dstMap.getNumDims())
        return false;
      for (unsigned i = 0; i < srcMap.getNumDims(); ++i) {
        auto srcLoop = getForInductionVarOwner(srcOperands[i]);
        auto dstLoop = getForInductionVarOwner(dstOperands[i]);
        auto srcIt = llvm::find(srcBand, srcLoop);
        auto dstIt = llvm::find(dstBand, dstLoop);
        if(srcIt!=srcBand.end() && dstIt!=dstBand.end()){
          auto srcDepth = srcIt - srcBand.begin();
          auto dstDepth = dstIt - dstBand.begin();
          if(srcDepth != dstDepth)
            return false;
        }
      }
    }
    return true;
  }

  bool compareOperations(Operation *op0, Operation *op1) {
    if(!generalCompareOp(op0, op1))
      return false;
    if(dyn_cast<AffineLoadOp>(op0) || dyn_cast<AffineStoreOp>(op0))
      if(!compareLoadStore(op0, op1))
        return false;
    return true;
  }
  bool compareFunctions(FuncOp func0, FuncOp func1) {
    if (func0.getFunctionType() != func1.getFunctionType())
      return false;
    if (func0.getBody().getBlocks().size() != 
        func1.getBody().getBlocks().size())
      return false;
    

    for (auto blockPair : llvm::zip(func0.getBody().getBlocks(),
                                    func1.getBody().getBlocks())) {
      Block &block0 = std::get<0>(blockPair);
      Block &block1 = std::get<1>(blockPair);

      if (block0.getOperations().size() != block1.getOperations().size())
        return false;

      for (auto opPair : llvm::zip(block0, block1)) {
        Operation& op0 = std::get<0>(opPair);
        Operation& op1 = std::get<1>(opPair);
        if(dyn_cast<arith::ConstantOp>(op0) && dyn_cast<arith::ConstantOp>(op1))
          continue;
        // Compare the operations
        if (!compareOperations(&op0, &op1))
          return false;
      }
    }
    return true;
  }
  void funcGroup(SmallVector<FuncOp, 4> funcs,
        SmallVector<SmallVector<FuncOp, 4>>& groups){
    // Group the funcs
    for(auto func : funcs){
      // Check if the loop can be grouped to any exisiting groups
      bool grouped = false;
      for(auto& group : groups){
        for(auto newFunc : group){
          if(func == newFunc)
            continue;
          llvm::outs() << "Pairing " << func.getName() << ", with " << newFunc.getName() << "\n";
          if(compareFunctions(func, newFunc)){
            llvm::outs() << "Find src func: " << func.getName() << ", dst func " << newFunc.getName() << "\n";
            group.push_back(func);
            grouped = true;
            break;
          }
        }
        if(grouped)
          break;
      }
      // If the loop haven't been added to any group, create a new group
      if(!grouped){
        SmallVector<FuncOp, 4> newGroup;
        newGroup.push_back(func);
        groups.push_back(newGroup);
      }
    }
  }

  // For each func group only keep the first func
  // Need to replace the caller with template mark and erase the old callee
  void funcMerge(OpBuilder builder, FuncOp topPLFunc, 
                 SmallVector<SmallVector<FuncOp, 4>>& groups){
    auto indexType = builder.getIndexType();
    for(auto group : groups){
      auto firstFunc = group[0];
      firstFunc->setAttr("template", builder.getUnitAttr());
      auto firstName = firstFunc.getName();
      // Set the first func as the first instance
      unsigned index = 0;
      topPLFunc.walk([&](CallOp call){
        auto callName = call.getCallee();
        if(callName != firstName)
          return WalkResult::advance();
        auto attr = builder.getIntegerAttr(indexType, index++);
        call->setAttr("template", attr);
        return WalkResult::interrupt();
      });
      for(auto func : group){
        if(func == firstFunc)
          continue;
        auto funcName = func.getName();
        topPLFunc.walk([&](CallOp call){
          auto callName = call.getCallee();
          if(callName != funcName)
            return WalkResult::advance();
          auto attr = builder.getIntegerAttr(indexType, index++);
          call->setAttr("template", attr);
          call.setCallee(firstName);
          return WalkResult::interrupt();
        });
      }
    }
  }

  bool hasCaller(ModuleOp& mod, FuncOp func) {
    auto funcName = func.getName();
    auto flag = false;
    mod.walk([&](CallOp call){
      if(call.getCallee() == funcName){
        flag = true;
        return WalkResult::interrupt();
      }
      return WalkResult::advance();
    });
    return flag;
  }
  // Eliminate unused funcs
  void funcClean(ModuleOp mod){
    for (auto func : llvm::make_early_inc_range(mod.getOps<FuncOp>())) {
      if(func->getAttr("top_func"))
        continue;
      if (!hasCaller(mod, func)) {
        func.erase();
      }
    }
  }

  // This is a work around pass to eliminate functions that share the same
  // functionality. Need to generalize it using formal verification.
  // This pass only carefully checks the affine.load and affine.store ops. 
  // For other ops, only do general check e.g. the name and type.
  bool FuncEliminate (ModuleOp mod) {
    auto builder = OpBuilder(mod);
    // Collect send and receive funcs
    SmallVector<FuncOp, 4> sendFuncs;
    SmallVector<FuncOp, 4> receiveFuncs;
    SmallVector<FuncOp, 4> loadFuncs;
    SmallVector<FuncOp, 4> storeFuncs;
    FuncOp topPLFunc;
    mod.walk([&](FuncOp func){
      if(!func->hasAttr("adf.pl"))
        return WalkResult::advance();
      auto plAttr = func->getAttr("adf.pl");
      if(auto boolAttr = dyn_cast<BoolAttr>(plAttr))
        if(boolAttr.getValue())
          topPLFunc = func;
      if(func->hasAttr("send"))
        sendFuncs.push_back(func);
      else if(func->hasAttr("receive"))
        receiveFuncs.push_back(func);
      else if(func->hasAttr("load"))
        loadFuncs.push_back(func);
      else if(func->hasAttr("store"))
        storeFuncs.push_back(func);
      else
        return WalkResult::advance();
      return WalkResult::advance();
    });
    if(!topPLFunc)
      return true;
    SmallVector<SmallVector<FuncOp, 4>> sendGroups;
    SmallVector<SmallVector<FuncOp, 4>> receiveGroups;
    SmallVector<SmallVector<FuncOp, 4>> loadGroups;
    SmallVector<SmallVector<FuncOp, 4>> storeGroups;
    funcGroup(sendFuncs, sendGroups);
    funcGroup(receiveFuncs, receiveGroups);
    funcGroup(loadFuncs, loadGroups);
    funcGroup(storeFuncs, storeGroups);
    funcMerge(builder, topPLFunc, sendGroups);
    funcMerge(builder, topPLFunc, receiveGroups);
    funcMerge(builder, topPLFunc, loadGroups);
    funcMerge(builder, topPLFunc, storeGroups);
    funcClean(mod);
    return true;
  }
};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesFuncEliminatePass() {
  return std::make_unique<AriesFuncEliminate>();
}

} // namespace aries
} // namespace mlir