#include "DMCodeTreeBuilder.h"
#include "DMCompiler.h"
#include "DMAST.h"
#include "DMASTStatement.h"
#include "DMObjectTree.h"
#include "DMObject.h"
#include "DMProc.h"
#include "DMVariable.h"
#include "DMASTExpression.h"
#include <iostream>

namespace DMCompiler {

DMCodeTreeBuilder::DMCodeTreeBuilder(DMCompiler* compiler)
    : Compiler_(compiler)
    , ObjectTree_(compiler->GetObjectTree())
    , LeftDMStandard_(false)
    , DMStandardFinalized_(false)
{
}

DMCodeTreeBuilder::~DMCodeTreeBuilder() = default;

void DMCodeTreeBuilder::BuildCodeTree(DMASTFile* astFile) {
    if (!astFile) {
        Compiler_->ForcedError(Location::Internal, "No AST file provided to code tree builder");
        return;
    }

    LeftDMStandard_ = false;
    DMStandardFinalized_ = false;

    if (Compiler_->GetSettings().Verbose) {
        std::cout << "  Building code tree from " << astFile->Statements.size() << " statements..." << std::endl;
    }

    // Add everything in the AST to the object tree
    ProcessStatements(astFile->Statements, DreamPath::Root);

    // Create each type's initialization proc (initializes vars that aren't constants)
    if (Compiler_->GetSettings().Verbose) {
        std::cout << "  Creating initialization procs..." << std::endl;
    }
    
    for (auto* dmObject : ObjectTree_->GetAllObjects()) {
        dmObject->CreateInitializationProc(Compiler_, ObjectTree_);
    }

    // Compile every proc
    if (Compiler_->GetSettings().Verbose) {
        std::cout << "  Compiling procs..." << std::endl;
    }
    
    for (auto* proc : ObjectTree_->GetAllProcs()) {
        proc->Compile(Compiler_);
    }
}

void DMCodeTreeBuilder::ProcessStatements(const std::vector<std::unique_ptr<DMASTStatement>>& statements, const DreamPath& currentType) {
    for (const auto& statement : statements) {
        ProcessStatement(statement.get(), currentType);
    }
}

void DMCodeTreeBuilder::ProcessStatement(DMASTStatement* statement, const DreamPath& currentType) {
    ProcessStatementWithVarContext(statement, currentType, std::nullopt);
}

void DMCodeTreeBuilder::ProcessStatementWithVarContext(DMASTStatement* statement, const DreamPath& currentType, std::optional<DreamPath> varBlockType) {
    if (!statement) return;

    // Track when we leave DMStandard
    if (!LeftDMStandard_ && !statement->Location_.InDMStandard) {
        LeftDMStandard_ = true;
        FinalizeDMStandard();
    }

    // Object definition: /mob/player { ... }
    if (auto* objectDef = dynamic_cast<DMASTObjectDefinition*>(statement)) {
        DreamPath typePath = objectDef->Path.Path;
        
        // Convert relative paths to absolute paths
        // Root-level relative paths like "mob" should become "/mob"
        if (typePath.GetPathType() == DreamPath::PathType::Relative) {
            // Combine with current type to get absolute path
            typePath = currentType.Combine(typePath);
        }
        
        // Check if this is a "var" block (path ends with "var")
        // In this case, we don't create a type, we just process the inner statements
        // with the type path (without "var")
        bool isVarBlock = false;
        DreamPath innerType = typePath;
        auto elements = typePath.GetElements();
        
        if (!elements.empty() && elements.back() == "var") {
            isVarBlock = true;
            // Remove "var" from the path to get the actual type
            innerType = typePath.RemoveLastElement();
            if (Compiler_->GetSettings().Verbose) {
                std::cout << "  Processing var block at: " << typePath.ToString() 
                         << " (type: " << innerType.ToString() << ")" << std::endl;
            }
            // Process inner statements with var block context (no accumulated type yet)
            for (const auto& innerStmt : objectDef->InnerStatements) {
                ProcessStatementWithVarContext(innerStmt.get(), innerType, DreamPath(DreamPath::PathType::Absolute, {}));
            }
        } else if (varBlockType.has_value()) {
            // We're inside a var block context, and this is a type specifier (like "list")
            // Don't create an object type for this - it's a type path for variables
            // Accumulate the type path
            DreamPath accumulatedType = varBlockType.value().Combine(objectDef->Path.Path);
            
            if (Compiler_->GetSettings().Verbose) {
                std::cout << "  Inside var block, accumulated type: " << accumulatedType.ToString() << std::endl;
            }
            
            // Process inner statements with the accumulated type
            for (const auto& innerStmt : objectDef->InnerStatements) {
                ProcessStatementWithVarContext(innerStmt.get(), currentType, accumulatedType);
            }
        } else {
            // Normal object definition - not in var block context
            // Add the type to the object tree
            ObjectTree_->AddType(typePath);
            
            for (const auto& innerStmt : objectDef->InnerStatements) {
                ProcessStatementWithVarContext(innerStmt.get(), innerType, std::nullopt);
            }
        }
        return;
    }
    // Variable definition: var/name = value
    else if (auto* varDef = dynamic_cast<DMASTObjectVarDefinition*>(statement)) {
        // If we're in a var block context, apply the accumulated type
        std::optional<DreamPath> effectiveType;
        
        if (!varDef->TypePath.Path.GetElements().empty()) {
            // TypePath is set - convert to absolute if relative
            if (varDef->TypePath.Path.GetPathType() == DreamPath::PathType::Relative) {
                // Make it absolute by combining with root
                effectiveType = DreamPath(DreamPath::PathType::Absolute, varDef->TypePath.Path.GetElements());
            } else {
                effectiveType = varDef->TypePath.Path;
            }
        } else if (varBlockType.has_value() && !varBlockType->GetElements().empty()) {
            effectiveType = varBlockType;
        }
        
        // Check for DMStandard modification
        if (DMStandardFinalized_) {
            DMObject* obj = ObjectTree_->GetType(currentType);
            if (obj && obj->IsFromDMStandard) {
                Compiler_->Emit(WarningCode::DMStandardModification, varDef->Location_, 
                    "Modifying DMStandard type '" + currentType.ToString() + "' by adding variable '" + varDef->Name + "'");
            }
        }

        // Add the variable to the current type
        // The variable's name is stored in varDef->Name
        // The variable's type constraint (if any) is in varDef->TypePath (or from var block context)
        ObjectTree_->AddType(currentType);
        ObjectTree_->AddObjectVar(currentType, varDef, effectiveType);
    }
    // Variable override: existing_var = new_value
    else if (auto* varOverride = dynamic_cast<DMASTObjectVarOverride*>(statement)) {
        // Check for DMStandard modification
        if (DMStandardFinalized_) {
            DMObject* obj = ObjectTree_->GetType(currentType);
            if (obj && obj->IsFromDMStandard) {
                Compiler_->Emit(WarningCode::DMStandardModification, varOverride->Location_, 
                    "Modifying DMStandard type '" + currentType.ToString() + "' by overriding variable '" + varOverride->VarName + "'");
            }
        }

        // Check for FinalVarOverride
        DMObject* currentObj = ObjectTree_->GetType(currentType);
        if (currentObj) {
            DMObject* search = currentObj;
            while (search) {
                auto it = search->Variables.find(varOverride->VarName);
                if (it != search->Variables.end()) {
                    if (it->second.IsFinal) {
                        Compiler_->Emit(WarningCode::FinalVarOverride, varOverride->Location_, 
                            "Cannot override final variable '" + varOverride->VarName + "'");
                    }
                    break;
                }
                search = search->Parent;
            }
        }

        // Variable overrides apply to the current type
        ObjectTree_->AddType(currentType);
        ObjectTree_->AddObjectVarOverride(currentType, varOverride);
    }
    // Proc definition: proc/test() { ... }
    else if (auto* procDef = dynamic_cast<DMASTObjectProcDefinition*>(statement)) {
        DreamPath procOwner = currentType.Combine(procDef->ObjectPath);
        
        // Check for DMStandard modification
        if (DMStandardFinalized_) {
            DMObject* obj = ObjectTree_->GetType(procOwner);
            if (obj && obj->IsFromDMStandard) {
                Compiler_->Emit(WarningCode::DMStandardModification, procDef->Location_, 
                    "Modifying DMStandard type '" + procOwner.ToString() + "' by adding/overriding proc '" + procDef->Name + "'");
            }
        }

        // Add the type that owns this proc
        ObjectTree_->AddType(procOwner);
        
        // Add the proc to the object tree
        DMProc* proc = ObjectTree_->AddProc(procOwner, procDef);

        if (proc) {
            // Register parameters as local variables immediately
            // This ensures parameters are available for identifier resolution during compilation
            for (const auto& param : procDef->Parameters) {
                std::optional<DreamPath> paramType;
                if (!param->TypePath.GetElements().empty()) {
                    paramType = param->TypePath;
                }
                
                proc->AddParameter(param->Name, paramType, param->ExplicitValueType);
            }
        }
    }
    // Other statement types are ignored (proc-level statements shouldn't be here)
    else {
        // This could be a proc-level statement at the wrong level
        // Just skip it - errors should have been reported during parsing
    }
}

void DMCodeTreeBuilder::FinalizeDMStandard() {
    if (DMStandardFinalized_) {
        return;  // Already finalized
    }
    
    DMStandardFinalized_ = true;
    
    if (Compiler_->GetSettings().Verbose) {
        std::cout << "  Finalizing DMStandard (marking " 
                  << ObjectTree_->AllObjects.size() << " objects as from DMStandard)..." << std::endl;
    }
    
    // Mark all existing objects as being from DMStandard
    ObjectTree_->SetDMStandardFinalized();
}

} // namespace DMCompiler
