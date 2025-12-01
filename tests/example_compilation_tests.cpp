#include "../include/DMCompiler.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

/// <summary>
/// Helper function to validate JSON output
/// </summary>
bool ValidateJsonOutput(const std::string& jsonPath) {
    if (!fs::exists(jsonPath)) {
        std::cerr << "ERROR: JSON output file not found: " << jsonPath << std::endl;
        return false;
    }
    
    // Basic JSON validation - check if file can be opened and has content
    std::ifstream jsonFile(jsonPath);
    if (!jsonFile.is_open()) {
        std::cerr << "ERROR: Could not open JSON file: " << jsonPath << std::endl;
        return false;
    }
    
    // Check if file has content
    jsonFile.seekg(0, std::ios::end);
    size_t fileSize = jsonFile.tellg();
    if (fileSize == 0) {
        std::cerr << "ERROR: JSON file is empty: " << jsonPath << std::endl;
        return false;
    }
    
    // Basic JSON syntax check - should start with { and end with }
    jsonFile.seekg(0, std::ios::beg);
    std::string content((std::istreambuf_iterator<char>(jsonFile)),
                        std::istreambuf_iterator<char>());
    
    // Trim whitespace
    size_t start = content.find_first_not_of(" \t\n\r");
    size_t end = content.find_last_not_of(" \t\n\r");
    
    if (start == std::string::npos || end == std::string::npos) {
        std::cerr << "ERROR: JSON file contains only whitespace" << std::endl;
        return false;
    }
    
    content = content.substr(start, end - start + 1);
    
    if (content.empty() || content[0] != '{' || content[content.length() - 1] != '}') {
        std::cerr << "ERROR: JSON file does not have valid structure (should start with { and end with })" << std::endl;
        return false;
    }
    
    std::cout << "JSON validation passed: " << jsonPath << " (" << fileSize << " bytes)" << std::endl;
    return true;
}

/// <summary>
/// Test compilation of Examples/Beams/DBZ.dme
/// Requirements: 1.1, 1.5, 12.1
/// </summary>
bool TestBeamsCompilation() {
    std::cout << "\n=== Testing Beams/DBZ.dme Compilation ===" << std::endl;
    
    // Check if example file exists
    // Try different paths depending on where test is run from
    std::string dmeFile = "../../../../Examples/Beams/DBZ.dme";  // From build/tests/Release
    if (!fs::exists(dmeFile)) {
        dmeFile = "../../../Examples/Beams/DBZ.dme";  // From build/tests (CTest)
        if (!fs::exists(dmeFile)) {
            dmeFile = "../../Examples/Beams/DBZ.dme";  // From build
            if (!fs::exists(dmeFile)) {
                std::cerr << "ERROR: Example file not found" << std::endl;
                std::cerr << "Please ensure Examples/Beams/DBZ.dme exists in the repository" << std::endl;
                return false;
            }
        }
    }
    
    std::cout << "Found example file: " << dmeFile << std::endl;
    
    // Configure compiler settings
    DMCompiler::DMCompilerSettings settings;
    settings.Files.push_back(dmeFile);
    settings.SuppressUnimplementedWarnings = false;  // We want to see all warnings
    settings.Verbose = true;
    
    // Create compiler instance
    DMCompiler::DMCompiler compiler;
    
    // Measure compilation time
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Compile
    std::cout << "Starting compilation..." << std::endl;
    bool compileResult = compiler.Compile(settings);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Compilation completed in " << duration.count() << " ms" << std::endl;
    
    // Report compilation messages
    const auto& messages = compiler.GetCompilerMessages();
    if (!messages.empty()) {
        std::cout << "\nCompiler messages (" << messages.size() << "):" << std::endl;
        for (const auto& msg : messages) {
            std::cout << "  " << msg << std::endl;
        }
    }
    
    if (!compileResult) {
        std::cerr << "\nERROR: Compilation failed for Beams/DBZ.dme" << std::endl;
        std::cerr << "This indicates missing features or bugs in the compiler" << std::endl;
        return false;
    }
    
    std::cout << "\nCompilation succeeded!" << std::endl;
    
    // Validate JSON output (compiler writes to source directory)
    // Determine the correct path based on where dmeFile was found
    std::string jsonOutput;
    if (dmeFile.find("../../../../") == 0) {
        jsonOutput = "../../../../Examples/Beams/DBZ.json";
    } else if (dmeFile.find("../../../") == 0) {
        jsonOutput = "../../../Examples/Beams/DBZ.json";
    } else {
        jsonOutput = "../../Examples/Beams/DBZ.json";
    }
    
    if (!ValidateJsonOutput(jsonOutput)) {
        return false;
    }
    
    // Check compilation time (should be < 5 seconds per requirement 11.1)
    if (duration.count() > 5000) {
        std::cerr << "WARNING: Compilation took longer than 5 seconds (" 
                  << duration.count() << " ms)" << std::endl;
    }
    
    std::cout << "\n✓ Beams/DBZ.dme compilation test PASSED" << std::endl;
    return true;
}

