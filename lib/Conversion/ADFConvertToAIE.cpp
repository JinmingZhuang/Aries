#include "aries/Conversion/Passes.h"
#include "aries/Dialect/ADF/ADFDialect.h"
#include "aries/Transform/Utils.h"
#include "aie/Dialect/AIE/IR/AIEDialect.h"
#include "aie/Dialect/AIEX/IR/AIEXDialect.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/Dialect/Affine/Analysis/LoopAnalysis.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"

using namespace mlir;
using namespace aries;
using namespace adf;
using namespace mlir::memref;
using namespace mlir::func;
using namespace mlir::arith;
using namespace mlir::affine;
using namespace xilinx::AIE;
using namespace xilinx::AIEX;


// Set the name to the graphio of the connectOp, find the corresponding
// graphio in the aiex.runtime_sequence
static void setGraphIOAttr(RuntimeSequenceOp seqOp, CreateGraphIOOp newio,
                           StringAttr stringAttr, bool input){
  std::string dirName;
  if(input)
    dirName = "input_io";
  else
    dirName = "output_io";
  CreateGraphIOOp oldio;
  auto input_attr = newio->getAttr(dirName);
  if(input_attr){
    auto idx = dyn_cast<IntegerAttr>(input_attr).getInt();
    seqOp.walk([&](CreateGraphIOOp graphio){
      auto attr = graphio->getAttr(dirName);
      if(!attr)
        return WalkResult::advance();
      auto idxOld = dyn_cast<IntegerAttr>(attr).getInt();
      if(idxOld == idx){
        oldio = graphio;
        return WalkResult::interrupt();
      }
      return WalkResult::advance();
    });
  }
  newio->setAttr("objFIFOName", stringAttr);
  newio->removeAttr(dirName);
  if(oldio){
    oldio->setAttr("objFIFOName", stringAttr);
    oldio->removeAttr(dirName);
  }
}

struct ConnectConvert : public OpConversionPattern<adf::ConnectOp> {
  using OpConversionPattern<adf::ConnectOp>::OpConversionPattern;
  RuntimeSequenceOp& seqOp;
  DenseMap<Value, std::pair<Value, std::string>>& memMap;
  ConnectConvert(MLIRContext *context, RuntimeSequenceOp& seqOp,
                 DenseMap<Value, std::pair<Value, std::string>>& memMap)
        : OpConversionPattern<adf::ConnectOp>(context), seqOp(seqOp), 
          memMap(memMap){}
  LogicalResult matchAndRewrite(
      adf::ConnectOp op, OpAdaptor adaptor,
      ConversionPatternRewriter &rewriter) const override {
    
    auto context = op.getContext();
    auto int32Type = rewriter.getIntegerType(32);
    auto depthAttr = rewriter.getIntegerAttr(int32Type, 2);
    auto src = op.getSrc();
    auto srcType = src.getType();
    auto dst = op.getDst();
    auto dstType = dst.getType();
    auto srcMemType = dyn_cast<MemRefType>(srcType);
    auto dstMemType = dyn_cast<MemRefType>(dstType);
    auto srcTile = memMap[src].first;
    auto dstTile = memMap[dst].first;
    MemRefType memrefType;
    std::string objFIFOName;
    StringAttr stringAttr;
    if(!srcMemType){
      memrefType = dstMemType;
      objFIFOName = memMap[dst].second;
      stringAttr = StringAttr::get(context, objFIFOName);
      auto newio = dyn_cast<CreateGraphIOOp>(src.getDefiningOp());
      setGraphIOAttr(seqOp,  newio, stringAttr, true);
    }else if(!dstMemType){
      memrefType = srcMemType;
      objFIFOName = memMap[src].second;
      stringAttr = StringAttr::get(context, objFIFOName);
      auto newio = dyn_cast<CreateGraphIOOp>(dst.getDefiningOp());
      setGraphIOAttr(seqOp, newio, stringAttr, false);
    }else{
      ;
      // TODO: Currently do nothing, but need to deal with connect between mems
    }
    if(op->hasAttr("initialize")){
      rewriter.eraseOp(op);
      return success();
    }
    auto objType = AIEObjectFifoType::get(memrefType);
    rewriter.replaceOpWithNewOp<ObjectFifoCreateOp>(op, stringAttr, srcTile, 
                                                    ValueRange{dstTile}, 
                                                    depthAttr, objType);
    return success();
  }
};

struct CellLaunchConvert : public OpConversionPattern<LaunchCellOp> {
  using OpConversionPattern<LaunchCellOp>::OpConversionPattern;
  DenseMap<CallOp, Value>& coreMap;
  CellLaunchConvert(MLIRContext *context, DenseMap<CallOp, Value>& coreMap)
                : OpConversionPattern<LaunchCellOp>(context), coreMap(coreMap){}
  LogicalResult matchAndRewrite(
      LaunchCellOp op, OpAdaptor adaptor,
      ConversionPatternRewriter &rewriter) const override {
      
    auto attr = op->getAttr("callName");
    if(!attr)
      return failure();
    auto strAttr = dyn_cast<StringAttr>(attr);
    CallOp tileCall;
    op.walk([&](CallOp call){
      if(call.getCallee() != strAttr.getValue())
        return WalkResult::advance();
      tileCall = call;
      return WalkResult::interrupt();
    });
    auto tile = coreMap[tileCall];
    auto coreOp = rewriter.create<xilinx::AIE::CoreOp>(op->getLoc(), tile);
    Region &r = coreOp.getBody();
    Block *endBlock = rewriter.createBlock(&r);
    rewriter.setInsertionPointToStart(endBlock);
    auto endOp = rewriter.create<EndOp>(op->getLoc());
    for(auto& newop : llvm::make_early_inc_range(op.getOps())){
      if(!dyn_cast<EndLaunchCellOp>(newop))
        newop.moveBefore(endOp);
    }
    rewriter.eraseOp(op);
    return success();
  }
};

