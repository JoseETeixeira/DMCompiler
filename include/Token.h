#pragma once

#include <string>
#include <memory>
#include "Location.h"

namespace DMCompiler {

enum class TokenType {
    Unknown,
    Skip,
    EndOfFile,
    
    // Punctuation
    Newline,
    Semicolon,
    Comma,
    Colon,
    Period,
    Question,
    
    // Indentation
    Indent,
    Dedent,
    
    // DM Preprocessor tokens
    DM_Preproc_ConstantString,
    DM_Preproc_Define,
    DM_Preproc_Else,
    DM_Preproc_EndIf,
    DM_Preproc_Error,
    DM_Preproc_Identifier,
    DM_Preproc_If,
    DM_Preproc_Ifdef,
    DM_Preproc_Ifndef,
    DM_Preproc_Elif,
    DM_Preproc_Include,
    DM_Preproc_LineSplice,
    DM_Preproc_Number,
    DM_Preproc_ParameterStringify,
    DM_Preproc_Pragma,
    DM_Preproc_Punctuator,
    DM_Preproc_Punctuator_Colon,
    DM_Preproc_Punctuator_Comma,
    DM_Preproc_Punctuator_LeftBracket,
    DM_Preproc_Punctuator_LeftParenthesis,
    DM_Preproc_Punctuator_Period,
    DM_Preproc_Punctuator_Question,
    DM_Preproc_Punctuator_RightBracket,
    DM_Preproc_Punctuator_RightParenthesis,
    DM_Preproc_Punctuator_Semicolon,
    DM_Preproc_StringBegin,
    DM_Preproc_StringMiddle,
    DM_Preproc_StringEnd,
    DM_Preproc_TokenConcat,
    DM_Preproc_Undefine,
    DM_Preproc_Warning,
    DM_Preproc_Whitespace,
    
    // Brackets
    LeftParenthesis,
    RightParenthesis,
    LeftBracket,
    RightBracket,
    LeftCurlyBracket,
    RightCurlyBracket,
    
    // Operators
    Assign,
    Plus,
    Minus,
    Multiply,
    Divide,
    Modulo,
    Power,
    Equals,
    NotEquals,
    Less,
    Greater,
    LessOrEqual,
    GreaterOrEqual,
    LogicalAnd,
    LogicalOr,
    LogicalNot,
    BitwiseAnd,
    BitwiseOr,
    BitwiseXor,
    BitwiseNot,
    LeftShift,
    RightShift,
    
    // Compound assignment
    PlusAssign,
    MinusAssign,
    MultiplyAssign,
    DivideAssign,
    ModuloAssign,
    AndAssign,
    OrAssign,
    XorAssign,
    LeftShiftAssign,
    RightShiftAssign,
    OrOrAssign,         // ||=
    AndAndAssign,       // &&=
    ModuloModuloAssign, // %%=
    AssignInto,         // :=
    
    // Comparison operators
    TildeEquals,        // ~= (case-insensitive string comparison)
    TildeExclamation,   // ~! (case-insensitive string non-comparison)
    
    // Null-conditional operators
    QuestionDot,        // ?. (null-conditional member access)
    QuestionColon,      // ?: (null-coalescing/Elvis operator)
    QuestionBracket,    // ?[ (null-conditional index access)
    
    // Increment/Decrement
    Increment,
    Decrement,
    
    // Keywords
    Var,
    Proc,
    Verb,
    If,
    Else,
    For,
    While,
    Do,
    Switch,
    Case,
    Default,
    Return,
    Break,
    Continue,
    Goto,
    Del,
    New,
    In,
    To,
    Step,
    As,
    Null,
    Set,
    Tmp,
    Const,
    Static,
    Global,
    Spawn,
    Try,
    Catch,
    Throw,
    
    // Literals
    Identifier,
    String,
    Number,
    Resource,
    
    // Special
    Slash,
    Dot,
    DotDot,
    DotDotDot,
    Arrow,
    DoubleColon
};

class Token {
public:
    TokenType Type;
    std::string Text;
    Location Loc;  // Renamed from Location to Loc to avoid name conflict
    
    // Optional value (for numbers, etc.)
    struct TokenValue {
        enum class Type { None, Int, Float, String };
        Type ValueType;
        union {
            int64_t IntValue;
            double FloatValue;
        };
        std::string StringValue;
        
        TokenValue() : ValueType(Type::None), IntValue(0) {}
        explicit TokenValue(int64_t i) : ValueType(Type::Int), IntValue(i) {}
        explicit TokenValue(double f) : ValueType(Type::Float), FloatValue(f) {}
        explicit TokenValue(const std::string& s) : ValueType(Type::String), IntValue(0), StringValue(s) {}
    } Value;

    Token() : Type(TokenType::Unknown) {}
    Token(TokenType type, const std::string& text, const Location& location)
        : Type(type), Text(text), Loc(location) {}
    Token(TokenType type, const std::string& text, const Location& location, const TokenValue& value)
        : Type(type), Text(text), Loc(location), Value(value) {}

    std::string ToString() const;
};

} // namespace DMCompiler
