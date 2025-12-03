#include "DMPreprocessor.h"
#include "DMCompiler.h"
#include "DMLexer.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <unordered_set>

namespace DMCompiler {

// ============================================================================
// DMMacro Implementations
// ============================================================================

std::vector<Token> DMMacroFunction::Expand(const std::vector<std::vector<Token>>& arguments, const Location& location) {
    if (arguments.size() != Parameters.size()) {
        std::cerr << "Error at " << location.ToString() << ": Macro expansion failed - expected " 
                  << Parameters.size() << " arguments but got " << arguments.size() << std::endl;
        return {}; // Error: wrong number of arguments
    }
    
    std::vector<Token> result;
    std::unordered_map<std::string, std::vector<Token>> argMap;
    
    // Map parameter names to argument token vectors
    for (size_t i = 0; i < Parameters.size(); ++i) {
        argMap[Parameters[i]] = arguments[i];
    }
    
    // Replace parameters in token stream
    for (const auto& token : Tokens) {
        if (token.Type == TokenType::Identifier) {
            auto it = argMap.find(token.Text);
            if (it != argMap.end()) {
                // Replace with argument tokens (preserving all tokens in multi-token args)
                for (const auto& argToken : it->second) {
                    result.push_back(argToken);
                }
                continue;
            }
        }
        result.push_back(token);
    }
    
    return result;
}

std::vector<Token> DMMacroLine::Expand(const std::vector<std::vector<Token>>& arguments, const Location& location) {
    std::vector<Token> result;
    Token::TokenValue value(static_cast<int64_t>(location.Line));
    result.push_back(Token(TokenType::Number, std::to_string(location.Line), location, value));
    return result;
}

std::vector<Token> DMMacroFile::Expand(const std::vector<std::vector<Token>>& arguments, const Location& location) {
    std::vector<Token> result;
    Token::TokenValue value(location.SourceFile);
    result.push_back(Token(TokenType::String, location.SourceFile, location, value));
    return result;
}

std::vector<Token> DMMacroVersion::Expand(const std::vector<std::vector<Token>>& arguments, const Location& location) {
    std::vector<Token> result;
    Token::TokenValue value(static_cast<int64_t>(515)); // Default DM version
    result.push_back(Token(TokenType::Number, "515", location, value));
    return result;
}

std::vector<Token> DMMacroBuild::Expand(const std::vector<std::vector<Token>>& arguments, const Location& location) {
    std::vector<Token> result;
    Token::TokenValue value(static_cast<int64_t>(1630)); // Default build number
    result.push_back(Token(TokenType::Number, "1630", location, value));
    return result;
}

// ============================================================================
// DMPreprocessor Implementation
// ============================================================================

DMPreprocessor::DMPreprocessor(DMCompiler* compiler)
    : Compiler_(compiler)
    , CanUseDirective_(true)
    , CurrentLineContainsNonWhitespace_(false)
    , PreviousNonWhitespaceToken_(TokenType::EndOfFile)
{
    // Add built-in macros
    Defines_["__LINE__"] = std::make_unique<DMMacroLine>();
    Defines_["__FILE__"] = std::make_unique<DMMacroFile>();
    Defines_["DM_VERSION"] = std::make_unique<DMMacroVersion>();
    Defines_["DM_BUILD"] = std::make_unique<DMMacroBuild>();
    
    // Reserve capacity for containers to avoid reallocations
    IncludedFiles_.reserve(200);  // Expect ~140 files for large projects
    PathCache_.reserve(200);      // Cache resolved paths for reuse
}

DMPreprocessor::~DMPreprocessor() = default;

bool DMPreprocessor::Initialize(const std::string& rootFilePath) {
    // Clear all state
    while (!FileStack_.empty()) {
        FileStack_.pop();
    }
    while (!UnprocessedTokens_.empty()) {
        UnprocessedTokens_.pop();
    }
    while (!BufferedWhitespace_.empty()) {
        BufferedWhitespace_.pop();
    }
    while (!LastIfEvaluations_.empty()) {
        LastIfEvaluations_.pop();
    }
    
    IncludedFiles_.clear();
    IncludedMaps_.clear();
    IncludedInterface_.clear();
    IncludeChain_.clear();
    PathCache_.clear();
    
    // Keep built-in macros, clear user-defined ones
    std::unordered_map<std::string, std::unique_ptr<DMMacro>> builtins;
    if (Defines_.count("__LINE__")) builtins["__LINE__"] = std::move(Defines_["__LINE__"]);
    if (Defines_.count("__FILE__")) builtins["__FILE__"] = std::move(Defines_["__FILE__"]);
    if (Defines_.count("DM_VERSION")) builtins["DM_VERSION"] = std::move(Defines_["DM_VERSION"]);
    if (Defines_.count("DM_BUILD")) builtins["DM_BUILD"] = std::move(Defines_["DM_BUILD"]);
    Defines_ = std::move(builtins);
    
    CanUseDirective_ = true;
    CurrentLineContainsNonWhitespace_ = false;
    PreviousNonWhitespaceToken_ = TokenType::EndOfFile;
    
    // Push root file onto stack
    return PushFile(rootFilePath, Location::Internal);
}

bool DMPreprocessor::IsComplete() const {
    return FileStack_.empty();
}

bool DMPreprocessor::PushFile(const std::string& filePath, const Location& includeLocation) {
    namespace fs = std::filesystem;
    
    // Resolve to absolute path
    std::string absolutePath;
    try {
        absolutePath = fs::absolute(filePath).string();
    } catch (...) {
        ReportError(includeLocation, "Invalid file path: " + filePath);
        return false;
    }
    
    // Check for circular include - if file is already in the include chain
    for (const auto& entry : IncludeChain_) {
        if (entry.FilePath == absolutePath) {
            // Circular include detected
            std::string errorMsg = "Circular include detected: " + filePath;
            ReportError(includeLocation, errorMsg);
            return false;
        }
    }
    
    // Check if file exists
    if (!fs::exists(filePath)) {
        ReportError(includeLocation, "File not found: " + filePath);
        return false;
    }
    
    // Read file content
    std::ifstream file(filePath);
    if (!file.is_open()) {
        ReportError(includeLocation, "Failed to open file: " + filePath);
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // Handle line continuation: backslash at end of line joins with next line
    // This needs to be done before lexing
    // We need to handle: \<newline>, \<CRLF>, and \ followed by optional whitespace then newline
    // Replace the entire continuation sequence with a single space to join lines
    std::string processedContent;
    processedContent.reserve(content.size());
    for (size_t i = 0; i < content.size(); ++i) {
        if (content[i] == '\\') {
            // Check if this backslash is followed by optional whitespace then newline
            size_t j = i + 1;
            // Skip any spaces or tabs (but not other whitespace)
            while (j < content.size() && (content[j] == ' ' || content[j] == '\t')) {
                ++j;
            }
            // Now check for newline
            if (j < content.size()) {
                if (content[j] == '\n') {
                    // Line continuation: replace everything with a single space
                    processedContent.push_back(' ');
                    i = j;  // Skip past newline (loop will increment)
                    continue;
                } else if (content[j] == '\r') {
                    // Check for CRLF
                    if (j + 1 < content.size() && content[j + 1] == '\n') {
                        // Replace with single space
                        processedContent.push_back(' ');
                        i = j + 1;  // Skip past CRLF
                        continue;
                    }
                }
            }
        }
        processedContent.push_back(content[i]);
    }
    content = std::move(processedContent);
    
    // Calculate include depth
    int depth = static_cast<int>(FileStack_.size());
    
    // Create lexer for this file (with whitespace emission enabled for preprocessing)
    auto lexer = std::make_unique<DMLexer>(absolutePath, content, true);
    
    // Create FileContext and push onto stack
    FileStack_.push(FileContext(std::move(lexer), absolutePath, depth));
    
    // Add to include chain for error reporting
    IncludeChain_.push_back(IncludeChainEntry(absolutePath, includeLocation));
    
    // Verbose logging
    if (Compiler_ && Compiler_->GetSettings().Verbose) {
        std::cout << "  Pushed file onto stack: " << absolutePath 
                  << " (depth: " << depth << ")" << std::endl;
    }
    
    return true;
}

void DMPreprocessor::PopFile() {
    if (!FileStack_.empty()) {
        if (Compiler_ && Compiler_->GetSettings().Verbose) {
            std::cout << "  Popped file from stack: " << FileStack_.top().FilePath << std::endl;
        }
        
        // Remove the corresponding entry from include chain
        if (!IncludeChain_.empty()) {
            IncludeChain_.pop_back();
        }
        
        FileStack_.pop();
    }
}

const FileContext* DMPreprocessor::GetCurrentContext() const {
    if (FileStack_.empty()) {
        return nullptr;
    }
    return &FileStack_.top();
}

std::string DMPreprocessor::GetIncludeChainString() const {
    if (IncludeChain_.empty()) {
        return "";
    }
    
    std::string result;
    for (const auto& entry : IncludeChain_) {
        result += "  Included from: " + entry.IncludeLocation.ToString() + "\n";
    }
    return result;
}

void DMPreprocessor::ReportError(const Location& loc, const std::string& message) {
    if (Compiler_) {
        std::string fullMessage = message;
        std::string chain = GetIncludeChainString();
        if (!chain.empty()) {
            fullMessage += "\n" + chain;
        }
        Compiler_->ForcedError(loc, fullMessage);
    }
}

std::vector<Token> DMPreprocessor::Preprocess(const std::string& filePath) {
    std::vector<Token> result;
    
    // Initialize streaming preprocessor
    if (!Initialize(filePath)) {
        return result;  // Error during initialization
    }
    
    // Loop: call GetNextToken() until IsComplete() returns true
    while (!IsComplete()) {
        Token token = GetNextToken();
        
        // Stop at EOF
        if (token.Type == TokenType::EndOfFile) {
            break;
        }
        
        // Collect tokens in result vector using move semantics
        result.push_back(std::move(token));
    }
    
    return result;
}

void DMPreprocessor::Define(const std::string& name, const std::string& value) {
    std::vector<Token> tokens;
    if (!value.empty()) {
        // Try to parse as integer
        try {
            size_t pos;
            int64_t intVal = std::stoll(value, &pos);
            if (pos == value.length()) {
                // Successfully parsed entire string as integer
                Token::TokenValue val(intVal);
                tokens.push_back(Token(TokenType::DM_Preproc_Number, value, Location::Internal, val));
                Defines_[name] = std::make_unique<DMMacroText>(tokens);
                return;
            }
        } catch (...) {
            // Not an integer, try float
        }
        
        // Try to parse as float
        try {
            size_t pos;
            double floatVal = std::stod(value, &pos);
            if (pos == value.length()) {
                // Successfully parsed entire string as float
                Token::TokenValue val(floatVal);
                tokens.push_back(Token(TokenType::DM_Preproc_Number, value, Location::Internal, val));
                Defines_[name] = std::make_unique<DMMacroText>(tokens);
                return;
            }
        } catch (...) {
            // Not a float either
        }
        
        // Default to string/identifier
        Token::TokenValue val(value);
        tokens.push_back(Token(TokenType::DM_Preproc_Identifier, value, Location::Internal, val));
    }
    Defines_[name] = std::make_unique<DMMacroText>(tokens);
}

void DMPreprocessor::Undefine(const std::string& name) {
    Defines_.erase(name);
}

bool DMPreprocessor::IsDefined(const std::string& name) const {
    return Defines_.find(name) != Defines_.end();
}

std::vector<Token> DMPreprocessor::ExpandMacroForExpression(const std::string& name) const {
    auto it = Defines_.find(name);
    if (it == Defines_.end()) {
        return {};
    }
    
    // Only support simple (non-parameterized) macros in expressions
    if (it->second->HasParameters()) {
        return {};
    }
    
    return it->second->Expand({}, Location::Internal);
}

Token DMPreprocessor::GetNextRawToken() {
    // Check UnprocessedTokens_ stack first
    if (!UnprocessedTokens_.empty()) {
        Token token = UnprocessedTokens_.top();
        UnprocessedTokens_.pop();
        return token;
    }
    
    // Check if FileStack_ is empty, return EOF if so
    if (FileStack_.empty()) {
        return Token(TokenType::EndOfFile, "", Location::Internal);
    }
    
    // Get token from top lexer
    return FileStack_.top().Lexer->GetNextToken();
}

Token DMPreprocessor::GetNextToken() {
    // Main streaming token processing loop
    while (true) {
        // Step 1: Check UnprocessedTokens_ stack first, return if not empty
        if (!UnprocessedTokens_.empty()) {
            Token token = UnprocessedTokens_.top();
            UnprocessedTokens_.pop();
            // Update previous token tracking for non-whitespace tokens
            if (token.Type != TokenType::DM_Preproc_Whitespace && 
                token.Type != TokenType::Newline) {
                PreviousNonWhitespaceToken_ = token.Type;
            }
            return token;
        }
        
        // Step 2: Check if FileStack_ is empty, return EOF if so
        if (FileStack_.empty()) {
            return Token(TokenType::EndOfFile, "", Location::Internal);
        }
        
        // Step 3: Get token from top lexer using GetNextRawToken()
        Token token = FileStack_.top().Lexer->GetNextToken();
        
        // Step 4: Process token based on type
        
        // Handle EOF tokens by calling PopFile() and continuing
        if (token.Type == TokenType::EndOfFile) {
            PopFile();
            // Continue to get token from next file (or return EOF if no more files)
            continue;
        }
        
        // Handle newline tokens with line state management
        if (token.Type == TokenType::Newline) {
            CanUseDirective_ = true;
            PreviousNonWhitespaceToken_ = TokenType::Newline; // Reset on newline
            if (CurrentLineContainsNonWhitespace_) {
                BufferedWhitespace_ = std::stack<Token>(); // Clear buffered whitespace
                CurrentLineContainsNonWhitespace_ = false;
                return token;
            } else {
                BufferedWhitespace_ = std::stack<Token>(); // Clear without output
                continue; // Skip empty lines
            }
        }
        
        // Handle whitespace tokens with buffering logic
        if (token.Type == TokenType::DM_Preproc_Whitespace) {
            if (CurrentLineContainsNonWhitespace_) {
                return token; // Emit immediately if we've seen non-whitespace
            } else {
                BufferedWhitespace_.push(token); // Buffer at start of line
                continue;
            }
        }
        
        // Handle preprocessor directives
        if (token.Type == TokenType::DM_Preproc_Include) {
            HandleIncludeDirectiveStreaming(token);
            continue; // Don't emit directive tokens
        }
        
        if (token.Type == TokenType::DM_Preproc_Define) {
            HandleDefineDirective(token);
            continue;
        }
        
        if (token.Type == TokenType::DM_Preproc_Undefine) {
            HandleUndefineDirective(token);
            continue;
        }
        
        if (token.Type == TokenType::DM_Preproc_If) {
            HandleIfDirectiveStreaming(token);
            continue;
        }
        
        if (token.Type == TokenType::DM_Preproc_Ifdef) {
            HandleIfDefDirectiveStreaming(token);
            continue;
        }
        
        if (token.Type == TokenType::DM_Preproc_Ifndef) {
            HandleIfNDefDirectiveStreaming(token);
            continue;
        }
        
        if (token.Type == TokenType::DM_Preproc_Elif) {
            HandleElifDirectiveStreaming(token);
            continue;
        }
        
        if (token.Type == TokenType::DM_Preproc_Else) {
            HandleElseDirectiveStreaming(token);
            continue;
        }
        
        if (token.Type == TokenType::DM_Preproc_EndIf) {
            HandleEndIfDirectiveStreaming(token);
            continue;
        }
        
        if (token.Type == TokenType::DM_Preproc_Error) {
            HandleErrorDirective(token);
            continue;
        }
        
        if (token.Type == TokenType::DM_Preproc_Warning) {
            HandleWarningDirective(token);
            continue;
        }
        
        if (token.Type == TokenType::DM_Preproc_Pragma) {
            HandlePragmaDirective(token);
            continue;
        }
        
        // Handle identifier tokens with macro expansion attempt
        if (token.Type == TokenType::DM_Preproc_Identifier || token.Type == TokenType::Identifier) {
            // Try macro expansion
            if (TryExpandMacro(token)) {
                continue; // Macro was expanded, continue to get next token
            }
            
            // Not a macro - flush buffered whitespace and emit token
            while (!BufferedWhitespace_.empty()) {
                Token ws = BufferedWhitespace_.top();
                BufferedWhitespace_.pop();
                PushToken(std::move(token)); // Push current token back
                return ws; // Return whitespace first
            }
            
            CurrentLineContainsNonWhitespace_ = true;
            PreviousNonWhitespaceToken_ = token.Type;
            return token;
        }
        
        // Handle all other tokens by emitting directly
        // Flush buffered whitespace first
        while (!BufferedWhitespace_.empty()) {
            Token ws = BufferedWhitespace_.top();
            BufferedWhitespace_.pop();
            PushToken(std::move(token)); // Push current token back
            return ws; // Return whitespace first
        }
        
        CurrentLineContainsNonWhitespace_ = true;
        PreviousNonWhitespaceToken_ = token.Type;
        return token;
    }
}

void DMPreprocessor::PushToken(Token&& token) {
    UnprocessedTokens_.push(std::move(token));
}

void DMPreprocessor::PushTokens(std::vector<Token>&& tokens) {
    // Push in reverse order so they come out in correct order
    for (auto it = tokens.rbegin(); it != tokens.rend(); ++it) {
        UnprocessedTokens_.push(std::move(*it));
    }
}

bool DMPreprocessor::TryExpandMacro(const Token& token) {
    // Don't expand macros when in a path context (previous token was / or .)
    // This prevents expansion in proc/MACRO(params), /datum/MACRO, etc.
    if (PreviousNonWhitespaceToken_ == TokenType::Divide ||
        PreviousNonWhitespaceToken_ == TokenType::Period ||
        PreviousNonWhitespaceToken_ == TokenType::DM_Preproc_Punctuator_Period) {
        return false;
    }
    
    auto it = Defines_.find(token.Text);
    if (it == Defines_.end()) {
        return false;
    }
    
    std::vector<Token> expandedTokens;
    if (it->second->HasParameters()) {
        // Function-like macro - read arguments (now returns vector of token vectors)
        std::vector<std::vector<Token>> arguments = ReadMacroArguments();
        expandedTokens = it->second->Expand(arguments, token.Loc);
    } else {
        // Simple macro
        expandedTokens = it->second->Expand({}, token.Loc);
    }
    
    PushTokens(std::move(expandedTokens));
    return true;
}

std::vector<std::vector<Token>> DMPreprocessor::ReadMacroArguments() {
    std::vector<std::vector<Token>> arguments;
    
    // Expect opening parenthesis
    Token token = GetNextToken();
    while (token.Type == TokenType::DM_Preproc_Whitespace) {
        token = GetNextToken();
    }
    
    // Accept both preprocessor and regular token types for '('
    if (token.Type != TokenType::DM_Preproc_Punctuator_LeftParenthesis &&
        token.Type != TokenType::LeftParenthesis) {
        PushToken(std::move(token));
        return arguments; // No arguments
    }
    
    // Read arguments separated by commas, handling nested parentheses
    std::vector<Token> currentArg;
    int parenDepth = 0;
    
    while (true) {
        token = GetNextToken();
        
        if (token.Type == TokenType::EndOfFile || token.Type == TokenType::Newline) {
            if (Compiler_) {
                Compiler_->ForcedError(token.Loc, "Unexpected end of macro arguments");
            }
            break;
        }
        
        // Track parenthesis depth for nested calls - accept both token types
        if (token.Type == TokenType::DM_Preproc_Punctuator_LeftParenthesis ||
            token.Type == TokenType::LeftParenthesis) {
            parenDepth++;
            currentArg.push_back(token);
        } else if (token.Type == TokenType::DM_Preproc_Punctuator_RightParenthesis ||
                   token.Type == TokenType::RightParenthesis) {
            if (parenDepth == 0) {
                // End of arguments
                if (!currentArg.empty() || !arguments.empty()) {
                    // Trim leading/trailing whitespace from current arg
                    while (!currentArg.empty() && currentArg.front().Type == TokenType::DM_Preproc_Whitespace) {
                        currentArg.erase(currentArg.begin());
                    }
                    while (!currentArg.empty() && currentArg.back().Type == TokenType::DM_Preproc_Whitespace) {
                        currentArg.pop_back();
                    }
                    // Keep all tokens for the argument (don't concatenate!)
                    if (!currentArg.empty()) {
                        arguments.push_back(std::move(currentArg));
                    } else if (!arguments.empty()) {
                        // Empty argument after comma - push empty vector
                        arguments.push_back({});
                    }
                }
                break;
            }
            parenDepth--;
            currentArg.push_back(token);
        } else if ((token.Type == TokenType::DM_Preproc_Punctuator_Comma || token.Type == TokenType::Comma) && parenDepth == 0) {
            // Argument separator
            // Trim leading/trailing whitespace from current arg
            while (!currentArg.empty() && currentArg.front().Type == TokenType::DM_Preproc_Whitespace) {
                currentArg.erase(currentArg.begin());
            }
            while (!currentArg.empty() && currentArg.back().Type == TokenType::DM_Preproc_Whitespace) {
                currentArg.pop_back();
            }
            // Keep all tokens for the argument (don't concatenate!)
            arguments.push_back(std::move(currentArg));
            currentArg.clear();
        } else {
            currentArg.push_back(token);
        }
    }
    
    return arguments;
}

bool DMPreprocessor::IncludeFile(const std::string& path, const Location& includeLocation) {
    namespace fs = std::filesystem;
    
    // Check if already included
    std::string absolutePath = fs::absolute(path).string();
    if (IncludedFiles_.find(absolutePath) != IncludedFiles_.end()) {
        return true; // Already included, skip
    }
    
    // Check if file exists
    if (!fs::exists(path)) {
        if (Compiler_) {
            Compiler_->ForcedError(includeLocation, "File not found: " + path);
        }
        return false;
    }
    
    // Read file content
    std::ifstream file(path);
    if (!file.is_open()) {
        if (Compiler_) {
            Compiler_->ForcedError(includeLocation, "Failed to open file: " + path);
        }
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // Mark as included
    IncludedFiles_.insert(absolutePath);
    
    // Track .dmm files
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".dmm") {
        IncludedMaps_.push_back(absolutePath);
    }
    
    // Track .dmf files  
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".dmf") {
        IncludedInterface_ = absolutePath;
    }
    
    // Create lexer for this file (with whitespace emission enabled for preprocessing)
    auto lexer = std::make_unique<DMLexer>(path, content, true);
    int depth = static_cast<int>(FileStack_.size());
    FileStack_.push(FileContext(std::move(lexer), absolutePath, depth));
    
    return true;
}

std::vector<Token> DMPreprocessor::PreprocessFile(const std::string& path, const Location& includeLocation) {
    namespace fs = std::filesystem;
    
    std::vector<Token> result;
    
    // Resolve to absolute path
    std::string absolutePath;
    try {
        absolutePath = fs::absolute(path).string();
    } catch (...) {
        if (Compiler_) {
            Compiler_->ForcedError(includeLocation, "Invalid file path: " + path);
        }
        return result;
    }
    
    // Check if already included (prevent circular includes)
    if (IncludedFiles_.find(absolutePath) != IncludedFiles_.end()) {
        // Already included, skip to prevent circular includes
        if (Compiler_ && Compiler_->GetSettings().Verbose) {
            std::cout << "  Skipping already included file: " << absolutePath << std::endl;
        }
        return result;
    }
    
    // Check if file exists
    if (!fs::exists(path)) {
        if (Compiler_) {
            Compiler_->ForcedError(includeLocation, "File not found: " + path);
        }
        return result;
    }
    
    // Read file content
    std::ifstream file(path);
    if (!file.is_open()) {
        if (Compiler_) {
            Compiler_->ForcedError(includeLocation, "Failed to open file: " + path);
        }
        return result;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // Handle line continuation: backslash at end of line joins with next line
    // This needs to be done before lexing
    // We need to handle: \<newline>, \<CRLF>, and \ followed by optional whitespace then newline
    // Replace the entire continuation sequence with a single space to join lines
    std::string processedContent;
    processedContent.reserve(content.size());
    for (size_t i = 0; i < content.size(); ++i) {
        if (content[i] == '\\') {
            // Check if this backslash is followed by optional whitespace then newline
            size_t j = i + 1;
            // Skip any spaces or tabs (but not other whitespace)
            while (j < content.size() && (content[j] == ' ' || content[j] == '\t')) {
                ++j;
            }
            // Now check for newline
            if (j < content.size()) {
                if (content[j] == '\n') {
                    // Line continuation: replace everything with a single space
                    processedContent.push_back(' ');
                    i = j;  // Skip past newline (loop will increment)
                    continue;
                } else if (content[j] == '\r') {
                    // Check for CRLF
                    if (j + 1 < content.size() && content[j + 1] == '\n') {
                        // Replace with single space
                        processedContent.push_back(' ');
                        i = j + 1;  // Skip past CRLF
                        continue;
                    }
                }
            }
        }
        processedContent.push_back(content[i]);
    }
    content = std::move(processedContent);
    
    // Mark as included BEFORE processing to prevent circular includes
    IncludedFiles_.insert(absolutePath);
    
    // Track .dmm files
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".dmm") {
        IncludedMaps_.push_back(absolutePath);
        // Don't preprocess map files, just mark them as included
        return result;
    }
    
    // Track .dmf files  
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".dmf") {
        IncludedInterface_ = absolutePath;
        // Don't preprocess interface files, just mark them as included
        return result;
    }
    
