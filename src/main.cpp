#include "DMCompiler.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>

void PrintHelp() {
    std::cout << "DM Compiler for OpenDream (C++ Implementation)" << std::endl;
    std::cout << "For more information please visit https://github.com/OpenDreamProject/OpenDream/wiki" << std::endl;
    std::cout << "\nUsage: dmcompiler [options] [file].dme\n" << std::endl;
    std::cout << "Options and arguments:" << std::endl;
    std::cout << "  --help                    : Show this help" << std::endl;
    std::cout << "  --version [VER].[BUILD]   : Used to set the DM_VERSION and DM_BUILD macros" << std::endl;
    std::cout << "  --skip-bad-args           : Skip arguments the compiler doesn't recognize" << std::endl;
    std::cout << "  --suppress-unimplemented  : Do not warn about unimplemented proc and var uses" << std::endl;
    std::cout << "  --suppress-unsupported    : Do not warn about proc and var uses that will not be supported" << std::endl;
    std::cout << "  --dump-preprocessor       : Save the result of preprocessing in a file" << std::endl;
    std::cout << "  --no-standard             : Disable built-in standard library" << std::endl;
    std::cout << "  --define [KEY=VAL]        : Add extra defines to the compilation" << std::endl;
    std::cout << "  --lib-path [PATH]         : Add a path to search for external libraries" << std::endl;
    std::cout << "  --verbose                 : Show verbose output during compile" << std::endl;
    std::cout << "  --notices-enabled         : Show notice output during compile" << std::endl;
    std::cout << "  --no-opts                 : Disable compiler optimizations (debug only)" << std::endl;
}

bool ParseArguments(int argc, char** argv, DMCompiler::DMCompilerSettings& settings) {
    bool skipBadArgs = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            PrintHelp();
            return false;
        }
        else if (arg == "--skip-bad-args") {
            skipBadArgs = true;
        }
        else if (arg == "--suppress-unimplemented") {
            settings.SuppressUnimplementedWarnings = true;
        }
        else if (arg == "--suppress-unsupported") {
            settings.SuppressUnsupportedAccessWarnings = true;
        }
        else if (arg == "--dump-preprocessor") {
            settings.DumpPreprocessor = true;
        }
        else if (arg == "--no-standard") {
            settings.NoStandard = true;
        }
        else if (arg == "--verbose") {
            settings.Verbose = true;
        }
        else if (arg == "--notices-enabled") {
            settings.NoticesEnabled = true;
        }
        else if (arg == "--no-opts") {
            settings.NoOpts = true;
        }
        else if (arg == "--skip-anything-typecheck") {
            settings.SkipAnythingTypecheck = true;
        }
        else if (arg == "--version" && i + 1 < argc) {
            std::string version = argv[++i];
            settings.MacroDefines["DM_VERSION"] = version;
        }
        else if (arg == "--define" && i + 1 < argc) {
            std::string define = argv[++i];
            size_t eqPos = define.find('=');
            if (eqPos != std::string::npos) {
                std::string key = define.substr(0, eqPos);
                std::string value = define.substr(eqPos + 1);
                settings.MacroDefines[key] = value;
            } else {
                settings.MacroDefines[define] = "1";
            }
        }
        else if (arg == "--lib-path" && i + 1 < argc) {
            settings.LibraryPaths.push_back(argv[++i]);
        }
        else if (arg[0] == '-') {
            if (!skipBadArgs) {
                std::cerr << "Unknown argument: " << arg << std::endl;
                return false;
            }
        }
        else {
            // Check if it's a .dme or .dm file
            if (arg.size() >= 3) {
                std::string ext = arg.substr(arg.size() - 3);
                if (ext == ".dm" || (arg.size() >= 4 && arg.substr(arg.size() - 4) == ".dme")) {
                    settings.Files.push_back(arg);
                } else if (!skipBadArgs) {
                    std::cerr << "Invalid file extension: " << arg << std::endl;
                    return false;
                }
            }
        }
    }
    
    if (settings.Files.empty()) {
        std::cerr << "Error: No input files specified" << std::endl;
        std::cerr << "Use --help for usage information" << std::endl;
        return false;
    }
    
    return true;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Error: No input files specified" << std::endl;
        std::cerr << "Use --help for usage information" << std::endl;
        return 1;
    }
    
    DMCompiler::DMCompilerSettings settings;
    if (!ParseArguments(argc, argv, settings)) {
        return 1;
    }
    
    DMCompiler::DMCompiler compiler;
    if (!compiler.Compile(settings)) {
        return 1;
    }
    
    return 0;
}