/// <summary>
/// Test compilation of Examples/GOA/NarutoGOA.dme
/// Requirements: 2.1, 2.5, 12.2
/// </summary>
bool TestGOACompilation() {
    std::cout << "\n=== Testing GOA/NarutoGOA.dme Compilation ===" << std::endl;
    
    // Check if example file exists
    // Try different paths depending on where test is run from
    std::string dmeFile = "../../../../Examples/GOA/NarutoGOA.dme";  // From build/tests/Release
    if (!fs::exists(dmeFile)) {
        dmeFile = "../../../Examples/GOA/NarutoGOA.dme";  // From build/tests (CTest)
        if (!fs::exists(dmeFile)) {
            dmeFile = "../../Examples/GOA/NarutoGOA.dme";  // From build
            if (!fs::exists(dmeFile)) {
                std::cerr << "ERROR: Example file not found" << std::endl;
                std::cerr << "Please ensure Examples/GOA/NarutoGOA.dme exists in the repository" << std::endl;
                return false;
            }
        }
    }
    
    std::cout << "Found example file: " << dmeFile << std::endl;
    
    // Configure compiler settings
    DMCompiler::DMCompilerSettings settings;
    settings.Files.push_back(dmeFile);
    settings.SuppressUnimplementedWarnings = false;  // We want to see all warnings
    settings.Verbose = true;
    
    // Create compiler instance
    DMCompiler::DMCompiler compiler;
    
    // Measure compilation time
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Compile
    std::cout << "Starting compilation..." << std::endl;
    bool compileResult = compiler.Compile(settings);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Compilation completed in " << duration.count() << " ms" << std::endl;
    
    // Report compilation messages
    const auto& messages = compiler.GetCompilerMessages();
    if (!messages.empty()) {
        std::cout << "\nCompiler messages (" << messages.size() << "):" << std::endl;
        for (const auto& msg : messages) {
            std::cout << "  " << msg << std::endl;
        }
    }
    
    if (!compileResult) {
        std::cerr << "\nERROR: Compilation failed for GOA/NarutoGOA.dme" << std::endl;
        std::cerr << "This indicates missing features or bugs in the compiler" << std::endl;
        return false;
    }
    
    std::cout << "\nCompilation succeeded!" << std::endl;
    
    // Validate JSON output (compiler writes to source directory)
    // Determine the correct path based on where dmeFile was found
    std::string jsonOutput;
    if (dmeFile.find("../../../../") == 0) {
        jsonOutput = "../../../../Examples/GOA/NarutoGOA.json";
    } else if (dmeFile.find("../../../") == 0) {
        jsonOutput = "../../../Examples/GOA/NarutoGOA.json";
    } else {
        jsonOutput = "../../Examples/GOA/NarutoGOA.json";
    }
    
    if (!ValidateJsonOutput(jsonOutput)) {
        return false;
    }
    
    // Check compilation time (should be < 30 seconds per requirement 11.2)
    if (duration.count() > 30000) {
        std::cerr << "WARNING: Compilation took longer than 30 seconds (" 
                  << duration.count() << " ms)" << std::endl;
    }
    
    std::cout << "\n✓ GOA/NarutoGOA.dme compilation test PASSED" << std::endl;
    return true;
}

/// <summary>
/// Main test runner for example compilation tests
/// </summary>
int RunExampleCompilationTests() {
    std::cout << "\n=== Running Example Compilation Tests ===" << std::endl;
    std::cout << "These tests validate that DMCompilerCpp can compile real-world DM projects" << std::endl;
    
    int failures = 0;
    
    // Test Beams example
    if (!TestBeamsCompilation()) {
        failures++;
        std::cerr << "\n✗ Beams compilation test FAILED" << std::endl;
    }
    
    // Test GOA example
    if (!TestGOACompilation()) {
        failures++;
        std::cerr << "\n✗ GOA compilation test FAILED" << std::endl;
    }
    
    // Summary
    std::cout << "\n=== Example Compilation Tests Summary ===" << std::endl;
    if (failures == 0) {
        std::cout << "✓ All example compilation tests PASSED" << std::endl;
    } else {
        std::cout << "✗ " << failures << " example compilation test(s) FAILED" << std::endl;
    }
    
    return failures;
}
