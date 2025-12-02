/// @file test_bytecode_writer.cpp
/// @brief Unit tests for BytecodeWriter enhancements (filter type emission)

#include "../include/BytecodeWriter.h"
#include "../include/DreamProcOpcode.h"
#include "../include/DreamPath.h"
#include <iostream>
#include <vector>
#include <cassert>

using namespace DMCompiler;

// Test counter
static int bytecode_tests_run = 0;
static int bytecode_tests_passed = 0;

#define TEST(name) void name()
#define EXPECT_EQ(a, b) do { \
    bytecode_tests_run++; \
    if ((a) == (b)) { \
        bytecode_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected " << (int)(b) << " but got " << (int)(a) << std::endl; \
    } \
} while(0)

// Mock BytecodeWriter to inspect output
class MockBytecodeWriter : public BytecodeWriter {
public:
    std::vector<uint8_t> GetBytecodeCopy() const {
        return GetBytecode();
    }
    
    // Helper to read int from bytecode at offset
    int ReadInt(size_t offset) const {
        const auto& bytecode = GetBytecode();
        if (offset + 4 > bytecode.size()) return -1;
        int val = 0;
        val |= bytecode[offset];
        val |= bytecode[offset + 1] << 8;
        val |= bytecode[offset + 2] << 16;
        val |= bytecode[offset + 3] << 24;
        return val;
    }
    
    // Helper to read opcode at offset
    DreamProcOpcode ReadOpcode(size_t offset) const {
        const auto& bytecode = GetBytecode();
        if (offset >= bytecode.size()) return DreamProcOpcode::Error;
        return static_cast<DreamProcOpcode>(bytecode[offset]);
    }
};

TEST(TestCreateFilteredListEnumerator) {
    MockBytecodeWriter writer;
    
    int enumeratorId = 42;
    int filterTypeId = 100;
    DreamPath filterPath("/mob/living");
    
    // Call the enhanced method
    writer.CreateFilteredListEnumerator(enumeratorId, filterTypeId, filterPath.ToString());
    
    auto bytecode = writer.GetBytecodeCopy();
    
    // Expected layout:
    // Opcode (1 byte)
    // EnumeratorID (4 bytes)
    // FilterTypeID (4 bytes)
    // FilterPathStringID (4 bytes)
    
    EXPECT_EQ(bytecode.size(), 13);
    
    // Check Opcode
    EXPECT_EQ(writer.ReadOpcode(0), DreamProcOpcode::CreateFilteredListEnumerator);
    
    // Check EnumeratorID
    EXPECT_EQ(writer.ReadInt(1), enumeratorId);
    
    // Check FilterTypeID
    EXPECT_EQ(writer.ReadInt(5), filterTypeId);
    
    // Check FilterPathStringID
    // Since we don't have a real string table in this mock test, 
    // we just verify that *some* ID was written.
    // In a real scenario, BytecodeWriter calls GetStringId() which adds to a table.
    // Here we assume BytecodeWriter implementation handles string ID generation correctly.
    // We just verify 4 bytes were written.
    // Note: The actual ID value depends on BytecodeWriter's internal state/mocking.
    // If BytecodeWriter uses a global or internal string table, it will return an ID.
}

TEST(TestCreateFilteredListEnumerator_NullPath) {
    MockBytecodeWriter writer;
    
    int enumeratorId = 1;
    int filterTypeId = 0; // No type filter
    DreamPath filterPath(""); // Empty path
    
    writer.CreateFilteredListEnumerator(enumeratorId, filterTypeId, filterPath.ToString());
    
    auto bytecode = writer.GetBytecodeCopy();
    EXPECT_EQ(bytecode.size(), 13);
    
    EXPECT_EQ(writer.ReadOpcode(0), DreamProcOpcode::CreateFilteredListEnumerator);
    EXPECT_EQ(writer.ReadInt(1), enumeratorId);
    EXPECT_EQ(writer.ReadInt(5), filterTypeId);
}

int main() {
    std::cout << "Running Bytecode Writer Tests..." << std::endl;
    std::cout << "========================================" << std::endl;
    
    TestCreateFilteredListEnumerator();
    TestCreateFilteredListEnumerator_NullPath();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Bytecode Writer Tests: " << bytecode_tests_passed << "/" << bytecode_tests_run << " passed" << std::endl;
    
    return (bytecode_tests_passed == bytecode_tests_run) ? 0 : 1;
}
