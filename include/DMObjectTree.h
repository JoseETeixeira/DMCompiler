#pragma once

#include "DMObject.h"
#include "DMVariable.h"
#include "DreamPath.h"
#include "Location.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <optional>

namespace DMCompiler {

class DMProc;
class DMCompiler;

/// <summary>
/// DMObjectTree - Central Type System Manager
/// 
/// This class manages the complete type hierarchy for DM code compilation.
/// It maintains all object types, global variables, procedures, string literals,
/// and resource paths encountered during compilation.
/// 
/// Usage Pattern:
/// 1. Create DMObjectTree at the start of compilation
/// 2. As types are encountered, call GetOrCreateDMObject() to build the hierarchy
/// 3. Variables and procs are added to their respective DMObject instances
/// 4. String literals are interned via AddString() for deduplication
/// 5. After compilation, the tree contains the complete type system for bytecode generation
/// 
/// Key Responsibilities:
/// - Type hierarchy management with automatic parent determination
/// - String interning to avoid duplicate string literals
/// - Global variable and proc tracking
/// - Resource path collection
/// - Type lookup and resolution
/// </summary>
class DMObjectTree {
public:
    /// All objects in the type hierarchy (owned via unique_ptr)
    /// Index in this vector corresponds to object's ID
    std::vector<std::unique_ptr<DMObject>> AllObjects;
    
    /// All procs in the object tree
    /// Index in this vector corresponds to proc's ID
    std::vector<std::unique_ptr<DMProc>> AllProcs;
    
    /// Global variables declared at file scope
    std::vector<DMVariable> Globals;
    
    /// Mapping from global proc name to proc ID
    /// Used for quick lookup of globally-defined procedures
    std::unordered_map<std::string, int> GlobalProcs;
    
    /// Interned string table - all string literals point here
    /// Deduplication saves memory and enables efficient string comparison
    std::vector<std::string> StringTable;
    
    /// Resource paths used in the compilation (icons, sounds, etc.)
    /// Collected for packaging with the compiled output
    std::unordered_set<std::string> Resources;
    
    /// Constructor
    /// @param compiler Optional pointer to the DMCompiler for error reporting
    explicit DMObjectTree(DMCompiler* compiler = nullptr);
    
    /// Destructor - defined in .cpp to allow unique_ptr<DMProc> with incomplete type
    ~DMObjectTree();
    
    /// Get the root object (/) - creates it if it doesn't exist
    /// All types in DM are ultimately children of the root object
    /// @return Pointer to the root DMObject (never null)
    DMObject* GetRoot();
    
    /// Add a string to the string table with automatic deduplication
    /// If the string already exists, returns the existing ID
    /// @param value The string literal to intern
    /// @return Index in StringTable where the string is stored
    int AddString(const std::string& value);
    
    /// Create or retrieve an existing object by path
    /// Automatically creates parent objects if they don't exist
    /// 
    /// Parent Determination Rules:
    /// - /datum, /client, /list, /savefile, /world -> parent is /root
    /// - /mob, /obj, /turf, /area -> parent is /datum
    /// - Other paths -> parent is immediate parent path
    /// 
    /// Example: GetOrCreateDMObject("/mob/player") will:
    /// 1. Create /root (if not exists)
    /// 2. Create /datum with parent /root (if not exists)
    /// 3. Create /mob with parent /datum (if not exists)
    /// 4. Create /mob/player with parent /mob
    /// 
    /// @param path The type path to create/retrieve
    /// @return Pointer to the DMObject (never null)
    DMObject* GetOrCreateDMObject(const DreamPath& path);
    
    /// Try to get an object by path without creating it
    /// @param path The type path to look up
    /// @param outObject Receives pointer to object if found (null otherwise)
    /// @return true if object exists, false otherwise
    bool TryGetDMObject(const DreamPath& path, DMObject** outObject);
    
    /// Try to get a type ID by path
    /// @param path The type path to look up
    /// @param outTypeId Receives the object's ID if found
    /// @return true if object exists, false otherwise
    bool TryGetTypeId(const DreamPath& path, int& outTypeId);
    
    /// Get a type by path with comprehensive resolution
    /// Handles absolute paths (/mob/player), relative paths (player), and parent references
    /// @param path The type path to resolve
    /// @param context Optional context object for relative path resolution
    /// @return Pointer to the DMObject if found, nullptr otherwise
    DMObject* GetType(const DreamPath& path, DMObject* context = nullptr);
    
    /// Get a proc by name from an object, searching through inheritance chain
    /// @param obj The object to search from
    /// @param procName The name of the proc to find
    /// @return Pointer to the first matching DMProc if found, nullptr otherwise
    DMProc* GetProc(DMObject* obj, const std::string& procName);
    
