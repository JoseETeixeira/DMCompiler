#include "DMDisassembler.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace DMCompiler {

// Static empty string for returning references
static const std::string EmptyString;

DMDisassembler::DMDisassembler()
    : loaded_(false)
{
}

DMDisassembler::~DMDisassembler() = default;

bool DMDisassembler::LoadJson(const std::string& jsonPath) {
    // Read file content
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    // Parse the JSON content
    if (!ParseJson(content)) {
        return false;
    }
    
    // Build lookup tables
    BuildLookupTables();
    
    filePath_ = jsonPath;
    loaded_ = true;
    return true;
}

bool DMDisassembler::ParseJson(const std::string& content) {
    // TODO: Implement JSON parsing in Task 11
    // For now, just return true to indicate the file was loaded
    // The actual parsing will use nlohmann_json or custom parsing
    (void)content; // Suppress unused parameter warning
    return true;
}

void DMDisassembler::BuildLookupTables() {
    // Build path to type ID lookup
    pathToTypeId_.clear();
    for (const auto& type : types_) {
        pathToTypeId_[type.Path] = type.Id;
    }
}

std::vector<std::string> DMDisassembler::Search(const std::string& query) const {
    std::vector<std::string> results;
    
    if (query.empty()) {
        return results;
    }
    
    // Convert query to lowercase for case-insensitive search
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    // Search types
    for (const auto& type : types_) {
        std::string lowerPath = type.Path;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        
        if (lowerPath.find(lowerQuery) != std::string::npos) {
            results.push_back(type.Path);
        }
    }
    
    // Search procs
    for (const auto& proc : procs_) {
        std::string lowerName = proc.Name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        
        if (lowerName.find(lowerQuery) != std::string::npos) {
            results.push_back(proc.OwnerPath + "/" + proc.Name);
        }
    }
    
    return results;
}

const DisasmType* DMDisassembler::GetType(const std::string& path) const {
    auto it = pathToTypeId_.find(path);
    if (it != pathToTypeId_.end()) {
        return GetTypeById(it->second);
    }
    return nullptr;
}

const DisasmType* DMDisassembler::GetTypeById(int typeId) const {
    if (typeId >= 0 && static_cast<size_t>(typeId) < types_.size()) {
        return &types_[typeId];
    }
    return nullptr;
}

const DisasmProc* DMDisassembler::GetProc(int procId) const {
    if (procId >= 0 && static_cast<size_t>(procId) < procs_.size()) {
        return &procs_[procId];
    }
    return nullptr;
}

std::vector<const DisasmProc*> DMDisassembler::GetProcsForType(const std::string& typePath) const {
    std::vector<const DisasmProc*> result;
    
    const DisasmType* type = GetType(typePath);
    if (type) {
        for (int procId : type->ProcIds) {
            const DisasmProc* proc = GetProc(procId);
            if (proc) {
                result.push_back(proc);
            }
        }
    }
    
    return result;
}

std::string DMDisassembler::DecompileProc(int procId) const {
    const DisasmProc* proc = GetProc(procId);
    if (!proc) {
        return "Error: Proc not found";
    }
    
    std::stringstream ss;
    ss << "// Proc: " << proc->OwnerPath << "/" << proc->Name << "\n";
    ss << "// Parameters: ";
    for (size_t i = 0; i < proc->Parameters.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << proc->Parameters[i];
    }
    ss << "\n";
    ss << "// Bytecode size: " << proc->Bytecode.size() << " bytes\n";
    
    // TODO: Implement actual bytecode decompilation in Task 12
    // For now, just show bytecode as hex dump
    if (!proc->Bytecode.empty()) {
        ss << "\nBytecode hex dump:\n";
        for (size_t i = 0; i < proc->Bytecode.size(); ++i) {
            if (i > 0 && i % 16 == 0) ss << "\n";
            char hex[4];
            snprintf(hex, sizeof(hex), "%02X ", proc->Bytecode[i]);
            ss << hex;
        }
        ss << "\n";
    }
    
    return ss.str();
}

void DMDisassembler::DumpAll(std::ostream& out) const {
    out << "=== DMDisassembler Dump ===\n\n";
    
    // Statistics
    auto stats = GetStats();
    out << "Statistics:\n";
    out << "  Types: " << stats.TypeCount << "\n";
    out << "  Procs: " << stats.ProcCount << "\n";
    out << "  Strings: " << stats.StringCount << "\n";
    out << "  Total Bytecode: " << stats.TotalBytecodeSize << " bytes\n\n";
    
    // Types
    out << "Types:\n";
    for (const auto& type : types_) {
        out << "  [" << type.Id << "] " << type.Path;
        if (type.ParentId >= 0) {
            out << " (parent: " << type.ParentId << ")";
        }
        out << "\n";
        
        if (!type.Variables.empty()) {
            out << "    Variables: ";
            for (size_t i = 0; i < type.Variables.size(); ++i) {
                if (i > 0) out << ", ";
                out << type.Variables[i];
            }
            out << "\n";
        }
        
        if (!type.ProcIds.empty()) {
            out << "    Procs: ";
            for (size_t i = 0; i < type.ProcIds.size(); ++i) {
                if (i > 0) out << ", ";
                const DisasmProc* proc = GetProc(type.ProcIds[i]);
                if (proc) {
                    out << proc->Name;
                } else {
                    out << "#" << type.ProcIds[i];
                }
            }
            out << "\n";
        }
    }
    
    // Procs
    out << "\nProcs:\n";
    for (const auto& proc : procs_) {
        out << "  [" << proc.Id << "] " << proc.OwnerPath << "/" << proc.Name;
        if (proc.IsVerb) out << " (verb)";
        out << "\n";
        
        if (!proc.Parameters.empty()) {
            out << "    Parameters: ";
            for (size_t i = 0; i < proc.Parameters.size(); ++i) {
                if (i > 0) out << ", ";
                out << proc.Parameters[i];
            }
            out << "\n";
        }
        
        out << "    Bytecode: " << proc.Bytecode.size() << " bytes\n";
    }
}

int DMDisassembler::TestAll() const {
    int failures = 0;
    
    // Try to decompile each proc
    for (const auto& proc : procs_) {
        try {
            std::string result = DecompileProc(proc.Id);
            if (result.empty() || result.find("Error:") == 0) {
                failures++;
            }
        } catch (...) {
            failures++;
        }
    }
    
    return failures;
}

DMDisassembler::Stats DMDisassembler::GetStats() const {
    Stats stats;
    stats.TypeCount = types_.size();
    stats.ProcCount = procs_.size();
    stats.StringCount = stringTable_.size();
    
    stats.TotalBytecodeSize = 0;
    for (const auto& proc : procs_) {
        stats.TotalBytecodeSize += proc.Bytecode.size();
    }
    
    return stats;
}

const std::string& DMDisassembler::GetString(size_t index) const {
    if (index < stringTable_.size()) {
        return stringTable_[index];
    }
    return EmptyString;
}

} // namespace DMCompiler