struct DmaConvert : public OpConversionPattern<DmaOp> {
  using OpConversionPattern<DmaOp>::OpConversionPattern;
  DenseMap<Value, std::pair<Value, std::string>>& memMap;
  DmaConvert(MLIRContext *context, 
             DenseMap<Value, std::pair<Value, std::string>>& memMap)
        : OpConversionPattern<adf::DmaOp>(context), memMap(memMap){}
  LogicalResult matchAndRewrite(
      DmaOp op, OpAdaptor adaptor,
      ConversionPatternRewriter &rewriter) const override {
    
    auto loc = rewriter.getUnknownLoc();
    auto context = op.getContext();
    auto int32Type = rewriter.getIntegerType(32);
    auto zeroAttr = rewriter.getIntegerAttr(int32Type, 0);
    auto depthAttr = rewriter.getIntegerAttr(int32Type, 2);
    auto src = op.getSrc();
    auto srcType = src.getType();
    auto dst = op.getDst();
    auto dstType = dst.getType();
    auto srcMemType = dyn_cast<MemRefType>(srcType);
    auto dstMemType = dyn_cast<MemRefType>(dstType);
    auto srcTile = memMap[src].first;
    auto dstTile = memMap[dst].first;
    auto srcDefOp = dyn_cast<adf::BufferOp>(src.getDefiningOp());
    auto dstDefOp = dyn_cast<adf::BufferOp>(dst.getDefiningOp());
    MemRefType memrefType;
    std::string objFIFOName;
    auto srcSpace = dyn_cast<IntegerAttr>(srcMemType.getMemorySpace());
    bool store_flag; // Identify if stored to outer mem hierarchy
    Value val;
    if(srcSpace.getInt() == (int)MemorySpace::L1){
      memrefType = srcMemType;
      objFIFOName = memMap[src].second;
      store_flag = true;
      val = src;
    }else{
      memrefType = dstMemType;
      objFIFOName = memMap[dst].second;
      store_flag = false;
      val = dst;
    }
    // Get the call op that uses the memrefs
    CallOp call;
    for(auto use: val.getUsers()){
      if(auto callOp = dyn_cast<CallOp>(use)){
        call = callOp;
        break;
      }
    }
    if(!call)
      return failure();
    auto coreOp = op->getParentOfType<xilinx::AIE::CoreOp>();
    SmallVector<AffineForOp, 6> band;
    getNestedLoopBand(coreOp.getRegion(), band);
    AffineForOp redLoop;
    // Get the outermost reduction loop
    for (auto loop : band){
      if(loop->hasAttr("reduction")){
        redLoop = loop;
        break;
      }
    }
    // Create ObjectFifo and ObjectFifoLink
    auto device = op->getParentOfType<xilinx::AIE::DeviceOp>();
    auto& entryBlock = device.getRegion().front();
    auto stringAttr = StringAttr::get(context, objFIFOName);
    auto objType = AIEObjectFifoType::get(memrefType);
    rewriter.setInsertionPoint(coreOp);
    rewriter.create<ObjectFifoCreateOp>(loc, stringAttr, srcTile, 
                                        ValueRange{dstTile}, 
                                        depthAttr, objType);
    auto srcName = srcDefOp.getSymbol();
    auto dstName = dstDefOp.getSymbol();
    auto srcAttr = SymbolRefAttr::get(context, srcName);
    auto dstAttr = SymbolRefAttr::get(context, dstName);
    auto srcArray = ArrayAttr::get(context, {srcAttr});
    auto dstArray = ArrayAttr::get(context, {dstAttr});
    auto empty = ArrayAttr::get(context, {});
    auto objLink = rewriter.create<ObjectFifoLinkOp>(
                   loc, srcArray, dstArray, empty, empty);
    if(store_flag){
      // For store operations now initialize the output buffer automatically
      // Create zero init callee at the beginning of deviceOp
      rewriter.setInsertionPointToStart(&entryBlock);
      auto eleType = srcMemType.getElementType();
      auto eleAttr = TypeAttr::get(eleType);
      std::string eleStr;
      llvm::raw_string_ostream os(eleStr);
      eleAttr.print(os);
      std::string funcName = "zero_" + eleStr;
      auto callee = device.lookupSymbol<FuncOp>(funcName);
      FuncOp funcOp = callee;
      if(!callee){
        auto funcType 
          = rewriter.getFunctionType(TypeRange({srcMemType}), TypeRange({}));
        auto newfunc = rewriter.create<FuncOp>(loc, funcName, funcType);
        funcOp = newfunc;
        newfunc.setVisibility(SymbolTable::Visibility::Private);
      }
      // Create zero init caller outside of reduction loop
      // The buffer should be changed from dst to the objectfifo
      if(!redLoop){
        rewriter.setInsertionPoint(call);
      }else{
        rewriter.setInsertionPoint(redLoop);
      }
      auto acqType = AIEObjectFifoSubviewType::get(srcMemType);
      auto objACQ = rewriter.create<ObjectFifoAcquireOp>(
                    loc, acqType, ObjectFifoPort::Produce, srcName, 1);
      auto objSubview = rewriter.create<ObjectFifoSubviewAccessOp>(
                        loc, srcMemType, objACQ.getSubview(), zeroAttr);
      rewriter.create<CallOp>(loc, funcOp, ValueRange{objSubview});
      // Replace the use of the buffer
      adf::BufferOp buffer;
      device.walk([&](adf::BufferOp bufferOp) {
        // Check if the symbol name matches.
        if (bufferOp.getSymbol() == srcName) {
          buffer = bufferOp;
          return WalkResult::interrupt();
        }
        return WalkResult::advance();
      });
      auto buf = buffer.getResult();
      buf.replaceUsesWithIf(objSubview.getResult(), [&](OpOperand &use){
        bool flag = !(dyn_cast<DmaOp>(use.getOwner()));
        return flag;
      });
      // Currently store (L1->L2->L3) is different than load
      // In load, layout transform happens during L2->L1
      // In store, layout transform happens during L2->L3 (instead of L1->L2)
      objLink->setAttr("reverse", rewriter.getUnitAttr());
      if(!redLoop){
        rewriter.setInsertionPointAfter(call);
      }else{
        rewriter.setInsertionPointAfter(redLoop);
      }
      rewriter.create<ObjectFifoReleaseOp>(loc, ObjectFifoPort::Produce, 
                                           objFIFOName, 1);
      rewriter.eraseOp(op);
    }else{
      rewriter.setInsertionPoint(op);
      auto acqType = AIEObjectFifoSubviewType::get(memrefType);
      auto objACQ = rewriter.create<ObjectFifoAcquireOp>(
                    loc, acqType, ObjectFifoPort::Consume, objFIFOName, 1);
      auto objSubview = rewriter.create<ObjectFifoSubviewAccessOp>(
                        loc, memrefType, objACQ.getSubview(), zeroAttr);
      rewriter.setInsertionPointAfter(call);
      rewriter.create<ObjectFifoReleaseOp>(loc, ObjectFifoPort::Consume, 
                                           objFIFOName, 1);
      // Replace the use of the buffer
      adf::BufferOp buffer;
      device.walk([&](adf::BufferOp bufferOp) {
        // Check if the symbol name matches.
        if (bufferOp.getSymbol().str() == objFIFOName) {
          buffer = bufferOp;
          return WalkResult::interrupt();
        }
        return WalkResult::advance();
      });
      buffer.getResult().replaceAllUsesWith(objSubview.getResult());
      rewriter.eraseOp(op);
    }
    return success();
  }
};

