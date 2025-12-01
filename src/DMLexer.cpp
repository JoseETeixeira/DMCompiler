#include "DMLexer.h"
#include "DMConstants.h"
#include <cctype>
#include <unordered_map>
#include <iostream>

namespace DMCompiler {

static const std::unordered_map<std::string, TokenType> Keywords = {
    {"var", TokenType::Var},
    {"proc", TokenType::Proc},
    {"verb", TokenType::Verb},
    {"if", TokenType::If},
    {"else", TokenType::Else},
    {"for", TokenType::For},
    {"while", TokenType::While},
    {"do", TokenType::Do},
    {"switch", TokenType::Switch},
    {"case", TokenType::Case},
    {"default", TokenType::Default},
    {"return", TokenType::Return},
    {"break", TokenType::Break},
    {"continue", TokenType::Continue},
    {"goto", TokenType::Goto},
    {"del", TokenType::Del},
    {"new", TokenType::New},
    {"in", TokenType::In},
    {"to", TokenType::To},
    {"step", TokenType::Step},
    {"as", TokenType::As},
    {"null", TokenType::Null},
    {"set", TokenType::Set},
    {"tmp", TokenType::Tmp},
    {"const", TokenType::Const},
    {"static", TokenType::Static},
    {"global", TokenType::Global},
    {"spawn", TokenType::Spawn},
    {"try", TokenType::Try},
    {"catch", TokenType::Catch},
    {"throw", TokenType::Throw}
};

DMLexer::DMLexer(const std::string& sourceName, const std::string& source, bool emitWhitespace)
    : Lexer<char>(sourceName, std::vector<char>(source.begin(), source.end()))
    , EmitWhitespace_(emitWhitespace)
{
}

Token DMLexer::ParseNextToken() {
    if (AtEndOfSource_) {
        return CreateToken(TokenType::EndOfFile, "");
    }
    
    char current = GetCurrent();
    
    // Skip whitespace (spaces and tabs) - don't emit tokens
    if (current == ' ' || current == '\t') {
        SkipWhitespace();
        return ParseNextToken();
    }
    
    // Comments
    if (current == '/' && Peek() == '/') {
        SkipLineComment();
        return ParseNextToken();
    }
    if (current == '/' && Peek() == '*') {
        SkipBlockComment();
        return ParseNextToken();
    }
    
    // Newlines - just emit them without checking indentation
    // Indentation is handled by TokenStreamDMLexer
    if (current == '\n') {
        Location newlineLoc = CurrentLocation_;
        Advance();
        CurrentLocation_.Line++;
        CurrentLocation_.Column = 0;
        return Token(TokenType::Newline, "\n", newlineLoc);
    }
    
    // Multi-line string {" ... "}
    if (current == '{' && Peek() == '"') {
        return ParseMultiLineString();
    }

    // Track bracket nesting (used by derived classes)
    if (current == '(' || current == '[' || current == '{') {
        bracketNesting_++;
    } else if (current == ')' || current == ']' || current == '}') {
        bracketNesting_ = std::max(0, bracketNesting_ - 1);
    }
    
    // Strings
    if (current == '"' || current == '\'') {
        return ParseString();
    }
    
    // Resources
    if (current == '\'') {
        return ParseResource();
    }
    
    // Numbers
    if (IsDigit(current) || (current == '.' && IsDigit(Peek()))) {
        return ParseNumber();
    }
    
    // Preprocessor directives (starts with #)
    if (current == '#') {
        return ParsePreprocessorDirective();
    }
    
    // Identifiers and keywords
    if (IsIdentifierStart(current)) {
        return ParseIdentifierOrKeyword();
    }
    
    // Operators and punctuation
    return ParseOperator();
}

Token DMLexer::ParseIdentifierOrKeyword() {
    std::string identifier;
    Location startLoc = CurrentLocation_;
    
    while (!AtEndOfSource_ && IsIdentifierChar(GetCurrent())) {
        identifier += GetCurrent();
        Advance();

        if (identifier.length() > Limits::MAX_IDENTIFIER_LENGTH) {
            // Stop consuming to avoid memory issues
            break;
        }
    }
    
    TokenType type = GetKeywordType(identifier);
    return Token(type, identifier, startLoc);
}

Token DMLexer::ParseNumber() {
    std::string number;
    Location startLoc = CurrentLocation_;
    bool isFloat = false;
    bool isHex = false;
    
    // Check for hex prefix
    if (GetCurrent() == '0' && (Peek() == 'x' || Peek() == 'X')) {
        number += GetCurrent();
        Advance();
        number += GetCurrent();
        Advance();
        isHex = true;
        
        while (!AtEndOfSource_ && IsHexDigit(GetCurrent())) {
            number += GetCurrent();
            Advance();
        }
    } else {
        while (!AtEndOfSource_ && IsDigit(GetCurrent())) {
            number += GetCurrent();
            Advance();
        }
        
        // Check for decimal point
        if (!AtEndOfSource_ && GetCurrent() == '.' && IsDigit(Peek())) {
            isFloat = true;
            number += GetCurrent();
            Advance();
            
            while (!AtEndOfSource_ && IsDigit(GetCurrent())) {
                number += GetCurrent();
                Advance();
            }
        }
        
        // Check for exponent
        if (!AtEndOfSource_ && (GetCurrent() == 'e' || GetCurrent() == 'E')) {
            isFloat = true;
            number += GetCurrent();
            Advance();
            
            if (!AtEndOfSource_ && (GetCurrent() == '+' || GetCurrent() == '-')) {
                number += GetCurrent();
                Advance();
            }
            
            while (!AtEndOfSource_ && IsDigit(GetCurrent())) {
                number += GetCurrent();
                Advance();
            }
        }
    }
    
    Token::TokenValue value;
    if (isFloat) {
        value = Token::TokenValue(std::stod(number));
    } else if (isHex) {
        value = Token::TokenValue(std::stoll(number, nullptr, 16));
    } else {
        value = Token::TokenValue(std::stoll(number));
    }
    
    return Token(TokenType::Number, number, startLoc, value);
}

Token DMLexer::ParseString() {
    std::string str;
    Location startLoc = CurrentLocation_;
    char quote = GetCurrent();
    Advance(); // Skip opening quote
    
    while (!AtEndOfSource_ && GetCurrent() != quote) {
        if (str.length() > Limits::MAX_STRING_LENGTH) {
            // Stop consuming
            break;
        }

        if (GetCurrent() == '\\' && Peek() != '\0') {
            Advance();
            char escaped = GetCurrent();
            switch (escaped) {
                case 'n': str += '\n'; break;
                case 't': str += '\t'; break;
                case 'r': str += '\r'; break;
                case '\\': str += '\\'; break;
                case '"': str += '"'; break;
                case '\'': str += '\''; break;
                default: 
                    // Not a recognized escape sequence, keep the backslash
                    str += '\\';
                    str += escaped; 
                    break;
            }
        } else {
            str += GetCurrent();
        }
        Advance();
    }
    
    if (!AtEndOfSource_) {
        Advance(); // Skip closing quote
    }
    
    return Token(TokenType::String, str, startLoc, Token::TokenValue(str));
}

Token DMLexer::ParseMultiLineString() {
    std::string str;
    Location startLoc = CurrentLocation_;
    
    Advance(); // Skip '{'
    Advance(); // Skip '"'
    
    while (!AtEndOfSource_) {
        if (GetCurrent() == '"' && Peek() == '}') {
            break;
        }
        
        // Handle newlines to keep line count correct
        if (GetCurrent() == '\n') {
            CurrentLocation_.Line++;
            CurrentLocation_.Column = 0;
        }
        
        str += GetCurrent();
        Advance();
    }
    
    if (!AtEndOfSource_) {
        Advance(); // Skip '"'
        if (!AtEndOfSource_) {
            Advance(); // Skip '}'
        }
    }
    
    return Token(TokenType::String, str, startLoc, Token::TokenValue(str));
}

Token DMLexer::ParseResource() {
    std::string resource = "'";
    Location startLoc = CurrentLocation_;
    Advance(); // Skip opening '
    
    while (!AtEndOfSource_ && GetCurrent() != '\'') {
        resource += GetCurrent();
        Advance();
    }
    
    if (!AtEndOfSource_) {
        resource += GetCurrent();
        Advance(); // Skip closing '
    }
    
    return Token(TokenType::Resource, resource, startLoc);
}

Token DMLexer::ParseOperator() {
    Location startLoc = CurrentLocation_;
    char current = GetCurrent();
    char next = Peek();
    
    // Two-character operators
    if (current == '=' && next == '=') {
        Advance(); Advance();
        return Token(TokenType::Equals, "==", startLoc);
    }
    if (current == '!' && next == '=') {
        Advance(); Advance();
        return Token(TokenType::NotEquals, "!=", startLoc);
    }
    if (current == '<' && next == '=') {
        Advance(); Advance();
        return Token(TokenType::LessOrEqual, "<=", startLoc);
    }
    if (current == '>' && next == '=') {
        Advance(); Advance();
        return Token(TokenType::GreaterOrEqual, ">=", startLoc);
    }
    if (current == '&' && next == '&') {
        Advance(); Advance();
        return Token(TokenType::LogicalAnd, "&&", startLoc);
    }
    if (current == '|' && next == '|') {
        Advance(); Advance();
        return Token(TokenType::LogicalOr, "||", startLoc);
    }
    if (current == '<' && next == '<') {
        Advance(); Advance();
        return Token(TokenType::LeftShift, "<<", startLoc);
    }
    if (current == '>' && next == '>') {
        Advance(); Advance();
        return Token(TokenType::RightShift, ">>", startLoc);
    }
    if (current == '+' && next == '+') {
        Advance(); Advance();
        return Token(TokenType::Increment, "++", startLoc);
    }
    if (current == '-' && next == '-') {
        Advance(); Advance();
        return Token(TokenType::Decrement, "--", startLoc);
    }
    if (current == '+' && next == '=') {
        Advance(); Advance();
        return Token(TokenType::PlusAssign, "+=", startLoc);
    }
    if (current == '-' && next == '=') {
        Advance(); Advance();
        return Token(TokenType::MinusAssign, "-=", startLoc);
    }
    if (current == '*' && next == '=') {
        Advance(); Advance();
        return Token(TokenType::MultiplyAssign, "*=", startLoc);
    }
    if (current == '/' && next == '=') {
        Advance(); Advance();
        return Token(TokenType::DivideAssign, "/=", startLoc);
    }
    if (current == '.' && next == '.') {
        Advance(); Advance();
        if (!AtEndOfSource_ && GetCurrent() == '.') {
            Advance();
            return Token(TokenType::DotDotDot, "...", startLoc);
        }
        return Token(TokenType::DotDot, "..", startLoc);
    }
    if (current == ':' && next == ':') {
        Advance(); Advance();
        return Token(TokenType::DoubleColon, "::", startLoc);
    }
    
    // Single-character operators
    Advance();
    switch (current) {
        case '(': return Token(TokenType::LeftParenthesis, "(", startLoc);
        case ')': return Token(TokenType::RightParenthesis, ")", startLoc);
        case '[': return Token(TokenType::LeftBracket, "[", startLoc);
        case ']': return Token(TokenType::RightBracket, "]", startLoc);
        case '{': return Token(TokenType::LeftCurlyBracket, "{", startLoc);
        case '}': return Token(TokenType::RightCurlyBracket, "}", startLoc);
        case ';': return Token(TokenType::Semicolon, ";", startLoc);
        case ',': return Token(TokenType::Comma, ",", startLoc);
        case ':': return Token(TokenType::Colon, ":", startLoc);
        case '.': return Token(TokenType::Period, ".", startLoc);
        case '?': return Token(TokenType::Question, "?", startLoc);
        case '=': return Token(TokenType::Assign, "=", startLoc);
        case '+': return Token(TokenType::Plus, "+", startLoc);
        case '-': return Token(TokenType::Minus, "-", startLoc);
        case '*': return Token(TokenType::Multiply, "*", startLoc);
        case '/': return Token(TokenType::Divide, "/", startLoc);
        case '%': return Token(TokenType::Modulo, "%", startLoc);
        case '<': return Token(TokenType::Less, "<", startLoc);
        case '>': return Token(TokenType::Greater, ">", startLoc);
        case '!': return Token(TokenType::LogicalNot, "!", startLoc);
        case '&': return Token(TokenType::BitwiseAnd, "&", startLoc);
        case '|': return Token(TokenType::BitwiseOr, "|", startLoc);
        case '^': return Token(TokenType::BitwiseXor, "^", startLoc);
        case '~': return Token(TokenType::BitwiseNot, "~", startLoc);
        default: return Token(TokenType::Unknown, std::string(1, current), startLoc);
    }
}

void DMLexer::SkipWhitespace() {
    while (!AtEndOfSource_ && IsWhitespace(GetCurrent()) && GetCurrent() != '\n') {
        Advance();
    }
}

void DMLexer::SkipLineComment() {
    // Skip to end of line
    while (!AtEndOfSource_ && GetCurrent() != '\n') {
        Advance();
    }
    // Do NOT consume the newline. The newline is a token that should be emitted.
}

void DMLexer::SkipBlockComment() {
    Advance(); // Skip '/'
    Advance(); // Skip '*'
    
    while (!AtEndOfSource_) {
        if (GetCurrent() == '*' && Peek() == '/') {
            Advance(); // Skip '*'
            Advance(); // Skip '/'
            break;
        }
        if (GetCurrent() == '\n') {
            CurrentLocation_.Line++;
            CurrentLocation_.Column = 0;
        }
        Advance();
    }
    
    // After closing */, skip any trailing spaces or tabs (but not newlines)
    // This matches the C# preprocessor behavior
    while (!AtEndOfSource_ && (GetCurrent() == ' ' || GetCurrent() == '\t')) {
        Advance();
    }
}

Token DMLexer::ParsePreprocessorDirective() {
    Location startLoc = CurrentLocation_;
    Advance(); // Skip '#'
    
    // Skip whitespace after '#'
    while (!AtEndOfSource_ && (GetCurrent() == ' ' || GetCurrent() == '\t')) {
        Advance();
    }
    
    // Read the directive name
    std::string directiveName;
    while (!AtEndOfSource_ && IsIdentifierChar(GetCurrent())) {
        directiveName += GetCurrent();
        Advance();
    }
    
    // If no identifier follows #, skip it
    if (directiveName.empty()) {
        return ParseNextToken();
    }
    
    // Convert to lowercase for case-insensitive comparison
    std::string directiveLower = directiveName;
    for (char& c : directiveLower) {
        c = std::tolower(c);
    }
    
    // Map directive names to token types
    if (directiveLower == "include") {
        return Token(TokenType::DM_Preproc_Include, "#include", startLoc);
    } else if (directiveLower == "define") {
        return Token(TokenType::DM_Preproc_Define, "#define", startLoc);
    } else if (directiveLower == "undef") {
        return Token(TokenType::DM_Preproc_Undefine, "#undef", startLoc);
    } else if (directiveLower == "if") {
        return Token(TokenType::DM_Preproc_If, "#if", startLoc);
    } else if (directiveLower == "ifdef") {
        return Token(TokenType::DM_Preproc_Ifdef, "#ifdef", startLoc);
    } else if (directiveLower == "ifndef") {
        return Token(TokenType::DM_Preproc_Ifndef, "#ifndef", startLoc);
    } else if (directiveLower == "elif") {
        return Token(TokenType::DM_Preproc_Elif, "#elif", startLoc);
    } else if (directiveLower == "else") {
        return Token(TokenType::DM_Preproc_Else, "#else", startLoc);
    } else if (directiveLower == "endif") {
        return Token(TokenType::DM_Preproc_EndIf, "#endif", startLoc);
    } else if (directiveLower == "error") {
        return Token(TokenType::DM_Preproc_Error, "#error", startLoc);
    } else if (directiveLower == "warn" || directiveLower == "warning") {
        return Token(TokenType::DM_Preproc_Warning, "#warning", startLoc);
    } else if (directiveLower == "pragma") {
        return Token(TokenType::DM_Preproc_Pragma, "#pragma", startLoc);
    } else {
        // Unknown directive - skip it
        return ParseNextToken();
    }
}

bool DMLexer::IsWhitespace(char c) const {
    return c == ' ' || c == '\t' || c == '\r';
}

bool DMLexer::IsDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool DMLexer::IsHexDigit(char c) const {
    return IsDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool DMLexer::IsIdentifierStart(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool DMLexer::IsIdentifierChar(char c) const {
    return IsIdentifierStart(c) || IsDigit(c);
}

TokenType DMLexer::GetKeywordType(const std::string& identifier) const {
    auto it = Keywords.find(identifier);
    if (it != Keywords.end()) {
        return it->second;
    }
    return TokenType::Identifier;
}

int DMLexer::CheckIndentation() {
    int indentationLevel = 0;
    size_t savedIndex = CurrentIndex_;
    
    // Count spaces and tabs at the beginning of the line (without consuming them)
    while (!AtEndOfSource_ && (GetCurrent() == ' ' || GetCurrent() == '\t')) {
        indentationLevel++;
        Advance();
    }
    
    // If we hit another newline or EOF, it's an empty line - keep current indentation level
    if (AtEndOfSource_ || GetCurrent() == '\n') {
        CurrentIndex_ = savedIndex;
        return indentationStack_.top(); // Return current level, not 0!
    }
    
    // Don't restore position - we want to consume the whitespace
    return indentationLevel;
}

} // namespace DMCompiler
