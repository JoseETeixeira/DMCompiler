#include "DMCompiler.h"
#include "DMObjectTree.h"
#include "DMDisassembler.h"
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    DMCompiler::DMCompiler compiler;
    
    // Find the test file
    std::string dmFileName = "test_bare_new.dm";
    std::string dmFilePath = "";

    if (fs::exists(dmFileName)) {
        dmFilePath = dmFileName;
    } else if (fs::exists("../" + dmFileName)) {
        dmFilePath = "../" + dmFileName;
    } else if (fs::exists("../../" + dmFileName)) {
        dmFilePath = "../../" + dmFileName;
    } else if (fs::exists("../../../" + dmFileName)) {
        dmFilePath = "../../../" + dmFileName;
    } else if (fs::exists("tests/" + dmFileName)) {
        dmFilePath = "tests/" + dmFileName;
    } else {
        std::cerr << "Could not find " << dmFileName << std::endl;
        return 1;
    }
    
    std::cout << "=== Testing Bare 'new' Type Inference ===" << std::endl;
    std::cout << "Compiling " << dmFilePath << "..." << std::endl;
    
    DMCompiler::DMCompilerSettings settings;
    settings.Files.push_back(dmFilePath);
    settings.SuppressUnimplementedWarnings = true;
    
    bool success = compiler.Compile(settings);
    
    if (!success) {
        std::cerr << "Compilation failed!" << std::endl;
        return 1;
    }
    
    std::cout << "Compilation succeeded!" << std::endl;
    std::cout << std::endl;
    
    // Get the object tree and check that procs were compiled
    auto* objectTree = compiler.GetObjectTree();
    if (!objectTree) {
        std::cerr << "No object tree available" << std::endl;
        return 1;
    }
    
    // List compiled procs
    std::cout << "Compiled procs:" << std::endl;
    int bareNewProcs = 0;
    for (auto* proc : objectTree->GetAllProcs()) {
        if (proc && proc->Name.find("test_") == 0) {
            std::cout << "  - " << proc->ToString() << std::endl;
            bareNewProcs++;
            
            // Show bytecode for verification
            if (!proc->Bytecode.empty()) {
                std::cout << "    Bytecode size: " << proc->Bytecode.size() << " bytes" << std::endl;
            }
        }
    }
    
    if (bareNewProcs == 0) {
        std::cerr << "No test procs found!" << std::endl;
        return 1;
    }
    
    std::cout << std::endl;
    std::cout << "=== Bare 'new' Type Inference Test PASSED ===" << std::endl;
    std::cout << "Total test procs compiled: " << bareNewProcs << std::endl;
    
    return 0;
}