    // Verbose logging
    if (Compiler_ && Compiler_->GetSettings().Verbose) {
        std::cout << "  Including file: " << absolutePath << std::endl;
    }
    
    // Create lexer for this file (with whitespace emission enabled for preprocessing)
    auto lexer = std::make_unique<DMLexer>(absolutePath, content, true);
    int depth = static_cast<int>(FileStack_.size());
    FileStack_.push(FileContext(std::move(lexer), absolutePath, depth));
    
    // Process tokens from this file
    bool processingThisFile = true;
    while (!FileStack_.empty() && processingThisFile) {
        Token token = GetNextRawToken();
        
        switch (token.Type) {
            case TokenType::EndOfFile:
                // Check if this EOF is from the file we're processing
                if (!FileStack_.empty() && 
                    FileStack_.top().Lexer->GetCurrentLocation().SourceFile == absolutePath) {
                    // We've finished processing this file
                    FileStack_.pop();
                    processingThisFile = false;
                } else {
                    // This EOF is from an included file, pop it and continue
                    FileStack_.pop();
                }
                break;
                
            case TokenType::Newline:
                CanUseDirective_ = true;
                if (CurrentLineContainsNonWhitespace_) {
                    BufferedWhitespace_ = std::stack<Token>(); // Clear
                    CurrentLineContainsNonWhitespace_ = false;
                    result.push_back(token);
                } else {
                    BufferedWhitespace_ = std::stack<Token>(); // Clear without output
                }
                break;
                
            // Whitespace - buffer at start of line, emit immediately otherwise
            case TokenType::DM_Preproc_Whitespace:
                if (CurrentLineContainsNonWhitespace_) {
                    result.push_back(token);
                } else {
                    BufferedWhitespace_.push(token);
                }
                break;
            
            // Preprocessor identifiers - try macro expansion first
            case TokenType::DM_Preproc_Identifier:
            case TokenType::Identifier: {
                // Try macro expansion before outputting
                if (TryExpandMacro(token)) {
                    break;
                }
                
                // Not a macro - flush buffered whitespace and output
                while (!BufferedWhitespace_.empty()) {
                    result.push_back(BufferedWhitespace_.top());
                    BufferedWhitespace_.pop();
                }
                
                CurrentLineContainsNonWhitespace_ = true;
                result.push_back(token);
                break;
            }
            
            // Preprocessor directives
            case TokenType::DM_Preproc_Include:
                HandleIncludeDirective(token);
                break;
                
            case TokenType::DM_Preproc_Define:
                HandleDefineDirective(token);
                break;
                
            case TokenType::DM_Preproc_Undefine:
                HandleUndefineDirective(token);
                break;
                
            case TokenType::DM_Preproc_If:
                HandleIfDirective(token);
                break;
                
            case TokenType::DM_Preproc_Ifdef:
                HandleIfDefDirective(token);
                break;
                
            case TokenType::DM_Preproc_Ifndef:
                HandleIfNDefDirective(token);
                break;
                
            case TokenType::DM_Preproc_Elif:
                HandleElifDirective(token);
                break;
                
            case TokenType::DM_Preproc_Else:
                HandleElseDirective(token);
                break;
                
            case TokenType::DM_Preproc_EndIf:
                HandleEndIfDirective(token);
                break;
                
            case TokenType::DM_Preproc_Error:
                HandleErrorDirective(token);
                break;
                
            case TokenType::DM_Preproc_Warning:
                HandleWarningDirective(token);
                break;
                
            case TokenType::DM_Preproc_Pragma:
                HandlePragmaDirective(token);
                break;
                
            default:
                // Flush buffered whitespace
                while (!BufferedWhitespace_.empty()) {
                    result.push_back(BufferedWhitespace_.top());
                    BufferedWhitespace_.pop();
                }
                
                CurrentLineContainsNonWhitespace_ = true;
                result.push_back(token);
                break;
        }
    }
    
