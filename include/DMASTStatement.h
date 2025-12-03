#pragma once

#include "DMAST.h"
#include "DMASTExpression.h"
#include <memory>
#include <vector>
#include <string>

namespace DMCompiler {

/// <summary>
/// Base class for all statement nodes
/// </summary>
class DMASTStatement : public DMASTNode {
public:
    explicit DMASTStatement(const Location& location) : DMASTNode(location) {}
    virtual ~DMASTStatement() = default;
};

/// <summary>
/// Base class for proc/verb statement nodes
/// </summary>
class DMASTProcStatement : public DMASTStatement {
public:
    explicit DMASTProcStatement(const Location& location) : DMASTStatement(location) {}
};

/// <summary>
/// Base class for object definition statement nodes
/// </summary>
class DMASTObjectStatement : public DMASTStatement {
public:
    explicit DMASTObjectStatement(const Location& location) : DMASTStatement(location) {}
};

// ============================================================================
// Proc/Verb Statements
// ============================================================================

/// <summary>
/// Expression statement (just an expression with a semicolon)
/// </summary>
class DMASTProcStatementExpression : public DMASTProcStatement {
public:
    std::unique_ptr<DMASTExpression> Expression;
    
    DMASTProcStatementExpression(const Location& location, std::unique_ptr<DMASTExpression> expr)
        : DMASTProcStatement(location), Expression(std::move(expr)) {}
};

/// <summary>
/// Variable declaration in proc/verb (var/x = 5)
/// </summary>
class DMASTProcStatementVarDeclaration : public DMASTProcStatement {
public:
    struct Decl {
        std::string Name;
        std::optional<DreamPath> TypePath;
        std::unique_ptr<DMASTExpression> Value;
        std::optional<DMComplexValueType> ExplicitValueType; // From "as" keyword
        bool IsList;
        Location Loc;

        Decl(const Location& loc, std::string name, std::optional<DreamPath> typePath,
             std::unique_ptr<DMASTExpression> value, std::optional<DMComplexValueType> explicitValueType, bool isList)
            : Name(name), TypePath(typePath), Value(std::move(value)),
              ExplicitValueType(explicitValueType), IsList(isList), Loc(loc) {}
    };

    std::vector<Decl> Decls;
    
    DMASTProcStatementVarDeclaration(const Location& location, std::vector<Decl> decls)
        : DMASTProcStatement(location), Decls(std::move(decls)) {}
};

/// <summary>
/// Return statement
/// </summary>
class DMASTProcStatementReturn : public DMASTProcStatement {
public:
    std::unique_ptr<DMASTExpression> Value;
    
    explicit DMASTProcStatementReturn(const Location& location, 
                                     std::unique_ptr<DMASTExpression> value = nullptr)
        : DMASTProcStatement(location), Value(std::move(value)) {}
};

/// <summary>
/// Break statement
/// </summary>
class DMASTProcStatementBreak : public DMASTProcStatement {
public:
    std::unique_ptr<DMASTIdentifier> Label; // Optional label
    
    explicit DMASTProcStatementBreak(const Location& location,
                                    std::unique_ptr<DMASTIdentifier> label = nullptr)
        : DMASTProcStatement(location), Label(std::move(label)) {}
};

/// <summary>
/// Continue statement
/// </summary>
class DMASTProcStatementContinue : public DMASTProcStatement {
public:
    std::unique_ptr<DMASTIdentifier> Label; // Optional label
    
    explicit DMASTProcStatementContinue(const Location& location,
                                       std::unique_ptr<DMASTIdentifier> label = nullptr)
        : DMASTProcStatement(location), Label(std::move(label)) {}
};

/// <summary>
/// Goto statement
/// </summary>
class DMASTProcStatementGoto : public DMASTProcStatement {
public:
    std::unique_ptr<DMASTIdentifier> Label;
    
    DMASTProcStatementGoto(const Location& location, std::unique_ptr<DMASTIdentifier> label)
        : DMASTProcStatement(location), Label(std::move(label)) {}
};

/// <summary>
/// Label statement (label:)
/// </summary>
class DMASTProcStatementLabel : public DMASTProcStatement {
public:
    std::string Name;
    std::unique_ptr<DMASTProcStatement> Body;
    
