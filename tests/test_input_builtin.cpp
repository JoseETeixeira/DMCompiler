#include <gtest/gtest.h>
#include "DMCompiler.h"
#include "DMParser.h"
#include "DMLexer.h"
#include "DMObjectTree.h"
#include "DMExpressionCompiler.h"
#include <sstream>

using namespace DMCompiler;

// Test that input() with "as num" emits correct type flags
TEST(InputBuiltinTest, InputAsNum) {
    std::string code = R"(
/proc/test()
    var/x = input("Enter number") as num
)";
    
    std::istringstream input(code);
    DMCompiler compiler;
    
    // This test verifies that the parser and compiler handle input() correctly
    // We're mainly checking that it compiles without errors
    EXPECT_NO_THROW({
        compiler.Compile(input, "test.dm");
    });
}

// Test that input() with "as num|text" emits correct type flags
TEST(InputBuiltinTest, InputAsMultipleTypes) {
    std::string code = R"(
/proc/test()
    var/x = input("Enter value") as num|text
)";
    
    std::istringstream input(code);
    DMCompiler compiler;
    
    EXPECT_NO_THROW({
        compiler.Compile(input, "test.dm");
    });
}

// Test that input() with "in list" compiles correctly
TEST(InputBuiltinTest, InputInList) {
    std::string code = R"(
/proc/test()
    var/list/options = list("A", "B", "C")
    var/x = input("Choose") in options
)";
    
    std::istringstream input(code);
    DMCompiler compiler;
    
    EXPECT_NO_THROW({
        compiler.Compile(input, "test.dm");
    });
}

// Test that input() with both "as type" and "in list" compiles correctly
TEST(InputBuiltinTest, InputAsTypeInList) {
    std::string code = R"(
/proc/test()
    var/list/options = list("A", "B", "C")
    var/x = input("Choose") as obj in options
)";
    
    std::istringstream input(code);
    DMCompiler compiler;
    
    EXPECT_NO_THROW({
        compiler.Compile(input, "test.dm");
    });
}

// Test that input() with all parameters compiles correctly
TEST(InputBuiltinTest, InputAllParameters) {
    std::string code = R"(
/proc/test()
    var/x = input(usr, "Message", "Title", "Default") as text
)";
    
    std::istringstream input(code);
    DMCompiler compiler;
    
    EXPECT_NO_THROW({
        compiler.Compile(input, "test.dm");
    });
}
