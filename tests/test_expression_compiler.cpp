#include <iostream>
#include <cassert>
#include "../include/DMExpressionCompiler.h"
#include "../include/BytecodeWriter.h"
#include "../include/DMASTExpression.h"
#include "../include/DMProc.h"
#include "../include/DMObject.h"
#include "../include/DMCompiler.h"
#include "../include/DMObjectTree.h"

// Simplified expression compiler tests
// Uses explicit namespace qualification to avoid collision between
// DMCompiler namespace and DMCompiler class

// Test compiling a simple integer constant
bool TestCompileIntegerConstant() {
    std::cout << "  TestCompileIntegerConstant... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: 42
    auto expr = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 42);
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile integer constant");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should be: PushFloat 42.0 (1 opcode + 4 bytes = 5 bytes)
    assert(bytecode.size() == 5 && "Integer constant should emit 5 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test compiling a binary expression (addition)
bool TestCompileAddition() {
    std::cout << "  TestCompileAddition... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: 5 + 3
    auto left = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 5);
    auto right = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 3);
    auto expr = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location(), 
        DMCompiler::BinaryOperator::Add, 
        std::move(left), 
        std::move(right)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile addition");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should be: PushFloat 5.0, PushFloat 3.0, Add = 5 + 5 + 1 = 11 bytes
    assert(bytecode.size() == 11 && "Addition should emit 11 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    assert(bytecode[5] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    assert(bytecode[10] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Add));
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test compiling a unary expression (negation)
bool TestCompileUnaryNegation() {
    std::cout << "  TestCompileUnaryNegation... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: -5
    auto five = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 5);
    auto negate = std::make_unique<DMCompiler::DMASTExpressionUnary>(
        DMCompiler::Location(), 
        DMCompiler::UnaryOperator::Negate, 
        std::move(five)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(negate.get());
    assert(success && "Should successfully compile negation");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should be: PushFloat 5.0, Negate = 5 + 1 = 6 bytes
    assert(bytecode.size() == 6 && "Negation should emit 6 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    assert(bytecode[5] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Negate));
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test compiling a local variable reference
bool TestCompileLocalVariable() {
    std::cout << "  TestCompileLocalVariable... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add a local variable
    proc.AddLocalVariable("myvar");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: myvar (identifier)
    auto expr = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "myvar");
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile local variable");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should be: PushReferenceValue <Type.Local=28> <Id=0> = 1 + 1 + 1 = 3 bytes
    assert(bytecode.size() == 3 && "Local variable reference should emit 3 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushReferenceValue));
    assert(bytecode[1] == 28 && "Should be Local reference type");
    assert(bytecode[2] == 0 && "Should be variable ID 0");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: Compile parameter reference
bool TestCompileParameter() {
    std::cout << "  TestCompileParameter... ";
    
    // Create compiler and proc with parameter "arg1"
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    proc.AddParameter("arg1");  // Parameters are also LocalVariables with IsParameter=true
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Compile: arg1
    auto expr = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "arg1");
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Failed to compile parameter reference");
    
    // Validate bytecode: PushReferenceValue + Type.Local + ID
    const std::vector<uint8_t>& bytecode = writer.GetBytecode();
    assert(bytecode.size() == 3 && "Parameter reference should produce 3 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushReferenceValue) && "First byte should be PushReferenceValue");
    assert(bytecode[1] == 28 && "Second byte should be Type.Local (28)");
    assert(bytecode[2] == 0 && "Third byte should be parameter ID (0)");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: Compile special identifier 'src'
bool TestCompileSpecialIdentifierSrc() {
    std::cout << "  TestCompileSpecialIdentifierSrc... ";
    
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Compile: src
    auto expr = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "src");
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Failed to compile 'src' identifier");
    
    // Validate bytecode: PushReferenceValue + Type.Src
    const std::vector<uint8_t>& bytecode = writer.GetBytecode();
    assert(bytecode.size() == 2 && "'src' reference should produce 2 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushReferenceValue) && "First byte should be PushReferenceValue");
    assert(bytecode[1] == 1 && "Second byte should be Type.Src (1)");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: Compile expression with local variable: x + 5
bool TestCompileExpressionWithVariable() {
    std::cout << "  TestCompileExpressionWithVariable... ";
    
    // Create compiler and proc with local variable "x"
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    proc.AddLocalVariable("x");
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Compile: x + 5
    auto left = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    auto right = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 5);
    auto expr = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location(),
        DMCompiler::BinaryOperator::Add,
        std::move(left),
        std::move(right)
    );
    
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Failed to compile 'x + 5'");
    
    // Validate bytecode:
    // 1. PushReferenceValue + Type.Local + ID (x) = 3 bytes
    // 2. PushFloat + value (5.0) = 5 bytes
    // 3. Add = 1 byte
    // Total = 9 bytes
    const std::vector<uint8_t>& bytecode = writer.GetBytecode();
    assert(bytecode.size() == 9 && "'x + 5' should produce 9 bytes");
    
    // x reference
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushReferenceValue));
    assert(bytecode[1] == 28);  // Type.Local
    assert(bytecode[2] == 0);   // Variable ID
    
    // Float 5.0
    assert(bytecode[3] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    float value = *reinterpret_cast<const float*>(&bytecode[4]);
    assert(value == 5.0f);
    
    // Add operation
    assert(bytecode[8] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Add));
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: Compile expression with multiple variables: x + y
bool TestCompileMultipleVariables() {
    std::cout << "  TestCompileMultipleVariables... ";
    
    // Create compiler and proc with two local variables
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    proc.AddLocalVariable("x");  // ID 0
    proc.AddLocalVariable("y");  // ID 1
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Compile: x + y
    auto left = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    auto right = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "y");
    auto expr = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location(),
        DMCompiler::BinaryOperator::Add,
        std::move(left),
        std::move(right)
    );
    
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Failed to compile 'x + y'");
    
    // Validate bytecode:
    // 1. PushReferenceValue + Type.Local + 0 (x)
    // 2. PushReferenceValue + Type.Local + 1 (y)
    // 3. Add
    const std::vector<uint8_t>& bytecode = writer.GetBytecode();
    assert(bytecode.size() == 7 && "'x + y' should produce 7 bytes");
    
    // x reference
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushReferenceValue));
    assert(bytecode[1] == 28);  // Type.Local
    assert(bytecode[2] == 0);   // Variable ID 0
    
    // y reference
    assert(bytecode[3] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushReferenceValue));
    assert(bytecode[4] == 28);  // Type.Local
    assert(bytecode[5] == 1);   // Variable ID 1
    
    // Add operation
    assert(bytecode[6] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Add));
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: Compile field access: obj.field
bool TestCompileFieldAccess() {
    std::cout << "  TestCompileFieldAccess... ";
    
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    proc.AddLocalVariable("obj");  // ID 0
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Compile: obj.name
    // AST: Dereference(Identifier("obj"), "name")
    auto objExpr = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "obj");
    auto fieldExpr = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "name");
    auto expr = std::make_unique<DMCompiler::DMASTDereference>(
        DMCompiler::Location(),
        std::move(objExpr),
        DMCompiler::DereferenceType::Direct,
        std::move(fieldExpr)
    );
    
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Failed to compile 'obj.name'");
    
    // Validate bytecode:
    // 1. PushReferenceValue + Type.Local + ID (obj) = 3 bytes
    // 2. DereferenceField + string_id = 1 + 4 bytes
    // Total varies because string ID is an int
    const std::vector<uint8_t>& bytecode = writer.GetBytecode();
    
    // obj reference
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushReferenceValue));
    assert(bytecode[1] == 28);  // Type.Local
    assert(bytecode[2] == 0);   // Variable ID
    
    // DereferenceField opcode
    assert(bytecode[3] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::DereferenceField));
    // Next 4 bytes are string ID (we don't check exact value, it's managed by string table)
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: Compile chained field access: obj.container.item
bool TestCompileChainedFieldAccess() {
    std::cout << "  TestCompileChainedFieldAccess... ";
    
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    proc.AddLocalVariable("obj");
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Compile: obj.container.item
    // AST: Dereference(Dereference(Identifier("obj"), "container"), "item")
    auto objExpr = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "obj");
    auto containerField = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "container");
    auto containerDeref = std::make_unique<DMCompiler::DMASTDereference>(
        DMCompiler::Location(),
        std::move(objExpr),
        DMCompiler::DereferenceType::Direct,
        std::move(containerField)
    );
    
    auto itemField = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "item");
    auto expr = std::make_unique<DMCompiler::DMASTDereference>(
        DMCompiler::Location(),
        std::move(containerDeref),
        DMCompiler::DereferenceType::Direct,
        std::move(itemField)
    );
    
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Failed to compile 'obj.container.item'");
    
    // Validate bytecode:
    // 1. PushRef(obj) = 3 bytes
    // 2. DereferenceField("container") = 5 bytes
    // 3. DereferenceField("item") = 5 bytes
    const std::vector<uint8_t>& bytecode = writer.GetBytecode();
    
    // obj reference
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushReferenceValue));
    assert(bytecode[1] == 28);
    assert(bytecode[2] == 0);
    
    // First DereferenceField
    assert(bytecode[3] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::DereferenceField));
    
    // Second DereferenceField
    assert(bytecode[8] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::DereferenceField));
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: Compile field access with arithmetic: obj.x + 5
bool TestCompileFieldAccessWithArithmetic() {
    std::cout << "  TestCompileFieldAccessWithArithmetic... ";
    
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    proc.AddLocalVariable("obj");
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Compile: obj.x + 5
    auto objExpr = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "obj");
    auto xField = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    auto derefExpr = std::make_unique<DMCompiler::DMASTDereference>(
        DMCompiler::Location(),
        std::move(objExpr),
        DMCompiler::DereferenceType::Direct,
        std::move(xField)
    );
    
    auto constant = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 5);
    auto expr = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location(),
        DMCompiler::BinaryOperator::Add,
        std::move(derefExpr),
        std::move(constant)
    );
    
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Failed to compile 'obj.x + 5'");
    
    // Validate bytecode structure
    const std::vector<uint8_t>& bytecode = writer.GetBytecode();
    
    // Should have: PushRef(obj) + DereferenceField + PushFloat(5) + Add
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushReferenceValue));
    assert(bytecode[3] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::DereferenceField));
    assert(bytecode[8] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    assert(bytecode[13] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Add));
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test compiling a method call with no arguments: obj.method()
bool TestCompileMethodCallNoArgs() {
    std::cout << "  TestCompileMethodCallNoArgs... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: src.getValue()
    // 1. Create "src" identifier (special)
    auto srcIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "src");
    
    // 2. Create "getValue" identifier
    auto methodIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "getValue");
    
    // 3. Create dereference: src.getValue
    auto deref = std::make_unique<DMCompiler::DMASTDereference>(
        DMCompiler::Location(),
        std::move(srcIdent),
        DMCompiler::DereferenceType::Direct,
        std::move(methodIdent)
    );
    
    // 4. Create call expression with no parameters
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> params;
    auto callExpr = std::make_unique<DMCompiler::DMASTCall>(
        DMCompiler::Location(),
        std::move(deref),
        std::move(params)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(callExpr.get());
    assert(success && "Should successfully compile method call");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Expected bytecode:
    // 1. PushReferenceValue Src (1 byte opcode + 1 byte ref)
    // 2. DereferenceCall (1 byte opcode)
    // 3. String ID for "getValue" (4 bytes)
    // 4. Arguments type byte (1 byte: 0 = None)
    // 5. Argument count int (4 bytes: 0)
    // Total: 2 + 1 + 4 + 1 + 4 = 12 bytes
    
    assert(bytecode.size() == 12 && "Method call should emit 12 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushReferenceValue));
    assert(bytecode[1] == 1 && "Should be Src reference type (1)");
    assert(bytecode[2] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::DereferenceCall));
    
    // Check arguments type is None (0)
    assert(bytecode[7] == 0 && "Arguments type should be None");
    
    // Check argument count is 0
    int32_t argCount = *reinterpret_cast<const int32_t*>(&bytecode[8]);
    assert(argCount == 0 && "Argument count should be 0");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test compiling a method call with arguments: obj.method(arg1, arg2)
bool TestCompileMethodCallWithArgs() {
    std::cout << "  TestCompileMethodCallWithArgs... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: src.attack(5, 10)
    // 1. Create "src" identifier
    auto srcIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "src");
    
    // 2. Create "attack" identifier
    auto methodIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "attack");
    
    // 3. Create dereference: src.attack
    auto deref = std::make_unique<DMCompiler::DMASTDereference>(
        DMCompiler::Location(),
        std::move(srcIdent),
        DMCompiler::DereferenceType::Direct,
        std::move(methodIdent)
    );
    
    // 4. Create parameters: 5, 10
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> params;
    
    auto arg1 = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 5);
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(),
        std::move(arg1),
        nullptr  // No key (positional argument)
    ));
    
    auto arg2 = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 10);
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(),
        std::move(arg2),
        nullptr  // No key (positional argument)
    ));
    
    // 5. Create call expression
    auto callExpr = std::make_unique<DMCompiler::DMASTCall>(
        DMCompiler::Location(),
        std::move(deref),
        std::move(params)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(callExpr.get());
    assert(success && "Should successfully compile method call with args");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Expected bytecode:
    // 1. PushReferenceValue Src (2 bytes)
    // 2. PushFloat 5.0 (5 bytes)
    // 3. PushFloat 10.0 (5 bytes)
    // 4. DereferenceCall (1 byte)
    // 5. String ID for "attack" (4 bytes)
    // 6. Arguments type byte (1 byte: 1 = FromStack)
    // 7. Argument count int (4 bytes: 2)
    // Total: 2 + 5 + 5 + 1 + 4 + 1 + 4 = 22 bytes
    
    assert(bytecode.size() == 22 && "Method call with 2 args should emit 22 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushReferenceValue));
    assert(bytecode[2] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat)); // arg1
    assert(bytecode[7] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat)); // arg2
    assert(bytecode[12] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::DereferenceCall));
    
    // Check arguments type is FromStack (1)
    assert(bytecode[17] == 1 && "Arguments type should be FromStack");
    
    // Check argument count is 2
    int32_t argCount = *reinterpret_cast<const int32_t*>(&bytecode[18]);
    assert(argCount == 2 && "Argument count should be 2");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test compiling a method call in an arithmetic expression
