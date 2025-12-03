#pragma once

#include "DreamPath.h"
#include "DMVariable.h"
#include "Location.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include <cstdint>
#include <variant>
#include <type_traits>

namespace DMCompiler {

// Forward declarations
class DMASTProcBlockInner;
class DMObject;
class DMCompiler;
class DMASTExpression;
class DMASTProcDefinition;
class DMASTDefinitionParameter;

/// <summary>
/// ProcAttributes - Bitflags for proc modifiers
/// Matches the C# ProcAttributes enum for binary compatibility
/// </summary>
enum class ProcAttributes : uint8_t {
    None = 0,
    Hidden = 1 << 0,      // proc/verb hidden from view
    Background = 1 << 1,  // Continues execution even if client disconnects
    WaitFor = 1 << 2,     // Wait for this proc to complete (default behavior)
    Popup = 1 << 3,       // Show as popup menu (verbs only)
    Instant = 1 << 4,     // Execute instantly without sleep
    IsOverride = 1 << 5,  // This proc overrides a parent proc
    Variadic = 1 << 6     // Accepts variable number of arguments
};

// Allow bitwise operations on ProcAttributes
inline ProcAttributes operator|(ProcAttributes a, ProcAttributes b) {
    return static_cast<ProcAttributes>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline ProcAttributes operator&(ProcAttributes a, ProcAttributes b) {
    return static_cast<ProcAttributes>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline bool HasFlag(ProcAttributes value, ProcAttributes flag) {
    return (value & flag) == flag;
}

/// <summary>
/// VerbSrc - Specifies what this verb can be used on
/// </summary>
enum class VerbSrc : uint16_t {
    Mob = 1 << 0,         // Can be used on mobs (default for /mob verbs)
    MobContents = 1 << 1, // Can be used on mob's inventory items
    MobGroup = 1 << 2,    // Can be used on mobs in same group
    World = 1 << 3,       // Can be used on world
    ObjContents = 1 << 4, // Can be used on obj's contents
    TurfContents = 1 << 5,// Can be used on turf's contents
    View = 1 << 6,        // Can be used on things in view
    OView = 1 << 7,       // Can be used on things in oview
    InWorld = 1 << 8      // Can be used on anything in world
};

/// <summary>
/// LocalVariable - Represents a local variable or parameter in a proc
/// </summary>
class LocalVariable {
public:
    /// Variable name
    std::string Name;
    
    /// Unique ID within this proc
    int Id;
    
    /// Is this a parameter (vs local var)?
    bool IsParameter;
    
    /// Optional type constraint
    std::optional<DreamPath> Type;
    
    /// Explicit value type information
    std::optional<DMComplexValueType> ExplicitValueType;
    
    /// Constructor for regular local variables
    LocalVariable(
        std::string name,
        int id,
        bool isParameter,
        std::optional<DreamPath> type = std::nullopt,
        std::optional<DMComplexValueType> explicitValueType = std::nullopt
    );
    
    virtual ~LocalVariable() = default;
};

/// <summary>
/// Constant - A type-safe representation of compile-time constant values
/// Used for const local variables, constant folding, and compile-time evaluation
/// </summary>
struct Constant {
    /// The actual constant value - can be null, integer, float, string, path, or resource
    std::variant<
        std::nullptr_t,     // null constant
        int64_t,            // integer constant
        double,             // float constant  
        std::string,        // string constant
        DreamPath           // path constant (also used for resources as path strings)
    > Value;
    
    /// Default constructor - creates null constant
    Constant() : Value(nullptr) {}
    
    /// Construct from nullptr (null constant)
    Constant(std::nullptr_t) : Value(nullptr) {}
    
    /// Construct from integer (use int64_t to avoid overload issues)
    explicit Constant(int64_t v) : Value(v) {}
    
    /// Construct from float/double
    explicit Constant(double v) : Value(v) {}
    explicit Constant(float v) : Value(static_cast<double>(v)) {}
    
    /// Construct from string
    explicit Constant(const std::string& v) : Value(v) {}
    explicit Constant(std::string&& v) : Value(std::move(v)) {}
    explicit Constant(const char* v) : Value(std::string(v)) {}
    
    /// Construct from path
    explicit Constant(const DreamPath& v) : Value(v) {}
    explicit Constant(DreamPath&& v) : Value(std::move(v)) {}
    
    /// Helper to create integer constant from any integer type
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, int64_t>>>
    static Constant FromInt(T v) { return Constant(static_cast<int64_t>(v)); }
    
    /// Check if this is a null constant
    bool IsNull() const { return std::holds_alternative<std::nullptr_t>(Value); }
    
    /// Check if this is an integer constant
    bool IsInteger() const { return std::holds_alternative<int64_t>(Value); }
    
    /// Check if this is a float constant
    bool IsFloat() const { return std::holds_alternative<double>(Value); }
    
    /// Check if this is a string constant
    bool IsString() const { return std::holds_alternative<std::string>(Value); }
    
    /// Check if this is a path constant
    bool IsPath() const { return std::holds_alternative<DreamPath>(Value); }
    
    /// Check if this is a numeric constant (integer or float)
    bool IsNumeric() const { return IsInteger() || IsFloat(); }
    
    /// Get integer value (throws if not integer)
    int64_t AsInteger() const { return std::get<int64_t>(Value); }
    
    /// Get float value (throws if not float)
    double AsFloat() const { return std::get<double>(Value); }
    
    /// Get string value (throws if not string)
    const std::string& AsString() const { return std::get<std::string>(Value); }
    
    /// Get path value (throws if not path)
    const DreamPath& AsPath() const { return std::get<DreamPath>(Value); }
    
    /// Get numeric value as double (works for both integer and float)
    double AsNumber() const {
        if (IsInteger()) return static_cast<double>(AsInteger());
        if (IsFloat()) return AsFloat();
        return 0.0;
    }
    
    /// Get a string representation for debugging
    std::string ToString() const {
        if (IsNull()) return "null";
        if (IsInteger()) return std::to_string(AsInteger());
        if (IsFloat()) return std::to_string(AsFloat());
        if (IsString()) return "\"" + AsString() + "\"";
        if (IsPath()) return AsPath().ToString();
        return "<unknown>";
    }
    
    /// Check equality
    bool operator==(const Constant& other) const {
        return Value == other.Value;
    }
    
    bool operator!=(const Constant& other) const {
        return !(*this == other);
    }
};

/// <summary>
/// LocalConstVariable - A local variable that's const with a known value
/// </summary>
class LocalConstVariable : public LocalVariable {
public:
    /// The constant value
    Constant ConstValue;
    
    LocalConstVariable(
        std::string name,
        int id,
        std::optional<DreamPath> type,
        Constant value
    );
};

/// <summary>
/// DMProc - Compiled Procedure Definition
/// 
/// Represents a single compiled proc (method) in the DM type system.
/// This stores everything needed to execute the proc at runtime:
/// - Bytecode instructions
/// - Parameter information
/// - Local variable declarations
/// - Attributes and metadata
/// 
/// Usage in Compiler Pipeline:
/// 1. Created during parsing when proc definitions are encountered
/// 2. Parameters and locals added during semantic analysis
/// 3. Bytecode emitted during code generation
/// 4. Stored in DMObjectTree::AllProcs for runtime execution
/// 
/// Relationship to DMObject:
/// - Each DMProc belongs to exactly one DMObject (its declaring type)
/// - DMObject::Procs maps proc names to lists of DMProc IDs (for overloads)
/// - Child types inherit parent procs (stored in parent's DMObject)
/// </summary>
class DMProc {
public:
    /// Unique identifier for this proc (index in DMObjectTree::AllProcs)
    const int Id;
    
    /// Proc name (e.g., "New", "Attack", "Move")
    std::string Name;
    
    /// The DMObject that owns this proc
    DMObject* OwningObject;
    
    /// Is this a verb (user-callable)?
    bool IsVerb;
    
    /// Is this a final proc (cannot be overridden)?
    bool IsFinal;
    
    /// Proc attributes (hidden, background, etc.)
    ProcAttributes Attributes;
    
    /// Parameter names in declaration order
    std::vector<std::string> Parameters;
    
    /// All local variables (parameters + local vars)
    std::unordered_map<std::string, std::unique_ptr<LocalVariable>> LocalVariables;
    
    /// Global variables accessed by this proc
    std::unordered_map<std::string, int> GlobalVariables;
    
    /// Bytecode for this proc (raw bytes in little-endian format)
    /// Generated by BytecodeWriter during compilation
    /// Format: sequence of opcodes (DreamProcOpcode) followed by their operands
    std::vector<uint8_t> Bytecode;
    
    /// Maximum stack size required by this proc
    int MaxStackSize;
    
    /// AST body for this proc (stored during Phase 3, compiled in Phase 4)
    /// This is a non-owning pointer - the AST is owned by the DMASTFile
    DMASTProcBlockInner* AstBody = nullptr;
    
    /// AST parameter definitions (stored during Phase 3, used in Phase 4)
    /// These are non-owning pointers - the AST is owned by the DMASTFile
    std::vector<DMASTDefinitionParameter*> AstParameters;
    
    /// Source location where this proc is defined
    Location SourceLocation;
    
    /// If set, reason why this proc is unsupported
    std::optional<std::string> UnsupportedReason;

    /// Raw set attributes captured from the source (lowercase attribute -> value)
    std::unordered_map<std::string, std::string> SetAttributes;
    
    // Verb-specific properties (only used if IsVerb == true)
    
    /// Verb source flags (what this verb can target)
    std::optional<VerbSrc> VerbSource;
    
    /// Display name for the verb (if different from Name)
    std::optional<std::string> VerbName;
    
    /// Category for organizing verbs in UI
    std::optional<std::string> VerbCategory;
    
    /// Description shown in verb help
    std::optional<std::string> VerbDescription;
    
    /// Invisibility level (lower values = more visible)
    int8_t Invisibility;
    
    /// Constructor
    /// @param id Unique proc identifier
    /// @param name Proc name
    /// @param owningObject DMObject that declares this proc
    /// @param isVerb Whether this is a verb
    /// @param location Source location
    DMProc(
        int id,
        std::string name,
        DMObject* owningObject,
        bool isVerb = false,
        Location location = Location()
    );
    
    ~DMProc() = default;
    
    // No copy (procs are owned by unique_ptr in DMObjectTree)
    DMProc(const DMProc&) = delete;
    DMProc& operator=(const DMProc&) = delete;
    
    // Move allowed
    DMProc(DMProc&&) noexcept = default;
    DMProc& operator=(DMProc&&) noexcept = default;
    
    /// Add a parameter to this proc
    /// @param name Parameter name
    /// @param type Optional type constraint
    /// @param explicitValueType Optional explicit value type
    /// @return Pointer to the created LocalVariable
    LocalVariable* AddParameter(
        const std::string& name,
        std::optional<DreamPath> type = std::nullopt,
        std::optional<DMComplexValueType> explicitValueType = std::nullopt
    );
    
    /// Add a local variable to this proc
    /// @param name Variable name
    /// @param type Optional type constraint
    /// @return Pointer to the created LocalVariable
    LocalVariable* AddLocalVariable(
        const std::string& name,
        std::optional<DreamPath> type = std::nullopt
    );
    
    /// Add a const local variable with a known value
    /// @param name Variable name
    /// @param type Optional type constraint
    /// @param value The constant value
    /// @return Pointer to the created LocalConstVariable
    LocalConstVariable* AddLocalConst(
        const std::string& name,
        std::optional<DreamPath> type,
        Constant value
    );
    
    /// Get a local variable by name (includes parameters)
    /// @param name Variable name to look up
    /// @return Pointer to variable if found, nullptr otherwise
    LocalVariable* GetLocalVariable(const std::string& name);
    const LocalVariable* GetLocalVariable(const std::string& name) const;
    
    /// Check if this proc has a parameter with the given name
    /// @param name Parameter name
    /// @return true if parameter exists
    bool HasParameter(const std::string& name) const;
    
    /// Get the number of parameters
    /// @return Parameter count
    int GetParameterCount() const { return static_cast<int>(Parameters.size()); }
    
    /// Get the number of local variables (excluding parameters)
    /// @return Local variable count
    int GetLocalVariableCount() const;
    
    /// Check if this proc is marked as unsupported
    /// @return true if UnsupportedReason is set
    bool IsUnsupported() const { return UnsupportedReason.has_value(); }
    
    /// Mark this proc as unsupported with a reason
    /// @param reason Why this proc cannot be compiled
    void MarkUnsupported(const std::string& reason);

    /// Apply a set statement attribute/value pair to this proc
    /// @param attribute Attribute name as written in source
    /// @param value String representation of the constant value
    /// @param boolValue Optional boolean interpretation of the value
    void ApplySetAttribute(const std::string& attribute, const std::string& value, std::optional<bool> boolValue);
    
    /// Set the verb source (what this verb can be used on)
    /// @param source The verb source flags
    void SetVerbSource(VerbSrc source);

    /// Compile this proc's AST body into bytecode
    /// This is called during Phase 4 (bytecode emission)
    /// @param compiler Pointer to the compiler for emitting bytecode
    void Compile(class DMCompiler* compiler);
    
    /// Get a string representation for debugging
    /// Format: "/mob/proc/Attack(target, damage)"
    /// @return String representation
    std::string ToString() const;
    
    /// Get the current enumerator ID and increment the counter
    /// Used for creating list/type/range enumerators
    /// @return The current enumerator ID before incrementing
    int GetNextEnumeratorId() { return EnumeratorIdCounter_++; }
    
    /// Get the current enumerator ID without incrementing
    /// Used for enumerate/destroy operations
    /// @return The current enumerator ID
    int GetCurrentEnumeratorId() const { return EnumeratorIdCounter_ - 1; }
    
    /// Decrement and return the enumerator ID
    /// Used when destroying enumerators
    /// @return The decremented enumerator ID
    int DecrementEnumeratorId() { return --EnumeratorIdCounter_; }

private:
    /// Next available local variable ID
    int LocalVariableIdCounter_;
    
    /// Enumerator ID counter for for-in loops
    int EnumeratorIdCounter_ = 0;
};

} // namespace DMCompiler
