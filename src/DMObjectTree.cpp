#include "DMObjectTree.h"
#include "DMCompiler.h"
#include "DMProc.h"
#include "DMASTStatement.h"
#include <stdexcept>
#include <iostream>

namespace DMCompiler {

DMObjectTree::DMObjectTree(DMCompiler* compiler)
    : Compiler_(compiler)
    , DmObjectIdCounter_(0)
    , DmProcIdCounter_(0)
{
    // Create root object immediately
    GetRoot();
    
    // Initialize built-in constants
    InitializeBuiltInConstants();
}

DMObjectTree::~DMObjectTree() = default;

void DMObjectTree::InitializeBuiltInConstants() {
    // Note: Most constants are now loaded from DMStandard/Defines.dm via InitializeDMStandard()
    // This method is kept for any constants that need to be available even with --no-standard
    
    // Common DMStandard parameter name constants
    // These appear as parameter names in DMStandard but are used as identifiers
    DMVariable trg, max, glue, start, end;
    CreateGlobal(trg, std::nullopt, "Trg", false, false, DMComplexValueType(DMValueType::Anything));
    CreateGlobal(max, std::nullopt, "Max", false, false, DMComplexValueType(DMValueType::Anything));
    CreateGlobal(glue, std::nullopt, "Glue", false, false, DMComplexValueType(DMValueType::Anything));
    CreateGlobal(start, std::nullopt, "Start", false, false, DMComplexValueType(DMValueType::Anything));
    CreateGlobal(end, std::nullopt, "End", false, false, DMComplexValueType(DMValueType::Anything));

    // Ensure built-in types exist
    GetOrCreateDMObject(DreamPath(DreamPath::PathType::Absolute, {"list"}));
    GetOrCreateDMObject(DreamPath(DreamPath::PathType::Absolute, {"client"}));
    GetOrCreateDMObject(DreamPath(DreamPath::PathType::Absolute, {"datum"}));
    GetOrCreateDMObject(DreamPath(DreamPath::PathType::Absolute, {"world"}));
    GetOrCreateDMObject(DreamPath(DreamPath::PathType::Absolute, {"savefile"}));
}

DMObject* DMObjectTree::GetRoot() {
    return GetOrCreateDMObject(DreamPath::Root);
}

int DMObjectTree::AddString(const std::string& value) {
    // Check if string already exists
    auto it = StringToStringId_.find(value);
    if (it != StringToStringId_.end()) {
        return it->second;
    }
    
    // Add new string
    int stringId = static_cast<int>(StringTable.size());
    StringTable.push_back(value);
    StringToStringId_[value] = stringId;
    
    return stringId;
}

DMObject* DMObjectTree::GetOrCreateDMObject(const DreamPath& path) {
    DMObject* existing = nullptr;
    if (TryGetDMObject(path, &existing)) {
        return existing;
    }
    
    // Determine parent
    DMObject* parent = DetermineParent(path);
    
    // Validate parent (except for root)
    if (path != DreamPath::Root && parent == nullptr) {
        throw std::runtime_error("Type " + path.ToString() + " did not have a parent");
    }
    
    // Create new object
    int id = DmObjectIdCounter_++;
    auto dmObject = std::make_unique<DMObject>(id, path, parent);
    DMObject* dmObjectPtr = dmObject.get();
    
    // Store object
    AllObjects.push_back(std::move(dmObject));
    PathToTypeId_[path] = id;
    
    return dmObjectPtr;
}

bool DMObjectTree::TryGetDMObject(const DreamPath& path, DMObject** outObject) {
    int typeId;
    if (TryGetTypeId(path, typeId)) {
        *outObject = AllObjects[typeId].get();
        return true;
    }
    
    *outObject = nullptr;
    return false;
}

bool DMObjectTree::TryGetTypeId(const DreamPath& path, int& outTypeId) {
    auto it = PathToTypeId_.find(path);
    if (it != PathToTypeId_.end()) {
        outTypeId = it->second;
        return true;
    }
    
    return false;
}

DMObject* DMObjectTree::GetType(const DreamPath& path, DMObject* context) {
    // Handle absolute paths directly
    if (path.GetPathType() == DreamPath::PathType::Absolute) {
        DMObject* obj = nullptr;
        if (TryGetDMObject(path, &obj)) {
            return obj;
        }
        
        // Fallback: Check for nested type in global scope
        // If path is /A/B and fails, check /B
        // This handles cases like /client/C where C is a global type
        if (path.GetElements().size() > 1) {
            std::string lastElement = path.GetElements().back();
            DreamPath globalPath(DreamPath::PathType::Absolute, {lastElement});
            if (TryGetDMObject(globalPath, &obj)) {
                return obj;
            }
        }
        
        return nullptr;
    }
    
    // Handle relative paths with context
    if (context != nullptr) {
        // Try combining with context path
        DreamPath combinedPath = context->Path.Combine(path);
        DMObject* obj = nullptr;
        if (TryGetDMObject(combinedPath, &obj)) {
            return obj;
        }
        
        // Try upward search through parent chain
        auto result = UpwardSearch(context->Path, path);
        if (result.has_value()) {
            if (TryGetDMObject(result.value(), &obj)) {
                return obj;
            }
        }
    }
    
    // Try as absolute path from root (for relative paths without context)
    DreamPath absolutePath(DreamPath::PathType::Absolute, path.GetElements());
    DMObject* obj = nullptr;
    if (TryGetDMObject(absolutePath, &obj)) {
        return obj;
    }
    
    return nullptr;
}

DMProc* DMObjectTree::GetProc(DMObject* obj, const std::string& procName) {
    if (!obj) {
        // If no object context, try global procs only
        int globalProcId = GetGlobalProcId(procName);
        if (globalProcId >= 0 && globalProcId < static_cast<int>(AllProcs.size())) {
            return AllProcs[globalProcId].get();
        }
        return nullptr;
    }
    
    // 1. Search current object and parent chain for the proc
    const std::vector<int>* procIds = obj->GetProcs(procName);
    if (procIds && !procIds->empty()) {
        // Return the first proc (most derived)
        int procId = (*procIds)[0];
        if (procId >= 0 && procId < static_cast<int>(AllProcs.size())) {
            return AllProcs[procId].get();
        }
    }
    
    // 2. If not found in object hierarchy, search global procs
    int globalProcId = GetGlobalProcId(procName);
    if (globalProcId >= 0 && globalProcId < static_cast<int>(AllProcs.size())) {
        return AllProcs[globalProcId].get();
    }
    
    // 3. Not found anywhere - return nullptr with appropriate diagnostic
    if (Compiler_) {
        std::string msg = "Proc '" + procName + "' not found";
        if (obj) {
            msg += " in type " + obj->Path.ToString() + " or its parents";
        }
        msg += " or in global procs";
        // Note: This is just for internal tracking - actual warnings are emitted by DMExpressionCompiler
    }
    
    return nullptr;
}

std::unordered_map<std::string, const DMVariable*> DMObjectTree::GetAllVariables(DMObject* obj) const {
    std::unordered_map<std::string, const DMVariable*> allVars;
    
    if (!obj) {
        return allVars;
    }
    
    // Collect variables from parent chain (bottom-up so child overrides parent)
    std::vector<DMObject*> chain;
    DMObject* current = obj;
    while (current != nullptr) {
        chain.push_back(current);
        current = current->Parent;
    }
    
    // Add variables from root to leaf (so child overrides parent)
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        DMObject* ancestor = *it;
        
        // Add regular variables
        for (const auto& [name, var] : ancestor->Variables) {
            allVars[name] = &var;
        }
        
        // Add variable overrides (these take precedence)
        for (const auto& [name, var] : ancestor->VariableOverrides) {
            allVars[name] = &var;
        }
    }
    