    /// Get all variables for an object including inherited ones
    /// @param obj The object to get variables for
    /// @return Map of variable name to variable definition (includes inherited)
    std::unordered_map<std::string, const DMVariable*> GetAllVariables(DMObject* obj) const;
    
    /// Create a global variable at file scope
    /// @param outGlobal Variable object to populate with the created global
    /// @param type Optional type path constraint (e.g., /mob for a mob variable)
    /// @param name Variable name
    /// @param isConst Whether this is a const global
    /// @param isFinal Whether this is a final global
    /// @param valType Complex type information
    /// @return Index in Globals vector
    int CreateGlobal(
        DMVariable& outGlobal,
        const std::optional<DreamPath>& type,
        const std::string& name,
        bool isConst,
        bool isFinal,
        const DMComplexValueType& valType
    );
    
    /// Upward search for types and procs in the hierarchy
    /// Walks up the parent chain looking for a matching type path
    /// TODO: Implement full proc resolution when needed
    /// @param path Starting point for the search
    /// @param search The path to search for
    /// @return The found path, or empty optional if not found
    std::optional<DreamPath> UpwardSearch(const DreamPath& path, const DreamPath& search);
    
    /// Register a global proc by name and ID
    /// Used during compilation to track globally-accessible procedures
    /// @param procName The name of the global proc
    /// @param procId The unique ID of the proc
    void RegisterGlobalProc(const std::string& procName, int procId);
    
    /// Try to get a global proc ID by name
    /// @param procName The name of the global proc to look up
    /// @return The proc ID if found, or -1 if not found
    int GetGlobalProcId(const std::string& procName) const;
    
    /// Try to get a global variable ID by name
    /// @param varName The name of the global variable to look up
    /// @return The global variable index in Globals vector if found, or -1 if not found
    int GetGlobalVariableId(const std::string& varName) const;
    
    /// Add a global constant with a numeric value
    /// Used for initializing DMStandard constants (NORTH, SOUTH, etc.)
    /// @param name The name of the constant
    /// @param value The numeric value of the constant
    void AddGlobalConstant(const std::string& name, int value);
    
    /// Create a new proc and add it to the object tree
    /// @param name Proc name
    /// @param owningObject The DMObject that owns this proc
    /// @param isVerb Whether this is a verb
    /// @param location Source location
    /// @return Pointer to the created DMProc
    DMProc* CreateProc(
        const std::string& name,
        DMObject* owningObject,
        bool isVerb,
        const Location& location
    );
    
    /// Add a type to the object tree (creates it if it doesn't exist)
    /// @param path The type path to add
    void AddType(const DreamPath& path);
    
    /// Add a variable definition to an object
    /// @param owner The object that owns this variable
    /// @param varDef The variable definition from the AST
    void AddObjectVar(const DreamPath& owner, class DMASTObjectVarDefinition* varDef);
    
    /// Add a variable override to an object
    /// @param owner The object that owns this variable
    /// @param varOverride The variable override from the AST
    void AddObjectVarOverride(const DreamPath& owner, class DMASTObjectVarOverride* varOverride);
    
    /// Add a proc definition to an object
    /// @param owner The object that owns this proc
    /// @param procDef The proc definition from the AST
    /// @return Pointer to the created DMProc
    DMProc* AddProc(const DreamPath& owner, class DMASTObjectProcDefinition* procDef);
    
    /// Get all objects in the tree (for iteration during compilation)
    /// @return Vector of pointers to all DMObject instances
    std::vector<DMObject*> GetAllObjects() const;
    
    /// Get all procs in the tree (for iteration during compilation)
    /// @return Vector of pointers to all DMProc instances
    std::vector<DMProc*> GetAllProcs() const;

private:
    /// Pointer to compiler for error reporting (may be null)
    DMCompiler* Compiler_;
    
    /// String to string ID mapping for O(1) deduplication lookup
    std::unordered_map<std::string, int> StringToStringId_;
    
    /// Path to type ID mapping for O(1) type lookup
    std::unordered_map<DreamPath, int, DreamPathHash> PathToTypeId_;
    
    /// Next available object ID (incremented on each object creation)
    int DmObjectIdCounter_;
    
    /// Next available proc ID (incremented on each proc creation)
    int DmProcIdCounter_;
    
    /// Initialize built-in global constants (TRUE, FALSE, NORTH, etc.)
    void InitializeBuiltInConstants();
    
    /// Determine the parent object for a given path
    /// Implements DM's special parent rules for /datum, /client, etc.
    /// @param path The path to determine parent for
    /// @return Pointer to parent DMObject (never null)
    DMObject* DetermineParent(const DreamPath& path);
};

} // namespace DMCompiler
