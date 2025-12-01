#include "DMCompiler.h"
#include "DMObjectTree.h"
#include "DMCodeTree.h"
#include "DMCodeTreeBuilder.h"
#include "DMProc.h"
#include "DMPreprocessor.h"
#include "DMParser.h"
#include "DMMParser.h"
#include "DMLexer.h"
#include "DMASTFolder.h"
#include "TokenStreamDMLexer.h"
#include "DMASTStatement.h"
#include "DMObject.h"
#include "DMVariable.h"
#include "BytecodeWriter.h"
#include "DMExpressionCompiler.h"
#include "DMStatementCompiler.h"
#include "JsonWriter.h"
#include "DMConstants.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>

// Platform-specific includes for executable path
#ifdef _WIN32
#include <windows.h>
// Undefine Windows macros that conflict with std::
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#elif defined(__linux__)
#include <unistd.h>
#include <linux/limits.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#endif

namespace DMCompiler {

DMCompiler::DMCompiler()
    : ErrorCount_(0)
    , WarningCount_(0)
    , ObjectTree_(std::make_unique<DMObjectTree>(this))
    , CodeTree_(std::make_unique<DMCodeTree>())
{
    // Initialize default error configuration
    ErrorConfig_[WarningCode::UnimplementedAccess] = ErrorLevel::Warning;
    ErrorConfig_[WarningCode::UnsupportedAccess] = ErrorLevel::Warning;
    ErrorConfig_[WarningCode::BadExpression] = ErrorLevel::Error;
    ErrorConfig_[WarningCode::BadStatement] = ErrorLevel::Error;
    ErrorConfig_[WarningCode::BadToken] = ErrorLevel::Error;
    ErrorConfig_[WarningCode::BadDefinition] = ErrorLevel::Error;
    ErrorConfig_[WarningCode::WriteToConstant] = ErrorLevel::Error;
    // Add more default configurations as needed
}

DMCompiler::~DMCompiler() = default;

bool DMCompiler::Compile(const DMCompilerSettings& settings) {
    Settings_ = settings;
    
    StartTime_ = std::chrono::steady_clock::now();
    LastProgressTime_ = StartTime_;
    
    std::cout << "OpenDream DM Compiler (C++ Implementation)" << std::endl;
    std::cout << "Compiling: " << settings.Files[0] << std::endl;
    
    if (Settings_.NoOpts) {
        ForcedWarning("Compiler optimizations disabled via --no-opts");
    }
    
    if (Settings_.SuppressUnimplementedWarnings) {
        Emit(WarningCode::UnimplementedAccess, Location::Internal,
             "Unimplemented proc & var warnings are suppressed");
    }
    
    // Compilation phases
    bool success = true;
    
    auto phaseStart = std::chrono::steady_clock::now();
    if (success && !ShouldAbort() && !PreprocessFiles()) {
        success = false;
    }
    if (Settings_.Verbose) {
        auto phaseEnd = std::chrono::steady_clock::now();
        std::cout << "Preprocessing took " << std::chrono::duration_cast<std::chrono::milliseconds>(phaseEnd - phaseStart).count() << "ms" << std::endl;
    }
    
    phaseStart = std::chrono::steady_clock::now();
    if (success && !ShouldAbort() && !InitializeDMStandard()) {
        success = false;
    }
    if (Settings_.Verbose) {
        auto phaseEnd = std::chrono::steady_clock::now();
        std::cout << "DMStandard init took " << std::chrono::duration_cast<std::chrono::milliseconds>(phaseEnd - phaseStart).count() << "ms" << std::endl;
    }
    
    phaseStart = std::chrono::steady_clock::now();
    if (success && !ShouldAbort() && !ParseFiles()) {
        success = false;
    }
    if (Settings_.Verbose) {
        auto phaseEnd = std::chrono::steady_clock::now();
        std::cout << "Parsing took " << std::chrono::duration_cast<std::chrono::milliseconds>(phaseEnd - phaseStart).count() << "ms" << std::endl;
    }
    
    phaseStart = std::chrono::steady_clock::now();
    if (success && !ShouldAbort() && !BuildObjectTree()) {
        success = false;
    }
    if (Settings_.Verbose) {
        auto phaseEnd = std::chrono::steady_clock::now();
        std::cout << "Object tree build took " << std::chrono::duration_cast<std::chrono::milliseconds>(phaseEnd - phaseStart).count() << "ms" << std::endl;
    }
    
    if (success && !ShouldAbort() && !IncludedMaps_.empty()) {
        phaseStart = std::chrono::steady_clock::now();
        std::cout << "Phase 5: Converting maps..." << std::endl;
        int zOffset = 1; // Start Z offset at 1
        ParsedMaps_ = ConvertMaps(IncludedMaps_, zOffset);
        
        if (Settings_.Verbose) {
            std::cout << "  Converted " << ParsedMaps_.size() << " maps" << std::endl;
            auto phaseEnd = std::chrono::steady_clock::now();
            std::cout << "Map conversion took " << std::chrono::duration_cast<std::chrono::milliseconds>(phaseEnd - phaseStart).count() << "ms" << std::endl;
        }
    }
    
    phaseStart = std::chrono::steady_clock::now();
    if (success && !ShouldAbort() && !EmitBytecode()) {
        success = false;
    }
    if (Settings_.Verbose) {
        auto phaseEnd = std::chrono::steady_clock::now();
        std::cout << "Bytecode emission took " << std::chrono::duration_cast<std::chrono::milliseconds>(phaseEnd - phaseStart).count() << "ms" << std::endl;
    }
    
    phaseStart = std::chrono::steady_clock::now();
    if (success && !ShouldAbort() && !OutputJson(settings.Files[0])) {
        success = false;
    }
    if (Settings_.Verbose) {
        auto phaseEnd = std::chrono::steady_clock::now();
        std::cout << "JSON output took " << std::chrono::duration_cast<std::chrono::milliseconds>(phaseEnd - phaseStart).count() << "ms" << std::endl;
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - StartTime_);
    
    if (success && ErrorCount_ == 0) {
        std::cout << "Compilation succeeded with " << WarningCount_ << " warnings" << std::endl;
    } else {
        std::cout << "Compilation failed with " << ErrorCount_ << " errors and " 
                  << WarningCount_ << " warnings" << std::endl;
    }
    
    std::cout << "Total time: " << duration.count() << "s" << std::endl;
    
    return success && (ErrorCount_ == 0);
}

void DMCompiler::Emit(WarningCode code, const Location& location, const std::string& message, const std::string& context) {
    if (Aborted_) return;

    if (UniqueEmissions_.find(code) != UniqueEmissions_.end()) {
        return; // Already emitted this warning
    }
    
    auto it = ErrorConfig_.find(code);
    ErrorLevel level = (it != ErrorConfig_.end()) ? it->second : ErrorLevel::Warning;
    
    if (level == ErrorLevel::Disabled) {
        return;
    }
    
    std::string levelStr;
    switch (level) {
        case ErrorLevel::Notice: levelStr = "Notice"; break;
        case ErrorLevel::Warning: levelStr = "Warning"; WarningCount_++; break;
        case ErrorLevel::Error: levelStr = "Error"; ErrorCount_++; break;
        default: levelStr = "Unknown"; break;
    }
    
    std::string fullMessage = location.ToString() + ": " + levelStr + ": " + message;
    if (!context.empty()) {
        fullMessage += "\n    Context: " + context;
    }
    
    std::cerr << fullMessage << std::endl;
    CompilerMessages_.push_back(fullMessage);
    
    UniqueEmissions_.insert(code);

    if (ErrorCount_ >= MaxErrors_ && MaxErrors_ > 0) {
        std::cerr << "Fatal: Error limit reached (" << MaxErrors_ << "). Aborting compilation." << std::endl;
        Aborted_ = true;
    }
}

void DMCompiler::SetMaxErrors(int maxErrors) {
    MaxErrors_ = maxErrors;
}

bool DMCompiler::ShouldAbort() const {
    return Aborted_;
}

void DMCompiler::ForcedWarning(const std::string& message) {
    std::cerr << "Warning: " << message << std::endl;
    CompilerMessages_.push_back("Warning: " + message);
    WarningCount_++;
}

void DMCompiler::ForcedError(const Location& location, const std::string& message) {
    std::string fullMessage = location.ToString() + ": Error: " + message;
    std::cerr << fullMessage << std::endl;
    CompilerMessages_.push_back(fullMessage);
    ErrorCount_++;
}

void DMCompiler::SetPragma(WarningCode code, ErrorLevel level) {
    ErrorConfig_[code] = level;
}

void DMCompiler::AddResourceDirectory(const std::string& dir, const Location& loc) {
    namespace fs = std::filesystem;
    
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        ForcedError(loc, "Resource directory does not exist: " + dir);
        return;
    }
    
