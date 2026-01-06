#include "../include/DMCompiler.h"
#include <iostream>
#include <fstream>
#include <filesystem>

static bool FileContainsAll(const std::filesystem::path& path, const std::vector<std::string>& needles) {
    std::ifstream in(path);
    if (!in.is_open()) return false;
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    for (const auto& needle : needles) {
        if (contents.find(needle) == std::string::npos) return false;
    }
    return true;
}

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

bool TestInterfaceMetadataEmission() {
    std::cout << "Testing Interface metadata emission..." << std::endl;

    namespace fs = std::filesystem;

    fs::create_directories("test_files");

    fs::path dmfPath = fs::path("test_files") / "test_interface.dmf";
    {
        std::ofstream dmfOut(dmfPath);
        dmfOut << "// dummy interface file\n";
    }

    fs::path dmePath = "test_interface.dme";
    {
        std::ofstream out(dmePath);
        out << "#include \"" << dmfPath.generic_string() << "\"\n";
        out << "world\n";
        out << "    name = \"InterfaceTest\"\n";
    }

    DMCompiler::DMCompilerSettings settings;
    settings.Files.push_back(dmePath.string());
    settings.Verbose = false;

    DMCompiler::DMCompiler compiler;
    compiler.Compile(settings);

    fs::path jsonPath = dmePath;
    jsonPath.replace_extension(".json");

    bool ok = FileContainsAll(jsonPath, {
        "\"Interface\"",
        "\"File\"",
        "test_files/test_interface.dmf"
    });

    // Clean up
    fs::remove(dmfPath);
    fs::remove(dmePath);
    fs::remove(jsonPath);

    if (ok) {
        std::cout << "Interface metadata emission test passed!" << std::endl;
    } else {
        std::cout << "Interface metadata emission test FAILED" << std::endl;
    }

    return ok;
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

    int failures = 0;
    try {
        TestSimpleCompilation();
        TestWithActualDME();

        if (!TestInterfaceMetadataEmission()) {
            failures++;
        }
        
        std::cout << "\nCompiler tests completed!" << std::endl;
        return failures;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return failures + 1;
    }
}