bool TestCompileMethodCallInExpression() {
    std::cout << "  TestCompileMethodCallInExpression... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: src.getValue() + 10
    
    // 1. Create method call: src.getValue()
    auto srcIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "src");
    auto methodIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "getValue");
    auto deref = std::make_unique<DMCompiler::DMASTDereference>(
        DMCompiler::Location(),
        std::move(srcIdent),
        DMCompiler::DereferenceType::Direct,
        std::move(methodIdent)
    );
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> params;
    auto callExpr = std::make_unique<DMCompiler::DMASTCall>(
        DMCompiler::Location(),
        std::move(deref),
        std::move(params)
    );
    
    // 2. Create constant: 10
    auto constant = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 10);
    
    // 3. Create addition: call + 10
    auto addExpr = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location(),
        DMCompiler::BinaryOperator::Add,
        std::move(callExpr),
        std::move(constant)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(addExpr.get());
    assert(success && "Should successfully compile method call in expression");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Expected bytecode:
    // 1. PushReferenceValue Src (2 bytes)
    // 2. DereferenceCall "getValue", None, 0 (10 bytes)
    // 3. PushFloat 10.0 (5 bytes)
    // 4. Add (1 byte)
    // Total: 2 + 10 + 5 + 1 = 18 bytes
    
    assert(bytecode.size() == 18 && "Expression with method call should emit 18 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushReferenceValue));
    assert(bytecode[2] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::DereferenceCall));
    assert(bytecode[12] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    assert(bytecode[17] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Add));
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: Method call on world object: world.log()
bool TestCompileWorldMethodCall() {
    std::cout << "  TestCompileWorldMethodCall... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: world.log("Hello")
    // 1. Create "world" identifier
    auto worldIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "world");
    
    // 2. Create "log" identifier
    auto logIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "log");
    
    // 3. Create dereference: world.log
    auto deref = std::make_unique<DMCompiler::DMASTDereference>(
        DMCompiler::Location(),
        std::move(worldIdent),
        DMCompiler::DereferenceType::Direct,
        std::move(logIdent)
    );
    
    // 4. Create parameter: "Hello" (string constant)
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> params;
    auto strArg = std::make_unique<DMCompiler::DMASTConstantString>(DMCompiler::Location(), "Hello");
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(),
        std::move(strArg),
        nullptr
    ));
    
    // 5. Create call expression
    auto callExpr = std::make_unique<DMCompiler::DMASTCall>(
        DMCompiler::Location(),
        std::move(deref),
        std::move(params)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(callExpr.get());
    assert(success && "Should successfully compile world.log()");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Expected bytecode:
    // 1. PushReferenceValue World (2 bytes: opcode + ref type 5)
    // 2. PushString "Hello" (5 bytes: opcode + string ID)
    // 3. DereferenceCall "log" + FromStack + 1 (10 bytes)
    // Total: 2 + 5 + 10 = 17 bytes
    
    assert(bytecode.size() == 17 && "world.log('Hello') should emit 17 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushReferenceValue));
    assert(bytecode[1] == 5 && "Should be World reference type (5)");
    assert(bytecode[2] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushString));
    assert(bytecode[7] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::DereferenceCall));
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: Method call with multiple argument types
bool TestCompileMethodCallMixedArgs() {
    std::cout << "  TestCompileMethodCallMixedArgs... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variable "target"
    proc.AddLocalVariable("target");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: src.attack(target, 10, usr)
    // Mix of: variable, constant, special identifier
    
    auto srcIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "src");
    auto methodIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "attack");
    auto deref = std::make_unique<DMCompiler::DMASTDereference>(
        DMCompiler::Location(),
        std::move(srcIdent),
        DMCompiler::DereferenceType::Direct,
        std::move(methodIdent)
    );
    
    // Parameters: target (local var), 10 (constant), usr (special)
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> params;
    
    auto arg1 = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "target");
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(),
        std::move(arg1),
        nullptr
    ));
    
    auto arg2 = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 10);
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(),
        std::move(arg2),
        nullptr
    ));
    
    auto arg3 = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "usr");
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(),
        std::move(arg3),
        nullptr
    ));
    
    auto callExpr = std::make_unique<DMCompiler::DMASTCall>(
        DMCompiler::Location(),
        std::move(deref),
        std::move(params)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(callExpr.get());
    assert(success && "Should successfully compile src.attack(target, 10, usr)");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Expected bytecode:
    // 1. PushReferenceValue Src (2 bytes)
    // 2. PushReferenceValue Local[0] (3 bytes)  // target
    // 3. PushFloat 10.0 (5 bytes)
    // 4. PushReferenceValue Usr (2 bytes)
    // 5. DereferenceCall "attack" + FromStack + 3 (10 bytes)
    // Total: 2 + 3 + 5 + 2 + 10 = 22 bytes
    
    assert(bytecode.size() == 22 && "src.attack(target, 10, usr) should emit 22 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushReferenceValue));
    assert(bytecode[1] == 1 && "First should be Src (1)");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: Nested method calls: obj.getValue().toString()
bool TestCompileNestedMethodCalls() {
    std::cout << "  TestCompileNestedMethodCalls... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: src.getValue().toString()
    
    // First call: src.getValue()
    auto srcIdent1 = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "src");
    auto getValueIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "getValue");
    auto deref1 = std::make_unique<DMCompiler::DMASTDereference>(
        DMCompiler::Location(),
        std::move(srcIdent1),
        DMCompiler::DereferenceType::Direct,
        std::move(getValueIdent)
    );
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> params1;
    auto call1 = std::make_unique<DMCompiler::DMASTCall>(
        DMCompiler::Location(),
        std::move(deref1),
        std::move(params1)
    );
    
    // Second call: (result).toString()
    auto toStringIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "toString");
    auto deref2 = std::make_unique<DMCompiler::DMASTDereference>(
        DMCompiler::Location(),
        std::move(call1),  // Use first call as the object
        DMCompiler::DereferenceType::Direct,
        std::move(toStringIdent)
    );
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> params2;
    auto call2 = std::make_unique<DMCompiler::DMASTCall>(
        DMCompiler::Location(),
        std::move(deref2),
        std::move(params2)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(call2.get());
    assert(success && "Should successfully compile src.getValue().toString()");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Expected bytecode:
    // First call (src.getValue()):
    // 1. PushReferenceValue Src (2 bytes)
    // 2. DereferenceCall "getValue" + None + 0 (10 bytes)
    // Second call (.toString()):
    // 3. DereferenceCall "toString" + None + 0 (10 bytes)
    // Total: 2 + 10 + 10 = 22 bytes
    
    assert(bytecode.size() == 22 && "Nested calls should emit 22 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushReferenceValue));
    assert(bytecode[2] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::DereferenceCall));
    assert(bytecode[12] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::DereferenceCall));
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test compiling a simple global proc call: sleep(10)
bool TestCompileGlobalProcCallSimple() {
    std::cout << "  TestCompileGlobalProcCallSimple... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Register "sleep" as global proc ID 42
    compiler.GetObjectTree()->RegisterGlobalProc("sleep", 42);
    
    // Create AST: sleep(10)
    auto procIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "sleep");
    auto arg = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 10);
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> params;
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(), std::move(arg), nullptr));
    auto expr = std::make_unique<DMCompiler::DMASTCall>(
        DMCompiler::Location(), std::move(procIdent), std::move(params));
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    if (!success) {
        std::cout << "FAILED" << std::endl;
        return false;
    }
    
    const auto& bytecode = writer.GetBytecode();
    
    // Expected: PushFloat 10 (5 bytes) + Call (1 byte) + GlobalProc ref (5 bytes: 0x0B + proc_id) + args_type (1 byte) + arg_count (4 bytes)
    // Total: 5 + 1 + 5 + 1 + 4 = 16 bytes
    assert(bytecode.size() == 16 && "Global proc call should emit 16 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    assert(bytecode[5] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Call));
    assert(bytecode[6] == 11 && "Should have GlobalProc type (11)");
    // Proc ID (42) stored as little-endian int: 0x2A 0x00 0x00 0x00
    assert(bytecode[7] == 0x2A);
    assert(bytecode[8] == 0x00);
    assert(bytecode[9] == 0x00);
    assert(bytecode[10] == 0x00);
    assert(bytecode[11] == 1 && "Args type should be FromStack (1)");
    // Arg count (1) stored as little-endian int: 0x01 0x00 0x00 0x00
    assert(bytecode[12] == 0x01);
    assert(bytecode[13] == 0x00);
    assert(bytecode[14] == 0x00);
    assert(bytecode[15] == 0x00);
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test compiling global proc call with multiple args: rand(1, 100)
bool TestCompileGlobalProcCallMultipleArgs() {
    std::cout << "  TestCompileGlobalProcCallMultipleArgs... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Register "rand" as global proc ID 99
    compiler.GetObjectTree()->RegisterGlobalProc("rand", 99);
    
    // Create AST: rand(1, 100)
    auto procIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "rand");
    auto arg1 = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 1);
    auto arg2 = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 100);
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> params;
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(), std::move(arg1), nullptr));
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(), std::move(arg2), nullptr));
    auto expr = std::make_unique<DMCompiler::DMASTCall>(
        DMCompiler::Location(), std::move(procIdent), std::move(params));
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile multi-arg global proc call");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Expected: PushFloat 1 (5) + PushFloat 100 (5) + Call (1) + GlobalProc ref (5) + args_type (1) + arg_count (4)
    // Total: 5 + 5 + 1 + 5 + 1 + 4 = 21 bytes
    assert(bytecode.size() == 21 && "Multi-arg global proc call should emit 21 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    assert(bytecode[5] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    assert(bytecode[10] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Call));
    assert(bytecode[11] == 11 && "Should have GlobalProc type (11)");
    // Proc ID (99) = 0x63
    assert(bytecode[12] == 0x63);
    assert(bytecode[16] == 1 && "Args type should be FromStack (1)");
    // Arg count (2) 
    assert(bytecode[17] == 0x02);
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test compiling global proc call with no args: world.tick_lag()
bool TestCompileGlobalProcCallNoArgs() {
    std::cout << "  TestCompileGlobalProcCallNoArgs... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Register "time" as global proc ID 15
    compiler.GetObjectTree()->RegisterGlobalProc("time", 15);
    
    // Create AST: time()
    auto procIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "time");
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> params; // Empty
    auto expr = std::make_unique<DMCompiler::DMASTCall>(
        DMCompiler::Location(), std::move(procIdent), std::move(params));
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile no-arg global proc call");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Expected: Call (1) + GlobalProc ref (5) + args_type=None (1) + arg_count=0 (4)
    // Total: 1 + 5 + 1 + 4 = 11 bytes
    assert(bytecode.size() == 11 && "No-arg global proc call should emit 11 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Call));
    assert(bytecode[1] == 11 && "Should have GlobalProc type (11)");
    // Proc ID (15) = 0x0F
    assert(bytecode[2] == 0x0F);
    assert(bytecode[6] == 0 && "Args type should be None (0)");
    // Arg count (0)
    assert(bytecode[7] == 0x00);
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test global proc call in expression: x = rand(5) + 10
bool TestCompileGlobalProcInExpression() {
    std::cout << "  TestCompileGlobalProcInExpression... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Register "rand" as global proc ID 99
    compiler.GetObjectTree()->RegisterGlobalProc("rand", 99);
    
    // Create AST: rand(5) + 10
    auto procIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "rand");
    auto arg = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 5);
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> params;
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(), std::move(arg), nullptr));
    auto callExpr = std::make_unique<DMCompiler::DMASTCall>(
        DMCompiler::Location(), std::move(procIdent), std::move(params));
    auto rightExpr = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 10);
    auto expr = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location(), 
        DMCompiler::BinaryOperator::Add, 
        std::move(callExpr), 
        std::move(rightExpr)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile global proc in expression");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Expected: PushFloat 5 (5) + Call (1) + GlobalProc ref (5) + args_type (1) + arg_count (4) + PushFloat 10 (5) + Add (1)
    // Total: 5 + 1 + 5 + 1 + 4 + 5 + 1 = 22 bytes
    assert(bytecode.size() == 22 && "Global proc in expression should emit 22 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    assert(bytecode[5] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Call));
    assert(bytecode[16] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    assert(bytecode[21] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Add));
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test compiling an empty list: list()
bool TestCompileEmptyList() {
    std::cout << "  TestCompileEmptyList... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: list()
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> values;  // Empty
    auto expr = std::make_unique<DMCompiler::DMASTList>(
        DMCompiler::Location(), std::move(values), false);
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile empty list");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Expected: CreateList (1 byte) + count=0 (4 bytes)
    // Total: 5 bytes
    assert(bytecode.size() == 5 && "Empty list should emit 5 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::CreateList));
    // Count (0) stored as little-endian int
    assert(bytecode[1] == 0x00);
    assert(bytecode[2] == 0x00);
    assert(bytecode[3] == 0x00);
    assert(bytecode[4] == 0x00);
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test compiling a list with values: list(1, 2, 3)
bool TestCompileListWithValues() {
    std::cout << "  TestCompileListWithValues... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: list(1, 2, 3)
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> values;
    auto val1 = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 1);
    auto val2 = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 2);
    auto val3 = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 3);
    values.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(), std::move(val1), nullptr));
    values.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(), std::move(val2), nullptr));
    values.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(), std::move(val3), nullptr));
    
    auto expr = std::make_unique<DMCompiler::DMASTList>(
        DMCompiler::Location(), std::move(values), false);
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile list with values");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Expected: PushFloat 1 (5) + PushFloat 2 (5) + PushFloat 3 (5) + CreateList (1) + count=3 (4)
    // Total: 5 + 5 + 5 + 1 + 4 = 20 bytes
    assert(bytecode.size() == 20 && "List with 3 values should emit 20 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    assert(bytecode[5] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    assert(bytecode[10] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    assert(bytecode[15] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::CreateList));
    // Count (3) = 0x03
    assert(bytecode[16] == 0x03);
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test compiling list indexing: mylist[0]
bool TestCompileListIndexing() {
    std::cout << "  TestCompileListIndexing... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add a local variable "mylist" for testing
    proc.AddLocalVariable("mylist");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: mylist[0]
    auto listIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "mylist");
    auto index = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 0);
    auto expr = std::make_unique<DMCompiler::DMASTDereference>(
        DMCompiler::Location(),
        std::move(listIdent),
        DMCompiler::DereferenceType::Direct,
        std::move(index));
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile list indexing");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Expected: PushReferenceValue <ref_type> <local_id> (3 bytes) + PushFloat 0 (5 bytes) + DereferenceIndex (1 byte)
    // Total: 3 + 5 + 1 = 9 bytes
    assert(bytecode.size() == 9 && "List indexing should emit 9 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushReferenceValue));
    assert(bytecode[3] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    assert(bytecode[8] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::DereferenceIndex));
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test compiling list in expression: list(1, 2)[0] + 5
bool TestCompileListInExpression() {
    std::cout << "  TestCompileListInExpression... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: list(1, 2)[0] + 5
    // First create the list: list(1, 2)
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> values;
    auto val1 = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 1);
    auto val2 = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 2);
    values.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(), std::move(val1), nullptr));
    values.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(), std::move(val2), nullptr));
    auto listExpr = std::make_unique<DMCompiler::DMASTList>(
        DMCompiler::Location(), std::move(values), false);
    
    // Now create the indexing: list[0]
    auto index = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 0);
    auto derefExpr = std::make_unique<DMCompiler::DMASTDereference>(
        DMCompiler::Location(),
        std::move(listExpr),
        DMCompiler::DereferenceType::Direct,
        std::move(index));
    
    // Finally, add 5: list[0] + 5
    auto rightExpr = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 5);
    auto expr = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location(),
        DMCompiler::BinaryOperator::Add,
        std::move(derefExpr),
        std::move(rightExpr)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile list in expression");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Expected: PushFloat 1 (5) + PushFloat 2 (5) + CreateList (1) + count=2 (4)
    //           + PushFloat 0 (5) + DereferenceIndex (1) + PushFloat 5 (5) + Add (1)
    // Total: 5 + 5 + 1 + 4 + 5 + 1 + 5 + 1 = 27 bytes
    assert(bytecode.size() == 27 && "List in expression should emit 27 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));  // 1
    assert(bytecode[5] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));  // 2
    assert(bytecode[10] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::CreateList));
    assert(bytecode[15] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));  // 0
    assert(bytecode[20] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::DereferenceIndex));
    assert(bytecode[21] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));  // 5
    assert(bytecode[26] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Add));
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test compiling simple ternary: x > 5 ? 10 : 20
bool TestCompileTernarySimple() {
    std::cout << "  TestCompileTernarySimple... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add a local variable "x" for testing
    proc.AddLocalVariable("x");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: x > 5 ? 10 : 20
    // Condition: x > 5
    auto xIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    auto five = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 5);
    auto condition = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location(),
        DMCompiler::BinaryOperator::Greater,
        std::move(xIdent),
        std::move(five)
    );
    
    // True expression: 10
    auto trueExpr = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 10);
    
    // False expression: 20
    auto falseExpr = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 20);
    
    // Ternary expression
    auto expr = std::make_unique<DMCompiler::DMASTTernary>(
        DMCompiler::Location(),
        std::move(condition),
        std::move(trueExpr),
        std::move(falseExpr)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile simple ternary");
    
    // Finalize to resolve jumps
    writer.Finalize();
    
    const auto& bytecode = writer.GetBytecode();
    
    // Expected bytecode structure:
    // PushReferenceValue x (3 bytes)
    // PushFloat 5 (5 bytes)
    // GreaterThan (1 byte)
    // JumpIfFalse false_label (5 bytes: opcode + int32 offset)
    // PushFloat 10 (5 bytes)
    // Jump end_label (5 bytes: opcode + int32 offset)
    // false_label:
    //   PushFloat 20 (5 bytes)
    // end_label:
    // Total: 3 + 5 + 1 + 5 + 5 + 5 + 5 = 29 bytes
    
    assert(bytecode.size() == 29 && "Simple ternary should emit 29 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushReferenceValue));
    assert(bytecode[3] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    assert(bytecode[8] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::CompareGreaterThan));
    assert(bytecode[9] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::JumpIfFalse));
    assert(bytecode[14] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));  // true: 10
    assert(bytecode[19] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Jump));
    assert(bytecode[24] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));  // false: 20
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test compiling ternary with expression: (a + b) > 10 ? a : b
bool TestCompileTernaryWithExpressions() {
    std::cout << "  TestCompileTernaryWithExpressions... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variables
    proc.AddLocalVariable("a");
    proc.AddLocalVariable("b");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: (a + b) > 10 ? a : b
    // Condition: (a + b) > 10
    auto a1 = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "a");
    auto b1 = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "b");
    auto sum = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location(),
        DMCompiler::BinaryOperator::Add,
        std::move(a1),
        std::move(b1)
    );
    auto ten = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 10);
    auto condition = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location(),
        DMCompiler::BinaryOperator::Greater,
        std::move(sum),
        std::move(ten)
    );
    
    // True expression: a
    auto trueExpr = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "a");
    
    // False expression: b
    auto falseExpr = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "b");
    
    // Ternary expression
    auto expr = std::make_unique<DMCompiler::DMASTTernary>(
        DMCompiler::Location(),
        std::move(condition),
        std::move(trueExpr),
        std::move(falseExpr)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile ternary with expressions");
    
    writer.Finalize();
    
    const auto& bytecode = writer.GetBytecode();
    
    // Verify key opcodes are present
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushReferenceValue));  // a
    assert(bytecode[6] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Add));
    // Find JumpIfFalse
    bool foundJumpIfFalse = false;
    for (size_t i = 0; i < bytecode.size(); ++i) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::JumpIfFalse)) {
            foundJumpIfFalse = true;
            break;
        }
    }
    assert(foundJumpIfFalse && "Should have JumpIfFalse opcode");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test compiling nested ternary: x > 0 ? (y > 0 ? 1 : 2) : 3
