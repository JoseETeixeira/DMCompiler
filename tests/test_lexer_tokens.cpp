/// @file test_lexer_tokens.cpp
/// @brief Unit tests for new token types (Task 6)

#include "../include/DMLexer.h"
#include "../include/Token.h"
#include <iostream>
#include <vector>
#include <string>
#include <cassert>

using namespace DMCompiler;

// Test counter
static int lexer_tests_run = 0;
static int lexer_tests_passed = 0;

#define TEST(name) void name()
#define EXPECT_EQ(a, b) do { \
    lexer_tests_run++; \
    if ((a) == (b)) { \
        lexer_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected " << (int)(b) << " but got " << (int)(a) << std::endl; \
    } \
} while(0)

// Helper to tokenize a string
std::vector<Token> Tokenize(const std::string& source) {
    DMLexer lexer("test", source, false);
    std::vector<Token> tokens;
    while (true) {
        Token token = lexer.GetNextToken();
        if (token.Type == TokenType::EndOfFile) break;
        tokens.push_back(token);
    }
    return tokens;
}

TEST(TestCompoundAssignments) {
    // Test ||=
    auto tokens = Tokenize("a ||= b");
    // a, ||=, b
    EXPECT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[1].Type, TokenType::OrOrAssign);
    
    // Test &&=
    tokens = Tokenize("a &&= b");
    EXPECT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[1].Type, TokenType::AndAndAssign);
}

TEST(TestComparisons) {
    // Test ~=
    auto tokens = Tokenize("a ~= b");
    EXPECT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[1].Type, TokenType::TildeEquals);
    
    // Test ~!
    tokens = Tokenize("a ~! b");
    EXPECT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[1].Type, TokenType::TildeExclamation);
}

TEST(TestNullConditionals) {
    // Test ?.
    auto tokens = Tokenize("a?.b");
    EXPECT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[1].Type, TokenType::QuestionDot);
    
    // Test ?[
    tokens = Tokenize("a?[b]");
    EXPECT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[1].Type, TokenType::QuestionBracket);
}

TEST(TestRightShift) {
    // Test >>
    auto tokens = Tokenize("a >> b");
    EXPECT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[1].Type, TokenType::RightShift);
}

int main() {
    std::cout << "Running Lexer Token Tests..." << std::endl;
    std::cout << "========================================" << std::endl;
    
    TestCompoundAssignments();
    TestComparisons();
    TestNullConditionals();
    TestRightShift();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Lexer Token Tests: " << lexer_tests_passed << "/" << lexer_tests_run << " passed" << std::endl;
    
    return (lexer_tests_passed == lexer_tests_run) ? 0 : 1;
}
