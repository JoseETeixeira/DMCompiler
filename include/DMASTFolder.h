#pragma once

#include "DMAST.h"
#include <memory>
#include <optional>

namespace DMCompiler {

/// <summary>
/// AST post-processor that performs constant folding and expression simplification
/// Walks the AST tree and attempts to fold constant expressions at compile time
/// </summary>
class DMASTFolder {
public:
    DMASTFolder() = default;
    ~DMASTFolder() = default;

    /// <summary>
    /// Main entry point - folds all expressions in an AST node tree
    /// </summary>
    void FoldAst(DMASTNode* ast);

private:
    /// <summary>
    /// Folds constant expressions, returns new folded expression or original
    /// </summary>
    std::unique_ptr<DMASTExpression> FoldExpression(std::unique_ptr<DMASTExpression> expression);

    /// <summary>
    /// Determines if an expression can be evaluated to a simple boolean truth value
    /// Returns true, false, or nullopt if cannot be determined at compile time
    /// </summary>
    static std::optional<bool> SimpleTruth(DMASTExpression* expr);
};

} // namespace DMCompiler
