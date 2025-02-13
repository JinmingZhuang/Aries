import ast
import astor
import inspect
import sympy as sp
from .aries_decorators import TaskTileWrapper, TaskKernelWrapper, TaskTopWrapper

# =========== Code Transformation and IR Builder ===========

# =========== Base Emitter Class ===========
class MLIRGenerator(ast.NodeVisitor):
    def __init__(self, mapInfo, map_cnt=0):
        self.mlir_func_code = [] # Store the code within func
        self.mlir_map_code = [] # Store the AffineMap code
        self.indent = 2
        self.var_count = 0
        self.map_cnt = map_cnt
        
        self.valName = {} # valName["A"] = "%A"
        self.valType = {} # E.g., valType["A"] = (T, [32, 32])
        self.valNameCnt = {} # valNameCnt["%A"] = cnt, to avoid name conflict
        self.map = {} # map["ti"] = # map, dims  now only record the offset map
        self.mapInfo = mapInfo # dic["ti"] = (32*i), stores key and expression
        self.in_assign  = False # Track whether inside an Assign node

    ## -- Variable & Type Management -- ##
    def add_var_name(self, val, name=""):
        if(name != ""):
            name = "%" + name
            if(self.valNameCnt.get(name, 0) > 0):
                valNameStr = name + "_" + str(self.valNameCnt[name])
                self.valNameCnt[name] += 1
            else:
                self.valNameCnt[name] = 1
                valNameStr = name
        else:
            valNameStr = "%v" + str(self.var_count)
            self.var_count += 1
        self.valName[val] = valNameStr
        return valNameStr
    
    def get_var_name(self, val):
        if val not in self.valName:
            assert False, f"KeyError: The val '{val}' was not defined."
        valNameStr = self.valName[val]
        return valNameStr
    
    def add_type_name(self, val, type, shape=[], memSpace = ""):
        self.valType[val] = (type, shape, memSpace)
        typeName = self.get_type_name(val)
        return typeName
    
    def get_elewidth(self, dtype: str) -> int:
        if dtype.startswith("int"):
            return int(dtype[3:])
        elif dtype.startswith("float"):
            return int(dtype[5:])
        elif dtype.startswith("i"):
            return int(dtype[1:])
        elif dtype.startswith("f"):
            return int(dtype[1:])
        raise ValueError(f"Invalid dtype: {dtype}")
        
    def get_eletype_name(self, arg):
        ty = self.valType[arg][0]
        if(ty == "float32"):
          return "f32"
        elif(ty == "int32"):
          return "i32"
        elif(ty == "int16"):
          return "i16"
        elif(ty == "int8"):
          return "i8"
        elif(ty == "index"):
          return "index"
        else:
          raise TypeError(f"Type {ty} not supported.")
    
    def addMemSpace(self, type_name, memSpace):
        if memSpace == "L1":
            type_name += ", " + str(2)
        elif memSpace == "L2":
            type_name += ", " + str(1)
        elif memSpace == "L3":
            type_name += ", " + str(0)
        return type_name
    
    def get_MemRefType_name(self, arg):
      _, shape, memSpace = self.valType[arg]
      shape_name = "x".join([str(s) for s in shape])
      type_name = self.get_eletype_name(arg)
      type_name = self.addMemSpace(type_name, memSpace)
      memrefType_name = f"memref<{shape_name}x{type_name}>"
      return memrefType_name
    
    def get_type_name(self, arg):
        if arg not in self.valType:
            assert False, f"KeyError: The type of '{arg}' was not found."
        if not self.is_array(arg):
            typeName = self.get_eletype_name(arg)
        else:
            typeName = self.get_MemRefType_name(arg)
        return typeName
    
    def is_declared(self, arg):
        if arg in self.valName:
            return True
        else:
            return False
    
    def is_array(self, arg):
        if arg not in self.valType:
            assert False, f"KeyError: The type of '{arg}' was not found." 
        _, shape, _ = self.valType[arg] 
        if len(shape) == 0:
            return False
        else:
            return True       
    
    def addArgType(self, node, memSpace = ""):
        # Record the type of arguments in the function
        for arg in node.args.args:
            self.add_var_name(arg.arg, arg.arg)
            if arg.annotation:
                ty = arg.annotation
                if isinstance(ty, ast.Subscript):
                    # Extract shape
                    if isinstance(ty.slice, ast.Tuple):
                        shape = tuple(constant.value for constant in ty.slice.elts)
                    else:
                        shape = [ty.slice.value]
                    self.add_type_name(arg.arg, ty.value.id, shape, memSpace)
                else:
                    assert False, "Unspported argument type found!"
        return
    
    ## -- Core AST Processing -- ##
    def visit_FunctionDef(self, node):
        # Add types for each arg
        self.addArgType(node)
        
        # Process the function definition and arguments
        func_name = node.name
        args = [arg.arg for arg in node.args.args]
        # Get annotation types
        memrefs = []
        for arg in args:
            if arg in self.valType:
                typeName = self.get_type_name(arg)
                memrefs.append(f"%{arg}: {typeName}")
            else:
                raise ValueError(f"Argument {arg} not found in valType.")
        self.emit(f"func.func @{func_name}({', '.join(memrefs)}) {{")

        self.indent += 2
        self.generic_visit(node)
        self.emit("return")
        self.indent -= 2
        self.emit("}")
    
    def visit_For(self, node):
        # Process affine.for loop
        loop_var = node.target.id  # e.g., i0, j0, k0
        loop_Name = self.add_var_name(loop_var, loop_var)
        loop_call = node.iter.func.id
        loop_lb = node.iter.args[0].value 
        loop_ub = node.iter.args[1].value 
        self.emit(f"affine.for {loop_Name} = {loop_lb} to {loop_ub} {{")

        self.indent += 2
        self.generic_visit(node)
        self.indent -= 2

        if loop_call == "reduction_range":
          self.emit("}{reduction}")
        else:
          self.emit("}")
    
    ## -- Emit Functions -- ##
    def emit(self, code, same_line=False):
        if same_line:
            self.mlir_func_code[-1] = self.mlir_func_code[-1] + code
        else:
            self.mlir_func_code.append(" " * self.indent + code)
    
    def emitMap(self, code):
        self.mlir_map_code.append(code)
    
    def find_dim(self, expr):
        """
        Extract dimension variables from a SymPy expression.

        Args:
            expr: SymPy expression (e.g., i * 32 + j)

        Returns:
            List of dimension variable names (sorted for consistency)
        """
        return sorted([str(symbol) for symbol in expr.free_symbols])  # Extract symbols as strings


    def emit_affine_map(self, expr, dims, syms):
        """
        Convert a SymPy expression to an MLIR affine map.

        Args:
            expr: SymPy expression (e.g., i * 32 + j)
            dims: List of dimension variables (e.g., ['i', 'j'])
            syms: List of symbolic variables (e.g., [])

        Returns:
            MLIR affine map string (e.g., affine_map<(d0, d1) -> (d0*32 + d1)>)
        """
        dim_map = {dim: f"d{i}" for i, dim in enumerate(dims)}
        sym_map = {sym: f"s{i}" for i, sym in enumerate(syms)}

        affine_expr = expr.subs(dim_map).subs(sym_map)

        dim_str = ", ".join(dim_map.values()) if dim_map else ""
        sym_str = ", ".join(sym_map.values()) if sym_map else ""

        map_str = f"affine_map<({dim_str})"
        if sym_str:
            map_str += f"[{sym_str}]"
        map_str += f" -> ({affine_expr})>"

        return map_str
    
    # Generate map for offset
    def gen_map(self):
        if self.mapInfo is None:
            return
        for key, value in self.mapInfo.items():
            offestExpr = value[0]
            dims = self.find_dim(offestExpr)
            affineMap = self.emit_affine_map(offestExpr, dims, [])
            mapName = "#map" + str(self.map_cnt)
            self.map_cnt +=1
            self.map[key] = (mapName, dims)
            self.emitMap(f"{mapName} = {affineMap}")
        return
    
    def generate(self, tree):
        # Generate the code for AffineMap
        self.gen_map()
        
        # Parse the input Python code and visit nodes
        self.visit(tree)
        func_code = "\n".join(self.mlir_func_code)
        map_code = "\n".join(self.mlir_map_code)
        return func_code, map_code, self.map_cnt


