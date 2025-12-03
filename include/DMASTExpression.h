#pragma once

#include "DMAST.h"
#include "JsonWriter.h"
#include "DMValueType.h"
#include <vector>
#include <memory>

namespace DMCompiler {

// Forward declarations
class DMCompiler;

/// <summary>
/// Base class for all expression nodes
/// </summary>
class DMASTExpression : public DMASTNode {
public:
    explicit DMASTExpression(const Location& location) : DMASTNode(location) {}
    virtual ~DMASTExpression() = default;
    
    virtual std::vector<DMASTExpression*> GetLeaves() { return {}; }
    
    /// <summary>
    /// Try to convert this expression to a JSON representation.
    /// Returns true if the expression can be serialized to JSON, false otherwise.
    /// Only constant expressions can be serialized.
    /// </summary>
    virtual bool TryAsJsonRepresentation(DMCompiler* compiler, JsonValue& outJson) { 
        return false; // Default: expressions cannot be serialized
    }
};

/// <summary>
/// Invalid expression (used for error recovery)
/// </summary>
class DMASTInvalidExpression : public DMASTExpression {
public:
    explicit DMASTInvalidExpression(const Location& location) : DMASTExpression(location) {}
};

/// <summary>
/// Void expression
/// </summary>
class DMASTVoid : public DMASTExpression {
public:
    explicit DMASTVoid(const Location& location) : DMASTExpression(location) {}
};

/// <summary>
/// Identifier (variable name, proc name, etc.)
/// </summary>
class DMASTIdentifier : public DMASTExpression {
public:
    std::string Identifier;
    
    DMASTIdentifier(const Location& location, const std::string& identifier)
        : DMASTExpression(location), Identifier(identifier) {}
};

// ============================================================================
// Constant Expressions
// ============================================================================

/// <summary>
/// Integer constant
/// </summary>
class DMASTConstantInteger : public DMASTExpression {
public:
    int32_t Value;
    
    DMASTConstantInteger(const Location& location, int32_t value)
        : DMASTExpression(location), Value(value) {}
    
    bool TryAsJsonRepresentation(DMCompiler* compiler, JsonValue& outJson) override;
};

/// <summary>
/// Float constant
/// </summary>
class DMASTConstantFloat : public DMASTExpression {
public:
    float Value;
    
    DMASTConstantFloat(const Location& location, float value)
        : DMASTExpression(location), Value(value) {}
    
    bool TryAsJsonRepresentation(DMCompiler* compiler, JsonValue& outJson) override;
};

/// <summary>
/// String constant
/// </summary>
class DMASTConstantString : public DMASTExpression {
public:
    std::string Value;
    
    DMASTConstantString(const Location& location, const std::string& value)
        : DMASTExpression(location), Value(value) {}
    
    bool TryAsJsonRepresentation(DMCompiler* compiler, JsonValue& outJson) override;
};

/// <summary>
/// Interpolated string with embedded expressions: "Hello [name]!"
/// Contains alternating string parts and expressions
/// </summary>
class DMASTStringFormat : public DMASTExpression {
public:
    // String parts (one more than expressions count, or same if string ends with expression)
    std::vector<std::string> StringParts;
    // Embedded expressions
    std::vector<std::unique_ptr<DMASTExpression>> Expressions;
    
    DMASTStringFormat(const Location& location,
                      std::vector<std::string> stringParts,
                      std::vector<std::unique_ptr<DMASTExpression>> expressions)
        : DMASTExpression(location)
        , StringParts(std::move(stringParts))
        , Expressions(std::move(expressions)) {}
    
    bool TryAsJsonRepresentation(DMCompiler* compiler, JsonValue& outJson) override;
};

/// <summary>
/// Resource constant ('icon.dmi')
/// </summary>
class DMASTConstantResource : public DMASTExpression {
public:
    std::string Path;
    
    DMASTConstantResource(const Location& location, const std::string& path)
        : DMASTExpression(location), Path(path) {}
    
    bool TryAsJsonRepresentation(DMCompiler* compiler, JsonValue& outJson) override;
};

/// <summary>
/// Null constant
/// </summary>
class DMASTConstantNull : public DMASTExpression {
public:
    explicit DMASTConstantNull(const Location& location) : DMASTExpression(location) {}
    
