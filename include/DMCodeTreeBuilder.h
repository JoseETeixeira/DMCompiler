#pragma once

#include "DreamPath.h"
#include "Location.h"
#include <memory>
#include <string>

namespace DMCompiler {

// Forward declarations
class DMCompiler;
class DMASTFile;
class DMASTStatement;
class DMASTObjectStatement;
class DMObjectTree;

/// <summary>
/// Builds the code tree from the AST by processing all definitions
/// and adding them to the object tree in the correct order
/// </summary>
class DMCodeTreeBuilder {
public:
    explicit DMCodeTreeBuilder(DMCompiler* compiler);
    ~DMCodeTreeBuilder();

    /// <summary>
    /// Main entry point - processes the AST file and builds the object tree
    /// </summary>
    void BuildCodeTree(DMASTFile* astFile);

private:
    DMCompiler* Compiler_;
    DMObjectTree* ObjectTree_;
    bool LeftDMStandard_;
    bool DMStandardFinalized_;

    /// <summary>
    /// Finalize DMStandard processing by marking all existing objects as from DMStandard.
    /// Called when transitioning from DMStandard files to user code.
    /// </summary>
    void FinalizeDMStandard();

    /// <summary>
    /// Processes all statements in the AST
    /// </summary>
    void ProcessStatements(const std::vector<std::unique_ptr<DMASTStatement>>& statements, const DreamPath& currentType);

    /// <summary>
    /// Processes a single statement and adds it to the object tree
    /// </summary>
    void ProcessStatement(DMASTStatement* statement, const DreamPath& currentType);
};

} // namespace DMCompiler