# =========== Emitter for task tile ===========
class TileMLIRGenerator(MLIRGenerator):
    # dmaInfo: dic["ti"] = (32*i, 32, 1) (offset, size, step)
    # This is a slightly different than this parent class where the mapInfo 
    # only contains the expression
    def __init__(self, dmaInfo, map_cnt=0):
        super().__init__(dmaInfo, map_cnt)
    
    def DmaInfo(self, arg, offsets, sizes, strides):
        eltName = arg.id
        # For each dim, create the affine.apply op for each offset
        mapName, dims = self.map[eltName] #mapName, dims
        dimNames = [self.get_var_name(dim) for dim in dims]
        offsetVar = arg.id + "_offset"
        offsetVarName = self.add_var_name(offsetVar)
        offsets.append(offsetVarName)
        self.emit(f"{offsetVarName} = affine.apply {mapName}({', '.join(dimNames)})")
        # For each dim, create constant for size and stride
        _, size, step = self.mapInfo[eltName]
        sizeVar = arg.id + "_size"
        sizeVarName = self.add_var_name(sizeVar, "c" + sizeVar)
        sizes.append(sizeVarName)
        self.emit(f"{sizeVarName} = arith.constant {size} : index")
        strideVar = arg.id + "_stride"
        strideVarName = self.add_var_name(strideVar, "c" + strideVar)
        strides.append(strideVarName)
        self.emit(f"{strideVarName} = arith.constant {step} : index")
        return offsets, sizes, strides
    
    def getDmaInfo(self, call, is_load = False):
        # Stores the VarName of offsets, sizes and strides
        # that can be printed out directly
        offsets = []
        sizes = []
        strides = []
        srcMemName = None
        dstMemName = None
        name_cnt = 0
        srcMemName = call.args[0].id
        if is_load:
            idx = 1
        else:
            dstMemName = call.args[1].id
            idx = 2
        if isinstance(call.args[idx], ast.Name):
            offsets, sizes, strides = self.DmaInfo(call.args[idx], offsets, sizes, strides)
        else:
            for elt in call.args[idx].elts:
                offsets, sizes, strides = self.DmaInfo(elt, offsets, sizes, strides)
        return offsets, sizes, strides, srcMemName, dstMemName
    
    def visit_Assign(self, node):
        assert len(node.targets) == 1
        target = node.targets[0]
        
        if isinstance(node.value, ast.Call):
            assert isinstance(node.value.func, ast.Attribute)
            assert isinstance(node.value.func.value, ast.Name)
            assert node.value.func.value.id == 'aries'
            
            if node.value.func.attr == 'load':
                call = node.value
                dstMemName = target.id
                offsets, sizes, strides, srcMemName, _ = self.getDmaInfo(call, True)
                srcMem = self.get_var_name(srcMemName)
                srcType = self.get_type_name(srcMemName)
                dstMem = self.get_var_name(dstMemName)
                dstType = self.get_type_name(dstMemName)
                self.emit(f"adf.dma({srcMem}[{' ,'.join(offsets)}] [{' ,'.join(sizes)}] [{' ,'.join(strides)}], {dstMem}[] [] []) : ({srcType} , {dstType})")  
                return
            
            elif node.value.func.attr == 'buffer':
                buffer = target.id
                self.add_var_name(buffer, buffer)
                shape_node = node.value.args[0]
                type_node = node.value.args[1]
                shape = tuple(constant.value for constant in shape_node.elts)
                type = type_node.value
                typeName = self.add_type_name(buffer, type, shape, "L1")
                bufferName = self.get_var_name(buffer)
                self.emit(f"{bufferName} = adf.buffer.create @L1_{buffer}() : {typeName}")
                return
    
    def visit_Expr(self, node):
        if isinstance(node.value, ast.Call):
            if isinstance(node.value.func, ast.Name):
                calleeName = node.value.func.id
                args = node.value.args
                argNames = []
                argTypes = []
                for arg in args:
                    argNames.append(self.get_var_name(arg.id))
                    argTypes.append(self.get_type_name(arg.id))
                self.emit(f"func.call @{calleeName}({', '.join(argNames)}) : (")
                self.emit(f"{', '.join(argTypes)}) -> ()", True)
            elif isinstance(node.value.func, ast.Attribute):
                if node.value.func.attr == 'store':
                    call = node.value
                    offsets, sizes, strides, srcMemName, dstMemName = self.getDmaInfo(call)
                    srcMem = self.get_var_name(srcMemName)
                    srcType = self.get_type_name(srcMemName)
                    dstMem = self.get_var_name(dstMemName)
                    dstType = self.get_type_name(dstMemName)
                    self.emit(f"adf.dma({srcMem}[] [] [], {dstMem}[{' ,'.join(offsets)}] [{' ,'.join(sizes)}] [{' ,'.join(strides)}]) : ({srcType} , {dstType})")  
                    return
                