    return allVars;
}

int DMObjectTree::CreateGlobal(
    DMVariable& outGlobal,
    const std::optional<DreamPath>& type,
    const std::string& name,
    bool isConst,
    bool isFinal,
    const DMComplexValueType& valType
) {
    int id = static_cast<int>(Globals.size());
    
    outGlobal = DMVariable(type, name, true, isConst, isFinal, false, valType);
    Globals.push_back(outGlobal);
    
    return id;
}

DMObject* DMObjectTree::DetermineParent(const DreamPath& path) {
    const auto& elements = path.GetElements();
    
    // Root has no parent
    if (path == DreamPath::Root) {
        return nullptr;
    }
    
    // Multi-element paths: parent is everything except the last element
    if (elements.size() > 1) {
        std::vector<std::string> parentElements(elements.begin(), elements.end() - 1);
        DreamPath parentPath(DreamPath::PathType::Absolute, parentElements);
        return GetOrCreateDMObject(parentPath);
    }
    
    // Single-element paths: special handling
    if (elements.size() == 1) {
        const std::string& element = elements[0];
        
        // Special root-level types
        if (element == "client" || element == "datum" || element == "list" ||
            element == "alist" || element == "vector" || element == "savefile" ||
            element == "world" || element == "callee") {
            return GetOrCreateDMObject(DreamPath::Root);
        }
        
        // Default parent is /datum, unless --no-standard is set (then it's root)
        // This matches the C# implementation
        if (Compiler_ && Compiler_->GetSettings().NoStandard) {
            return GetOrCreateDMObject(DreamPath::Root);
        }
        return GetOrCreateDMObject(DreamPath::Datum);
    }
    
    return nullptr;
}

