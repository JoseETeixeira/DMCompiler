#pragma once

#include "Location.h"
#include "DreamPath.h"
#include "DMVariable.h"
#include <memory>
#include <vector>
#include <string>
#include <optional>

namespace DMCompiler {

// Forward declarations
class DMASTExpression;
class DMASTStatement;
class DMASTProcStatement;
class DMASTObjectStatement;

/// <summary>
/// Base class for all AST nodes
/// </summary>
class DMASTNode {
public:
    Location Location_;
    
    explicit DMASTNode(const Location& location) : Location_(location) {}
    virtual ~DMASTNode() = default;
    
    // Prevent copying
    DMASTNode(const DMASTNode&) = delete;
    DMASTNode& operator=(const DMASTNode&) = delete;
    
    // Allow moving
    DMASTNode(DMASTNode&&) = default;
    DMASTNode& operator=(DMASTNode&&) = default;
    
    virtual std::string ToString() const;
};

/// <summary>
/// Root node representing an entire DM file
/// </summary>
class DMASTFile : public DMASTNode {
public:
    std::vector<std::unique_ptr<DMASTStatement>> Statements;
    
    DMASTFile(const Location& location, std::vector<std::unique_ptr<DMASTStatement>> statements);
};

/// <summary>
/// Block of statements in object definitions
/// </summary>
class DMASTBlockInner : public DMASTNode {
public:
    std::vector<std::unique_ptr<DMASTStatement>> Statements;
    
    DMASTBlockInner(const Location& location, std::vector<std::unique_ptr<DMASTStatement>> statements);
};

/// <summary>
/// Block of statements in proc/verb definitions
/// </summary>
class DMASTProcBlockInner : public DMASTNode {
public:
    std::vector<std::unique_ptr<DMASTProcStatement>> Statements;
    std::vector<std::unique_ptr<DMASTProcStatement>> SetStatements; // Hoisted set statements
    
    // Empty block
    explicit DMASTProcBlockInner(const Location& location);
    
    // Block with statements
    DMASTProcBlockInner(const Location& location, 
                        std::vector<std::unique_ptr<DMASTProcStatement>> statements,
                        std::vector<std::unique_ptr<DMASTProcStatement>> setStatements = {});
};

/// <summary>
/// DM type path (e.g., /mob/player, /obj/item)
/// This is a value type, not a node, so it doesn't inherit from DMASTNode
/// </summary>
struct DMASTPath {
    DreamPath Path;
    bool IsOperator; // For operator overloading paths
    Location Location_;
    
    DMASTPath(const Location& location, const DreamPath& path, bool isOperator = false)
        : Location_(location), Path(path), IsOperator(isOperator) {}
    
    // Allow copying and moving
    DMASTPath(const DMASTPath&) = default;
    DMASTPath& operator=(const DMASTPath&) = default;
    DMASTPath(DMASTPath&&) = default;
    DMASTPath& operator=(DMASTPath&&) = default;
};

/// <summary>
/// Call parameter in function/proc calls
/// </summary>
class DMASTCallParameter : public DMASTNode {
public:
    std::unique_ptr<DMASTExpression> Value;
    std::unique_ptr<DMASTExpression> Key; // For named parameters
    
    DMASTCallParameter(const Location& location, 
                      std::unique_ptr<DMASTExpression> value,
                      std::unique_ptr<DMASTExpression> key = nullptr);
};

/// <summary>
/// Definition parameter in proc/verb definitions
/// </summary>
class DMASTDefinitionParameter : public DMASTNode {
public:
    std::string Name;
    DreamPath TypePath;
    bool IsList;
    std::unique_ptr<DMASTExpression> DefaultValue;
    std::unique_ptr<DMASTExpression> PossibleValues; // For input() constraints
    std::optional<DMComplexValueType> ExplicitValueType; // From "as" keyword
    
    DMASTDefinitionParameter(const Location& location,
                            const std::string& name,
                            const DreamPath& typePath,
                            bool isList = false,
                            std::unique_ptr<DMASTExpression> defaultValue = nullptr,
                            std::unique_ptr<DMASTExpression> possibleValues = nullptr,
                            std::optional<DMComplexValueType> explicitValueType = std::nullopt);
};

} // namespace DMCompiler
