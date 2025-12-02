#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <set>
#include <chrono>
#include "Location.h"
#include "Token.h"
#include "DreamPath.h"

namespace DMCompiler {

// Forward declarations
class DMObjectTree;
class DMCodeTree;
class DMProc;
class DMObject;
class DMASTFile;
class DMASTStatement;
class DMASTObjectDefinition;
class DMASTObjectVarDefinition;
class DMASTObjectVarOverride;
class DMASTObjectProcDefinition;
class DMASTExpression;
struct DreamMapJson;

/// <summary>
/// Compiler settings
/// </summary>
struct DMCompilerSettings {
    std::vector<std::string> Files;
    std::unordered_map<std::string, std::string> MacroDefines;
    bool SuppressUnimplementedWarnings = false;
    bool SuppressUnsupportedAccessWarnings = false;
    bool SkipAnythingTypecheck = false;
    bool DumpPreprocessor = false;
    bool NoStandard = false;
    bool Verbose = false;
    bool NoticesEnabled = false;
    bool NoOpts = false;
};

/// <summary>
/// Warning and error codes
/// </summary>
enum class WarningCode {
    SoftReservedKeyword,
    UnimplementedAccess,
    UnsupportedAccess,
    MissingInterpolatedExpression,
    PointlessParentCall,
    PointlessBuiltinCall,
    SuspiciousMatrixCall,
    FallbackBuiltinArgument,
    MalformedRange,
    InvalidRange,
    BadExpression,
    BadStatement,
    BadToken,
    BadDefinition,
    SoftConstant,
    HardConstant,
    WriteToConstant,
    DuplicateVariable,
    DuplicateProcDefinition,
    PointlessScope,
    MalformedMacro,
    UnsupportedTypeCheck,
    UnknownVariable,
    Unknown
};

enum class ErrorLevel {
    Disabled,
    Notice,
    Warning,
    Error
};

/// <summary>
/// Main DM Compiler class
/// </summary>
class DMCompiler {
public:
    DMCompiler();
    ~DMCompiler();

    // Main compilation entry point
    bool Compile(const DMCompilerSettings& settings);

    // Error/warning reporting
    void Emit(WarningCode code, const Location& location, const std::string& message, const std::string& context = "");
    void ForcedWarning(const std::string& message);
    void ForcedError(const Location& location, const std::string& message);
    void SetPragma(WarningCode code, ErrorLevel level);
    
    // Error limits
    void SetMaxErrors(int maxErrors);
    bool ShouldAbort() const;

    // Progress tracking
    void CheckProgress(const std::string& phase);

    // Resource management
    void AddResourceDirectory(const std::string& dir, const Location& loc);
    
    // Accessors
    DMObjectTree* GetObjectTree() { return ObjectTree_.get(); }
    DMCodeTree* GetCodeTree() { return CodeTree_.get(); }
    const DMCompilerSettings& GetSettings() const { return Settings_; }
    
    const std::set<std::string>& GetResourceDirectories() const { return ResourceDirectories_; }
    const std::vector<std::string>& GetCompilerMessages() const { return CompilerMessages_; }

private:
    DMCompilerSettings Settings_;
    std::unique_ptr<DMObjectTree> ObjectTree_;
    std::unique_ptr<DMCodeTree> CodeTree_;
    std::unique_ptr<DMProc> GlobalInitProc_;
    
    std::set<std::string> ResourceDirectories_;
    std::vector<std::string> CompilerMessages_;
    std::unordered_map<WarningCode, ErrorLevel> ErrorConfig_;
    
    std::set<WarningCode> UniqueEmissions_;
    int ErrorCount_;
    int WarningCount_;
    int MaxErrors_ = 100; // Default limit
    bool Aborted_ = false;
    
    std::string CodeDirectory_;
    
    // Preprocessor results
    std::vector<std::string> IncludedMaps_;
    std::string IncludedInterface_;
    
    // Parsed maps
    std::vector<std::unique_ptr<DreamMapJson>> ParsedMaps_;
    
    // Internal compilation phases
    bool PreprocessFiles();
    bool ParseFiles();
    bool BuildObjectTree();
    bool EmitBytecode();
    bool OutputJson(const std::string& outputPath);
    
    // Progress tracking
    std::chrono::steady_clock::time_point StartTime_;
    std::chrono::steady_clock::time_point LastProgressTime_;

    // DMStandard initialization
    bool InitializeDMStandard();
    
    // Map conversion
    std::vector<std::unique_ptr<struct DreamMapJson>> ConvertMaps(const std::vector<std::string>& mapPaths, int& zOffset);
    
    // Helper methods for BuildObjectTree
    bool ProcessObjectStatement(DMASTStatement* statement, const DreamPath& currentPath);
    bool ProcessObjectDefinition(DMASTObjectDefinition* objDef, const DreamPath& currentPath);
    bool ProcessVarDefinition(DMASTObjectVarDefinition* varDef, const DreamPath& currentPath);
    bool ProcessVarOverride(DMASTObjectVarOverride* varOverride, const DreamPath& currentPath);
    bool ProcessProcDefinition(DMASTObjectProcDefinition* procDef, const DreamPath& currentPath);
    
    /// Update a variable's default value
    /// Searches the inheritance chain to find the original variable definition,
    /// then creates an override with the new default value in the target object.
    /// @param obj The object where the override is declared
    /// @param varName Name of the variable to override
    /// @param newValue The new default value expression
    /// @param location Source location for error reporting
    /// @return true if successful, false if variable not found
    bool UpdateVariableDefault(DMObject* obj, const std::string& varName, 
                               DMASTExpression* newValue, const Location& location);
    
    std::vector<Token> PreprocessedTokens_;
    std::unique_ptr<DMASTFile> ParsedAST_;  // Parsed Abstract Syntax Tree
};

} // namespace DMCompiler
