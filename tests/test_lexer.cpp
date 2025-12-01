#include "../include/DMLexer.h"
#include <iostream>
#include <cassert>

using namespace DMCompiler;

void TestBasicTokenization() {
    std::string source = "var x = 10;\nproc test() { return x; }";
    DMLexer lexer("test.dm", source);
    
    Token tok1 = lexer.GetNextToken();
    assert(tok1.Type == TokenType::Var);
    
    Token tok2 = lexer.GetNextToken();
    assert(tok2.Type == TokenType::Identifier);
    assert(tok2.Text == "x");
    
    Token tok3 = lexer.GetNextToken();
    assert(tok3.Type == TokenType::Assign);
    
    Token tok4 = lexer.GetNextToken();
    assert(tok4.Type == TokenType::Number);
    
    std::cout << "TestBasicTokenization passed!" << std::endl;
}

void TestStrings() {
    std::string source = R"(var s = "hello world";)";
    DMLexer lexer("test.dm", source);
    
    Token tok1 = lexer.GetNextToken();
    assert(tok1.Type == TokenType::Var);
    
    Token tok2 = lexer.GetNextToken();
    assert(tok2.Type == TokenType::Identifier);
    
    Token tok3 = lexer.GetNextToken();
    assert(tok3.Type == TokenType::Assign);
    
    Token tok4 = lexer.GetNextToken();
    assert(tok4.Type == TokenType::String);
    assert(tok4.Value.StringValue == "hello world");
    
    std::cout << "TestStrings passed!" << std::endl;
}

void TestNumbers() {
    std::string source = "var a = 42; var b = 3.14; var c = 0xFF;";
    DMLexer lexer("test.dm", source);
    
    // Skip to first number
    lexer.GetNextToken(); // var
    lexer.GetNextToken(); // a
    lexer.GetNextToken(); // =
    
    Token num1 = lexer.GetNextToken();
    assert(num1.Type == TokenType::Number);
    assert(num1.Value.IntValue == 42);
    
    // Skip to second number
    lexer.GetNextToken(); // ;
    lexer.GetNextToken(); // var
    lexer.GetNextToken(); // b
    lexer.GetNextToken(); // =
    
    Token num2 = lexer.GetNextToken();
    assert(num2.Type == TokenType::Number);
    assert(num2.Value.FloatValue > 3.13 && num2.Value.FloatValue < 3.15);
    
    // Skip to third number
    lexer.GetNextToken(); // ;
    lexer.GetNextToken(); // var
    lexer.GetNextToken(); // c
    lexer.GetNextToken(); // =
    
    Token num3 = lexer.GetNextToken();
    assert(num3.Type == TokenType::Number);
    assert(num3.Value.IntValue == 255);
    
    std::cout << "TestNumbers passed!" << std::endl;
}

void TestComments() {
    std::string source = "// Line comment\nvar x = 1;\n/* Block\ncomment */\nvar y = 2;";
    DMLexer lexer("test.dm", source);
    
    // After line comment (which consumes its trailing newline), should get first real token
    Token tok1 = lexer.GetNextToken();
    assert(tok1.Type == TokenType::Var);
    
    Token tok2 = lexer.GetNextToken();
    assert(tok2.Type == TokenType::Identifier);
    assert(tok2.Text == "x");
    
    // Skip ahead: =, 1, ;
    lexer.GetNextToken(); // =
    lexer.GetNextToken(); // 1
    lexer.GetNextToken(); // ;
    
    // After the semicolon there's a newline, then block comment, then another newline
    Token newline1 = lexer.GetNextToken();
    assert(newline1.Type == TokenType::Newline);
    
    // After the first newline, there's a block comment (which gets skipped), then another newline
    Token newline2 = lexer.GetNextToken();
    assert(newline2.Type == TokenType::Newline);
    
    // Now after both newlines and the block comment, should get the second var
    Token tok3 = lexer.GetNextToken();
    assert(tok3.Type == TokenType::Var);
    
    Token tok4 = lexer.GetNextToken();
    assert(tok4.Type == TokenType::Identifier);
    assert(tok4.Text == "y");
    
    std::cout << "TestComments passed!" << std::endl;
}

void TestOperators() {
    std::string source = "== != <= >= && || << >> ++ --";
    DMLexer lexer("test.dm", source);
    
    assert(lexer.GetNextToken().Type == TokenType::Equals);
    assert(lexer.GetNextToken().Type == TokenType::NotEquals);
    assert(lexer.GetNextToken().Type == TokenType::LessOrEqual);
    assert(lexer.GetNextToken().Type == TokenType::GreaterOrEqual);
    assert(lexer.GetNextToken().Type == TokenType::LogicalAnd);
    assert(lexer.GetNextToken().Type == TokenType::LogicalOr);
    assert(lexer.GetNextToken().Type == TokenType::LeftShift);
    assert(lexer.GetNextToken().Type == TokenType::RightShift);
    assert(lexer.GetNextToken().Type == TokenType::Increment);
    assert(lexer.GetNextToken().Type == TokenType::Decrement);
    
    std::cout << "TestOperators passed!" << std::endl;
}

int RunLexerTests() {
    std::cout << "\n=== Running Lexer Tests ===" << std::endl;
    
    try {
        TestBasicTokenization();
        TestStrings();
        TestNumbers();
        TestComments();
        TestOperators();
        
        std::cout << "\nAll lexer tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