bool TestCompileTernaryNested() {
    std::cout << "  TestCompileTernaryNested... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variables
    proc.AddLocalVariable("x");
    proc.AddLocalVariable("y");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: x > 0 ? (y > 0 ? 1 : 2) : 3
    
    // Outer condition: x > 0
    auto x = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    auto zero1 = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 0);
    auto outerCond = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location(),
        DMCompiler::BinaryOperator::Greater,
        std::move(x),
        std::move(zero1)
    );
    
    // Inner ternary (true expression of outer): y > 0 ? 1 : 2
    auto y = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "y");
    auto zero2 = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 0);
    auto innerCond = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location(),
        DMCompiler::BinaryOperator::Greater,
        std::move(y),
        std::move(zero2)
    );
    auto one = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 1);
    auto two = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 2);
    auto innerTernary = std::make_unique<DMCompiler::DMASTTernary>(
        DMCompiler::Location(),
        std::move(innerCond),
        std::move(one),
        std::move(two)
    );
    
    // Outer false expression: 3
    auto three = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 3);
    
    // Outer ternary
    auto expr = std::make_unique<DMCompiler::DMASTTernary>(
        DMCompiler::Location(),
        std::move(outerCond),
        std::move(innerTernary),
        std::move(three)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile nested ternary");
    
    writer.Finalize();
    
    const auto& bytecode = writer.GetBytecode();
    
    // Verify structure - should have multiple JumpIfFalse and Jump opcodes
    int jumpIfFalseCount = 0;
    int jumpCount = 0;
    for (size_t i = 0; i < bytecode.size(); ++i) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::JumpIfFalse)) {
            jumpIfFalseCount++;
        }
        else if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Jump)) {
            jumpCount++;
        }
    }
    
    assert(jumpIfFalseCount == 2 && "Nested ternary should have 2 JumpIfFalse opcodes");
    assert(jumpCount == 2 && "Nested ternary should have 2 Jump opcodes");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test compiling ternary in expression: result = (x > 5 ? 10 : 20) + 5