// Broadcast is only for load from L2->L1
struct DmaBroadcastConvert : public OpConversionPattern<DmaBroadcastOp> {
  using OpConversionPattern<DmaBroadcastOp>::OpConversionPattern;
  DenseMap<Value, std::pair<Value, std::string>>& memMap;
  DmaBroadcastConvert(MLIRContext *context, 
             DenseMap<Value, std::pair<Value, std::string>>& memMap)
        : OpConversionPattern<adf::DmaBroadcastOp>(context), memMap(memMap){}
  LogicalResult matchAndRewrite(
      DmaBroadcastOp op, OpAdaptor adaptor,
      ConversionPatternRewriter &rewriter) const override {
    
    auto loc = rewriter.getUnknownLoc();
    auto context = op.getContext();
    auto int32Type = rewriter.getIntegerType(32);
    auto zeroAttr = rewriter.getIntegerAttr(int32Type, 0);
    auto depthAttr = rewriter.getIntegerAttr(int32Type, 2);
    auto src = op.getSrc();
    auto srcTile = memMap[src].first;
    auto srcDefOp = dyn_cast<adf::BufferOp>(src.getDefiningOp());
    auto srcName = srcDefOp.getSymbol(); 
    auto dsts = op.getDst();
    auto dst = dsts[0];
    auto dstTile = memMap[dst].first;
    auto dstDefOp = dyn_cast<adf::BufferOp>(dst.getDefiningOp());
    auto dstName = dstDefOp.getSymbol();
    if (dsts.size() > 1){
      llvm::errs() << "DmaBroadcast not been properly splited\n";
      return failure();
    }
    // Get the call op that uses the memrefs
    CallOp call;
    for(auto use: dst.getUsers()){
      if(auto callOp = dyn_cast<CallOp>(use)){
        call = callOp;
        break;
      }
    }
    if(!call)
      return failure();
    auto memrefType = dyn_cast<MemRefType>(dst.getType());
    auto objFIFOName = memMap[dst].second;
    auto objType = AIEObjectFifoType::get(memrefType);
    // Create ObjectFifo or update dsttiles of ObjectFifo
    auto device = op->getParentOfType<xilinx::AIE::DeviceOp>();
    auto coreOp = op->getParentOfType<xilinx::AIE::CoreOp>();
    ObjectFifoCreateOp objfifo;
    // Find if the object fifo has already been created
    for(auto objop : device.getOps<ObjectFifoCreateOp>()){
      if(auto attr = objop->getAttr("srcName")){
        auto srcArr = dyn_cast<StringAttr>(attr);
        if(srcArr.getValue() == srcName){
          objfifo = objop;
          objFIFOName = objfifo.getSymName();
          break;
        }
      }
    }
    auto stringAttr = StringAttr::get(context, objFIFOName);
    if(objfifo){
      SmallVector<Value, 4> consumeTiles = objfifo.getConsumerTiles();
      consumeTiles.push_back(dstTile);
      rewriter.setInsertionPoint(objfifo);
      auto objOp = rewriter.create<ObjectFifoCreateOp>(loc, stringAttr, srcTile, 
                                          consumeTiles, depthAttr, objType);
      auto strAttr = objfifo->getAttr("srcName");
      objOp->setAttr("srcName", strAttr);
      objfifo.erase();
    }else{
      rewriter.setInsertionPoint(coreOp);
      auto objOp = rewriter.create<ObjectFifoCreateOp>(loc, stringAttr, srcTile, 
                                          ValueRange{dstTile}, 
                                          depthAttr, objType);
      auto strAttr = StringAttr::get(context, srcName);
      objOp->setAttr("srcName", strAttr);
      auto srcAttr = SymbolRefAttr::get(context, srcName);
      auto dstAttr = SymbolRefAttr::get(context, dstName);
      auto srcArr = ArrayAttr::get(context, {srcAttr});
      auto dstArr = ArrayAttr::get(context, {dstAttr});
      auto empty = ArrayAttr::get(context, {});
      rewriter.create<ObjectFifoLinkOp>(loc, srcArr, dstArr, empty, empty);
    }
    rewriter.setInsertionPoint(op);
    auto acqType = AIEObjectFifoSubviewType::get(memrefType);
    auto objACQ = rewriter.create<ObjectFifoAcquireOp>(
                  loc, acqType, ObjectFifoPort::Consume, objFIFOName, 1);
    auto objSubview = rewriter.create<ObjectFifoSubviewAccessOp>(
                      loc, memrefType, objACQ.getSubview(), zeroAttr);
    rewriter.setInsertionPoint(call);
    rewriter.create<ObjectFifoReleaseOp>(loc, ObjectFifoPort::Consume, 
                                         objFIFOName, 1);
    // Replace the use of the buffer by using the original objFIFOName
    adf::BufferOp buffer;
    auto objFIFONameOrigin = memMap[dst].second;
    device.walk([&](adf::BufferOp bufferOp) {
      // Check if the symbol name matches.
      if (bufferOp.getSymbol().str() == objFIFONameOrigin) {
        buffer = bufferOp;
        return WalkResult::interrupt();
      }
      return WalkResult::advance();
    });
    if(buffer)
      buffer.getResult().replaceAllUsesWith(objSubview.getResult());
    rewriter.eraseOp(op);
    return success();
  }
};

struct BufferConvert : public OpConversionPattern<adf::BufferOp> {
  using OpConversionPattern<adf::BufferOp>::OpConversionPattern;
  LogicalResult matchAndRewrite(
      adf::BufferOp op, OpAdaptor adaptor,
      ConversionPatternRewriter &rewriter) const override {
    
    rewriter.eraseOp(op);
    return success();
  }
};

static bool getDMALayout(mlir::AffineMap map, MemRefType memType,
                         SmallVector<Value> operands,
                         SmallVector<AffineForOp, 6> nestedBand,
                         SmallVector<int64_t, 4>& offsets,
                         SmallVectorImpl<int64_t>& sizes,
                         SmallVector<int64_t, 4>& strides,
                         const int dmaDim, const int numZeroLoop){
  auto dimNum = map.getNumDims();
  auto dimSymbol = map.getNumSymbols();
  auto shape = memType.getShape();
  auto rank = memType.getRank();
  // Get sizes
  for(auto loop : nestedBand){
    auto size = getConstantTripCount(loop);
    if(!size.has_value())
      return false;
    sizes.push_back(size.value());
  }
  //Get offset and strides
  for(unsigned i = 0; i < rank; i++){
    auto expr = map.getResult(i);
    // flattened form [dims, symbols, locals, constant]
    llvm::SmallVector<int64_t> flattenedExpr;
    if (failed(getFlattenedAffineExpr(expr, dimNum, dimSymbol, 
                                      &flattenedExpr)))
      return false;
    // Get stides by tranversing all the operands
    for (unsigned dim = 0; dim < dimNum; ++dim) {
      auto loop = getForInductionVarOwner(operands[dim]);
      if (!loop)
        continue;
      auto it = llvm::find(nestedBand, loop);
      if(it!=nestedBand.end()){
        auto coeff = flattenedExpr[dim];
        if(coeff==0)
          continue;
        // Serialize the strides of current rank by multiplying the size of 
        // the higher ranks
        for(unsigned j = i+1; j < rank; j++)
          coeff = coeff * shape[j];
        unsigned depth = it - nestedBand.begin();
        strides[depth+numZeroLoop] += coeff;
      }else{
        llvm::errs() << "Loop not found in the surrounding loop band\n";
        return false;
      }
    }
    // Get offset by analyze the constant
    auto coeff = flattenedExpr[flattenedExpr.size()-1];
    // Serialize the strides of current rank by multiplying the size of 
    // the higher ranks
    for(unsigned j = i+1; j < rank; j++)
      coeff = coeff * shape[j];
    offsets[dmaDim-1] += coeff;
  }
  return true;
}