    return result;
}

std::string DMPreprocessor::ResolvePath(const std::string& path, const std::string& currentFile) {
    namespace fs = std::filesystem;
    
    // Create cache key from path and current file
    std::string cacheKey = currentFile + "|" + path;
    
    // Check cache first
    auto it = PathCache_.find(cacheKey);
    if (it != PathCache_.end()) {
        return it->second;
    }
    
    // Normalize path separators (convert backslashes to forward slashes for consistency)
    std::string normalizedPath = path;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
    
    // If absolute path, use it directly
    if (fs::path(normalizedPath).is_absolute()) {
        PathCache_[cacheKey] = normalizedPath;
        return normalizedPath;
    }
    
    // Otherwise, resolve relative to current file's directory
    fs::path currentPath = fs::path(currentFile).parent_path();
    fs::path resolvedPath = currentPath / normalizedPath;
    
    // Normalize the resolved path
    try {
        resolvedPath = fs::canonical(resolvedPath);
    } catch (const fs::filesystem_error&) {
        // If canonical fails (file doesn't exist yet), just use the resolved path
        resolvedPath = fs::absolute(resolvedPath);
    }
    
    std::string result = resolvedPath.string();
    
    // Cache the resolved path for reuse
    PathCache_[cacheKey] = result;
    
    return result;
}