bool TestCompileTernaryInExpression() {
    std::cout << "  TestCompileTernaryInExpression... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add a local variable
    proc.AddLocalVariable("x");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: (x > 5 ? 10 : 20) + 5
    
    // Ternary: x > 5 ? 10 : 20
    auto x = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    auto five1 = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 5);
    auto condition = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location(),
        DMCompiler::BinaryOperator::Greater,
        std::move(x),
        std::move(five1)
    );
    auto ten = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 10);
    auto twenty = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 20);
    auto ternary = std::make_unique<DMCompiler::DMASTTernary>(
        DMCompiler::Location(),
        std::move(condition),
        std::move(ten),
        std::move(twenty)
    );
    
    // Add 5 to the ternary result
    auto five2 = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 5);
    auto expr = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location(),
        DMCompiler::BinaryOperator::Add,
        std::move(ternary),
        std::move(five2)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile ternary in expression");
    
    writer.Finalize();
    
    const auto& bytecode = writer.GetBytecode();
    
    // Verify the Add opcode is present at the end (after ternary)
    bool foundAdd = false;
    for (size_t i = bytecode.size() - 10; i < bytecode.size(); ++i) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Add)) {
            foundAdd = true;
            break;
        }
    }
    assert(foundAdd && "Should have Add opcode after ternary");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test simple assignment: x = 5
