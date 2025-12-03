#pragma once

#include "DMLexer.h"
#include "DMAST.h"
#include "DMASTExpression.h"
#include "DMASTStatement.h"
#include "DreamPath.h"
#include "Token.h"
#include "DMValueType.h"
#include "DMConstants.h"
#include <vector>
#include <memory>
#include <array>
#include <unordered_set>
#include <stack>
#include <stdexcept>

namespace DMCompiler {

/// <summary>
/// RAII guard for tracking recursion depth in parser.
/// Automatically decrements depth when going out of scope.
/// Throws exception if max depth is exceeded.
/// </summary>
class RecursionGuard {
public:
    RecursionGuard(int& depth, int maxDepth, const char* context = nullptr)
        : Depth_(depth), Context_(context) {
        if (++Depth_ > maxDepth) {
            --Depth_;  // Restore before throwing
            throw std::runtime_error(
                std::string("Maximum nesting depth exceeded") + 
                (context ? std::string(" in ") + context : ""));
        }
    }
    
    ~RecursionGuard() { --Depth_; }
    
    // Non-copyable
    RecursionGuard(const RecursionGuard&) = delete;
    RecursionGuard& operator=(const RecursionGuard&) = delete;
    
private:
    int& Depth_;
    const char* Context_;
};

class DMCompiler;
enum class WarningCode;

/// <summary>
/// DM language parser that builds an Abstract Syntax Tree from tokens.
/// Implements recursive descent parsing with operator precedence handling.
/// </summary>
class DMParser {
public:
    DMParser(DMCompiler* compiler, DMLexer* lexer);
    
    /// <summary>
    /// Parse an entire DM file and return the root AST node.
    /// </summary>
    std::unique_ptr<DMASTFile> ParseFile();
    
    /// <summary>
    /// Parse a single expression (useful for testing)
    /// </summary>
    std::unique_ptr<DMASTExpression> Expression();
    
    /// <summary>
    /// Parse a single proc statement (useful for testing)
    /// </summary>
    std::unique_ptr<DMASTProcStatement> ProcStatement();
    
    /// <summary>
    /// Parse a top-level statement (proc/object definition)
    /// </summary>
    std::unique_ptr<DMASTStatement> Statement();
    
    /// <summary>
    /// Parse an object statement (for testing)
    /// </summary>
    std::unique_ptr<DMASTObjectStatement> ObjectStatement();

protected:
    // Base parser functionality
    Token Current() const { return CurrentToken_; }
    Token Advance();
    bool Check(TokenType type);
    void Consume(TokenType type, const std::string& errorMessage);
    void ReuseToken(Token token);
    void Warning(const std::string& message, const Token* token = nullptr);
    void Emit(WarningCode code, const std::string& message);
    
    // Path parsing (exposed for derived parsers like DMMParser)
    DMASTPath ParsePath();
    
    // Parser state
    DMCompiler* Compiler_;
    
private:
    DMLexer* Lexer_;
    Token CurrentToken_;
    std::stack<Token> TokenStack_;
    
    // Current parsing state
    DreamPath CurrentPath_;
    bool AllowVarDeclExpression_;
    
    // Progress tracking for loop detection
    size_t LastTokenPosition_;      // Last token position (line * 10000 + column)
    int NoProgressCounter_;         // Count of iterations without position change
    int NestingDepth_;              // Current recursion depth for expression parsing
    
    /// <summary>
    /// Check if parser is making progress. Returns false if stuck at same position
    /// for MAX_NO_PROGRESS_ITERATIONS iterations.
    /// </summary>
    bool CheckProgress();
    
    /// <summary>
    /// Get current token position as a single value for comparison.
    /// </summary>
    size_t GetTokenPosition() const;
    
    // Token type sets for parsing decisions
    static const std::array<TokenType, 15> AssignTypes_;
    static const std::array<TokenType, 4> ComparisonTypes_;
    static const std::array<TokenType, 4> LtGtComparisonTypes_;
    static const std::array<TokenType, 2> ShiftTypes_;
    static const std::array<TokenType, 2> PlusMinusTypes_;
    static const std::array<TokenType, 4> MulDivModTypes_;
    static const std::array<TokenType, 6> DereferenceTypes_;
    static const std::array<TokenType, 1> WhitespaceTypes_;
    static const std::array<TokenType, 3> IdentifierTypes_;
    static const std::array<TokenType, 11> ValidPathElementTokens_;
    static const std::array<TokenType, 2> ForSeparatorTypes_;
    
    // Helper methods for token checking
    Location CurrentLocation() const;
    bool IsInSet(TokenType type, const std::vector<TokenType>& set) const;
    template<size_t N>
    bool IsInSet(TokenType type, const std::array<TokenType, N>& set) const;
    
    // Whitespace and delimiter handling
    void Whitespace();
    bool Newline();
    bool Delimiter();
    bool PeekDelimiter();
    void LocateNextTopLevel();
    
    // Block parsing
    std::vector<std::unique_ptr<DMASTStatement>> BlockInner();
    std::unique_ptr<DMASTProcBlockInner> ProcBlock();
    
