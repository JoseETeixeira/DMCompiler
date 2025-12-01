#include "../include/DMLexer.h"
#include <iostream>

using namespace DMCompiler;

int main() {
    std::string source = "// Line comment\nvar x = 1;\n/* Block\ncomment */\nvar y = 2;";
    DMLexer lexer("test.dm", source);
    
    std::cout << "Tokens produced:\n";
    int count = 0;
    while (true) {
        Token tok = lexer.GetNextToken();
        count++;
        
        std::cout << count << ". Type=" << static_cast<int>(tok.Type) << " Text=\"" << tok.Text << "\"\n";
        
        if (tok.Type == TokenType::EndOfFile) {
            break;
        }
        
        if (count > 20) {
            std::cout << "Too many tokens, stopping...\n";
            break;
        }
    }
    
    return 0;
}
