#include "DMObject.h"
#include "DreamPath.h"
#include "DMCompiler.h"
#include "DMObjectTree.h"
#include "DMProc.h"
#include "Location.h"

namespace DMCompiler {

DMObject::DMObject(int id, DreamPath path, DMObject* parent)
    : Id(id)
    , Path(std::move(path))
    , Parent(parent)
{
}

bool DMObject::IsRoot() const {
    return Path == DreamPath::Root;
}

void DMObject::AddProc(int procId, const std::string& procName, bool forceFirst) {
    auto& procList = Procs[procName];
    
    if (forceFirst) {
        procList.insert(procList.begin(), procId);
    } else {
        procList.push_back(procId);
    }
}

const DMVariable* DMObject::GetVariable(const std::string& name) const {
    // Check local variables
    auto it = Variables.find(name);
    if (it != Variables.end()) {
        return &it->second;
    }
    
    // Check variable overrides
    auto overrideIt = VariableOverrides.find(name);
    if (overrideIt != VariableOverrides.end()) {
        return &overrideIt->second;
    }
    
    // Search parent
    if (Parent != nullptr) {
        return Parent->GetVariable(name);
    }
    
    return nullptr;
}

DMVariable* DMObject::GetVariable(const std::string& name) {
    // Const cast to reuse const version
    return const_cast<DMVariable*>(
        const_cast<const DMObject*>(this)->GetVariable(name)
    );
}

bool DMObject::HasLocalVariable(const std::string& name) const {
    if (Variables.find(name) != Variables.end()) {
        return true;
    }
    
    if (Parent == nullptr) {
        return false;
    }
    
    return Parent->HasLocalVariable(name);
}

bool DMObject::HasGlobalVariable(const std::string& name) const {
    if (IsRoot()) {
        return GlobalVariables.find(name) != GlobalVariables.end();
    }
    
    return HasGlobalVariableNotInRoot(name);
}

bool DMObject::HasGlobalVariableNotInRoot(const std::string& name) const {
    if (GlobalVariables.find(name) != GlobalVariables.end()) {
        return true;
    }
    
    if (Parent == nullptr || Parent->IsRoot()) {
        return false;
    }
    
    return Parent->HasGlobalVariable(name);
}

bool DMObject::HasProc(const std::string& name) const {
    if (Procs.find(name) != Procs.end()) {
        return true;
    }
    
    if (Parent != nullptr) {
        return Parent->HasProc(name);
    }
    
    return false;
}

bool DMObject::OwnsProc(const std::string& name) const {
    return Procs.find(name) != Procs.end();
}

const std::vector<int>* DMObject::GetProcs(const std::string& name) const {
    auto it = Procs.find(name);
    if (it != Procs.end()) {
        return &it->second;
    }
    
    if (Parent != nullptr) {
        return Parent->GetProcs(name);
    }
    
    return nullptr;
}

void DMObject::CreateInitializationProc(DMCompiler* compiler, DMObjectTree* objectTree) {
    // Only create init proc if we have variables with values and don't already have one
    if (InitializationProc != -1) {
        return; // Already have an init proc
    }
    
    // Check if we have any variables with initial values
    bool hasInitialValues = false;
    for (const auto& [name, var] : Variables) {
        if (var.Value != nullptr) {
            hasInitialValues = true;
            break;
        }
    }
    
    // Also check variable overrides
    if (!hasInitialValues) {
        for (const auto& [name, var] : VariableOverrides) {
            if (var.Value != nullptr) {
                hasInitialValues = true;
                break;
            }
        }
    }
    
    if (!hasInitialValues) {
        return; // No initialization needed
    }
    
    // Create the initialization proc
    // For now, we just mark that one is needed - actual compilation happens in DMProc::Compile
    DMProc* initProc = objectTree->CreateProc("__init__", this, false, Location());
    InitializationProc = initProc->Id;
    
    // TODO: In Phase 4 (bytecode emission), we'll need to:
    // 1. Call parent's init proc if it exists
    // 2. Emit bytecode to evaluate each variable's Value expression
    // 3. Emit bytecode to assign the result to the variable
}

} // namespace DMCompiler
