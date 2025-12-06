#include "DMLexer.h"
#include "Token.h"
#include <vector>
#include <stack>
#include <queue>
#include <iostream>

namespace DMCompiler {

/// <summary>
/// Special DMLexer that streams pre-existing tokens instead of parsing source.
/// Used by the parser when working with preprocessed token streams.
/// Adds Indent/Dedent tokens based on whitespace tokens (matching C# implementation).
/// </summary>
class TokenStreamDMLexer : public DMLexer {
public:
    TokenStreamDMLexer(const std::vector<Token>& tokens)
        : DMLexer("preprocessed", ""),  // Empty source
          Tokens_(tokens),
          CurrentIndex_(0),
          BracketNesting_(0)
    {
        IndentationStack_.push(0);  // Initialize with 0 indentation
    }
    
protected:
    Token ParseNextToken() override {
        // Return any pending tokens first
        if (!PendingTokens_.empty()) {
            Token token = PendingTokens_.front();
            PendingTokens_.pop();
            return token;
        }
        
        // At end of file, emit remaining dedents
        if (CurrentIndex_ >= Tokens_.size()) {
            while (IndentationStack_.top() > 0) {
                IndentationStack_.pop();
                PendingTokens_.push(CreateToken(TokenType::Dedent, ""));
            }
            
            if (!PendingTokens_.empty()) {
                Token token = PendingTokens_.front();
                PendingTokens_.pop();
                return token;
            }
            
            // EOF should have column 0 (indicates start of a "virtual" line after file end)
            // This is important for indentation-based parsing
            Location eofLoc;
            if (!Tokens_.empty()) {
                eofLoc = Tokens_.back().Loc;
                eofLoc.Column = 0;  // Force column 0 for proper dedent handling
            } else {
                eofLoc = Location("", 0, 0);
            }
            CurrentLocation_ = eofLoc;
            AtEndOfSource_ = true;
            return CreateToken(TokenType::EndOfFile, "");
        }
        
        // Safety check to prevent infinite loops
        if (CurrentIndex_ >= Tokens_.size() + 1000) {
            std::cerr << "ERROR: Token index overflow at " << CurrentIndex_ << std::endl;
            AtEndOfSource_ = true;
            return CreateToken(TokenType::EndOfFile, "");
        }
        
        Token preprocToken = Tokens_[CurrentIndex_++];
        
        // Update location tracking
        PreviousLocation_ = CurrentLocation_;
        CurrentLocation_ = preprocToken.Loc;
        
        // Handle newlines - check for indentation changes
        if (preprocToken.Type == TokenType::Newline) {
            if (BracketNesting_ == 0) {  // Don't parse indentation inside brackets
                int currentIndentationLevel = IndentationStack_.top();
                int indentationLevel = CheckIndentation();
                
                if (indentationLevel > currentIndentationLevel) {
                    // Indent
                    IndentationStack_.push(indentationLevel);
                    PendingTokens_.push(preprocToken);  // Queue the newline
                    return CreateToken(TokenType::Indent, "");  // Return indent first
                } else if (indentationLevel < currentIndentationLevel) {
                    // Dedent
                    PendingTokens_.push(preprocToken);  // Queue the newline
                    
                    // Check if the dedent level is valid
                    bool validDedent = false;
                    std::stack<int> tempStack = IndentationStack_;
                    while (!tempStack.empty() && tempStack.top() > indentationLevel) {
                        tempStack.pop();
                        if (tempStack.top() == indentationLevel) {
                            validDedent = true;
                            break;
                        }
                    }
                    
                    // Emit dedent tokens
                    while (IndentationStack_.top() > indentationLevel) {
                        IndentationStack_.pop();
                        PendingTokens_.push(CreateToken(TokenType::Dedent, ""));
                    }
                    
                    Token token = PendingTokens_.front();
                    PendingTokens_.pop();
                    return token;
                } else {
                    // Same indentation
                    return preprocToken;
                }
            } else {
                // Inside brackets - just return newline
                return preprocToken;
            }
        }
        
        // Handle brackets for nesting tracking
        // Accept both preprocessor token types and regular token types
        if (preprocToken.Type == TokenType::DM_Preproc_Punctuator_LeftParenthesis ||
            preprocToken.Type == TokenType::DM_Preproc_Punctuator_LeftBracket ||
            preprocToken.Type == TokenType::LeftParenthesis ||
            preprocToken.Type == TokenType::LeftBracket) {
            BracketNesting_++;
        } else if (preprocToken.Type == TokenType::DM_Preproc_Punctuator_RightParenthesis ||
                   preprocToken.Type == TokenType::DM_Preproc_Punctuator_RightBracket ||
                   preprocToken.Type == TokenType::RightParenthesis ||
                   preprocToken.Type == TokenType::RightBracket) {
            BracketNesting_ = std::max(BracketNesting_ - 1, 0);
        }
        
        // Skip whitespace tokens - they're only used for indentation detection
        if (preprocToken.Type == TokenType::DM_Preproc_Whitespace) {
            // Recursively get the next token
            return ParseNextToken();
        }
        
        // Return the preprocessed token
        return preprocToken;
    }

private:
    /// <summary>
    /// Check indentation level after a newline.
    /// Looks for a DM_Preproc_Whitespace token and returns its length.
    /// Does NOT consume the token - caller must handle that.
    /// </summary>
    int CheckIndentation() {
        int indentationLevel = 0;
        
        // Check if the next token is whitespace
        if (CurrentIndex_ < Tokens_.size()) {
            const Token& current = Tokens_[CurrentIndex_];
            if (current.Type == TokenType::DM_Preproc_Whitespace) {
                indentationLevel = current.Text.length();
                // NOTE: We consume the whitespace token here
                CurrentIndex_++;
            }
        }
        
        return indentationLevel;
    }
    
    const std::vector<Token>& Tokens_;
    size_t CurrentIndex_;
    int BracketNesting_;
    std::stack<int> IndentationStack_;
    std::queue<Token> PendingTokens_;
};

} // namespace DMCompiler
