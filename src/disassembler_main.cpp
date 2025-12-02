#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include "DMDisassembler.h"

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
    
    DMCompiler::DMDisassembler disassembler;
    if (!disassembler.LoadJson(jsonFile)) {
        std::cerr << "Error: Failed to load JSON file" << std::endl;
        return 1;
    }
    
    if (argc == 3) {
        std::string command = argv[2];
        
        if (command == "crash-on-test") {
            std::cout << "Testing disassembly of all procs..." << std::endl;
            int failures = disassembler.TestAll();
            if (failures > 0) {
                std::cerr << "Disassembly failed for " << failures << " procs." << std::endl;
                return 1;
            }
            std::cout << "No errors detected. Exiting cleanly." << std::endl;
            return 0;
        }
        else if (command == "dump-all") {
            std::cout << "Dumping all types and procs..." << std::endl;
            disassembler.DumpAll(std::cout);
            return 0;
        }
        else {
            std::cerr << "Unknown command: " << command << std::endl;
            return 1;
        }
    }
    
    // Interactive mode
    std::cout << "\nEnter a command or \"help\" for more information." << std::endl;
    
    const DMCompiler::DisasmType* selectedType = nullptr;
    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        
        if (input.empty()) {
            continue;
        }
        
        std::stringstream ss(input);
        std::string cmd;
        ss >> cmd;
        
        if (cmd == "quit" || cmd == "exit" || cmd == "q") {
            break;
        }
        else if (cmd == "help" || cmd == "h") {
            PrintHelp();
        }
        else if (cmd == "stats") {
            auto stats = disassembler.GetStats();
            std::cout << "Types: " << stats.TypeCount << "\n";
            std::cout << "Procs: " << stats.ProcCount << "\n";
            std::cout << "Strings: " << stats.StringCount << "\n";
            std::cout << "Total Bytecode: " << stats.TotalBytecodeSize << " bytes\n";
        }
        else if (cmd == "search") {
            std::string query;
            ss >> query;
            if (query.empty()) {
                std::cout << "Usage: search [name]" << std::endl;
                continue;
            }
            
            auto results = disassembler.Search(query);
            std::cout << "Found " << results.size() << " results:\n";
            for (size_t i = 0; i < std::min(results.size(), size_t(20)); ++i) {
                std::cout << "  " << results[i] << "\n";
            }
            if (results.size() > 20) {
                std::cout << "  ... and " << (results.size() - 20) << " more\n";
            }
        }
        else if (cmd == "select") {
            std::string path;
            ss >> path;
            if (path.empty()) {
                std::cout << "Usage: select [path]" << std::endl;
                continue;
            }
            
            const auto* type = disassembler.GetType(path);
            if (type) {
                selectedType = type;
                std::cout << "Selected type: " << type->Path << " (ID: " << type->Id << ")\n";
            } else {
                std::cout << "Type not found: " << path << "\n";
            }
        }
        else if (cmd == "list") {
            if (!selectedType) {
                std::cout << "No type selected. Use 'select [path]' first.\n";
                continue;
            }
            
            std::cout << "Procs for " << selectedType->Path << ":\n";
            auto procs = disassembler.GetProcsForType(selectedType->Path);
            for (const auto* proc : procs) {
                std::cout << "  [" << proc->Id << "] " << proc->Name << "\n";
            }
        }
        else if (cmd == "decompile") {
            int id;
            if (ss >> id) {
                std::cout << disassembler.DecompileProc(id) << "\n";
            } else {
                std::cout << "Usage: decompile [id]" << std::endl;
            }
        }
        else {
            std::cout << "Unknown command: " << cmd << std::endl;
            std::cout << "Type 'help' for available commands" << std::endl;
        }
    }
    
    return 0;
}