    DMASTProcStatementLabel(const Location& location, 
                           const std::string& name,
                           std::unique_ptr<DMASTProcStatement> body = nullptr)
        : DMASTProcStatement(location), Name(name), Body(std::move(body)) {}
};

/// <summary>
/// Del statement (del obj)
/// </summary>
class DMASTProcStatementDel : public DMASTProcStatement {
public:
    std::unique_ptr<DMASTExpression> Value;
    
    DMASTProcStatementDel(const Location& location, std::unique_ptr<DMASTExpression> value)
        : DMASTProcStatement(location), Value(std::move(value)) {}
};

/// <summary>
/// Spawn statement (spawn(delay) { ... })
/// </summary>
class DMASTProcStatementSpawn : public DMASTProcStatement {
public:
    std::unique_ptr<DMASTExpression> Delay; // nullptr means spawn(0)
    std::unique_ptr<DMASTProcBlockInner> Body;
    
    DMASTProcStatementSpawn(const Location& location,
                           std::unique_ptr<DMASTExpression> delay,
                           std::unique_ptr<DMASTProcBlockInner> body)
        : DMASTProcStatement(location), Delay(std::move(delay)), Body(std::move(body)) {}
};

/// <summary>
/// If statement
/// </summary>
class DMASTProcStatementIf : public DMASTProcStatement {
public:
    std::unique_ptr<DMASTExpression> Condition;
    std::unique_ptr<DMASTProcBlockInner> Body;
    std::unique_ptr<DMASTProcBlockInner> ElseBody; // nullptr if no else
    
    DMASTProcStatementIf(const Location& location,
                        std::unique_ptr<DMASTExpression> condition,
                        std::unique_ptr<DMASTProcBlockInner> body,
                        std::unique_ptr<DMASTProcBlockInner> elseBody = nullptr)
        : DMASTProcStatement(location), Condition(std::move(condition)), 
          Body(std::move(body)), ElseBody(std::move(elseBody)) {}
};

/// <summary>
/// For loop statement
/// </summary>
class DMASTProcStatementFor : public DMASTProcStatement {
public:
    std::unique_ptr<DMASTProcStatement> Initializer; // var x = 0 or x = 0
    std::unique_ptr<DMASTExpression> Condition;
    std::unique_ptr<DMASTExpression> Increment;
    std::unique_ptr<DMASTProcBlockInner> Body;
    
    DMASTProcStatementFor(const Location& location,
                         std::unique_ptr<DMASTProcStatement> initializer,
                         std::unique_ptr<DMASTExpression> condition,
                         std::unique_ptr<DMASTExpression> increment,
                         std::unique_ptr<DMASTProcBlockInner> body)
        : DMASTProcStatement(location), Initializer(std::move(initializer)),
          Condition(std::move(condition)), Increment(std::move(increment)), Body(std::move(body)) {}
};

/// <summary>
/// For-in loop statement (for(var x in list))
/// </summary>
class DMASTProcStatementForIn : public DMASTProcStatement {
public:
    /// <summary>
    /// Enhanced variable declaration information for for-in loops
    /// Captures variable name, type path, and type filter from declarations like:
    /// - for(var/mob/M in world)
    /// - for(var/mob/M as /mob|mob in world)
    /// - for(var/atom/movable/A in src)
    /// </summary>
    struct VariableDeclaration {
        std::string Name;                           // Variable name (e.g., "M", "A")
        std::optional<DreamPath> TypePath;          // Type path (e.g., /mob, /atom/movable)
        std::optional<std::string> TypeFilter;      // Type filter (e.g., "/mob|mob")
        Location Loc;                               // Source location
        
        VariableDeclaration()
            : Name(""), TypePath(std::nullopt), TypeFilter(std::nullopt), Loc() {}
        
        VariableDeclaration(const std::string& name,
                          std::optional<DreamPath> typePath = std::nullopt,
                          std::optional<std::string> typeFilter = std::nullopt,
                          const Location& loc = Location())
            : Name(name), TypePath(typePath), TypeFilter(typeFilter), Loc(loc) {}
    };
    