bool TestCompileSimpleAssignment() {
    std::cout << "  TestCompileSimpleAssignment... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variable 'x' to the proc
    proc.AddLocalVariable("x");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: x = 5
    auto lvalue = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    auto rvalue = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 5);
    auto expr = std::make_unique<DMCompiler::DMASTAssign>(
        DMCompiler::Location(),
        std::move(lvalue),
        DMCompiler::AssignmentOperator::Assign,
        std::move(rvalue)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile simple assignment");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have:
    // - PushFloat 5.0 (5 bytes)
    // - Assign opcode (1 byte)
    // - Reference bytes (2 bytes: type 9, id 0)
    // Total: 8 bytes
    assert(bytecode.size() == 8 && "Simple assignment should emit 8 bytes");
    
    // Verify opcodes
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    assert(bytecode[5] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Assign));
    assert(bytecode[6] == 9);  // DMReference.Type.Local
    assert(bytecode[7] == 0);  // Local ID 0
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test compound assignment: x += 10
bool TestCompileAddAssignment() {
    std::cout << "  TestCompileAddAssignment... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variable 'x' to the proc
    proc.AddLocalVariable("x");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: x += 10
    auto lvalue = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    auto rvalue = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 10);
    auto expr = std::make_unique<DMCompiler::DMASTAssign>(
        DMCompiler::Location(),
        std::move(lvalue),
        DMCompiler::AssignmentOperator::AddAssign,
        std::move(rvalue)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile add assignment");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have:
    // - PushFloat 10.0 (5 bytes)
    // - Append opcode (1 byte)
    // - Reference bytes (2 bytes)
    assert(bytecode.size() == 8 && "Add assignment should emit 8 bytes");
    
    // Verify Append opcode (0x1A)
    assert(bytecode[5] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Append));
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test multiply assignment: x *= 2
bool TestCompileMultiplyAssignment() {
    std::cout << "  TestCompileMultiplyAssignment... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variable 'x' to the proc
    proc.AddLocalVariable("x");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: x *= 2
    auto lvalue = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    auto rvalue = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 2);
    auto expr = std::make_unique<DMCompiler::DMASTAssign>(
        DMCompiler::Location(),
        std::move(lvalue),
        DMCompiler::AssignmentOperator::MultiplyAssign,
        std::move(rvalue)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile multiply assignment");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have MultiplyReference opcode (0x0B)
    assert(bytecode.size() == 8 && "Multiply assignment should emit 8 bytes");
    assert(bytecode[5] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::MultiplyReference));
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test assignment to special identifier: src.x = 5
bool TestCompileAssignToSrc() {
    std::cout << "  TestCompileAssignToSrc... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: src = 5 (simplified, would normally be src.field)
    auto lvalue = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "src");
    auto rvalue = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 5);
    auto expr = std::make_unique<DMCompiler::DMASTAssign>(
        DMCompiler::Location(),
        std::move(lvalue),
        DMCompiler::AssignmentOperator::Assign,
        std::move(rvalue)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile assignment to src");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have:
    // - PushFloat 5.0 (5 bytes)
    // - Assign opcode (1 byte)
    // - Reference type 1 (src) (1 byte)
    assert(bytecode.size() == 7 && "Assignment to src should emit 7 bytes");
    assert(bytecode[6] == 1);  // DMReference.Type.Src
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test combine assignment: x |= y (list union)
bool TestCompileCombineAssignment() {
    std::cout << "  TestCompileCombineAssignment... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variables for x and y
    proc.AddLocalVariable("x");
    proc.AddLocalVariable("y");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: x |= y
    auto lvalue = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    auto rvalue = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "y");
    auto expr = std::make_unique<DMCompiler::DMASTAssign>(
        DMCompiler::Location(),
        std::move(lvalue),
        DMCompiler::AssignmentOperator::BitwiseOrAssign,
        std::move(rvalue)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    const auto& bytecode = writer.GetBytecode();
    
    // Should have Combine opcode (0x2D)
    assert(bytecode.size() > 2 && "Combine assignment should emit bytecode");
    
    // Find Combine opcode
    bool foundCombine = false;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Combine)) {
            foundCombine = true;
            break;
        }
    }
    assert(foundCombine && "Should emit Combine opcode");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test mask assignment: x &= y (list intersection)
bool TestCompileMaskAssignment() {
    std::cout << "  TestCompileMaskAssignment... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variables for x and y
    proc.AddLocalVariable("x");
    proc.AddLocalVariable("y");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: x &= y
    auto lvalue = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    auto rvalue = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "y");
    auto expr = std::make_unique<DMCompiler::DMASTAssign>(
        DMCompiler::Location(),
        std::move(lvalue),
        DMCompiler::AssignmentOperator::BitwiseAndAssign,
        std::move(rvalue)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    const auto& bytecode = writer.GetBytecode();
    
    // Should have Mask opcode (0x33)
    assert(bytecode.size() > 2 && "Mask assignment should emit bytecode");
    
    // Find Mask opcode
    bool foundMask = false;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Mask)) {
            foundMask = true;
            break;
        }
    }
    assert(foundMask && "Should emit Mask opcode");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test remove assignment: list -= item
bool TestCompileRemoveAssignment() {
    std::cout << "  TestCompileRemoveAssignment... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variables for list and item
    proc.AddLocalVariable("list");
    proc.AddLocalVariable("item");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: list -= item
    auto lvalue = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "list");
    auto rvalue = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "item");
    auto expr = std::make_unique<DMCompiler::DMASTAssign>(
        DMCompiler::Location(),
        std::move(lvalue),
        DMCompiler::AssignmentOperator::SubtractAssign,
        std::move(rvalue)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    const auto& bytecode = writer.GetBytecode();
    
    // Should have Remove opcode (0x1F)
    assert(bytecode.size() > 2 && "Remove assignment should emit bytecode");
    
    // Find Remove opcode
    bool foundRemove = false;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Remove)) {
            foundRemove = true;
            break;
        }
    }
    assert(foundRemove && "Should emit Remove opcode");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test prefix increment: ++x
