#include "DMASTFolder.h"
#include "DMAST.h"
#include "DMASTStatement.h"
#include "DMASTExpression.h"
#include <memory>

namespace DMCompiler {

void DMASTFolder::FoldAst(DMASTNode* ast) {
    if (!ast) return;

    // Handle different node types
    if (auto* file = dynamic_cast<DMASTFile*>(ast)) {
        for (auto& statement : file->Statements) {
            FoldAst(statement.get());
        }
    }
    else if (auto* objectDef = dynamic_cast<DMASTObjectDefinition*>(ast)) {
        for (auto& statement : objectDef->InnerStatements) {
            FoldAst(statement.get());
        }
    }
    else if (auto* objectVarDef = dynamic_cast<DMASTObjectVarDefinition*>(ast)) {
        objectVarDef->Value = FoldExpression(std::move(objectVarDef->Value));
    }
    else if (auto* objectVarOverride = dynamic_cast<DMASTObjectVarOverride*>(ast)) {
        objectVarOverride->Value = FoldExpression(std::move(objectVarOverride->Value));
    }
    else if (auto* procExpr = dynamic_cast<DMASTProcStatementExpression*>(ast)) {
        procExpr->Expression = FoldExpression(std::move(procExpr->Expression));
    }
    else if (auto* procRet = dynamic_cast<DMASTProcStatementReturn*>(ast)) {
        procRet->Value = FoldExpression(std::move(procRet->Value));
    }
    else if (auto* procDel = dynamic_cast<DMASTProcStatementDel*>(ast)) {
        procDel->Value = FoldExpression(std::move(procDel->Value));
    }
    else if (auto* procThrow = dynamic_cast<DMASTProcStatementThrow*>(ast)) {
        procThrow->Value = FoldExpression(std::move(procThrow->Value));
    }
    else if (auto* procVarDecl = dynamic_cast<DMASTProcStatementVarDeclaration*>(ast)) {
        for (auto& decl : procVarDecl->Decls) {
            decl.Value = FoldExpression(std::move(decl.Value));
        }
    }
    else if (auto* procBlockInner = dynamic_cast<DMASTProcBlockInner*>(ast)) {
        for (auto& statement : procBlockInner->Statements) {
            FoldAst(statement.get());
        }
    }
    else if (auto* procDef = dynamic_cast<DMASTObjectProcDefinition*>(ast)) {
        for (auto& param : procDef->Parameters) {
            param->DefaultValue = FoldExpression(std::move(param->DefaultValue));
            param->PossibleValues = FoldExpression(std::move(param->PossibleValues));
        }
        if (procDef->Body) {
            FoldAst(procDef->Body.get());
        }
    }
    else if (auto* statementIf = dynamic_cast<DMASTProcStatementIf*>(ast)) {
        statementIf->Condition = FoldExpression(std::move(statementIf->Condition));
        if (statementIf->Body) {
            FoldAst(statementIf->Body.get());
        }
        if (statementIf->ElseBody) {
            FoldAst(statementIf->ElseBody.get());
        }
    }
    else if (auto* statementFor = dynamic_cast<DMASTProcStatementFor*>(ast)) {
        // Initializer is a statement, so fold it recursively
        if (statementFor->Initializer) {
            FoldAst(statementFor->Initializer.get());
        }
        statementFor->Condition = FoldExpression(std::move(statementFor->Condition));
        statementFor->Increment = FoldExpression(std::move(statementFor->Increment));
        if (statementFor->Body) {
            FoldAst(statementFor->Body.get());
        }
    }
    else if (auto* statementWhile = dynamic_cast<DMASTProcStatementWhile*>(ast)) {
        statementWhile->Condition = FoldExpression(std::move(statementWhile->Condition));
        if (statementWhile->Body) {
            FoldAst(statementWhile->Body.get());
        }
    }
    else if (auto* statementDoWhile = dynamic_cast<DMASTProcStatementDoWhile*>(ast)) {
        statementDoWhile->Condition = FoldExpression(std::move(statementDoWhile->Condition));
        if (statementDoWhile->Body) {
            FoldAst(statementDoWhile->Body.get());
        }
    }
    else if (auto* statementSwitch = dynamic_cast<DMASTProcStatementSwitch*>(ast)) {
        statementSwitch->Value = FoldExpression(std::move(statementSwitch->Value));
        for (auto& switchCase : statementSwitch->Cases) {
            // Fold values in case
            for (auto& value : switchCase.Values) {
                value = FoldExpression(std::move(value));
            }
            if (switchCase.Body) {
                FoldAst(switchCase.Body.get());
            }
        }
    }
    else if (auto* statementSpawn = dynamic_cast<DMASTProcStatementSpawn*>(ast)) {
        statementSpawn->Delay = FoldExpression(std::move(statementSpawn->Delay));
        if (statementSpawn->Body) {
            FoldAst(statementSpawn->Body.get());
        }
    }
    else if (auto* tryCatch = dynamic_cast<DMASTProcStatementTryCatch*>(ast)) {
        if (tryCatch->TryBody) {
            FoldAst(tryCatch->TryBody.get());
        }
        if (tryCatch->CatchBody) {
            FoldAst(tryCatch->CatchBody.get());
        }
    }
}

std::unique_ptr<DMASTExpression> DMASTFolder::FoldExpression(std::unique_ptr<DMASTExpression> expression) {
    if (!expression) return expression;

    // Recursively fold sub-expressions first
    if (auto* unary = dynamic_cast<DMASTExpressionUnary*>(expression.get())) {
        unary->Expression = FoldExpression(std::move(unary->Expression));
        
        // Perform constant folding for unary operations
        if (unary->Operator == UnaryOperator::Negate) {
            if (auto* constInt = dynamic_cast<DMASTConstantInteger*>(unary->Expression.get())) {
                return std::make_unique<DMASTConstantInteger>(expression->Location_, -constInt->Value);
            }
            else if (auto* constFloat = dynamic_cast<DMASTConstantFloat*>(unary->Expression.get())) {
                return std::make_unique<DMASTConstantFloat>(expression->Location_, -constFloat->Value);
            }
        }
        else if (unary->Operator == UnaryOperator::Not) {
            if (auto* constInt = dynamic_cast<DMASTConstantInteger*>(unary->Expression.get())) {
                return std::make_unique<DMASTConstantInteger>(expression->Location_, constInt->Value != 0 ? 0 : 1);
            }
            else if (auto* constFloat = dynamic_cast<DMASTConstantFloat*>(unary->Expression.get())) {
                return std::make_unique<DMASTConstantInteger>(expression->Location_, constFloat->Value != 0.0f ? 0 : 1);
            }
        }
    }
    else if (auto* binary = dynamic_cast<DMASTExpressionBinary*>(expression.get())) {
        binary->Left = FoldExpression(std::move(binary->Left));
        binary->Right = FoldExpression(std::move(binary->Right));
        
        // Perform constant folding for logical operations
        if (binary->Operator == BinaryOperator::LogicalOr) {
            auto simpleTruth = SimpleTruth(binary->Left.get());
            if (simpleTruth.has_value()) {
                if (*simpleTruth) {
                    return std::move(binary->Left); // Left side is truthy, return it
                } else {
                    return std::move(binary->Right); // Left side is falsy, return right
                }
            }
        }
        else if (binary->Operator == BinaryOperator::LogicalAnd) {
            auto simpleTruth = SimpleTruth(binary->Left.get());
            if (simpleTruth.has_value()) {
                if (!*simpleTruth) {
                    return std::move(binary->Left); // Left side is falsy, return it
                } else {
                    return std::move(binary->Right); // Left side is truthy, return right
                }
            }
        }
    }

    // Handle specific expression types for recursive folding
    if (auto* list = dynamic_cast<DMASTList*>(expression.get())) {
        for (auto& param : list->Values) {
            param->Value = FoldExpression(std::move(param->Value));
        }
    }
    else if (auto* newlist = dynamic_cast<DMASTNewList*>(expression.get())) {
        for (auto& param : newlist->Parameters) {
            param->Value = FoldExpression(std::move(param->Value));
        }
    }
    else if (auto* newPath = dynamic_cast<DMASTNewPath*>(expression.get())) {
        for (auto& param : newPath->Parameters) {
            param->Value = FoldExpression(std::move(param->Value));
        }
    }
    else if (auto* call = dynamic_cast<DMASTCall*>(expression.get())) {
        for (auto& param : call->Parameters) {
            param->Value = FoldExpression(std::move(param->Value));
        }
    }
    else if (auto* ternary = dynamic_cast<DMASTTernary*>(expression.get())) {
        ternary->Condition = FoldExpression(std::move(ternary->Condition));
        ternary->TrueExpression = FoldExpression(std::move(ternary->TrueExpression));
        ternary->FalseExpression = FoldExpression(std::move(ternary->FalseExpression));
    }
    else if (auto* switchCaseRange = dynamic_cast<DMASTSwitchCaseRange*>(expression.get())) {
        switchCaseRange->RangeStart = FoldExpression(std::move(switchCaseRange->RangeStart));
        switchCaseRange->RangeEnd = FoldExpression(std::move(switchCaseRange->RangeEnd));
    }

    return expression;
}

std::optional<bool> DMASTFolder::SimpleTruth(DMASTExpression* expr) {
    if (!expr) return std::nullopt;

    if (auto* constInt = dynamic_cast<DMASTConstantInteger*>(expr)) {
        return constInt->Value != 0;
    }
    else if (auto* constFloat = dynamic_cast<DMASTConstantFloat*>(expr)) {
        return constFloat->Value != 0.0f;
    }
    else if (auto* constString = dynamic_cast<DMASTConstantString*>(expr)) {
        return !constString->Value.empty();
    }
    else if (dynamic_cast<DMASTConstantNull*>(expr)) {
        return false;
    }
    else if (dynamic_cast<DMASTConstantPath*>(expr)) {
        return true;
    }
    else if (dynamic_cast<DMASTConstantResource*>(expr)) {
        return true;
    }

    return std::nullopt; // Cannot determine at compile time
}

} // namespace DMCompiler
