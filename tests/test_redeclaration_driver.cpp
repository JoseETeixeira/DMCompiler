#include "DMCompiler.h"
#include "DMObjectTree.h"
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    DMCompiler::DMCompiler compiler;
    
    // Find the test file
    std::string dmFileName = "test_redeclaration.dm";
    std::string dmFilePath = "";

    if (fs::exists(dmFileName)) {
        dmFilePath = dmFileName;
    } else if (fs::exists("../" + dmFileName)) {
        dmFilePath = "../" + dmFileName;
    } else if (fs::exists("../../" + dmFileName)) {
        dmFilePath = "../../" + dmFileName;
    } else if (fs::exists("../../../" + dmFileName)) {
        dmFilePath = "../../../" + dmFileName;
    } else {
        // Try absolute path to source directory if relative paths fail
        // This is a fallback for when running from deep build directories
        dmFilePath = "C:/Users/josee/OneDrive/Ambiente de Trabalho/OpenDream/DMCompilerCpp/tests/" + dmFileName;
        if (!fs::exists(dmFilePath)) {
            std::cerr << "Could not find " << dmFileName << std::endl;
            return 1;
        }
    }
    
    std::cout << "Compiling " << dmFilePath << "..." << std::endl;
    
    DMCompiler::DMCompilerSettings settings;
    settings.Files.push_back(dmFilePath);
    settings.SuppressUnimplementedWarnings = true;
    
    bool success = compiler.Compile(settings);
    
    if (success) {
        std::cout << "Compilation passed!" << std::endl;
        return 0;
    } else {
        std::cerr << "Compilation failed!" << std::endl;
        return 1;
    }
}
