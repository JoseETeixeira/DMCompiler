#include "../include/DMCompiler.h"
#include <iostream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

int main() {
    std::cout << "Testing type resolution..." << std::endl;

    std::string dmFileName = "test_type_resolution.dm";
    std::string dmFilePath = "";

    if (fs::exists(dmFileName)) {
        dmFilePath = dmFileName;
    } else if (fs::exists("../tests/" + dmFileName)) {
        dmFilePath = "../tests/" + dmFileName;
    } else if (fs::exists("../../tests/" + dmFileName)) {
        dmFilePath = "../../tests/" + dmFileName;
    } else if (fs::exists("../../../tests/" + dmFileName)) {
        dmFilePath = "../../../tests/" + dmFileName;
    } else {
        std::cerr << "Could not find " << dmFileName << std::endl;
        return 1;
    }

    std::cout << "Compiling " << dmFilePath << "..." << std::endl;

    DMCompiler::DMCompilerSettings settings;
    settings.Files.push_back(dmFilePath);
    settings.Verbose = true;
    settings.SuppressUnimplementedWarnings = true;

    DMCompiler::DMCompiler compiler;
    bool result = compiler.Compile(settings);

    if (result) {
        std::cout << "Compilation passed!" << std::endl;
        return 0;
    } else {
        std::cerr << "Compilation failed!" << std::endl;
        return 1;
    }
}
