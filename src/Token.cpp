#include "Token.h"
#include <sstream>

namespace DMCompiler {

/// Convert TokenType enum to string for debugging
static const char* TokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::Unknown: return "Unknown";
        case TokenType::Skip: return "Skip";
        case TokenType::EndOfFile: return "EndOfFile";
        case TokenType::Newline: return "Newline";
        case TokenType::Semicolon: return "Semicolon";
        case TokenType::Comma: return "Comma";
        case TokenType::Colon: return "Colon";
        case TokenType::Period: return "Period";
        case TokenType::Question: return "Question";
        case TokenType::Indent: return "Indent";
        case TokenType::Dedent: return "Dedent";
        
        // Brackets
        case TokenType::LeftParenthesis: return "LeftParenthesis";
        case TokenType::RightParenthesis: return "RightParenthesis";
        case TokenType::LeftBracket: return "LeftBracket";
        case TokenType::RightBracket: return "RightBracket";
        case TokenType::LeftCurlyBracket: return "LeftCurlyBracket";
        case TokenType::RightCurlyBracket: return "RightCurlyBracket";
        
        // Operators
        case TokenType::Assign: return "Assign";
        case TokenType::Plus: return "Plus";
        case TokenType::Minus: return "Minus";
        case TokenType::Multiply: return "Multiply";
        case TokenType::Divide: return "Divide";
        case TokenType::Modulo: return "Modulo";
        case TokenType::Power: return "Power";
        case TokenType::Equals: return "Equals";
        case TokenType::NotEquals: return "NotEquals";
        case TokenType::Less: return "Less";
        case TokenType::Greater: return "Greater";
        case TokenType::LessOrEqual: return "LessOrEqual";
        case TokenType::GreaterOrEqual: return "GreaterOrEqual";
        case TokenType::LogicalAnd: return "LogicalAnd";
        case TokenType::LogicalOr: return "LogicalOr";
        case TokenType::LogicalNot: return "LogicalNot";
        case TokenType::BitwiseAnd: return "BitwiseAnd";
        case TokenType::BitwiseOr: return "BitwiseOr";
        case TokenType::BitwiseXor: return "BitwiseXor";
        case TokenType::BitwiseNot: return "BitwiseNot";
        case TokenType::LeftShift: return "LeftShift";
        case TokenType::RightShift: return "RightShift";
        
        // Compound assignment
        case TokenType::PlusAssign: return "PlusAssign";
        case TokenType::MinusAssign: return "MinusAssign";
        case TokenType::MultiplyAssign: return "MultiplyAssign";
        case TokenType::DivideAssign: return "DivideAssign";
        case TokenType::ModuloAssign: return "ModuloAssign";
        case TokenType::AndAssign: return "AndAssign";
        case TokenType::OrAssign: return "OrAssign";
        case TokenType::XorAssign: return "XorAssign";
        case TokenType::LeftShiftAssign: return "LeftShiftAssign";
        case TokenType::RightShiftAssign: return "RightShiftAssign";
        case TokenType::OrOrAssign: return "OrOrAssign";
        case TokenType::AndAndAssign: return "AndAndAssign";
        case TokenType::ModuloModuloAssign: return "ModuloModuloAssign";
        case TokenType::AssignInto: return "AssignInto";
        
        // Comparison operators
        case TokenType::TildeEquals: return "TildeEquals";
        case TokenType::TildeExclamation: return "TildeExclamation";
        
        // Null-conditional operators
        case TokenType::QuestionDot: return "QuestionDot";
        case TokenType::QuestionColon: return "QuestionColon";
        case TokenType::QuestionBracket: return "QuestionBracket";
        
        // Increment/Decrement
        case TokenType::Increment: return "Increment";
        case TokenType::Decrement: return "Decrement";
        
        // Keywords
        case TokenType::Var: return "Var";
        case TokenType::Proc: return "Proc";
        case TokenType::Verb: return "Verb";
        case TokenType::If: return "If";
        case TokenType::Else: return "Else";
        case TokenType::For: return "For";
        case TokenType::While: return "While";
        case TokenType::Do: return "Do";
        case TokenType::Switch: return "Switch";
        case TokenType::Case: return "Case";
        case TokenType::Default: return "Default";
        case TokenType::Return: return "Return";
        case TokenType::Break: return "Break";
        case TokenType::Continue: return "Continue";
        case TokenType::Goto: return "Goto";
        case TokenType::Del: return "Del";
        case TokenType::New: return "New";
        case TokenType::In: return "In";
        case TokenType::To: return "To";
        case TokenType::Step: return "Step";
        case TokenType::As: return "As";
        case TokenType::Null: return "Null";
        case TokenType::Set: return "Set";
        case TokenType::Tmp: return "Tmp";
        case TokenType::Const: return "Const";
        case TokenType::Static: return "Static";
        case TokenType::Global: return "Global";
        case TokenType::Spawn: return "Spawn";
        case TokenType::Try: return "Try";
        case TokenType::Catch: return "Catch";
        case TokenType::Throw: return "Throw";
        
