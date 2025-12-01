#include <iostream>
#include <fstream>
#include <filesystem>
#include "../include/DMPreprocessor.h"
#include "../include/DMLexer.h"

using namespace DMCompiler;

namespace fs = std::filesystem;

int RunPreprocessorTests() {
    std::cout << "\n[Preprocessor Tests]" << std::endl;
    int failures = 0;
    
    // Test 1: Define and Undefine
    {
        std::cout << "  Test 1: Define and Undefine... ";
        try {
            DMPreprocessor preprocessor;
            
            // Test Define
            preprocessor.Define("TEST_MACRO", "123");
            if (!preprocessor.IsDefined("TEST_MACRO")) {
                std::cout << "FAILED (TEST_MACRO not defined)" << std::endl;
                failures++;
            } else {
                // Test Undefine
                preprocessor.Undefine("TEST_MACRO");
                if (preprocessor.IsDefined("TEST_MACRO")) {
                    std::cout << "FAILED (TEST_MACRO still defined after Undefine)" << std::endl;
                    failures++;
                } else {
                    std::cout << "PASSED" << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "FAILED (exception: " << e.what() << ")" << std::endl;
            failures++;
        }
    }
    
    // Test 2: Multiple defines
    {
        std::cout << "  Test 2: Multiple defines... ";
        try {
            DMPreprocessor preprocessor;
            
            preprocessor.Define("FIRST", "1");
            preprocessor.Define("SECOND", "2");
            preprocessor.Define("THIRD", "3");
            
            if (!preprocessor.IsDefined("FIRST") || 
                !preprocessor.IsDefined("SECOND") || 
                !preprocessor.IsDefined("THIRD")) {
                std::cout << "FAILED (Not all macros defined)" << std::endl;
                failures++;
            } else {
                std::cout << "PASSED" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "FAILED (exception: " << e.what() << ")" << std::endl;
            failures++;
        }
    }
    
    // Test 3: Redefine macro
    {
        std::cout << "  Test 3: Redefine macro... ";
        try {
            DMPreprocessor preprocessor;
            
            preprocessor.Define("VERSION", "1");
            preprocessor.Define("VERSION", "2"); // Redefine
            
            if (!preprocessor.IsDefined("VERSION")) {
                std::cout << "FAILED (VERSION not defined)" << std::endl;
                failures++;
            } else {
                std::cout << "PASSED" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "FAILED (exception: " << e.what() << ")" << std::endl;
            failures++;
        }
    }
    
    // Test 4: Preprocess test file
    {
        std::cout << "  Test 4: Preprocess test file... ";
        try {
            // Create a simple test file
            std::string testFilePath = "test_files/simple_test.dm";
            std::ofstream testFile(testFilePath);
            if (testFile.is_open()) {
                testFile << "#define TEST 1\n";
                testFile << "var/test_var = TEST\n";
                testFile.close();
                
                DMPreprocessor preprocessor;
                auto tokens = preprocessor.Preprocess(testFilePath);
                
                // Check that we got some tokens
                if (tokens.empty()) {
                    std::cout << "FAILED (No tokens produced)" << std::endl;
                    failures++;
                } else {
                    std::cout << "PASSED (" << tokens.size() << " tokens)" << std::endl;
                }
                
                // Clean up
                fs::remove(testFilePath);
            } else {
                std::cout << "FAILED (Could not create test file)" << std::endl;
                failures++;
            }
        } catch (const std::exception& e) {
            std::cout << "FAILED (exception: " << e.what() << ")" << std::endl;
            failures++;
        }
    }
    
    // Test 5: Built-in macros
    {
        std::cout << "  Test 5: Built-in macros (__LINE__, __FILE__, DM_VERSION, DM_BUILD)... ";
        try {
            DMPreprocessor preprocessor;
            
            // Check that built-in macros are defined
            if (!preprocessor.IsDefined("__LINE__") || 
                !preprocessor.IsDefined("__FILE__") ||
                !preprocessor.IsDefined("DM_VERSION") ||
                !preprocessor.IsDefined("DM_BUILD")) {
                std::cout << "FAILED (Built-in macro not defined)" << std::endl;
                failures++;
            } else {
                std::cout << "PASSED" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "FAILED (exception: " << e.what() << ")" << std::endl;
            failures++;
        }
    }
    
    // Test 6: Streaming interface (Initialize, GetNextToken, IsComplete)
    {
        std::cout << "  Test 6: Streaming interface (Initialize, GetNextToken, IsComplete)... ";
        try {
            // Create a simple test file
            std::string testFilePath = "test_files/streaming_test.dm";
            fs::create_directories("test_files");
            std::ofstream testFile(testFilePath);
            if (testFile.is_open()) {
                testFile << "var/x = 1\n";
                testFile << "var/y = 2\n";
                testFile.close();
                
                DMPreprocessor preprocessor;
                
                // Test Initialize
                if (!preprocessor.Initialize(testFilePath)) {
                    std::cout << "FAILED (Initialize returned false)" << std::endl;
                    failures++;
                } else {
                    // Test GetNextToken and IsComplete
                    int tokenCount = 0;
                    while (!preprocessor.IsComplete()) {
                        Token token = preprocessor.GetNextToken();
                        if (token.Type == TokenType::EndOfFile) {
                            break;
                        }
                        tokenCount++;
                        
                        // Safety check to prevent infinite loop
                        if (tokenCount > 1000) {
                            std::cout << "FAILED (Too many tokens, possible infinite loop)" << std::endl;
                            failures++;
                            break;
                        }
                    }
                    
                    if (tokenCount > 0 && tokenCount < 1000) {
                        std::cout << "PASSED (" << tokenCount << " tokens)" << std::endl;
                    } else if (tokenCount == 0) {
                        std::cout << "FAILED (No tokens produced)" << std::endl;
                        failures++;
                    }
                }
                
                // Clean up
                fs::remove(testFilePath);
            } else {
                std::cout << "FAILED (Could not create test file)" << std::endl;
                failures++;
            }
        } catch (const std::exception& e) {
            std::cout << "FAILED (exception: " << e.what() << ")" << std::endl;
            failures++;
        }
    }
    
    std::cout << "\n  Preprocessor tests: " << (6 - failures) << "/6 passed" << std::endl;
    return failures;
}
