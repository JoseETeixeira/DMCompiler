#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <string>
#include "Token.h"
#include "Location.h"

namespace DMCompiler {

/// <summary>
/// Base lexer class that tokenizes input
/// </summary>
template<typename TSourceType>
class Lexer {
protected:
    Location CurrentLocation_;
    Location PreviousLocation_;
    std::vector<TSourceType> Source_;
    size_t CurrentIndex_;
    bool AtEndOfSource_;
    std::queue<Token> PendingTokenQueue_;

public:
    Lexer(const std::string& sourceName, const std::vector<TSourceType>& source)
        : CurrentLocation_(sourceName, 1, 0)
        , PreviousLocation_(CurrentLocation_)
        , Source_(source)
        , CurrentIndex_(0)
        , AtEndOfSource_(false)
    {
    }

    virtual ~Lexer() = default;

    Token GetNextToken() {
        if (!PendingTokenQueue_.empty()) {
            Token token = PendingTokenQueue_.front();
            PendingTokenQueue_.pop();
            return token;
        }

        Token nextToken = ParseNextToken();
        while (nextToken.Type == TokenType::Skip) {
            nextToken = ParseNextToken();
        }

        if (!PendingTokenQueue_.empty()) {
            PendingTokenQueue_.push(nextToken);
            Token result = PendingTokenQueue_.front();
            PendingTokenQueue_.pop();
            return result;
        }

        return nextToken;
    }

    Location GetCurrentLocation() const { return CurrentLocation_; }
    Location GetPreviousLocation() const { return PreviousLocation_; }
    bool IsAtEndOfSource() const { return AtEndOfSource_; }

protected:
    virtual Token ParseNextToken() {
        if (CurrentIndex_ >= Source_.size()) {
            AtEndOfSource_ = true;
            return CreateToken(TokenType::EndOfFile, "");
        }
        return CreateToken(TokenType::Unknown, "");
    }

    Token CreateToken(TokenType type, const std::string& text) {
        return Token(type, text, PreviousLocation_);
    }

    Token CreateToken(TokenType type, const std::string& text, const Token::TokenValue& value) {
        return Token(type, text, PreviousLocation_, value);
    }

    TSourceType GetCurrent() const {
        if (CurrentIndex_ < Source_.size()) {
            return Source_[CurrentIndex_];
        }
        return TSourceType();
    }

    bool Advance() {
        if (CurrentIndex_ < Source_.size()) {
            PreviousLocation_ = CurrentLocation_;
            CurrentIndex_++;
            CurrentLocation_.Column++;
            return true;
        }
        AtEndOfSource_ = true;
        return false;
    }

    TSourceType Peek(size_t offset = 1) const {
        size_t index = CurrentIndex_ + offset;
        if (index < Source_.size()) {
            return Source_[index];
        }
        return TSourceType();
    }
};

} // namespace DMCompiler
