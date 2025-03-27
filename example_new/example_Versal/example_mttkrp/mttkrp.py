import os
import sys
cur_dir = os.path.dirname(os.path.abspath(__file__))
aries_path = cur_dir + "/../../../"
sys.path.append(aries_path)
from frontend import *

# MTTKRP: D[i0, j0] += A[i0, k0, l0] * B[k0, j0] * C[l0, j0]
I, J, K, L = 8, 128, 32, 128
TI, TJ, TK, TL = 2, 16, 8, 16

@task_kernel(external_path="aie1/adf/kernel_mttkrp/aie_fp32", para = [TI, TJ, TK, TL])
def kernel_mttkrp(TileA: float32[TI, TK, TL],
                  TileB: float32[TK, TJ],
                  TileC: float32[TL, TJ],
                  TileD: float32[TI, TJ]):
    for i0 in range(0, TI):
        for j0 in range(0, TJ):
            TileD[i0, j0] = float32(0)
            for k0 in range(0, TK):
                for l0 in range(0, TL):
                    TileD[i0, j0] += TileA[i0, k0, l0] * TileB[k0, j0] * TileC[l0, j0]

@task_tile()
def mttkrp(A: float32[I, K, L], B: float32[K, J], 
           C: float32[L, J],    D: float32[I, J], **kwargs):
    i, j, k, l = aries.tile_ranks(**kwargs)

    # Compute tile slices for multiple dimensions
    ti = aries.arange(i*TI, (i+1)*TI)  # I tile range
    tj = aries.arange(j*TJ, (j+1)*TJ)  # J tile range
    tk = aries.arange(k*TK, (k+1)*TK)  # K tile range
    tl = aries.arange(l*TL, (l+1)*TL)  # L tile range
    
    L1_A = aries.buffer((TI, TK, TL), "float32")
    L1_B = aries.buffer((TK, TJ), "float32")
    L1_C = aries.buffer((TL, TJ), "float32")
    L1_D = aries.accbuffer((TI, TJ), "float32")
    
    L1_A = aries.load(A, (ti, tk, tl))
    L1_B = aries.load(B, (tk, tj))
    L1_C = aries.load(C, (tl, tj))
    kernel_mttkrp(L1_A, L1_B, L1_C, L1_D)
    aries.accstore(L1_D, D, (ti, tj))

@task_top()
def top(A: float32[I, K, L], B: float32[K, J], 
        C: float32[L, J],    D: float32[I, J]):
    grid, size = (I//TI, J//TJ, K//TK, L//TL), (TI, TJ, TK, TL)
    mttkrp_task = mttkrp[grid, size](A, B, C, D)
    return mttkrp_task

def mttkrp_sw(A: float32[I, K, L], B: float32[K, J], C: float32[L, J]):
    D = np.zeros((I, J)).astype(np.float32)
    for i0 in range(0, I):
        for j0 in range(0, J):
            for k0 in range(0, K):
                  for l0 in range(0, L):
                      D[i0, j0] += A[i0, k0, l0] * B[k0, j0] * C[l0, j0]
    return D

# Set the project dir and template dir
prj_dir= cur_dir + '/my_project'
temp_dir= aries_path + '/templates'
module = sys.modules[__name__]
    
# Initialize the buffers
A = np.random.rand(I, K, L).astype(np.float32)
B = np.random.rand(K, J).astype(np.float32)
C = np.random.rand(L, J).astype(np.float32)
D = np.zeros((I, J)).astype(np.float32)

# Execute on CPU
mttkrp_task = top(A, B, C, D)

# Golden file generation
E = mttkrp_sw(A, B, C)

# Compare the program with golden file
print(np.allclose(D, E))

# Applying schedulings
sch = Schedule(mttkrp_task)
sch.parallel(mttkrp_task, [2, 2, 1, 2]) # AIE Array Parallelism
sch.l2buffer(mttkrp_task, [2, 2, 2, 2]) # L2 buffer data reuse
sch.bufsel(mttkrp_task, [0, 1, 0, 1]) # Select the type of buffer of A, B, C, 1:BRAM; 0:URAM
sch.to("VCK190")

# Generate files for harware test
aries.gen_sim([A, B, C, E])

sch.build(module, prj_dir, temp_dir)