# =========== Emitter for task kernel ===========
class KernelMLIRGenerator(MLIRGenerator):
    def __init__(self, dmaInfo, map_cnt=0):
        super().__init__(dmaInfo, map_cnt)
        self.is_assign = False
        
    def get_type(self, node):
        """Retrieve the type of a variable or constant."""
        if isinstance(node, ast.Constant):
            if isinstance(node.value, int):
                return "int32"
            elif isinstance(node.value, float):
                return "float32"
            else:
                raise TypeError(f"Unsupported constant type: {type(node.value)}")
        elif isinstance(node, ast.Call): # Defined constant type
            return node.func.id
        elif isinstance(node, ast.Subscript): # Memref
            if node.value.id in self.valType:
                return self.valType[node.value.id][0]
            else:
                raise KeyError(f"Variable '{node.value.id}' is not declared.")
        elif isinstance(node, ast.Name):
            if node.id in self.valType:
                return self.valType[node.id][0]  # Extract stored type
            else:
                raise KeyError(f"Variable '{node.id}' is not declared.")
        elif isinstance(node, ast.BinOp):
            # If it's a BinOp, recursively check the types of the left and right operands
            left_type = self.get_type(node.left)
            right_type = self.get_type(node.right)
            # Check that the types of the left and right operands are compatible
            if left_type != right_type:
                raise TypeError(f"Type mismatch: {left_type} and {right_type} are not compatible.")
            # Return the type of the BinOp (same as the operand types)
            return left_type
        else:
            raise TypeError(f"Unsupported type retrieval for node: {ast.dump(node)}")
    
    def addArgType(self, node, memSpace="L1"):
        """Overrides parent method, specifying memorySpace = 'L1'."""
        super().addArgType(node, memSpace=memSpace)  # Calls parent with L1
    
    def get_op_type(self, op, operand_type):
        """Maps Python operators to MLIR integer or floating-point operations."""
        float_ops = {
            ast.Add: "arith.addf",
            ast.Sub: "arith.subf",
            ast.Mult: "arith.mulf",
            ast.Div: "arith.divf",
        }
        int_ops = {
            ast.Add: "arith.addi",
            ast.Sub: "arith.subi",
            ast.Mult: "arith.muli",
            ast.Div: "arith.divsi",  # Signed integer division
        }

        if operand_type.startswith("f"):
            return float_ops.get(type(op), None)
        elif operand_type.startswith("i"):
            return int_ops.get(type(op), None)
        else:
            raise NotImplementedError(f"Unsupported operand type: {operand_type}")
    
    def visit_Call(self, node):
        if self.is_assign == False:
            return
        assert isinstance(node.func, ast.Name)
        assert len(node.args) == 1
        value = node.args[0].value
        
        if node.func.id.startswith("f"):
            formatted_value = f"{value:.6f}"
        elif node.func.id.startswith("i"):
            formatted_value = value
        else:
            raise NotImplementedError(f"Unsupported operand type: {node.func.id}")
        varName = "c" + str(value)
        result = self.add_var_name("temp", varName)
        type = self.add_type_name("temp", node.func.id)
        self.emit(f"{result} = arith.constant {formatted_value} : {type}")
        return result
    
    def visit_BinOp(self, node):
        """Processes binary operations."""
        lhs = self.visit(node.left)
        rhs = self.visit(node.right)
        
        # Retrieve types from stored values
        lhs_type = self.get_type(node.left)
        rhs_type = self.get_type(node.right)
        assert lhs_type == rhs_type, f"Type mismatch: {lhs_type} vs {rhs_type}"
        
        result = self.add_var_name("temp")
        type = self.add_type_name("temp", lhs_type)
        
        op_type = self.get_op_type(node.op, type)
        self.emit(f"{result} = {op_type} {lhs}, {rhs} : {type}")

        return result

    def visit_Subscript(self, node):
        if self.is_assign == False:
            return
        """Handles memory accesses like chunk[i, j]."""
        memref = node.value.id
        memType = self.get_type_name(memref)
        memName = self.get_var_name(memref)
        if isinstance(node.slice, ast.Tuple):
            indices = [self.get_var_name(idx.id) for idx in node.slice.elts]
        else:
            indices = [self.get_var_name(node.slice.id)]
        
        if isinstance(node.ctx, ast.Store):
            return memref, indices
        elif isinstance(node.ctx, ast.Load):
            result = self.add_var_name("load")
            self.emit(f"{result} = affine.load {memName}[{', '.join(indices)}] : {memType}")
            return result 
        else:
            raise NotImplementedError(f"Unsupported context {type(node.ctx)} in Subscript.")

    def visit_Assign(self, node):
        assert len(node.targets) == 1
        target_node = node.targets[0]
        self.is_assign = True
        value = self.visit(node.value)
        if isinstance(target_node, ast.Subscript):
            # Handle memory store case
            memref, indices = self.visit(target_node)
            memType = self.get_type_name(memref)
            memName = self.get_var_name(memref)
            self.emit(f"memref.store {value}, {memName}[{', '.join(indices)}] : {memType}")
        self.is_assign = False
    
    def visit_AugAssign(self, node):
        target_node = node.target
        self.is_assign = True
        value = self.visit(node.value)
        if isinstance(target_node, ast.Subscript):
            # Handle memory store case
            memref, indices = self.visit(target_node)
            memType = self.get_type_name(memref)
            memName = self.get_var_name(memref)
            lhs = self.add_var_name("load")
            self.emit(f"{lhs} = affine.load {memName}[{', '.join(indices)}] : {memType}")
            type = self.get_eletype_name(memref)
            op_type = self.get_op_type(node.op, type)
            result = self.add_var_name("temp")
            self.emit(f"{result} = {op_type} {lhs}, {value} : {type}")
            self.emit(f"memref.store {result}, {memName}[{', '.join(indices)}] : {memType}")
        self.is_assign = False
        return

