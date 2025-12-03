#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <memory>
#include <filesystem>
#include "Token.h"
#include "Location.h"

namespace DMCompiler {

class DMCompiler;
class DMLexer;

/// <summary>
/// Represents a preprocessor macro
/// </summary>
class DMMacro {
public:
    virtual ~DMMacro() = default;
    // Each argument is now a vector of tokens to support multi-token arguments like "++ x"
    virtual std::vector<Token> Expand(const std::vector<std::vector<Token>>& arguments, const Location& location) = 0;
    virtual bool HasParameters() const { return false; }
};

/// <summary>
/// Simple text replacement macro
/// </summary>
class DMMacroText : public DMMacro {
public:
    std::vector<Token> Tokens;
    
    explicit DMMacroText(const std::vector<Token>& tokens) : Tokens(tokens) {}
    
    std::vector<Token> Expand(const std::vector<std::vector<Token>>& arguments, const Location& location) override {
        return Tokens;
    }
};

/// <summary>
/// Function-like macro with parameters
/// </summary>
class DMMacroFunction : public DMMacro {
public:
    std::vector<std::string> Parameters;
    std::vector<Token> Tokens;
    
    DMMacroFunction(const std::vector<std::string>& params, const std::vector<Token>& tokens)
        : Parameters(params), Tokens(tokens) {}
    
    bool HasParameters() const override { return true; }
    
    std::vector<Token> Expand(const std::vector<std::vector<Token>>& arguments, const Location& location) override;
};

/// <summary>
/// Built-in __LINE__ macro
/// </summary>
class DMMacroLine : public DMMacro {
public:
    std::vector<Token> Expand(const std::vector<std::vector<Token>>& arguments, const Location& location) override;
};

/// <summary>
/// Built-in __FILE__ macro
/// </summary>
class DMMacroFile : public DMMacro {
public:
    std::vector<Token> Expand(const std::vector<std::vector<Token>>& arguments, const Location& location) override;
};

/// <summary>
/// Built-in DM_VERSION macro
/// </summary>
class DMMacroVersion : public DMMacro {
public:
    std::vector<Token> Expand(const std::vector<std::vector<Token>>& arguments, const Location& location) override;
};

/// <summary>
/// Built-in DM_BUILD macro
/// </summary>
class DMMacroBuild : public DMMacro {
public:
    std::vector<Token> Expand(const std::vector<std::vector<Token>>& arguments, const Location& location) override;
};

/// <summary>
/// File context for tracking file state during preprocessing
/// </summary>
struct FileContext {
    std::unique_ptr<DMLexer> Lexer;
    std::string FilePath;
    std::string Directory;
    int IncludeDepth;
    
    FileContext(std::unique_ptr<DMLexer> lexer, const std::string& path, int depth)
        : Lexer(std::move(lexer))
        , FilePath(path)
        , Directory(std::filesystem::path(path).parent_path().string())
        , IncludeDepth(depth)
    {}
    
    // Move constructor
    FileContext(FileContext&& other) noexcept
        : Lexer(std::move(other.Lexer))
        , FilePath(std::move(other.FilePath))
        , Directory(std::move(other.Directory))
        , IncludeDepth(other.IncludeDepth)
    {}
    
    // Move assignment
    FileContext& operator=(FileContext&& other) noexcept {
        if (this != &other) {
            Lexer = std::move(other.Lexer);
            FilePath = std::move(other.FilePath);
            Directory = std::move(other.Directory);
            IncludeDepth = other.IncludeDepth;
        }
        return *this;
    }
    
    // Delete copy constructor and copy assignment
    FileContext(const FileContext&) = delete;
    FileContext& operator=(const FileContext&) = delete;
};

/// <summary>
/// Include chain entry for error reporting
/// </summary>
struct IncludeChainEntry {
    std::string FilePath;
    Location IncludeLocation;
    
    IncludeChainEntry(const std::string& path, const Location& loc)
        : FilePath(path), IncludeLocation(loc) {}
};

/// <summary>
/// DM Preprocessor - handles #include, #define, #if, etc.
/// </summary>
class DMPreprocessor {
public:
    explicit DMPreprocessor(DMCompiler* compiler = nullptr);
    ~DMPreprocessor();

