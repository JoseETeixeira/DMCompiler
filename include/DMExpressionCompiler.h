#pragma once

#include "BytecodeWriter.h"
#include "DMASTExpression.h"
#include "DMProc.h"
#include "DMReference.h"
#include <memory>
#include <vector>

namespace DMCompiler {

// Forward declarations
class DMCompiler;
class DMObject;

/// <summary>
/// Compiles DM expressions to bytecode.
/// This is a simplified proof-of-concept implementation demonstrating
/// how AST nodes are converted to bytecode instructions.
/// 
/// Full compilation requires additional infrastructure:
/// - Scope management (DMProcScope)
/// - Type inference and checking
/// - Constant folding
/// - Reference tracking (local vs field access)
/// - List and object operations
/// </summary>
class DMExpressionCompiler {
public:
    DMExpressionCompiler(DMCompiler* compiler, DMProc* proc, BytecodeWriter* writer);
    
    /// <summary>
    /// Compile an expression to bytecode.
    /// The result is left on the stack.
    /// </summary>
    /// <returns>true if compilation succeeded, false if unsupported</returns>
    bool CompileExpression(DMASTExpression* expr);

    // Result of LValue analysis
    struct LValueInfo {
        enum class Kind { Local, Global, Field, Index, Invalid };
        Kind Type;
        std::vector<uint8_t> ReferenceBytes;  // For Local/Global/Field
        bool NeedsStackTarget;  // For Field/Index (target already on stack)
    };
    
    // Analyze an expression as an LValue
    LValueInfo ResolveLValue(DMASTExpression* expr);
    
private:
    DMCompiler* Compiler_;
    DMProc* Proc_;
    BytecodeWriter* Writer_;
    
    // Compile specific expression types
    bool CompileConstantInteger(DMASTConstantInteger* expr);
    bool CompileConstantFloat(DMASTConstantFloat* expr);
    bool CompileConstantString(DMASTConstantString* expr);
    bool CompileConstantNull(DMASTConstantNull* expr);
    bool CompileConstantPath(DMASTConstantPath* expr);
    
    bool CompileBinaryOp(DMASTExpressionBinary* expr);
    bool CompileUnaryOp(DMASTExpressionUnary* expr);
    bool CompileIdentifier(DMASTIdentifier* expr);
    bool CompileDereference(DMASTDereference* expr);
    bool CompileCall(DMASTCall* expr);
    bool CompileList(DMASTList* expr);
    bool CompileNewList(DMASTNewList* expr);
    bool CompileTernary(DMASTTernary* expr);
    bool CompileAssign(DMASTAssign* expr);
    bool CompileIncrementDecrement(DMASTExpressionUnary* expr);
    bool CompileNewPath(DMASTNewPath* expr);
    
    // Assignment helpers
    bool CompileLocalAssignment(const LValueInfo& lvalue, DMASTExpression* value, AssignmentOperator op);
    bool CompileGlobalAssignment(const LValueInfo& lvalue, DMASTExpression* value, AssignmentOperator op);
    bool CompileFieldAssignment(const LValueInfo& lvalue, DMASTExpression* value, AssignmentOperator op, DMASTExpression* lvalueExpr);
    bool CompileIndexAssignment(const LValueInfo& lvalue, DMASTExpression* value, AssignmentOperator op, DMASTExpression* lvalueExpr);
    
    // Call helpers
    bool CompileCallTarget(DMASTExpression* target);

    // Built-in functions
    bool CompileLocate(DMASTCall* expr);
    bool CompilePick(DMASTCall* expr);
    bool CompileInput(DMASTCall* expr);
    bool CompileRgb(DMASTCall* expr);
    bool CompileProb(DMASTCall* expr);
    bool CompileIsType(DMASTCall* expr);
    bool CompileGetDir(DMASTCall* expr);
    bool CompileGetStep(DMASTCall* expr);
    bool CompileLength(DMASTCall* expr);
    bool CompileSqrt(DMASTCall* expr);
    
    // Helper to get opcode for binary operator
    DreamProcOpcode GetBinaryOpcode(BinaryOperator op);
};

} // namespace DMCompiler