class TopMLIRGenerator(MLIRGenerator):
    def __init__(self, dmaInfo, map_cnt=0):
        super().__init__(dmaInfo, map_cnt)
        self.is_assign = False
    
    def visit_Expr(self, node):
        if isinstance(node.value, ast.Call):
            if isinstance(node.value.func, ast.Name):
                calleeName = node.value.func.id
                args = node.value.args
                argNames = []
                argTypes = []
                for arg in args:
                    argNames.append(self.get_var_name(arg.id))
                    argTypes.append(self.get_type_name(arg.id))
                self.emit(f"func.call @{calleeName}({', '.join(argNames)}) : (")
                self.emit(f"{', '.join(argTypes)}) -> ()", True)
            elif isinstance(node.value.func, ast.Subscript):
                calleeName = node.value.func.value.id
                args = node.value.args
                argNames = []
                argTypes = []
                for arg in args:
                    argNames.append(self.get_var_name(arg.id))
                    argTypes.append(self.get_type_name(arg.id))
                self.emit(f"func.call @{calleeName}({', '.join(argNames)}) : (")
                self.emit(f"{', '.join(argTypes)}) -> ()", True)
    
    def visit_Assign(self, node):
        if isinstance(node.value, ast.Call):
            if isinstance(node.value.func, ast.Name):
                calleeName = node.value.func.id
                args = node.value.args
                argNames = []
                argTypes = []
                for arg in args:
                    argNames.append(self.get_var_name(arg.id))
                    argTypes.append(self.get_type_name(arg.id))
                self.emit(f"func.call @{calleeName}({', '.join(argNames)}) : (")
                self.emit(f"{', '.join(argTypes)}) -> ()", True)
            elif isinstance(node.value.func, ast.Subscript):
                calleeName = node.value.func.value.id
                args = node.value.args
                argNames = []
                argTypes = []
                for arg in args:
                    argNames.append(self.get_var_name(arg.id))
                    argTypes.append(self.get_type_name(arg.id))
                self.emit(f"func.call @{calleeName}({', '.join(argNames)}) : (")
                self.emit(f"{', '.join(argTypes)}) -> ()", True)     

