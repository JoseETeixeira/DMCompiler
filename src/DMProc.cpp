// Additional stub files
#include "DMProc.h"
#include "DMObject.h"
#include "DMAST.h"
#include "DMCompiler.h"
#include "DMObjectTree.h"
#include "BytecodeWriter.h"
#include "DMExpressionCompiler.h"
#include "DMStatementCompiler.h"
#include "DreamProcOpcode.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace DMCompiler {

// ========================================
// LocalVariable Implementation
// ========================================

LocalVariable::LocalVariable(
    std::string name,
    int id,
    bool isParameter,
    std::optional<DreamPath> type,
    std::optional<DMComplexValueType> explicitValueType
)
    : Name(std::move(name))
    , Id(id)
    , IsParameter(isParameter)
    , Type(type)
    , ExplicitValueType(explicitValueType)
{
}

// ========================================
// LocalConstVariable Implementation
// ========================================

LocalConstVariable::LocalConstVariable(
    std::string name,
    int id,
    std::optional<DreamPath> type,
    Constant value
)
    : LocalVariable(std::move(name), id, false, type, std::nullopt)
    , ConstValue(std::move(value))
{
}

// ========================================
// DMProc Implementation
// ========================================

DMProc::DMProc(
    int id,
    std::string name,
    DMObject* owningObject,
    bool isVerb,
    Location location
)
    : Id(id)
    , Name(std::move(name))
    , OwningObject(owningObject)
    , IsVerb(isVerb)
    , IsFinal(false)
    , Attributes(ProcAttributes::None)
    , MaxStackSize(0)
    , Invisibility(0)
    , SourceLocation(location)
    , LocalVariableIdCounter_(0)
{
}

LocalVariable* DMProc::AddParameter(
    const std::string& name,
    std::optional<DreamPath> type,
    std::optional<DMComplexValueType> explicitValueType
) {
    // Check if already exists
    if (LocalVariables.find(name) != LocalVariables.end()) {
        return nullptr; // Parameter already exists
    }
    
    // Add to parameters list
    Parameters.push_back(name);
    
    // Create the local variable
    auto var = std::make_unique<LocalVariable>(
        name,
        LocalVariableIdCounter_++,
        true, // isParameter
        type,
        explicitValueType
    );
    
    auto* varPtr = var.get();
    LocalVariables[name] = std::move(var);
    
    return varPtr;
}

LocalVariable* DMProc::AddLocalVariable(
    const std::string& name,
    std::optional<DreamPath> type
) {
    // Check if already exists
    if (LocalVariables.find(name) != LocalVariables.end()) {
        return nullptr; // Variable already exists
    }
    
    // Create the local variable
    auto var = std::make_unique<LocalVariable>(
        name,
        LocalVariableIdCounter_++,
        false, // not a parameter
        type,
        std::nullopt
    );
    
    auto* varPtr = var.get();
    LocalVariables[name] = std::move(var);
    
    return varPtr;
}

LocalConstVariable* DMProc::AddLocalConst(
    const std::string& name,
    std::optional<DreamPath> type,
    Constant value
) {
    // Check if already exists
    if (LocalVariables.find(name) != LocalVariables.end()) {
        return nullptr; // Variable already exists
    }
    
    // Create the const local variable
    auto var = std::make_unique<LocalConstVariable>(
        name,
        LocalVariableIdCounter_++,
        type,
        std::move(value)
    );
    
    auto* varPtr = var.get();
    LocalVariables[name] = std::move(var);
    
    return varPtr;
}

LocalVariable* DMProc::GetLocalVariable(const std::string& name) {
    auto it = LocalVariables.find(name);
    if (it != LocalVariables.end()) {
        return it->second.get();
    }
    return nullptr;
}