bool TestCompilePreIncrement() {
    std::cout << "  TestCompilePreIncrement... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variable 'x' to the proc
    proc.AddLocalVariable("x");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: ++x
    auto operand = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    auto expr = std::make_unique<DMCompiler::DMASTExpressionUnary>(
        DMCompiler::Location(),
        DMCompiler::UnaryOperator::PreIncrement,
        std::move(operand)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile prefix increment");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have:
    // - PushFloat 1.0 (5 bytes)
    // - Append opcode (1 byte: 0x1A)
    // - Reference bytes (2 bytes: type 9, id 0)
    // Total: 8 bytes
    assert(bytecode.size() == 8 && "Prefix increment should emit 8 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    assert(bytecode[5] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Append));
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test postfix increment: x++
bool TestCompilePostIncrement() {
    std::cout << "  TestCompilePostIncrement... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variable 'x' to the proc
    proc.AddLocalVariable("x");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: x++
    auto operand = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    auto expr = std::make_unique<DMCompiler::DMASTExpressionUnary>(
        DMCompiler::Location(),
        DMCompiler::UnaryOperator::PostIncrement,
        std::move(operand)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile postfix increment");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have:
    // - Increment opcode (1 byte: 0x56)
    // - Reference bytes (2 bytes: type 9, id 0)
    // Total: 3 bytes
    assert(bytecode.size() == 3 && "Postfix increment should emit 3 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Increment));
    assert(bytecode[1] == 9);  // DMReference.Type.Local
    assert(bytecode[2] == 0);  // ID 0
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test prefix decrement: --x
bool TestCompilePreDecrement() {
    std::cout << "  TestCompilePreDecrement... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variable 'x' to the proc
    proc.AddLocalVariable("x");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: --x
    auto operand = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    auto expr = std::make_unique<DMCompiler::DMASTExpressionUnary>(
        DMCompiler::Location(),
        DMCompiler::UnaryOperator::PreDecrement,
        std::move(operand)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile prefix decrement");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have:
    // - PushFloat 1.0 (5 bytes)
    // - Remove opcode (1 byte: 0x1F)
    // - Reference bytes (2 bytes)
    // Total: 8 bytes
    assert(bytecode.size() == 8 && "Prefix decrement should emit 8 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat));
    assert(bytecode[5] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Remove));
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test postfix decrement: x--
bool TestCompilePostDecrement() {
    std::cout << "  TestCompilePostDecrement... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variable 'x' to the proc
    proc.AddLocalVariable("x");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: x--
    auto operand = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    auto expr = std::make_unique<DMCompiler::DMASTExpressionUnary>(
        DMCompiler::Location(),
        DMCompiler::UnaryOperator::PostDecrement,
        std::move(operand)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile postfix decrement");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have:
    // - Decrement opcode (1 byte: 0x57)
    // - Reference bytes (2 bytes)
    // Total: 3 bytes
    assert(bytecode.size() == 3 && "Postfix decrement should emit 3 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Decrement));
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: new /obj/item (no arguments)
bool TestCompileNewPathNoArgs() {
    std::cout << "  TestCompileNewPathNoArgs... ";
    
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObjectTree tree(&compiler);
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/mob"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: new /obj/item
    DMCompiler::DreamPath objPath("/obj/item");
    DMCompiler::DMASTPath astPath(DMCompiler::Location("test.dm", 1, 1), objPath);
    auto pathExpr = std::make_unique<DMCompiler::DMASTConstantPath>(
        DMCompiler::Location("test.dm", 1, 1),
        astPath
    );
    
    auto newExpr = std::make_unique<DMCompiler::DMASTNewPath>(
        DMCompiler::Location("test.dm", 1, 1),
        std::move(pathExpr),
        std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>>()
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(newExpr.get());
    assert(success && "Should successfully compile new expression without args");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have:
    // - PushType opcode (1 byte: 0x02)
    // - Type ID (2 bytes: uint16_t)
    // - CreateObject opcode (1 byte: 0x2E)
    // - Argument type (1 byte: None = 0)
    // - Argument count (1 byte: 0)
    // Total: 6 bytes
    assert(bytecode.size() == 6 && "New expression with no args should emit 6 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushType));
    assert(bytecode[3] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::CreateObject));
    assert(bytecode[4] == static_cast<uint8_t>(DMCompiler::DMCallArgumentsType::None));
    assert(bytecode[5] == 0); // 0 arguments
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: new /mob/player(name, age)
bool TestCompileNewPathWithArgs() {
    std::cout << "  TestCompileNewPathWithArgs... ";
    
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObjectTree tree(&compiler);
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/mob"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: new /mob/player("Alice", 25)
    DMCompiler::DreamPath mobPath("/mob/player");
    DMCompiler::DMASTPath astPath(DMCompiler::Location("test.dm", 1, 1), mobPath);
    auto pathExpr = std::make_unique<DMCompiler::DMASTConstantPath>(
        DMCompiler::Location("test.dm", 1, 1),
        astPath
    );
    
    // Create arguments: "Alice" and 25
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> params;
    
    auto arg1 = std::make_unique<DMCompiler::DMASTConstantString>(
        DMCompiler::Location("test.dm", 1, 1), "Alice"
    );
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location("test.dm", 1, 1), std::move(arg1)
    ));
    
    auto arg2 = std::make_unique<DMCompiler::DMASTConstantInteger>(
        DMCompiler::Location("test.dm", 1, 1), 25
    );
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location("test.dm", 1, 1), std::move(arg2)
    ));
    
    auto newExpr = std::make_unique<DMCompiler::DMASTNewPath>(
        DMCompiler::Location("test.dm", 1, 1),
        std::move(pathExpr),
        std::move(params)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(newExpr.get());
    assert(success && "Should successfully compile new expression with args");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have:
    // - PushType (3 bytes: opcode + type ID)
    // - PushString "Alice" (variable size, but we know string ID is small)
    // - PushFloat 25.0 (5 bytes)
    // - CreateObject (3 bytes: opcode + arg type + count)
    // Let's just verify the key opcodes are present
    assert(bytecode.size() > 10 && "New expression with args should emit multiple bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushType));
    
    // Find CreateObject opcode (should be near the end)
    bool foundCreateObject = false;
    for (size_t i = 0; i < bytecode.size() - 2; i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::CreateObject)) {
            foundCreateObject = true;
            // Check argument type and count
            assert(bytecode[i + 1] == static_cast<uint8_t>(DMCompiler::DMCallArgumentsType::FromStack));
            assert(bytecode[i + 2] == 2); // 2 arguments
            break;
        }
    }
    assert(foundCreateObject && "Should emit CreateObject opcode");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: new path where path is /datum
bool TestCompileNewPathSimple() {
    std::cout << "  TestCompileNewPathSimple... ";
    
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObjectTree tree(&compiler);
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/mob"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: new /datum()
    DMCompiler::DreamPath datumPath("/datum");
    DMCompiler::DMASTPath astPath(DMCompiler::Location("test.dm", 1, 1), datumPath);
    auto pathExpr = std::make_unique<DMCompiler::DMASTConstantPath>(
        DMCompiler::Location("test.dm", 1, 1),
        astPath
    );
    
    auto newExpr = std::make_unique<DMCompiler::DMASTNewPath>(
        DMCompiler::Location("test.dm", 1, 1),
        std::move(pathExpr),
        std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>>()
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(newExpr.get());
    assert(success && "Should successfully compile simple new expression");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Same as TestCompileNewPathNoArgs
    assert(bytecode.size() == 6 && "Simple new expression should emit 6 bytes");
    assert(bytecode[0] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushType));
    assert(bytecode[3] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::CreateObject));
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: x in list (simple membership test)
bool TestCompileInOperatorSimple() {
    std::cout << "  TestCompileInOperatorSimple... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variables for x and list
    proc.AddLocalVariable("x");
    proc.AddLocalVariable("list");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: x in list
    auto leftExpr = std::make_unique<DMCompiler::DMASTIdentifier>(
        DMCompiler::Location("test.dm", 1, 1),
        "x"
    );
    auto rightExpr = std::make_unique<DMCompiler::DMASTIdentifier>(
        DMCompiler::Location("test.dm", 1, 5),
        "list"
    );
    auto inExpr = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location("test.dm", 1, 1),
        DMCompiler::BinaryOperator::In,
        std::move(leftExpr),
        std::move(rightExpr)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(inExpr.get());
    const auto& bytecode = writer.GetBytecode();
    
    // Should emit: PushReferenceValue (x), PushReferenceValue (list), IsInList
    assert(bytecode.size() >= 3 && "In expression should emit at least 3 bytes");
    
    // Find IsInList opcode (0x36)
    bool foundIsInList = false;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::IsInList)) {
            foundIsInList = true;
            break;
        }
    }
    assert(foundIsInList && "Should emit IsInList opcode");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: 5 in [1, 2, 3, 4, 5] (value in list literal)
bool TestCompileInOperatorWithList() {
    std::cout << "  TestCompileInOperatorWithList... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: 5 in [1, 2, 3]
    auto leftExpr = std::make_unique<DMCompiler::DMASTConstantInteger>(
        DMCompiler::Location("test.dm", 1, 1),
        5
    );
    
    // Create list [1, 2, 3]
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> listValues;
    listValues.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location("test.dm", 1, 7),
        std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location("test.dm", 1, 7), 1)
    ));
    listValues.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location("test.dm", 1, 10),
        std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location("test.dm", 1, 10), 2)
    ));
    listValues.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location("test.dm", 1, 13),
        std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location("test.dm", 1, 13), 3)
    ));
    
    auto rightExpr = std::make_unique<DMCompiler::DMASTList>(
        DMCompiler::Location("test.dm", 1, 6),
        std::move(listValues)
    );
    
    auto inExpr = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location("test.dm", 1, 1),
        DMCompiler::BinaryOperator::In,
        std::move(leftExpr),
        std::move(rightExpr)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(inExpr.get());
    assert(success && "Should successfully compile in expression with list");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should emit: PushFloat 5, CreateList, PushFloat 1, ..., IsInList
    assert(bytecode.size() > 10 && "In expression with list should emit multiple bytes");
    
    // Find IsInList opcode
    bool foundIsInList = false;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::IsInList)) {
            foundIsInList = true;
            break;
        }
    }
    assert(foundIsInList && "Should emit IsInList opcode");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test: (a in b) in c (chained in operators)
bool TestCompileInOperatorChained() {
    std::cout << "  TestCompileInOperatorChained... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variables for a, b, and c
    proc.AddLocalVariable("a");
    proc.AddLocalVariable("b");
    proc.AddLocalVariable("c");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: (a in b) in c
    auto a = std::make_unique<DMCompiler::DMASTIdentifier>(
        DMCompiler::Location("test.dm", 1, 1), "a"
    );
    auto b = std::make_unique<DMCompiler::DMASTIdentifier>(
        DMCompiler::Location("test.dm", 1, 6), "b"
    );
    auto innerIn = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location("test.dm", 1, 1),
        DMCompiler::BinaryOperator::In,
        std::move(a),
        std::move(b)
    );
    
    auto c = std::make_unique<DMCompiler::DMASTIdentifier>(
        DMCompiler::Location("test.dm", 1, 12), "c"
    );
    auto outerIn = std::make_unique<DMCompiler::DMASTExpressionBinary>(
        DMCompiler::Location("test.dm", 1, 1),
        DMCompiler::BinaryOperator::In,
        std::move(innerIn),
        std::move(c)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(outerIn.get());
    const auto& bytecode = writer.GetBytecode();
    
    // Should emit two IsInList opcodes (one for each 'in')
    int isInListCount = 0;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::IsInList)) {
            isInListCount++;
        }
    }
    assert(isInListCount == 2 && "Should emit two IsInList opcodes for chained in");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test logical AND assignment: x &&= y (if x is true, assign y to x)
