#include <iostream>
#include <cassert>
#include <memory>
#include "../include/DMParser.h"
#include "../include/DMLexer.h"
#include "../include/DMCompiler.h"
#include "../include/DMASTExpression.h"
#include "../include/DMASTStatement.h"

// Helper function to parse an expression from a string
std::unique_ptr<DMCompiler::DMASTExpression> ParseExpression(const std::string& code) {
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMLexer lexer("test.dm", code);
    DMCompiler::DMParser parser(&compiler, &lexer);
    return parser.Expression();
}

// Helper function to parse an object statement from a string
std::unique_ptr<DMCompiler::DMASTObjectStatement> ParseObjectStatement(const std::string& code) {
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMLexer lexer("test.dm", code);
    DMCompiler::DMParser parser(&compiler, &lexer);
    return parser.ObjectStatement();
}

// Helper function to parse a complete file from a string
std::unique_ptr<DMCompiler::DMASTFile> ParseFile(const std::string& code) {
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMLexer lexer("test.dm", code);
    DMCompiler::DMParser parser(&compiler, &lexer);
    return parser.ParseFile();
}

// Test basic integer literal
bool TestIntegerLiteral() {
    std::cout << "  Testing integer literal..." << std::endl;
    auto expr = ParseExpression("42");
    
    // Check it's a constant integer
    auto* constInt = dynamic_cast<DMCompiler::DMASTConstantInteger*>(expr.get());
    if (!constInt) {
        std::cerr << "    FAILED: Not a constant integer" << std::endl;
        return false;
    }
    
    if (constInt->Value != 42) {
        std::cerr << "    FAILED: Expected 42, got " << constInt->Value << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test basic addition
bool TestAddition() {
    std::cout << "  Testing addition (2 + 3)..." << std::endl;
    auto expr = ParseExpression("2 + 3");
    
    // Check it's a binary expression
    auto* binary = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(expr.get());
    if (!binary) {
        std::cerr << "    FAILED: Not a binary expression" << std::endl;
        return false;
    }
    
    if (binary->Operator != DMCompiler::BinaryOperator::Add) {
        std::cerr << "    FAILED: Not an addition operator" << std::endl;
        return false;
    }
    
    // Check left operand is 2
    auto* left = dynamic_cast<DMCompiler::DMASTConstantInteger*>(binary->Left.get());
    if (!left || left->Value != 2) {
        std::cerr << "    FAILED: Left operand is not 2" << std::endl;
        return false;
    }
    
    // Check right operand is 3
    auto* right = dynamic_cast<DMCompiler::DMASTConstantInteger*>(binary->Right.get());
    if (!right || right->Value != 3) {
        std::cerr << "    FAILED: Right operand is not 3" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test operator precedence (2 + 3 * 4 should be 2 + (3 * 4))
bool TestOperatorPrecedence() {
    std::cout << "  Testing operator precedence (2 + 3 * 4)..." << std::endl;
    auto expr = ParseExpression("2 + 3 * 4");
    
    // Root should be addition
    auto* add = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(expr.get());
    if (!add || add->Operator != DMCompiler::BinaryOperator::Add) {
        std::cerr << "    FAILED: Root is not addition" << std::endl;
        return false;
    }
    
    // Left should be 2
    auto* left = dynamic_cast<DMCompiler::DMASTConstantInteger*>(add->Left.get());
    if (!left || left->Value != 2) {
        std::cerr << "    FAILED: Left is not 2" << std::endl;
        return false;
    }
    
    // Right should be multiplication (3 * 4)
    auto* mul = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(add->Right.get());
    if (!mul || mul->Operator != DMCompiler::BinaryOperator::Multiply) {
        std::cerr << "    FAILED: Right is not multiplication" << std::endl;
        return false;
    }
    
    // Check 3 and 4
    auto* three = dynamic_cast<DMCompiler::DMASTConstantInteger*>(mul->Left.get());
    auto* four = dynamic_cast<DMCompiler::DMASTConstantInteger*>(mul->Right.get());
    if (!three || three->Value != 3 || !four || four->Value != 4) {
        std::cerr << "    FAILED: Multiplication operands incorrect" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test unary negation
bool TestUnaryNegation() {
    std::cout << "  Testing unary negation (-5)..." << std::endl;
    auto expr = ParseExpression("-5");
    
    // Should be unary expression
    auto* unary = dynamic_cast<DMCompiler::DMASTExpressionUnary*>(expr.get());
    if (!unary || unary->Operator != DMCompiler::UnaryOperator::Negate) {
        std::cerr << "    FAILED: Not a unary negation" << std::endl;
        return false;
    }
    
    // Operand should be 5
    auto* five = dynamic_cast<DMCompiler::DMASTConstantInteger*>(unary->Expression.get());
    if (!five || five->Value != 5) {
        std::cerr << "    FAILED: Operand is not 5" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test parenthesized expression ((2 + 3) * 4)
bool TestParentheses() {
    std::cout << "  Testing parentheses ((2 + 3) * 4)..." << std::endl;
    auto expr = ParseExpression("(2 + 3) * 4");
    
    // Root should be multiplication
    auto* mul = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(expr.get());
    if (!mul || mul->Operator != DMCompiler::BinaryOperator::Multiply) {
        std::cerr << "    FAILED: Root is not multiplication" << std::endl;
        return false;
    }
    
    // Left should be addition (2 + 3)
    auto* add = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(mul->Left.get());
    if (!add || add->Operator != DMCompiler::BinaryOperator::Add) {
        std::cerr << "    FAILED: Left is not addition" << std::endl;
        return false;
    }
    
    // Right should be 4
    auto* four = dynamic_cast<DMCompiler::DMASTConstantInteger*>(mul->Right.get());
    if (!four || four->Value != 4) {
        std::cerr << "    FAILED: Right is not 4" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test comparison operators (5 > 3)
bool TestComparison() {
    std::cout << "  Testing comparison (5 > 3)..." << std::endl;
    auto expr = ParseExpression("5 > 3");
    
    // Should be binary expression with Greater operator
    auto* binary = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(expr.get());
    if (!binary || binary->Operator != DMCompiler::BinaryOperator::Greater) {
        std::cerr << "    FAILED: Not a greater-than comparison" << std::endl;
        return false;
    }
    
    // Left should be 5
    auto* left = dynamic_cast<DMCompiler::DMASTConstantInteger*>(binary->Left.get());
    if (!left || left->Value != 5) {
        std::cerr << "    FAILED: Left operand is not 5" << std::endl;
        return false;
    }
    
    // Right should be 3
    auto* right = dynamic_cast<DMCompiler::DMASTConstantInteger*>(binary->Right.get());
    if (!right || right->Value != 3) {
        std::cerr << "    FAILED: Right operand is not 3" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test equality comparison (10 == 10)
bool TestEquality() {
    std::cout << "  Testing equality (10 == 10)..." << std::endl;
    auto expr = ParseExpression("10 == 10");
    
    // Should be binary expression with Equal operator
    auto* binary = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(expr.get());
    if (!binary || binary->Operator != DMCompiler::BinaryOperator::Equal) {
        std::cerr << "    FAILED: Not an equality comparison" << std::endl;
        return false;
    }
    
    // Both operands should be 10
    auto* left = dynamic_cast<DMCompiler::DMASTConstantInteger*>(binary->Left.get());
    auto* right = dynamic_cast<DMCompiler::DMASTConstantInteger*>(binary->Right.get());
    if (!left || left->Value != 10 || !right || right->Value != 10) {
        std::cerr << "    FAILED: Operands are not both 10" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test comparison with arithmetic (x + 5 > 10)
bool TestComparisonWithArithmetic() {
    std::cout << "  Testing comparison with arithmetic (2 + 3 > 4)..." << std::endl;
    auto expr = ParseExpression("2 + 3 > 4");
    
    // Root should be Greater comparison
    auto* cmp = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(expr.get());
    if (!cmp || cmp->Operator != DMCompiler::BinaryOperator::Greater) {
        std::cerr << "    FAILED: Root is not Greater comparison" << std::endl;
        return false;
    }
    
    // Left should be addition (2 + 3)
    auto* add = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(cmp->Left.get());
    if (!add || add->Operator != DMCompiler::BinaryOperator::Add) {
        std::cerr << "    FAILED: Left is not addition" << std::endl;
        return false;
    }
    
    // Right should be 4
    auto* four = dynamic_cast<DMCompiler::DMASTConstantInteger*>(cmp->Right.get());
    if (!four || four->Value != 4) {
        std::cerr << "    FAILED: Right is not 4" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test logical AND (a && b)
bool TestLogicalAnd() {
    std::cout << "  Testing logical AND (a && b)..." << std::endl;
    auto expr = ParseExpression("a && b");
    
    // Should be binary expression with LogicalAnd operator
    auto* binary = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(expr.get());
    if (!binary || binary->Operator != DMCompiler::BinaryOperator::LogicalAnd) {
        std::cerr << "    FAILED: Not a logical AND" << std::endl;
        return false;
    }
    
    // Left should be identifier 'a'
    auto* left = dynamic_cast<DMCompiler::DMASTIdentifier*>(binary->Left.get());
    if (!left || left->Identifier != "a") {
        std::cerr << "    FAILED: Left operand is not 'a'" << std::endl;
        return false;
    }
    
    // Right should be identifier 'b'
    auto* right = dynamic_cast<DMCompiler::DMASTIdentifier*>(binary->Right.get());
    if (!right || right->Identifier != "b") {
        std::cerr << "    FAILED: Right operand is not 'b'" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test logical OR (x || y)
bool TestLogicalOr() {
    std::cout << "  Testing logical OR (x || y)..." << std::endl;
    auto expr = ParseExpression("x || y");
    
    // Should be binary expression with LogicalOr operator
    auto* binary = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(expr.get());
    if (!binary || binary->Operator != DMCompiler::BinaryOperator::LogicalOr) {
        std::cerr << "    FAILED: Not a logical OR" << std::endl;
        return false;
    }
    
    // Left should be identifier 'x'
    auto* left = dynamic_cast<DMCompiler::DMASTIdentifier*>(binary->Left.get());
    if (!left || left->Identifier != "x") {
        std::cerr << "    FAILED: Left operand is not 'x'" << std::endl;
        return false;
    }
    
    // Right should be identifier 'y'
    auto* right = dynamic_cast<DMCompiler::DMASTIdentifier*>(binary->Right.get());
    if (!right || right->Identifier != "y") {
        std::cerr << "    FAILED: Right operand is not 'y'" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test logical operator precedence (a || b && c should be a || (b && c))
bool TestLogicalPrecedence() {
    std::cout << "  Testing logical precedence (a || b && c)..." << std::endl;
    auto expr = ParseExpression("a || b && c");
    
    // Root should be OR
    auto* orOp = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(expr.get());
    if (!orOp || orOp->Operator != DMCompiler::BinaryOperator::LogicalOr) {
        std::cerr << "    FAILED: Root is not LogicalOr" << std::endl;
        return false;
    }
    
    // Left should be identifier 'a'
    auto* left = dynamic_cast<DMCompiler::DMASTIdentifier*>(orOp->Left.get());
    if (!left || left->Identifier != "a") {
        std::cerr << "    FAILED: Left is not 'a'" << std::endl;
        return false;
    }
    
    // Right should be AND (b && c)
    auto* andOp = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(orOp->Right.get());
    if (!andOp || andOp->Operator != DMCompiler::BinaryOperator::LogicalAnd) {
        std::cerr << "    FAILED: Right is not LogicalAnd" << std::endl;
        return false;
    }
    
    // Check b and c
    auto* b = dynamic_cast<DMCompiler::DMASTIdentifier*>(andOp->Left.get());
    auto* c = dynamic_cast<DMCompiler::DMASTIdentifier*>(andOp->Right.get());
    if (!b || b->Identifier != "b" || !c || c->Identifier != "c") {
        std::cerr << "    FAILED: AND operands incorrect" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test combined comparison and logical (x > 5 && y < 10)
bool TestComparisonWithLogical() {
    std::cout << "  Testing comparison with logical (x > 5 && y < 10)..." << std::endl;
    auto expr = ParseExpression("x > 5 && y < 10");
    
    // Root should be AND
    auto* andOp = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(expr.get());
    if (!andOp || andOp->Operator != DMCompiler::BinaryOperator::LogicalAnd) {
        std::cerr << "    FAILED: Root is not LogicalAnd" << std::endl;
        return false;
    }
    
    // Left should be Greater comparison (x > 5)
    auto* leftCmp = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(andOp->Left.get());
    if (!leftCmp || leftCmp->Operator != DMCompiler::BinaryOperator::Greater) {
        std::cerr << "    FAILED: Left is not Greater comparison" << std::endl;
        return false;
    }
    
    // Right should be Less comparison (y < 10)
    auto* rightCmp = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(andOp->Right.get());
    if (!rightCmp || rightCmp->Operator != DMCompiler::BinaryOperator::Less) {
        std::cerr << "    FAILED: Right is not Less comparison" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test bitwise AND (flags & mask)
bool TestBitwiseAnd() {
    std::cout << "  Testing bitwise AND (flags & mask)..." << std::endl;
    auto expr = ParseExpression("flags & mask");
    
    // Should be binary expression with BitwiseAnd operator
    auto* binary = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(expr.get());
    if (!binary || binary->Operator != DMCompiler::BinaryOperator::BitwiseAnd) {
        std::cerr << "    FAILED: Not a bitwise AND" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test bitwise OR (a | b)
bool TestBitwiseOr() {
    std::cout << "  Testing bitwise OR (a | b)..." << std::endl;
    auto expr = ParseExpression("a | b");
    
    // Should be binary expression with BitwiseOr operator
    auto* binary = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(expr.get());
    if (!binary || binary->Operator != DMCompiler::BinaryOperator::BitwiseOr) {
        std::cerr << "    FAILED: Not a bitwise OR" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test bitwise XOR (x ^ y)
bool TestBitwiseXor() {
    std::cout << "  Testing bitwise XOR (x ^ y)..." << std::endl;
    auto expr = ParseExpression("x ^ y");
    
    // Should be binary expression with BitwiseXor operator
    auto* binary = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(expr.get());
    if (!binary || binary->Operator != DMCompiler::BinaryOperator::BitwiseXor) {
        std::cerr << "    FAILED: Not a bitwise XOR" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test left shift (value << 2)
bool TestLeftShift() {
    std::cout << "  Testing left shift (value << 2)..." << std::endl;
    auto expr = ParseExpression("value << 2");
    
    // Should be binary expression with LeftShift operator
    auto* binary = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(expr.get());
    if (!binary || binary->Operator != DMCompiler::BinaryOperator::LeftShift) {
        std::cerr << "    FAILED: Not a left shift" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test right shift (bits >> 4)
bool TestRightShift() {
    std::cout << "  Testing right shift (bits >> 4)..." << std::endl;
    auto expr = ParseExpression("bits >> 4");
    
    // Should be binary expression with RightShift operator
    auto* binary = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(expr.get());
    if (!binary || binary->Operator != DMCompiler::BinaryOperator::RightShift) {
        std::cerr << "    FAILED: Not a right shift" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test bitwise precedence (a | b & c should be a | (b & c))
bool TestBitwisePrecedence() {
    std::cout << "  Testing bitwise precedence (a | b & c)..." << std::endl;
    auto expr = ParseExpression("a | b & c");
    
    // Root should be OR
    auto* orOp = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(expr.get());
    if (!orOp || orOp->Operator != DMCompiler::BinaryOperator::BitwiseOr) {
        std::cerr << "    FAILED: Root is not BitwiseOr" << std::endl;
        return false;
    }
    
    // Right should be AND (b & c)
    auto* andOp = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(orOp->Right.get());
    if (!andOp || andOp->Operator != DMCompiler::BinaryOperator::BitwiseAnd) {
        std::cerr << "    FAILED: Right is not BitwiseAnd" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test simple assignment (x = 5)
bool TestSimpleAssignment() {
    std::cout << "  Testing simple assignment (x = 5)..." << std::endl;
    auto expr = ParseExpression("x = 5");
    
    // Should be an assignment expression
    auto* assign = dynamic_cast<DMCompiler::DMASTAssign*>(expr.get());
    if (!assign || assign->Operator != DMCompiler::AssignmentOperator::Assign) {
        std::cerr << "    FAILED: Not a simple assignment" << std::endl;
        return false;
    }
    
    // LValue should be identifier 'x'
    auto* lvalue = dynamic_cast<DMCompiler::DMASTIdentifier*>(assign->LValue.get());
    if (!lvalue || lvalue->Identifier != "x") {
        std::cerr << "    FAILED: LValue is not 'x'" << std::endl;
        return false;
    }
    
    // Value should be 5
    auto* value = dynamic_cast<DMCompiler::DMASTConstantInteger*>(assign->Value.get());
    if (!value || value->Value != 5) {
        std::cerr << "    FAILED: Value is not 5" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test compound assignment (count += 1)
bool TestCompoundAssignment() {
    std::cout << "  Testing compound assignment (count += 1)..." << std::endl;
    auto expr = ParseExpression("count += 1");
    
    // Should be an assignment expression with AddAssign
    auto* assign = dynamic_cast<DMCompiler::DMASTAssign*>(expr.get());
    if (!assign || assign->Operator != DMCompiler::AssignmentOperator::AddAssign) {
        std::cerr << "    FAILED: Not an AddAssign" << std::endl;
        return false;
    }
    
    // LValue should be identifier 'count'
    auto* lvalue = dynamic_cast<DMCompiler::DMASTIdentifier*>(assign->LValue.get());
    if (!lvalue || lvalue->Identifier != "count") {
        std::cerr << "    FAILED: LValue is not 'count'" << std::endl;
        return false;
    }
    
    // Value should be 1
    auto* value = dynamic_cast<DMCompiler::DMASTConstantInteger*>(assign->Value.get());
    if (!value || value->Value != 1) {
        std::cerr << "    FAILED: Value is not 1" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test chained assignment (a = b = c)
bool TestChainedAssignment() {
    std::cout << "  Testing chained assignment (a = b = c)..." << std::endl;
    auto expr = ParseExpression("a = b = c");
    
    // Root should be assignment (a = ...)
    auto* outer = dynamic_cast<DMCompiler::DMASTAssign*>(expr.get());
    if (!outer || outer->Operator != DMCompiler::AssignmentOperator::Assign) {
        std::cerr << "    FAILED: Root is not assignment" << std::endl;
        return false;
    }
    
    // LValue should be 'a'
    auto* a = dynamic_cast<DMCompiler::DMASTIdentifier*>(outer->LValue.get());
    if (!a || a->Identifier != "a") {
        std::cerr << "    FAILED: LValue is not 'a'" << std::endl;
        return false;
    }
    
    // Value should be another assignment (b = c)
    auto* inner = dynamic_cast<DMCompiler::DMASTAssign*>(outer->Value.get());
    if (!inner || inner->Operator != DMCompiler::AssignmentOperator::Assign) {
        std::cerr << "    FAILED: Value is not an assignment" << std::endl;
        return false;
    }
    
    // Inner LValue should be 'b'
    auto* b = dynamic_cast<DMCompiler::DMASTIdentifier*>(inner->LValue.get());
    if (!b || b->Identifier != "b") {
        std::cerr << "    FAILED: Inner LValue is not 'b'" << std::endl;
        return false;
    }
    
    // Inner Value should be 'c'
    auto* c = dynamic_cast<DMCompiler::DMASTIdentifier*>(inner->Value.get());
    if (!c || c->Identifier != "c") {
        std::cerr << "    FAILED: Inner Value is not 'c'" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test assignment with expression (x = y + 5)
bool TestAssignmentWithExpression() {
    std::cout << "  Testing assignment with expression (x = y + 5)..." << std::endl;
    auto expr = ParseExpression("x = y + 5");
    
    // Should be assignment
    auto* assign = dynamic_cast<DMCompiler::DMASTAssign*>(expr.get());
    if (!assign) {
        std::cerr << "    FAILED: Not an assignment" << std::endl;
        return false;
    }
    
    // LValue should be 'x'
    auto* x = dynamic_cast<DMCompiler::DMASTIdentifier*>(assign->LValue.get());
    if (!x || x->Identifier != "x") {
        std::cerr << "    FAILED: LValue is not 'x'" << std::endl;
        return false;
    }
    
    // Value should be addition (y + 5)
    auto* add = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(assign->Value.get());
    if (!add || add->Operator != DMCompiler::BinaryOperator::Add) {
        std::cerr << "    FAILED: Value is not addition" << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

// Test: condition ? true_val : false_val
bool TestSimpleTernary() {
    std::cout << "  TestSimpleTernary... ";
    
    auto expr = ParseExpression("x > 5 ? 10 : 20");
    if (!expr) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a ternary
    auto* ternary = dynamic_cast<DMCompiler::DMASTTernary*>(expr.get());
    if (!ternary) {
        std::cerr << "FAILED: Not a ternary" << std::endl;
        return false;
    }
    
    // Condition should be comparison (x > 5)
    auto* comparison = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(ternary->Condition.get());
    if (!comparison || comparison->Operator != DMCompiler::BinaryOperator::Greater) {
        std::cerr << "FAILED: Condition is not >" << std::endl;
        return false;
    }
    
    // True branch should be 10
    auto* trueVal = dynamic_cast<DMCompiler::DMASTConstantInteger*>(ternary->TrueExpression.get());
    if (!trueVal || trueVal->Value != 10) {
        std::cerr << "FAILED: True branch is not 10" << std::endl;
        return false;
    }
    
    // False branch should be 20
    auto* falseVal = dynamic_cast<DMCompiler::DMASTConstantInteger*>(ternary->FalseExpression.get());
    if (!falseVal || falseVal->Value != 20) {
        std::cerr << "FAILED: False branch is not 20" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: a ? b : c ? d : e (right-associativity)
bool TestNestedTernary() {
    std::cout << "  TestNestedTernary... ";
    
    auto expr = ParseExpression("a ? b : c ? d : e");
    if (!expr) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a ternary: a ? b : (c ? d : e)
    auto* outer = dynamic_cast<DMCompiler::DMASTTernary*>(expr.get());
    if (!outer) {
        std::cerr << "FAILED: Not a ternary" << std::endl;
        return false;
    }
    
    // Condition should be 'a'
    auto* a = dynamic_cast<DMCompiler::DMASTIdentifier*>(outer->Condition.get());
    if (!a || a->Identifier != "a") {
        std::cerr << "FAILED: Condition is not 'a'" << std::endl;
        return false;
    }
    
    // True branch should be 'b'
    auto* b = dynamic_cast<DMCompiler::DMASTIdentifier*>(outer->TrueExpression.get());
    if (!b || b->Identifier != "b") {
        std::cerr << "FAILED: True branch is not 'b'" << std::endl;
        return false;
    }
    
    // False branch should be another ternary: c ? d : e
    auto* inner = dynamic_cast<DMCompiler::DMASTTernary*>(outer->FalseExpression.get());
    if (!inner) {
        std::cerr << "FAILED: False branch is not a ternary" << std::endl;
        return false;
    }
    
    // Inner condition should be 'c'
    auto* c = dynamic_cast<DMCompiler::DMASTIdentifier*>(inner->Condition.get());
    if (!c || c->Identifier != "c") {
        std::cerr << "FAILED: Inner condition is not 'c'" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: (x + 5) > 10 ? y * 2 : z / 3
bool TestTernaryWithExpressions() {
    std::cout << "  TestTernaryWithExpressions... ";
    
    auto expr = ParseExpression("(x + 5) > 10 ? y * 2 : z / 3");
    if (!expr) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a ternary
    auto* ternary = dynamic_cast<DMCompiler::DMASTTernary*>(expr.get());
    if (!ternary) {
        std::cerr << "FAILED: Not a ternary" << std::endl;
        return false;
    }
    
    // Condition should be comparison
    auto* comparison = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(ternary->Condition.get());
    if (!comparison || comparison->Operator != DMCompiler::BinaryOperator::Greater) {
        std::cerr << "FAILED: Condition is not comparison" << std::endl;
        return false;
    }
    
    // True branch should be multiplication (y * 2)
    auto* mult = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(ternary->TrueExpression.get());
    if (!mult || mult->Operator != DMCompiler::BinaryOperator::Multiply) {
        std::cerr << "FAILED: True branch is not multiplication" << std::endl;
        return false;
    }
    
    // False branch should be division (z / 3)
    auto* div = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(ternary->FalseExpression.get());
    if (!div || div->Operator != DMCompiler::BinaryOperator::Divide) {
        std::cerr << "FAILED: False branch is not division" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: result = condition ? value1 : value2
bool TestTernaryWithAssignment() {
    std::cout << "  TestTernaryWithAssignment... ";
    
    auto expr = ParseExpression("result = condition ? value1 : value2");
    if (!expr) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be an assignment
    auto* assign = dynamic_cast<DMCompiler::DMASTAssign*>(expr.get());
    if (!assign) {
        std::cerr << "FAILED: Not an assignment" << std::endl;
        return false;
    }
    
    // LValue should be 'result'
    auto* result = dynamic_cast<DMCompiler::DMASTIdentifier*>(assign->LValue.get());
    if (!result || result->Identifier != "result") {
        std::cerr << "FAILED: LValue is not 'result'" << std::endl;
        return false;
    }
    
    // Value should be ternary
    auto* ternary = dynamic_cast<DMCompiler::DMASTTernary*>(assign->Value.get());
    if (!ternary) {
        std::cerr << "FAILED: Value is not a ternary" << std::endl;
        return false;
    }
    
    // Condition should be 'condition'
    auto* cond = dynamic_cast<DMCompiler::DMASTIdentifier*>(ternary->Condition.get());
    if (!cond || cond->Identifier != "condition") {
        std::cerr << "FAILED: Condition is not 'condition'" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: obj.property
bool TestMemberAccess() {
    std::cout << "  TestMemberAccess... ";
    
    auto expr = ParseExpression("player.health");
    if (!expr) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a dereference
    auto* deref = dynamic_cast<DMCompiler::DMASTDereference*>(expr.get());
    if (!deref) {
        std::cerr << "FAILED: Not a dereference" << std::endl;
        return false;
    }
    
    // Type should be Direct (.)
    if (deref->Type != DMCompiler::DereferenceType::Direct) {
        std::cerr << "FAILED: Not Direct dereference" << std::endl;
        return false;
    }
    
    // Expression should be 'player'
    auto* player = dynamic_cast<DMCompiler::DMASTIdentifier*>(deref->Expression.get());
    if (!player || player->Identifier != "player") {
        std::cerr << "FAILED: Expression is not 'player'" << std::endl;
        return false;
    }
    
    // Property should be 'health'
    auto* health = dynamic_cast<DMCompiler::DMASTIdentifier*>(deref->Property.get());
    if (!health || health->Identifier != "health") {
        std::cerr << "FAILED: Property is not 'health'" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: func(arg1, arg2)
bool TestFunctionCall() {
    std::cout << "  TestFunctionCall... ";
    
    auto expr = ParseExpression("calculate(x, y + 5)");
    if (!expr) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a call
    auto* call = dynamic_cast<DMCompiler::DMASTCall*>(expr.get());
    if (!call) {
        std::cerr << "FAILED: Not a call" << std::endl;
        return false;
    }
    
    // Target should be 'calculate'
    auto* func = dynamic_cast<DMCompiler::DMASTIdentifier*>(call->Target.get());
    if (!func || func->Identifier != "calculate") {
        std::cerr << "FAILED: Target is not 'calculate'" << std::endl;
        return false;
    }
    
    // Should have 2 parameters
    if (call->Parameters.size() != 2) {
        std::cerr << "FAILED: Expected 2 parameters, got " << call->Parameters.size() << std::endl;
        return false;
    }
    
    // First parameter should be 'x'
    auto* param1 = dynamic_cast<DMCompiler::DMASTIdentifier*>(call->Parameters[0]->Value.get());
    if (!param1 || param1->Identifier != "x") {
        std::cerr << "FAILED: First parameter is not 'x'" << std::endl;
        return false;
    }
    
    // Second parameter should be addition (y + 5)
    auto* param2 = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(call->Parameters[1]->Value.get());
    if (!param2 || param2->Operator != DMCompiler::BinaryOperator::Add) {
        std::cerr << "FAILED: Second parameter is not addition" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: func(name = value) - named parameters
bool TestNamedParameters() {
    std::cout << "  TestNamedParameters... ";
    
    auto expr = ParseExpression("do_thing(target = player, amount = 5)");
    if (!expr) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a call
    auto* call = dynamic_cast<DMCompiler::DMASTCall*>(expr.get());
    if (!call) {
        std::cerr << "FAILED: Not a call" << std::endl;
        return false;
    }
    
    // Target should be 'do_thing'
    auto* func = dynamic_cast<DMCompiler::DMASTIdentifier*>(call->Target.get());
    if (!func || func->Identifier != "do_thing") {
        std::cerr << "FAILED: Target is not 'do_thing'" << std::endl;
        return false;
    }
    
    // Should have 2 parameters
    if (call->Parameters.size() != 2) {
        std::cerr << "FAILED: Expected 2 parameters, got " << call->Parameters.size() << std::endl;
        return false;
    }
    
    // First parameter should be named 'target' with value 'player'
    if (!call->Parameters[0]->Key) {
        std::cerr << "FAILED: First parameter has no key (not a named parameter)" << std::endl;
        return false;
    }
    auto* key1 = dynamic_cast<DMCompiler::DMASTIdentifier*>(call->Parameters[0]->Key.get());
    if (!key1 || key1->Identifier != "target") {
        std::cerr << "FAILED: First parameter key is not 'target'" << std::endl;
        return false;
    }
    auto* value1 = dynamic_cast<DMCompiler::DMASTIdentifier*>(call->Parameters[0]->Value.get());
    if (!value1 || value1->Identifier != "player") {
        std::cerr << "FAILED: First parameter value is not 'player'" << std::endl;
        return false;
    }
    
    // Second parameter should be named 'amount' with value 5
    if (!call->Parameters[1]->Key) {
        std::cerr << "FAILED: Second parameter has no key (not a named parameter)" << std::endl;
        return false;
    }
    auto* key2 = dynamic_cast<DMCompiler::DMASTIdentifier*>(call->Parameters[1]->Key.get());
    if (!key2 || key2->Identifier != "amount") {
        std::cerr << "FAILED: Second parameter key is not 'amount'" << std::endl;
        return false;
    }
    auto* value2 = dynamic_cast<DMCompiler::DMASTConstantInteger*>(call->Parameters[1]->Value.get());
    if (!value2 || value2->Value != 5) {
        std::cerr << "FAILED: Second parameter value is not 5" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: mixed positional and named parameters
bool TestMixedParameters() {
    std::cout << "  TestMixedParameters... ";
    
    auto expr = ParseExpression("create(item, count = 3)");
    if (!expr) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a call
    auto* call = dynamic_cast<DMCompiler::DMASTCall*>(expr.get());
    if (!call) {
        std::cerr << "FAILED: Not a call" << std::endl;
        return false;
    }
    
    // Should have 2 parameters
    if (call->Parameters.size() != 2) {
        std::cerr << "FAILED: Expected 2 parameters, got " << call->Parameters.size() << std::endl;
        return false;
    }
    
    // First parameter should be positional (no key)
    if (call->Parameters[0]->Key) {
        std::cerr << "FAILED: First parameter should be positional (has unexpected key)" << std::endl;
        return false;
    }
    auto* value1 = dynamic_cast<DMCompiler::DMASTIdentifier*>(call->Parameters[0]->Value.get());
    if (!value1 || value1->Identifier != "item") {
        std::cerr << "FAILED: First parameter value is not 'item'" << std::endl;
        return false;
    }
    
    // Second parameter should be named 'count' with value 3
    if (!call->Parameters[1]->Key) {
        std::cerr << "FAILED: Second parameter has no key (not a named parameter)" << std::endl;
        return false;
    }
    auto* key2 = dynamic_cast<DMCompiler::DMASTIdentifier*>(call->Parameters[1]->Key.get());
    if (!key2 || key2->Identifier != "count") {
        std::cerr << "FAILED: Second parameter key is not 'count'" << std::endl;
        return false;
    }
    auto* value2 = dynamic_cast<DMCompiler::DMASTConstantInteger*>(call->Parameters[1]->Value.get());
    if (!value2 || value2->Value != 3) {
        std::cerr << "FAILED: Second parameter value is not 3" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: array[index]
bool TestArrayIndexing() {
    std::cout << "  TestArrayIndexing... ";
    
    auto expr = ParseExpression("items[index]");
    if (!expr) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a dereference (array indexing is treated as dereference)
    auto* deref = dynamic_cast<DMCompiler::DMASTDereference*>(expr.get());
    if (!deref) {
        std::cerr << "FAILED: Not a dereference" << std::endl;
        return false;
    }
    
    // Expression should be 'items'
    auto* items = dynamic_cast<DMCompiler::DMASTIdentifier*>(deref->Expression.get());
    if (!items || items->Identifier != "items") {
        std::cerr << "FAILED: Expression is not 'items'" << std::endl;
        return false;
    }
    
    // Property (index) should be 'index'
    auto* index = dynamic_cast<DMCompiler::DMASTIdentifier*>(deref->Property.get());
    if (!index || index->Identifier != "index") {
        std::cerr << "FAILED: Index is not 'index'" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: obj.method(arg)[0].property
bool TestChainedPostfix() {
    std::cout << "  TestChainedPostfix... ";
    
    auto expr = ParseExpression("obj.method(arg)[0].property");
    if (!expr) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Outermost should be dereference (.property)
    auto* deref3 = dynamic_cast<DMCompiler::DMASTDereference*>(expr.get());
    if (!deref3) {
        std::cerr << "FAILED: Outermost is not a dereference" << std::endl;
        return false;
    }
    
    // Property should be 'property'
    auto* prop = dynamic_cast<DMCompiler::DMASTIdentifier*>(deref3->Property.get());
    if (!prop || prop->Identifier != "property") {
        std::cerr << "FAILED: Property is not 'property'" << std::endl;
        return false;
    }
    
    // Next level should be dereference ([0])
    auto* deref2 = dynamic_cast<DMCompiler::DMASTDereference*>(deref3->Expression.get());
    if (!deref2) {
        std::cerr << "FAILED: Second level is not a dereference" << std::endl;
        return false;
    }
    
    // Index should be 0
    auto* zero = dynamic_cast<DMCompiler::DMASTConstantInteger*>(deref2->Property.get());
    if (!zero || zero->Value != 0) {
        std::cerr << "FAILED: Index is not 0" << std::endl;
        return false;
    }
    
    // Next level should be call (method(arg))
    auto* call = dynamic_cast<DMCompiler::DMASTCall*>(deref2->Expression.get());
    if (!call) {
        std::cerr << "FAILED: Third level is not a call" << std::endl;
        return false;
    }
    
    // Call target should be dereference (obj.method)
    auto* deref1 = dynamic_cast<DMCompiler::DMASTDereference*>(call->Target.get());
    if (!deref1) {
        std::cerr << "FAILED: Call target is not a dereference" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: /mob/player (absolute path)
bool TestAbsolutePath() {
    std::cout << "  TestAbsolutePath... ";
    
    auto expr = ParseExpression("/mob/player");
    if (!expr) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a constant path
    auto* path = dynamic_cast<DMCompiler::DMASTConstantPath*>(expr.get());
    if (!path) {
        std::cerr << "FAILED: Not a constant path" << std::endl;
        return false;
    }
    
    // Check path string
    std::string pathStr = path->Path.Path.ToString();
    if (pathStr != "/mob/player") {
        std::cerr << "FAILED: Path is '" << pathStr << "', expected '/mob/player'" << std::endl;
        return false;
    }
    
    // Check path type
    if (path->Path.Path.GetPathType() != DMCompiler::DreamPath::PathType::Absolute) {
        std::cerr << "FAILED: Path type is not Absolute" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: ../parent (relative path)
bool TestRelativePath() {
    std::cout << "  TestRelativePath... ";
    
    auto expr = ParseExpression("../parent");
    if (!expr) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a constant path
    auto* path = dynamic_cast<DMCompiler::DMASTConstantPath*>(expr.get());
    if (!path) {
        std::cerr << "FAILED: Not a constant path" << std::endl;
        return false;
    }
    
    // Check path type
    if (path->Path.Path.GetPathType() != DMCompiler::DreamPath::PathType::UpwardSearch) {
        std::cerr << "FAILED: Path type is not UpwardSearch" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: new /mob/player
bool TestNewExpression() {
    std::cout << "  TestNewExpression... ";
    
    auto expr = ParseExpression("new /mob/player");
    if (!expr) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a new expression
    auto* newExpr = dynamic_cast<DMCompiler::DMASTNewPath*>(expr.get());
    if (!newExpr) {
        std::cerr << "FAILED: Not a new expression" << std::endl;
        return false;
    }
    
    // Check path
    if (!newExpr->Path) {
        std::cerr << "FAILED: New expression has no path" << std::endl;
        return false;
    }
    
    auto* constPath = dynamic_cast<DMCompiler::DMASTConstantPath*>(newExpr->Path.get());
    if (!constPath) {
        std::cerr << "FAILED: Path is not a constant path" << std::endl;
        return false;
    }
    std::string pathStr = constPath->Path.Path.ToString();
    if (pathStr != "/mob/player") {
        std::cerr << "FAILED: Path is '" << pathStr << "', expected '/mob/player'" << std::endl;
        return false;
    }
    
    // Should have no parameters
    if (!newExpr->Parameters.empty()) {
        std::cerr << "FAILED: Expected no parameters" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: new /obj/item(x, y)
bool TestNewWithArguments() {
    std::cout << "  TestNewWithArguments... ";
    
    auto expr = ParseExpression("new /obj/item(x, y)");
    if (!expr) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a new expression
    auto* newExpr = dynamic_cast<DMCompiler::DMASTNewPath*>(expr.get());
    if (!newExpr) {
        std::cerr << "FAILED: Not a new expression" << std::endl;
        return false;
    }
    
    // Should have 2 parameters
    if (newExpr->Parameters.size() != 2) {
        std::cerr << "FAILED: Expected 2 parameters, got " << newExpr->Parameters.size() << std::endl;
        return false;
    }
    
    // First parameter should be 'x'
    auto* param1 = dynamic_cast<DMCompiler::DMASTIdentifier*>(newExpr->Parameters[0]->Value.get());
    if (!param1 || param1->Identifier != "x") {
        std::cerr << "FAILED: First parameter is not 'x'" << std::endl;
        return false;
    }
    
    // Second parameter should be 'y'
    auto* param2 = dynamic_cast<DMCompiler::DMASTIdentifier*>(newExpr->Parameters[1]->Value.get());
    if (!param2 || param2->Identifier != "y") {
        std::cerr << "FAILED: Second parameter is not 'y'" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: list(1, 2, 3)
bool TestListExpression() {
    std::cout << "  TestListExpression... ";
    
    auto expr = ParseExpression("list(1, 2, 3)");
    if (!expr) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a list
    auto* list = dynamic_cast<DMCompiler::DMASTList*>(expr.get());
    if (!list) {
        std::cerr << "FAILED: Not a list" << std::endl;
        return false;
    }
    
    // Should have 3 values
    if (list->Values.size() != 3) {
        std::cerr << "FAILED: Expected 3 values, got " << list->Values.size() << std::endl;
        return false;
    }
    
    // First value should be 1
    auto* val1 = dynamic_cast<DMCompiler::DMASTConstantInteger*>(list->Values[0]->Value.get());
    if (!val1 || val1->Value != 1) {
        std::cerr << "FAILED: First value is not 1" << std::endl;
        return false;
    }
    
    // Second value should be 2
    auto* val2 = dynamic_cast<DMCompiler::DMASTConstantInteger*>(list->Values[1]->Value.get());
    if (!val2 || val2->Value != 2) {
        std::cerr << "FAILED: Second value is not 2" << std::endl;
        return false;
    }
    
    // Third value should be 3
    auto* val3 = dynamic_cast<DMCompiler::DMASTConstantInteger*>(list->Values[2]->Value.get());
    if (!val3 || val3->Value != 3) {
        std::cerr << "FAILED: Third value is not 3" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// ============================================================================
// Statement Parsing Tests
// ============================================================================

// Helper function to parse a statement from a string
std::unique_ptr<DMCompiler::DMASTProcStatement> ParseStatement(const std::string& code) {
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMLexer lexer("test.dm", code);
    DMCompiler::DMParser parser(&compiler, &lexer);
    return parser.ProcStatement();
}

// Test: return
bool TestReturnStatement() {
    std::cout << "  TestReturnStatement... ";
    
    auto stmt = ParseStatement("return;");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a return statement
    auto* ret = dynamic_cast<DMCompiler::DMASTProcStatementReturn*>(stmt.get());
    if (!ret) {
        std::cerr << "FAILED: Not a return statement" << std::endl;
        return false;
    }
    
    // Should have no value (void return)
    if (ret->Value) {
        std::cerr << "FAILED: Expected void return, but has value" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: return 42
bool TestReturnWithValue() {
    std::cout << "  TestReturnWithValue... ";
    
    auto stmt = ParseStatement("return 42;");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a return statement
    auto* ret = dynamic_cast<DMCompiler::DMASTProcStatementReturn*>(stmt.get());
    if (!ret) {
        std::cerr << "FAILED: Not a return statement" << std::endl;
        return false;
    }
    
    // Should have a value
    if (!ret->Value) {
        std::cerr << "FAILED: Expected return with value" << std::endl;
        return false;
    }
    
    // Value should be 42
    auto* value = dynamic_cast<DMCompiler::DMASTConstantInteger*>(ret->Value.get());
    if (!value || value->Value != 42) {
        std::cerr << "FAILED: Return value is not 42" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: if (condition) statement
bool TestIfStatement() {
    std::cout << "  TestIfStatement... ";
    
    auto stmt = ParseStatement("if (x > 5) return 1");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be an if statement
    auto* ifStmt = dynamic_cast<DMCompiler::DMASTProcStatementIf*>(stmt.get());
    if (!ifStmt) {
        std::cerr << "FAILED: Not an if statement" << std::endl;
        return false;
    }
    
    // Should have a condition
    if (!ifStmt->Condition) {
        std::cerr << "FAILED: No condition" << std::endl;
        return false;
    }
    
    // Condition should be comparison (x > 5)
    auto* cond = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(ifStmt->Condition.get());
    if (!cond || cond->Operator != DMCompiler::BinaryOperator::Greater) {
        std::cerr << "FAILED: Condition is not comparison" << std::endl;
        return false;
    }
    
    // Should have a body
    if (!ifStmt->Body) {
        std::cerr << "FAILED: No body" << std::endl;
        return false;
    }
    
    // Should have no else body
    if (ifStmt->ElseBody) {
        std::cerr << "FAILED: Should not have else body" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: if (condition) statement else statement
bool TestIfElseStatement() {
    std::cout << "  TestIfElseStatement... ";
    
    auto stmt = ParseStatement("if (x > 5) return 1 else return 0");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be an if statement
    auto* ifStmt = dynamic_cast<DMCompiler::DMASTProcStatementIf*>(stmt.get());
    if (!ifStmt) {
        std::cerr << "FAILED: Not an if statement" << std::endl;
        return false;
    }
    
    // Should have a condition
    if (!ifStmt->Condition) {
        std::cerr << "FAILED: No condition" << std::endl;
        return false;
    }
    
    // Should have a body
    if (!ifStmt->Body) {
        std::cerr << "FAILED: No body" << std::endl;
        return false;
    }
    
    // Should have an else body
    if (!ifStmt->ElseBody) {
        std::cerr << "FAILED: Should have else body" << std::endl;
        return false;
    }
    
    // Else body should have statements
    if (ifStmt->ElseBody->Statements.empty()) {
        std::cerr << "FAILED: Else body has no statements" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: while (condition) statement
bool TestWhileStatement() {
    std::cout << "  TestWhileStatement... ";
    
    auto stmt = ParseStatement("while (count < 10) count++");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a while statement
    auto* whileStmt = dynamic_cast<DMCompiler::DMASTProcStatementWhile*>(stmt.get());
    if (!whileStmt) {
        std::cerr << "FAILED: Not a while statement" << std::endl;
        return false;
    }
    
    // Should have a condition
    if (!whileStmt->Condition) {
        std::cerr << "FAILED: No condition" << std::endl;
        return false;
    }
    
    // Condition should be comparison (count < 10)
    auto* cond = dynamic_cast<DMCompiler::DMASTExpressionBinary*>(whileStmt->Condition.get());
    if (!cond || cond->Operator != DMCompiler::BinaryOperator::Less) {
        std::cerr << "FAILED: Condition is not less-than comparison" << std::endl;
        return false;
    }
    
    // Should have a body
    if (!whileStmt->Body) {
        std::cerr << "FAILED: No body" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: for (init; condition; increment) statement
bool TestForStatement() {
    std::cout << "  TestForStatement... ";
    
    auto stmt = ParseStatement("for (i = 0; i < 10; i++) total += i");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a for statement
    auto* forStmt = dynamic_cast<DMCompiler::DMASTProcStatementFor*>(stmt.get());
    if (!forStmt) {
        std::cerr << "FAILED: Not a for statement" << std::endl;
        return false;
    }
    
    // Should have initializer
    if (!forStmt->Initializer) {
        std::cerr << "FAILED: No initializer" << std::endl;
        return false;
    }
    
    // Should have condition
    if (!forStmt->Condition) {
        std::cerr << "FAILED: No condition" << std::endl;
        return false;
    }
    
    // Should have increment
    if (!forStmt->Increment) {
        std::cerr << "FAILED: No increment" << std::endl;
        return false;
    }
    
    // Should have body
    if (!forStmt->Body) {
        std::cerr << "FAILED: No body" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: break
bool TestBreakStatement() {
    std::cout << "  TestBreakStatement... ";
    
    auto stmt = ParseStatement("break;");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a break statement
    auto* breakStmt = dynamic_cast<DMCompiler::DMASTProcStatementBreak*>(stmt.get());
    if (!breakStmt) {
        std::cerr << "FAILED: Not a break statement" << std::endl;
        return false;
    }
    
    // Should have no label
    if (breakStmt->Label) {
        std::cerr << "FAILED: Should not have label" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: continue
bool TestContinueStatement() {
    std::cout << "  TestContinueStatement... ";
    
    auto stmt = ParseStatement("continue;");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a continue statement
    auto* continueStmt = dynamic_cast<DMCompiler::DMASTProcStatementContinue*>(stmt.get());
    if (!continueStmt) {
        std::cerr << "FAILED: Not a continue statement" << std::endl;
        return false;
    }
    
    // Should have no label
    if (continueStmt->Label) {
        std::cerr << "FAILED: Should not have label" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: do-while
bool TestDoWhileStatement() {
    std::cout << "  TestDoWhileStatement... ";
    
    auto stmt = ParseStatement("do { x++ } while (x < 10);");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a do-while statement
    auto* doWhileStmt = dynamic_cast<DMCompiler::DMASTProcStatementDoWhile*>(stmt.get());
    if (!doWhileStmt) {
        std::cerr << "FAILED: Not a do-while statement" << std::endl;
        return false;
    }
    
    // Should have body
    if (!doWhileStmt->Body) {
        std::cerr << "FAILED: Missing body" << std::endl;
        return false;
    }
    
    // Should have condition
    if (!doWhileStmt->Condition) {
        std::cerr << "FAILED: Missing condition" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: switch
bool TestSwitchStatement() {
    std::cout << "  TestSwitchStatement... ";
    
    auto stmt = ParseStatement("switch(x) { if(1, 2) return 1 else return 0 }");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a switch statement
    auto* switchStmt = dynamic_cast<DMCompiler::DMASTProcStatementSwitch*>(stmt.get());
    if (!switchStmt) {
        std::cerr << "FAILED: Not a switch statement" << std::endl;
        return false;
    }
    
    // Should have value expression
    if (!switchStmt->Value) {
        std::cerr << "FAILED: Missing value expression" << std::endl;
        return false;
    }
    
    // Should have at least one case
    if (switchStmt->Cases.empty()) {
        std::cerr << "FAILED: No cases found" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: del
bool TestDelStatement() {
    std::cout << "  TestDelStatement... ";
    
    auto stmt = ParseStatement("del(obj);");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a del statement
    auto* delStmt = dynamic_cast<DMCompiler::DMASTProcStatementDel*>(stmt.get());
    if (!delStmt) {
        std::cerr << "FAILED: Not a del statement" << std::endl;
        return false;
    }
    
    // Should have value
    if (!delStmt->Value) {
        std::cerr << "FAILED: Missing value" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: spawn
bool TestSpawnStatement() {
    std::cout << "  TestSpawnStatement... ";
    
    auto stmt = ParseStatement("spawn(10) { x++ }");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a spawn statement
    auto* spawnStmt = dynamic_cast<DMCompiler::DMASTProcStatementSpawn*>(stmt.get());
    if (!spawnStmt) {
        std::cerr << "FAILED: Not a spawn statement" << std::endl;
        return false;
    }
    
    // Should have delay
    if (!spawnStmt->Delay) {
        std::cerr << "FAILED: Missing delay" << std::endl;
        return false;
    }
    
    // Should have body
    if (!spawnStmt->Body) {
        std::cerr << "FAILED: Missing body" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: try-catch
bool TestTryCatchStatement() {
    std::cout << "  TestTryCatchStatement... ";
    
    auto stmt = ParseStatement("try { risky() } catch(e) { handle(e) }");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a try-catch statement
    auto* tryStmt = dynamic_cast<DMCompiler::DMASTProcStatementTryCatch*>(stmt.get());
    if (!tryStmt) {
        std::cerr << "FAILED: Not a try-catch statement" << std::endl;
        return false;
    }
    
    // Should have try body
    if (!tryStmt->TryBody) {
        std::cerr << "FAILED: Missing try body" << std::endl;
        return false;
    }
    
    // Should have catch body
    if (!tryStmt->CatchBody) {
        std::cerr << "FAILED: Missing catch body" << std::endl;
        return false;
    }
    
    // Should have catch variable
    if (!tryStmt->CatchVariable) {
        std::cerr << "FAILED: Missing catch variable" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: throw
bool TestThrowStatement() {
    std::cout << "  TestThrowStatement... ";
    
    auto stmt = ParseStatement("throw error;");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a throw statement
    auto* throwStmt = dynamic_cast<DMCompiler::DMASTProcStatementThrow*>(stmt.get());
    if (!throwStmt) {
        std::cerr << "FAILED: Not a throw statement" << std::endl;
        return false;
    }
    
    // Should have value
    if (!throwStmt->Value) {
        std::cerr << "FAILED: Missing value" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: set
bool TestSetStatement() {
    std::cout << "  TestSetStatement... ";
    
    auto stmt = ParseStatement("set name = \"test\";");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a set statement
    auto* setStmt = dynamic_cast<DMCompiler::DMASTProcStatementSet*>(stmt.get());
    if (!setStmt) {
        std::cerr << "FAILED: Not a set statement" << std::endl;
        return false;
    }
    
    // Should have attribute name
    if (setStmt->Attribute.empty()) {
        std::cerr << "FAILED: Missing attribute name" << std::endl;
        return false;
    }
    
    // Should have value
    if (!setStmt->Value) {
        std::cerr << "FAILED: Missing value" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: var declaration without initialization
bool TestVarDeclaration() {
    std::cout << "  TestVarDeclaration... ";
    
    auto stmt = ParseStatement("var x;");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a var declaration
    auto* varDecl = dynamic_cast<DMCompiler::DMASTProcStatementVarDeclaration*>(stmt.get());
    if (!varDecl) {
        std::cerr << "FAILED: Not a var declaration" << std::endl;
        return false;
    }
    
    // Should have one declaration
    if (varDecl->Decls.size() != 1) {
        std::cerr << "FAILED: Wrong decls size" << std::endl;
        return false;
    }
    
    // Should have name "x"
    if (varDecl->Decls[0].Name != "x") {
        std::cerr << "FAILED: Wrong name" << std::endl;
        return false;
    }
    
    // Should not have initialization value
    if (varDecl->Decls[0].Value) {
        std::cerr << "FAILED: Should not have value" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: var declaration with initialization
bool TestVarDeclarationWithValue() {
    std::cout << "  TestVarDeclarationWithValue... ";
    
    auto stmt = ParseStatement("var count = 0;");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a var declaration
    auto* varDecl = dynamic_cast<DMCompiler::DMASTProcStatementVarDeclaration*>(stmt.get());
    if (!varDecl) {
        std::cerr << "FAILED: Not a var declaration" << std::endl;
        return false;
    }
    
    // Should have initialization value
    if (!varDecl->Decls[0].Value) {
        std::cerr << "FAILED: Missing initialization value" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: typed var declaration  
bool TestTypedVarDeclaration() {
    std::cout << "  TestTypedVarDeclaration... ";
    
    // First test simple typed var without initialization
    auto stmt = ParseStatement("var/mob/player;");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null (even without value)" << std::endl;
        return false;
    }
    
    // Should be a var declaration
    auto* varDecl = dynamic_cast<DMCompiler::DMASTProcStatementVarDeclaration*>(stmt.get());
    if (!varDecl) {
        std::cerr << "FAILED: Not a var declaration" << std::endl;
        return false;
    }
    
    // Should have type path /mob
    if (!varDecl->Decls[0].TypePath || varDecl->Decls[0].TypePath->ToString() != "/mob") {
        std::cerr << "FAILED: Wrong type path" << std::endl;
        return false;
    }

    // Should have name player
    if (varDecl->Decls[0].Name != "player") {
        std::cerr << "FAILED: Wrong name" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: multiple var declaration
bool TestMultipleVarDeclaration() {
    std::cout << "  TestMultipleVarDeclaration... ";
    
    auto stmt = ParseStatement("var/x, y = 5, z;");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    auto* varDecl = dynamic_cast<DMCompiler::DMASTProcStatementVarDeclaration*>(stmt.get());
    if (!varDecl) {
        std::cerr << "FAILED: Not a var declaration" << std::endl;
        return false;
    }
    
    if (varDecl->Decls.size() != 3) {
        std::cerr << "FAILED: Should have 3 declarations, got " << varDecl->Decls.size() << std::endl;
        return false;
    }
    
    if (varDecl->Decls[0].Name != "x") {
        std::cerr << "FAILED: First var should be x" << std::endl;
        return false;
    }
    
    if (varDecl->Decls[1].Name != "y") {
        std::cerr << "FAILED: Second var should be y" << std::endl;
        return false;
    }
    
    if (!varDecl->Decls[1].Value) {
        std::cerr << "FAILED: Second var should have value" << std::endl;
        return false;
    }
    
    if (varDecl->Decls[2].Name != "z") {
        std::cerr << "FAILED: Third var should be z" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: goto statement
bool TestGotoStatement() {
    std::cout << "  TestGotoStatement... ";
    
    auto stmt = ParseStatement("goto loop_start;");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a goto statement
    auto* gotoStmt = dynamic_cast<DMCompiler::DMASTProcStatementGoto*>(stmt.get());
    if (!gotoStmt) {
        std::cerr << "FAILED: Not a goto statement" << std::endl;
        return false;
    }
    
    // Should have label
    if (!gotoStmt->Label) {
        std::cerr << "FAILED: Missing label" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: label statement
bool TestLabelStatement() {
    std::cout << "  TestLabelStatement... ";
    
    auto stmt = ParseStatement("loop_start: x++");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a label statement
    auto* labelStmt = dynamic_cast<DMCompiler::DMASTProcStatementLabel*>(stmt.get());
    if (!labelStmt) {
        std::cerr << "FAILED: Not a label statement" << std::endl;
        return false;
    }
    
    // Should have name
    if (labelStmt->Name.empty()) {
        std::cerr << "FAILED: Label name is empty" << std::endl;
        return false;
    }
    
    // Should have body
    if (!labelStmt->Body) {
        std::cerr << "FAILED: Missing body" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: break with label
bool TestBreakWithLabel() {
    std::cout << "  TestBreakWithLabel... ";
    
    auto stmt = ParseStatement("break outer_loop;");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a break statement
    auto* breakStmt = dynamic_cast<DMCompiler::DMASTProcStatementBreak*>(stmt.get());
    if (!breakStmt) {
        std::cerr << "FAILED: Not a break statement" << std::endl;
        return false;
    }
    
    // Should have label
    if (!breakStmt->Label) {
        std::cerr << "FAILED: Missing label" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: continue with label
bool TestContinueWithLabel() {
    std::cout << "  TestContinueWithLabel... ";
    
    auto stmt = ParseStatement("continue outer_loop;");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a continue statement
    auto* continueStmt = dynamic_cast<DMCompiler::DMASTProcStatementContinue*>(stmt.get());
    if (!continueStmt) {
        std::cerr << "FAILED: Not a continue statement" << std::endl;
        return false;
    }
    
    // Should have label
    if (!continueStmt->Label) {
        std::cerr << "FAILED: Missing label" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: nested if statements
bool TestNestedIf() {
    std::cout << "  TestNestedIf... ";
    
    auto stmt = ParseStatement("if (x > 0) if (y > 0) return 1 else return 0");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be an if statement
    auto* ifStmt = dynamic_cast<DMCompiler::DMASTProcStatementIf*>(stmt.get());
    if (!ifStmt) {
        std::cerr << "FAILED: Not an if statement" << std::endl;
        return false;
    }
    
    // Should have body with nested if
    if (!ifStmt->Body) {
        std::cerr << "FAILED: Missing body" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: nested loops
bool TestNestedLoops() {
    std::cout << "  TestNestedLoops... ";
    
    auto stmt = ParseStatement("while (i < 10) { while (j < 10) j++ }");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a while statement
    auto* whileStmt = dynamic_cast<DMCompiler::DMASTProcStatementWhile*>(stmt.get());
    if (!whileStmt) {
        std::cerr << "FAILED: Not a while statement" << std::endl;
        return false;
    }
    
    // Should have body
    if (!whileStmt->Body) {
        std::cerr << "FAILED: Missing body" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: empty block
bool TestEmptyBlock() {
    std::cout << "  TestEmptyBlock... ";
    
    auto stmt = ParseStatement("if (x) { }");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be an if statement
    auto* ifStmt = dynamic_cast<DMCompiler::DMASTProcStatementIf*>(stmt.get());
    if (!ifStmt) {
        std::cerr << "FAILED: Not an if statement" << std::endl;
        return false;
    }
    
    // Should have empty body (no statements)
    if (!ifStmt->Body) {
        std::cerr << "FAILED: Missing body (should be empty block)" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: multiple statements in block
bool TestMultipleStatements() {
    std::cout << "  TestMultipleStatements... ";
    
    auto stmt = ParseStatement("if (x) { y++ ; z-- ; return }");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be an if statement
    auto* ifStmt = dynamic_cast<DMCompiler::DMASTProcStatementIf*>(stmt.get());
    if (!ifStmt) {
        std::cerr << "FAILED: Not an if statement" << std::endl;
        return false;
    }
    
    // Should have body with multiple statements
    if (!ifStmt->Body || ifStmt->Body->Statements.size() < 2) {
        std::cerr << "FAILED: Body should have multiple statements" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// ============================================================================
// Top-Level Parsing Tests
// ============================================================================

bool TestProcDefinition() {
    std::cout << "Testing proc definition... ";
    
    auto stmt = ParseObjectStatement("proc/TestProc() { return 42 }");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    // Should be a proc definition
    auto* procDef = dynamic_cast<DMCompiler::DMASTObjectProcDefinition*>(stmt.get());
    if (!procDef) {
        std::cerr << "FAILED: Not a proc definition" << std::endl;
        return false;
    }
    
    if (procDef->Name != "TestProc") {
        std::cerr << "FAILED: Proc name should be 'TestProc', got '" << procDef->Name << "'" << std::endl;
        return false;
    }
    
    if (procDef->IsVerb) {
        std::cerr << "FAILED: Should not be a verb" << std::endl;
        return false;
    }
    
    if (procDef->Parameters.size() != 0) {
        std::cerr << "FAILED: Should have no parameters" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool TestProcWithParameters() {
    std::cout << "Testing proc with parameters... ";
    
    auto stmt = ParseObjectStatement("proc/Add(a, b) { return a + b }");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    auto* procDef = dynamic_cast<DMCompiler::DMASTObjectProcDefinition*>(stmt.get());
    if (!procDef) {
        std::cerr << "FAILED: Not a proc definition" << std::endl;
        return false;
    }
    
    if (procDef->Parameters.size() != 2) {
        std::cerr << "FAILED: Should have 2 parameters" << std::endl;
        return false;
    }
    
    if (procDef->Parameters[0]->Name != "a" || procDef->Parameters[1]->Name != "b") {
        std::cerr << "FAILED: Parameter names incorrect" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool TestProcWithDefaultParameter() {
    std::cout << "Testing proc with default parameter... ";
    
    auto stmt = ParseObjectStatement("proc/Greet(name = \"World\") { return \"Hello\" }");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    auto* procDef = dynamic_cast<DMCompiler::DMASTObjectProcDefinition*>(stmt.get());
    if (!procDef) {
        std::cerr << "FAILED: Not a proc definition" << std::endl;
        return false;
    }
    
    if (procDef->Parameters.size() != 1) {
        std::cerr << "FAILED: Should have 1 parameter" << std::endl;
        return false;
    }
    
    if (!procDef->Parameters[0]->DefaultValue) {
        std::cerr << "FAILED: Parameter should have default value" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool TestVerbDefinition() {
    std::cout << "Testing verb definition... ";
    
    auto stmt = ParseObjectStatement("verb/Say(msg as text) { return msg }");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    auto* procDef = dynamic_cast<DMCompiler::DMASTObjectProcDefinition*>(stmt.get());
    if (!procDef) {
        std::cerr << "FAILED: Not a proc definition" << std::endl;
        return false;
    }
    
    if (!procDef->IsVerb) {
        std::cerr << "FAILED: Should be a verb" << std::endl;
        return false;
    }
    
    if (procDef->Name != "Say") {
        std::cerr << "FAILED: Verb name should be 'Say'" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool TestObjectVarDefinition() {
    std::cout << "Testing object var definition... ";
    
    auto stmt = ParseObjectStatement("var/health = 100");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    auto* varDef = dynamic_cast<DMCompiler::DMASTObjectVarDefinition*>(stmt.get());
    if (!varDef) {
        std::cerr << "FAILED: Not a var definition" << std::endl;
        return false;
    }
    
    if (varDef->Name != "health") {
        std::cerr << "FAILED: Var name should be 'health'" << std::endl;
        return false;
    }
    
    if (!varDef->Value) {
        std::cerr << "FAILED: Should have initialization value" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool TestObjectDefinition() {
    std::cout << "Testing object definition... ";
    
    auto stmt = ParseObjectStatement("/mob/player { }");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    auto* objDef = dynamic_cast<DMCompiler::DMASTObjectDefinition*>(stmt.get());
    if (!objDef) {
        std::cerr << "FAILED: Not an object definition" << std::endl;
        return false;
    }
    
    if (objDef->Path.Path.GetElements().size() != 2) {
        std::cerr << "FAILED: Path should have 2 elements" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool TestNestedObjectDefinition() {
    std::cout << "Testing nested object definition... ";
    
    auto stmt = ParseObjectStatement("/mob { /player { var/health = 100 } }");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    auto* objDef = dynamic_cast<DMCompiler::DMASTObjectDefinition*>(stmt.get());
    if (!objDef) {
        std::cerr << "FAILED: Not an object definition" << std::endl;
        return false;
    }
    
    if (objDef->InnerStatements.size() != 1) {
        std::cerr << "FAILED: Should have 1 inner statement" << std::endl;
        return false;
    }
    
    auto* innerObj = dynamic_cast<DMCompiler::DMASTObjectDefinition*>(objDef->InnerStatements[0].get());
    if (!innerObj) {
        std::cerr << "FAILED: Inner statement should be object definition" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool TestProcInObject() {
    std::cout << "Testing proc in object... ";
    
    auto stmt = ParseObjectStatement("/mob/player { proc/Attack() { return 10 } }");
    if (!stmt) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    auto* objDef = dynamic_cast<DMCompiler::DMASTObjectDefinition*>(stmt.get());
    if (!objDef) {
        std::cerr << "FAILED: Not an object definition" << std::endl;
        return false;
    }
    
    if (objDef->InnerStatements.size() != 1) {
        std::cerr << "FAILED: Should have 1 inner statement" << std::endl;
        return false;
    }
    
    auto* procDef = dynamic_cast<DMCompiler::DMASTObjectProcDefinition*>(objDef->InnerStatements[0].get());
    if (!procDef) {
        std::cerr << "FAILED: Inner statement should be proc definition" << std::endl;
        return false;
    }
    
    if (procDef->Name != "Attack") {
        std::cerr << "FAILED: Proc name should be 'Attack'" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// ============================================================================
// Integration Tests - Parsing Complete Files
// ============================================================================

bool TestParseSimpleFile() {
    std::cout << "Testing simple file parsing... ";
    
    const char* source = "/mob/player { var/health = 100 }";
    
    auto file = ParseFile(source);
    if (!file) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    if (file->Statements.size() != 1) {
        std::cerr << "FAILED: Expected 1 statement, got " << file->Statements.size() << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool TestParseMultipleProcs() {
    std::cout << "Testing multiple procs... ";
    
    const char* source = R"(
        proc/Add(a, b)
            return a + b
            
        proc/Multiply(x, y)
            return x * y
            
        verb/Say(msg as text)
            return msg
    )";
    
    auto file = ParseFile(source);
    if (!file) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    if (file->Statements.size() != 3) {
        std::cerr << "FAILED: Expected 3 statements, got " << file->Statements.size() << std::endl;
        return false;
    }
    
    // Check first is proc
    auto* proc1 = dynamic_cast<DMCompiler::DMASTObjectProcDefinition*>(file->Statements[0].get());
    if (!proc1 || proc1->Name != "Add") {
        std::cerr << "FAILED: First statement should be proc Add" << std::endl;
        return false;
    }
    
    // Check third is verb
    auto* verb = dynamic_cast<DMCompiler::DMASTObjectProcDefinition*>(file->Statements[2].get());
    if (!verb || !verb->IsVerb) {
        std::cerr << "FAILED: Third statement should be a verb" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool TestParseObjectHierarchy() {
    std::cout << "Testing object hierarchy... ";
    
    const char* source = R"(/mob
    var/health = 100
    var/mana = 50
    
    proc/Attack()
        return 10
        
    /player
        var/level = 1
        
        proc/LevelUp()
            level++
            
    /enemy
        var/aggro = 0
)";
    
    auto file = ParseFile(source);
    if (!file) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    if (file->Statements.size() != 1) {
        std::cerr << "FAILED: Expected 1 top-level statement" << std::endl;
        return false;
    }
    
    auto* mobDef = dynamic_cast<DMCompiler::DMASTObjectDefinition*>(file->Statements[0].get());
    if (!mobDef) {
        std::cerr << "FAILED: Should be object definition" << std::endl;
        return false;
    }
    
    // Should have: 2 vars, 1 proc, 2 nested objects
    if (mobDef->InnerStatements.size() < 3) {
        std::cerr << "FAILED: Expected at least 3 inner statements" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool TestParseMixedContent() {
    std::cout << "Testing mixed content... ";
    
    const char* source = R"(var/global_var = "test"

/obj/item
    var/weight = 1.0
    
proc/GlobalProc()
    return 100
    
/mob/player
    proc/Move()
        return 1
)";
    
    auto file = ParseFile(source);
    if (!file) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    if (file->Statements.size() != 4) {
        std::cerr << "FAILED: Expected 4 statements, got " << file->Statements.size() << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool TestParseEmptyFile() {
    std::cout << "Testing empty file... ";
    
    auto file = ParseFile("");
    if (!file) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    if (file->Statements.size() != 0) {
        std::cerr << "FAILED: Expected 0 statements" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool TestParseComplexProc() {
    std::cout << "Testing complex proc body... ";
    
    const char* source = R"(proc/Calculate(x, y, z = 0)
    var/result = 0
    
    if (x > y)
        result = x + y
    else
        result = x - y
        
    for (var/i = 0; i < z; i++)
        result += i
        
    return result
)";
    
    auto file = ParseFile(source);
    if (!file) {
        std::cerr << "FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    if (file->Statements.size() != 1) {
        std::cerr << "FAILED: Expected 1 statement, got " << file->Statements.size() << std::endl;
        return false;
    }
    
    auto* procDef = dynamic_cast<DMCompiler::DMASTObjectProcDefinition*>(file->Statements[0].get());
    if (!procDef) {
        std::cerr << "FAILED: Should be proc definition" << std::endl;
        return false;
    }
    
    if (procDef->Parameters.size() != 3) {
        std::cerr << "FAILED: Expected 3 parameters" << std::endl;
        return false;
    }
    
    // Check third parameter has default value
    if (!procDef->Parameters[2]->DefaultValue) {
        std::cerr << "FAILED: Third parameter should have default value" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Placeholder for parser tests
int RunParserTests() {
    std::cout << "\n=== Running Parser Tests ===" << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    // Run expression tests
    std::cout << "\nExpression Parsing Tests:" << std::endl;
    if (TestIntegerLiteral()) passed++; else failed++;
    if (TestAddition()) passed++; else failed++;
    if (TestOperatorPrecedence()) passed++; else failed++;
    if (TestUnaryNegation()) passed++; else failed++;
    if (TestParentheses()) passed++; else failed++;
    
    std::cout << "\nComparison Operator Tests:" << std::endl;
    if (TestComparison()) passed++; else failed++;
    if (TestEquality()) passed++; else failed++;
    if (TestComparisonWithArithmetic()) passed++; else failed++;
    
    std::cout << "\nLogical Operator Tests:" << std::endl;
    if (TestLogicalAnd()) passed++; else failed++;
    if (TestLogicalOr()) passed++; else failed++;
    if (TestLogicalPrecedence()) passed++; else failed++;
    if (TestComparisonWithLogical()) passed++; else failed++;
    
    std::cout << "\nBitwise Operator Tests:" << std::endl;
    if (TestBitwiseAnd()) passed++; else failed++;
    if (TestBitwiseOr()) passed++; else failed++;
    if (TestBitwiseXor()) passed++; else failed++;
    if (TestLeftShift()) passed++; else failed++;
    if (TestRightShift()) passed++; else failed++;
    if (TestBitwisePrecedence()) passed++; else failed++;
    
    std::cout << "\nAssignment Operator Tests:" << std::endl;
    if (TestSimpleAssignment()) passed++; else failed++;
    if (TestCompoundAssignment()) passed++; else failed++;
    if (TestChainedAssignment()) passed++; else failed++;
    if (TestAssignmentWithExpression()) passed++; else failed++;
    
    // Ternary Operator Tests
    if (TestSimpleTernary()) passed++; else failed++;
    if (TestNestedTernary()) passed++; else failed++;
    if (TestTernaryWithExpressions()) passed++; else failed++;
    if (TestTernaryWithAssignment()) passed++; else failed++;
    
    std::cout << "\nPostfix Expression Tests:" << std::endl;
    if (TestMemberAccess()) passed++; else failed++;
    if (TestFunctionCall()) passed++; else failed++;
    if (TestNamedParameters()) passed++; else failed++;
    if (TestMixedParameters()) passed++; else failed++;
    if (TestArrayIndexing()) passed++; else failed++;
    if (TestChainedPostfix()) passed++; else failed++;
    
    std::cout << "\nSpecial DM Expression Tests:" << std::endl;
    if (TestAbsolutePath()) passed++; else failed++;
    if (TestRelativePath()) passed++; else failed++;
    if (TestNewExpression()) passed++; else failed++;
    if (TestNewWithArguments()) passed++; else failed++;
    if (TestListExpression()) passed++; else failed++;
    
    std::cout << "\nStatement Parsing Tests:" << std::endl;
    if (TestReturnStatement()) passed++; else failed++;
    if (TestReturnWithValue()) passed++; else failed++;
    if (TestIfStatement()) passed++; else failed++;
    if (TestIfElseStatement()) passed++; else failed++;
    if (TestWhileStatement()) passed++; else failed++;
    if (TestDoWhileStatement()) passed++; else failed++;
    if (TestForStatement()) passed++; else failed++;
    if (TestSwitchStatement()) passed++; else failed++;
    if (TestBreakStatement()) passed++; else failed++;
    if (TestContinueStatement()) passed++; else failed++;
    if (TestDelStatement()) passed++; else failed++;
    if (TestSpawnStatement()) passed++; else failed++;
    if (TestTryCatchStatement()) passed++; else failed++;
    if (TestThrowStatement()) passed++; else failed++;
    if (TestSetStatement()) passed++; else failed++;
    if (TestVarDeclaration()) passed++; else failed++;
    if (TestVarDeclarationWithValue()) passed++; else failed++;
    if (TestTypedVarDeclaration()) passed++; else failed++;
    if (TestMultipleVarDeclaration()) passed++; else failed++;
    if (TestGotoStatement()) passed++; else failed++;
    if (TestLabelStatement()) passed++; else failed++;
    if (TestBreakWithLabel()) passed++; else failed++;
    if (TestContinueWithLabel()) passed++; else failed++;
    
    std::cout << "\nAdvanced Control Flow Tests:" << std::endl;
    if (TestNestedIf()) passed++; else failed++;
    if (TestNestedLoops()) passed++; else failed++;
    if (TestEmptyBlock()) passed++; else failed++;
    if (TestMultipleStatements()) passed++; else failed++;
    
    std::cout << "\nTop-Level Parsing Tests:" << std::endl;
    if (TestProcDefinition()) passed++; else failed++;
    if (TestProcWithParameters()) passed++; else failed++;
    if (TestProcWithDefaultParameter()) passed++; else failed++;
    if (TestVerbDefinition()) passed++; else failed++;
    if (TestObjectVarDefinition()) passed++; else failed++;
    if (TestObjectDefinition()) passed++; else failed++;
    if (TestNestedObjectDefinition()) passed++; else failed++;
    if (TestProcInObject()) passed++; else failed++;
    
    std::cout << "\nIntegration Tests - Complete Files:" << std::endl;
    if (TestParseSimpleFile()) passed++; else failed++;
    if (TestParseMultipleProcs()) passed++; else failed++;
    if (TestParseObjectHierarchy()) passed++; else failed++;
    if (TestParseMixedContent()) passed++; else failed++;
    if (TestParseEmptyFile()) passed++; else failed++;
    if (TestParseComplexProc()) passed++; else failed++;
    
    // Summary
    std::cout << "\n=== Parser Test Summary ===" << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    
    return (failed == 0) ? 0 : 1;
}