std::optional<DreamPath> DMObjectTree::UpwardSearch(const DreamPath& path, const DreamPath& search, const std::string& procName) {
    // Implement full upward search with proc support
    // This searches up the hierarchy for a matching type or proc
    
    DreamPath currentPath = path;
    
    while (true) {
        DreamPath combined = currentPath.Combine(search);
        
        int typeId;
        if (TryGetTypeId(combined, typeId)) {
            // If procName is specified, check if this type has the proc
            if (!procName.empty()) {
                DMObject* obj = AllObjects[typeId].get();
                if (obj && obj->HasProc(procName)) {
                    return combined;
                }
                // Type exists but doesn't have the proc - continue searching
            } else {
                // Just looking for a type, found it
                return combined;
            }
        }
        
        // Reached root (empty elements means root), search failed
        if (currentPath.GetElements().empty()) {
            break;
        }
        
        // Move up one level by removing the last element
        currentPath = currentPath.RemoveLastElement();
    }
    
    // If searching for a proc, also check if it's a global proc
    if (!procName.empty()) {
        auto globalIt = GlobalProcs.find(procName);
        if (globalIt != GlobalProcs.end()) {
            return DreamPath::Root;  // Global proc found at root
        }
    }
    
    return std::nullopt;
}

void DMObjectTree::RegisterGlobalProc(const std::string& procName, int procId) {
    GlobalProcs[procName] = procId;
}

int DMObjectTree::GetGlobalProcId(const std::string& procName) const {
    auto it = GlobalProcs.find(procName);
    if (it != GlobalProcs.end()) {
        return it->second;
    }
    return -1;  // Not found
}

int DMObjectTree::GetGlobalVariableId(const std::string& varName) const {
    // Search through the Globals vector for a variable with matching name
    for (size_t i = 0; i < Globals.size(); ++i) {
        if (Globals[i].Name == varName) {
            return static_cast<int>(i);
        }
    }
    return -1;  // Not found
}

void DMObjectTree::AddGlobalConstant(const std::string& name, int value) {
    // Check if constant already exists
    for (const auto& global : Globals) {
        if (global.Name == name) {
            // Already exists, don't add duplicate
            return;
        }
    }
    
    // Create a new global constant
    DMVariable constant;
    constant.Name = name;
    constant.IsConst = true;
    constant.IsGlobal = true;
    constant.IsFinal = false;
    constant.IsTmp = false;
    constant.ValType = DMComplexValueType(DMValueType::Num);
    constant.Value = nullptr;  // Value will be set during initialization
    
    Globals.push_back(constant);
}

DMProc* DMObjectTree::CreateProc(
    const std::string& name,
    DMObject* owningObject,
    bool isVerb,
    const Location& location
) {
    int procId = DmProcIdCounter_++;
    auto proc = std::make_unique<DMProc>(procId, name, owningObject, isVerb, location);
    DMProc* procPtr = proc.get();
    AllProcs.push_back(std::move(proc));
    return procPtr;
}

void DMObjectTree::AddType(const DreamPath& path) {
    // Simply ensure the type exists - GetOrCreateDMObject creates it if needed
    GetOrCreateDMObject(path);
}

void DMObjectTree::AddObjectVar(const DreamPath& owner, DMASTObjectVarDefinition* varDef, std::optional<DreamPath> effectiveType) {
    if (!varDef) return;
    
    // Get or create the owning object
    DMObject* ownerObj = GetOrCreateDMObject(owner);
    
    // Parse variable modifiers from the type path
    // In DM, variable paths can contain modifiers like var/const/mob/myvar or tmp/squad/squad
    // We need to parse modifiers from both the AST type path AND the effective type
    VarModifiers mods = VarModifiers::Parse(varDef->TypePath.Path);
    
    // Also parse modifiers from effectiveType if it contains modifiers like tmp
    VarModifiers effectiveMods;
    if (effectiveType.has_value()) {
        effectiveMods = VarModifiers::Parse(*effectiveType);
    }
    
    // Create a DMVariable from the AST definition
    DMVariable var;
    var.Name = varDef->Name;
    
    // Use the parsed type from effectiveType if available, otherwise from AST type path
    if (effectiveMods.TypePath.has_value() && !effectiveMods.TypePath->GetElements().empty()) {
        var.Type = effectiveMods.TypePath;
    } else if (mods.TypePath.has_value()) {
        var.Type = mods.TypePath;
    } else {
        // No type constraint
        var.Type = std::nullopt;
    }
    
    // Apply extracted modifiers (merge from both sources)
    var.IsConst = mods.IsConst || effectiveMods.IsConst;
    var.IsFinal = mods.IsFinal || effectiveMods.IsFinal;
    var.IsGlobal = mods.IsGlobal || mods.IsStatic || effectiveMods.IsGlobal || effectiveMods.IsStatic;
    var.IsTmp = mods.IsTmp || effectiveMods.IsTmp;
    var.Value = varDef->Value.get();  // Store pointer to AST expression (non-owning)
    
    // Set explicit value type from "as" clause if present
    if (varDef->ExplicitValueType.has_value()) {
        var.ValType = varDef->ExplicitValueType.value();
    }
    
    // Add to the object's variable map
    ownerObj->Variables[var.Name] = var;
}