    bool TryAsJsonRepresentation(DMCompiler* compiler, JsonValue& outJson) override;
};

/// <summary>
/// Path constant (/mob/player)
/// </summary>
class DMASTConstantPath : public DMASTExpression {
public:
    DMASTPath Path;
    
    DMASTConstantPath(const Location& location, const DMASTPath& path)
        : DMASTExpression(location), Path(path) {}
    
    bool TryAsJsonRepresentation(DMCompiler* compiler, JsonValue& outJson) override;
};

// ============================================================================
// Binary Expressions
// ============================================================================

enum class BinaryOperator {
    // Arithmetic
    Add, Subtract, Multiply, Divide, Modulo, Power,
    
    // Comparison
    Equal, NotEqual, Less, Greater, LessOrEqual, GreaterOrEqual,
    
    // Equivalence (DM-specific: ~= and ~!)
    Equivalent, NotEquivalent,
    
    // Logical
    LogicalAnd, LogicalOr,
    
    // Bitwise
    BitwiseAnd, BitwiseOr, BitwiseXor, LeftShift, RightShift,
    
    // DM-specific
    In, To, Append, Combine, Mask, Remove
};

/// <summary>
/// Binary expression (a + b, a == b, etc.)
/// </summary>
class DMASTExpressionBinary : public DMASTExpression {
public:
    BinaryOperator Operator;
    std::unique_ptr<DMASTExpression> Left;
    std::unique_ptr<DMASTExpression> Right;
    
    DMASTExpressionBinary(const Location& location, BinaryOperator op,
                         std::unique_ptr<DMASTExpression> left,
                         std::unique_ptr<DMASTExpression> right)
        : DMASTExpression(location), Operator(op), Left(std::move(left)), Right(std::move(right)) {}
    
    std::vector<DMASTExpression*> GetLeaves() override {
        return {Left.get(), Right.get()};
    }
};

// ============================================================================
// Unary Expressions
// ============================================================================

enum class UnaryOperator {
    Negate,         // -x
    Not,            // !x
    BitNot,         // ~x
    PreIncrement,   // ++x
    PreDecrement,   // --x
    PostIncrement,  // x++
    PostDecrement,  // x--
};

/// <summary>
/// Unary expression (-x, !x, ++x, etc.)
/// </summary>
class DMASTExpressionUnary : public DMASTExpression {
public:
    UnaryOperator Operator;
    std::unique_ptr<DMASTExpression> Expression;
    
    DMASTExpressionUnary(const Location& location, UnaryOperator op,
                        std::unique_ptr<DMASTExpression> expr)
        : DMASTExpression(location), Operator(op), Expression(std::move(expr)) {}
    
    std::vector<DMASTExpression*> GetLeaves() override {
        return {Expression.get()};
    }
};

// ============================================================================
// Special Expressions
// ============================================================================

/// <summary>
/// List literal ([1, 2, 3] or list(1, 2, 3))
/// </summary>
class DMASTList : public DMASTExpression {
public:
    std::vector<std::unique_ptr<DMASTCallParameter>> Values;
    bool IsAssociativeList; // list() vs alist()
    
    DMASTList(const Location& location,
             std::vector<std::unique_ptr<DMASTCallParameter>> values,
             bool isAssociativeList = false)
        : DMASTExpression(location), Values(std::move(values)), IsAssociativeList(isAssociativeList) {}
};

/// <summary>
/// newlist(...) - creates a list of new objects
/// </summary>
class DMASTNewList : public DMASTExpression {
public:
    std::vector<std::unique_ptr<DMASTCallParameter>> Parameters;
    
    DMASTNewList(const Location& location,
                std::vector<std::unique_ptr<DMASTCallParameter>> parameters)
        : DMASTExpression(location), Parameters(std::move(parameters)) {}
};

/// <summary>
/// new expression (new /mob/player or new type())
/// </summary>
class DMASTNewPath : public DMASTExpression {
public:
    std::unique_ptr<DMASTExpression> Path;
    std::vector<std::unique_ptr<DMASTCallParameter>> Parameters;
    
    DMASTNewPath(const Location& location,
                std::unique_ptr<DMASTExpression> path,
                std::vector<std::unique_ptr<DMASTCallParameter>> parameters = {})
        : DMASTExpression(location), Path(std::move(path)), Parameters(std::move(parameters)) {}
};

/// <summary>
/// Procedure/verb call (proc(), verb(), src.proc())
/// </summary>
class DMASTCall : public DMASTExpression {
public:
    std::unique_ptr<DMASTExpression> Target; // What we're calling (identifier, dereference, etc.)
    std::vector<std::unique_ptr<DMASTCallParameter>> Parameters;
    
