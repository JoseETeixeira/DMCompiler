#include "DMStatementCompiler.h"
#include "DMExpressionCompiler.h"
#include "DMCompiler.h"
#include "DMProc.h"
#include "DMObject.h"
#include "BytecodeWriter.h"
#include "DreamProcOpcode.h"
#include "DMASTStatement.h"
#include "DreamPath.h"
#include <cassert>
#include <iostream>
#include <vector>

// Test goto with forward reference: goto target; label target:
bool TestGotoForwardReference() {
    std::cout << "  TestGotoForwardReference... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    DMCompiler::DMStatementCompiler stmtCompiler(&compiler, &proc, &writer, &exprCompiler);
    
    // Create AST: goto target; label target:
    
    // goto target
    auto gotoLabel = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "target");
    auto gotoStmt = std::make_unique<DMCompiler::DMASTProcStatementGoto>(
        DMCompiler::Location(),
        std::move(gotoLabel)
    );
    
    // label target:
    auto labelStmt = std::make_unique<DMCompiler::DMASTProcStatementLabel>(
        DMCompiler::Location(),
        "target"
    );
    
    // Compile goto (forward reference)
    bool success1 = stmtCompiler.CompileStatement(gotoStmt.get());
    assert(success1 && "Should successfully compile goto statement");
    
    // Compile label (defines the target)
    bool success2 = stmtCompiler.CompileStatement(labelStmt.get());
    assert(success2 && "Should successfully compile label statement");
    
    // Finalize to resolve forward references
    bool finalized = stmtCompiler.Finalize();
    assert(finalized && "Should successfully finalize and resolve forward references");
    
    // Finalize bytecode writer to resolve jumps
    writer.Finalize();
    
    const auto& bytecode = writer.GetBytecode();
    
    // Expected bytecode:
    //   Jump target_label
    // target_label:
    
    assert(bytecode.size() > 0 && "Should emit bytecode");
    
    // Find Jump opcode
    bool foundJump = false;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Jump)) {
            foundJump = true;
            break;
        }
    }
    assert(foundJump && "Should emit Jump opcode for goto");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test goto with backward reference: label target:; goto target;
bool TestGotoBackwardReference() {
    std::cout << "  TestGotoBackwardReference... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    DMCompiler::DMStatementCompiler stmtCompiler(&compiler, &proc, &writer, &exprCompiler);
    
    // Create AST: label target:; goto target;
    
    // label target:
    auto labelStmt = std::make_unique<DMCompiler::DMASTProcStatementLabel>(
        DMCompiler::Location(),
        "target"
    );
    
    // goto target
    auto gotoLabel = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "target");
    auto gotoStmt = std::make_unique<DMCompiler::DMASTProcStatementGoto>(
        DMCompiler::Location(),
        std::move(gotoLabel)
    );
    
    // Compile label first
    bool success1 = stmtCompiler.CompileStatement(labelStmt.get());
    assert(success1 && "Should successfully compile label statement");
    
    // Compile goto (backward reference)
    bool success2 = stmtCompiler.CompileStatement(gotoStmt.get());
    assert(success2 && "Should successfully compile goto statement");
    
    // Finalize (should have no unresolved forward references)
    bool finalized = stmtCompiler.Finalize();
    assert(finalized && "Should successfully finalize with no forward references");
    
    // Finalize bytecode writer
    writer.Finalize();
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have Jump opcode
    bool foundJump = false;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Jump)) {
            foundJump = true;
            break;
        }
    }
    assert(foundJump && "Should emit Jump opcode for goto");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test goto with undefined label (should fail during finalize)
bool TestGotoUndefinedLabel() {
    std::cout << "  TestGotoUndefinedLabel... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    DMCompiler::DMStatementCompiler stmtCompiler(&compiler, &proc, &writer, &exprCompiler);
    
    // Create AST: goto undefined_label; (no label definition)
    auto gotoLabel = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "undefined_label");
    auto gotoStmt = std::make_unique<DMCompiler::DMASTProcStatementGoto>(
        DMCompiler::Location(),
        std::move(gotoLabel)
    );
    
    // Compile goto
    bool success = stmtCompiler.CompileStatement(gotoStmt.get());
    assert(success && "Should successfully compile goto statement (error detected during finalize)");
    
    // Finalize should detect the undefined label
    bool finalized = stmtCompiler.Finalize();
    assert(!finalized && "Should fail to finalize due to undefined label");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test multiple gotos to same forward label
bool TestMultipleGotosToSameLabel() {
    std::cout << "  TestMultipleGotosToSameLabel... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    DMCompiler::DMStatementCompiler stmtCompiler(&compiler, &proc, &writer, &exprCompiler);
    
    // Create AST: goto target; goto target; label target:
    
    // First goto
    auto gotoLabel1 = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "target");
    auto gotoStmt1 = std::make_unique<DMCompiler::DMASTProcStatementGoto>(
        DMCompiler::Location(),
        std::move(gotoLabel1)
    );
    
    // Second goto
    auto gotoLabel2 = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "target");
    auto gotoStmt2 = std::make_unique<DMCompiler::DMASTProcStatementGoto>(
        DMCompiler::Location(),
        std::move(gotoLabel2)
    );
    
    // Label
    auto labelStmt = std::make_unique<DMCompiler::DMASTProcStatementLabel>(
        DMCompiler::Location(),
        "target"
    );
    
    // Compile all statements
    bool success1 = stmtCompiler.CompileStatement(gotoStmt1.get());
    assert(success1 && "Should successfully compile first goto");
    
    bool success2 = stmtCompiler.CompileStatement(gotoStmt2.get());
    assert(success2 && "Should successfully compile second goto");
    
    bool success3 = stmtCompiler.CompileStatement(labelStmt.get());
    assert(success3 && "Should successfully compile label");
    
    // Finalize
    bool finalized = stmtCompiler.Finalize();
    assert(finalized && "Should successfully finalize with multiple forward references");
    
    // Finalize bytecode writer
    writer.Finalize();
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have two Jump opcodes
    int jumpCount = 0;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Jump)) {
            jumpCount++;
        }
    }
    assert(jumpCount == 2 && "Should emit two Jump opcodes");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

int RunGotoForwardRefTests() {
    std::cout << "\n=== Running Goto Forward Reference Tests ===" << std::endl;
    
    int failures = 0;
    
    try {
        if (!TestGotoForwardReference()) failures++;
        if (!TestGotoBackwardReference()) failures++;
        if (!TestGotoUndefinedLabel()) failures++;
        if (!TestMultipleGotosToSameLabel()) failures++;
    } catch (const std::exception& e) {
        std::cerr << "Exception during goto forward reference tests: " << e.what() << std::endl;
        failures++;
    }
    
    if (failures == 0) {
        std::cout << "All goto forward reference tests passed!" << std::endl;
    } else {
        std::cout << failures << " goto forward reference test(s) failed!" << std::endl;
    }
    
    return failures;
}