// Directive handler implementations
void DMPreprocessor::HandleIncludeDirective(const Token& token) {
    // Read the file path token using GetNextRawToken
    Token pathToken = GetNextRawToken();
    
    // Skip whitespace
    while (pathToken.Type == TokenType::DM_Preproc_Whitespace) {
        pathToken = GetNextRawToken();
    }
    
    // Extract file path
    if (pathToken.Type != TokenType::DM_Preproc_ConstantString && 
        pathToken.Type != TokenType::String && 
        pathToken.Type != TokenType::Identifier) {
        ReportError(token.Loc, "#include directive requires a file path (string or identifier)");
        return;
    }
    
    // Extract the file path (remove quotes if present)
    std::string filePath = pathToken.Text;
    if (filePath.length() >= 2 && filePath.front() == '"' && filePath.back() == '"') {
        filePath = filePath.substr(1, filePath.length() - 2);
    }
    
    // Resolve path relative to current file
    std::string resolvedPath = filePath;
    if (!FileStack_.empty()) {
        std::string currentFile = FileStack_.top().FilePath;
        namespace fs = std::filesystem;
        fs::path currentDir = fs::path(currentFile).parent_path();
        resolvedPath = (currentDir / filePath).string();
    }
    
    // Check if already included (skip if so)
    namespace fs = std::filesystem;
    std::string absolutePath;
    try {
        absolutePath = fs::absolute(resolvedPath).string();
    } catch (...) {
        ReportError(token.Loc, "Invalid file path: " + filePath);
        return;
    }
    
    if (IncludedFiles_.count(absolutePath)) {
        if (Compiler_ && Compiler_->GetSettings().Verbose) {
            std::cout << "  Skipping already included: " << absolutePath << std::endl;
        }
        return; // Already included, skip
    }
    
    // Mark as included
    IncludedFiles_.insert(absolutePath);
    
    // Handle special file types (.dmm, .dmf) by tracking but not preprocessing
    if (resolvedPath.size() >= 4 && resolvedPath.substr(resolvedPath.size() - 4) == ".dmm") {
        IncludedMaps_.push_back(absolutePath);
        return; // Don't preprocess map files
    }
    
    if (resolvedPath.size() >= 4 && resolvedPath.substr(resolvedPath.size() - 4) == ".dmf") {
        IncludedInterface_ = absolutePath;
        return; // Don't preprocess interface files
    }
    
    // Call PushFile() to push included file onto stack (NO token accumulation)
    PushFile(resolvedPath, token.Loc);
}