    // For input() built-in: "as type" and "in list" clauses
    DMValueType InputTypes = DMValueType::Anything;  // Type flags for input() "as" clause
    std::unique_ptr<DMASTExpression> InputList;      // List expression for input() "in" clause
    bool IsInputCall = false;                         // True if this is an input() call
    
    DMASTCall(const Location& location,
             std::unique_ptr<DMASTExpression> target,
             std::vector<std::unique_ptr<DMASTCallParameter>> parameters)
        : DMASTExpression(location), Target(std::move(target)), Parameters(std::move(parameters)) {}
};

/// <summary>
/// Member access (a.b, a:b, a?.b)
/// </summary>
enum class DereferenceType {
    Direct,       // .
    Search,       // :
    Safe,         // ?.
    SafeSearch,   // ?:
    Index         // []
};

class DMASTDereference : public DMASTExpression {
public:
    std::unique_ptr<DMASTExpression> Expression;
    DereferenceType Type;
    std::unique_ptr<DMASTExpression> Property; // Could be identifier or expression
    
    DMASTDereference(const Location& location,
                    std::unique_ptr<DMASTExpression> expr,
                    DereferenceType type,
                    std::unique_ptr<DMASTExpression> property)
        : DMASTExpression(location), Expression(std::move(expr)), Type(type), Property(std::move(property)) {}
};

/// <summary>
/// Ternary expression (condition ? true_expr : false_expr)
/// </summary>
class DMASTTernary : public DMASTExpression {
public:
    std::unique_ptr<DMASTExpression> Condition;
    std::unique_ptr<DMASTExpression> TrueExpression;
    std::unique_ptr<DMASTExpression> FalseExpression;
    
    DMASTTernary(const Location& location,
                std::unique_ptr<DMASTExpression> condition,
                std::unique_ptr<DMASTExpression> trueExpr,
                std::unique_ptr<DMASTExpression> falseExpr)
        : DMASTExpression(location), Condition(std::move(condition)),
          TrueExpression(std::move(trueExpr)), FalseExpression(std::move(falseExpr)) {}
};

/// <summary>
/// Assignment expression (a = b, a += b, etc.)
/// </summary>
enum class AssignmentOperator {
    Assign,             // =
    AddAssign,          // +=
    SubtractAssign,     // -=
    MultiplyAssign,     // *=
    DivideAssign,       // /=
    ModuloAssign,       // %=
    BitwiseAndAssign,   // &=
    BitwiseOrAssign,    // |=
    BitwiseXorAssign,   // ^=
    LeftShiftAssign,    // <<=
    RightShiftAssign,   // >>=
    LogicalAndAssign,   // &&=
    LogicalOrAssign,    // ||=
    ModuloModuloAssign, // %%= (DM-style modulo for negative numbers)
    AssignInto          // :=
};

class DMASTAssign : public DMASTExpression {
public:
    std::unique_ptr<DMASTExpression> LValue;
    AssignmentOperator Operator;
    std::unique_ptr<DMASTExpression> Value;
    
    DMASTAssign(const Location& location,
               std::unique_ptr<DMASTExpression> lvalue,
               AssignmentOperator op,
               std::unique_ptr<DMASTExpression> value)
        : DMASTExpression(location), LValue(std::move(lvalue)), Operator(op), Value(std::move(value)) {}
};

/// <summary>
/// Switch case range expression (1 to 10)
/// Used in switch statements to match a range of values
/// </summary>
class DMASTSwitchCaseRange : public DMASTExpression {
public:
    std::unique_ptr<DMASTExpression> RangeStart;
    std::unique_ptr<DMASTExpression> RangeEnd;
    
    DMASTSwitchCaseRange(const Location& location,
                        std::unique_ptr<DMASTExpression> rangeStart,
                        std::unique_ptr<DMASTExpression> rangeEnd)
        : DMASTExpression(location), RangeStart(std::move(rangeStart)), RangeEnd(std::move(rangeEnd)) {}
    
    std::vector<DMASTExpression*> GetLeaves() override {
        return {RangeStart.get(), RangeEnd.get()};
    }
};

} // namespace DMCompiler
