#map = affine_map<(d0) -> (d0 * 128 + 127)>
#map1 = affine_map<(d0) -> (d0 * 128)>
module {
  func.func @kernel_mttkrp0(%arg0: memref<2x8x16xf32, 2>, %arg1: memref<8x16xf32, 2>, %arg2: memref<16x16xf32, 2>) -> memref<2x16xf32, 2> attributes {adf.kernel, edge_kernel} {
    %cst = arith.constant 0.000000e+00 : f32
    %alloc = memref.alloc() : memref<2x16xf32, 2>
    affine.for %arg3 = 0 to 2 {
      affine.for %arg4 = 0 to 16 {
        affine.store %cst, %alloc[%arg3, %arg4] : memref<2x16xf32, 2>
      }
    }
    affine.for %arg3 = 0 to 2 {
      affine.for %arg4 = 0 to 16 {
        affine.store %cst, %alloc[%arg3, %arg4] : memref<2x16xf32, 2>
        affine.for %arg5 = 0 to 8 {
          affine.for %arg6 = 0 to 16 {
            %0 = affine.load %arg0[%arg3, %arg5, %arg6] : memref<2x8x16xf32, 2>
            %1 = affine.load %arg1[%arg5, %arg4] : memref<8x16xf32, 2>
            %2 = arith.mulf %0, %1 : f32
            %3 = affine.load %arg2[%arg6, %arg4] : memref<16x16xf32, 2>
            %4 = arith.mulf %2, %3 : f32
            %5 = affine.load %alloc[%arg3, %arg4] : memref<2x16xf32, 2>
            %6 = arith.addf %5, %4 : f32
            affine.store %6, %alloc[%arg3, %arg4] : memref<2x16xf32, 2>
          }
        }
      }
    }
    return %alloc : memref<2x16xf32, 2>
  }
  func.func @kernel_mttkrp(%arg0: memref<2x8x16xf32, 2>, %arg1: memref<8x16xf32, 2>, %arg2: memref<16x16xf32, 2>, %arg3: memref<2x16xf32, 2>) -> memref<2x16xf32, 2> attributes {adf.kernel} {
    %cst = arith.constant 0.000000e+00 : f32
    %alloc = memref.alloc() : memref<2x16xf32, 2>
    affine.for %arg4 = 0 to 2 {
      affine.for %arg5 = 0 to 16 {
        %0 = affine.load %arg3[%arg4, %arg5] : memref<2x16xf32, 2>
        affine.store %0, %alloc[%arg4, %arg5] : memref<2x16xf32, 2>
        affine.store %cst, %alloc[%arg4, %arg5] : memref<2x16xf32, 2>
        affine.for %arg6 = 0 to 8 {
          affine.for %arg7 = 0 to 16 {
            %1 = affine.load %arg0[%arg4, %arg6, %arg7] : memref<2x8x16xf32, 2>
            %2 = affine.load %arg1[%arg6, %arg5] : memref<8x16xf32, 2>
            %3 = arith.mulf %1, %2 : f32
            %4 = affine.load %arg2[%arg7, %arg5] : memref<16x16xf32, 2>
            %5 = arith.mulf %3, %4 : f32
            %6 = affine.load %alloc[%arg4, %arg5] : memref<2x16xf32, 2>
            %7 = arith.addf %6, %5 : f32
            affine.store %7, %alloc[%arg4, %arg5] : memref<2x16xf32, 2>
          }
        }
      }
    }
    return %alloc : memref<2x16xf32, 2>
  }
  func.func @adf_cell0(%arg0: !adf.plio<In, 128>, %arg1: !adf.plio<In, 128>, %arg2: !adf.plio<In, 128>, %arg3: !adf.plio<In, 128>, %arg4: !adf.plio<In, 128>, %arg5: !adf.plio<Out, 128>, %arg6: !adf.plio<In, 128>, %arg7: !adf.plio<In, 128>, %arg8: !adf.plio<In, 128>, %arg9: !adf.plio<Out, 128>, %arg10: !adf.plio<In, 128>, %arg11: !adf.plio<In, 128>, %arg12: !adf.plio<Out, 128>, %arg13: !adf.plio<Out, 128>) attributes {adf.cell, tripCount = [2 : index, 2 : index, 2 : index]} {
    %c6 = arith.constant 6 : index
    %c7 = arith.constant 7 : index
    %c2 = arith.constant 2 : index
    %c3 = arith.constant 3 : index
    %c4 = arith.constant 4 : index
    %c5 = arith.constant 5 : index
    %c25 = arith.constant 25 : index
    %c12288 = arith.constant 12288 : index
    %c4096 = arith.constant 4096 : index
    %c24576 = arith.constant 24576 : index
    %c16384 = arith.constant 16384 : index
    %c8192 = arith.constant 8192 : index
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c24 = arith.constant 24 : index
    adf.config.plio(%arg0, 250) {"col, chl" = [24 : index, 4 : index]} : <In, 128>
    adf.config.plio(%arg1, 250) {"col, chl" = [24 : index, 2 : index]} : <In, 128>
    adf.config.plio(%arg2, 250) {"col, chl" = [23 : index, 4 : index]} : <In, 128>
    adf.config.plio(%arg3, 250) {"col, chl" = [23 : index, 2 : index]} : <In, 128>
    adf.config.plio(%arg4, 250) {"col, chl" = [25 : index, 4 : index]} : <In, 128>
    adf.config.plio(%arg5, 250) {"col, chl" = [24 : index, 4 : index]} : <Out, 128>
    adf.config.plio(%arg6, 250) {"col, chl" = [25 : index, 2 : index]} : <In, 128>
    adf.config.plio(%arg7, 250) {"col, chl" = [22 : index, 4 : index]} : <In, 128>
    adf.config.plio(%arg8, 250) {"col, chl" = [22 : index, 2 : index]} : <In, 128>
    adf.config.plio(%arg9, 250) {"col, chl" = [24 : index, 2 : index]} : <Out, 128>
    adf.config.plio(%arg10, 250) {"col, chl" = [26 : index, 4 : index]} : <In, 128>
    adf.config.plio(%arg11, 250) {"col, chl" = [26 : index, 2 : index]} : <In, 128>
    adf.config.plio(%arg12, 250) {"col, chl" = [24 : index, 0 : index]} : <Out, 128>
    adf.config.plio(%arg13, 250) {"col, chl" = [23 : index, 4 : index]} : <Out, 128>
    %0 = adf.buffer.create @L1_L1_A() : memref<2x8x16xf32, 2>
    %1 = adf.buffer.create @L1_L1_B() : memref<8x16xf32, 2>
    %2 = adf.buffer.create @L1_L1_C() : memref<16x16xf32, 2>
    %3 = adf.buffer.create @L1_L1_A_1() : memref<2x8x16xf32, 2>
    %4 = adf.buffer.create @L1_L1_B_1() : memref<8x16xf32, 2>
    %5 = adf.buffer.create @L1_L1_C_1() : memref<16x16xf32, 2>
    %6 = adf.buffer.create @L1_L1_D_1() {accumulator} : memref<2x16xf32, 2>
    %7 = adf.buffer.create @L1_L1_A_2() : memref<2x8x16xf32, 2>
    %8 = adf.buffer.create @L1_L1_B_2() : memref<8x16xf32, 2>
    %9 = adf.buffer.create @L1_L1_C_2() : memref<16x16xf32, 2>
    %10 = adf.buffer.create @L1_L1_A_3() : memref<2x8x16xf32, 2>
    %11 = adf.buffer.create @L1_L1_B_3() : memref<8x16xf32, 2>
    %12 = adf.buffer.create @L1_L1_C_3() : memref<16x16xf32, 2>
    %13 = adf.buffer.create @L1_L1_D_3() {accumulator} : memref<2x16xf32, 2>
    %14 = adf.buffer.create @L1_L1_A_4() : memref<2x8x16xf32, 2>
    %15 = adf.buffer.create @L1_L1_B_4() : memref<8x16xf32, 2>
    %16 = adf.buffer.create @L1_L1_C_4() : memref<16x16xf32, 2>
    %17 = adf.buffer.create @L1_L1_A_5() : memref<2x8x16xf32, 2>
    %18 = adf.buffer.create @L1_L1_B_5() : memref<8x16xf32, 2>
    %19 = adf.buffer.create @L1_L1_C_5() : memref<16x16xf32, 2>
    %20 = adf.buffer.create @L1_L1_D_5() {accumulator} : memref<2x16xf32, 2>
    %21 = adf.buffer.create @L1_L1_A_6() : memref<2x8x16xf32, 2>
    %22 = adf.buffer.create @L1_L1_B_6() : memref<8x16xf32, 2>
    %23 = adf.buffer.create @L1_L1_C_6() : memref<16x16xf32, 2>
    %24 = adf.buffer.create @L1_L1_A_7() : memref<2x8x16xf32, 2>
    %25 = adf.buffer.create @L1_L1_B_7() : memref<8x16xf32, 2>
    %26 = adf.buffer.create @L1_L1_C_7() : memref<16x16xf32, 2>
    %27 = adf.buffer.create @L1_L1_D_7() {accumulator} : memref<2x16xf32, 2>
    adf.connect(%arg0, %0) : (!adf.plio<In, 128>, memref<2x8x16xf32, 2>)
    adf.connect(%arg0, %7) : (!adf.plio<In, 128>, memref<2x8x16xf32, 2>)
    adf.connect(%arg1, %1) : (!adf.plio<In, 128>, memref<8x16xf32, 2>)
    adf.connect(%arg1, %4) : (!adf.plio<In, 128>, memref<8x16xf32, 2>)
    adf.connect(%arg1, %15) : (!adf.plio<In, 128>, memref<8x16xf32, 2>)
    adf.connect(%arg1, %18) : (!adf.plio<In, 128>, memref<8x16xf32, 2>)
    adf.connect(%arg2, %2) : (!adf.plio<In, 128>, memref<16x16xf32, 2>)
    adf.connect(%arg2, %16) : (!adf.plio<In, 128>, memref<16x16xf32, 2>)
    %28 = call @kernel_mttkrp0(%0, %1, %2) {adf.kernel, "col, row" = [24 : index, 0 : index], ivs = [0 : index, 0 : index, 0 : index], kernel = 0 : index, kernel_mttkrp0 = 0 : index} : (memref<2x8x16xf32, 2>, memref<8x16xf32, 2>, memref<16x16xf32, 2>) -> memref<2x16xf32, 2>
    adf.buffer.location(%28, %c24, %c0, %c16384, %c24576) : (memref<2x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%1, %c24, %c0, %c0, %c8192) : (memref<8x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%2, %c24, %c1, %c16384, %c24576) : (memref<16x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%0, %c24, %c1, %c0, %c8192) : (memref<2x8x16xf32, 2>, index, index, index, index)
    adf.connect(%28, %6) : (memref<2x16xf32, 2>, memref<2x16xf32, 2>)
    adf.connect(%arg3, %3) : (!adf.plio<In, 128>, memref<2x8x16xf32, 2>)
    adf.connect(%arg3, %10) : (!adf.plio<In, 128>, memref<2x8x16xf32, 2>)
    adf.connect(%arg4, %5) : (!adf.plio<In, 128>, memref<16x16xf32, 2>)
    adf.connect(%arg4, %19) : (!adf.plio<In, 128>, memref<16x16xf32, 2>)
    %29 = call @kernel_mttkrp(%3, %4, %5, %6) {adf.kernel, "col, row" = [24 : index, 1 : index], ivs = [1 : index, 0 : index, 0 : index], kernel_mttkrp = 1 : index} : (memref<2x8x16xf32, 2>, memref<8x16xf32, 2>, memref<16x16xf32, 2>, memref<2x16xf32, 2>) -> memref<2x16xf32, 2>
    adf.buffer.location(%29, %c25, %c1, %c4096, %c12288) : (memref<2x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%4, %c25, %c1, %c16384, %c24576) : (memref<8x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%5, %c25, %c1, %c0, %c8192) : (memref<16x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%3, %c24, %c0, %c4096, %c12288) : (memref<2x8x16xf32, 2>, index, index, index, index)
    adf.connect(%29, %arg5) : (memref<2x16xf32, 2>, !adf.plio<Out, 128>)
    adf.connect(%arg6, %8) : (!adf.plio<In, 128>, memref<8x16xf32, 2>)
    adf.connect(%arg6, %11) : (!adf.plio<In, 128>, memref<8x16xf32, 2>)
    adf.connect(%arg6, %22) : (!adf.plio<In, 128>, memref<8x16xf32, 2>)
    adf.connect(%arg6, %25) : (!adf.plio<In, 128>, memref<8x16xf32, 2>)
    adf.connect(%arg7, %9) : (!adf.plio<In, 128>, memref<16x16xf32, 2>)
    adf.connect(%arg7, %23) : (!adf.plio<In, 128>, memref<16x16xf32, 2>)
    %30 = call @kernel_mttkrp0(%7, %8, %9) {adf.kernel, "col, row" = [24 : index, 4 : index], ivs = [0 : index, 1 : index, 0 : index], kernel = 0 : index, kernel_mttkrp0 = 2 : index} : (memref<2x8x16xf32, 2>, memref<8x16xf32, 2>, memref<16x16xf32, 2>) -> memref<2x16xf32, 2>
    adf.buffer.location(%30, %c24, %c4, %c16384, %c24576) : (memref<2x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%8, %c24, %c4, %c0, %c8192) : (memref<8x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%9, %c24, %c5, %c16384, %c24576) : (memref<16x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%7, %c24, %c5, %c0, %c8192) : (memref<2x8x16xf32, 2>, index, index, index, index)
    adf.connect(%30, %13) : (memref<2x16xf32, 2>, memref<2x16xf32, 2>)
    adf.connect(%arg8, %12) : (!adf.plio<In, 128>, memref<16x16xf32, 2>)
    adf.connect(%arg8, %26) : (!adf.plio<In, 128>, memref<16x16xf32, 2>)
    %31 = call @kernel_mttkrp(%10, %11, %12, %13) {adf.kernel, "col, row" = [24 : index, 5 : index], ivs = [1 : index, 1 : index, 0 : index], kernel_mttkrp = 3 : index} : (memref<2x8x16xf32, 2>, memref<8x16xf32, 2>, memref<16x16xf32, 2>, memref<2x16xf32, 2>) -> memref<2x16xf32, 2>
    adf.buffer.location(%31, %c25, %c5, %c4096, %c12288) : (memref<2x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%11, %c25, %c5, %c16384, %c24576) : (memref<8x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%12, %c25, %c5, %c0, %c8192) : (memref<16x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%10, %c24, %c4, %c4096, %c12288) : (memref<2x8x16xf32, 2>, index, index, index, index)
    adf.connect(%31, %arg9) : (memref<2x16xf32, 2>, !adf.plio<Out, 128>)
    adf.connect(%arg10, %14) : (!adf.plio<In, 128>, memref<2x8x16xf32, 2>)
    adf.connect(%arg10, %21) : (!adf.plio<In, 128>, memref<2x8x16xf32, 2>)
    %32 = call @kernel_mttkrp0(%14, %15, %16) {adf.kernel, "col, row" = [24 : index, 2 : index], ivs = [0 : index, 0 : index, 1 : index], kernel = 0 : index, kernel_mttkrp0 = 4 : index} : (memref<2x8x16xf32, 2>, memref<8x16xf32, 2>, memref<16x16xf32, 2>) -> memref<2x16xf32, 2>
    adf.buffer.location(%32, %c24, %c2, %c16384, %c24576) : (memref<2x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%15, %c24, %c2, %c0, %c8192) : (memref<8x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%16, %c24, %c3, %c16384, %c24576) : (memref<16x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%14, %c24, %c3, %c0, %c8192) : (memref<2x8x16xf32, 2>, index, index, index, index)
    adf.connect(%32, %20) : (memref<2x16xf32, 2>, memref<2x16xf32, 2>)
    adf.connect(%arg11, %17) : (!adf.plio<In, 128>, memref<2x8x16xf32, 2>)
    adf.connect(%arg11, %24) : (!adf.plio<In, 128>, memref<2x8x16xf32, 2>)
    %33 = call @kernel_mttkrp(%17, %18, %19, %20) {adf.kernel, "col, row" = [24 : index, 3 : index], ivs = [1 : index, 0 : index, 1 : index], kernel_mttkrp = 5 : index} : (memref<2x8x16xf32, 2>, memref<8x16xf32, 2>, memref<16x16xf32, 2>, memref<2x16xf32, 2>) -> memref<2x16xf32, 2>
    adf.buffer.location(%33, %c25, %c3, %c4096, %c12288) : (memref<2x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%18, %c25, %c3, %c16384, %c24576) : (memref<8x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%19, %c25, %c3, %c0, %c8192) : (memref<16x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%17, %c24, %c2, %c4096, %c12288) : (memref<2x8x16xf32, 2>, index, index, index, index)
    adf.connect(%33, %arg12) : (memref<2x16xf32, 2>, !adf.plio<Out, 128>)
    %34 = call @kernel_mttkrp0(%21, %22, %23) {adf.kernel, "col, row" = [24 : index, 6 : index], ivs = [0 : index, 1 : index, 1 : index], kernel = 0 : index, kernel_mttkrp0 = 6 : index} : (memref<2x8x16xf32, 2>, memref<8x16xf32, 2>, memref<16x16xf32, 2>) -> memref<2x16xf32, 2>
    adf.buffer.location(%34, %c24, %c6, %c16384, %c24576) : (memref<2x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%22, %c24, %c6, %c0, %c8192) : (memref<8x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%23, %c24, %c7, %c16384, %c24576) : (memref<16x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%21, %c24, %c7, %c0, %c8192) : (memref<2x8x16xf32, 2>, index, index, index, index)
    adf.connect(%34, %27) : (memref<2x16xf32, 2>, memref<2x16xf32, 2>)
    %35 = call @kernel_mttkrp(%24, %25, %26, %27) {adf.kernel, "col, row" = [24 : index, 7 : index], ivs = [1 : index, 1 : index, 1 : index], kernel_mttkrp = 7 : index} : (memref<2x8x16xf32, 2>, memref<8x16xf32, 2>, memref<16x16xf32, 2>, memref<2x16xf32, 2>) -> memref<2x16xf32, 2>
    adf.buffer.location(%35, %c25, %c7, %c4096, %c12288) : (memref<2x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%25, %c25, %c7, %c16384, %c24576) : (memref<8x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%26, %c25, %c7, %c0, %c8192) : (memref<16x16xf32, 2>, index, index, index, index)
    adf.buffer.location(%24, %c24, %c6, %c4096, %c12288) : (memref<2x8x16xf32, 2>, index, index, index, index)
    adf.connect(%35, %arg13) : (memref<2x16xf32, 2>, !adf.plio<Out, 128>)
    return
  }
  func.func @send3_0(%arg0: memref<1xi128, "stream">, %arg1: memref<4x16x8xi128, 1>, %arg2: i1) attributes {adf.pl, inline = false} {
    scf.if %arg2 {
      affine.for %arg3 = 0 to 2 {
        affine.for %arg4 = 0 to 2 {
          affine.for %arg5 = 0 to 2 {
            affine.for %arg6 = 0 to 8 {
              affine.for %arg7 = 0 to 2 {
                affine.for %arg8 = 0 to 4 {
                  %0 = affine.load %arg0[0] : memref<1xi128, "stream">
                  affine.store %0, %arg1[%arg5 + %arg3 * 2, %arg6 + %arg4 * 8, %arg8 + %arg7 * 4] : memref<4x16x8xi128, 1>
                } {pipeline_ii = 1 : index}
              }
            }
          }
        }
      }
    }
    return
  }
  func.func @send3_1(%arg0: memref<4x16x8xi128, 1>, %arg1: memref<1xi128, "plio">, %arg2: i1) attributes {adf.pl, inline = false} {
    scf.if %arg2 {
      affine.for %arg3 = 0 to 2 {
        affine.for %arg4 = 0 to 2 {
          affine.for %arg5 = 0 to 2 {
            affine.for %arg6 = 0 to 2 {
              affine.for %arg7 = 0 to 2 {
                affine.for %arg8 = 0 to 8 {
                  affine.for %arg9 = 0 to 4 {
                    %0 = affine.load %arg0[%arg7 + %arg3 * 2, %arg8 + %arg5 * 8, %arg9 + %arg6 * 4] : memref<4x16x8xi128, 1>
                    affine.store %0, %arg1[0] : memref<1xi128, "plio">
                  } {pipeline_ii = 1 : index}
                }
              }
            }
          }
        }
      }
    }
    return
  }
  func.func @send3(%arg0: memref<1xi128, "plio">, %arg1: memref<1xi128, "stream">) attributes {adf.pl, inline = false, send, template} {
    %c8 = arith.constant 8 : index
    %c4 = arith.constant 4 : index
    %true = arith.constant true
    %c0 = arith.constant 0 : index
    %c2 = arith.constant 2 : index
    %alloc = memref.alloc() {buffer_type = "uram_t2p"} : memref<4x16x8xi128, 1>
    %alloc_0 = memref.alloc() {buffer_type = "uram_t2p"} : memref<4x16x8xi128, 1>
    affine.for %arg2 = 0 to 1 {
      affine.for %arg3 = 0 to 2 {
        affine.for %arg4 = 0 to 2 {
          affine.for %arg5 = 0 to 2 {
            %0 = arith.muli %arg4, %c2 : index
            %1 = arith.addi %arg5, %0 : index
            %2 = arith.muli %arg3, %c4 : index
            %3 = arith.addi %1, %2 : index
            %4 = arith.muli %arg2, %c8 : index
            %5 = arith.addi %3, %4 : index
            %6 = arith.remsi %5, %c2 : index
            %7 = arith.cmpi eq, %6, %c0 : index
            %8 = arith.cmpi ne, %5, %c0 : index
            scf.if %7 {
              func.call @send3_0(%arg1, %alloc, %true) : (memref<1xi128, "stream">, memref<4x16x8xi128, 1>, i1) -> ()
              func.call @send3_1(%alloc_0, %arg0, %8) : (memref<4x16x8xi128, 1>, memref<1xi128, "plio">, i1) -> ()
            } else {
              func.call @send3_0(%arg1, %alloc_0, %true) : (memref<1xi128, "stream">, memref<4x16x8xi128, 1>, i1) -> ()
              func.call @send3_1(%alloc, %arg0, %8) : (memref<4x16x8xi128, 1>, memref<1xi128, "plio">, i1) -> ()
            }
          } {Array_Partition, reduction = 1 : i64}
        } {reduction = 0 : i64}
      }
    }
    call @send3_1(%alloc_0, %arg0, %true) : (memref<4x16x8xi128, 1>, memref<1xi128, "plio">, i1) -> ()
    return
  }
  func.func @send3_top(%arg0: memref<1xi128, "plio">, %arg1: memref<1xi128, "stream">, %arg2: memref<1xi128, "plio">, %arg3: memref<1xi128, "stream">, %arg4: memref<1xi128, "plio">, %arg5: memref<1xi128, "stream">, %arg6: memref<1xi128, "plio">, %arg7: memref<1xi128, "stream">) attributes {adf.pl, inline = false} {
    call @send3(%arg0, %arg1) {template = 0 : index} : (memref<1xi128, "plio">, memref<1xi128, "stream">) -> ()
    call @send3(%arg2, %arg3) {template = 1 : index} : (memref<1xi128, "plio">, memref<1xi128, "stream">) -> ()
    call @send3(%arg4, %arg5) {template = 2 : index} : (memref<1xi128, "plio">, memref<1xi128, "stream">) -> ()
    call @send3(%arg6, %arg7) {template = 3 : index} : (memref<1xi128, "plio">, memref<1xi128, "stream">) -> ()
    return
  }
  func.func @load2(%arg0: memref<128x8xi512>, %arg1: memref<1xi512, "stream1">, %arg2: memref<1xi512, "stream1">, %arg3: memref<1xi512, "stream1">, %arg4: memref<1xi512, "stream1">) attributes {adf.pl, inline = false, load, mem_idx = [0 : i32], mem_type = [f32], template} {
    %c1 = arith.constant 1 : index
    affine.for %arg5 = 0 to 1 {
      affine.for %arg6 = 0 to 2 {
        affine.for %arg7 = 0 to 2 {
          affine.for %arg8 = 0 to 2 {
            affine.for %arg9 = 0 to 2 {
              affine.for %arg10 = 0 to 16 {
                affine.for %arg11 = 0 to 2 {
                  affine.for %arg12 = 0 to 2 {
                    %0 = affine.load %arg0[%arg10 + %arg9 * 32 + %arg8 * 64, %arg12 + %arg11 * 2 + %arg6 * 4] : memref<128x8xi512>
                    %1 = arith.cmpi slt, %arg12, %c1 : index
                    scf.if %1 {
                      affine.store %0, %arg4[0] : memref<1xi512, "stream1">
                    } else {
                      affine.store %0, %arg2[0] : memref<1xi512, "stream1">
                    }
                  } {pipeline_ii = 1 : index}
                }
              }
            } {merge}
            affine.for %arg9 = 0 to 2 {
              affine.for %arg10 = 0 to 16 {
                affine.for %arg11 = 0 to 2 {
                  affine.for %arg12 = 0 to 2 {
                    %0 = affine.load %arg0[%arg10 + %arg9 * 32 + %arg8 * 64 + 16, %arg12 + %arg11 * 2 + %arg6 * 4] : memref<128x8xi512>
                    %1 = arith.cmpi slt, %arg12, %c1 : index
                    scf.if %1 {
                      affine.store %0, %arg3[0] : memref<1xi512, "stream1">
                    } else {
                      affine.store %0, %arg1[0] : memref<1xi512, "stream1">
                    }
                  } {pipeline_ii = 1 : index}
                }
              }
            } {merge}
          } {Array_Partition, reduction = 1 : i64}
        } {reduction = 0 : i64}
      }
    }
    return
  }
  func.func @load2_top(%arg0: memref<128x8xi512>, %arg1: memref<1xi512, "stream1">, %arg2: memref<1xi512, "stream1">, %arg3: memref<1xi512, "stream1">, %arg4: memref<1xi512, "stream1">) attributes {adf.pl, inline = false} {
    call @load2(%arg0, %arg1, %arg2, %arg3, %arg4) {template = 0 : index} : (memref<128x8xi512>, memref<1xi512, "stream1">, memref<1xi512, "stream1">, memref<1xi512, "stream1">, memref<1xi512, "stream1">) -> ()
    return
  }
  func.func @load2_3(%arg0: memref<1xi512, "stream1">, %arg1: memref<1xi128, "stream">) attributes {adf.pl, inline = false, load, template} {
    affine.for %arg2 = 0 to 1 {
      affine.for %arg3 = 0 to 2 {
        affine.for %arg4 = 0 to 2 {
          affine.for %arg5 = 0 to 2 {
            affine.for %arg6 = 0 to 2 {
              affine.for %arg7 = 0 to 16 {
                affine.for %arg8 = 0 to 2 {
                  affine.for %arg9 = 0 to 1 {
                    %0 = affine.load %arg0[0] : memref<1xi512, "stream1">
                    affine.for %arg10 = 0 to 4 {
                      %1 = affine.apply #map(%arg10)
                      %2 = affine.apply #map1(%arg10)
                      %3 = adf.get_slice(%0 : i512, %1, %2) -> i128
                      affine.store %3, %arg1[0] : memref<1xi128, "stream">
                    } {pipeline_ii = 1 : index}
                  } {pipeline_ii = 4 : index}
                }
              }
            }
          }
        }
      }
    }
    return
  }
  func.func @load2_3_top(%arg0: memref<1xi512, "stream1">, %arg1: memref<1xi128, "stream">, %arg2: memref<1xi512, "stream1">, %arg3: memref<1xi128, "stream">, %arg4: memref<1xi512, "stream1">, %arg5: memref<1xi128, "stream">, %arg6: memref<1xi512, "stream1">, %arg7: memref<1xi128, "stream">) attributes {adf.pl, inline = false} {
    call @load2_3(%arg0, %arg1) {template = 0 : index} : (memref<1xi512, "stream1">, memref<1xi128, "stream">) -> ()
    call @load2_3(%arg2, %arg3) {template = 1 : index} : (memref<1xi512, "stream1">, memref<1xi128, "stream">) -> ()
    call @load2_3(%arg4, %arg5) {template = 2 : index} : (memref<1xi512, "stream1">, memref<1xi128, "stream">) -> ()
    call @load2_3(%arg6, %arg7) {template = 3 : index} : (memref<1xi512, "stream1">, memref<1xi128, "stream">) -> ()
    return
  }
  func.func @send5_0(%arg0: memref<1xi128, "stream">, %arg1: memref<16x8xi128, 1>, %arg2: i1) attributes {adf.pl, inline = false} {
    scf.if %arg2 {
      affine.for %arg3 = 0 to 2 {
        affine.for %arg4 = 0 to 8 {
          affine.for %arg5 = 0 to 2 {
            affine.for %arg6 = 0 to 4 {
              %0 = affine.load %arg0[0] : memref<1xi128, "stream">
              affine.store %0, %arg1[%arg4 + %arg3 * 8, %arg6 + %arg5 * 4] : memref<16x8xi128, 1>
            } {pipeline_ii = 1 : index}
          }
        }
      }
    }
    return
  }
  func.func @send5_1(%arg0: memref<16x8xi128, 1>, %arg1: memref<1xi128, "plio">, %arg2: i1) attributes {adf.pl, inline = false} {
    scf.if %arg2 {
      affine.for %arg3 = 0 to 2 {
        affine.for %arg4 = 0 to 2 {
          affine.for %arg5 = 0 to 2 {
            affine.for %arg6 = 0 to 2 {
              affine.for %arg7 = 0 to 8 {
                affine.for %arg8 = 0 to 4 {
                  %0 = affine.load %arg0[%arg7 + %arg5 * 8, %arg8 + %arg4 * 4] : memref<16x8xi128, 1>
                  affine.store %0, %arg1[0] : memref<1xi128, "plio">
                } {pipeline_ii = 1 : index}
              }
            }
          }
        }
      }
    }
    return
  }
  func.func @send5(%arg0: memref<1xi128, "plio">, %arg1: memref<1xi128, "stream">) attributes {adf.pl, inline = false, send, template} {
    %c8 = arith.constant 8 : index
    %c4 = arith.constant 4 : index
    %true = arith.constant true
    %c0 = arith.constant 0 : index
    %c2 = arith.constant 2 : index
    %alloc = memref.alloc() {buffer_type = "bram_s2p"} : memref<16x8xi128, 1>
    %alloc_0 = memref.alloc() {buffer_type = "bram_s2p"} : memref<16x8xi128, 1>
    affine.for %arg2 = 0 to 1 {
      affine.for %arg3 = 0 to 2 {
        affine.for %arg4 = 0 to 2 {
          affine.for %arg5 = 0 to 2 {
            %0 = arith.muli %arg4, %c2 : index
            %1 = arith.addi %arg5, %0 : index
            %2 = arith.muli %arg3, %c4 : index
            %3 = arith.addi %1, %2 : index
            %4 = arith.muli %arg2, %c8 : index
            %5 = arith.addi %3, %4 : index
            %6 = arith.remsi %5, %c2 : index
            %7 = arith.cmpi eq, %6, %c0 : index
            %8 = arith.cmpi ne, %5, %c0 : index
            scf.if %7 {
              func.call @send5_0(%arg1, %alloc, %true) : (memref<1xi128, "stream">, memref<16x8xi128, 1>, i1) -> ()
              func.call @send5_1(%alloc_0, %arg0, %8) : (memref<16x8xi128, 1>, memref<1xi128, "plio">, i1) -> ()
            } else {
              func.call @send5_0(%arg1, %alloc_0, %true) : (memref<1xi128, "stream">, memref<16x8xi128, 1>, i1) -> ()
              func.call @send5_1(%alloc, %arg0, %8) : (memref<16x8xi128, 1>, memref<1xi128, "plio">, i1) -> ()
            }
          } {Array_Partition, reduction = 1 : i64}
        } {reduction = 0 : i64}
      }
    }
    call @send5_1(%alloc_0, %arg0, %true) : (memref<16x8xi128, 1>, memref<1xi128, "plio">, i1) -> ()
    return
  }
  func.func @send5_top(%arg0: memref<1xi128, "plio">, %arg1: memref<1xi128, "stream">, %arg2: memref<1xi128, "plio">, %arg3: memref<1xi128, "stream">) attributes {adf.pl, inline = false} {
    call @send5(%arg0, %arg1) {template = 0 : index} : (memref<1xi128, "plio">, memref<1xi128, "stream">) -> ()
    call @send5(%arg2, %arg3) {template = 1 : index} : (memref<1xi128, "plio">, memref<1xi128, "stream">) -> ()
    return
  }
  func.func @receive2(%arg0: memref<1xi128, "plio">, %arg1: memref<1xi128, "stream">) attributes {adf.pl, inline = false, receive, template} {
    %c127 = arith.constant 127 : index
    %c96 = arith.constant 96 : index
    %c31 = arith.constant 31 : index
    %c63 = arith.constant 63 : index
    %c0 = arith.constant 0 : index
    %c32 = arith.constant 32 : index
    %c0_i128 = arith.constant 0 : i128
    %c64 = arith.constant 64 : index
    %c95 = arith.constant 95 : index
    %alloc = memref.alloc() {buffer_type = "bram_s2p"} : memref<4x8xi128, 1>
    affine.for %arg2 = 0 to 4 {
      affine.for %arg3 = 0 to 8 {
        affine.store %c0_i128, %alloc[%arg2, %arg3] : memref<4x8xi128, 1>
      } {pipeline_ii = 1 : index}
    }
    affine.for %arg2 = 0 to 1 {
      affine.for %arg3 = 0 to 2 {
        affine.for %arg4 = 0 to 2 {
          affine.for %arg5 = 0 to 2 {
            affine.for %arg6 = 0 to 2 {
              affine.for %arg7 = 0 to 2 {
                affine.for %arg8 = 0 to 2 {
                  affine.for %arg9 = 0 to 2 {
                    affine.for %arg10 = 0 to 2 {
                      affine.for %arg11 = 0 to 4 {
                        %0 = affine.load %arg0[0] : memref<1xi128, "plio">
                        %1 = affine.load %alloc[%arg10 + %arg6 * 2, %arg11 + %arg7 * 4] : memref<4x8xi128, 1>
                        %2 = adf.int_to_apint(%0) : (i128) -> i128
                        %3 = adf.int_to_apint(%1) : (i128) -> i128
                        %4 = adf.int_to_apint(%c0_i128) : (i128) -> i128
                        %5 = adf.get_slice(%2 : i128, %c31, %c0) -> i32
                        %6 = adf.get_slice(%3 : i128, %c31, %c0) -> i32
                        %7 = arith.bitcast %5 : i32 to f32
                        %8 = arith.bitcast %6 : i32 to f32
                        %9 = arith.addf %7, %8 : f32
                        %10 = arith.bitcast %9 : f32 to i32
                        adf.set_slice(%4 : i128, %c31, %c0, %10 : i32)
                        %11 = adf.get_slice(%2 : i128, %c63, %c32) -> i32
                        %12 = adf.get_slice(%3 : i128, %c63, %c32) -> i32
                        %13 = arith.bitcast %11 : i32 to f32
                        %14 = arith.bitcast %12 : i32 to f32
                        %15 = arith.addf %13, %14 : f32
                        %16 = arith.bitcast %15 : f32 to i32
                        adf.set_slice(%4 : i128, %c63, %c32, %16 : i32)
                        %17 = adf.get_slice(%2 : i128, %c95, %c64) -> i32
                        %18 = adf.get_slice(%3 : i128, %c95, %c64) -> i32
                        %19 = arith.bitcast %17 : i32 to f32
                        %20 = arith.bitcast %18 : i32 to f32
                        %21 = arith.addf %19, %20 : f32
                        %22 = arith.bitcast %21 : f32 to i32
                        adf.set_slice(%4 : i128, %c95, %c64, %22 : i32)
                        %23 = adf.get_slice(%2 : i128, %c127, %c96) -> i32
                        %24 = adf.get_slice(%3 : i128, %c127, %c96) -> i32
                        %25 = arith.bitcast %23 : i32 to f32
                        %26 = arith.bitcast %24 : i32 to f32
                        %27 = arith.addf %25, %26 : f32
                        %28 = arith.bitcast %27 : f32 to i32
                        adf.set_slice(%4 : i128, %c127, %c96, %28 : i32)
                        %29 = adf.apint_to_int(%4) : (i128) -> i128
                        affine.store %29, %alloc[%arg10 + %arg6 * 2, %arg11 + %arg7 * 4] : memref<4x8xi128, 1>
                      } {pipeline_ii = 1 : index}
                    }
                  }
                }
              }
            }
          } {Array_Partition, reduction = 1 : i64}
        } {reduction = 0 : i64}
        affine.for %arg4 = 0 to 2 {
          affine.for %arg5 = 0 to 2 {
            affine.for %arg6 = 0 to 2 {
              affine.for %arg7 = 0 to 4 {
                %0 = affine.load %alloc[%arg5 + %arg4 * 2, %arg7 + %arg6 * 4] : memref<4x8xi128, 1>
                affine.store %0, %arg1[0] : memref<1xi128, "stream">
                affine.store %c0_i128, %alloc[%arg5 + %arg4 * 2, %arg7 + %arg6 * 4] : memref<4x8xi128, 1>
              } {pipeline_ii = 1 : index}
            }
          }
        }
      }
    }
    return
  }
  func.func @receive2_top(%arg0: memref<1xi128, "plio">, %arg1: memref<1xi128, "stream">, %arg2: memref<1xi128, "plio">, %arg3: memref<1xi128, "stream">, %arg4: memref<1xi128, "plio">, %arg5: memref<1xi128, "stream">, %arg6: memref<1xi128, "plio">, %arg7: memref<1xi128, "stream">) attributes {adf.pl, inline = false} {
    call @receive2(%arg0, %arg1) {template = 0 : index} : (memref<1xi128, "plio">, memref<1xi128, "stream">) -> ()
    call @receive2(%arg2, %arg3) {template = 1 : index} : (memref<1xi128, "plio">, memref<1xi128, "stream">) -> ()
    call @receive2(%arg4, %arg5) {template = 2 : index} : (memref<1xi128, "plio">, memref<1xi128, "stream">) -> ()
    call @receive2(%arg6, %arg7) {template = 3 : index} : (memref<1xi128, "plio">, memref<1xi128, "stream">) -> ()
    return
  }
  func.func @send6_0(%arg0: memref<1xi128, "stream">, %arg1: memref<32x8xi128, 1>, %arg2: i1) attributes {adf.pl, inline = false} {
    scf.if %arg2 {
      affine.for %arg3 = 0 to 2 {
        affine.for %arg4 = 0 to 16 {
          affine.for %arg5 = 0 to 2 {
            affine.for %arg6 = 0 to 4 {
              %0 = affine.load %arg0[0] : memref<1xi128, "stream">
              affine.store %0, %arg1[%arg4 + %arg3 * 16, %arg6 + %arg5 * 4] : memref<32x8xi128, 1>
            } {pipeline_ii = 1 : index}
          }
        }
      }
    }
    return
  }
  func.func @send6_1(%arg0: memref<32x8xi128, 1>, %arg1: memref<1xi128, "plio">, %arg2: i1) attributes {adf.pl, inline = false} {
    scf.if %arg2 {
      affine.for %arg3 = 0 to 2 {
        affine.for %arg4 = 0 to 2 {
          affine.for %arg5 = 0 to 2 {
            affine.for %arg6 = 0 to 2 {
              affine.for %arg7 = 0 to 16 {
                affine.for %arg8 = 0 to 4 {
                  %0 = affine.load %arg0[%arg7 + %arg6 * 16, %arg8 + %arg4 * 4] : memref<32x8xi128, 1>
                  affine.store %0, %arg1[0] : memref<1xi128, "plio">
                } {pipeline_ii = 1 : index}
              }
            }
          }
        }
      }
    }
    return
  }
  func.func @send6(%arg0: memref<1xi128, "plio">, %arg1: memref<1xi128, "stream">) attributes {adf.pl, inline = false, send, template} {
    %c8 = arith.constant 8 : index
    %c4 = arith.constant 4 : index
    %true = arith.constant true
    %c0 = arith.constant 0 : index
    %c2 = arith.constant 2 : index
    %alloc = memref.alloc() {buffer_type = "uram_t2p"} : memref<32x8xi128, 1>
    %alloc_0 = memref.alloc() {buffer_type = "uram_t2p"} : memref<32x8xi128, 1>
    affine.for %arg2 = 0 to 1 {
      affine.for %arg3 = 0 to 2 {
        affine.for %arg4 = 0 to 2 {
          affine.for %arg5 = 0 to 2 {
            %0 = arith.muli %arg4, %c2 : index
            %1 = arith.addi %arg5, %0 : index
            %2 = arith.muli %arg3, %c4 : index
            %3 = arith.addi %1, %2 : index
            %4 = arith.muli %arg2, %c8 : index
            %5 = arith.addi %3, %4 : index
            %6 = arith.remsi %5, %c2 : index
            %7 = arith.cmpi eq, %6, %c0 : index
            %8 = arith.cmpi ne, %5, %c0 : index
            scf.if %7 {
              func.call @send6_0(%arg1, %alloc, %true) : (memref<1xi128, "stream">, memref<32x8xi128, 1>, i1) -> ()
              func.call @send6_1(%alloc_0, %arg0, %8) : (memref<32x8xi128, 1>, memref<1xi128, "plio">, i1) -> ()
            } else {
              func.call @send6_0(%arg1, %alloc_0, %true) : (memref<1xi128, "stream">, memref<32x8xi128, 1>, i1) -> ()
              func.call @send6_1(%alloc, %arg0, %8) : (memref<32x8xi128, 1>, memref<1xi128, "plio">, i1) -> ()
            }
          } {Array_Partition, reduction = 1 : i64}
        } {reduction = 0 : i64}
      }
    }
    call @send6_1(%alloc_0, %arg0, %true) : (memref<32x8xi128, 1>, memref<1xi128, "plio">, i1) -> ()
    return
  }
  func.func @send6_top(%arg0: memref<1xi128, "plio">, %arg1: memref<1xi128, "stream">, %arg2: memref<1xi128, "plio">, %arg3: memref<1xi128, "stream">, %arg4: memref<1xi128, "plio">, %arg5: memref<1xi128, "stream">, %arg6: memref<1xi128, "plio">, %arg7: memref<1xi128, "stream">) attributes {adf.pl, inline = false} {
    call @send6(%arg0, %arg1) {template = 0 : index} : (memref<1xi128, "plio">, memref<1xi128, "stream">) -> ()
    call @send6(%arg2, %arg3) {template = 1 : index} : (memref<1xi128, "plio">, memref<1xi128, "stream">) -> ()
    call @send6(%arg4, %arg5) {template = 2 : index} : (memref<1xi128, "plio">, memref<1xi128, "stream">) -> ()
    call @send6(%arg6, %arg7) {template = 3 : index} : (memref<1xi128, "plio">, memref<1xi128, "stream">) -> ()
    return
  }
  func.func @store0_0(%arg0: memref<1xi128, "stream">, %arg1: memref<1xi512, "stream1">) attributes {adf.pl, inline = false, store, template} {
    %c0_i512 = arith.constant 0 : i512
    affine.for %arg2 = 0 to 1 {
      affine.for %arg3 = 0 to 2 {
        affine.for %arg4 = 0 to 2 {
          affine.for %arg5 = 0 to 2 {
            affine.for %arg6 = 0 to 2 {
              affine.for %arg7 = 0 to 1 {
                %0 = adf.int_to_apint(%c0_i512) : (i512) -> i512
                affine.for %arg8 = 0 to 4 {
                  %1 = affine.load %arg0[0] : memref<1xi128, "stream">
                  %2 = affine.apply #map(%arg8)
                  %3 = affine.apply #map1(%arg8)
                  adf.set_slice(%0 : i512, %2, %3, %1 : i128)
                } {pipeline_ii = 1 : index}
                affine.store %0, %arg1[0] : memref<1xi512, "stream1">
              } {pipeline_ii = 4 : index}
            }
          }
        }
      }
    }
    return
  }
  func.func @store0_0_top(%arg0: memref<1xi128, "stream">, %arg1: memref<1xi512, "stream1">, %arg2: memref<1xi128, "stream">, %arg3: memref<1xi512, "stream1">, %arg4: memref<1xi128, "stream">, %arg5: memref<1xi512, "stream1">, %arg6: memref<1xi128, "stream">, %arg7: memref<1xi512, "stream1">) attributes {adf.pl, inline = false} {
    call @store0_0(%arg0, %arg1) {template = 0 : index} : (memref<1xi128, "stream">, memref<1xi512, "stream1">) -> ()
    call @store0_0(%arg2, %arg3) {template = 1 : index} : (memref<1xi128, "stream">, memref<1xi512, "stream1">) -> ()
    call @store0_0(%arg4, %arg5) {template = 2 : index} : (memref<1xi128, "stream">, memref<1xi512, "stream1">) -> ()
    call @store0_0(%arg6, %arg7) {template = 3 : index} : (memref<1xi128, "stream">, memref<1xi512, "stream1">) -> ()
    return
  }
  func.func @store0(%arg0: memref<8x8xi512>, %arg1: memref<1xi512, "stream1">, %arg2: memref<1xi512, "stream1">, %arg3: memref<1xi512, "stream1">, %arg4: memref<1xi512, "stream1">) attributes {adf.pl, inline = false, mem_idx = [0 : i32], mem_type = [f32], store, template} {
    %c1 = arith.constant 1 : index
    affine.for %arg5 = 0 to 1 {
      affine.for %arg6 = 0 to 2 {
        affine.for %arg7 = 0 to 2 {
          affine.for %arg8 = 0 to 2 {
            affine.for %arg9 = 0 to 2 {
              affine.for %arg10 = 0 to 2 {
                %0 = arith.cmpi slt, %arg10, %c1 : index
                %1 = scf.if %0 -> (i512) {
                  %2 = affine.load %arg1[0] : memref<1xi512, "stream1">
                  scf.yield %2 : i512
                } else {
                  %2 = affine.load %arg3[0] : memref<1xi512, "stream1">
                  scf.yield %2 : i512
                }
                affine.store %1, %arg0[%arg8 + %arg7 * 4 + %arg5 * 8, %arg10 + %arg9 * 2 + %arg6 * 4] : memref<8x8xi512>
              } {pipeline_ii = 1 : index}
            }
          }
        } {merge}
        affine.for %arg7 = 0 to 2 {
          affine.for %arg8 = 0 to 2 {
            affine.for %arg9 = 0 to 2 {
              affine.for %arg10 = 0 to 2 {
                %0 = arith.cmpi slt, %arg10, %c1 : index
                %1 = scf.if %0 -> (i512) {
                  %2 = affine.load %arg2[0] : memref<1xi512, "stream1">
                  scf.yield %2 : i512
                } else {
                  %2 = affine.load %arg4[0] : memref<1xi512, "stream1">
                  scf.yield %2 : i512
                }
                affine.store %1, %arg0[%arg8 + %arg7 * 4 + %arg5 * 8 + 2, %arg10 + %arg9 * 2 + %arg6 * 4] : memref<8x8xi512>
              } {pipeline_ii = 1 : index}
            }
          }
        } {merge}
      }
    }
    return
  }
  func.func @store0_top(%arg0: memref<8x8xi512>, %arg1: memref<1xi512, "stream1">, %arg2: memref<1xi512, "stream1">, %arg3: memref<1xi512, "stream1">, %arg4: memref<1xi512, "stream1">) attributes {adf.pl, inline = false} {
    call @store0(%arg0, %arg1, %arg2, %arg3, %arg4) {template = 0 : index} : (memref<8x8xi512>, memref<1xi512, "stream1">, memref<1xi512, "stream1">, memref<1xi512, "stream1">, memref<1xi512, "stream1">) -> ()
    return
  }
  func.func @load0(%arg0: memref<8x32x8xi512>, %arg1: memref<1xi512, "stream1">, %arg2: memref<1xi512, "stream1">, %arg3: memref<1xi512, "stream1">, %arg4: memref<1xi512, "stream1">) attributes {adf.pl, inline = false, load, mem_idx = [0 : i32], mem_type = [f32], template} {
    %c1 = arith.constant 1 : index
    affine.for %arg5 = 0 to 1 {
      affine.for %arg6 = 0 to 2 {
        affine.for %arg7 = 0 to 2 {
          affine.for %arg8 = 0 to 2 {
            affine.for %arg9 = 0 to 2 {
              affine.for %arg10 = 0 to 2 {
                affine.for %arg11 = 0 to 2 {
                  affine.for %arg12 = 0 to 8 {
                    affine.for %arg13 = 0 to 2 {
                      affine.for %arg14 = 0 to 2 {
                        %0 = affine.load %arg0[%arg11 + %arg9 * 4 + %arg5 * 8, %arg12 + %arg10 * 8 + %arg7 * 16, %arg14 + %arg13 * 2 + %arg8 * 4] : memref<8x32x8xi512>
                        %1 = arith.cmpi slt, %arg14, %c1 : index
                        scf.if %1 {
                          affine.store %0, %arg2[0] : memref<1xi512, "stream1">
                        } else {
                          affine.store %0, %arg3[0] : memref<1xi512, "stream1">
                        }
                      } {pipeline_ii = 1 : index}
                    }
                  }
                }
              }
            } {merge}
            affine.for %arg9 = 0 to 2 {
              affine.for %arg10 = 0 to 2 {
                affine.for %arg11 = 0 to 2 {
                  affine.for %arg12 = 0 to 8 {
                    affine.for %arg13 = 0 to 2 {
                      affine.for %arg14 = 0 to 2 {
                        %0 = affine.load %arg0[%arg11 + %arg9 * 4 + %arg5 * 8 + 2, %arg12 + %arg10 * 8 + %arg7 * 16, %arg14 + %arg13 * 2 + %arg8 * 4] : memref<8x32x8xi512>
                        %1 = arith.cmpi slt, %arg14, %c1 : index
                        scf.if %1 {
                          affine.store %0, %arg4[0] : memref<1xi512, "stream1">
                        } else {
                          affine.store %0, %arg1[0] : memref<1xi512, "stream1">
                        }
                      } {pipeline_ii = 1 : index}
                    }
                  }
                }
              }
            } {merge}
          } {Array_Partition, reduction = 1 : i64}
        } {reduction = 0 : i64}
      }
    }
    return
  }
  func.func @load0_top(%arg0: memref<8x32x8xi512>, %arg1: memref<1xi512, "stream1">, %arg2: memref<1xi512, "stream1">, %arg3: memref<1xi512, "stream1">, %arg4: memref<1xi512, "stream1">) attributes {adf.pl, inline = false} {
    call @load0(%arg0, %arg1, %arg2, %arg3, %arg4) {template = 0 : index} : (memref<8x32x8xi512>, memref<1xi512, "stream1">, memref<1xi512, "stream1">, memref<1xi512, "stream1">, memref<1xi512, "stream1">) -> ()
    return
  }
  func.func @load0_3(%arg0: memref<1xi512, "stream1">, %arg1: memref<1xi128, "stream">) attributes {adf.pl, inline = false, load, template} {
    affine.for %arg2 = 0 to 1 {
      affine.for %arg3 = 0 to 2 {
        affine.for %arg4 = 0 to 2 {
          affine.for %arg5 = 0 to 2 {
            affine.for %arg6 = 0 to 2 {
              affine.for %arg7 = 0 to 2 {
                affine.for %arg8 = 0 to 2 {
                  affine.for %arg9 = 0 to 8 {
                    affine.for %arg10 = 0 to 2 {
                      affine.for %arg11 = 0 to 1 {
                        %0 = affine.load %arg0[0] : memref<1xi512, "stream1">
                        affine.for %arg12 = 0 to 4 {
                          %1 = affine.apply #map(%arg12)
                          %2 = affine.apply #map1(%arg12)
                          %3 = adf.get_slice(%0 : i512, %1, %2) -> i128
                          affine.store %3, %arg1[0] : memref<1xi128, "stream">
                        } {pipeline_ii = 1 : index}
                      } {pipeline_ii = 4 : index}
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    return
  }
  func.func @load0_3_top(%arg0: memref<1xi512, "stream1">, %arg1: memref<1xi128, "stream">, %arg2: memref<1xi512, "stream1">, %arg3: memref<1xi128, "stream">, %arg4: memref<1xi512, "stream1">, %arg5: memref<1xi128, "stream">, %arg6: memref<1xi512, "stream1">, %arg7: memref<1xi128, "stream">) attributes {adf.pl, inline = false} {
    call @load0_3(%arg0, %arg1) {template = 0 : index} : (memref<1xi512, "stream1">, memref<1xi128, "stream">) -> ()
    call @load0_3(%arg2, %arg3) {template = 1 : index} : (memref<1xi512, "stream1">, memref<1xi128, "stream">) -> ()
    call @load0_3(%arg4, %arg5) {template = 2 : index} : (memref<1xi512, "stream1">, memref<1xi128, "stream">) -> ()
    call @load0_3(%arg6, %arg7) {template = 3 : index} : (memref<1xi512, "stream1">, memref<1xi128, "stream">) -> ()
    return
  }
  func.func @load1(%arg0: memref<32x8xi512>, %arg1: memref<1xi512, "stream1">, %arg2: memref<1xi512, "stream1">) attributes {adf.pl, inline = false, load, mem_idx = [0 : i32], mem_type = [f32], template} {
    %c1 = arith.constant 1 : index
    affine.for %arg3 = 0 to 1 {
      affine.for %arg4 = 0 to 2 {
        affine.for %arg5 = 0 to 2 {
          affine.for %arg6 = 0 to 2 {
            affine.for %arg7 = 0 to 2 {
              affine.for %arg8 = 0 to 8 {
                affine.for %arg9 = 0 to 2 {
                  affine.for %arg10 = 0 to 2 {
                    %0 = affine.load %arg0[%arg8 + %arg7 * 8 + %arg5 * 16, %arg10 + %arg9 * 2 + %arg4 * 4] : memref<32x8xi512>
                    %1 = arith.cmpi slt, %arg10, %c1 : index
                    scf.if %1 {
                      affine.store %0, %arg2[0] : memref<1xi512, "stream1">
                    } else {
                      affine.store %0, %arg1[0] : memref<1xi512, "stream1">
                    }
                  } {pipeline_ii = 1 : index}
                }
              }
            } {merge}
          } {Array_Partition, reduction = 1 : i64}
        } {reduction = 0 : i64}
      }
    }
    return
  }
  func.func @load1_top(%arg0: memref<32x8xi512>, %arg1: memref<1xi512, "stream1">, %arg2: memref<1xi512, "stream1">) attributes {adf.pl, inline = false} {
    call @load1(%arg0, %arg1, %arg2) {template = 0 : index} : (memref<32x8xi512>, memref<1xi512, "stream1">, memref<1xi512, "stream1">) -> ()
    return
  }
  func.func @load1_1(%arg0: memref<1xi512, "stream1">, %arg1: memref<1xi128, "stream">) attributes {adf.pl, inline = false, load, template} {
    affine.for %arg2 = 0 to 1 {
      affine.for %arg3 = 0 to 2 {
        affine.for %arg4 = 0 to 2 {
          affine.for %arg5 = 0 to 2 {
            affine.for %arg6 = 0 to 2 {
              affine.for %arg7 = 0 to 8 {
                affine.for %arg8 = 0 to 2 {
                  affine.for %arg9 = 0 to 1 {
                    %0 = affine.load %arg0[0] : memref<1xi512, "stream1">
                    affine.for %arg10 = 0 to 4 {
                      %1 = affine.apply #map(%arg10)
                      %2 = affine.apply #map1(%arg10)
                      %3 = adf.get_slice(%0 : i512, %1, %2) -> i128
                      affine.store %3, %arg1[0] : memref<1xi128, "stream">
                    } {pipeline_ii = 1 : index}
                  } {pipeline_ii = 4 : index}
                }
              }
            }
          }
        }
      }
    }
    return
  }
  func.func @load1_1_top(%arg0: memref<1xi512, "stream1">, %arg1: memref<1xi128, "stream">, %arg2: memref<1xi512, "stream1">, %arg3: memref<1xi128, "stream">) attributes {adf.pl, inline = false} {
    call @load1_1(%arg0, %arg1) {template = 0 : index} : (memref<1xi512, "stream1">, memref<1xi128, "stream">) -> ()
    call @load1_1(%arg2, %arg3) {template = 1 : index} : (memref<1xi512, "stream1">, memref<1xi128, "stream">) -> ()
    return
  }
  func.func @mttkrp_pl(%arg0: memref<8x32x8xi512>, %arg1: memref<32x8xi512>, %arg2: memref<128x8xi512>, %arg3: memref<8x8xi512>, %arg4: memref<1xi128, "plio">, %arg5: memref<1xi128, "plio">, %arg6: memref<1xi128, "plio">, %arg7: memref<1xi128, "plio">, %arg8: memref<1xi128, "plio">, %arg9: memref<1xi128, "plio">, %arg10: memref<1xi128, "plio">, %arg11: memref<1xi128, "plio">, %arg12: memref<1xi128, "plio">, %arg13: memref<1xi128, "plio">, %arg14: memref<1xi128, "plio">, %arg15: memref<1xi128, "plio">, %arg16: memref<1xi128, "plio">, %arg17: memref<1xi128, "plio">) attributes {adf.pl = true, dataflow, inline = false, mem_idx = [0 : i32, 1 : i32, 2 : i32, 3 : i32], mem_type = [f32, f32, f32, f32]} {
    %c0_i128 = arith.constant 0 : i128
    %alloc = memref.alloc() : memref<1xi128, "stream">
    %alloc_0 = memref.alloc() : memref<1xi128, "stream">
    %alloc_1 = memref.alloc() : memref<1xi128, "stream">
    %alloc_2 = memref.alloc() : memref<1xi128, "stream">
    %alloc_3 = memref.alloc() : memref<1xi128, "stream">
    %alloc_4 = memref.alloc() : memref<1xi128, "stream">
    %alloc_5 = memref.alloc() : memref<1xi128, "stream">
    %alloc_6 = memref.alloc() : memref<1xi128, "stream">
    %alloc_7 = memref.alloc() : memref<1xi128, "stream">
    %alloc_8 = memref.alloc() : memref<1xi128, "stream">
    %alloc_9 = memref.alloc() : memref<1xi128, "stream">
    %alloc_10 = memref.alloc() : memref<1xi128, "stream">
    %alloc_11 = memref.alloc() : memref<1xi128, "stream">
    %alloc_12 = memref.alloc() : memref<1xi128, "stream">
    %alloc_13 = memref.alloc() {buffer_type = "bram_s2p"} : memref<4x8xi128, 1>
    affine.for %arg18 = 0 to 4 {
      affine.for %arg19 = 0 to 8 {
        affine.store %c0_i128, %alloc_13[%arg18, %arg19] : memref<4x8xi128, 1>
      } {pipeline_ii = 1 : index}
    }
    %alloc_14 = memref.alloc() {buffer_type = "bram_s2p"} : memref<4x8xi128, 1>
    affine.for %arg18 = 0 to 4 {
      affine.for %arg19 = 0 to 8 {
        affine.store %c0_i128, %alloc_14[%arg18, %arg19] : memref<4x8xi128, 1>
      } {pipeline_ii = 1 : index}
    }
    %alloc_15 = memref.alloc() {buffer_type = "bram_s2p"} : memref<4x8xi128, 1>
    affine.for %arg18 = 0 to 4 {
      affine.for %arg19 = 0 to 8 {
        affine.store %c0_i128, %alloc_15[%arg18, %arg19] : memref<4x8xi128, 1>
      } {pipeline_ii = 1 : index}
    }
    %alloc_16 = memref.alloc() {buffer_type = "bram_s2p"} : memref<4x8xi128, 1>
    affine.for %arg18 = 0 to 4 {
      affine.for %arg19 = 0 to 8 {
        affine.store %c0_i128, %alloc_16[%arg18, %arg19] : memref<4x8xi128, 1>
      } {pipeline_ii = 1 : index}
    }
    %alloc_17 = memref.alloc() : memref<1xi512, "stream1">
    %alloc_18 = memref.alloc() : memref<1xi512, "stream1">
    %alloc_19 = memref.alloc() : memref<1xi512, "stream1">
    %alloc_20 = memref.alloc() : memref<1xi512, "stream1">
    %alloc_21 = memref.alloc() : memref<1xi512, "stream1">
    %alloc_22 = memref.alloc() : memref<1xi512, "stream1">
    %alloc_23 = memref.alloc() : memref<1xi512, "stream1">
    %alloc_24 = memref.alloc() : memref<1xi512, "stream1">
    %alloc_25 = memref.alloc() : memref<1xi512, "stream1">
    %alloc_26 = memref.alloc() : memref<1xi512, "stream1">
    %alloc_27 = memref.alloc() : memref<1xi512, "stream1">
    %alloc_28 = memref.alloc() : memref<1xi512, "stream1">
    %alloc_29 = memref.alloc() : memref<1xi512, "stream1">
    %alloc_30 = memref.alloc() : memref<1xi512, "stream1">
    call @send3_top(%arg11, %alloc_9, %arg10, %alloc_4, %arg4, %alloc_3, %arg5, %alloc_12) : (memref<1xi128, "plio">, memref<1xi128, "stream">, memref<1xi128, "plio">, memref<1xi128, "stream">, memref<1xi128, "plio">, memref<1xi128, "stream">, memref<1xi128, "plio">, memref<1xi128, "stream">) -> ()
    call @load2_top(%arg2, %alloc_17, %alloc_18, %alloc_19, %alloc_20) : (memref<128x8xi512>, memref<1xi512, "stream1">, memref<1xi512, "stream1">, memref<1xi512, "stream1">, memref<1xi512, "stream1">) -> ()
    call @load2_3_top(%alloc_20, %alloc_10, %alloc_19, %alloc_8, %alloc_18, %alloc_6, %alloc_17, %alloc_5) : (memref<1xi512, "stream1">, memref<1xi128, "stream">, memref<1xi512, "stream1">, memref<1xi128, "stream">, memref<1xi512, "stream1">, memref<1xi128, "stream">, memref<1xi512, "stream1">, memref<1xi128, "stream">) -> ()
    call @send5_top(%arg15, %alloc_7, %arg16, %alloc_11) : (memref<1xi128, "plio">, memref<1xi128, "stream">, memref<1xi128, "plio">, memref<1xi128, "stream">) -> ()
    call @receive2_top(%arg6, %alloc_1, %arg14, %alloc, %arg7, %alloc_2, %arg17, %alloc_0) : (memref<1xi128, "plio">, memref<1xi128, "stream">, memref<1xi128, "plio">, memref<1xi128, "stream">, memref<1xi128, "plio">, memref<1xi128, "stream">, memref<1xi128, "plio">, memref<1xi128, "stream">) -> ()
    call @send6_top(%arg12, %alloc_6, %arg13, %alloc_8, %arg9, %alloc_5, %arg8, %alloc_10) : (memref<1xi128, "plio">, memref<1xi128, "stream">, memref<1xi128, "plio">, memref<1xi128, "stream">, memref<1xi128, "plio">, memref<1xi128, "stream">, memref<1xi128, "plio">, memref<1xi128, "stream">) -> ()
    call @store0_0_top(%alloc_2, %alloc_21, %alloc_1, %alloc_22, %alloc_0, %alloc_23, %alloc, %alloc_24) : (memref<1xi128, "stream">, memref<1xi512, "stream1">, memref<1xi128, "stream">, memref<1xi512, "stream1">, memref<1xi128, "stream">, memref<1xi512, "stream1">, memref<1xi128, "stream">, memref<1xi512, "stream1">) -> ()
    call @store0_top(%arg3, %alloc_24, %alloc_22, %alloc_23, %alloc_21) : (memref<8x8xi512>, memref<1xi512, "stream1">, memref<1xi512, "stream1">, memref<1xi512, "stream1">, memref<1xi512, "stream1">) -> ()
    call @load0_top(%arg0, %alloc_25, %alloc_28, %alloc_27, %alloc_26) : (memref<8x32x8xi512>, memref<1xi512, "stream1">, memref<1xi512, "stream1">, memref<1xi512, "stream1">, memref<1xi512, "stream1">) -> ()
    call @load0_3_top(%alloc_28, %alloc_12, %alloc_27, %alloc_9, %alloc_26, %alloc_4, %alloc_25, %alloc_3) : (memref<1xi512, "stream1">, memref<1xi128, "stream">, memref<1xi512, "stream1">, memref<1xi128, "stream">, memref<1xi512, "stream1">, memref<1xi128, "stream">, memref<1xi512, "stream1">, memref<1xi128, "stream">) -> ()
    call @load1_top(%arg1, %alloc_29, %alloc_30) : (memref<32x8xi512>, memref<1xi512, "stream1">, memref<1xi512, "stream1">) -> ()
    call @load1_1_top(%alloc_30, %alloc_11, %alloc_29, %alloc_7) : (memref<1xi512, "stream1">, memref<1xi128, "stream">, memref<1xi512, "stream1">, memref<1xi128, "stream">) -> ()
    return
  }
  func.func @mttkrp(%arg0: memref<8x32x8xi512>, %arg1: memref<32x8xi512>, %arg2: memref<128x8xi512>, %arg3: memref<8x8xi512>, %arg4: memref<1xi128, "plio">, %arg5: memref<1xi128, "plio">, %arg6: memref<1xi128, "plio">, %arg7: memref<1xi128, "plio">, %arg8: memref<1xi128, "plio">, %arg9: memref<1xi128, "plio">, %arg10: memref<1xi128, "plio">, %arg11: memref<1xi128, "plio">, %arg12: memref<1xi128, "plio">, %arg13: memref<1xi128, "plio">, %arg14: memref<1xi128, "plio">, %arg15: memref<1xi128, "plio">, %arg16: memref<1xi128, "plio">, %arg17: memref<1xi128, "plio">) attributes {adf.func, mem_idx = [0 : i32, 1 : i32, 2 : i32, 3 : i32], mem_type = [f32, f32, f32, f32], plio = true} {
    %0 = adf.graph.io(PLIO) : !adf.plio<Out, 128>
    adf.connect(%0, %arg7) {top_config} : (!adf.plio<Out, 128>, memref<1xi128, "plio">)
    %1 = adf.graph.io(PLIO) : !adf.plio<Out, 128>
    adf.connect(%1, %arg6) {top_config} : (!adf.plio<Out, 128>, memref<1xi128, "plio">)
    %2 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    adf.connect(%arg4, %2) {top_config} : (memref<1xi128, "plio">, !adf.plio<In, 128>)
    %3 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    adf.connect(%arg10, %3) {top_config} : (memref<1xi128, "plio">, !adf.plio<In, 128>)
    %4 = adf.graph.io(PLIO) : !adf.plio<Out, 128>
    adf.connect(%4, %arg17) {top_config} : (!adf.plio<Out, 128>, memref<1xi128, "plio">)
    %5 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    adf.connect(%arg9, %5) {top_config} : (memref<1xi128, "plio">, !adf.plio<In, 128>)
    %6 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    adf.connect(%arg12, %6) {top_config} : (memref<1xi128, "plio">, !adf.plio<In, 128>)
    %7 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    adf.connect(%arg15, %7) {top_config} : (memref<1xi128, "plio">, !adf.plio<In, 128>)
    %8 = adf.graph.io(PLIO) : !adf.plio<Out, 128>
    adf.connect(%8, %arg14) {top_config} : (!adf.plio<Out, 128>, memref<1xi128, "plio">)
    %9 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    adf.connect(%arg13, %9) {top_config} : (memref<1xi128, "plio">, !adf.plio<In, 128>)
    %10 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    adf.connect(%arg11, %10) {top_config} : (memref<1xi128, "plio">, !adf.plio<In, 128>)
    %11 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    adf.connect(%arg8, %11) {top_config} : (memref<1xi128, "plio">, !adf.plio<In, 128>)
    %12 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    adf.connect(%arg16, %12) {top_config} : (memref<1xi128, "plio">, !adf.plio<In, 128>)
    %13 = adf.graph.io(PLIO) : !adf.plio<In, 128>
    adf.connect(%arg5, %13) {top_config} : (memref<1xi128, "plio">, !adf.plio<In, 128>)
    adf.cell.launch @adf_cell0 {
      func.call @adf_cell0(%13, %12, %11, %10, %9, %8, %7, %6, %5, %4, %3, %2, %1, %0) {adf.cell} : (!adf.plio<In, 128>, !adf.plio<In, 128>, !adf.plio<In, 128>, !adf.plio<In, 128>, !adf.plio<In, 128>, !adf.plio<Out, 128>, !adf.plio<In, 128>, !adf.plio<In, 128>, !adf.plio<In, 128>, !adf.plio<Out, 128>, !adf.plio<In, 128>, !adf.plio<In, 128>, !adf.plio<Out, 128>, !adf.plio<Out, 128>) -> ()
      adf.cell.launch.end
    }
    adf.pl.launch @mttkrp_pl {
      func.call @mttkrp_pl(%arg0, %arg1, %arg2, %arg3, %arg4, %arg5, %arg6, %arg7, %arg8, %arg9, %arg10, %arg11, %arg12, %arg13, %arg14, %arg15, %arg16, %arg17) {adf.pl} : (memref<8x32x8xi512>, memref<32x8xi512>, memref<128x8xi512>, memref<8x8xi512>, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">) -> ()
      adf.pl.launch.wait
    }
    return
  }
  func.func @top(%arg0: memref<8x32x8xi512>, %arg1: memref<32x8xi512>, %arg2: memref<128x8xi512>, %arg3: memref<8x8xi512>, %arg4: memref<1xi128, "plio">, %arg5: memref<1xi128, "plio">, %arg6: memref<1xi128, "plio">, %arg7: memref<1xi128, "plio">, %arg8: memref<1xi128, "plio">, %arg9: memref<1xi128, "plio">, %arg10: memref<1xi128, "plio">, %arg11: memref<1xi128, "plio">, %arg12: memref<1xi128, "plio">, %arg13: memref<1xi128, "plio">, %arg14: memref<1xi128, "plio">, %arg15: memref<1xi128, "plio">, %arg16: memref<1xi128, "plio">, %arg17: memref<1xi128, "plio">) attributes {outArgs = [3 : i32], top_func = "plio"} {
    call @mttkrp_pl(%arg0, %arg1, %arg2, %arg3, %arg4, %arg5, %arg6, %arg7, %arg8, %arg9, %arg10, %arg11, %arg12, %arg13, %arg14, %arg15, %arg16, %arg17) : (memref<8x32x8xi512>, memref<32x8xi512>, memref<128x8xi512>, memref<8x8xi512>, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">, memref<1xi128, "plio">) -> ()
    return
  }
  func.func private @mttkrp_host(memref<8x32x128xf32>, memref<32x128xf32>, memref<128x128xf32>, memref<8x128xf32>) attributes {origin_func = "mttkrp"}
  func.func @top_host(%arg0: memref<8x32x128xf32>, %arg1: memref<32x128xf32>, %arg2: memref<128x128xf32>, %arg3: memref<8x128xf32>) attributes {origin_func = "top", outArgs = [3 : i32], top_host} {
    call @mttkrp_host(%arg0, %arg1, %arg2, %arg3) {origin_func = "mttkrp"} : (memref<8x32x128xf32>, memref<32x128xf32>, memref<128x128xf32>, memref<8x128xf32>) -> ()
    return
  }
}

