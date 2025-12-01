#include "../include/DMLexer.h"
#include <iostream>

using namespace DMCompiler;

int main() {
    std::string source = "// Line comment\nvar x = 1;";
    DMLexer lexer("test.dm", source);
    
    std::cout << "Source: " << source << std::endl << std::endl;
    
    for (int i = 0; i < 10; i++) {
        Token tok = lexer.GetNextToken();
        std::cout << "Token " << i << ": Type=" << static_cast<int>(tok.Type) 
                  << " Text='" << tok.Text << "'" << std::endl;
        
        if (tok.Type == TokenType::EndOfFile) {
            break;
        }
    }
    
    return 0;
}
