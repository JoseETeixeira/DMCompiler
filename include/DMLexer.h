#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stack>
#include <queue>
#include "Lexer.h"

namespace DMCompiler {

/// <summary>
/// DM-specific lexer that tokenizes Dream Maker code
/// </summary>
class DMLexer : public Lexer<char> {
public:
    DMLexer(const std::string& sourceName, const std::string& source, bool emitWhitespace = false);
    ~DMLexer() override = default;

protected:
    Token ParseNextToken() override;

private:
    bool EmitWhitespace_;  // Whether to emit whitespace tokens (for preprocessing)
    
    Token ParseIdentifierOrKeyword();
    Token ParseNumber();
    Token ParseString();
    Token ParseMultiLineString();
    Token ParseResource();
    Token ParseOperator();
    Token ParsePreprocessorDirective();
    Token ContinueString();  // Continue parsing a string after embedded expression
    
    bool IsWhitespace(char c) const;
    bool IsDigit(char c) const;
    bool IsHexDigit(char c) const;
    bool IsIdentifierStart(char c) const;
    bool IsIdentifierChar(char c) const;
    
    void SkipWhitespace();
    void SkipLineComment();
    void SkipBlockComment();
    
    TokenType GetKeywordType(const std::string& identifier) const;
    
    // Indentation tracking
    int CheckIndentation();
    std::stack<int> indentationStack_;
    std::queue<Token> pendingTokens_;
    int bracketNesting_ = 0;
    
    // String interpolation state
    bool inInterpolatedString_ = false;
    char stringQuoteChar_ = 0;
    int stringBracketNesting_ = 0;
    Location stringStartLoc_;
};

} // namespace DMCompiler
