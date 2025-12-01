// Additional stub files
#include "DMProc.h"
#include "DMObject.h"
#include "DMAST.h"
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
    void* value
)
    : LocalVariable(std::move(name), id, false, type, std::nullopt)
    , ConstValue(value)
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
    void* value
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
        value
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
    
    // Initialization procs have null AstBody - they're created dynamically
    if (AstBody == nullptr) {
        // For initialization procs, we need to:
        // 1. Call parent's init proc if it exists
        // 2. Compile variable initializations from OwningObject
        // This will be implemented in Phase 4 (bytecode emission)
        return;
    }
    
    // NOTE: Parameters are now registered in DMObjectTree::AddProc() during code tree building
    // This ensures they are available immediately for identifier resolution
    // No need to register them again here
    
    // TODO: Implement full proc compilation in Phase 4
    // This requires:
    // 1. ✅ Set up local variable scope (parameters already registered in AddProc)
    // 2. Compile AstBody->Statements using DMStatementCompiler
    // 3. Handle set statements (AstBody->SetStatements)
    // 4. Emit bytecode using BytecodeEmitter
    // 5. Add implicit return if needed
    
    // For now, compilation is deferred to Phase 4 (bytecode emission)
    // The proc is ready to be compiled when BytecodeEmitter is implemented
}

} // namespace DMCompiler