void DMObjectTree::AddObjectVarOverride(const DreamPath& owner, DMASTObjectVarOverride* varOverride) {
    if (!varOverride) return;
    
    // Get or create the owning object
    DMObject* ownerObj = GetOrCreateDMObject(owner);
    
    // Create a DMVariable from the override
    DMVariable var;
    var.Name = varOverride->VarName;
    // Type will be inherited from parent's definition
    var.IsConst = false;
    var.IsFinal = false;
    var.IsGlobal = false;
    var.IsTmp = false;
    var.Value = varOverride->Value.get();  // Store pointer to AST expression (non-owning)
    
    // Add to the object's variable override map
    ownerObj->VariableOverrides[var.Name] = var;
}

DMProc* DMObjectTree::AddProc(const DreamPath& owner, DMASTObjectProcDefinition* procDef) {
    if (!procDef) return nullptr;
    
    // Get or create the owning object
    DMObject* ownerObj = GetOrCreateDMObject(owner);
    
    // Create the proc
    DMProc* proc = CreateProc(procDef->Name, ownerObj, procDef->IsVerb, procDef->Location_);
    
    // Store the proc definition AST for later compilation
    proc->AstBody = procDef->Body.get();  // Store non-owning pointer to proc body
    
    // Store the parameter definitions (non-owning pointers)
    for (const auto& param : procDef->Parameters) {
        proc->AstParameters.push_back(param.get());
    }
    
    // Add the proc to the object
    ownerObj->AddProc(proc->Id, procDef->Name);
    
    // If it's a global proc (owned by root), register it
    if (owner == DreamPath::Root) {
        RegisterGlobalProc(procDef->Name, proc->Id);
    }

    return proc;
}

std::vector<DMObject*> DMObjectTree::GetAllObjects() const {
    std::vector<DMObject*> result;
    result.reserve(AllObjects.size());
    
    for (const auto& obj : AllObjects) {
        result.push_back(obj.get());
    }
    
    return result;
}

std::vector<DMProc*> DMObjectTree::GetAllProcs() const {
    std::vector<DMProc*> result;
    result.reserve(AllProcs.size());
    
    for (const auto& proc : AllProcs) {
        result.push_back(proc.get());
    }
    
    return result;
}

std::unordered_map<std::string, DMProc*> DMObjectTree::GetAllProcsForObject(DMObject* obj) const {
    std::unordered_map<std::string, DMProc*> procMap;
    
    if (!obj) {
        return procMap;
    }
    
    // Build hierarchy from current object to root
    std::vector<DMObject*> hierarchy;
    DMObject* current = obj;
    while (current != nullptr) {
        hierarchy.push_back(current);
        current = current->Parent;
    }
    
    // Process from root to derived (so derived procs override parent procs)
    for (auto it = hierarchy.rbegin(); it != hierarchy.rend(); ++it) {
        DMObject* ancestor = *it;
        
        // Add all procs from this object
        for (const auto& [name, ids] : ancestor->Procs) {
            if (!ids.empty()) {
                // Get the most recent definition (last in list)
                int procId = ids.back();
                if (procId >= 0 && procId < static_cast<int>(AllProcs.size())) {
                    procMap[name] = AllProcs[procId].get();
                }
            }
        }
    }
    
    return procMap;
}

void DMObjectTree::SetDMStandardFinalized() {
    // Mark all existing objects as being from DMStandard
    for (const auto& obj : AllObjects) {
        if (obj) {
            obj->IsFromDMStandard = true;
        }
    }
}

} // namespace DMCompiler