void DMPreprocessor::HandleDefineDirective(const Token& token) {
    // Use GetNextRawToken() throughout to avoid recursive preprocessing
    // Macro definitions should read raw tokens, not preprocessed tokens
    Token defineIdentifier = GetNextRawToken();
    
    // Skip whitespace
    while (defineIdentifier.Type == TokenType::DM_Preproc_Whitespace) {
        defineIdentifier = GetNextRawToken();
    }
    
    // Accept both DM_Preproc_Identifier and regular Identifier tokens
    if (defineIdentifier.Type != TokenType::DM_Preproc_Identifier && 
        defineIdentifier.Type != TokenType::Identifier) {
        if (Compiler_) {
            Compiler_->ForcedError(token.Loc, "Expected identifier after #define");
        }
        return;
    }
    
    std::string macroName = defineIdentifier.Text;
    
    // Handle FILE_DIR specially
    if (macroName == "FILE_DIR") {
        Token dirToken = GetNextRawToken();
        
        // Skip whitespace
        while (dirToken.Type == TokenType::DM_Preproc_Whitespace) {
            dirToken = GetNextRawToken();
        }
        
        std::string dirValue;
        if (dirToken.Type == TokenType::DM_Preproc_ConstantString || dirToken.Type == TokenType::String) {
            // Extract string value (remove quotes)
            dirValue = dirToken.Text;
            if (dirValue.size() >= 2 && dirValue.front() == '"' && dirValue.back() == '"') {
                dirValue = dirValue.substr(1, dirValue.size() - 2);
            }
        } else if (dirToken.Type == TokenType::DM_Preproc_Punctuator_Period) {
            dirValue = ".";
        } else {
            if (Compiler_) {
                Compiler_->ForcedError(dirToken.Loc, "FILE_DIR requires a string or '.' as value");
            }
            ReadLineTokens(); // Consume rest of line
            return;
        }
        
        // Get current file's directory and combine with dirValue
        namespace fs = std::filesystem;
        std::string currentDir = ".";
        if (!FileStack_.empty()) {
            currentDir = FileStack_.top().Directory;
        }
        std::string fullPath = (fs::path(currentDir) / dirValue).string();
        
        if (Compiler_) {
            Compiler_->AddResourceDirectory(fullPath, dirToken.Loc);
        }
        
        // Consume rest of line
        ReadLineTokens();
        return;
    }
    
    // Check for reserved keyword 'defined'
    if (macroName == "defined") {
        if (Compiler_) {
            Compiler_->Emit(WarningCode::SoftReservedKeyword, defineIdentifier.Loc, 
                          "Reserved keyword 'defined' cannot be used as macro name");
        }
    }
    
    // Check for function-like macro
    Token nextToken = GetNextRawToken();
    
    std::vector<std::string> parameters;
    bool isFunctionMacro = false;
    bool foundVariadic = false;
    
    // Check for '(' - accept both preprocessor and regular token types
    if (nextToken.Type == TokenType::DM_Preproc_Punctuator_LeftParenthesis ||
        nextToken.Type == TokenType::LeftParenthesis) {
        // Function-like macro
        isFunctionMacro = true;
        
        while (true) {
            Token paramToken = GetNextRawToken();
            
            // Skip whitespace
            while (paramToken.Type == TokenType::DM_Preproc_Whitespace) {
                paramToken = GetNextRawToken();
            }
            
            // Check for ')' - accept both token types
            if (paramToken.Type == TokenType::DM_Preproc_Punctuator_RightParenthesis ||
                paramToken.Type == TokenType::RightParenthesis) {
                break;
            }
            
            if (paramToken.Type == TokenType::DM_Preproc_Identifier || paramToken.Type == TokenType::Identifier) {
                std::string paramName = paramToken.Text;
                
                // Check for variadic parameter (...)
                Token peek = GetNextRawToken();
                while (peek.Type == TokenType::DM_Preproc_Whitespace) {
                    peek = GetNextRawToken();
                }
                
                if (peek.Type == TokenType::DM_Preproc_Punctuator_Period || peek.Type == TokenType::Period) {
                    // Check for ... (three dots)
                    Token dot2 = GetNextRawToken();
                    Token dot3 = GetNextRawToken();
                    
                    if ((dot2.Type == TokenType::DM_Preproc_Punctuator_Period || dot2.Type == TokenType::Period) && 
                        (dot3.Type == TokenType::DM_Preproc_Punctuator_Period || dot3.Type == TokenType::Period)) {
                        paramName += "...";
                        foundVariadic = true;
                        
                        // Next token should be comma or right paren
                        peek = GetNextRawToken();
                        while (peek.Type == TokenType::DM_Preproc_Whitespace) {
                            peek = GetNextRawToken();
                        }
                    } else {
                        if (Compiler_) {
                            Compiler_->ForcedError(paramToken.Loc, 
                                "Invalid macro parameter, expected '...' after parameter name");
                        }
                        return;
                    }
                }
                
                if (foundVariadic && !parameters.empty() && parameters.back() != paramName) {
                    if (Compiler_) {
                        Compiler_->Emit(WarningCode::MalformedMacro, paramToken.Loc, 
                            "Variadic argument must be the last argument");
                    }
                }
                
                parameters.push_back(paramName);
                
                // peek already has the next token (comma or right paren)
                // Accept both token types for ')' and ','
                if (peek.Type == TokenType::DM_Preproc_Punctuator_RightParenthesis ||
                    peek.Type == TokenType::RightParenthesis) {
                    break;
                } else if (peek.Type != TokenType::DM_Preproc_Punctuator_Comma &&
                           peek.Type != TokenType::Comma) {
                    if (Compiler_) {
                        Compiler_->ForcedError(peek.Loc, "Expected ',' or ')' in macro parameter list");
                    }
                    return;
                }
            } else {
                if (Compiler_) {
                    Compiler_->ForcedError(paramToken.Loc, "Expected parameter name in macro");
                }
                return;
            }
        }
        
        nextToken = GetNextRawToken();
    }
    
    // Skip whitespace before macro body
    while (nextToken.Type == TokenType::DM_Preproc_Whitespace) {
        nextToken = GetNextRawToken();
    }
    
    // Read macro body tokens until newline
    // Use GetNextRawToken() to read raw tokens without preprocessing
    // This avoids recursive processing and stores raw tokens for later expansion
    std::vector<Token> macroBodyTokens;
    while (nextToken.Type != TokenType::Newline && nextToken.Type != TokenType::EndOfFile) {
        macroBodyTokens.push_back(nextToken);
        nextToken = GetNextRawToken();
    }
    
    // Remove trailing whitespace tokens from body
    while (!macroBodyTokens.empty() && macroBodyTokens.back().Type == TokenType::DM_Preproc_Whitespace) {
        macroBodyTokens.pop_back();
    }
    
    // Create and store the macro
    if (isFunctionMacro) {
        Defines_[macroName] = std::make_unique<DMMacroFunction>(parameters, macroBodyTokens);
    } else {
        Defines_[macroName] = std::make_unique<DMMacroText>(macroBodyTokens);
    }
}

void DMPreprocessor::HandleUndefineDirective(const Token& token) {
    // Use raw tokens for consistency with other directive handling
    Token defineIdentifier = GetNextRawToken();
    
    // Skip whitespace
    while (defineIdentifier.Type == TokenType::DM_Preproc_Whitespace) {
        defineIdentifier = GetNextRawToken();
    }
    
    if (defineIdentifier.Type != TokenType::DM_Preproc_Identifier && 
        defineIdentifier.Type != TokenType::Identifier) {
        std::cerr << "Error at " << token.Loc.ToString() << ": #undef directive requires a macro name (identifier), got token type " 
                  << static_cast<int>(defineIdentifier.Type) << std::endl;
        return;
    }
    
    Undefine(defineIdentifier.Text);
}

void DMPreprocessor::HandleIfDirective(const Token& token) {
    std::vector<Token> tokens = ReadLineTokens();
    
    if (tokens.empty()) {
        std::cerr << "Error at " << token.Loc.ToString() << ": #if directive requires an expression" << std::endl;
        LastIfEvaluations_.push(false);
        SkipIfBody(false);
        return;
    }
    
    bool result = EvaluateCondition(tokens);
    LastIfEvaluations_.push(result);
    
    if (!result) {
        SkipIfBody(false);
    }
}

void DMPreprocessor::HandleIfDefDirective(const Token& token) {
    Token defineToken = GetNextToken();
    
    // Skip whitespace
    while (defineToken.Type == TokenType::DM_Preproc_Whitespace) {
        defineToken = GetNextToken();
    }
    
    if (defineToken.Type != TokenType::DM_Preproc_Identifier && 
        defineToken.Type != TokenType::Identifier) {
        std::cerr << "Error at " << token.Loc.ToString() << ": #ifdef directive requires a macro name (identifier), got token type " 
                  << static_cast<int>(defineToken.Type) << std::endl;
        LastIfEvaluations_.push(false);
        SkipIfBody(false);
        return;
    }
    
    bool result = IsDefined(defineToken.Text);
    LastIfEvaluations_.push(result);
    
    if (!result) {
        SkipIfBody(false);
    }
}

void DMPreprocessor::HandleIfNDefDirective(const Token& token) {
    Token defineToken = GetNextToken();
    
    // Skip whitespace
    while (defineToken.Type == TokenType::DM_Preproc_Whitespace) {
        defineToken = GetNextToken();
    }
    
    if (defineToken.Type != TokenType::DM_Preproc_Identifier && 
        defineToken.Type != TokenType::Identifier) {
        std::cerr << "Error at " << token.Loc.ToString() << ": #ifndef directive requires a macro name (identifier), got token type " 
                  << static_cast<int>(defineToken.Type) << std::endl;
        LastIfEvaluations_.push(false);
        SkipIfBody(false);
        return;
    }
    
    bool result = !IsDefined(defineToken.Text);
    LastIfEvaluations_.push(result);
    
    if (!result) {
        SkipIfBody(false);
    }
}

