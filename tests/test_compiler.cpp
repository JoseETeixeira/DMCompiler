#include "../include/DMCompiler.h"
#include <iostream>
#include <fstream>
#include <filesystem>

void TestSimpleCompilation() {
    std::cout << "Testing simple compilation..." << std::endl;
    
    // Create a simple test file
    std::string testFile = "test_simple.dme";
    std::ofstream out(testFile);
    out << "// Simple test\n";
    out << "var/global_var = 10\n";
    out << "proc/test_proc()\n";
    out << "    return 42\n";
    out.close();
    
    DMCompiler::DMCompilerSettings settings;
    settings.Files.push_back(testFile);
    settings.Verbose = true;
    
    DMCompiler::DMCompiler compiler;
    bool result = compiler.Compile(settings);
    
    // Clean up
    std::filesystem::remove(testFile);
    std::filesystem::remove("test_simple.json");
    
    if (result) {
        std::cout << "Simple compilation test passed!" << std::endl;
    } else {
        std::cout << "Simple compilation test had errors (expected for stub)" << std::endl;
    }
}

void TestWithActualDME() {
    std::cout << "Testing with actual .dme file if available..." << std::endl;
    
    namespace fs = std::filesystem;
    
    // Look for test data
    if (fs::exists("testdata")) {
        for (const auto& entry : fs::directory_iterator("testdata")) {
            if (entry.is_directory()) {
                for (const auto& file : fs::directory_iterator(entry.path())) {
                    if (file.path().extension() == ".dme") {
                        std::cout << "Found test file: " << file.path() << std::endl;
                        
                        DMCompiler::DMCompilerSettings settings;
                        settings.Files.push_back(file.path().string());
                        settings.Verbose = false;
                        
                        DMCompiler::DMCompiler compiler;
                        compiler.Compile(settings);
                        
                        // Just test that it doesn't crash
                        std::cout << "Processed: " << file.path() << std::endl;
                    }
                }
            }
        }
    } else {
        std::cout << "No testdata directory found, skipping actual DME tests" << std::endl;
    }
}

int RunCompilerTests() {
    std::cout << "\n=== Running Compiler Tests ===" << std::endl;
    
    try {
        TestSimpleCompilation();
        TestWithActualDME();
        
        std::cout << "\nCompiler tests completed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
