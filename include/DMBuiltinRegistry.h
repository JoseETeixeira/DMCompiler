#pragma once

#include "DreamPath.h"
#include "DMValueType.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace DMCompiler {

/// Represents a built-in proc signature
struct ProcSignature {
    std::string Name;
    std::vector<std::string> Parameters;
    DMValueType ReturnType;
    bool IsVariadic = false;
    
    ProcSignature() : ReturnType(DMValueType::Anything) {}
    ProcSignature(const std::string& name, 
                  std::vector<std::string> params = {},
                  DMValueType returnType = DMValueType::Anything,
                  bool variadic = false)
        : Name(name), Parameters(std::move(params)), ReturnType(returnType), IsVariadic(variadic) {}
};

/// <summary>
/// Singleton registry of built-in DM procs and variables.
/// This allows the compiler to recognize built-in functions and variables
/// without requiring DMStandard to be compiled.
/// </summary>
class DMBuiltinRegistry {
public:
    /// Get singleton instance
    static DMBuiltinRegistry& Instance();
    
    /// Check if a proc name is a recognized global built-in proc
    /// @param name Global proc name (e.g., "sleep", "spawn", "abs")
    /// @return true if name is a recognized global built-in proc
    bool IsGlobalBuiltinProc(const std::string& name) const;
    
    /// Check if a proc is a recognized built-in on a specific type
    /// @param type Type path (e.g., /list, /atom, /world)
    /// @param name Proc name on that type (e.g., "Add", "Move")
    /// @return true if proc is a recognized built-in on the type
    bool IsTypeBuiltinProc(const DreamPath& type, const std::string& name) const;
    
    /// Get the signature of a global built-in proc
    /// @param name Global proc name
    /// @return Optional signature if proc is a built-in
    std::optional<ProcSignature> GetGlobalProcSignature(const std::string& name) const;
    
    /// Get the signature of a type-specific built-in proc
    /// @param type Type path
    /// @param name Proc name
    /// @return Optional signature if proc is a built-in
    std::optional<ProcSignature> GetTypeProcSignature(const DreamPath& type, 
                                                       const std::string& name) const;
    
    /// Check if a variable is a recognized built-in on a specific type
    /// @param type Type path
    /// @param name Variable name
    /// @return true if variable is a recognized built-in on the type
    bool IsBuiltinVar(const DreamPath& type, const std::string& name) const;
    
    /// Check if a name is a context variable (src, usr, args, global, world)
    /// @param name Identifier name
    /// @return true if name is a context variable
    bool IsContextVariable(const std::string& name) const;
    
    /// Get the expected type of a built-in variable
    /// @param type Type path
    /// @param name Variable name
    /// @return Optional value type if variable is a built-in
    std::optional<DMValueType> GetVarType(const DreamPath& type, 
                                           const std::string& name) const;
    
    /// Check if a type path represents a built-in type (datum, atom, list, etc.)
    /// @param type Type path
    /// @return true if type is a built-in DM type
    bool IsBuiltinType(const DreamPath& type) const;
    
    /// Check if a type inherits from another type (including built-in hierarchy)
    /// @param derived The derived type
    /// @param base The base type to check
    /// @return true if derived inherits from base
    bool TypeInheritsFrom(const DreamPath& derived, const DreamPath& base) const;

private:
    DMBuiltinRegistry();
    ~DMBuiltinRegistry() = default;
    
    // Non-copyable
    DMBuiltinRegistry(const DMBuiltinRegistry&) = delete;
    DMBuiltinRegistry& operator=(const DMBuiltinRegistry&) = delete;
    
    /// Initialize all built-in registrations
    void InitializeBuiltins();
    
    /// Register global procs
    void RegisterGlobalProcs();
    
    /// Register type-specific procs
    void RegisterTypeProcs();
    
    /// Register built-in variables
    void RegisterBuiltinVars();
    
    /// Helper to register procs for a type and all its subtypes
    void RegisterProcForType(const std::string& typePath, const ProcSignature& sig);
    
    /// Helper to register a variable for a type and all its subtypes
    void RegisterVarForType(const std::string& typePath, const std::string& name, DMValueType type);
    
    // Global procs (not on any type)
    std::unordered_map<std::string, ProcSignature> GlobalProcs_;
    
    // Type-specific procs: type path -> proc name -> signature
    std::unordered_map<std::string, std::unordered_map<std::string, ProcSignature>> TypeProcs_;
    
    // Type-specific variables: type path -> var name -> value type
    std::unordered_map<std::string, std::unordered_map<std::string, DMValueType>> TypeVars_;
    
    // Context variables always available
    std::unordered_set<std::string> ContextVars_;
    
    // Built-in type paths
    std::unordered_set<std::string> BuiltinTypes_;
    
    // Type hierarchy for inheritance checking: type -> parent type
    std::unordered_map<std::string, std::string> TypeHierarchy_;
};

} // namespace DMCompiler