    std::unique_ptr<DMASTExpression> Variable; // Could be declaration or identifier
    VariableDeclaration VarDecl;               // Enhanced variable information
    std::unique_ptr<DMASTExpression> List;
    std::unique_ptr<DMASTProcBlockInner> Body;
    
    DMASTProcStatementForIn(const Location& location,
                           std::unique_ptr<DMASTExpression> variable,
                           std::unique_ptr<DMASTExpression> list,
                           std::unique_ptr<DMASTProcBlockInner> body)
        : DMASTProcStatement(location), Variable(std::move(variable)), 
          VarDecl(), List(std::move(list)), Body(std::move(body)) {}
    
    DMASTProcStatementForIn(const Location& location,
                           std::unique_ptr<DMASTExpression> variable,
                           const VariableDeclaration& varDecl,
                           std::unique_ptr<DMASTExpression> list,
                           std::unique_ptr<DMASTProcBlockInner> body)
        : DMASTProcStatement(location), Variable(std::move(variable)), 
          VarDecl(varDecl), List(std::move(list)), Body(std::move(body)) {}
};

/// <summary>
/// For-range loop statement (for(var/x = start to end) or for(var/x = start to end step inc))
/// </summary>
class DMASTProcStatementForRange : public DMASTProcStatement {
public:
    std::unique_ptr<DMASTExpression> Initializer;  // The initializer expression (var/x = start)
    std::unique_ptr<DMASTExpression> End;          // Ending value
    std::unique_ptr<DMASTExpression> Step;         // Optional step value (nullptr means step of 1)
    std::unique_ptr<DMASTProcBlockInner> Body;
    
    DMASTProcStatementForRange(const Location& location,
                              std::unique_ptr<DMASTExpression> initializer,
                              std::unique_ptr<DMASTExpression> end,
                              std::unique_ptr<DMASTExpression> step,
                              std::unique_ptr<DMASTProcBlockInner> body)
        : DMASTProcStatement(location), Initializer(std::move(initializer)),
          End(std::move(end)), Step(std::move(step)), Body(std::move(body)) {}
};

/// <summary>
/// While loop statement
/// </summary>
class DMASTProcStatementWhile : public DMASTProcStatement {
public:
    std::unique_ptr<DMASTExpression> Condition;
    std::unique_ptr<DMASTProcBlockInner> Body;
    
    DMASTProcStatementWhile(const Location& location,
                           std::unique_ptr<DMASTExpression> condition,
                           std::unique_ptr<DMASTProcBlockInner> body)
        : DMASTProcStatement(location), Condition(std::move(condition)), Body(std::move(body)) {}
};

/// <summary>
/// Do-while loop statement
/// </summary>
class DMASTProcStatementDoWhile : public DMASTProcStatement {
public:
    std::unique_ptr<DMASTProcBlockInner> Body;
    std::unique_ptr<DMASTExpression> Condition;
    
    DMASTProcStatementDoWhile(const Location& location,
                             std::unique_ptr<DMASTProcBlockInner> body,
                             std::unique_ptr<DMASTExpression> condition)
        : DMASTProcStatement(location), Body(std::move(body)), Condition(std::move(condition)) {}
};

/// <summary>
/// Switch statement
/// </summary>
class DMASTProcStatementSwitch : public DMASTProcStatement {
public:
    struct SwitchCase {
        std::vector<std::unique_ptr<DMASTExpression>> Values; // Multiple values per case
        std::unique_ptr<DMASTProcBlockInner> Body;
    };
    
    std::unique_ptr<DMASTExpression> Value;
    std::vector<SwitchCase> Cases;
    
    DMASTProcStatementSwitch(const Location& location,
                            std::unique_ptr<DMASTExpression> value,
                            std::vector<SwitchCase> cases)
        : DMASTProcStatement(location), Value(std::move(value)), Cases(std::move(cases)) {}
};

/// <summary>
/// Try-catch statement
/// </summary>
class DMASTProcStatementTryCatch : public DMASTProcStatement {
public:
    std::unique_ptr<DMASTProcBlockInner> TryBody;
    std::unique_ptr<DMASTIdentifier> CatchVariable; // Optional catch(e)
    std::unique_ptr<DMASTProcBlockInner> CatchBody;
    