// Convert the Surrounding nest for loops and the IOWriteOp to dma_memcpy_nd
struct IOWriteConvert : public OpConversionPattern<adf::IOWriteOp> {
  using OpConversionPattern<adf::IOWriteOp>::OpConversionPattern;
  IOWriteConvert(MLIRContext *context)
        : OpConversionPattern<adf::IOWriteOp>(context){}
  LogicalResult matchAndRewrite(
      adf::IOWriteOp op, OpAdaptor adaptor,
      ConversionPatternRewriter &rewriter) const override {
    
    const int dmaDim=4;
    auto loc = rewriter.getUnknownLoc();
    // May need to set as an input of the conversion
    auto loop = op->getParentOfType<AffineForOp>();
    if(!loop){
      //TODO:: Convert to dma_memcpy_nd with only offset
      return success();
    }
    // Get nested loop from outermost and assume the loops have been normalized
    SmallVector<AffineForOp, 6> nestedBand;
    getSurroundingLoops(*loop, nestedBand);
    nestedBand.push_back(loop);
    int numLoop = nestedBand.size();
    int numZeroLoop = dmaDim - numLoop;
    if(numZeroLoop<0){
      llvm::errs() << "Number of loop invariant exceeds DMA capability(4)\n";
      return failure();
    }
    // Serialize the nd memory access and get the offsets, sizes and strides
    // Currently only considered static offsets, stride
    // And only consider the loop-index as operands of affine.load
    auto loadVal = op.getSrc();
    auto loadOp = dyn_cast<AffineLoadOp>(loadVal.getDefiningOp());
    auto map = loadOp.getAffineMap();
    auto operands = loadOp.getMapOperands();
    auto memType = loadOp.getMemRefType();
    auto mem = loadOp.getMemRef();
    SmallVector<int64_t, 4> offsets(dmaDim, 0);
    SmallVector<int64_t, dmaDim> sizes(numZeroLoop, 1);
    SmallVector<int64_t, 4> strides(dmaDim, 0);
    if(!getDMALayout(map, memType, operands, nestedBand, offsets, sizes, 
                     strides, dmaDim, numZeroLoop))
      return failure();
    // Create NpuDmaMemcpyNdOp op and delete the outerLoop
    auto outLoop = nestedBand[0];
    rewriter.setInsertionPoint(outLoop);
    PacketInfoAttr emptyPacket;
    auto dst = op.getDst();
    auto graphio = dyn_cast<CreateGraphIOOp>(dst.getDefiningOp());
    StringRef metadata = dyn_cast<StringAttr>(graphio->getAttr("objFIFOName"));
    int64_t id = dyn_cast<IntegerAttr>(op->getAttr("id")).getInt();
    rewriter.create<NpuDmaMemcpyNdOp>(loc, 0, 0, mem,
                                      SmallVector<Value>{}, 
                                      SmallVector<Value>{},
                                      SmallVector<Value>{},
                                      ArrayRef(offsets), ArrayRef(sizes),
                                      ArrayRef(strides), emptyPacket, metadata, 
                                      /*bd_id*/ id, false, 0, 0, 0, 0, 0, 0);
    rewriter.eraseOp(op);
    rewriter.eraseOp(outLoop);
    return success();
  }
};

// Convert the Surrounding nest for loops and the IOReadOp to dma_memcpy_nd
struct IOReadConvert : public OpConversionPattern<adf::IOReadOp> {
  using OpConversionPattern<adf::IOReadOp>::OpConversionPattern;
  IOReadConvert(MLIRContext *context)
        : OpConversionPattern<adf::IOReadOp>(context){}
  LogicalResult matchAndRewrite(
      adf::IOReadOp op, OpAdaptor adaptor,
      ConversionPatternRewriter &rewriter) const override {
    
    const int dmaDim=4;
    auto loc = rewriter.getUnknownLoc();
    // May need to set as an input of the conversion
    auto loop = op->getParentOfType<AffineForOp>();
    if(!loop){
      //TODO:: Convert to dma_memcpy_nd with only offset
      return success();
    }
    // Get nested loop from outermost and assume the loops have been normalized
    SmallVector<AffineForOp, 6> nestedBand;
    getSurroundingLoops(*loop, nestedBand);
    nestedBand.push_back(loop);
    int numLoop = nestedBand.size();
    int numZeroLoop = dmaDim - numLoop;
    if(numZeroLoop<0){
      llvm::errs() << "Number of loop invariant exceeds DMA capability(4)\n";
      return failure();
    }
    // Serialize the nd memory access and get the offsets, sizes and strides
    // Currently only considered static offsets, stride
    // And only consider the loop-index as operands of affine.load
    auto readVal = op.getResult();
    AffineStoreOp storeOp;
    for(auto use: readVal.getUsers()){
      if(auto store = dyn_cast<AffineStoreOp>(use)){
        storeOp = store;
        break;
      }
    }
    if(!storeOp)
      return failure();
    auto map = storeOp.getAffineMap();
    auto operands = storeOp.getMapOperands();
    auto memType = storeOp.getMemRefType();
    auto mem = storeOp.getMemRef();
    SmallVector<int64_t, 4> offsets(dmaDim, 0);
    SmallVector<int64_t, dmaDim> sizes(numZeroLoop, 1);
    SmallVector<int64_t, 4> strides(dmaDim, 0);
    if(!getDMALayout(map, memType, operands, nestedBand, offsets, sizes, 
                     strides, dmaDim, numZeroLoop))
      return failure();
    // Create NpuDmaMemcpyNdOp op and delete the outerLoop
    auto outLoop = nestedBand[0];
    rewriter.setInsertionPoint(outLoop);
    PacketInfoAttr emptyPacket;
    auto ioVal = op.getSrc();
    auto graphio = dyn_cast<CreateGraphIOOp>(ioVal.getDefiningOp());
    StringRef metadata = dyn_cast<StringAttr>(graphio->getAttr("objFIFOName"));
    int64_t id = dyn_cast<IntegerAttr>(op->getAttr("id")).getInt();
    rewriter.create<NpuDmaMemcpyNdOp>(loc, 0, 0, mem,
                                      SmallVector<Value>{}, 
                                      SmallVector<Value>{},
                                      SmallVector<Value>{},
                                      ArrayRef(offsets), ArrayRef(sizes),
                                      ArrayRef(strides), emptyPacket, metadata, 
                                      /*bd_id*/ id, false, 0, 0, 0, 0, 0, 0);
    rewriter.create<NpuDmaWaitOp>(loc, metadata);
    rewriter.eraseOp(op);
    rewriter.eraseOp(outLoop);
    return success();
  }
};

struct GraphIOConvert : public OpConversionPattern<adf::CreateGraphIOOp> {
  using OpConversionPattern<adf::CreateGraphIOOp>::OpConversionPattern;
  LogicalResult matchAndRewrite(
      adf::CreateGraphIOOp op, OpAdaptor adaptor,
      ConversionPatternRewriter &rewriter) const override {
    
    rewriter.eraseOp(op);
    return success();
  }
};

namespace {

struct ADFConvertToAIE : public ADFConvertToAIEBase<ADFConvertToAIE> {
public:
  void runOnOperation() override {
    auto mod = dyn_cast<ModuleOp>(getOperation());  
    if (!ConvertToAIE(mod))
      signalPassFailure();
  }

private:
  // Create a new device Op and specify it to npu1_4col
  // Move all the funcs makred by adf.kernel, adf.cell, adf.func to device
  void createDevice(ModuleOp mod, OpBuilder builder, DeviceOp& device, 
                    EndOp& endOp){
    auto loc = builder.getUnknownLoc();
    auto context = builder.getContext();
    auto& destBlock = mod.getRegion().front();
    builder.setInsertionPointToStart(&destBlock);
    auto deviceAttr = AIEDeviceAttr::get(context, AIEDevice::npu1_4col);
    device = builder.create<DeviceOp>(loc, deviceAttr);
    auto& entryBlock = device.getRegion().emplaceBlock();
    DeviceOp::ensureTerminator(device.getBodyRegion(), builder, loc);
    endOp = dyn_cast<EndOp>(entryBlock.getTerminator());
    mod.walk([&](FuncOp func){
      if(!func->hasAttr("adf.kernel") && !func->hasAttr("adf.cell") && 
         !func->hasAttr("adf.func") )
        func.erase();
      else
        func->moveBefore(endOp);
    });
  }