void DMPreprocessor::HandleElifDirective(const Token& token) {
    if (LastIfEvaluations_.empty()) {
        std::cerr << "Error at " << token.Loc.ToString() << ": #elif directive without matching #if, #ifdef, or #ifndef" << std::endl;
        return;
    }
    
    bool wasTruthy = LastIfEvaluations_.top();
    
    if (wasTruthy) {
        // Previous condition was true, skip this branch
        SkipIfBody(false);
    } else {
        // Previous was false, evaluate this condition
        LastIfEvaluations_.pop();
        HandleIfDirective(token);  // Reuse #if logic
    }
}

void DMPreprocessor::HandleElseDirective(const Token& token) {
    if (LastIfEvaluations_.empty()) {
        std::cerr << "Error at " << token.Loc.ToString() << ": #else directive without matching #if, #ifdef, or #ifndef" << std::endl;
        return;
    }
    
    bool wasTruthy = LastIfEvaluations_.top();
    
    if (wasTruthy) {
        // Previous condition was true, skip else branch
        SkipIfBody(true);  // Skip until #endif
    }
    // If previous was false, we continue processing normally
}

void DMPreprocessor::HandleEndIfDirective(const Token& token) {
    if (LastIfEvaluations_.empty()) {
        std::cerr << "Error at " << token.Loc.ToString() << ": #endif directive without matching #if, #ifdef, or #ifndef" << std::endl;
        return;
    }
    
    LastIfEvaluations_.pop();
}

void DMPreprocessor::HandleErrorDirective(const Token& token) {
    std::vector<Token> tokens = ReadLineTokens();
    
    std::string message;
    for (const auto& t : tokens) {
        message += t.Text;
    }
    
    std::cerr << "Error directive at " << token.Loc.ToString() << ": " << message << std::endl;
}

void DMPreprocessor::HandleWarningDirective(const Token& token) {
    std::vector<Token> tokens = ReadLineTokens();
    
    std::string message;
    for (const auto& t : tokens) {
        message += t.Text;
    }
    
    std::cerr << "Warning directive at " << token.Loc.ToString() << ": " << message << std::endl;
}

void DMPreprocessor::HandlePragmaDirective(const Token& token) {
    // Pragma format: #pragma <warning_name_or_number> <disabled|notice|warning|error>
    Token warningNameToken = GetNextToken();
    
    // Skip whitespace
    while (warningNameToken.Type == TokenType::DM_Preproc_Whitespace) {
        warningNameToken = GetNextToken();
    }
    
    WarningCode warningCode = WarningCode::Unknown;
    
    if (warningNameToken.Type == TokenType::DM_Preproc_Identifier || 
        warningNameToken.Type == TokenType::Identifier) {
        // Try to parse warning name as enum
        std::string warningName = warningNameToken.Text;
        
        // Simple mapping - in real implementation, use a proper enum parser
        if (warningName == "SoftReservedKeyword") warningCode = WarningCode::SoftReservedKeyword;
        else if (warningName == "UnimplementedAccess") warningCode = WarningCode::UnimplementedAccess;
        else if (warningName == "UnsupportedAccess") warningCode = WarningCode::UnsupportedAccess;
        else if (warningName == "MissingInterpolatedExpression") warningCode = WarningCode::MissingInterpolatedExpression;
        else if (warningName == "PointlessParentCall") warningCode = WarningCode::PointlessParentCall;
        else if (warningName == "PointlessBuiltinCall") warningCode = WarningCode::PointlessBuiltinCall;
        else if (warningName == "SuspiciousMatrixCall") warningCode = WarningCode::SuspiciousMatrixCall;
        else if (warningName == "FallbackBuiltinArgument") warningCode = WarningCode::FallbackBuiltinArgument;
        else if (warningName == "MalformedRange") warningCode = WarningCode::MalformedRange;
        else if (warningName == "InvalidRange") warningCode = WarningCode::InvalidRange;
        else if (warningName == "BadExpression") warningCode = WarningCode::BadExpression;
        else if (warningName == "BadStatement") warningCode = WarningCode::BadStatement;
        else if (warningName == "BadToken") warningCode = WarningCode::BadToken;
        else if (warningName == "BadDefinition") warningCode = WarningCode::BadDefinition;
        else if (warningName == "SoftConstant") warningCode = WarningCode::SoftConstant;
        else if (warningName == "HardConstant") warningCode = WarningCode::HardConstant;
        else if (warningName == "WriteToConstant") warningCode = WarningCode::WriteToConstant;
        else if (warningName == "DuplicateVariable") warningCode = WarningCode::DuplicateVariable;
        else if (warningName == "DuplicateProcDefinition") warningCode = WarningCode::DuplicateProcDefinition;
        else if (warningName == "PointlessScope") warningCode = WarningCode::PointlessScope;
        else if (warningName == "MalformedMacro") warningCode = WarningCode::MalformedMacro;
        else if (warningName == "UnsupportedTypeCheck") warningCode = WarningCode::UnsupportedTypeCheck;
        else {
            if (Compiler_) {
                Compiler_->Emit(WarningCode::Unknown, warningNameToken.Loc, 
                    "Warning '" + warningName + "' does not exist");
            }
            ReadLineTokens(); // Consume rest of line
            return;
        }
    } else if (warningNameToken.Type == TokenType::DM_Preproc_Number || 
               warningNameToken.Type == TokenType::Number) {
        // Parse as integer
        try {
            int warningNum = std::stoi(warningNameToken.Text);
            warningCode = static_cast<WarningCode>(warningNum);
        } catch (...) {
            if (Compiler_) {
                Compiler_->Emit(WarningCode::Unknown, warningNameToken.Loc, 
                    "Invalid warning number: " + warningNameToken.Text);
            }
            ReadLineTokens();
            return;
        }
    } else {
        if (Compiler_) {
            Compiler_->Emit(WarningCode::Unknown, warningNameToken.Loc, 
                "Invalid warning identifier");
        }
        ReadLineTokens();
        return;
    }
    
    // Read the pragma action (disabled/notice/warning/error)
    Token actionToken = GetNextToken();
    while (actionToken.Type == TokenType::DM_Preproc_Whitespace) {
        actionToken = GetNextToken();
    }
    
    if (actionToken.Type != TokenType::DM_Preproc_Identifier && 
        actionToken.Type != TokenType::Identifier) {
        if (Compiler_) {
            Compiler_->Emit(WarningCode::Unknown, actionToken.Loc, 
                "Warnings can only be set to disabled, notice, warning, or error");
        }
        ReadLineTokens();
        return;
    }
    
    std::string action = actionToken.Text;
    std::transform(action.begin(), action.end(), action.begin(), ::tolower);
    
    ErrorLevel level;
    if (action == "disabled" || action == "disable") {
        level = ErrorLevel::Disabled;
    } else if (action == "notice" || action == "pedantic" || action == "info") {
        level = ErrorLevel::Notice;
    } else if (action == "warning" || action == "warn") {
        level = ErrorLevel::Warning;
    } else if (action == "error" || action == "err") {
        level = ErrorLevel::Error;
    } else {
        if (Compiler_) {
            Compiler_->Emit(WarningCode::Unknown, actionToken.Loc, 
                "Warnings can only be set to disabled, notice, warning, or error");
        }
        ReadLineTokens();
        return;
    }
    
    if (Compiler_) {
        Compiler_->SetPragma(warningCode, level);
    }
    
    // Consume rest of line
    ReadLineTokens();
}

// Helper class for expression evaluation
class PreprocessorExpressionEvaluator {
private:
    std::vector<Token> tokens_;
    size_t pos_;
    DMPreprocessor* preprocessor_;
    std::unordered_set<std::string> expanding_macros_;  // Track macros being expanded to detect cycles

    Token Current() const {
        if (pos_ >= tokens_.size()) {
            return Token(TokenType::EndOfFile, "", Location::Internal, Token::TokenValue());
        }
        return tokens_[pos_];
    }

    void Advance() {
        if (pos_ < tokens_.size()) pos_++;
    }

    void SkipWhitespace() {
        while (pos_ < tokens_.size() && tokens_[pos_].Type == TokenType::DM_Preproc_Whitespace) {
            pos_++;
        }
    }