    ResourceDirectories_.insert(dir);
}

bool DMCompiler::PreprocessFiles() {
    if (Settings_.Verbose) {
        std::cout << "Phase 1: Preprocessing files..." << std::endl;
    }

    DMPreprocessor preprocessor;

    // Add custom defines from settings
    for (const auto& [name, value] : Settings_.MacroDefines) {
        if (!value.empty()) {
            preprocessor.Define(name, value);
        } else {
            preprocessor.Define(name, "1");
        }
    }

    // Include DMStandard if not suppressed (matching C# implementation)
    if (!Settings_.NoStandard) {
        namespace fs = std::filesystem;
        
        // Get the directory where the compiler executable is located
        fs::path compilerDir;
#ifdef _WIN32
        // Windows implementation
        wchar_t exePath[MAX_PATH];
        DWORD length = GetModuleFileNameW(NULL, exePath, MAX_PATH);
        if (length == 0 || length == MAX_PATH) {
            ForcedWarning("Failed to get executable path, using current directory");
            compilerDir = fs::current_path();
        } else {
            // Convert wide string to regular string
            std::wstring ws(exePath);
            std::string str(ws.begin(), ws.end());
            compilerDir = fs::path(str).parent_path();
        }
#elif defined(__linux__) || defined(__APPLE__)
        // Unix-like systems
        char exePath[PATH_MAX];
#ifdef __linux__
        ssize_t length = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
#elif defined(__APPLE__)
        uint32_t bufsize = PATH_MAX;
        int length = _NSGetExecutablePath(exePath, &bufsize);
        if (length != 0) length = -1;
        else length = strlen(exePath);
#endif
        if (length == -1) {
            ForcedWarning("Failed to get executable path, using current directory");
            compilerDir = fs::current_path();
        } else {
            exePath[length] = '\0';
            compilerDir = fs::path(exePath).parent_path();
        }
#else
        // Fallback for other platforms
        ForcedWarning("Executable path detection not implemented for this platform, using current directory");
        compilerDir = fs::current_path();
#endif
        fs::path dmStandardDir = compilerDir / "DMStandard";
        fs::path standardFile = dmStandardDir / "_Standard.dm";
        
        if (fs::exists(standardFile)) {
            try {
                if (Settings_.Verbose) {
                    std::cout << "  Including DMStandard: " << standardFile.string() << std::endl;
                }
                
                std::vector<Token> tokens = preprocessor.Preprocess(standardFile.string());
                PreprocessedTokens_.insert(PreprocessedTokens_.end(), tokens.begin(), tokens.end());
                
                if (Settings_.Verbose) {
                    std::cout << "  DMStandard tokens: " << tokens.size() << std::endl;
                }
            } catch (const std::exception& e) {
                ForcedError(Location::Internal, 
                    "Error preprocessing DMStandard: " + std::string(e.what()));
                return false;
            }
        } else {
            ForcedWarning("DMStandard/_Standard.dm not found at: " + standardFile.string());
            ForcedWarning("Compiling without standard library. Use --no-standard to suppress this warning.");
        }
    } else {
        if (Settings_.Verbose) {
            std::cout << "  Skipping DMStandard (--no-standard flag set)" << std::endl;
        }
    }

    // Preprocess all user files
    for (const auto& filePath : Settings_.Files) {
        CheckProgress("Preprocessing");
        try {
            std::vector<Token> tokens = preprocessor.Preprocess(filePath);
            
            // Append to PreprocessedTokens_
            PreprocessedTokens_.insert(PreprocessedTokens_.end(), tokens.begin(), tokens.end());
            
            // Check token limit
            if (PreprocessedTokens_.size() > Limits::MAX_TOKENS) {
                ForcedError(Location::Internal, "Token limit exceeded (" + 
                    std::to_string(Limits::MAX_TOKENS) + " tokens)");
                return false;
            }
            
            if (Settings_.Verbose) {
                std::cout << "  Preprocessed " << filePath << ": " 
                         << tokens.size() << " tokens" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error preprocessing " << filePath << ": " << e.what() << std::endl;
            return false;
        }
    }

    if (Settings_.Verbose) {
        std::cout << "  Total preprocessed tokens: " << PreprocessedTokens_.size() << std::endl;
        
        // DEBUG: Print first 50 tokens to see what's happening
        std::cout << "  First 50 preprocessed tokens:" << std::endl;
        for (size_t i = 0; i < std::min(size_t(50), PreprocessedTokens_.size()); ++i) {
            std::cout << "    [" << i << "] type=" << static_cast<int>(PreprocessedTokens_[i].Type)
                      << " text='" << PreprocessedTokens_[i].Text << "'" << std::endl;
        }
    }

    // Store the included maps and interface from preprocessor
    IncludedMaps_ = preprocessor.GetIncludedMaps();
    IncludedInterface_ = preprocessor.GetIncludedInterface();
    
    if (Settings_.Verbose) {
        std::cout << "  Included maps: " << IncludedMaps_.size() << std::endl;
        for (const auto& map : IncludedMaps_) {
            std::cout << "    - " << map << std::endl;
        }
        if (!IncludedInterface_.empty()) {
            std::cout << "  Included interface: " << IncludedInterface_ << std::endl;
        }
    }

    return true;
}

bool DMCompiler::ParseFiles() {
    std::cout << "Phase 2: Parsing..." << std::endl;
    
    if (PreprocessedTokens_.empty()) {
        ForcedError(Location::Internal, "No tokens to parse");
        return false;
    }
    
    if (Settings_.Verbose) {
        std::cout << "  Parsing " << PreprocessedTokens_.size() << " tokens..." << std::endl;
    }
    
    // Create a token stream lexer that feeds our preprocessed tokens to the parser
    TokenStreamDMLexer lexer(PreprocessedTokens_);
    
    // Create the parser
    DMParser parser(this, &lexer);
    
    // Parse the token stream into an AST
    try {
        ParsedAST_ = parser.ParseFile();
        
        if (!ParsedAST_) {
            ForcedError(Location::Internal, "Parser returned null AST");
            return false;
        }
        
        if (Settings_.Verbose) {
            std::cout << "  Parsed " << ParsedAST_->Statements.size() << " top-level statements" << std::endl;
        }
        
        // Phase 2.5: Constant folding
        if (Settings_.Verbose) {
            std::cout << "  Performing constant folding..." << std::endl;
        }
        
        DMASTFolder folder;
        folder.FoldAst(ParsedAST_.get());
        
        return true;
        
    } catch (const std::exception& e) {
        ForcedError(Location::Internal, std::string("Parser exception: ") + e.what());
        return false;
    }
}

bool DMCompiler::BuildObjectTree() {
    std::cout << "Phase 3: Building object tree..." << std::endl;
    
    if (!ParsedAST_) {
        ForcedError(Location::Internal, "No AST available for object tree building");
        return false;
    }
    
    // Use the DMCodeTreeBuilder to process the AST and build the object tree
    DMCodeTreeBuilder builder(this);
    builder.BuildCodeTree(ParsedAST_.get());
    
    if (Settings_.Verbose) {
        std::cout << "  Object tree built successfully" << std::endl;
        std::cout << "  Types: " << ObjectTree_->AllObjects.size() << std::endl;
        std::cout << "  Procs: " << ObjectTree_->AllProcs.size() << std::endl;
    }
    
    return true;
}

bool DMCompiler::ProcessObjectStatement(DMASTStatement* statement, const DreamPath& currentPath) {
    // Skip proc-level statements (they're inside proc bodies, not object definitions)
    // We only process DMASTObjectStatement types here
    if (!dynamic_cast<DMASTObjectStatement*>(statement)) {
        // This is a proc-level statement (if, for, expression, etc.)
        // It should not be processed at object tree level
        return true;
    }
    
    // Object definition: /mob/player { ... }
    if (auto* objDef = dynamic_cast<DMASTObjectDefinition*>(statement)) {
        // No longer needed - parser now properly handles return type annotations
        // and doesn't create spurious object definitions
        return ProcessObjectDefinition(objDef, currentPath);
    }
    
    // Variable definition: var/name = "value"
    else if (auto* varDef = dynamic_cast<DMASTObjectVarDefinition*>(statement)) {
        if (Settings_.Verbose) {
            std::cout << "  Found variable definition statement for: " << varDef->Name << std::endl;
        }
        return ProcessVarDefinition(varDef, currentPath);
    }
    
    // Variable override: name = "value"
    else if (auto* varOverride = dynamic_cast<DMASTObjectVarOverride*>(statement)) {
        return ProcessVarOverride(varOverride, currentPath);
    }
    
    // Proc definition: proc/test() { ... }
    else if (auto* procDef = dynamic_cast<DMASTObjectProcDefinition*>(statement)) {
        return ProcessProcDefinition(procDef, currentPath);
    }
    
    // Unknown statement type - skip with warning
    else {
        if (Settings_.Verbose) {
            std::cout << "  Skipping unknown statement type at " 
                      << statement->Location_.ToString() << std::endl;
            // Try to identify the type for debugging
            if (dynamic_cast<DMASTObjectDefinition*>(statement)) {
                std::cout << "    (It's an ObjectDefinition that wasn't caught earlier)" << std::endl;
            }
        }
        return true;  // Continue processing
    }
}

bool DMCompiler::ProcessObjectDefinition(DMASTObjectDefinition* objDef, const DreamPath& currentPath) {
    // Combine current path with the object's path
    DreamPath fullPath = currentPath.Combine(objDef->Path.Path);
    
    // Check if this is a "var" block (path ends with "var")
    // In this case, we don't create an object, we just process the inner statements
    // which will be variable definitions
    bool isVarBlock = false;
    auto elements = fullPath.GetElements();
    if (!elements.empty() && elements.back() == "var") {
        isVarBlock = true;
        if (Settings_.Verbose) {
            std::cout << "  Processing var block at: " << fullPath.ToString() << std::endl;
        }
    } else {
        if (Settings_.Verbose) {
            std::cout << "  Defining object: " << fullPath.ToString() << std::endl;
        }
        
        // Create or get the DMObject
        DMObject* obj = ObjectTree_->GetOrCreateDMObject(fullPath);
        if (!obj) {
            ForcedError(objDef->Location_, "Failed to create object: " + fullPath.ToString());
            return false;
        }
    }
    
    // Process all inner statements (vars, procs, nested objects)
    for (const auto& innerStmt : objDef->InnerStatements) {
        if (!ProcessObjectStatement(innerStmt.get(), fullPath)) {
            return false;
        }
    }
    
    return true;
}

bool DMCompiler::ProcessVarDefinition(DMASTObjectVarDefinition* varDef, const DreamPath& currentPath) {
    // The parser now correctly separates the variable name and type
    // TypePath contains just the type (e.g., "Beam" for "Beam/myBeam")
    // currentPath contains the owner object path (e.g., "/mob" or "/mob/var")
    // varDef->Name contains the variable name (e.g., "myBeam")
    
    if (Settings_.Verbose) {
        std::cout << "  Processing var definition: " << varDef->Name 
                  << " with type: " << varDef->TypePath.Path.ToString()
                  << " on object: " << currentPath.ToString() << std::endl;
    }
    
    // Remove "var" from currentPath if present to get the actual object path
    std::vector<std::string> objectPathElements = currentPath.GetElements();
    for (auto it = objectPathElements.begin(); it != objectPathElements.end(); ++it) {
        if (*it == "var") {
            objectPathElements.erase(it);
            if (Settings_.Verbose) {
                std::cout << "    Removed 'var' from object path" << std::endl;
            }
            break;
        }
    }
    
    DreamPath actualObjectPath = DreamPath(currentPath.GetPathType(), objectPathElements);
    
    if (Settings_.Verbose) {
        std::cout << "    Actual object path: " << actualObjectPath.ToString() << std::endl;
    }
    
    // Check if this is a global variable (defined on /global)
    if (actualObjectPath.ToString() == "/global") {
        // This is a global variable - add to Globals vector instead of object variables
        if (Settings_.Verbose) {
            std::cout << "  Defining global var: " << varDef->Name << std::endl;
        }
        
        DMVariable var;
        var.Name = varDef->Name;
        var.IsConst = false;
        var.IsFinal = false;
        var.IsGlobal = true;  // Mark as global
        var.IsTmp = false;
        var.Value = varDef->Value.get();  // Store the default value expression (non-owning pointer)
        
        // Add to the Globals vector
        ObjectTree_->Globals.push_back(var);
        
        return true;
    }
    
    // Get or create the object this var belongs to
    DMObject* obj = ObjectTree_->GetOrCreateDMObject(actualObjectPath);
    if (!obj) {
        ForcedError(varDef->Location_, "Cannot define variable on non-existent object: " + actualObjectPath.ToString());
        return false;
    }
    
    if (Settings_.Verbose) {
        std::string pathStr = actualObjectPath.ToString();
        // Don't add extra slash if path already ends with one (for root path)
        if (pathStr.empty() || pathStr.back() != '/') {
            pathStr += "/";
        }
        std::cout << "  Defining var: " << pathStr << varDef->Name << std::endl;
    }
    
    // Create the variable
    DMVariable var;
    var.Name = varDef->Name;
    var.IsConst = false;
    var.IsFinal = false;
    var.IsGlobal = false;
    var.IsTmp = false;
    var.Value = varDef->Value.get();  // Store the default value expression (non-owning pointer)
    
    // Add to the Variables map
    obj->Variables[varDef->Name] = var;
    
    return true;
}

bool DMCompiler::ProcessVarOverride(DMASTObjectVarOverride* varOverride, const DreamPath& currentPath) {
    // Get or create the object this override belongs to
    DMObject* obj = ObjectTree_->GetOrCreateDMObject(currentPath);
    if (!obj) {
        ForcedError(varOverride->Location_, "Cannot override variable on non-existent object: " + currentPath.ToString());
        return false;
    }
    
    if (Settings_.Verbose) {
        std::string pathStr = currentPath.ToString();
        // Don't add extra slash if path already ends with one (for root path)
        if (pathStr.empty() || pathStr.back() != '/') {
            pathStr += "/";
        }
        std::cout << "  Overriding var: " << pathStr << varOverride->VarName << std::endl;
    }
    
    // TODO: Find the variable and update its default value
    // For now, just log it
    
    return true;
}

bool DMCompiler::ProcessProcDefinition(DMASTObjectProcDefinition* procDef, const DreamPath& currentPath) {
    // Combine current path with the proc's object path to get full path
    DreamPath fullPath = currentPath.Combine(procDef->ObjectPath);
    
    // Get or create the object this proc belongs to
    DMObject* obj = ObjectTree_->GetOrCreateDMObject(fullPath);
    if (!obj) {
        ForcedError(procDef->Location_, "Cannot define proc on non-existent object: " + fullPath.ToString());
        return false;
    }
    
    if (Settings_.Verbose) {
        std::string pathStr = fullPath.ToString();
        // Don't add extra slash if path already ends with one (for root path)
        if (pathStr.empty() || pathStr.back() != '/') {
            pathStr += "/";
        }
        std::cout << "  Defining proc: " << pathStr << procDef->Name << std::endl;
    }
    
    // Create the DMProc object
    DMProc* proc = ObjectTree_->CreateProc(procDef->Name, obj, procDef->IsVerb, procDef->Location_);
    
    // Store the AST body for later bytecode compilation (Phase 4)
    proc->AstBody = procDef->Body.get();
    
    // Process parameters
    int paramIndex = 0;
    for (const auto& param : procDef->Parameters) {
        // Add to parameter list
        proc->Parameters.push_back(param->Name);
        
        // Create local variable for parameter
        std::optional<DreamPath> paramType = std::nullopt;
        if (!param->TypePath.GetElements().empty()) {
            paramType = param->TypePath;
        }
        
        auto localVar = std::make_unique<LocalVariable>(
            param->Name,
            paramIndex,
            true, // IsParameter
            paramType
        );
        
        proc->LocalVariables[param->Name] = std::move(localVar);
        paramIndex++;
    }
    
    // Add the proc to the object
    obj->AddProc(proc->Id, procDef->Name);
    
    // Register as global proc if on root
    if (fullPath == DreamPath::Root) {
        ObjectTree_->RegisterGlobalProc(procDef->Name, proc->Id);
    }
    
    return true;
}

bool DMCompiler::EmitBytecode() {
    std::cout << "Phase 4: Emitting bytecode..." << std::endl;
    
    // Iterate through all procs
    for (const auto& proc : ObjectTree_->AllProcs) {
        if (!proc->AstBody) {
            // No body to compile (e.g., native procs)
            continue;
        }
        
        // Create bytecode writer
        BytecodeWriter writer;
        
        // Create expression compiler
        DMExpressionCompiler exprCompiler(this, proc.get(), &writer);
        
        // Create statement compiler
        DMStatementCompiler stmtCompiler(this, proc.get(), &writer, &exprCompiler);
        
        // Compile the proc body
        if (!stmtCompiler.CompileBlockInner(proc->AstBody)) {
            ForcedWarning("Failed to compile proc: " + proc->Name);
            continue;
        }
        
        // Store bytecode and max stack size
        proc->Bytecode = writer.GetBytecode();
        proc->MaxStackSize = writer.GetMaxStackSize();
        
        if (Settings_.Verbose) {
            std::string pathStr = proc->OwningObject->Path.ToString();
            // Don't add extra slash if path already ends with one (for root path)
            if (pathStr.empty() || pathStr.back() != '/') {
                pathStr += "/";
            }
            std::cout << "  Compiled proc: " << pathStr << proc->Name 
                      << " (" << proc->Bytecode.size() << " bytes)" << std::endl;
        }
    }
    
    return true;
}

bool DMCompiler::OutputJson(const std::string& outputPath) {
    std::cout << "Phase 5: Writing JSON output..." << std::endl;
    
    namespace fs = std::filesystem;
    fs::path jsonPath = fs::path(outputPath).replace_extension(".json");
    
    std::ofstream out(jsonPath);
    if (!out) {
        ForcedError(Location::Internal, "Failed to open output file: " + jsonPath.string());
        return false;
    }
    
    JsonWriter json;
    json.BeginObject();
    
    // Metadata
    json.WriteKey("Metadata");
    json.BeginObject();
    json.WriteKeyValue("Version", "DMCompilerCpp-1.0");
    json.EndObject();
    
    // Strings table
    json.WriteKey("Strings");
    json.BeginArray();
    for (const auto& str : ObjectTree_->StringTable) {
        json.WriteString(str);
    }
    json.EndArray();
    
    // Resources (if any)
    if (!ObjectTree_->Resources.empty()) {
        json.WriteKey("Resources");
        json.BeginArray();
        for (const auto& res : ObjectTree_->Resources) {
            json.WriteString(res);
        }
        json.EndArray();
    }
    
    // Global procs
    if (!ObjectTree_->GlobalProcs.empty()) {
        json.WriteKey("GlobalProcs");
        json.BeginArray();
        for (const auto& [name, id] : ObjectTree_->GlobalProcs) {
            json.WriteInt(id);
        }
        json.EndArray();
    }
    
    // Globals
    if (!ObjectTree_->Globals.empty()) {
        json.WriteKey("Globals");
        json.BeginObject();
        json.WriteKeyValue("GlobalCount", static_cast<int>(ObjectTree_->Globals.size()));
        
        json.WriteKey("Names");
        json.BeginArray();
        for (const auto& global : ObjectTree_->Globals) {
            json.WriteString(global.Name);
        }
        json.EndArray();
        
        json.WriteKey("Globals");
        json.BeginObject();
        for (const auto& global : ObjectTree_->Globals) {
            json.WriteKey(global.Name);
            
            // Try to serialize the default value
            if (global.Value) {
                JsonValue jsonValue;
                if (global.Value->TryAsJsonRepresentation(this, jsonValue)) {
                    json.WriteValue(jsonValue);
                } else {
                    json.WriteNull();
                }
            } else {
                json.WriteNull();
            }
        }
        json.EndObject();
        
        json.EndObject();
    }
    
    // Types
    json.WriteKey("Types");
    json.BeginArray();
    for (const auto& obj : ObjectTree_->AllObjects) {
        json.BeginObject();
        
        json.WriteKeyValue("Path", obj->Path.ToString());
        
        if (obj->Parent) {
            json.WriteKeyValue("Parent", obj->Parent->Id);
        }
        
        if (obj->InitializationProc != -1) {
            json.WriteKeyValue("InitProc", obj->InitializationProc);
        }
        
        // Procs on this type
        if (!obj->Procs.empty()) {
            json.WriteKey("Procs");
            json.BeginArray();
            for (const auto& [name, procIds] : obj->Procs) {
                json.BeginArray();
                for (int procId : procIds) {
                    json.WriteInt(procId);
                }
                json.EndArray();
            }
            json.EndArray();
        }
        
        // Variables
        if (!obj->Variables.empty()) {
            json.WriteKey("Variables");
            json.BeginObject();
            for (const auto& [name, var] : obj->Variables) {
                json.WriteKey(name);
                
                // Try to serialize the default value
                if (var.Value) {
                    JsonValue jsonValue;
                    if (var.Value->TryAsJsonRepresentation(this, jsonValue)) {
                        json.WriteValue(jsonValue);
                    } else {
                        json.WriteNull();
                    }
                } else {
                    json.WriteNull();
                }
            }
            json.EndObject();
        }
        
        // Const variables
        if (!obj->ConstVariables.empty()) {
            json.WriteKey("ConstVariables");
            json.BeginArray();
            for (const auto& name : obj->ConstVariables) {
                json.WriteString(name);
            }
            json.EndArray();
        }
        
        // Tmp variables
        if (!obj->TmpVariables.empty()) {
            json.WriteKey("TmpVariables");
            json.BeginArray();
            for (const auto& name : obj->TmpVariables) {
                json.WriteString(name);
            }
            json.EndArray();
        }
        
        json.EndObject();
    }
    json.EndArray();
    
    // Procs
    json.WriteKey("Procs");
    json.BeginArray();
    for (const auto& proc : ObjectTree_->AllProcs) {
        json.BeginObject();
        
        json.WriteKeyValue("OwningTypeId", proc->OwningObject->Id);
        json.WriteKeyValue("Name", proc->Name);
        json.WriteKeyValue("Attributes", static_cast<int>(proc->Attributes));
        
        // Use actual max stack size calculated during bytecode emission
        json.WriteKeyValue("MaxStackSize", proc->MaxStackSize);
        
        // Arguments
        if (!proc->Parameters.empty()) {
            json.WriteKey("Arguments");
            json.BeginArray();
            for (const auto& paramName : proc->Parameters) {
                json.BeginObject();
                json.WriteKeyValue("Name", paramName);
                
                // Get the type information from LocalVariables
                uint32_t typeFlags = 0; // Default to Anything (0)
                auto it = proc->LocalVariables.find(paramName);
                if (it != proc->LocalVariables.end()) {
                    const LocalVariable* param = it->second.get();
                    
                    // If there's an explicit value type (from "as" keyword), use that
                    if (param->ExplicitValueType.has_value()) {
                        typeFlags = static_cast<uint32_t>(param->ExplicitValueType->Type);
                    }
                    // Otherwise, infer from the type path
                    else if (param->Type.has_value()) {
                        // Get the DMObject for this type path and determine its DMValueType
                        DMObject* typeObj = nullptr;
                        if (ObjectTree_->TryGetDMObject(param->Type.value(), &typeObj)) {
                            // Convert the type path to a DMValueType
                            std::string pathStr = param->Type->ToString();
                            if (pathStr == "/obj" || pathStr.rfind("/obj/", 0) == 0) {
                                typeFlags = static_cast<uint32_t>(DMValueType::Obj);
                            } else if (pathStr == "/mob" || pathStr.rfind("/mob/", 0) == 0) {
                                typeFlags = static_cast<uint32_t>(DMValueType::Mob);
                            } else if (pathStr == "/turf" || pathStr.rfind("/turf/", 0) == 0) {
                                typeFlags = static_cast<uint32_t>(DMValueType::Turf);
                            } else if (pathStr == "/area" || pathStr.rfind("/area/", 0) == 0) {
                                typeFlags = static_cast<uint32_t>(DMValueType::Area);
                            }
                            // Otherwise leave as Anything (0)
                        }
                    }
                    // If no type specified, it's Anything (0)
                }
                
                json.WriteKeyValue("Type", static_cast<int>(typeFlags));
                json.EndObject();
            }
            json.EndArray();
        }
        
        // Source info (placeholder)
        json.WriteKey("SourceInfo");
        json.BeginArray();
        json.BeginObject();
        json.WriteKeyValue("Offset", 0);
        json.WriteKeyValue("Line", proc->SourceLocation.Line);
        json.EndObject();
        json.EndArray();
        
        // Bytecode
        if (!proc->Bytecode.empty()) {
            json.WriteKey("Bytecode");
            json.WriteByteArray(proc->Bytecode);
        }
        
        // Verb properties
        json.WriteKeyValue("IsVerb", proc->IsVerb);
        if (proc->IsVerb) {
            if (proc->VerbSource.has_value()) {
                json.WriteKeyValue("VerbSrc", static_cast<int>(proc->VerbSource.value()));
            }
            if (proc->VerbName.has_value()) {
                json.WriteKeyValue("VerbName", proc->VerbName.value());
            }
            if (proc->VerbCategory.has_value()) {
                json.WriteKeyValue("VerbCategory", proc->VerbCategory.value());
            }
            if (proc->VerbDescription.has_value()) {
                json.WriteKeyValue("VerbDesc", proc->VerbDescription.value());
            }
            json.WriteKeyValue("Invisibility", static_cast<int>(proc->Invisibility));
        }
        
        json.EndObject();
    }
    json.EndArray();
    
    // Optional errors (runtime configuration warnings in range 4000-4999)
    json.WriteKey("OptionalErrors");
    json.BeginObject();
    for (const auto& [code, level] : ErrorConfig_) {
        int codeValue = static_cast<int>(code);
        if (codeValue >= 4000 && codeValue <= 4999) {
            json.WriteKey(std::to_string(codeValue));
            json.WriteInt(static_cast<int>(level));
        }
    }
    json.EndObject();
    
    json.EndObject();
    
    out << json.ToString();
    
    std::cout << "Output written to: " << jsonPath.string() << std::endl;
    if (Settings_.Verbose) {
        std::cout << "  Types: " << ObjectTree_->AllObjects.size() << std::endl;
        std::cout << "  Procs: " << ObjectTree_->AllProcs.size() << std::endl;
        std::cout << "  Strings: " << ObjectTree_->StringTable.size() << std::endl;
    }
    return true;
}

std::vector<std::unique_ptr<DreamMapJson>> DMCompiler::ConvertMaps(const std::vector<std::string>& mapPaths, int& zOffset) {
    std::vector<std::unique_ptr<DreamMapJson>> maps;
    
    for (const auto& mapPath : mapPaths) {
        CheckProgress("Converting Maps");
        if (Settings_.Verbose) {
            std::cout << "  Converting map: " << mapPath << std::endl;
        }
        
        // Read the map file
        std::ifstream file(mapPath);
        if (!file.is_open()) {
            ForcedError(Location::Internal, "Failed to open map file: " + mapPath);
            continue;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        
        // Create a lexer for the map file
        DMLexer lexer(mapPath, content);
        
        // Create the DMM parser
        DMMParser parser(this, &lexer, zOffset);
        
        // Parse the map
        auto map = parser.ParseMap();
        
        if (map) {
            // Update z-offset for next map
            zOffset = std::max(zOffset + 1, map->MaxZ);
            maps.push_back(std::move(map));
        } else {
            ForcedError(Location::Internal, "Failed to parse map: " + mapPath);
        }
    }
    
    return maps;
}

bool DMCompiler::InitializeDMStandard() {
    // Skip if --no-standard flag is set
    if (Settings_.NoStandard) {
        if (Settings_.Verbose) {
            std::cout << "Phase 1.5: Skipping DMStandard initialization (--no-standard flag set)" << std::endl;
        }
        return true;
    }
    
    if (Settings_.Verbose) {
        std::cout << "Phase 1.5: Initializing DMStandard constants..." << std::endl;
    }
    
    namespace fs = std::filesystem;
    
    // Get the directory where the compiler executable is located
    fs::path compilerDir;
#ifdef _WIN32
    // Windows implementation
    wchar_t exePath[MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, exePath, MAX_PATH);
    if (length == 0 || length == MAX_PATH) {
        ForcedWarning("Failed to get executable path for DMStandard initialization");
        return true;  // Non-fatal, continue compilation
    } else {
        // Convert wide string to regular string
        std::wstring ws(exePath);
        std::string str(ws.begin(), ws.end());
        compilerDir = fs::path(str).parent_path();
    }
#elif defined(__linux__) || defined(__APPLE__)
    // Unix-like systems
    char exePath[PATH_MAX];
#ifdef __linux__
    ssize_t length = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
#elif defined(__APPLE__)
    uint32_t bufsize = PATH_MAX;
    int length = _NSGetExecutablePath(exePath, &bufsize);
    if (length != 0) length = -1;
    else length = strlen(exePath);
#endif
    if (length == -1) {
        ForcedWarning("Failed to get executable path for DMStandard initialization");
        return true;  // Non-fatal, continue compilation
    } else {
        exePath[length] = '\0';
        compilerDir = fs::path(exePath).parent_path();
    }
#else
    // Fallback for other platforms
    ForcedWarning("Executable path detection not implemented for this platform");
    return true;  // Non-fatal, continue compilation
#endif
    
    fs::path dmStandardDir = compilerDir / "DMStandard";
    fs::path definesFile = dmStandardDir / "Defines.dm";
    
    if (!fs::exists(definesFile)) {
        ForcedWarning("DMStandard/Defines.dm not found at: " + definesFile.string());
        return true;  // Non-fatal, continue compilation
    }
    
    // Read the Defines.dm file
    std::ifstream file(definesFile);
    if (!file.is_open()) {
        ForcedWarning("Failed to open DMStandard/Defines.dm");
        return true;  // Non-fatal, continue compilation
    }
    
    std::string line;
    int constantsAdded = 0;
    
    // Parse #define directives and extract numeric constants
    while (std::getline(file, line)) {
        // Trim leading whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        
        // Check if line starts with #define
        if (line.substr(start, 7) != "#define") continue;
        
        // Extract the macro name and value
        size_t nameStart = start + 7;
        nameStart = line.find_first_not_of(" \t", nameStart);
        if (nameStart == std::string::npos) continue;
        
        size_t nameEnd = line.find_first_of(" \t", nameStart);
        if (nameEnd == std::string::npos) continue;
        
        std::string name = line.substr(nameStart, nameEnd - nameStart);
        
        // Extract the value
        size_t valueStart = line.find_first_not_of(" \t", nameEnd);
        if (valueStart == std::string::npos) continue;
        
        // Find the end of the value (before any comment)
        size_t valueEnd = line.find("//", valueStart);
        if (valueEnd == std::string::npos) {
            valueEnd = line.length();
        }
        
        // Trim trailing whitespace from value
        while (valueEnd > valueStart && (line[valueEnd - 1] == ' ' || line[valueEnd - 1] == '\t')) {
            valueEnd--;
        }
        
        std::string valueStr = line.substr(valueStart, valueEnd - valueStart);
        
        // Try to parse as integer
        try {
            // Handle simple numeric values
            if (valueStr.find_first_not_of("0123456789-") == std::string::npos) {
                int value = std::stoi(valueStr);
                ObjectTree_->AddGlobalConstant(name, value);
                constantsAdded++;
                
                if (Settings_.Verbose) {
                    std::cout << "  Added constant: " << name << " = " << value << std::endl;
                }
            }
            // Handle hex values (0x...)
            else if (valueStr.substr(0, 2) == "0x" || valueStr.substr(0, 2) == "0X") {
                int value = std::stoi(valueStr, nullptr, 16);
                ObjectTree_->AddGlobalConstant(name, value);
                constantsAdded++;
                
                if (Settings_.Verbose) {
                    std::cout << "  Added constant: " << name << " = " << value << std::endl;
                }
            }
            // Handle expressions like "NORTH | EAST" (5)
            else if (name == "NORTHEAST") {
                ObjectTree_->AddGlobalConstant(name, 5);  // NORTH | EAST
                constantsAdded++;
            }
            else if (name == "SOUTHEAST") {
                ObjectTree_->AddGlobalConstant(name, 6);  // SOUTH | EAST
                constantsAdded++;
            }
            else if (name == "SOUTHWEST") {
                ObjectTree_->AddGlobalConstant(name, 10);  // SOUTH | WEST
                constantsAdded++;
            }
            else if (name == "NORTHWEST") {
                ObjectTree_->AddGlobalConstant(name, 9);  // NORTH | WEST
                constantsAdded++;
            }
            // Handle bit shift expressions like "(1<<0)"
            else if (valueStr.find("<<") != std::string::npos) {
                // Parse simple bit shift: (1<<N)
                size_t shiftPos = valueStr.find("<<");
                size_t numStart = valueStr.find_first_of("0123456789", shiftPos + 2);
                if (numStart != std::string::npos) {
                    size_t numEnd = valueStr.find_first_not_of("0123456789", numStart);
                    if (numEnd == std::string::npos) numEnd = valueStr.length();
                    
                    int shiftAmount = std::stoi(valueStr.substr(numStart, numEnd - numStart));
                    int value = 1 << shiftAmount;
                    ObjectTree_->AddGlobalConstant(name, value);
                    constantsAdded++;
                    
                    if (Settings_.Verbose) {
                        std::cout << "  Added constant: " << name << " = " << value << std::endl;
                    }
                }
            }
            // Skip string constants and complex expressions
        } catch (const std::exception&) {
            // Skip constants that can't be parsed as integers
            continue;
        }
    }
    
    // Add lowercase boolean aliases (not in Defines.dm but commonly used in DM code)
    // Note: These are separate from TRUE/FALSE and are case-sensitive in DM
    ObjectTree_->AddGlobalConstant("true", 1);
    ObjectTree_->AddGlobalConstant("false", 0);
    constantsAdded += 2;
    
    if (Settings_.Verbose) {
        std::cout << "  Added lowercase boolean aliases: true, false" << std::endl;
    }
    
    if (Settings_.Verbose) {
        std::cout << "  Added " << constantsAdded << " DMStandard constants" << std::endl;
    }
    
    return true;
}

void DMCompiler::CheckProgress(const std::string& phase) {
    auto now = std::chrono::steady_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::seconds>(now - StartTime_).count();
    
    // Only report progress if compilation has been running for more than 60 seconds
    if (totalDuration > 60) {
        auto sinceLastReport = std::chrono::duration_cast<std::chrono::seconds>(now - LastProgressTime_).count();
        
        // Report every 10 seconds
        if (sinceLastReport >= 10) {
            std::cout << "[" << totalDuration << "s] " << phase << " in progress..." << std::endl;
            LastProgressTime_ = now;
        }
    }
}

} // namespace DMCompiler
