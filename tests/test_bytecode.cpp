#include <iostream>
#include <cassert>
#include "../include/BytecodeWriter.h"
#include "../include/DreamProcOpcode.h"

using namespace DMCompiler;

// Test basic opcode emission
bool TestBasicOpcodes() {
    std::cout << "  TestBasicOpcodes... ";
    
    BytecodeWriter writer;
    
    // Emit some simple opcodes
    writer.Emit(DreamProcOpcode::Add);
    writer.Emit(DreamProcOpcode::Subtract);
    writer.Emit(DreamProcOpcode::Return);
    
    const auto& bytecode = writer.GetBytecode();
    
    assert(bytecode.size() == 3);
    assert(bytecode[0] == 0x08);  // Add
    assert(bytecode[1] == 0x12);  // Subtract
    assert(bytecode[2] == 0x10);  // Return
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test opcodes with operands
bool TestOpcodesWithOperands() {
    std::cout << "  TestOpcodesWithOperands... ";
    
    BytecodeWriter writer;
    
    // EmitByte
    writer.EmitByte(DreamProcOpcode::Pop, 5);
    
    // EmitShort
    writer.EmitShort(DreamProcOpcode::PushType, 1000);
    
    // EmitInt
    writer.EmitInt(DreamProcOpcode::PushProc, 123456);
    
    // EmitFloat
    writer.EmitFloat(DreamProcOpcode::PushFloat, 3.14f);
    
    const auto& bytecode = writer.GetBytecode();
    
    // Check byte opcode (1 byte opcode + 1 byte operand = 2 bytes)
    assert(bytecode[0] == 0x51);  // Pop opcode
    assert(bytecode[1] == 5);     // operand
    
    // Check short opcode (1 byte opcode + 2 byte operand = 3 bytes)
    assert(bytecode[2] == 0x02);  // PushType opcode
    assert(bytecode[3] == 0xE8);  // 1000 & 0xFF (little-endian low byte)
    assert(bytecode[4] == 0x03);  // (1000 >> 8) & 0xFF (high byte)
    
    // Check int opcode (1 byte opcode + 4 byte operand = 5 bytes)
    assert(bytecode[5] == 0x26);   // PushProc opcode
    assert(bytecode[6] == 0x40);   // 123456 & 0xFF
    assert(bytecode[7] == 0xE2);   // (123456 >> 8) & 0xFF
    assert(bytecode[8] == 0x01);   // (123456 >> 16) & 0xFF
    assert(bytecode[9] == 0x00);   // (123456 >> 24) & 0xFF
    
    // Check float opcode (1 byte opcode + 4 byte float = 5 bytes)
    assert(bytecode[10] == 0x38);  // PushFloat opcode
    // Float bytes depend on IEEE 754 representation, just check size
    assert(bytecode.size() == 15);  // 2 + 3 + 5 + 5
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test string emission
bool TestStringEmission() {
    std::cout << "  TestStringEmission... ";
    
    BytecodeWriter writer;
    
    // Emit strings
    writer.EmitString(DreamProcOpcode::PushString, "Hello");
    writer.EmitString(DreamProcOpcode::PushString, "World");
    writer.EmitString(DreamProcOpcode::PushString, "Hello");  // Duplicate
    
    const auto& bytecode = writer.GetBytecode();
    
    // Each EmitString is 1 byte opcode + 4 bytes string ID = 5 bytes
    assert(bytecode.size() == 15);
    
    // Check first string (ID 0)
    assert(bytecode[0] == 0x03);  // PushString opcode
    assert(bytecode[1] == 0x00);  // ID 0
    assert(bytecode[2] == 0x00);
    assert(bytecode[3] == 0x00);
    assert(bytecode[4] == 0x00);
    
    // Check second string (ID 1)
    assert(bytecode[5] == 0x03);  // PushString opcode
    assert(bytecode[6] == 0x01);  // ID 1
    
    // Check third string (should reuse ID 0)
    assert(bytecode[10] == 0x03);  // PushString opcode
    assert(bytecode[11] == 0x00);  // ID 0 (reused)
    
    // Verify string table
    assert(writer.GetStringId("Hello") == 0);
    assert(writer.GetStringId("World") == 1);
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test label and jump functionality
bool TestLabelsAndJumps() {
    std::cout << "  TestLabelsAndJumps... ";
    
    BytecodeWriter writer;
    
    // Create a simple control flow:
    // 0: PushFloat 1.0
    // 5: JumpIfFalse -> end
    // 10: Add
    // 11: end: Return
    
    writer.EmitFloat(DreamProcOpcode::PushFloat, 1.0f);
    
    int endLabel = writer.CreateLabel();
    writer.EmitJump(DreamProcOpcode::JumpIfFalse, endLabel);
    
    writer.Emit(DreamProcOpcode::Add);
    
    writer.MarkLabel(endLabel);
    writer.Emit(DreamProcOpcode::Return);
    
    // Finalize to resolve jumps
    writer.Finalize();
    
    const auto& bytecode = writer.GetBytecode();
    
    // Check structure
    assert(bytecode[0] == 0x38);  // PushFloat
    assert(bytecode[5] == 0x0C);  // JumpIfFalse
    // Jump offset should point from position 10 to position 11 (offset = 1)
    // But offset is relative to AFTER the jump instruction, so offset = 11 - 10 = 1
    int32_t jumpOffset = bytecode[6] | (bytecode[7] << 8) | (bytecode[8] << 16) | (bytecode[9] << 24);
    assert(jumpOffset == 1);  // Jump forward by 1 byte (over the Add instruction)
    
    assert(bytecode[10] == 0x08);  // Add
    assert(bytecode[11] == 0x10);  // Return
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test backward jump
bool TestBackwardJump() {
    std::cout << "  TestBackwardJump... ";
    
    BytecodeWriter writer;
    
    // Create a loop:
    // 0: loop: Add
    // 1: PushFloat 1.0
    // 6: Jump -> loop
    // 11: (end)
    
    int loopLabel = writer.CreateLabel();
    writer.MarkLabel(loopLabel);
    
    writer.Emit(DreamProcOpcode::Add);
    writer.EmitFloat(DreamProcOpcode::PushFloat, 1.0f);
    writer.EmitJump(DreamProcOpcode::Jump, loopLabel);
    
    writer.Finalize();
    
    const auto& bytecode = writer.GetBytecode();
    
    // Jump at position 6, targets position 0
    // Offset is relative to position AFTER the jump instruction (position 11)
    // Offset = 0 - 11 = -11
    assert(bytecode[6] == 0x0E);  // Jump opcode
    int32_t jumpOffset = bytecode[7] | (bytecode[8] << 8) | (bytecode[9] << 16) | (static_cast<int32_t>(bytecode[10]) << 24);
    assert(jumpOffset == -11);  // Jump backward by 11 bytes
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test Reset functionality
bool TestReset() {
    std::cout << "  TestReset... ";
    
    BytecodeWriter writer;
    
    // Generate some bytecode
    writer.EmitString(DreamProcOpcode::PushString, "test");
    writer.Emit(DreamProcOpcode::Add);
    int label = writer.CreateLabel();
    writer.MarkLabel(label);
    
    assert(writer.GetBytecode().size() > 0);
    assert(writer.GetStringId("test") == 0);
    
    // Reset
    writer.Reset();
    
    // Verify everything is cleared
    assert(writer.GetBytecode().size() == 0);
    assert(writer.GetPosition() == 0);
    
    // Can reuse - string IDs should start from 0 again
    writer.EmitString(DreamProcOpcode::PushString, "new");
    assert(writer.GetStringId("new") == 0);
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test multiple labels
bool TestMultipleLabels() {
    std::cout << "  TestMultipleLabels... ";
    
    BytecodeWriter writer;
    
    int label1 = writer.CreateLabel();
    int label2 = writer.CreateLabel();
    int label3 = writer.CreateLabel();
    
    assert(label1 != label2);
    assert(label2 != label3);
    assert(label1 < label2);
    assert(label2 < label3);
    
    writer.MarkLabel(label1);
    writer.Emit(DreamProcOpcode::Add);
    
    writer.MarkLabel(label2);
    writer.Emit(DreamProcOpcode::Subtract);
    
    writer.MarkLabel(label3);
    writer.Emit(DreamProcOpcode::Return);
    
    // Jump to each label
    writer.EmitJump(DreamProcOpcode::Jump, label1);
    writer.EmitJump(DreamProcOpcode::Jump, label2);
    writer.EmitJump(DreamProcOpcode::Jump, label3);
    
    writer.Finalize();
    
    // Should not throw
    assert(writer.GetBytecode().size() > 0);
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// Test complex bytecode generation (simulating a real proc)
bool TestComplexBytecode() {
    std::cout << "  TestComplexBytecode... ";
    
    BytecodeWriter writer;
    
    // Simulate: proc/test(x)
    //   if (x > 5)
    //     return x + 1
    //   else
    //     return x - 1
    
    // Load parameter x (simulated with PushReferenceValue)
    writer.EmitInt(DreamProcOpcode::PushProc, 0);  // param index
    
    // Push constant 5
    writer.EmitFloat(DreamProcOpcode::PushFloat, 5.0f);
    
    // Compare: x > 5
    writer.Emit(DreamProcOpcode::CompareGreaterThan);
    
    // Jump if false to else branch
    int elseLabel = writer.CreateLabel();
    writer.EmitJump(DreamProcOpcode::JumpIfFalse, elseLabel);
    
    // Then branch: x + 1
    writer.EmitInt(DreamProcOpcode::PushProc, 0);  // x again
    writer.EmitFloat(DreamProcOpcode::PushFloat, 1.0f);
    writer.Emit(DreamProcOpcode::Add);
    writer.Emit(DreamProcOpcode::Return);
    
    // Else branch: x - 1
    writer.MarkLabel(elseLabel);
    writer.EmitInt(DreamProcOpcode::PushProc, 0);  // x again
    writer.EmitFloat(DreamProcOpcode::PushFloat, 1.0f);
    writer.Emit(DreamProcOpcode::Subtract);
    writer.Emit(DreamProcOpcode::Return);
    
    writer.Finalize();
    
    const auto& bytecode = writer.GetBytecode();
    
    // Verify it generated reasonable bytecode (should be > 30 bytes)
    assert(bytecode.size() > 30);
    
    // Verify some key instructions are present
    bool hasCompareGreaterThan = false;
    bool hasAdd = false;
    bool hasSubtract = false;
    bool hasReturn = false;
    
    for (uint8_t byte : bytecode) {
        if (byte == 0x14) hasCompareGreaterThan = true;
        if (byte == 0x08) hasAdd = true;
        if (byte == 0x12) hasSubtract = true;
        if (byte == 0x10) hasReturn = true;
    }
    
    assert(hasCompareGreaterThan);
    assert(hasAdd);
    assert(hasSubtract);
    assert(hasReturn);
    
    std::cout << "PASSED" << std::endl;
    return true;
}

int RunBytecodeTests() {
    std::cout << "\n=== Running Bytecode Tests ===" << std::endl;
    
    int passed = 0;
    int total = 8;
    
    if (TestBasicOpcodes()) passed++;
    if (TestOpcodesWithOperands()) passed++;
    if (TestStringEmission()) passed++;
    if (TestLabelsAndJumps()) passed++;
    if (TestBackwardJump()) passed++;
    if (TestReset()) passed++;
    if (TestMultipleLabels()) passed++;
    if (TestComplexBytecode()) passed++;
    
    std::cout << "\n=== Bytecode Test Results ===" << std::endl;
    std::cout << "Passed: " << passed << "/" << total << std::endl;
    
    if (passed == total) {
        std::cout << "✓ All bytecode tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "✗ Some bytecode tests failed!" << std::endl;
        return 1;
    }
}
