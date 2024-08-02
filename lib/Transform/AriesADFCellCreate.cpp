#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "llvm/Support/Debug.h"
#include "aries/Transform/Passes.h"
#include "aries/Transform/Utils.h"
#include "aries/Dialect/ADF/ADFDialect.h"

using namespace mlir;
using namespace aries;
using namespace adf;
using namespace mlir::func;

namespace {

struct AriesADFCellCreate : public AriesADFCellCreateBase<AriesADFCellCreate> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());
    StringRef topFuncName = "top_func";
  
    if (!ADFCellCreate(mod,topFuncName))
      signalPassFailure();
  }

private:
  void OpCollect(CellOp cellop, 
                 SmallVector<Operation*> &TopOps, 
                 SmallVector<Operation*> &GraphOps, 
                 SmallVector<IOPopOp> &IOPopOps, 
                 SmallVector<Value> &ArgIns){
    cellop.walk([&](Operation *op){
      if( dyn_cast<BufferOp>(op) || dyn_cast<ConnectOp>(op) || 
          dyn_cast<CallOp>(op)   || dyn_cast<ConfigPLIOOp>(op) ||
          dyn_cast<ConfigGMIOOp>(op)){
            GraphOps.push_back(op);
      }else if(auto graphio = dyn_cast<CreateGraphIOOp>(op)){
          TopOps.push_back(op);
          ArgIns.push_back(graphio.getResult());
      }else if(auto iopopOp = dyn_cast<IOPopOp>(op)){
        IOPopOps.push_back(iopopOp);
      }else{
        TopOps.push_back(op);
      }
    });
  }

  bool ADFCellCreate (ModuleOp mod, StringRef topFuncName) {
    auto builder = OpBuilder(mod);
    auto loc = builder.getUnknownLoc();
    FuncOp topFunc;
    if(!topFind(mod, topFunc, topFuncName)){
      topFunc->emitOpError("Top function not found\n");
      return false;
    }

    // Find the CellOp
    // TODO: Handle Multiple CellOps
    CellOp cellOp;
    topFunc.walk([&](CellOp op){
      cellOp = op;
    });
    if(!cellOp)
      return true;

    // Eliminate the cell.end Op
    cellOp.getBody().front().back().erase();
    
    SmallVector<Operation*> TopOps;
    SmallVector<Operation*> GraphOps;
    SmallVector<IOPopOp> IOPopOps;
    SmallVector<Value> ArgIns;
    OpCollect(cellOp, TopOps, GraphOps, IOPopOps, ArgIns);

    //Create an empty func adf_graph before the outmost band loop
    builder.setInsertionPoint(topFunc);
    auto funcName = "adf_" + cellOp.getCellName().str();
    auto funcType 
         = builder.getFunctionType(ValueRange(ArgIns), ValueRange());
    auto newfunc = builder.create<FuncOp>(loc, funcName, funcType);
    newfunc->setAttr("adf.cell",builder.getUnitAttr());
    auto destBlock = newfunc.addEntryBlock();
    builder.setInsertionPointToEnd(destBlock);
    auto returnOp = builder.create<ReturnOp>(loc);

    //Move GraphOps into adf_cell
    builder.setInsertionPoint(returnOp);
    for (Operation *op : GraphOps) {
      op->moveBefore(returnOp);
    }

    // Create the function CallOp
    builder.setInsertionPoint(cellOp);
    for (auto op : TopOps) {
      op->moveBefore(cellOp);
    }

    builder.setInsertionPoint(cellOp);
    auto callop = builder.create<CallOp>(loc, newfunc, ValueRange(ArgIns));
    callop->setAttr("adf.cell",builder.getUnitAttr());

    builder.setInsertionPoint(cellOp);
    for (auto op : IOPopOps) {
      op->moveBefore(cellOp);
    }

    builder.setInsertionPoint(cellOp);
    for (auto op : IOPopOps) {
      builder.create<IOWaitOp>(loc, op.getSrc());
    }

    // Update the references in the newfunc after move
    auto num_arg = destBlock->getNumArguments();
    for (unsigned i = 0; i < num_arg; ++i) {
        auto sourceArg = ArgIns[i];
        auto destArg = destBlock->getArgument(i);
        sourceArg.replaceUsesWithIf(destArg,[&](OpOperand &use){
            return newfunc->isProperAncestor(use.getOwner());
        });
    }

    //Create WaitLauchCellOp after all the IOwait
    builder.create<WaitLauchCellOp>(loc,callop.getCallee());

    //Erase cellop after func with adf.cell attributes has been created 
    cellOp.erase();

    // Create LaunchCellOp and move all the operations inside LaunchCellOp
    // TODO:: This is because current LaunchCellOp will be converted to
    // graph.init() which should be declared only once. Thus, now all the ops
    // are moved inside it. 
    //----------This part May need to change------------.
    Block &entryBlock = topFunc.getBody().front();
    builder.setInsertionPointToStart(&entryBlock);
    auto cellLaunchop = builder.create<LauchCellOp>(loc);
    Block *cellLaunchBlock = builder.createBlock(&cellLaunchop.getRegion());
    builder.setInsertionPointToEnd(cellLaunchBlock);
    auto endLaunchCell = builder.create<EndLauchCellOp>(loc);

    for (auto &op: llvm::make_early_inc_range(entryBlock)){
      if(!dyn_cast<LauchCellOp>(op)&&!dyn_cast<ReturnOp>(op))
        op.moveBefore(endLaunchCell);
    }

    return true;
  }

};
} // namespace



namespace mlir {
namespace aries {

std::unique_ptr<Pass> createAriesADFCellCreatePass() {
  return std::make_unique<AriesADFCellCreate>();
}

} // namespace aries
} // namespace mlir