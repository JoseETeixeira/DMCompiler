#include "DMStatementCompiler.h"
#include "DMExpressionCompiler.h"
#include "DMCompiler.h"
#include "DMProc.h"
#include "DMObject.h"
#include "BytecodeWriter.h"
#include "DreamProcOpcode.h"
#include "DMASTExpression.h"
#include "DMASTStatement.h"
#include "DreamPath.h"
#include <cassert>
#include <iostream>
#include <vector>

// Test simple if statement: if (x) { return 1 }
bool TestCompileSimpleIf() {
    std::cout << "  TestCompileSimpleIf... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variable 'x'
    proc.AddLocalVariable("x");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    DMCompiler::DMStatementCompiler stmtCompiler(&compiler, &proc, &writer, &exprCompiler);
    
    // Create AST: if (x) { return 1 }
    auto condition = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    
    // Body: return 1
    auto returnValue = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 1);
    auto returnStmt = std::make_unique<DMCompiler::DMASTProcStatementReturn>(DMCompiler::Location(), std::move(returnValue));
    
    std::vector<std::unique_ptr<DMCompiler::DMASTProcStatement>> bodyStatements;
    bodyStatements.push_back(std::move(returnStmt));
    auto body = std::make_unique<DMCompiler::DMASTProcBlockInner>(DMCompiler::Location(), std::move(bodyStatements));
    
    auto ifStmt = std::make_unique<DMCompiler::DMASTProcStatementIf>(
        DMCompiler::Location(),
        std::move(condition),
        std::move(body),
        nullptr // no else
    );
    
    // Compile
    bool success = stmtCompiler.CompileStatement(ifStmt.get());
    assert(success && "Should successfully compile if statement");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Expected bytecode:
    //   Push ReferenceValue (x)
    //   JumpIfFalse end_label
    //   PushFloat 1.0
    //   Return
    // end_label:
    
    assert(bytecode.size() > 5 && "If statement should emit bytecode");
    
    // Find JumpIfFalse opcode
    bool foundJumpIfFalse = false;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::JumpIfFalse)) {
            foundJumpIfFalse = true;
            break;
        }
    }
    assert(foundJumpIfFalse && "Should emit JumpIfFalse opcode");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test if-else statement: if (x) { return 1 } else { return 2 }
bool TestCompileIfElse() {
    std::cout << "  TestCompileIfElse... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    proc.AddLocalVariable("x");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    DMCompiler::DMStatementCompiler stmtCompiler(&compiler, &proc, &writer, &exprCompiler);
    
    // Create AST: if (x) { return 1 } else { return 2 }
    auto condition = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    
    // If body: return 1
    auto returnValue1 = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 1);
    auto returnStmt1 = std::make_unique<DMCompiler::DMASTProcStatementReturn>(DMCompiler::Location(), std::move(returnValue1));
    std::vector<std::unique_ptr<DMCompiler::DMASTProcStatement>> ifBody;
    ifBody.push_back(std::move(returnStmt1));
    auto ifBodyBlock = std::make_unique<DMCompiler::DMASTProcBlockInner>(DMCompiler::Location(), std::move(ifBody));
    
    // Else body: return 2
    auto returnValue2 = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 2);
    auto returnStmt2 = std::make_unique<DMCompiler::DMASTProcStatementReturn>(DMCompiler::Location(), std::move(returnValue2));
    std::vector<std::unique_ptr<DMCompiler::DMASTProcStatement>> elseBody;
    elseBody.push_back(std::move(returnStmt2));
    auto elseBodyBlock = std::make_unique<DMCompiler::DMASTProcBlockInner>(DMCompiler::Location(), std::move(elseBody));
    
    auto ifStmt = std::make_unique<DMCompiler::DMASTProcStatementIf>(
        DMCompiler::Location(),
        std::move(condition),
        std::move(ifBodyBlock),
        std::move(elseBodyBlock)
    );
    
    // Compile
    bool success = stmtCompiler.CompileStatement(ifStmt.get());
    assert(success && "Should successfully compile if-else statement");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Expected bytecode should have both JumpIfFalse and Jump
    int jumpCount = 0;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::JumpIfFalse) ||
            bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Jump)) {
            jumpCount++;
        }
    }
    assert(jumpCount >= 2 && "If-else should have at least 2 jumps");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test while loop: while (x) { x = x - 1 }