  // Tranverse each operands of adf.cell calls and create a newIO before endOp
  void createNewIO(OpBuilder builder, CallOp call, EndOp endOp,
                   unsigned &inCnt, unsigned &outCnt,
                   SmallVector<Value, 6>& graphIOs,
                   DenseMap<CreateGraphIOOp, CreateGraphIOOp>& graphIOMap){
    auto indexType = builder.getIndexType();
    for(auto operand : call.getOperands()){
      auto defineOp = operand.getDefiningOp();
      if(!defineOp || !dyn_cast<CreateGraphIOOp>(defineOp)){
        llvm::errs() << "Adf cell has non-graphio arguments\n";
        return;
      }
      auto graphio = dyn_cast<CreateGraphIOOp>(defineOp);
      auto tempType = graphio.getType();
      auto ioType = dyn_cast<GMIOType>(tempType);
      if(!ioType){
        llvm::errs() << "Adf cell has non-gmio arguments\n";
        return;
      }
      auto dir = ioType.getDir();
      if(dir == PortDir::In){
        auto intAttr = builder.getIntegerAttr(indexType, inCnt++);
        graphio->setAttr("input_io", intAttr);
      }else{
        auto intAttr = builder.getIntegerAttr(indexType, outCnt++);
        graphio->setAttr("output_io", intAttr);
      }
      auto newio = graphio.clone();
      builder.setInsertionPoint(endOp);
      builder.insert(newio);
      graphIOs.push_back(newio.getResult());
      // Record all the GraphIO maps
      if(graphIOMap.count(newio)){
        auto oldio = graphIOMap[newio];
        if (oldio != graphio){
          llvm::errs() << "Find IOs has one-to-many mapping\n";
          return;
        }
      }else{
        graphIOMap[newio] = graphio;
      }
    }
  }

  // This function create a callee, move all the operations of callee before
  // the endOp and replace the arguments by graphios
  void moveToDevice(OpBuilder builder, CallOp call, DeviceOp device, 
                    EndOp endOp, SmallVector<Value, 6>& graphIOs){
    auto calleeName = call.getCallee();
    auto callee = device.lookupSymbol<FuncOp>(calleeName);
    auto newCallee = dyn_cast<FuncOp>(callee->clone());
    builder.setInsertionPoint(callee);
    builder.insert(newCallee);
    auto numIO = graphIOs.size();
    for (unsigned i = 0; i < numIO; ++i) {
      auto oldio = newCallee.getArgument(i);
      auto newio = graphIOs[i];
      oldio.replaceUsesWithIf(newio,[&](OpOperand &use){
          return newCallee->isProperAncestor(use.getOwner());
      });
    }
    for(auto& op: llvm::make_early_inc_range(newCallee.getOps())){
      if(!dyn_cast<ReturnOp>(op))
        op.moveBefore(endOp);
    }
    newCallee.erase();
  }

  // This function creates the graphios and lift the operations
  // inside the func marked by adf.cell to deviceOp
  void liftCell(OpBuilder builder, DeviceOp device, EndOp endOp,
                DenseMap<CreateGraphIOOp, CreateGraphIOOp>& graphIOMap){
    unsigned inCnt=0, outCnt=0;
    // First tranverse create newIOs and move ops from callee to device
    for(auto func: device.getOps<FuncOp>()){
      if(!func->hasAttr("adf.func"))
        continue;
      func.walk([&](CallOp call){
        if(!call->hasAttr("adf.cell"))
          return WalkResult::advance();
        SmallVector<Value, 6> graphIOs;
        createNewIO(builder, call, endOp, inCnt, outCnt, graphIOs, graphIOMap);
        moveToDevice(builder, call, device, endOp, graphIOs);
        call.erase();
        return WalkResult::advance();
      });
    }
    // Second tranverse, remove adf.cells
    for(auto func: llvm::make_early_inc_range(device.getOps<FuncOp>())){
      if(!func->hasAttr("adf.cell"))
        continue;
      func.erase();
    }
  }

  void eraseUnusedOp(Value src, Value dst, Operation* op,
                     SmallVector<Value, 4> oprands,
                     SmallVector<Value, 4> results){
    auto it0 = llvm::find(oprands, src);
    auto it1 = llvm::find(oprands, dst);
    auto it2 = llvm::find(results, src);
    auto it3 = llvm::find(results, dst);
    // If op not used in operands and results then erase it
    if(it0 == oprands.end() && it1 == oprands.end() &&
       it2 == results.end() && it3 == results.end())
      op->erase();
  }

  // Keep a specific callOp and delete the unused callOp, DmaOp, ConnectOp,
  // and DmaBroadcastOp
  void removeOps(LaunchCellOp cellLaunch, IntegerAttr attr){
    // First traverse find the specific CallOp
    CallOp call;
    cellLaunch.walk([&](CallOp op){
      auto callName = op.getCallee();
      auto intAttr = dyn_cast<IntegerAttr>(op->getAttr(callName));
      if(intAttr.getInt() == attr.getInt()){
        call = op;
        return WalkResult::interrupt();
      }
      return WalkResult::advance();
    });
    SmallVector<Value, 4> oprands = call.getOperands();
    SmallVector<Value, 4> results = call.getResults();
    cellLaunch.walk([&](Operation* op){
      if(auto dmaOp = dyn_cast<adf::DmaOp>(op)){
        auto src = dmaOp.getSrc();
        auto dst = dmaOp.getDst();
        eraseUnusedOp(src, dst, op, oprands, results);
      }else if(auto conncetOp = dyn_cast<adf::ConnectOp>(op)){
        auto src = conncetOp.getSrc();
        auto dst = conncetOp.getDst();
        eraseUnusedOp(src, dst, op, oprands, results);
      }else if(auto broadcast = dyn_cast<adf::DmaBroadcastOp>(op)){
        auto src = broadcast.getSrc();
        auto dst = broadcast.getDst()[0];
        eraseUnusedOp(src, dst, op, oprands, results);
      }else if(auto newCall = dyn_cast<CallOp>(op)){
        if(newCall != call)
          op->erase();
      }
    });
  }

  // 1) Remove tripCount attr of cellLauchOp
  // 2) Split callOp in cellLauchOp to difference cellLauchOps
  void splitCores(OpBuilder builder, DeviceOp device){
    auto loc = builder.getUnknownLoc();
    auto context = builder.getContext();
    for (auto cellLaunch: 
              llvm::make_early_inc_range(device.getOps<LaunchCellOp>())){
      cellLaunch->removeAttr("tripCount");
      cellLaunch.walk([&](CallOp call){
        builder.setInsertionPoint(cellLaunch);
        auto callName = call.getCallee();
        if(!call->hasAttr("adf.kernel"))
          return WalkResult::advance();
        auto intAttr = dyn_cast<IntegerAttr>(call->getAttr(callName));
        auto newLaunch = dyn_cast<LaunchCellOp>(cellLaunch->clone());
        auto strAttr = StringAttr::get(context, callName);
        newLaunch->setAttr("callName", strAttr);
        builder.insert(newLaunch);
        removeOps(newLaunch, intAttr);
        auto& entryBlock = newLaunch.getRegion().front();
        builder.setInsertionPointToStart(&entryBlock);
        auto infinteLoop = builder.create<AffineForOp>(loc, 0, UINT_MAX, 1);
        auto forblock = infinteLoop.getBody();
        auto forYiledOp = dyn_cast<AffineYieldOp>(forblock->getTerminator()); 
        for(auto& op : llvm::make_early_inc_range(newLaunch.getOps())){
          if(auto loop = dyn_cast<AffineForOp>(op)){
            if( loop!= infinteLoop)
              op.moveBefore(forYiledOp);
          }else if(!dyn_cast<EndLaunchCellOp>(op)){
            op.moveBefore(forYiledOp);
          }
        }
        return WalkResult::advance();
      });
      cellLaunch.erase();
    }
  }