    // Expression parsing (adding incrementally)
    // Expression() is public for testing
    std::unique_ptr<DMASTExpression> PrimaryExpression();
    std::unique_ptr<DMASTExpression> UnaryExpression();
    std::unique_ptr<DMASTExpression> MultiplicationExpression();
    std::unique_ptr<DMASTExpression> AdditionExpression();
    std::unique_ptr<DMASTExpression> ShiftExpression();
    std::unique_ptr<DMASTExpression> ComparisonExpression();
    std::unique_ptr<DMASTExpression> BitwiseAndExpression();
    std::unique_ptr<DMASTExpression> BitwiseXorExpression();
    std::unique_ptr<DMASTExpression> BitwiseOrExpression();
    std::unique_ptr<DMASTExpression> LogicalAndExpression();
    std::unique_ptr<DMASTExpression> LogicalOrExpression();
    std::unique_ptr<DMASTExpression> TernaryExpression();
    std::unique_ptr<DMASTExpression> AssignmentExpression();
    std::unique_ptr<DMASTExpression> PostfixExpression();
    
    // Special DM expressions
    std::unique_ptr<DMASTExpression> PathExpression();
    std::unique_ptr<DMASTExpression> NewExpression();
    std::unique_ptr<DMASTExpression> ListExpression(const Location& loc, bool isAssociative);
    std::unique_ptr<DMASTExpression> NewListExpression(const Location& loc);
    std::unique_ptr<DMASTExpression> ParseInterpolatedString();  // Strings with [embedded] expressions
    
    // Proc statement parsing (ProcStatement is public for testing)
    std::unique_ptr<DMASTProcStatement> ProcStatementVarDeclaration();
    void ParseVarDeclarations(std::vector<DMASTProcStatementVarDeclaration::Decl>& decls, std::optional<DreamPath>& currentTypePath);
    std::unique_ptr<DMASTProcStatement> ProcStatementReturn();
    std::unique_ptr<DMASTProcStatement> ProcStatementIf();
    std::unique_ptr<DMASTProcStatement> ProcStatementIfWithBaseIndent(int baseIndent);  // For else-if chains
    std::unique_ptr<DMASTProcStatement> ProcStatementWhile();
    std::unique_ptr<DMASTProcStatement> ProcStatementDoWhile();
    std::unique_ptr<DMASTProcStatement> ProcStatementFor();
    std::unique_ptr<DMASTProcStatement> ProcStatementSwitch();
    std::unique_ptr<DMASTProcStatement> ProcStatementBreak();
    std::unique_ptr<DMASTProcStatement> ProcStatementContinue();
    std::unique_ptr<DMASTProcStatement> ProcStatementGoto();
    std::unique_ptr<DMASTProcStatement> ProcStatementLabel();
    std::unique_ptr<DMASTProcStatement> ProcStatementLabelNoColon();  // Label without colon (DM syntax)
    std::unique_ptr<DMASTProcStatement> ProcStatementDel();
    std::unique_ptr<DMASTProcStatement> ProcStatementSpawn();
    std::unique_ptr<DMASTProcStatement> ProcStatementTryCatch();
    std::unique_ptr<DMASTProcStatement> ProcStatementThrow();
    std::unique_ptr<DMASTProcStatement> ProcStatementSet();
    
    // Helper to parse a block of proc statements
    // baseIndent: the column position where the containing construct (e.g., proc definition) started
    //             used to determine when an indented block ends
    std::unique_ptr<DMASTProcBlockInner> ProcBlockInner(int baseIndent = -1);
    
    // Object/top-level statement parsing
    std::unique_ptr<DMASTObjectStatement> ObjectProcDefinition(bool isVerb = false);
    std::unique_ptr<DMASTObjectStatement> ObjectVarDefinition();
    std::unique_ptr<DMASTObjectStatement> ObjectDefinition();
    std::unique_ptr<DMASTDefinitionParameter> ProcParameter();
    
    // Helper methods
    BinaryOperator TokenTypeToBinaryOp(TokenType type);
    UnaryOperator TokenTypeToUnaryOp(TokenType type);
    AssignmentOperator TokenTypeToAssignmentOp(TokenType type);
    int GetCurrentIndentation();
    std::vector<std::unique_ptr<DMASTObjectStatement>> ParseIndentedObjectBlock(int baseIndent);
    
    // Type parsing for input() "as type" clause
    DMValueType ParseInputTypes();
    DMValueType ParseSingleInputType();
    
    // Error recovery
    void SkipToNextStatement();
    
    /// <summary>
    /// Recover from a parse error by skipping to the next statement boundary
    /// and emitting a warning about the skipped content.
    /// </summary>
    /// <param name="errorMessage">Description of what went wrong</param>
    /// <param name="loc">Location where the error occurred</param>
    void RecoverFromError(const std::string& errorMessage, const Location& loc);
};

// Template implementation for IsInSet
template<size_t N>
inline bool DMParser::IsInSet(TokenType type, const std::array<TokenType, N>& set) const {
    for (const auto& t : set) {
        if (t == type) return true;
    }
    return false;
}

}  // namespace DMCompiler