bool TestCompileWhileLoop() {
    std::cout << "  TestCompileWhileLoop... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    proc.AddLocalVariable("x");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    DMCompiler::DMStatementCompiler stmtCompiler(&compiler, &proc, &writer, &exprCompiler);
    
    // Create AST: while (x) { x = x - 1 }
    auto condition = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    
    // Body: x = x - 1
    auto lhs = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    auto rhs_x = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    auto rhs_one = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 1);
    auto subtraction = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location(),
        DMCompiler::BinaryOperator::Subtract,
        std::move(rhs_x),
        std::move(rhs_one)
    );
    auto assignment = std::make_unique<DMCompiler::DMASTAssign>(
        DMCompiler::Location(),
        std::move(lhs),
        DMCompiler::AssignmentOperator::Assign,
        std::move(subtraction)
    );
    auto exprStmt = std::make_unique<DMCompiler::DMASTProcStatementExpression>(DMCompiler::Location(), std::move(assignment));
    
    std::vector<std::unique_ptr<DMCompiler::DMASTProcStatement>> bodyStatements;
    bodyStatements.push_back(std::move(exprStmt));
    auto body = std::make_unique<DMCompiler::DMASTProcBlockInner>(DMCompiler::Location(), std::move(bodyStatements));
    
    auto whileStmt = std::make_unique<DMCompiler::DMASTProcStatementWhile>(
        DMCompiler::Location(),
        std::move(condition),
        std::move(body)
    );
    
    // Compile
    bool success = stmtCompiler.CompileStatement(whileStmt.get());
    assert(success && "Should successfully compile while loop");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Expected: should have both Jump and JumpIfFalse (loop back and loop exit)
    bool foundJump = false;
    bool foundJumpIfFalse = false;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Jump)) {
            foundJump = true;
        }
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::JumpIfFalse)) {
            foundJumpIfFalse = true;
        }
    }
    assert(foundJump && "While loop should have Jump (loop back)");
    assert(foundJumpIfFalse && "While loop should have JumpIfFalse (exit)");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test for loop: for(var/i = 0; i < 10; i++) { }
bool TestCompileForLoop() {
    std::cout << "  TestCompileForLoop... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    DMCompiler::DMStatementCompiler stmtCompiler(&compiler, &proc, &writer, &exprCompiler);
    
    // Create AST: for(var/i = 0; i < 10; i++) { }
    
    // Initializer: var/i = 0
    auto initValue = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 0);
    
    std::vector<DMCompiler::DMASTProcStatementVarDeclaration::Decl> decls;
    decls.emplace_back(
        DMCompiler::Location(),
        "i",
        std::nullopt,
        std::move(initValue),
        std::nullopt,
        false
    );
    
    auto initializer = std::make_unique<DMCompiler::DMASTProcStatementVarDeclaration>(
        DMCompiler::Location(),
        std::move(decls)
    );
    
    // Condition: i < 10
    auto condLeft = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "i");
    auto condRight = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 10);
    auto condition = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location(),
        DMCompiler::BinaryOperator::Less,
        std::move(condLeft),
        std::move(condRight)
    );
    
    // Increment: i++
    auto incOperand = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "i");
    auto increment = std::make_unique<DMCompiler::DMASTExpressionUnary>(
        DMCompiler::Location(),
        DMCompiler::UnaryOperator::PostIncrement,
        std::move(incOperand)
    );
    
    // Empty body
    std::vector<std::unique_ptr<DMCompiler::DMASTProcStatement>> bodyStatements;
    auto body = std::make_unique<DMCompiler::DMASTProcBlockInner>(DMCompiler::Location(), std::move(bodyStatements));
    
    auto forStmt = std::make_unique<DMCompiler::DMASTProcStatementFor>(
        DMCompiler::Location(),
        std::move(initializer),
        std::move(condition),
        std::move(increment),
        std::move(body)
    );
    
    // Compile
    bool success = stmtCompiler.CompileStatement(forStmt.get());
    assert(success && "Should successfully compile for loop");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have jump opcodes for loop control
    bool foundJump = false;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Jump)) {
            foundJump = true;
            break;
        }
    }
    assert(foundJump && "For loop should have Jump opcode");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test break statement in while loop
bool TestCompileBreakStatement() {
    std::cout << "  TestCompileBreakStatement... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    proc.AddLocalVariable("x");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    DMCompiler::DMStatementCompiler stmtCompiler(&compiler, &proc, &writer, &exprCompiler);
    
    // Create AST: while (1) { break }
    auto condition = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 1);
    
    // Body: break
    auto breakStmt = std::make_unique<DMCompiler::DMASTProcStatementBreak>(DMCompiler::Location(), nullptr);
    std::vector<std::unique_ptr<DMCompiler::DMASTProcStatement>> bodyStatements;
    bodyStatements.push_back(std::move(breakStmt));
    auto body = std::make_unique<DMCompiler::DMASTProcBlockInner>(DMCompiler::Location(), std::move(bodyStatements));
    
    auto whileStmt = std::make_unique<DMCompiler::DMASTProcStatementWhile>(
        DMCompiler::Location(),
        std::move(condition),
        std::move(body)
    );
    
    // Compile
    bool success = stmtCompiler.CompileStatement(whileStmt.get());
    assert(success && "Should successfully compile while with break");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have multiple jump opcodes
    int jumpCount = 0;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Jump)) {
            jumpCount++;
        }
    }
    assert(jumpCount >= 2 && "Should have jumps for break and loop");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

int RunStatementCompilerTests() {
    std::cout << "\n=== Running Statement Compiler Tests ===" << std::endl;
    
    int failures = 0;
    
    try {
        if (!TestCompileSimpleIf()) failures++;
        if (!TestCompileIfElse()) failures++;
        if (!TestCompileWhileLoop()) failures++;
        if (!TestCompileForLoop()) failures++;
        if (!TestCompileBreakStatement()) failures++;
    } catch (const std::exception& e) {
        std::cerr << "Exception during statement compiler tests: " << e.what() << std::endl;
        failures++;
    }
    
    if (failures == 0) {
        std::cout << "All statement compiler tests passed!" << std::endl;
    } else {
        std::cout << failures << " statement compiler test(s) failed!" << std::endl;
    }
    
    return failures;
}