  bool findADFFunc(DeviceOp device, FuncOp& adfFunc){
    device.walk([&](FuncOp func){
      if(func->hasAttr("adf.func")){
        adfFunc = func;
        return WalkResult::interrupt();
      }
      return WalkResult::advance();
    });
    if(!adfFunc)
      return false;
    return true;
  }

  // Move store to after reduction loop to construct output stationary dataflow
  bool moveStore(FuncOp func){
    AffineForOp arrayForOp;  
    func.walk([&](AffineForOp forOp){
      if(forOp->hasAttr("Array_Partition"))
        arrayForOp = forOp;
    });
    if(!arrayForOp)
      return false;
    SmallVector<AffineForOp, 6> band;
    band.push_back(arrayForOp);
    getSurroundingLoops(*arrayForOp, band, false);
    if(!band[0]->hasAttr("reduction"))
      return true;
    AffineForOp outerRedLoop;
    for(auto loop: band){
      if(loop->hasAttr("reduction")){
        if(loop == band[0] || outerRedLoop->getParentOp() == loop){
          outerRedLoop = loop;
        }else{
          llvm::errs() << "Find non-nested reduction loops\n";
          return false;
        }
      }else{
        break;
      }
    }
    for (auto loop: llvm::make_early_inc_range(
                    arrayForOp.getOps<AffineForOp>())){
      if(loop->hasAttr("store"))
        loop->moveAfter(outerRedLoop);
    }
    return true;
  }

  // 1) Eliminate the output initialization loops
  // 2) Unroll the loops that has tripCount = 1
  bool unrollDMA(OpBuilder builder, FuncOp adfFunc){
    AffineForOp arrayForOp;   
    auto flag = adfFunc.walk([&](AffineForOp forOp){
      if(forOp->hasAttr("initialize"))
        forOp.erase();
      else{
        auto tripCount = getConstantTripCount(forOp);
        if(tripCount.has_value() && tripCount == 1)
          if (failed(loopUnrollFull(forOp)))
            return WalkResult::interrupt();
      }
      return WalkResult::advance();
    });
    if(flag == WalkResult::interrupt())
      return false;
    PassManager pm(&getContext());
    pm.addPass(createCSEPass());
    pm.addPass(createCanonicalizerPass());
    if (failed(pm.run(adfFunc)))
      signalPassFailure();
    return true;
  }

  // Convert adf.io.push/pop that moves data between memrefs and ios to 
  // affine load&store
  WalkResult ConvertIOToAffine(OpBuilder builder, Operation* op,  bool iopush){
    auto loc = builder.getUnknownLoc();
    IOPushOp iopushOp;
    IOPopOp iopopOp;
    Value src, dst;
    SmallVector<Value> offsets;
    SmallVector<Value> sizes;
    MemRefType type;
    if(iopush){
      iopushOp = dyn_cast<IOPushOp>(op);
      src = iopushOp.getSrc();
      type = dyn_cast<MemRefType>(src.getType());
      dst = iopushOp.getDst();
      offsets = iopushOp.getSrcOffsets();
      sizes = iopushOp.getSrcSizes();
      builder.setInsertionPoint(iopushOp);
    }else{
      iopopOp = dyn_cast<IOPopOp>(op);
      src = iopopOp.getSrc();
      dst = iopopOp.getDst();
      type = dyn_cast<MemRefType>(dst.getType());
      offsets = iopopOp.getDstOffsets();
      sizes = iopopOp.getDstSizes();
      builder.setInsertionPoint(iopopOp);
    }
    auto elementType = type.getElementType();
    // Create for loops for IOPush/Pop Ops
    SmallVector<AffineForOp, 4> newIOLoops;
    auto rank = offsets.size();
    for(unsigned i = 0; i < rank; i++){
      auto size = sizes[i];
      auto constantSize = dyn_cast<arith::ConstantOp>(size.getDefiningOp());
      if(!constantSize)
        return WalkResult::interrupt();
      auto sizeAttr = dyn_cast<IntegerAttr>(constantSize.getValue());
      auto sizeInt = sizeAttr.getInt();
      auto newIOForOp 
           = builder.create<AffineForOp>(loc, 0, sizeInt, 1);
      newIOLoops.push_back(newIOForOp);
      builder.setInsertionPointToStart(newIOForOp.getBody());
    }
    // Get the access operands of affine load/store
    auto newInnerIOLoop = newIOLoops[newIOLoops.size()-1];
    auto newInnerIOYiled = newInnerIOLoop.getBody()->getTerminator();
    auto d0 = builder.getAffineDimExpr(0);
    auto d1 = builder.getAffineDimExpr(1);
    // new map is var * stride + offset (stride is assumed to be 1)
    auto map = AffineMap::get(2, 0, d0 + d1, builder.getContext());
    SmallVector<Value> newApplyopIOs;
    for(unsigned i = 0; i < rank; i++){
      SmallVector<Value, 2> applyOperands;
      auto newIOLoop = newIOLoops[i];
      auto var = newIOLoop.getInductionVar();
      auto offset = offsets[i];
      applyOperands.push_back(var);
      applyOperands.push_back(offset);
      builder.setInsertionPoint(newInnerIOYiled);
      auto newApplyopIO=builder.create<AffineApplyOp>(loc, map, applyOperands);
      newApplyopIOs.push_back(newApplyopIO);
    }if(iopush){
      auto loadOp = builder.create<AffineLoadOp>(loc, src, newApplyopIOs);
      builder.create<IOWriteOp>(loc, loadOp, dst);
    }else{
      auto result = builder.create<IOReadOp>(loc, elementType, src);
      builder.create<AffineStoreOp>(loc, result, dst, newApplyopIOs);
    }
    return WalkResult::advance();
  }

  // Materialize IOPush/IOPop to affine.load and affine.store
  bool processIO(OpBuilder builder, FuncOp adfFunc){
    auto flag = adfFunc.walk([&](Operation* op){
      WalkResult result;
      if(dyn_cast<IOPushOp>(op)){
        result = ConvertIOToAffine(builder, op, true);
        op->erase();
      }else if(dyn_cast<IOPopOp>(op)){
        result = ConvertIOToAffine(builder, op, false);
        op->erase();
      }else{
        result = WalkResult::advance();
      }
      return result;
    });
    if (flag == WalkResult::interrupt()) 
      return false;
    // Canonicalize the affine.applys and the affine maps
    PassManager pm(&getContext());
    pm.addPass(createCSEPass());
    pm.addPass(createCanonicalizerPass());
    if (failed(pm.run(adfFunc)))
      signalPassFailure();
    return true;
  }

