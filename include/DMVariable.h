#pragma once

#include "DreamPath.h"
#include "DMValueType.h"
#include <string>
#include <optional>

namespace DMCompiler {

// Forward declaration
class DMASTExpression;

/// <summary>
/// VarModifiers - Parsed Variable Modifiers
/// 
/// Holds the parsed modifiers extracted from a variable declaration path.
/// In DM, variable paths can contain modifiers like:
///   var/const/myvar = 5
///   var/tmp/mytmp
///   var/global/myglobal
///   var/static/mystatic
///   var/final/myfinal
/// 
/// This struct captures those modifiers and the remaining type path.
/// </summary>
struct VarModifiers {
    /// Is this a global/static variable?
    bool IsGlobal = false;
    
    /// Is this a const variable?
    bool IsConst = false;
    
    /// Is this a final variable?
    bool IsFinal = false;
    
    /// Is this a tmp variable?
    bool IsTmp = false;
    
    /// Is this a static variable? (alias for global)
    bool IsStatic = false;
    
    /// The remaining type path after extracting modifiers
    /// e.g., for "var/const/mob/foo", this would be "/mob"
    std::optional<DreamPath> TypePath;
    
    /// Parse variable modifiers from a DreamPath
    /// Extracts const, tmp, global, static, final from the path elements
    /// and returns the remaining elements as a type path.
    /// @param varPath The variable path to parse (e.g., /var/const/mob)
    /// @return VarModifiers with extracted flags and remaining type path
    static VarModifiers Parse(const DreamPath& varPath) {
        VarModifiers mods;
        std::vector<std::string> typeElements;
        bool passedVar = false;
        
        for (const auto& element : varPath.GetElements()) {
            // Skip the "var" element itself
            if (element == "var") {
                passedVar = true;
                continue;
            }
            
            // Check for modifier keywords
            if (element == "const") {
                mods.IsConst = true;
            }
            else if (element == "tmp") {
                mods.IsTmp = true;
            }
            else if (element == "global") {
                mods.IsGlobal = true;
            }
            else if (element == "static") {
                mods.IsStatic = true;
                mods.IsGlobal = true; // static is an alias for global
            }
            else if (element == "final") {
                mods.IsFinal = true;
            }
            else {
                // This is part of the type path
                typeElements.push_back(element);
            }
        }
        
        // Build the type path from remaining elements
        if (!typeElements.empty()) {
            mods.TypePath = DreamPath(DreamPath::PathType::Absolute, typeElements);
        }
        
        return mods;
    }
};

/// <summary>
/// DMComplexValueType - Complex Variable Type Information
/// 
/// Allows for more complex things than DMValueType does, such as supporting type paths.
/// Combines DMValueType flags with an optional type path constraint.
/// 
/// Examples:
/// - num: DMValueType::Num with no type path
/// - /mob: DMValueType::Path with typePath = /mob
/// - mob: DMValueType::Mob with no type path
/// - anything: DMValueType::Anything with no type path
/// </summary>
struct DMComplexValueType {
    /// Type flags (e.g., Num, Text, Mob|Obj)
    DMValueType Type;
    
    /// Optional type path constraint (e.g., /mob, /obj/item)
    std::optional<DreamPath> TypePath;
    
    /// Is this marked as unimplemented?
    bool IsUnimplemented;
    
    /// Is this marked as unsupported?
    bool IsUnsupported;
    
    /// Is this marked as compile-time readonly?
    bool IsCompileTimeReadOnly;
    
    /// Default constructor - creates "anything" type
    DMComplexValueType() 
        : Type(DMValueType::Anything)
        , TypePath(std::nullopt)
        , IsUnimplemented(false)
        , IsUnsupported(false)
        , IsCompileTimeReadOnly(false)
    {}
    
    /// Constructor from DMValueType
    explicit DMComplexValueType(DMValueType type, std::optional<DreamPath> typePath = std::nullopt)
        : Type(type & ~(DMValueType::Unimplemented | DMValueType::CompiletimeReadonly))
        , TypePath(typePath)
        , IsUnimplemented(HasFlag(type, DMValueType::Unimplemented))
        , IsUnsupported(HasFlag(type, DMValueType::Unsupported))
        , IsCompileTimeReadOnly(HasFlag(type, DMValueType::CompiletimeReadonly))
    {
        // Path type must have a type path
        if (HasFlag(type, DMValueType::Path) && !typePath.has_value()) {
            // In C# this throws, but we'll just clear the Path flag
            Type = Type & ~DMValueType::Path;
        }
    }
    
    /// Create a type representing "anything" (no constraints)
    static DMComplexValueType Anything() { 
        return DMComplexValueType(DMValueType::Anything); 
    }
    
    /// Check if this is the "anything" type
    bool IsAnything() const { 
        return static_cast<uint32_t>(Type) == 0; 
    }
    
    /// Check if this is a path type
    bool IsPath() const { 
        return HasFlag(Type, DMValueType::Path); 
    }
};

/// <summary>
/// DMVariable - Variable Declaration
/// 
/// Represents a variable declaration in DM code, either:
/// - Instance variables on objects (mob.health, obj.icon)
/// - Global variables at file scope
/// - Local variables in procs
/// 
/// Stores:
/// - Type constraint (optional, e.g., /mob for a mob-typed variable)
/// - Name
/// - Modifiers (global, const, final, tmp)
/// - Initial value expression
/// 
/// Usage:
/// - Created during parsing when var declarations are encountered
/// - Stored in DMObject::Variables or DMObjectTree::Globals
/// - At runtime, used to allocate and initialize variable storage
/// </summary>
class DMVariable {
public:
    /// Optional type path constraint (e.g., /mob, /obj/item)
    /// If present, variable can only hold values of that type
    std::optional<DreamPath> Type;
    
    /// Variable name (e.g., "health", "icon", "my_var")
    std::string Name;
    
    /// Is this a global/static variable?
    /// Global vars are shared across all instances
    bool IsGlobal;
    
    /// Is this a const variable?
    /// Const vars cannot be modified after initialization
    bool IsConst;
    
    /// Is this a final variable?
    /// Final vars can be set once at construction but not changed later
    bool IsFinal;
    
    /// Is this a tmp variable?
    /// Tmp vars are not saved in save files (transient state)
    bool IsTmp;
    
    /// Initial value expression (non-owning pointer to AST)
    /// The AST is owned by the DMASTFile and remains valid throughout compilation
    /// nullptr if no initial value was specified
    DMASTExpression* Value;
    
    /// Additional type information beyond basic type path
    DMComplexValueType ValType;
    
    /// Default constructor - creates an empty variable
    DMVariable();
    
    /// Full constructor
    /// @param type Optional type constraint
    /// @param name Variable name
    /// @param isGlobal Whether this is a global/static variable
    /// @param isConst Whether this is a const variable
    /// @param isFinal Whether this is a final variable
    /// @param isTmp Whether this is a tmp variable
    /// @param valType Additional type information
    DMVariable(
        std::optional<DreamPath> type,
        std::string name,
        bool isGlobal,
        bool isConst,
        bool isFinal,
        bool isTmp,
        DMComplexValueType valType = DMComplexValueType::Anything()
    );
    
    // Copy constructor
    DMVariable(const DMVariable& other);
    
    // Move constructor
    DMVariable(DMVariable&& other) noexcept;
    
    // Copy assignment
    DMVariable& operator=(const DMVariable& other);
    
    // Move assignment
    DMVariable& operator=(DMVariable&& other) noexcept;
    
    ~DMVariable() = default;
};

} // namespace DMCompiler
