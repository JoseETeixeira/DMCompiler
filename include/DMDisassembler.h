#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <iostream>

namespace DMCompiler {

/// Loaded type information for disassembly
struct DisasmType {
    int Id;                                 ///< Type ID in the compiled output
    std::string Path;                       ///< Full type path (e.g., "/mob/player")
    int ParentId;                           ///< Parent type ID (-1 for root)
    std::vector<std::string> Variables;     ///< Variable names defined on this type
    std::vector<int> ProcIds;               ///< IDs of procs defined on this type
    
    DisasmType() : Id(-1), ParentId(-1) {}
};

/// Loaded proc information for disassembly
struct DisasmProc {
    int Id;                                 ///< Proc ID in the compiled output
    std::string Name;                       ///< Proc name
    std::string OwnerPath;                  ///< Owner type path
    int OwnerTypeId;                        ///< Owner type ID
    std::vector<std::string> Parameters;    ///< Parameter names
    std::vector<uint8_t> Bytecode;          ///< Compiled bytecode
    bool IsVerb;                            ///< Whether this is a verb
    
    DisasmProc() : Id(-1), OwnerTypeId(-1), IsVerb(false) {}
};

/// Main disassembler class for inspecting compiled DM JSON output
class DMDisassembler {
public:
    DMDisassembler();
    ~DMDisassembler();
    
    /// Load compiled JSON file
    /// @param jsonPath Path to the .json file
    /// @return true on success, false on parse error
    bool LoadJson(const std::string& jsonPath);
    
    /// Check if data has been loaded
    bool IsLoaded() const { return loaded_; }
    
    /// Get the loaded file path
    const std::string& GetFilePath() const { return filePath_; }
    
    /// Search for types/procs by name (partial match)
    /// @param query Search string
    /// @return List of matching type/proc paths
    std::vector<std::string> Search(const std::string& query) const;
    
    /// Get type by path
    /// @param path Full type path (e.g., "/mob/player")
    /// @return Pointer to type, or nullptr if not found
    const DisasmType* GetType(const std::string& path) const;
    
    /// Get type by ID
    /// @param typeId Type ID
    /// @return Pointer to type, or nullptr if not found
    const DisasmType* GetTypeById(int typeId) const;
    
    /// Get proc by ID
    /// @param procId Proc ID
    /// @return Pointer to proc, or nullptr if not found
    const DisasmProc* GetProc(int procId) const;
    
    /// Get all procs for a type
    /// @param typePath Type path
    /// @return Vector of proc pointers
    std::vector<const DisasmProc*> GetProcsForType(const std::string& typePath) const;
    
    /// Decompile a proc to human-readable output
    /// @param procId Proc ID to decompile
    /// @return Decompiled string representation
    std::string DecompileProc(int procId) const;
    
    /// Dump all types and procs to output stream
    /// @param out Output stream (e.g., std::cout)
    void DumpAll(std::ostream& out) const;
    
    /// Test disassembly of all procs (for CI)
    /// @return Number of failed procs
    int TestAll() const;
    
    /// Get statistics about loaded data
    struct Stats {
        size_t TypeCount;
        size_t ProcCount;
        size_t StringCount;
        size_t TotalBytecodeSize;
    };
    Stats GetStats() const;
    
    /// Get string from string table
    /// @param index String table index
    /// @return String value, or empty string if invalid index
    const std::string& GetString(size_t index) const;
    
    /// Get all types
    const std::vector<DisasmType>& GetTypes() const { return types_; }
    
    /// Get all procs
    const std::vector<DisasmProc>& GetProcs() const { return procs_; }
    
private:
    bool loaded_;
    std::string filePath_;
    std::vector<DisasmType> types_;
    std::vector<DisasmProc> procs_;
    std::unordered_map<std::string, int> pathToTypeId_;
    std::vector<std::string> stringTable_;
    
    /// Helper to parse the JSON content
    bool ParseJson(const std::string& content);
    
    /// Build lookup tables after loading
    void BuildLookupTables();
};

} // namespace DMCompiler