class TileToLoop(ast.NodeTransformer):
    """Transform tile ranks to loops"""
    def __init__(self, grid_dims=None):
        # tile sizes and grid ranks
        self.grid_dims = grid_dims
    
    def visit_FunctionDef(self, node):
        # Extract assignment to i, j
        new_body = []
        loop_vars = {}

        for stmt in node.body:
            if (isinstance(stmt, ast.Assign) and 
                stmt.value.func.value.id == "aries" and 
                stmt.value.func.attr == "tile_ranks"):
                if isinstance(stmt.targets[0], ast.Tuple):
                    loop_var_names = [elt.id for elt in stmt.targets[0].elts]
                else:
                    loop_var_names = [stmt.targets[0].id]
                for idx, var in enumerate(loop_var_names):
                    loop_vars[var] = self.grid_dims[idx]
                continue
            
            new_body.append(stmt)

        for var, max_range in reversed(loop_vars.items()):
            new_body = [ast.For(
                target=ast.Name(id=var, ctx=ast.Store()),
                iter=ast.Call(
                    func=ast.Name(id="range", ctx=ast.Load()),
                    args=[ast.Constant(value=0), ast.Constant(value=max_range)],
                    keywords=[]
                ),
                body=new_body,
                orelse=[]
            )]

        node.body = new_body  # Replace function body with transformed version
        return node

