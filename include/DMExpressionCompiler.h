#pragma once

#include "BytecodeWriter.h"
#include "DMASTExpression.h"
#include "DMProc.h"
#include "DMReference.h"
#include "DreamPath.h"
#include <memory>
#include <optional>
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
        bool IsConst = false;   // Is this LValue a const variable?
        std::optional<DreamPath> ResolvedType;  // The resolved type of this LValue (for type inference)
    };
    
    /// <summary>
    /// Result of compiling call arguments, including named argument support.
    /// Used by CompileCallArguments() to return comprehensive argument info.
    /// </summary>
    struct CallArgumentsResult {
        int positionalCount = 0;     ///< Number of positional arguments
        int namedCount = 0;          ///< Number of named arguments
        int totalCount = 0;          ///< positionalCount + namedCount
        DMCallArgumentsType argsType = DMCallArgumentsType::None;  ///< None, FromStack, or FromStackKeyed
        bool success = true;         ///< Compilation succeeded
    };
    
    // Analyze an expression as an LValue
    LValueInfo ResolveLValue(DMASTExpression* expr);
    
    /// <summary>
    /// Set the expected type for type inference in bare 'new' expressions.
    /// Call this before compiling an expression that may contain a bare 'new'.
    /// </summary>
    /// <param name="type">The expected type, or std::nullopt to clear</param>
    void SetExpectedType(std::optional<DreamPath> type);
    
    /// <summary>
    /// Get the current expected type for type inference.
    /// </summary>
    /// <returns>The expected type, or std::nullopt if not set</returns>
    std::optional<DreamPath> GetExpectedType() const;
    
private:
    DMCompiler* Compiler_;
    DMProc* Proc_;
    BytecodeWriter* Writer_;
    std::optional<DreamPath> ExpectedType_;  // Expected type for bare 'new' inference
    
    // Compile specific expression types
    bool CompileConstantInteger(DMASTConstantInteger* expr);
    bool CompileConstantFloat(DMASTConstantFloat* expr);
    bool CompileConstantString(DMASTConstantString* expr);
    bool CompileConstantResource(DMASTConstantResource* expr);
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
    bool CompileStringFormat(DMASTStringFormat* expr);
    
    // Assignment helpers
    bool CompileLocalAssignment(const LValueInfo& lvalue, DMASTExpression* value, AssignmentOperator op);
    bool CompileGlobalAssignment(const LValueInfo& lvalue, DMASTExpression* value, AssignmentOperator op);
    bool CompileFieldAssignment(const LValueInfo& lvalue, DMASTExpression* value, AssignmentOperator op, DMASTExpression* lvalueExpr);
    bool CompileIndexAssignment(const LValueInfo& lvalue, DMASTExpression* value, AssignmentOperator op, DMASTExpression* lvalueExpr);
    
    // Call helpers
    bool CompileCallTarget(DMASTExpression* target);
    
    /// <summary>
    /// Compile call arguments, handling both positional and named arguments.
    /// Emits positional arguments first, then named arguments as (key_string, value) pairs.
    /// </summary>
    /// <param name="params">The call parameters to compile</param>
    /// <returns>CallArgumentsResult with counts, argument type, and success status</returns>
    CallArgumentsResult CompileCallArguments(
        const std::vector<std::unique_ptr<DMASTCallParameter>>& params);

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
    bool CompileMathOp(DMASTCall* expr, DreamProcOpcode opcode);
    
    // Helper to get opcode for binary operator
    DreamProcOpcode GetBinaryOpcode(BinaryOperator op);
    
    /// <summary>
    /// Resolve the type of an expression based on static analysis.
    /// Used for type inference in assignments and dereferences.
    /// Does NOT compile the expression - only analyzes for type.
    /// </summary>
    /// <param name="expr">The expression to analyze</param>
    /// <returns>The resolved type path, or std::nullopt if unknown</returns>
    std::optional<DreamPath> ResolveExpressionType(DMASTExpression* expr);
};

} // namespace DMCompiler