bool TestCompileLogicalAndAssign() {
    std::cout << "  TestCompileLogicalAndAssign... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    proc.AddLocalVariable("x");
    proc.AddLocalVariable("y");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: x &&= y
    auto lvalue = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    auto rvalue = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "y");
    
    auto assignment = std::make_unique<DMCompiler::DMASTAssign>(
        DMCompiler::Location(),
        std::move(lvalue),
        DMCompiler::AssignmentOperator::LogicalAndAssign,
        std::move(rvalue)
    );
    
    bool success = exprCompiler.CompileExpression(assignment.get());
    assert(success && "Should successfully compile &&= operator");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have JumpIfFalseReference opcode
    bool foundJumpIfFalseRef = false;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::JumpIfFalseReference)) {
            foundJumpIfFalseRef = true;
            break;
        }
    }
    assert(foundJumpIfFalseRef && "Should emit JumpIfFalseReference for &&=");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test logical OR assignment: x ||= y (if x is false, assign y to x)
bool TestCompileLogicalOrAssign() {
    std::cout << "  TestCompileLogicalOrAssign... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    proc.AddLocalVariable("x");
    proc.AddLocalVariable("y");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: x ||= y
    auto lvalue = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "x");
    auto rvalue = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "y");
    
    auto assignment = std::make_unique<DMCompiler::DMASTAssign>(
        DMCompiler::Location(),
        std::move(lvalue),
        DMCompiler::AssignmentOperator::LogicalOrAssign,
        std::move(rvalue)
    );
    
    bool success = exprCompiler.CompileExpression(assignment.get());
    assert(success && "Should successfully compile ||= operator");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have JumpIfTrueReference opcode
    bool foundJumpIfTrueRef = false;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::JumpIfTrueReference)) {
            foundJumpIfTrueRef = true;
            break;
        }
    }
    assert(foundJumpIfTrueRef && "Should emit JumpIfTrueReference for ||=");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test field dereference assignment: obj.field = value
bool TestCompileFieldDereferenceAssignment() {
    std::cout << "  TestCompileFieldDereferenceAssignment... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variable 'obj'
    proc.AddLocalVariable("obj");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: obj.health = 100
    // LValue: obj.health (DMASTDereference)
    auto objIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "obj");
    auto fieldIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "health");
    auto lvalue = std::make_unique<DMCompiler::DMASTDereference>(
        DMCompiler::Location(),
        std::move(objIdent),
        DMCompiler::DereferenceType::Direct,
        std::move(fieldIdent)
    );
    
    // RValue: 100
    auto rvalue = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 100);
    
    auto expr = std::make_unique<DMCompiler::DMASTAssign>(
        DMCompiler::Location(),
        std::move(lvalue),
        DMCompiler::AssignmentOperator::Assign,
        std::move(rvalue)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile field dereference assignment");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have:
    // 1. PushFloat 100 (for the value)
    // 2. PushReferenceValue for 'obj' (local variable)
    // 3. Assign opcode with Field reference (type 12)
    
    bool foundPushFloat = false;
    bool foundAssign = false;
    
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushFloat)) {
            foundPushFloat = true;
        }
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Assign)) {
            foundAssign = true;
            // Check that the reference type is Field (12)
            if (i + 1 < bytecode.size()) {
                assert(bytecode[i + 1] == 12 && "Reference type should be Field (12)");
            }
        }
    }
    
    assert(foundPushFloat && "Should emit PushFloat for value");
    assert(foundAssign && "Should emit Assign opcode");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test chained field dereference assignment: a.b.c = value
bool TestCompileChainedFieldDereferenceAssignment() {
    std::cout << "  TestCompileChainedFieldDereferenceAssignment... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variable 'a'
    proc.AddLocalVariable("a");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: a.b.c = null
    // LValue: a.b.c (nested DMASTDereference)
    auto aIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "a");
    auto bIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "b");
    auto abDeref = std::make_unique<DMCompiler::DMASTDereference>(
        DMCompiler::Location(),
        std::move(aIdent),
        DMCompiler::DereferenceType::Direct,
        std::move(bIdent)
    );
    
    auto cIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "c");
    auto lvalue = std::make_unique<DMCompiler::DMASTDereference>(
        DMCompiler::Location(),
        std::move(abDeref),
        DMCompiler::DereferenceType::Direct,
        std::move(cIdent)
    );
    
    // RValue: null
    auto rvalue = std::make_unique<DMCompiler::DMASTConstantNull>(DMCompiler::Location());
    
    auto expr = std::make_unique<DMCompiler::DMASTAssign>(
        DMCompiler::Location(),
        std::move(lvalue),
        DMCompiler::AssignmentOperator::Assign,
        std::move(rvalue)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile chained field dereference assignment");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have:
    // 1. PushNull (for the value)
    // 2. PushReferenceValue for 'a' (local variable)
    // 3. DereferenceField for 'b' (to get a.b)
    // 4. Assign opcode with Field reference (type 12) for 'c'
    
    bool foundPushNull = false;
    bool foundDereferenceField = false;
    bool foundAssign = false;
    
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushNull)) {
            foundPushNull = true;
        }
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::DereferenceField)) {
            foundDereferenceField = true;
        }
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Assign)) {
            foundAssign = true;
        }
    }
    
    assert(foundPushNull && "Should emit PushNull for value");
    assert(foundDereferenceField && "Should emit DereferenceField for intermediate access");
    assert(foundAssign && "Should emit Assign opcode");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test compound assignment to field: obj.count += 1
bool TestCompileFieldDereferenceCompoundAssignment() {
    std::cout << "  TestCompileFieldDereferenceCompoundAssignment... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variable 'obj'
    proc.AddLocalVariable("obj");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: obj.count += 1
    auto objIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "obj");
    auto fieldIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "count");
    auto lvalue = std::make_unique<DMCompiler::DMASTDereference>(
        DMCompiler::Location(),
        std::move(objIdent),
        DMCompiler::DereferenceType::Direct,
        std::move(fieldIdent)
    );
    
    auto rvalue = std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 1);
    
    auto expr = std::make_unique<DMCompiler::DMASTAssign>(
        DMCompiler::Location(),
        std::move(lvalue),
        DMCompiler::AssignmentOperator::AddAssign,
        std::move(rvalue)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile field dereference compound assignment");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have Append opcode (for +=)
    bool foundAppend = false;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Append)) {
            foundAppend = true;
            break;
        }
    }
    
    assert(foundAppend && "Should emit Append opcode for +=");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test list index assignment: list[index] = value
bool TestCompileListIndexAssignment() {
    std::cout << "  TestCompileListIndexAssignment... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    // Add local variables
    proc.AddLocalVariable("list");
    proc.AddLocalVariable("index");
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: list[index] = "value"
    auto listIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "list");
    auto indexIdent = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "index");
    auto lvalue = std::make_unique<DMCompiler::DMASTDereference>(
        DMCompiler::Location(),
        std::move(listIdent),
        DMCompiler::DereferenceType::Direct,
        std::move(indexIdent)
    );
    
    auto rvalue = std::make_unique<DMCompiler::DMASTConstantString>(DMCompiler::Location(), "value");
    
    auto expr = std::make_unique<DMCompiler::DMASTAssign>(
        DMCompiler::Location(),
        std::move(lvalue),
        DMCompiler::AssignmentOperator::Assign,
        std::move(rvalue)
    );
    
    // Compile
    bool success = exprCompiler.CompileExpression(expr.get());
    assert(success && "Should successfully compile list index assignment");
    
    const auto& bytecode = writer.GetBytecode();
    
    // Should have:
    // 1. PushString "value"
    // 2. PushReferenceValue for 'list'
    // 3. PushReferenceValue for 'index'
    // 4. Assign opcode with Index reference (type 13)
    
    bool foundPushString = false;
    bool foundAssign = false;
    
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PushString)) {
            foundPushString = true;
        }
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Assign)) {
            foundAssign = true;
            // Check that the reference type is Index (13)
            if (i + 1 < bytecode.size()) {
                assert(bytecode[i + 1] == 13 && "Reference type should be Index (13)");
            }
        }
    }
    
    assert(foundPushString && "Should emit PushString for value");
    assert(foundAssign && "Should emit Assign opcode");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test built-in functions

