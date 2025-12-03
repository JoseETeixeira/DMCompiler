#pragma once

#include "DMASTStatement.h"
#include "BytecodeWriter.h"
#include "DMExpressionCompiler.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace DMCompiler {

class DMCompiler;
class DMProc;
class DMExpressionCompiler;

/// <summary>
/// Compiles DM statement AST nodes into bytecode
/// </summary>
class DMStatementCompiler {
public:
    DMStatementCompiler(DMCompiler* compiler, DMProc* proc, BytecodeWriter* writer, DMExpressionCompiler* exprCompiler);
    
    /// Compile a single statement
    bool CompileStatement(DMASTProcStatement* statement);
    
    /// Compile a block of statements
    bool CompileBlockInner(DMASTProcBlockInner* block);
    
    /// Finalize compilation by resolving forward references
    /// Must be called after all statements have been compiled
    bool Finalize();
    
private:
    DMCompiler* Compiler_;
    DMProc* Proc_;
    BytecodeWriter* Writer_;
    DMExpressionCompiler* ExprCompiler_;
    
    // Label generation counter
    int LabelCounter_ = 0;
    
    // Named label tracking (for label:/goto statements)
    std::unordered_map<std::string, std::string> NamedLabels_;
    std::unordered_set<std::string> DefinedLabels_;
    
    // Forward reference tracking for goto statements
    struct ForwardReference {
        std::string LabelName;      // The name of the label being referenced
        size_t BytecodePosition;    // Position in bytecode where the jump is
        std::string PlaceholderLabel; // The placeholder label that was created
    };
    std::vector<ForwardReference> ForwardReferences_;
    
    // Loop context for break/continue
    struct LoopContext {
        std::string StartLabel;
        std::string EndLabel;
        std::string ContinueLabel; // Where continue jumps to
        bool IsSwitch = false;
    };
    std::vector<LoopContext> LoopStack_;
    
    // ===== Label Management =====
    std::string NewLabel();
    void EmitLabel(const std::string& label);
    
    // ===== Jump Emission =====
    void EmitJump(const std::string& label);
    void EmitJumpIfFalse(const std::string& label);
    void EmitJumpIfTrue(const std::string& label);
    
    // ===== Statement Compilation Methods =====
    bool CompileExpression(DMASTProcStatementExpression* stmt);
    bool CompileReturn(DMASTProcStatementReturn* stmt);
    bool CompileIf(DMASTProcStatementIf* stmt);
    bool CompileWhile(DMASTProcStatementWhile* stmt);
    bool CompileDoWhile(DMASTProcStatementDoWhile* stmt);
    bool CompileFor(DMASTProcStatementFor* stmt);
    bool CompileForRange(DMASTProcStatementForRange* stmt);
    bool CompileForIn(DMASTProcStatementForIn* stmt);
    bool CompileSwitch(DMASTProcStatementSwitch* stmt);
    bool CompileBreak(DMASTProcStatementBreak* stmt);
    bool CompileContinue(DMASTProcStatementContinue* stmt);
    bool CompileVarDeclaration(DMASTProcStatementVarDeclaration* stmt);
    bool CompileDel(DMASTProcStatementDel* stmt);
    bool CompileSpawn(DMASTProcStatementSpawn* stmt);
    bool CompileLabel(DMASTProcStatementLabel* stmt);
    bool CompileGoto(DMASTProcStatementGoto* stmt);
    bool CompileSet(DMASTProcStatementSet* stmt);
    
    // ===== Helper Methods =====
    void PushLoopContext(const std::string& startLabel, const std::string& endLabel, const std::string& continueLabel, bool isSwitch = false);
    void PopLoopContext();
    LoopContext* GetCurrentLoop();
};

} // namespace DMCompiler