  // Current AIE-ML supports 4D slicing DMA instructions.
  // This function will splict each IO with maximum 4 loops under
  // common nested loops for all the IOs. The common nested loops will
  // be unrolled and converted to different DMA ins. 
  bool splitIO(OpBuilder builder, FuncOp adfFunc){
    auto loc = builder.getUnknownLoc();
    auto firstForOp = getFirstOpOfType<AffineForOp>(adfFunc.getRegion());
    if(!firstForOp)
      return true;
    // Check if the first op is a common nested loop for all the IOWrite/IORead
    bool flag = true;
    adfFunc.walk([&](Operation* op){
      if(auto writeOp = dyn_cast<IOWriteOp>(op)){
        if(!firstForOp->isAncestor(writeOp)){
          flag = false;
          return WalkResult::interrupt();
        }
      }else if(auto readOp = dyn_cast<IOReadOp>(op)){
        if(!firstForOp->isAncestor(readOp)){
          flag = false;
          return WalkResult::interrupt();
        }
      }
      return WalkResult::advance();
    });
    //Get the common perfect nested loops
    SmallVector<AffineForOp, 6> comBand;
    // flag=1 means firstForOp is a common nested loop
    if(flag)
      getPerfectlyNestedLoops(comBand, firstForOp);
    unsigned comBandSize = comBand.size();
    // Get the surrounding non-common loops for IOWrite/IORead
    // From innermost to outermost, meaning nestedBand[0] is innermost loop
    SmallVector<SmallVector<AffineForOp, 6>, 6> bands;
    SmallVector<unsigned, 6> bandSizes;
    adfFunc.walk([&](Operation* op){
      if(auto writeOp = dyn_cast<IOWriteOp>(op)){
        SmallVector<AffineForOp, 6> nestedBand;
        auto loop = writeOp->getParentOfType<AffineForOp>();
        if(!loop)
          return WalkResult::advance();
        nestedBand.push_back(loop);
        getSurroundingLoops(*loop, nestedBand, false);
        for(unsigned i = 0 ; i < comBandSize; i++)
          nestedBand.pop_back();
        bands.push_back(nestedBand);
        bandSizes.push_back(nestedBand.size());
      }else if(auto readOp = dyn_cast<IOReadOp>(op)){
        SmallVector<AffineForOp, 6> nestedBand;
        auto loop = readOp->getParentOfType<AffineForOp>();
        if(!loop)
          return WalkResult::advance();
        nestedBand.push_back(loop);
        getSurroundingLoops(*loop, nestedBand, false);
        for(unsigned i = 0 ; i < comBandSize; i++)
          nestedBand.pop_back();
        bands.push_back(nestedBand);
        bandSizes.push_back(nestedBand.size());
      }
      return WalkResult::advance();
    });
    // For each band, limit the number of loops to 4. If for every band, it's
    // under 4. Then add common loops in until one of the band first reach 4.
    auto maxIt = std::max_element(bandSizes.begin(), bandSizes.end());
    auto maxSize = *maxIt;
    AffineForOp oldComLoop; //Outermost loop that needs to be deleted
    if(maxSize < 4){
      for(unsigned i = 0; i < 4-maxSize; i++){
        if(comBand.empty())
          break;
        auto newLoop = comBand.back();
        oldComLoop = newLoop;
        comBand.pop_back();
        comBandSize--;
        for (auto &band : bands)
          band.push_back(newLoop);
        for (auto &bandSize : bandSizes)
          bandSize++;
      }
      // Split the IOs into different region under the remaining common nested
      // loops. And unroll the common nested for loops 
      // 1) Tranverse the bands, and clone the forLoops, then move the
      // operations inside the new innermost Loop.
      SmallVector<AffineForOp, 4> oldLoops;
      for (auto band : bands){
        auto newBand = band;
        // Reverse it back, newBand start with the outermost loop
        std::reverse(newBand.begin(), newBand.end());
        if(oldComLoop){
          builder.setInsertionPoint(oldComLoop);
        }else{
          auto outerLoop = newBand[0];
          builder.setInsertionPoint(outerLoop);
          auto it = llvm::find(oldLoops, outerLoop);
          if(it == oldLoops.end())
            oldLoops.push_back(outerLoop);
        }
        SmallVector<AffineForOp, 4> newForOps;
        for(auto loop : newBand){
          auto newForOp 
               = builder.create<AffineForOp>(loc,
                                             loop.getLowerBoundOperands(),
                                             loop.getLowerBoundMap(),
                                             loop.getUpperBoundOperands(),
                                             loop.getUpperBoundMap(),
                                             loop.getStepAsInt());
          newForOps.push_back(newForOp);
          if(loop->hasAttr("reduction"))
            newForOp->setAttr("reduction",builder.getUnitAttr());
          builder.setInsertionPointToStart(newForOp.getBody());
        }
        auto innerNewLoop = newForOps[newForOps.size()-1];
        auto innerNewYiled = innerNewLoop.getBody()->getTerminator();
        // Move operations insert newInnerLoop
        auto innerLoop = newBand[newBand.size()-1];
        for(auto& op : llvm::make_early_inc_range(innerLoop.getOps())){
          if(!dyn_cast<AffineYieldOp>(op))
            op.moveBefore(innerNewYiled);
        }
        // Update the loop variables
        auto numVi = newForOps.size();
        for (unsigned i = 0; i < numVi; ++i) {
          auto oldvi = newBand[i].getInductionVar();
          auto newvi = newForOps[i].getInductionVar();
          oldvi.replaceUsesWithIf(newvi,[&](OpOperand &use){
              return innerNewLoop->isProperAncestor(use.getOwner());
          });
        }
      }
      // Delete the original loops
      // If the loops are moved to a common loop then just delete it
      if(oldComLoop)
        oldComLoop.erase();
      else{ // Delete the previous loops
        for(auto loop : llvm::make_early_inc_range(oldLoops))
          loop.erase();
      }
      // Unroll the common nested Loops
      for(auto loop : comBand){
        if (failed(loopUnrollFull(loop)))
            return false;
      }
    }else{ 
      // TODO:: Deal with the case that maxSize is greater than 5 
      // Need to tranverse the loop nest and unroll some of the non-common loops
      // Unroll the common nested Loops
      for(auto loop : comBand){
        if (failed(loopUnrollFull(loop)))
            return false;
      }
    }
    PassManager pm(&getContext());
    pm.addPass(createCSEPass());
    pm.addPass(createCanonicalizerPass());
    if (failed(pm.run(adfFunc)))
      signalPassFailure();
    return true;
  }

  // Assign bd ID for IOReadOp and IOWriteOp
  void assignID(OpBuilder builder, FuncOp adfFunc){
    const int maxBD = 16;
    int halfBD = maxBD/2;
    bool ping_pang = false;
    int cnt = 0;
    adfFunc.walk([&](Operation* op){
      int id = ping_pang * halfBD + (cnt%halfBD);
      auto attr = builder.getIntegerAttr(builder.getIndexType(), id);
      if(auto writeOp = dyn_cast<IOWriteOp>(op)){
        writeOp->setAttr("id", attr);
        cnt++;
      }else if(auto readOp = dyn_cast<IOReadOp>(op)){
        readOp->setAttr("id", attr);
        cnt=0;
        ping_pang = !ping_pang;
      }
      return WalkResult::advance();
    });
  }

  // Create aiex.runtime_sequence with deviceOp
  // Move all the operations in func.func makred by adf.func to runtime_sequence
  void createSeq(OpBuilder builder, FuncOp adfFunc, EndOp endOp,
                 RuntimeSequenceOp & seqOp){
    auto loc = builder.getUnknownLoc();
    auto& srcBlock = adfFunc.getRegion().front();
    builder.setInsertionPoint(endOp);
    StringAttr empty_attr;
    seqOp = builder.create<RuntimeSequenceOp>(loc, empty_attr);
    seqOp.getBody().push_back(new Block);
    auto& destBlock = seqOp.getRegion().front();
    for (auto arg : srcBlock.getArguments()) {
      destBlock.addArgument(arg.getType(), arg.getLoc());
    }
    for (auto &op : llvm::make_early_inc_range(srcBlock)) {
      if (isa<func::ReturnOp>(op))
        continue;
      op.moveBefore(&destBlock, destBlock.end());
    }
    // Update the references in the newfunc after move
    auto argNum = destBlock.getNumArguments();
    for (unsigned i = 0; i < argNum; ++i) {
      auto srcArg = srcBlock.getArgument(i);
      auto destArg = destBlock.getArgument(i);
      srcArg.replaceUsesWithIf(destArg,[&](OpOperand &use){
          return seqOp->isProperAncestor(use.getOwner());
      });
    }
    adfFunc.erase();
  }