    // Try to expand a macro and evaluate it
    int64_t ExpandAndEvaluateMacro(const std::string& macroName) {
        // Check for cycles - if we're already expanding this macro, return 0 to prevent infinite recursion
        if (expanding_macros_.count(macroName)) {
            return 0;
        }
        
        // Check if it's defined
        if (!preprocessor_->IsDefined(macroName)) {
            return 0;
        }
        
        // Mark this macro as being expanded
        expanding_macros_.insert(macroName);
        
        // Expand the macro
        std::vector<Token> expandedTokens = preprocessor_->ExpandMacroForExpression(macroName);
        
        // If expansion failed (e.g., function-like macro), return 0
        if (expandedTokens.empty()) {
            expanding_macros_.erase(macroName);
            return 0;
        }
        
        // Filter out whitespace tokens
        std::vector<Token> filteredTokens;
        for (const auto& t : expandedTokens) {
            if (t.Type != TokenType::DM_Preproc_Whitespace) {
                filteredTokens.push_back(t);
            }
        }
        
        // If the expansion is empty, return 0
        if (filteredTokens.empty()) {
            expanding_macros_.erase(macroName);
            return 0;
        }
        
        // If the expansion is a single number, return it
        if (filteredTokens.size() == 1 && 
            (filteredTokens[0].Type == TokenType::DM_Preproc_Number || 
             filteredTokens[0].Type == TokenType::Number)) {
            expanding_macros_.erase(macroName);
            try {
                return std::stoll(filteredTokens[0].Text);
            } catch (...) {
                return 0;
            }
        }
        
        // If the expansion is a single identifier, recursively expand it
        if (filteredTokens.size() == 1 && 
            (filteredTokens[0].Type == TokenType::DM_Preproc_Identifier || 
             filteredTokens[0].Type == TokenType::Identifier)) {
            // Avoid infinite recursion by checking if it's the same identifier
            if (filteredTokens[0].Text == macroName) {
                expanding_macros_.erase(macroName);
                return 0;
            }
            int64_t result = ExpandAndEvaluateMacro(filteredTokens[0].Text);
            expanding_macros_.erase(macroName);
            return result;
        }
        
        // For complex expansions, create a new evaluator
        PreprocessorExpressionEvaluator subEvaluator(filteredTokens, preprocessor_);
        // Copy the expanding_macros_ set to the sub-evaluator to maintain cycle detection
        subEvaluator.expanding_macros_ = expanding_macros_;
        int64_t result = subEvaluator.Evaluate();
        expanding_macros_.erase(macroName);
        return result;
    }

    int64_t ParsePrimary() {
        SkipWhitespace();
        Token tok = Current();

        // Number literal
        if (tok.Type == TokenType::DM_Preproc_Number || tok.Type == TokenType::Number) {
            Advance();
            try {
                return std::stoll(tok.Text);
            } catch (...) {
                return 0;
            }
        }

        // defined(MACRO) operator
        if ((tok.Type == TokenType::DM_Preproc_Identifier || tok.Type == TokenType::Identifier) && tok.Text == "defined") {
            Advance();
            SkipWhitespace();
            
            bool hasParens = false;
            if (Current().Type == TokenType::DM_Preproc_Punctuator_LeftParenthesis || Current().Type == TokenType::LeftParenthesis) {
                hasParens = true;
                Advance();
                SkipWhitespace();
            }
            
            Token macroName = Current();
            if (macroName.Type != TokenType::DM_Preproc_Identifier && macroName.Type != TokenType::Identifier) {
                return 0;
            }
            Advance();
            
            if (hasParens) {
                SkipWhitespace();
                if (Current().Type == TokenType::DM_Preproc_Punctuator_RightParenthesis || Current().Type == TokenType::RightParenthesis) {
                    Advance();
                }
            }
            
            return preprocessor_->IsDefined(macroName.Text) ? 1 : 0;
        }

        // Identifier (macro name) - expand and evaluate it
        if (tok.Type == TokenType::DM_Preproc_Identifier || tok.Type == TokenType::Identifier) {
            std::string macroName = tok.Text;
            Advance();
            return ExpandAndEvaluateMacro(macroName);
        }

        // Parenthesized expression
        if (tok.Type == TokenType::DM_Preproc_Punctuator_LeftParenthesis || tok.Type == TokenType::LeftParenthesis) {
            Advance();
            int64_t result = ParseTernary();
            SkipWhitespace();
            if (Current().Type == TokenType::DM_Preproc_Punctuator_RightParenthesis || Current().Type == TokenType::RightParenthesis) {
                Advance();
            }
            return result;
        }

        // Unary operators
        if (tok.Text == "!") {
            Advance();
            return !ParseUnary();
        }
        if (tok.Text == "~") {
            Advance();
            return ~ParseUnary();
        }
        if (tok.Text == "-") {
            Advance();
            return -ParseUnary();
        }
        if (tok.Text == "+") {
            Advance();
            return ParseUnary();
        }

        return 0;
    }

    int64_t ParseUnary() {
        return ParsePrimary();
    }

    int64_t ParseMultiplicative() {
        int64_t left = ParseUnary();
        SkipWhitespace();
        
        while (true) {
            Token tok = Current();
            if (tok.Text == "*") {
                Advance();
                left *= ParseUnary();
            } else if (tok.Text == "/") {
                Advance();
                int64_t right = ParseUnary();
                left = (right != 0) ? (left / right) : 0;
            } else if (tok.Text == "%") {
                Advance();
                int64_t right = ParseUnary();
                left = (right != 0) ? (left % right) : 0;
            } else {
                break;
            }
            SkipWhitespace();
        }
        
        return left;
    }

    int64_t ParseAdditive() {
        int64_t left = ParseMultiplicative();
        SkipWhitespace();
        
        while (true) {
            Token tok = Current();
            if (tok.Text == "+") {
                Advance();
                left += ParseMultiplicative();
            } else if (tok.Text == "-") {
                Advance();
                left -= ParseMultiplicative();
            } else {
                break;
            }
            SkipWhitespace();
        }
        
        return left;
    }

    int64_t ParseShift() {
        int64_t left = ParseAdditive();
        SkipWhitespace();
        
        while (true) {
            Token tok = Current();
            if (tok.Text == "<<") {
                Advance();
                left <<= ParseAdditive();
            } else if (tok.Text == ">>") {
                Advance();
                left >>= ParseAdditive();
            } else {
                break;
            }
            SkipWhitespace();
        }
        
        return left;
    }

    int64_t ParseRelational() {
        int64_t left = ParseShift();
        SkipWhitespace();
        
        while (true) {
            Token tok = Current();
            if (tok.Text == "<") {
                Advance();
                left = left < ParseShift() ? 1 : 0;
            } else if (tok.Text == ">") {
                Advance();
                left = left > ParseShift() ? 1 : 0;
            } else if (tok.Text == "<=") {
                Advance();
                left = left <= ParseShift() ? 1 : 0;
            } else if (tok.Text == ">=") {
                Advance();
                left = left >= ParseShift() ? 1 : 0;
            } else {
                break;
            }
            SkipWhitespace();
        }
        
        return left;
    }

    int64_t ParseEquality() {
        int64_t left = ParseRelational();
        SkipWhitespace();
        
        while (true) {
            Token tok = Current();
            if (tok.Text == "==") {
                Advance();
                left = left == ParseRelational() ? 1 : 0;
            } else if (tok.Text == "!=") {
                Advance();
                left = left != ParseRelational() ? 1 : 0;
            } else {
                break;
            }
            SkipWhitespace();
        }
        
        return left;
    }

    int64_t ParseBitwiseAnd() {
        int64_t left = ParseEquality();
        SkipWhitespace();
        
        while (Current().Text == "&") {
            Advance();
            left &= ParseEquality();
            SkipWhitespace();
        }
        
        return left;
    }

    int64_t ParseBitwiseXor() {
        int64_t left = ParseBitwiseAnd();
        SkipWhitespace();
        
        while (Current().Text == "^") {
            Advance();
            left ^= ParseBitwiseAnd();
            SkipWhitespace();
        }
        
        return left;
    }

    int64_t ParseBitwiseOr() {
        int64_t left = ParseBitwiseXor();
        SkipWhitespace();
        
        while (Current().Text == "|") {
            Advance();
            left |= ParseBitwiseXor();
            SkipWhitespace();
        }
        
        return left;
    }

    int64_t ParseLogicalAnd() {
        int64_t left = ParseBitwiseOr();
        SkipWhitespace();
        
        while (Current().Text == "&&") {
            Advance();
            int64_t right = ParseBitwiseOr();
            left = (left && right) ? 1 : 0;
            SkipWhitespace();
        }
        
        return left;
    }

    int64_t ParseLogicalOr() {
        int64_t left = ParseLogicalAnd();
        SkipWhitespace();
        
        while (Current().Text == "||") {
            Advance();
            int64_t right = ParseLogicalAnd();
            left = (left || right) ? 1 : 0;
            SkipWhitespace();
        }
        
        return left;
    }

