#include "../include/DMCompiler.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

int main() {
    std::cout << "Testing istype variadic compilation..." << std::endl;

    // Path to the DM file created earlier
    // The build system copies test_files to the binary directory, but we created it in the source directory.
    // We should look for it in the source directory or assume it's copied.
    // For now, let's try to find it relative to the current working directory or create it if missing.
    
    std::string dmFileName = "test_istype_variadic.dm";
    std::string dmFilePath = "";

    if (fs::exists(dmFileName)) {
        dmFilePath = dmFileName;
    } else if (fs::exists("../tests/" + dmFileName)) {
        dmFilePath = "../tests/" + dmFileName;
    } else if (fs::exists("../../tests/" + dmFileName)) {
        dmFilePath = "../../tests/" + dmFileName;
    } else {
        // Create it if it doesn't exist (fallback)
        std::cout << "Creating temporary test file..." << std::endl;
        std::ofstream out(dmFileName);
        out << "// Test istype with 1 argument (implicit src)\n";
        out << "proc/test_implicit_src(datum/D) {\n";
        out << "    if (istype(D)) {\n";
        out << "        return 1\n";
        out << "    }\n";
        out << "    return 0\n";
        out << "}\n";
        out << "\n";
        out << "// Test input with extra arguments\n";
        out << "proc/test_input_extra(mob/M) {\n";
        out << "    var/x = input(M, \"Message\", \"Title\", \"Default\") as text\n";
        out << "    var/y = input(M, \"Message\", \"Title\", \"Default\", \"Extra\") as text\n";
        out << "}\n";
        out.close();
        dmFilePath = dmFileName;
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