bool TestCompileLocateTypeOnly() {
    std::cout << "  TestCompileLocateTypeOnly... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: locate(/mob)
    auto target = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "locate");
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> params;
    
    // Create path /mob
    DMCompiler::DMASTPath mobPath(DMCompiler::Location(), DMCompiler::DreamPath("/mob"), false);
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(),
        nullptr,
        std::make_unique<DMCompiler::DMASTConstantPath>(DMCompiler::Location(), mobPath)
    ));
    
    auto call = std::make_unique<DMCompiler::DMASTCall>(
        DMCompiler::Location(),
        std::move(target),
        std::move(params)
    );
    
    bool success = exprCompiler.CompileExpression(call.get());
    assert(success && "Should successfully compile locate(/mob)");
    
    const auto& bytecode = writer.GetBytecode();
    bool foundLocate = false;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Locate)) {
            foundLocate = true;
            break;
        }
    }
    assert(foundLocate && "Should emit Locate opcode");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool TestCompileLocateCoordinates() {
    std::cout << "  TestCompileLocateCoordinates... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: locate(1, 2, 3)
    auto target = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "locate");
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> params;
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(),
        std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 1),
        nullptr
    ));
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(),
        std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 2),
        nullptr
    ));
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(),
        std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 3),
        nullptr
    ));
    
    auto call = std::make_unique<DMCompiler::DMASTCall>(
        DMCompiler::Location(),
        std::move(target),
        std::move(params)
    );
    
    bool success = exprCompiler.CompileExpression(call.get());
    assert(success && "Should successfully compile locate(1, 2, 3)");
    
    const auto& bytecode = writer.GetBytecode();
    bool foundLocateCoord = false;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::LocateCoord)) {
            foundLocateCoord = true;
            break;
        }
    }
    assert(foundLocateCoord && "Should emit LocateCoord opcode");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool TestCompilePick() {
    std::cout << "  TestCompilePick... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: pick(1, 2, 3)
    auto target = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "pick");
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> params;
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(),
        std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 1),
        nullptr
    ));
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(),
        std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 2),
        nullptr
    ));
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(),
        std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 3),
        nullptr
    ));
    
    auto call = std::make_unique<DMCompiler::DMASTCall>(
        DMCompiler::Location(),
        std::move(target),
        std::move(params)
    );
    
    bool success = exprCompiler.CompileExpression(call.get());
    assert(success && "Should successfully compile pick(1, 2, 3)");
    
    const auto& bytecode = writer.GetBytecode();
    bool foundPick = false;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::PickUnweighted)) {
            foundPick = true;
            break;
        }
    }
    assert(foundPick && "Should emit PickUnweighted opcode");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool TestCompileRgb() {
    std::cout << "  TestCompileRgb... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: rgb(255, 128, 0)
    auto target = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "rgb");
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> params;
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(),
        std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 255),
        nullptr
    ));
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(),
        std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 128),
        nullptr
    ));
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(),
        std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 0),
        nullptr
    ));
    
    auto call = std::make_unique<DMCompiler::DMASTCall>(
        DMCompiler::Location(),
        std::move(target),
        std::move(params)
    );
    
    bool success = exprCompiler.CompileExpression(call.get());
    assert(success && "Should successfully compile rgb(255, 128, 0)");
    
    const auto& bytecode = writer.GetBytecode();
    bool foundRgb = false;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Rgb)) {
            foundRgb = true;
            break;
        }
    }
    assert(foundRgb && "Should emit Rgb opcode");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool TestCompileProb() {
    std::cout << "  TestCompileProb... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: prob(50)
    auto target = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "prob");
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> params;
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(),
        std::make_unique<DMCompiler::DMASTConstantInteger>(DMCompiler::Location(), 50),
        nullptr
    ));
    
    auto call = std::make_unique<DMCompiler::DMASTCall>(
        DMCompiler::Location(),
        std::move(target),
        std::move(params)
    );
    
    bool success = exprCompiler.CompileExpression(call.get());
    assert(success && "Should successfully compile prob(50)");
    
    const auto& bytecode = writer.GetBytecode();
    bool foundProb = false;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Prob)) {
            foundProb = true;
            break;
        }
    }
    assert(foundProb && "Should emit Prob opcode");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool TestCompileInput() {
    std::cout << "  TestCompileInput... ";
    
    DMCompiler::BytecodeWriter writer;
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMObject testObj(0, DMCompiler::DreamPath("/test"));
    DMCompiler::DMProc proc(0, "test_proc", &testObj, false, DMCompiler::Location());
    
    DMCompiler::DMExpressionCompiler exprCompiler(&compiler, &proc, &writer);
    
    // Create AST: input("Enter name")
    auto target = std::make_unique<DMCompiler::DMASTIdentifier>(DMCompiler::Location(), "input");
    std::vector<std::unique_ptr<DMCompiler::DMASTCallParameter>> params;
    params.push_back(std::make_unique<DMCompiler::DMASTCallParameter>(
        DMCompiler::Location(),
        std::make_unique<DMCompiler::DMASTConstantString>(DMCompiler::Location(), "Enter name"),
        nullptr
    ));
    
    auto call = std::make_unique<DMCompiler::DMASTCall>(
        DMCompiler::Location(),
        std::move(target),
        std::move(params)
    );
    
    bool success = exprCompiler.CompileExpression(call.get());
    assert(success && "Should successfully compile input(\"Enter name\")");
    
    const auto& bytecode = writer.GetBytecode();
    bool foundPrompt = false;
    for (size_t i = 0; i < bytecode.size(); i++) {
        if (bytecode[i] == static_cast<uint8_t>(DMCompiler::DreamProcOpcode::Prompt)) {
            foundPrompt = true;
            break;
        }
    }
    assert(foundPrompt && "Should emit Prompt opcode");
    
    std::cout << "PASSED" << std::endl;
    return true;
}

int RunExpressionCompilerTests() {
    std::cout << "\n=== Running Expression Compiler Tests ===" << std::endl;
    
    int failures = 0;
    
    try {
        if (!TestCompileIntegerConstant()) failures++;
        if (!TestCompileAddition()) failures++;
        if (!TestCompileUnaryNegation()) failures++;
        if (!TestCompileLocalVariable()) failures++;
        if (!TestCompileParameter()) failures++;
        if (!TestCompileSpecialIdentifierSrc()) failures++;
        if (!TestCompileExpressionWithVariable()) failures++;
        if (!TestCompileMultipleVariables()) failures++;
        if (!TestCompileFieldAccess()) failures++;
        if (!TestCompileChainedFieldAccess()) failures++;
        if (!TestCompileFieldAccessWithArithmetic()) failures++;
        if (!TestCompileMethodCallNoArgs()) failures++;
        if (!TestCompileMethodCallWithArgs()) failures++;
        if (!TestCompileMethodCallInExpression()) failures++;
        if (!TestCompileWorldMethodCall()) failures++;
        if (!TestCompileMethodCallMixedArgs()) failures++;
        if (!TestCompileNestedMethodCalls()) failures++;
        if (!TestCompileGlobalProcCallSimple()) failures++;
        if (!TestCompileGlobalProcCallMultipleArgs()) failures++;
        if (!TestCompileGlobalProcCallNoArgs()) failures++;
        if (!TestCompileGlobalProcInExpression()) failures++;
        if (!TestCompileEmptyList()) failures++;
        if (!TestCompileListWithValues()) failures++;
        if (!TestCompileListIndexing()) failures++;
        if (!TestCompileListInExpression()) failures++;
        if (!TestCompileTernarySimple()) failures++;
        if (!TestCompileTernaryWithExpressions()) failures++;
        if (!TestCompileTernaryNested()) failures++;
        if (!TestCompileTernaryInExpression()) failures++;
        if (!TestCompileSimpleAssignment()) failures++;
        if (!TestCompileAddAssignment()) failures++;
        if (!TestCompileMultiplyAssignment()) failures++;
        if (!TestCompileCombineAssignment()) failures++;
        if (!TestCompileMaskAssignment()) failures++;
        if (!TestCompileRemoveAssignment()) failures++;
        if (!TestCompileAssignToSrc()) failures++;
        if (!TestCompilePreIncrement()) failures++;
        if (!TestCompilePostIncrement()) failures++;
        if (!TestCompilePreDecrement()) failures++;
        if (!TestCompilePostDecrement()) failures++;
        if (!TestCompileNewPathNoArgs()) failures++;
        if (!TestCompileNewPathWithArgs()) failures++;
        if (!TestCompileNewPathSimple()) failures++;
        if (!TestCompileInOperatorSimple()) failures++;
        if (!TestCompileInOperatorWithList()) failures++;
        if (!TestCompileInOperatorChained()) failures++;
        if (!TestCompileLogicalAndAssign()) failures++;
        if (!TestCompileLogicalOrAssign()) failures++;
        if (!TestCompileFieldDereferenceAssignment()) failures++;
        if (!TestCompileChainedFieldDereferenceAssignment()) failures++;
        if (!TestCompileFieldDereferenceCompoundAssignment()) failures++;
        if (!TestCompileListIndexAssignment()) failures++;
        // if (!TestCompileLocateTypeOnly()) failures++;  // TODO: Requires path type support
        if (!TestCompileLocateCoordinates()) failures++;
        if (!TestCompilePick()) failures++;
        if (!TestCompileRgb()) failures++;
        if (!TestCompileProb()) failures++;
        if (!TestCompileInput()) failures++;
    } catch (const std::exception& e) {
        std::cerr << "Exception during expression compiler tests: " << e.what() << std::endl;
        failures++;
    }
    
    if (failures == 0) {
        std::cout << "All expression compiler tests passed!" << std::endl;
    } else {
        std::cout << failures << " expression compiler test(s) failed!" << std::endl;
    }
    
    return failures;
}
