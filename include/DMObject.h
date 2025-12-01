#pragma once

#include "DreamPath.h"
#include "DMVariable.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace DMCompiler {

class DMCompiler;
class DMProc;

/// <summary>
/// DMObject - Compile-Time Type Definition
/// 
/// Represents a single type in the DM object hierarchy (e.g., /mob, /obj/item/weapon).
/// This is NOT a runtime instance - it's the compile-time blueprint describing:
/// - What variables the type has
/// - What procs (methods) are defined on it
/// - Its position in the inheritance hierarchy
/// - Initialization code
/// 
/// Usage in Compiler Pipeline:
/// 1. DMObjectTree creates DMObject instances as types are encountered
/// 2. During parsing, variables and procs are added via AddProc(), Variables map, etc.
/// 3. At runtime, the VM uses these definitions to instantiate actual objects
/// 
/// Inheritance:
/// - Variables and procs are inherited from Parent
/// - GetVariable() searches up the parent chain
/// - HasProc() checks both owned and inherited procs
/// </summary>
class DMObject {
public:
    /// Unique identifier for this type (index in DMObjectTree::AllObjects)
    const int Id;
    
    /// Type path (e.g., /mob/player, /obj/item/weapon/sword)
    DreamPath Path;
    
    /// Parent type in the hierarchy (null only for root object /)
    DMObject* Parent;
    
    /// Procs defined on this type
    /// Maps proc name -> list of proc IDs (multiple entries for overloads)
    /// Example: "Attack" -> [100, 101] for two different Attack() signatures
    std::unordered_map<std::string, std::vector<int>> Procs;
    
    /// Instance variables defined on this type
    /// Example: "health", "icon", "name"
    std::unordered_map<std::string, DMVariable> Variables;
    
    /// Global/static variables on this type
    /// Maps variable name -> global variable ID
    std::unordered_map<std::string, int> GlobalVariables;
    
    /// Variable overrides from parent types
    /// When a child redefines a parent's variable, it goes here
    std::unordered_map<std::string, DMVariable> VariableOverrides;
    
    /// Temporary variables (declared with tmp modifier)
    /// These are not saved in save files
    std::unordered_set<std::string> TmpVariables;
    
    /// Constant variables (declared with const modifier)
    /// Cannot be modified after initialization
    std::unordered_set<std::string> ConstVariables;
    
    /// Optional initialization proc ID
    /// If present, this proc is called when instances of this type are created
    /// -1 indicates no initialization proc
    int InitializationProc = -1;
    
    /// Constructor
    /// @param id Unique type identifier
    /// @param path Type path in hierarchy
    /// @param parent Parent type (null for root)
    DMObject(int id, DreamPath path, DMObject* parent = nullptr);
    ~DMObject() = default;
    
    /// Check if this is the root object (/)
    /// @return true if this is the root, false otherwise
    bool IsRoot() const;
    
    /// Add a proc to this type
    /// @param procId ID of the proc to add (from DMObjectTree::AllProcs)
    /// @param procName Name of the proc (e.g., "Attack", "New")
    /// @param forceFirst If true, prepend to list (used for override priority)
    void AddProc(int procId, const std::string& procName, bool forceFirst = false);
    
    /// Get a variable by name, searching up the inheritance chain
    /// Checks in order: VariableOverrides -> Variables -> Parent
    /// @param name Variable name to search for
    /// @return Pointer to variable if found, nullptr otherwise
    const DMVariable* GetVariable(const std::string& name) const;
    DMVariable* GetVariable(const std::string& name);
    
    /// Check if this type has a local (instance) variable
    /// Searches up the parent chain recursively
    /// @param name Variable name to check
    /// @return true if variable exists on this type or any parent
    bool HasLocalVariable(const std::string& name) const;
    
    /// Check if this type has a global/static variable
    /// For root object: checks GlobalVariables map
    /// For non-root: recursively checks parent chain
    /// @param name Variable name to check
    /// @return true if global variable exists
    bool HasGlobalVariable(const std::string& name) const;
    
    /// Check if this type has a proc (includes inherited procs)
    /// Searches up the parent chain
    /// @param name Proc name to check
    /// @return true if proc exists on this type or any parent
    bool HasProc(const std::string& name) const;
    
    /// Check if a proc was defined directly on this type (not inherited)
    /// @param name Proc name to check
    /// @return true if proc is directly owned by this type
    bool OwnsProc(const std::string& name) const;
    
    /// Get proc IDs for a given name, searching up the hierarchy
    /// Returns the first match (either from this type or nearest parent)
    /// @param name Proc name to look up
    /// @return Pointer to vector of proc IDs if found, nullptr otherwise
    const std::vector<int>* GetProcs(const std::string& name) const;
    
    /// Create an initialization proc for this object if needed
    /// This proc will be called when instances are created to initialize variables
    /// Only creates if there are variables with initial values
    /// @param compiler Pointer to the compiler for creating the proc
    /// @param objectTree Pointer to the object tree for proc creation
    void CreateInitializationProc(class DMCompiler* compiler, class DMObjectTree* objectTree);
    
private:
    // Helper for HasGlobalVariable that doesn't check root
    bool HasGlobalVariableNotInRoot(const std::string& name) const;
};

} // namespace DMCompiler