# Traverse the tree for constant propagation and record arange info
class TilePreprocess(ast.NodeTransformer):
    def __init__(self, grid_dims=None, tile_sizes=None):

        # tile sizes and grid ranks
        self.grid_dims = grid_dims
        self.tile_sizes = tile_sizes

        # Record the constant value
        self.constant_mapping = {}
        
        # Record the offset, size, step information from aries.arange
        self.dmaInfo = {} # dic["ti"] = (32*i, 32, 1) (offset, size, step)
        self.startExpr = None
        self.stopExpr = None
        self.stepExpr = sp.Integer(1) # Default step is 1 if not assigned

    def visit_FunctionDef(self, node):
        # Traverse function body and update
        node.body = [self.visit(statement) for statement in node.body]
        return node
    
    def generic_visit(self, node):
        return super().generic_visit(node)
    
    # Recursively converts a nested BinOp AST node into a SymPy expression
    def arange_to_sympy(self, node):
        if isinstance(node, ast.BinOp):
            left = self.arange_to_sympy(node.left)  # Recursively process left operand
            right = self.arange_to_sympy(node.right)  # Recursively process right operand

            if isinstance(node.op, ast.Add):
                return left + right
            elif isinstance(node.op, ast.Sub):
                return left - right
            elif isinstance(node.op, ast.Mult):
                return left * right
            elif isinstance(node.op, ast.Div):
                return left / right
            elif isinstance(node.op, ast.Mod):
                return left % right
            elif isinstance(node.op, ast.Pow):
                return left**right
            else:
                raise ValueError(f"Unsupported operation: {node.op}")

        elif isinstance(node, ast.Name):
            return sp.Symbol(node.id)  # Convert variable names to SymPy symbols
        elif isinstance(node, ast.Constant):
            return sp.Integer(node.value)  # Convert constants to SymPy Integers

        raise ValueError(f"Unsupported node type: {type(node)}")
    
    def visit_Assign(self, node):
        assert len(node.targets) == 1
        target = node.targets[0]
        
        if isinstance(node.value, ast.Call):
            assert isinstance(node.value.func, ast.Attribute)
            assert isinstance(node.value.func.value, ast.Name)
            assert node.value.func.value.id == 'aries'
            
            # TI, TJ, TK = tile_sizes()
            # Record the tile size which are constant values
            if node.value.func.attr == "tile_sizes":
                if isinstance(target, ast.Tuple):
                    assert len(target.elts) == len(self.tile_sizes)
                    for i, tsize in enumerate(self.tile_sizes):
                        self.constant_mapping[target.elts[i].id] = int(tsize)
                    return None
                else:
                    tsize = self.tile_sizes
                    self.constant_mapping[target.id] = int(tsize)
                    return None
            
            # Need to visit the nested nodes to propagate constants and 
            # record the arange Info
            elif node.value.func.attr == "arange":
                call = node.value
                # Recursively visit and propagate constants
                call.args = [self.visit(arg) for arg in call.args]
                self.startExpr = None
                self.stopExpr = None
                self.stepExpr = sp.Integer(1)
                # Record the arange information
                for idx, arg in enumerate(call.args):
                    if idx == 0: # Processes start
                        self.startExpr = self.arange_to_sympy(arg)
                    elif idx == 1: # Processes stop
                        self.stopExpr = self.arange_to_sympy(arg)
                    elif idx == 1: # Processes step
                        self.stepExpr = self.arange_to_sympy(arg)
                # offset = start, size = (stop-start)/step
                sizeExpr = (self.stopExpr-self.startExpr) / self.stepExpr
                coeffs = sizeExpr.as_coefficients_dict()
                for coeff in coeffs.values():
                    assert coeff.is_integer, f"Non-integer coefficient found: {coeff}"
                sp.simplify(self.startExpr)
                sp.simplify(sizeExpr)
                sp.simplify(self.stepExpr)
                self.dmaInfo[target.id] = (self.startExpr, sizeExpr, self.stepExpr)
                return node
            
            elif node.value.func.attr == "buffer":
                call = node.value
                # Recursively visit and propagate constants
                call.args = [self.visit(arg) for arg in call.args]
                return node
            
            elif node.value.func.attr in ["load", "store"]:
                return node
            
            else:
                raise ValueError(
                    f"Unsupported function call: {node.value.func.attr}")
        return node
    
    def visit_Name(self, node):
        # Replace variable names in constant_mapping with their corresponding values
        if node.id in self.constant_mapping:
            return ast.Constant(value=self.constant_mapping[node.id], kind=None)
        return node

    def visit_BinOp(self, node):
        # Recursively replace variable names inside binary operations
        node.left = self.visit(node.left)
        node.right = self.visit(node.right)
        return node