    DMASTProcStatementTryCatch(const Location& location,
                              std::unique_ptr<DMASTProcBlockInner> tryBody,
                              std::unique_ptr<DMASTIdentifier> catchVariable,
                              std::unique_ptr<DMASTProcBlockInner> catchBody)
        : DMASTProcStatement(location), TryBody(std::move(tryBody)),
          CatchVariable(std::move(catchVariable)), CatchBody(std::move(catchBody)) {}
};

/// <summary>
/// Throw statement
/// </summary>
class DMASTProcStatementThrow : public DMASTProcStatement {
public:
    std::unique_ptr<DMASTExpression> Value;
    
    DMASTProcStatementThrow(const Location& location, std::unique_ptr<DMASTExpression> value)
        : DMASTProcStatement(location), Value(std::move(value)) {}
};

/// <summary>
/// Set statement (set name = "value", set src = usr, etc.)
/// </summary>
class DMASTProcStatementSet : public DMASTProcStatement {
public:
    std::string Attribute;
    std::unique_ptr<DMASTExpression> Value;
    
    DMASTProcStatementSet(const Location& location,
                         const std::string& attribute,
                         std::unique_ptr<DMASTExpression> value)
        : DMASTProcStatement(location), Attribute(attribute), Value(std::move(value)) {}
};

// ============================================================================
// Object Definition Statements
// ============================================================================

/// <summary>
/// Variable definition in object (var/x = 5)
/// </summary>
class DMASTObjectVarDefinition : public DMASTObjectStatement {
public:
    std::string Name;
    DMASTPath TypePath;
    std::unique_ptr<DMASTExpression> Value;
    std::optional<DMComplexValueType> ExplicitValueType; // From "as" keyword
    
    DMASTObjectVarDefinition(const Location& location,
                            const std::string& name,
                            const DMASTPath& typePath,
                            std::unique_ptr<DMASTExpression> value = nullptr,
                            std::optional<DMComplexValueType> explicitValueType = std::nullopt)
        : DMASTObjectStatement(location), Name(name), TypePath(typePath), Value(std::move(value)),
          ExplicitValueType(explicitValueType) {}
};

/// <summary>
/// Variable override in object (x = 5)
/// </summary>
class DMASTObjectVarOverride : public DMASTObjectStatement {
public:
    std::string VarName;
    std::unique_ptr<DMASTExpression> Value;
    
    DMASTObjectVarOverride(const Location& location,
                          const std::string& varName,
                          std::unique_ptr<DMASTExpression> value)
        : DMASTObjectStatement(location), VarName(varName), Value(std::move(value)) {}
};

/// <summary>
/// Proc/verb definition
/// </summary>
class DMASTObjectProcDefinition : public DMASTObjectStatement {
public:
    DreamPath ObjectPath; // Path to the object this proc belongs to (e.g., /mob for /mob/proc/test)
    std::string Name;
    std::vector<std::unique_ptr<DMASTDefinitionParameter>> Parameters;
    std::unique_ptr<DMASTProcBlockInner> Body;
    bool IsVerb; // true = verb, false = proc
    
    DMASTObjectProcDefinition(const Location& location,
                             const DreamPath& objectPath,
                             const std::string& name,
                             std::vector<std::unique_ptr<DMASTDefinitionParameter>> parameters,
                             std::unique_ptr<DMASTProcBlockInner> body,
                             bool isVerb = false)
        : DMASTObjectStatement(location), ObjectPath(objectPath), Name(name), Parameters(std::move(parameters)),
          Body(std::move(body)), IsVerb(isVerb) {}
};

/// <summary>
/// Object definition (/mob/player { ... })
/// </summary>
class DMASTObjectDefinition : public DMASTObjectStatement {
public:
    DMASTPath Path;
    std::vector<std::unique_ptr<DMASTObjectStatement>> InnerStatements;
    
    DMASTObjectDefinition(const Location& location,
                         const DMASTPath& path,
                         std::vector<std::unique_ptr<DMASTObjectStatement>> innerStatements)
        : DMASTObjectStatement(location), Path(path), InnerStatements(std::move(innerStatements)) {}
};

} // namespace DMCompiler
