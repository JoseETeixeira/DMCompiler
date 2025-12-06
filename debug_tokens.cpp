#include "DMCompiler.h"
#include "DMParser.h"
#include "DMPreprocessor.h"
#include "TokenStreamDMLexer.h"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: debug_tokens <file.dm>" << std::endl;
        return 1;
    }
    
    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Cannot open file: " << argv[1] << std::endl;
        return 1;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    DMCompiler::DMCompilerSettings settings;
    settings.Verbose = true;
    DMCompiler::DMCompiler compiler(settings);
    
    DMCompiler::DMPreprocessor preprocessor(&compiler);
    auto tokens = preprocessor.Preprocess(argv[1], content);
    
    std::cout << "Preprocessed tokens around line 100-102:" << std::endl;
    for (const auto& token : tokens) {
        if (token.Loc.Line >= 99 && token.Loc.Line <= 103) {
            std::cout << "  L" << token.Loc.Line << ":C" << token.Loc.Column 
                      << " Type=" << static_cast<int>(token.Type) 
                      << " Text='" << token.Text << "'" << std::endl;
        }
    }
    
    return 0;
}