const LocalVariable* DMProc::GetLocalVariable(const std::string& name) const {
    auto it = LocalVariables.find(name);
    if (it != LocalVariables.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool DMProc::HasParameter(const std::string& name) const {
    return std::find(Parameters.begin(), Parameters.end(), name) != Parameters.end();
}

int DMProc::GetLocalVariableCount() const {
    return static_cast<int>(LocalVariables.size()) - static_cast<int>(Parameters.size());
}

void DMProc::MarkUnsupported(const std::string& reason) {
    UnsupportedReason = reason;
}

void DMProc::ApplySetAttribute(const std::string& attribute, const std::string& value, std::optional<bool> boolValue) {
    std::string attrLower = attribute;
    std::transform(attrLower.begin(), attrLower.end(), attrLower.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    SetAttributes[attrLower] = value;

    auto asBool = [&]() -> bool {
        if (boolValue.has_value()) {
            return boolValue.value();
        }

        std::string valueLower = value;
        std::transform(valueLower.begin(), valueLower.end(), valueLower.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

        if (valueLower == "true" || valueLower == "yes" || valueLower == "1") {
            return true;
        }

        if (valueLower == "false" || valueLower == "no" || valueLower == "0" || valueLower.empty()) {
            return false;
        }

        return false;
    };

    auto updateFlag = [&](ProcAttributes flag, bool enabled) {
        uint8_t raw = static_cast<uint8_t>(Attributes);
        const uint8_t flagValue = static_cast<uint8_t>(flag);

        if (enabled) {
            raw |= flagValue;
        } else {
            raw &= static_cast<uint8_t>(~flagValue);
        }

        Attributes = static_cast<ProcAttributes>(raw);
    };

    if (attrLower == "name") {
        VerbName = value;
    } else if (attrLower == "category") {
        VerbCategory = value;
    } else if (attrLower == "desc" || attrLower == "description") {
        VerbDescription = value;
    } else if (attrLower == "background") {
        updateFlag(ProcAttributes::Background, asBool());
    } else if (attrLower == "hidden") {
        updateFlag(ProcAttributes::Hidden, asBool());
    } else if (attrLower == "popup") {
        updateFlag(ProcAttributes::Popup, asBool());
    } else if (attrLower == "instant") {
        updateFlag(ProcAttributes::Instant, asBool());
    } else if (attrLower == "waitfor") {
        updateFlag(ProcAttributes::WaitFor, asBool());
    } else if (attrLower == "opendream_unimplemented") {
        if (!UnsupportedReason.has_value()) {
            if (boolValue.has_value()) {
                if (boolValue.value()) {
                    MarkUnsupported("Marked as opendream_unimplemented");
                }
            } else if (!value.empty()) {
                MarkUnsupported(value);
            } else {
                MarkUnsupported("Marked as opendream_unimplemented");
            }
        }
    } else if (attrLower == "opendream_unsupported") {
        if (!UnsupportedReason.has_value()) {
            if (!value.empty()) {
                MarkUnsupported(value);
            } else if (asBool()) {
                MarkUnsupported("Marked as opendream_unsupported");
            }
        }
    }
}

void DMProc::SetVerbSource(VerbSrc source) {
    VerbSource = source;
}

std::string DMProc::ToString() const {
    std::ostringstream oss;
    
    // Get the full path from owning object
    if (OwningObject) {
        oss << OwningObject->Path.ToString();
    } else {
        oss << "/unknown";
    }
    
    // Add proc or verb keyword
    oss << (IsVerb ? "/verb/" : "/proc/");
    
    // Add proc name
    oss << Name;
    
    // Add parameters
    oss << "(";
    for (size_t i = 0; i < Parameters.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << Parameters[i];
        
        // Add type if available
        auto* localVar = GetLocalVariable(Parameters[i]);
        if (localVar && localVar->Type.has_value()) {
            oss << " as " << localVar->Type->ToString();
        }
    }
    oss << ")";
    
    return oss.str();
}

void DMProc::Compile(DMCompiler* compiler) {
    // Skip if already compiled or marked as unsupported
    if (IsUnsupported()) {
        return;
    }
    
    // Skip if already has bytecode
    if (!Bytecode.empty()) {
        return;
    }
    
    // Initialization procs have null AstBody - they're created dynamically
    if (AstBody == nullptr && Name == "__init__") {
        // For initialization procs, we need to:
        // 1. Call parent's init proc if it exists
        // 2. Compile variable initializations from OwningObject
        
        if (!OwningObject) {
            return;
        }
        
        BytecodeWriter writer;
        DMExpressionCompiler exprCompiler(compiler, this, &writer);
        
        // 1. Call parent's init proc if it exists
        if (OwningObject->Parent != nullptr && OwningObject->Parent->InitializationProc != -1) {
            // Call ..() to invoke parent's __init__ proc
            // SuperProc reference type is 7 (DMReference::Type::SuperProc)
            writer.Emit(DreamProcOpcode::CallStatement);
            writer.AppendByte(7);  // DMReference::Type::SuperProc
            writer.AppendByte(static_cast<uint8_t>(DMCallArgumentsType::None));
            writer.AppendInt(0);  // No arguments
            
            // CallStatement pushes result, but we don't need it - pop it
            writer.Emit(DreamProcOpcode::Pop);
        }
        
        // 2. Emit bytecode to evaluate and assign each variable's Value expression
        // Process Variables (defined on this type)
        for (const auto& [varName, variable] : OwningObject->Variables) {
            if (variable.Value == nullptr) {
                continue;  // No initialization needed
            }
            
            // Set expected type for bare 'new' inference
            exprCompiler.SetExpectedType(variable.Type);
            
            // Compile the initializer expression (pushes value onto stack)
            if (!exprCompiler.CompileExpression(variable.Value)) {
                exprCompiler.SetExpectedType(std::nullopt);
                compiler->ForcedWarning("Failed to compile initializer for variable '" + varName + "'");
                continue;
            }
            
            // Clear expected type after compilation
            exprCompiler.SetExpectedType(std::nullopt);
            
            // Emit assignment to src.varName
            // We need to assign to the instance variable on src
            // Use SrcField reference type
            DMObjectTree* objectTree = compiler->GetObjectTree();
            int stringId = objectTree->AddString(varName);
            
            // Emit Assign opcode with SrcField reference
            writer.Emit(DreamProcOpcode::Assign);
            writer.AppendByte(static_cast<uint8_t>(DMReference::Type::SrcField));
            writer.AppendInt(stringId);
            
            // Assign pops value and pushes result - pop the result since we don't need it
            writer.Emit(DreamProcOpcode::Pop);
        }
        
        // Process VariableOverrides (overridden from parent types)
        for (const auto& [varName, variable] : OwningObject->VariableOverrides) {
            if (variable.Value == nullptr) {
                continue;  // No initialization needed
            }
            
            // Set expected type for bare 'new' inference
            exprCompiler.SetExpectedType(variable.Type);
            
            // Compile the initializer expression (pushes value onto stack)
            if (!exprCompiler.CompileExpression(variable.Value)) {
                exprCompiler.SetExpectedType(std::nullopt);
                compiler->ForcedWarning("Failed to compile initializer for variable override '" + varName + "'");
                continue;
            }
            
            // Clear expected type after compilation
            exprCompiler.SetExpectedType(std::nullopt);
            
            // Emit assignment to src.varName
            DMObjectTree* objectTree = compiler->GetObjectTree();
            int stringId = objectTree->AddString(varName);
            
            // Emit Assign opcode with SrcField reference
            writer.Emit(DreamProcOpcode::Assign);
            writer.AppendByte(static_cast<uint8_t>(DMReference::Type::SrcField));
            writer.AppendInt(stringId);
            
            // Assign pops value and pushes result - pop the result since we don't need it
            writer.Emit(DreamProcOpcode::Pop);
        }
        
        // 3. Add implicit return null
        writer.Emit(DreamProcOpcode::PushNull);
        writer.Emit(DreamProcOpcode::Return);
        
        // Finalize and copy bytecode to proc
        writer.Finalize();
        Bytecode = writer.GetBytecode();
        MaxStackSize = writer.GetMaxStackSize();
        
        return;
    }
    
    // Regular procs with null AstBody - nothing to compile
    if (AstBody == nullptr) {
        return;
    }
    
    // NOTE: Parameters are now registered in DMObjectTree::AddProc() during code tree building
    // This ensures they are available immediately for identifier resolution
    // No need to register them again here
    
    // Create bytecode writer and compilers
    BytecodeWriter writer;
    DMExpressionCompiler exprCompiler(compiler, this, &writer);
    DMStatementCompiler stmtCompiler(compiler, this, &writer, &exprCompiler);
    
    // 1. Handle set statements first (they are hoisted to the top)
    // Set statements configure proc attributes like name, category, src, etc.
    for (const auto& setStmt : AstBody->SetStatements) {
        if (!stmtCompiler.CompileStatement(setStmt.get())) {
            // Set statements are non-critical, just warn and continue
        }
    }
    
    // 2. Compile the proc body statements
    if (!stmtCompiler.CompileBlockInner(AstBody)) {
        // Compilation failed - don't generate bytecode, let EmitBytecode handle it
        // (it has stub fallback logic for critical procs)
        return;
    }
    
    // 3. Finalize statement compiler (resolve forward references for goto)
    if (!stmtCompiler.Finalize()) {
        // Forward reference resolution failed - don't generate bytecode
        // (let EmitBytecode handle it with stub fallback logic)
        return;
    }
    
    // 4. Add implicit return null if the proc doesn't end with a return
    // Check if bytecode is empty or doesn't end with Return opcode
    const auto& bytecode = writer.GetBytecode();
    bool needsImplicitReturn = bytecode.empty() || 
                               bytecode.back() != static_cast<uint8_t>(DreamProcOpcode::Return);
    
    if (needsImplicitReturn) {
        writer.Emit(DreamProcOpcode::PushNull);
        writer.ResizeStack(1);
        writer.Emit(DreamProcOpcode::Return);
    }
    
    // 5. Finalize bytecode (resolve jump labels)
    writer.Finalize();
    
    // Store the compiled bytecode and max stack size
    Bytecode = writer.GetBytecode();
    MaxStackSize = writer.GetMaxStackSize();
}

} // namespace DMCompiler