# =========== Schedule ===========
class Schedule:
    def __init__(self, *tasks):
        self.tasks = tasks
        self.flags = ""
        self.mlir_func_code = []
        self.mlir_map_code = [] # AffineMap should be defined outside of Module
        self.map_cnt = 0
    
    # A helper function to collect funcs from the given module
    def collect_func(self, module):
        funcs= []
        top_func_flag = False # Can only have one top function
        for _, obj in module.items():
            if isinstance(obj, TaskKernelWrapper):
                funcs.append(obj.func)
            elif isinstance(obj, TaskTileWrapper):
                funcs.append(obj.func)
            elif isinstance(obj, TaskTopWrapper):
                if top_func_flag == False:
                    top_func_flag = True
                    funcs.append(obj.func)
                else:
                    raise TypeError("Can only has one TaskTopWrapper")
        return funcs
    
    def task_tile_emit(self, func_name, parsed_ast):
        task = None
        # TODOs: Now assumes tasks has unique funcs 
        for task_temp in self.tasks:
            if task_temp.func.__name__ == func_name:
                task = task_temp
                break
            
        # Transform the tile ranks to index
        tileTrans = TileToLoop(task.grid_dims)
        tree = tileTrans.visit(parsed_ast)
        ast.fix_missing_locations(tree)
        # print("Parsed New AST 0", ast.dump(tree, indent=4))
        # print("\n\n\n=== Python AST 0 code ===")
        # print(astor.to_source(tree))
        
        # Collect dmaInfo and conduct constant propagation
        preInstance = TilePreprocess(task.grid_dims, task.tile_sizes)
        tree = preInstance.visit(tree)
        ast.fix_missing_locations(tree)
        dmaInfo = preInstance.dmaInfo
        # print("Parsed New AST 1", ast.dump(tree, indent=4))
        
        # Add func and map code
        func_code, map_code, self.map_cnt = TileMLIRGenerator(dmaInfo, self.map_cnt).generate(tree)
        self.mlir_func_code.append(func_code)
        self.mlir_map_code.append(map_code)
        # print(func_code)
    
    def task_kernel_emit(self, parsed_ast):
        print("Parsed AIE Kernel AST", ast.dump(parsed_ast, indent=4))
        func_code, map_code, self.map_cnt = KernelMLIRGenerator(None, self.map_cnt).generate(parsed_ast)
        self.mlir_func_code.append(func_code)
        self.mlir_map_code.append(map_code)
        # print(func_code)
    
    def task_top_emit(self, parsed_ast):
        # print("Parsed Top  AST", ast.dump(parsed_ast, indent=4))
        func_code, map_code, self.map_cnt = TopMLIRGenerator(None, self.map_cnt).generate(parsed_ast)
        self.mlir_func_code.append(func_code)
        self.mlir_map_code.append(map_code)
        # print(func_code)
    
    def code_emit(self):
        final_map_code = "\n".join(filter(None, self.mlir_map_code))
        final_func_code = "\n".join(filter(None, self.mlir_func_code))
        print(final_map_code)
        print("module {")
        print(final_func_code)
        print("}")
        return
    
    def build(self, module):
        funcs = self.collect_func(module)
        for func in funcs:
            source_code = inspect.getsource(func)
            parsed_ast = ast.parse(source_code)
            decorator_id = None
            func_name = None
            # Identify the decorators and remove the decorator_list
            for node in ast.walk(parsed_ast):
                if isinstance(node, ast.FunctionDef):
                    # Assume there's only one decorator for each func
                    decorator_id = node.decorator_list[0].func.id
                    func_name = node.name
                    break
            if decorator_id is None:
                continue
            if decorator_id == 'task_kernel':
                self.task_kernel_emit(parsed_ast)
                continue
            elif decorator_id == 'task_tile':
                self.task_tile_emit(func_name, parsed_ast)
                continue
            elif decorator_id == 'task_top':
                self.task_top_emit(parsed_ast)
                continue
            else:
                raise ValueError(f"Unsupported decorator: {decorator_id}")
        self.code_emit()