  // Create and record tileOp for shim, mem-tile and cores
  void recordMap(OpBuilder builder, DeviceOp device,
                 DenseMap<Value, std::pair<Value, std::string>>& memMap,
                 DenseMap<CallOp, Value>& coreMap){
    auto loc = builder.getUnknownLoc();
    auto& entryBlock = device.getRegion().front();
    builder.setInsertionPointToStart(&entryBlock);
    SmallVector<std::tuple<Value, int32_t, int32_t>, 4> tileLocs;
    device.walk([&](BuffLocOp bufLoc){
      Value buf = bufLoc.getBuffer();
      auto buffOp = dyn_cast<adf::BufferOp>(buf.getDefiningOp());
      auto col = bufLoc.getCol();
      auto constCol = dyn_cast<arith::ConstantOp>(col.getDefiningOp());
      int32_t colInt = dyn_cast<IntegerAttr>(constCol.getValue()).getInt();
      auto row = bufLoc.getRow();
      auto constRow = dyn_cast<arith::ConstantOp>(row.getDefiningOp());
      int32_t rowInt = dyn_cast<IntegerAttr>(constRow.getValue()).getInt();
      auto it = llvm::find_if(tileLocs, [&colInt, &rowInt](
                            const std::tuple<Value, int32_t, int32_t>& tileLoc){
        return std::get<1>(tileLoc) == colInt && std::get<2>(tileLoc) == rowInt;
      });
      if(it != tileLocs.end()){
        memMap[buf].first = std::get<0>(*it);
        memMap[buf].second = buffOp.getSymbol();
      }else{
        auto tile = builder.create<TileOp>(loc, colInt, rowInt);
        memMap[buf].first = tile.getResult();
        memMap[buf].second = buffOp.getSymbol();
        tileLocs.push_back(std::tuple(tile, colInt, rowInt));
      }
      bufLoc.erase();
    });
    device.walk([&](ConfigGMIOOp configOp){
      auto gmio = configOp.getGmio();
      auto attr = dyn_cast<ArrayAttr>(configOp->getAttr("col, chl"));
      auto colAttr = attr[0];
      int32_t colInt = dyn_cast<IntegerAttr>(colAttr).getInt();
      int32_t rowInt = 0;
      auto it = llvm::find_if(tileLocs, [&colInt, &rowInt](
                            const std::tuple<Value, int32_t, int32_t>& tileLoc){
        return std::get<1>(tileLoc) == colInt && std::get<2>(tileLoc) == rowInt;
      });
      if(it != tileLocs.end())
        memMap[gmio].first = std::get<0>(*it);
      else{
        auto tile = builder.create<TileOp>(loc, colInt, rowInt);
        memMap[gmio].first = tile.getResult();
        tileLocs.push_back(std::tuple(tile, colInt, rowInt));
      }
      configOp.erase();
    });
    device.walk([&](CallOp call){
      auto attr = dyn_cast<ArrayAttr>(call->getAttr("col, row"));
      auto colAttr = attr[0];
      auto rowAttr = attr[1];
      int32_t colInt = dyn_cast<IntegerAttr>(colAttr).getInt();
      int32_t rowInt = dyn_cast<IntegerAttr>(rowAttr).getInt();
      auto it = llvm::find_if(tileLocs, [&colInt, &rowInt](
                            const std::tuple<Value, int32_t, int32_t>& tileLoc){
        return std::get<1>(tileLoc) == colInt && std::get<2>(tileLoc) == rowInt;
      });
      if(it != tileLocs.end())
        coreMap[call] = std::get<0>(*it);
      else{
        auto tile = builder.create<TileOp>(loc, colInt, rowInt);
        coreMap[call] = tile.getResult();
        tileLocs.push_back(std::tuple(tile, colInt, rowInt));
      }
    });
  }

  bool ADFtoAIE(OpBuilder builder, DeviceOp& device, RuntimeSequenceOp seqOp,
                DenseMap<Value, std::pair<Value, std::string>>& memMap,
                DenseMap<CallOp, Value>& coreMap){
    MLIRContext *context = device->getContext();
    RewritePatternSet patterns(context);
    ConversionTarget target(*context);
    target.addIllegalOp<adf::ConnectOp>();
    target.addIllegalOp<adf::LaunchCellOp>();
    target.addIllegalOp<adf::DmaOp>();
    target.addIllegalOp<adf::DmaBroadcastOp>();
    target.addIllegalOp<adf::BufferOp>();
    target.addIllegalOp<adf::IOWriteOp>();
    target.addIllegalOp<adf::IOReadOp>();
    target.addIllegalOp<adf::CreateGraphIOOp>();
    patterns.add<ConnectConvert>(patterns.getContext(), seqOp, memMap);
    patterns.add<CellLaunchConvert>(patterns.getContext(), coreMap);
    patterns.add<DmaConvert>(patterns.getContext(), memMap);
    patterns.add<DmaBroadcastConvert>(patterns.getContext(), memMap);
    patterns.add<BufferConvert>(patterns.getContext());
    patterns.add<IOWriteConvert>(patterns.getContext());
    patterns.add<IOReadConvert>(patterns.getContext());
    patterns.add<GraphIOConvert>(patterns.getContext());
    target.addLegalOp<ObjectFifoAcquireOp>();
    target.addLegalOp<ObjectFifoSubviewAccessOp>();
    target.addLegalDialect<FuncDialect>();
    target.addLegalDialect<AffineDialect>();
    target.addLegalDialect<AIEDialect>();
    target.addLegalDialect<AIEXDialect>();
    if (failed(applyPartialConversion(device, target, std::move(patterns))))
      return false;
    return true;
  }

  bool ConvertToAIE(ModuleOp mod) {
    auto builder = OpBuilder(mod);
    DeviceOp device;
    EndOp endOp;
    RuntimeSequenceOp seqOp;
    FuncOp adfFunc;
    // This map records the corresponding IOs in the adf.cell and adf.func
    DenseMap<CreateGraphIOOp, CreateGraphIOOp> graphIOMap;
    createDevice(mod, builder, device, endOp);
    liftCell(builder, device, endOp, graphIOMap);
    splitCores(builder, device);
    if(!findADFFunc(device, adfFunc))
      return false;
    if(!moveStore(adfFunc))
      return false;
    if(!unrollDMA(builder, adfFunc))
      return false;
    processIO(builder, adfFunc);
    if(!splitIO(builder, adfFunc))
      return false;
    assignID(builder, adfFunc);
    createSeq(builder, adfFunc, endOp, seqOp);
    // Record tiles of shim, L1 and L2 mem, tile = memMap[buf/shim].first
    DenseMap<Value, std::pair<Value, std::string>> memMap;
    DenseMap<CallOp, Value> coreMap;
    recordMap(builder, device, memMap, coreMap);
    if(!ADFtoAIE(builder, device, seqOp, memMap, coreMap))
      return false;
    return true;
  }

};

} // namespace


namespace mlir {
namespace aries {
namespace adf {

std::unique_ptr<Pass> createADFConvertToAIEPass() {
  return std::make_unique<ADFConvertToAIE>();
}

} // namespace adf
} // namespace aries
} // namespace mlir