    int64_t ParseTernary() {
        int64_t condition = ParseLogicalOr();
        SkipWhitespace();
        
        if (Current().Text == "?") {
            Advance();
            int64_t trueVal = ParseTernary();
            SkipWhitespace();
            if (Current().Text == ":") {
                Advance();
                int64_t falseVal = ParseTernary();
                return condition ? trueVal : falseVal;
            }
        }
        
        return condition;
    }

public:
    PreprocessorExpressionEvaluator(const std::vector<Token>& tokens, DMPreprocessor* preprocessor)
        : tokens_(tokens), pos_(0), preprocessor_(preprocessor) {}

    int64_t Evaluate() {
        return ParseTernary();
    }
};

bool DMPreprocessor::EvaluateCondition(const std::vector<Token>& tokens) {
    if (tokens.empty()) {
        return false;
    }
    
    PreprocessorExpressionEvaluator evaluator(tokens, this);
    int64_t result = evaluator.Evaluate();
    return result != 0;
}

void DMPreprocessor::SkipIfBody(bool skipElse) {
    int depth = 1;  // Track nesting depth of #if directives
    
    while (depth > 0) {
        Token token = GetNextRawToken();
        
        if (token.Type == TokenType::EndOfFile) {
            std::cerr << "Error: Unexpected end of file while skipping conditional block" << std::endl;
            return;
        }
        
        // Check for preprocessor directives using token types
        if (token.Type == TokenType::DM_Preproc_If || 
            token.Type == TokenType::DM_Preproc_Ifdef || 
            token.Type == TokenType::DM_Preproc_Ifndef) {
            depth++;
        } else if (token.Type == TokenType::DM_Preproc_EndIf) {
            depth--;
        } else if (depth == 1 && !skipElse) {
            // Only stop at #else or #elif if we're at the top level and not skipping else
            if (token.Type == TokenType::DM_Preproc_Else) {
                return;  // Stop here, continue processing after #else
            } else if (token.Type == TokenType::DM_Preproc_Elif) {
                // Push the token back so HandleElifDirective can process it
                PushToken(std::move(token));
                return;
            }
        }
    }
}

Token DMPreprocessor::ConsumeToken() {
    return GetNextToken();
}

std::vector<Token> DMPreprocessor::ReadLineTokens() {
    std::vector<Token> tokens;
    Token token = GetNextToken();
    while (token.Type != TokenType::Newline && token.Type != TokenType::EndOfFile) {
        tokens.push_back(token);
        token = GetNextToken();
    }
    return tokens;
}

bool DMPreprocessor::IsDirective(TokenType type) const {
    // Check if token type is a preprocessor directive
    return false; // Simplified for now
}

// ============================================================================
// Streaming Directive Handlers (for GetNextToken)
// ============================================================================

void DMPreprocessor::HandleIncludeDirectiveStreaming(const Token& token) {
    // Read the file path token
    Token pathToken = GetNextRawToken();
    
    // Skip whitespace
    while (pathToken.Type == TokenType::DM_Preproc_Whitespace) {
        pathToken = GetNextRawToken();
    }
    
    // Extract file path
    if (pathToken.Type != TokenType::DM_Preproc_ConstantString && 
        pathToken.Type != TokenType::String && 
        pathToken.Type != TokenType::Identifier) {
        ReportError(token.Loc, "#include directive requires a file path (string or identifier)");
        return;
    }
    
    // Extract the file path (remove quotes if present)
    std::string filePath = pathToken.Text;
    if (filePath.length() >= 2 && filePath.front() == '"' && filePath.back() == '"') {
        filePath = filePath.substr(1, filePath.length() - 2);
    }
    
    // Resolve path relative to current file
    std::string resolvedPath = filePath;
    if (!FileStack_.empty()) {
        std::string currentFile = FileStack_.top().FilePath;
        namespace fs = std::filesystem;
        fs::path currentDir = fs::path(currentFile).parent_path();
        resolvedPath = (currentDir / filePath).string();
    }
    
    // Check if already included
    namespace fs = std::filesystem;
    std::string absolutePath;
    try {
        absolutePath = fs::absolute(resolvedPath).string();
    } catch (...) {
        ReportError(token.Loc, "Invalid file path: " + filePath);
        return;
    }
    
    if (IncludedFiles_.count(absolutePath)) {
        if (Compiler_ && Compiler_->GetSettings().Verbose) {
            std::cout << "  Skipping already included: " << absolutePath << std::endl;
        }
        return; // Already included, skip
    }
    
    // Mark as included
    IncludedFiles_.insert(absolutePath);
    
    // Handle special file types (.dmm, .dmf)
    if (resolvedPath.size() >= 4 && resolvedPath.substr(resolvedPath.size() - 4) == ".dmm") {
        IncludedMaps_.push_back(absolutePath);
        return; // Don't preprocess map files
    }
    
    if (resolvedPath.size() >= 4 && resolvedPath.substr(resolvedPath.size() - 4) == ".dmf") {
        IncludedInterface_ = absolutePath;
        return; // Don't preprocess interface files
    }
    
    // Push the file onto the stack (STREAMING - no token accumulation)
    PushFile(resolvedPath, token.Loc);
}

void DMPreprocessor::HandleIfDirectiveStreaming(const Token& token) {
    std::vector<Token> tokens = ReadLineTokens();
    
    if (tokens.empty()) {
        ReportError(token.Loc, "#if directive requires an expression");
        LastIfEvaluations_.push(false);
        SkipIfBody(false);
        return;
    }
    
    bool result = EvaluateCondition(tokens);
    LastIfEvaluations_.push(result);
    
    if (!result) {
        SkipIfBody(false);
    }
}

void DMPreprocessor::HandleIfDefDirectiveStreaming(const Token& token) {
    Token defineToken = GetNextRawToken();
    
    // Skip whitespace
    while (defineToken.Type == TokenType::DM_Preproc_Whitespace) {
        defineToken = GetNextRawToken();
    }
    
    if (defineToken.Type != TokenType::DM_Preproc_Identifier && 
        defineToken.Type != TokenType::Identifier) {
        ReportError(token.Loc, "#ifdef directive requires a macro name (identifier)");
        LastIfEvaluations_.push(false);
        SkipIfBody(false);
        return;
    }
    
    bool result = IsDefined(defineToken.Text);
    LastIfEvaluations_.push(result);
    
    if (!result) {
        SkipIfBody(false);
    }
}

void DMPreprocessor::HandleIfNDefDirectiveStreaming(const Token& token) {
    Token defineToken = GetNextRawToken();
    
    // Skip whitespace
    while (defineToken.Type == TokenType::DM_Preproc_Whitespace) {
        defineToken = GetNextRawToken();
    }
    
    if (defineToken.Type != TokenType::DM_Preproc_Identifier && 
        defineToken.Type != TokenType::Identifier) {
        ReportError(token.Loc, "#ifndef directive requires a macro name (identifier)");
        LastIfEvaluations_.push(false);
        SkipIfBody(false);
        return;
    }
    
    bool result = !IsDefined(defineToken.Text);
    LastIfEvaluations_.push(result);
    
    if (!result) {
        SkipIfBody(false);
    }
}

void DMPreprocessor::HandleElifDirectiveStreaming(const Token& token) {
    if (LastIfEvaluations_.empty()) {
        ReportError(token.Loc, "#elif directive without matching #if, #ifdef, or #ifndef");
        return;
    }
    
    bool wasTruthy = LastIfEvaluations_.top();
    
    if (wasTruthy) {
        // Previous condition was true, skip this branch
        SkipIfBody(false);
    } else {
        // Previous was false, evaluate this condition
        LastIfEvaluations_.pop();
        HandleIfDirectiveStreaming(token);  // Reuse #if logic
    }
}

void DMPreprocessor::HandleElseDirectiveStreaming(const Token& token) {
    if (LastIfEvaluations_.empty()) {
        ReportError(token.Loc, "#else directive without matching #if, #ifdef, or #ifndef");
        return;
    }
    
    bool wasTruthy = LastIfEvaluations_.top();
    
    if (wasTruthy) {
        // Previous condition was true, skip else branch
        SkipIfBody(true);  // Skip until #endif
    }
    // If previous was false, we continue processing normally
}

void DMPreprocessor::HandleEndIfDirectiveStreaming(const Token& token) {
    if (LastIfEvaluations_.empty()) {
        ReportError(token.Loc, "#endif directive without matching #if, #ifdef, or #ifndef");
        return;
    }
    
    LastIfEvaluations_.pop();
}

} // namespace DMCompiler
