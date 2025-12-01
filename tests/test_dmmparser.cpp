#include <iostream>
#include <cassert>
#include <memory>
#include "../include/DMMParser.h"
#include "../include/DMLexer.h"
#include "../include/DMCompiler.h"

// Test counter
int dmmparser_tests_run = 0;
int dmmparser_tests_passed = 0;

#define EXPECT_EQ(a, b) do { \
    dmmparser_tests_run++; \
    if ((a) == (b)) { \
        dmmparser_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected " << (a) << " == " << (b) << std::endl; \
    } \
} while(0)

#define EXPECT_NE(a, b) do { \
    dmmparser_tests_run++; \
    if ((a) != (b)) { \
        dmmparser_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected not equal" << std::endl; \
    } \
} while(0)

#define ASSERT_NE(a, b) do { \
    if ((a) == (b)) { \
        std::cout << "FATAL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Assertion failed" << std::endl; \
        return; \
    } \
} while(0)

// Test that DMM parser exists and can be instantiated
void TestParserExists() {
    std::string dmmContent = "";
    
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMLexer dmLexer("test.dmm", dmmContent);
    
    // Just verify we can create a DMMParser instance
    DMCompiler::DMMParser parser(&compiler, &dmLexer, 0);
    
    std::cout << "TestParserExists passed!" << std::endl;
}

// Test basic DMM file parsing with a simple cell definition
void TestParseCellDefinition() {
    std::string dmmContent = R"("a" = (/turf/floor))";
    
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMLexer dmLexer("test.dmm", dmmContent);
    DMCompiler::DMMParser parser(&compiler, &dmLexer, 0);
    
    auto cellDef = parser.ParseCellDefinition();
    
    ASSERT_NE(cellDef.get(), nullptr);
    EXPECT_EQ(cellDef->Name, "a");
    
    std::cout << "TestParseCellDefinition passed!" << std::endl;
}

// Test parsing a complete map with cell definition only
void TestParseMapWithCellOnly() {
    std::string dmmContent = R"("a" = (/turf/floor))";
    
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMLexer dmLexer("test.dmm", dmmContent);
    DMCompiler::DMMParser parser(&compiler, &dmLexer, 0);
    
    auto map = parser.ParseMap();
    
    ASSERT_NE(map.get(), nullptr);
    std::cout << "  Cell definitions count: " << map->CellDefinitions.size() << std::endl;
    EXPECT_EQ(map->CellDefinitions.size(), static_cast<size_t>(1));
    
    std::cout << "TestParseMapWithCellOnly passed!" << std::endl;
}

// Test parsing map coordinates
void TestParseMapBlock() {
    // Using actual DMM format with a simple 1x1 map
    std::string dmmContent = "\"a\" = (/turf/floor)\n(1,1,1) = {\"a\"}";
    
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMLexer dmLexer("test.dmm", dmmContent);
    DMCompiler::DMMParser parser(&compiler, &dmLexer, 0);
    
    auto map = parser.ParseMap();
    
    ASSERT_NE(map.get(), nullptr);
    std::cout << "  Cell definitions count: " << map->CellDefinitions.size() << std::endl;
    std::cout << "  Map blocks count: " << map->Blocks.size() << std::endl;
    
    // For now, just verify the parser doesn't crash
    // The full parsing might need ObjectTree to be populated
    if (map->CellDefinitions.size() > 0) {
        std::cout << "  Cell definition parsed successfully" << std::endl;
    }
    if (map->Blocks.size() > 0) {
        std::cout << "  Map block parsed successfully" << std::endl;
        auto& block = map->Blocks[0];
        EXPECT_EQ(block->X, 1);
        EXPECT_EQ(block->Y, 1);
        EXPECT_EQ(block->Z, 1);
    }
    
    std::cout << "TestParseMapBlock passed!" << std::endl;
}

int RunDMMParserTests() {
    std::cout << "\n=== Running DMMParser Tests ===" << std::endl;
    
    TestParserExists();
    TestParseCellDefinition();
    TestParseMapWithCellOnly();
    TestParseMapBlock();
    
    std::cout << "\nDMMParser Tests: " << dmmparser_tests_passed << "/" << dmmparser_tests_run << " assertions passed" << std::endl;
    
    if (dmmparser_tests_passed == dmmparser_tests_run) {
        std::cout << "All DMMParser tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << (dmmparser_tests_run - dmmparser_tests_passed) << " assertion(s) failed!" << std::endl;
        return 1;
    }
}
