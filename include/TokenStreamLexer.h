#pragma once

#include "Token.h"
#include "Location.h"
#include <vector>

namespace DMCompiler {

/// <summary>
/// Simple lexer that streams pre-existing tokens (from preprocessor).
/// Unlike DMLexer which parses source text, this just returns tokens from a vector.
/// </summary>
class TokenStreamLexer {
public:
    TokenStreamLexer(const std::vector<Token>& tokens)
        : Tokens_(tokens), CurrentIndex_(0) {}
    
    /// Get the next token from the stream
    Token GetNextToken() {
        if (CurrentIndex_ >= Tokens_.size()) {
            // Return EOF token
            Location eofLoc = Tokens_.empty() ? Location("", 0, 0) : Tokens_.back().Location;
            return Token(TokenType::EndOfFile, "", eofLoc);
        }
        return Tokens_[CurrentIndex_++];
    }
    
    /// Check if we're at the end of the token stream
    bool IsAtEndOfSource() const {
        return CurrentIndex_ >= Tokens_.size();
    }
    
    /// Get current location (location of the token we just returned)
    Location GetCurrentLocation() const {
        if (CurrentIndex_ == 0) {
            return Tokens_.empty() ? Location("", 0, 0) : Tokens_[0].Location;
        }
        if (CurrentIndex_ > Tokens_.size()) {
            return Tokens_.empty() ? Location("", 0, 0) : Tokens_.back().Location;
        }
        return Tokens_[CurrentIndex_ - 1].Location;
    }
    
    /// Get previous location
    Location GetPreviousLocation() const {
        if (CurrentIndex_ <= 1) {
            return Tokens_.empty() ? Location("", 0, 0) : Tokens_[0].Location;
        }
        return Tokens_[CurrentIndex_ - 2].Location;
    }

private:
    const std::vector<Token>& Tokens_;
    size_t CurrentIndex_;
};

} // namespace DMCompiler