        // Literals
        case TokenType::Identifier: return "Identifier";
        case TokenType::String: return "String";
        case TokenType::Number: return "Number";
        case TokenType::Resource: return "Resource";
        
        // Special
        case TokenType::Slash: return "Slash";
        case TokenType::Dot: return "Dot";
        case TokenType::DotDot: return "DotDot";
        case TokenType::DotDotDot: return "DotDotDot";
        case TokenType::Arrow: return "Arrow";
        case TokenType::DoubleColon: return "DoubleColon";
        
        // Preprocessor tokens
        case TokenType::DM_Preproc_ConstantString: return "DM_Preproc_ConstantString";
        case TokenType::DM_Preproc_Define: return "DM_Preproc_Define";
        case TokenType::DM_Preproc_Else: return "DM_Preproc_Else";
        case TokenType::DM_Preproc_EndIf: return "DM_Preproc_EndIf";
        case TokenType::DM_Preproc_Error: return "DM_Preproc_Error";
        case TokenType::DM_Preproc_Identifier: return "DM_Preproc_Identifier";
        case TokenType::DM_Preproc_If: return "DM_Preproc_If";
        case TokenType::DM_Preproc_Ifdef: return "DM_Preproc_Ifdef";
        case TokenType::DM_Preproc_Ifndef: return "DM_Preproc_Ifndef";
        case TokenType::DM_Preproc_Elif: return "DM_Preproc_Elif";
        case TokenType::DM_Preproc_Include: return "DM_Preproc_Include";
        case TokenType::DM_Preproc_LineSplice: return "DM_Preproc_LineSplice";
        case TokenType::DM_Preproc_Number: return "DM_Preproc_Number";
        case TokenType::DM_Preproc_ParameterStringify: return "DM_Preproc_ParameterStringify";
        case TokenType::DM_Preproc_Pragma: return "DM_Preproc_Pragma";
        case TokenType::DM_Preproc_Punctuator: return "DM_Preproc_Punctuator";
        case TokenType::DM_Preproc_Punctuator_Colon: return "DM_Preproc_Punctuator_Colon";
        case TokenType::DM_Preproc_Punctuator_Comma: return "DM_Preproc_Punctuator_Comma";
        case TokenType::DM_Preproc_Punctuator_LeftBracket: return "DM_Preproc_Punctuator_LeftBracket";
        case TokenType::DM_Preproc_Punctuator_LeftParenthesis: return "DM_Preproc_Punctuator_LeftParenthesis";
        case TokenType::DM_Preproc_Punctuator_Period: return "DM_Preproc_Punctuator_Period";
        case TokenType::DM_Preproc_Punctuator_Question: return "DM_Preproc_Punctuator_Question";
        case TokenType::DM_Preproc_Punctuator_RightBracket: return "DM_Preproc_Punctuator_RightBracket";
        case TokenType::DM_Preproc_Punctuator_RightParenthesis: return "DM_Preproc_Punctuator_RightParenthesis";
        case TokenType::DM_Preproc_Punctuator_Semicolon: return "DM_Preproc_Punctuator_Semicolon";
        case TokenType::DM_Preproc_StringBegin: return "DM_Preproc_StringBegin";
        case TokenType::DM_Preproc_StringMiddle: return "DM_Preproc_StringMiddle";
        case TokenType::DM_Preproc_StringEnd: return "DM_Preproc_StringEnd";
        case TokenType::DM_Preproc_TokenConcat: return "DM_Preproc_TokenConcat";
        case TokenType::DM_Preproc_Undefine: return "DM_Preproc_Undefine";
        case TokenType::DM_Preproc_Warning: return "DM_Preproc_Warning";
        case TokenType::DM_Preproc_Whitespace: return "DM_Preproc_Whitespace";
        
        default: return "Unknown";
    }
}

std::string Token::ToString() const {
    std::ostringstream oss;
    oss << "Token(" << TokenTypeToString(Type) << ", \"" << Text << "\")";
    return oss.str();
}

} // namespace DMCompiler