    // Streaming interface (NEW)
    bool Initialize(const std::string& rootFilePath);
    Token GetNextToken();
    bool IsComplete() const;
    
    // Main preprocessing entry point (kept for backward compatibility)
    std::vector<Token> Preprocess(const std::string& filePath);
    
    // Include tracking
    std::vector<std::string> GetIncludedMaps() const { return IncludedMaps_; }
    std::string GetIncludedInterface() const { return IncludedInterface_; }
    
    // Define management
    void Define(const std::string& name, const std::string& value);
    void Undefine(const std::string& name);
    bool IsDefined(const std::string& name) const;
    
    // Macro expansion for expression evaluation
    std::vector<Token> ExpandMacroForExpression(const std::string& name) const;

private:
    DMCompiler* Compiler_;
    
    std::vector<std::string> IncludedMaps_;
    std::string IncludedInterface_;
    
    // File stack for nested includes (NEW - replaces LexerStack_)
    std::stack<FileContext> FileStack_;
    
    // Include chain tracking for error reporting
    std::vector<IncludeChainEntry> IncludeChain_;
    
    // Token buffering
    std::stack<Token> UnprocessedTokens_;
    std::stack<Token> BufferedWhitespace_;
    
    // Macro definitions
    std::unordered_map<std::string, std::unique_ptr<DMMacro>> Defines_;
    
    // Include tracking
    std::unordered_set<std::string> IncludedFiles_;
    
    // Path resolution cache for performance
    std::unordered_map<std::string, std::string> PathCache_;
    
    // Conditional compilation state
    std::stack<bool> LastIfEvaluations_;
    bool CanUseDirective_;
    bool CurrentLineContainsNonWhitespace_;
    TokenType PreviousNonWhitespaceToken_; // Track previous token for path context detection
    
    // Token processing (GetNextToken is now public for streaming interface)
    void PushToken(Token&& token);
    void PushTokens(std::vector<Token>&& tokens);
    Token GetNextRawToken(); // NEW: Get token from lexer without preprocessing
    
    // Directive handlers (batch mode - for backward compatibility)
    void HandleIncludeDirective(const Token& token);
    void HandleDefineDirective(const Token& token);
    void HandleUndefineDirective(const Token& token);
    void HandleIfDirective(const Token& token);
    void HandleIfDefDirective(const Token& token);
    void HandleIfNDefDirective(const Token& token);
    void HandleElifDirective(const Token& token);
    void HandleElseDirective(const Token& token);
    void HandleEndIfDirective(const Token& token);
    void HandleErrorDirective(const Token& token);
    void HandleWarningDirective(const Token& token);
    void HandlePragmaDirective(const Token& token);
    
    // Streaming directive handlers (for GetNextToken)
    void HandleIncludeDirectiveStreaming(const Token& token);
    void HandleIfDirectiveStreaming(const Token& token);
    void HandleIfDefDirectiveStreaming(const Token& token);
    void HandleIfNDefDirectiveStreaming(const Token& token);
    void HandleElifDirectiveStreaming(const Token& token);
    void HandleElseDirectiveStreaming(const Token& token);
    void HandleEndIfDirectiveStreaming(const Token& token);
    
    // Macro handling
    bool TryExpandMacro(const Token& token);
    std::vector<std::vector<Token>> ReadMacroArguments();
    
    // Conditional evaluation
    bool EvaluateCondition(const std::vector<Token>& tokens);
    void SkipIfBody(bool skipElse);
    
    // File inclusion
    bool IncludeFile(const std::string& path, const Location& includeLocation);
    std::vector<Token> PreprocessFile(const std::string& path, const Location& includeLocation);
    std::string ResolvePath(const std::string& path, const std::string& currentFile);
    
    // File stack management (NEW)
    bool PushFile(const std::string& filePath, const Location& includeLocation);
    void PopFile();
    const FileContext* GetCurrentContext() const;
    
    // Error reporting helpers
    std::string GetIncludeChainString() const;
    void ReportError(const Location& loc, const std::string& message);
    
    // Helpers
    Token ConsumeToken();
    std::vector<Token> ReadLineTokens();
    bool IsDirective(TokenType type) const;
};

} // namespace DMCompiler
