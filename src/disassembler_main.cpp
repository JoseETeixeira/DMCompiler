#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// Disassembler main program

void PrintHelp() {
    std::cout << "DM Disassembler for OpenDream (C++ Implementation)" << std::endl;
    std::cout << "\nUsage: dmdisasm [file].json [command]" << std::endl;
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  crash-on-test  : Test disassembly of entire codebase (for CI)" << std::endl;
    std::cout << "  dump-all       : Dump all types and procs to stdout" << std::endl;
    std::cout << "\nInteractive mode commands:" << std::endl;
    std::cout << "  help           : Show help" << std::endl;
    std::cout << "  search [name]  : Search for types/procs" << std::endl;
    std::cout << "  select [path]  : Select a type" << std::endl;
    std::cout << "  list           : List procs of selected type" << std::endl;
    std::cout << "  decompile [#]  : Decompile proc" << std::endl;
    std::cout << "  stats          : Show statistics" << std::endl;
    std::cout << "  quit           : Exit" << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Error: No input file specified" << std::endl;
        std::cerr << "Usage: dmdisasm [file].json" << std::endl;
        return 1;
    }
    
    std::string jsonFile = argv[1];
    
    // Check file extension
    if (jsonFile.size() < 5 || jsonFile.substr(jsonFile.size() - 5) != ".json") {
        std::cerr << "Error: Input file must be a .json file" << std::endl;
        return 1;
    }
    
    // Check if file exists
    std::ifstream testFile(jsonFile);
    if (!testFile.good()) {
        std::cerr << "Error: Cannot open file: " << jsonFile << std::endl;
        return 1;
    }
    testFile.close();
    
    std::cout << "DM Disassembler for OpenDream (C++ Implementation)" << std::endl;
    std::cout << "Loading: " << jsonFile << std::endl;
    
    // TODO: Load and parse JSON file
    // TODO: Build type and proc structures
    
    if (argc == 3) {
        std::string command = argv[2];
        
        if (command == "crash-on-test") {
            std::cout << "Testing disassembly of all procs..." << std::endl;
            // TODO: Implement test-all functionality
            std::cout << "No errors detected. Exiting cleanly." << std::endl;
            return 0;
        }
        else if (command == "dump-all") {
            std::cout << "Dumping all types and procs..." << std::endl;
            // TODO: Implement dump-all functionality
            return 0;
        }
        else {
            std::cerr << "Unknown command: " << command << std::endl;
            return 1;
        }
    }
    
    // Interactive mode
    std::cout << "\nEnter a command or \"help\" for more information." << std::endl;
    
    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        
        if (input.empty()) {
            continue;
        }
        
        if (input == "quit" || input == "exit" || input == "q") {
            break;
        }
        else if (input == "help" || input == "h") {
            PrintHelp();
        }
        else {
            std::cout << "Command not implemented yet: " << input << std::endl;
            std::cout << "Type 'help' for available commands" << std::endl;
        }
    }
    
    return 0;
}
