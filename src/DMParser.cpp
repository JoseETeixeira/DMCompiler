#include "DMParser.h"
#include "DMCompiler.h"
#include "DMValueType.h"
#include <algorithm>
#include <iostream>

namespace DMCompiler {

// Token type sets for parsing decisions
const std::array<TokenType, 15> DMParser::AssignTypes_ = {
    TokenType::Assign,
    TokenType::PlusAssign,
    TokenType::MinusAssign,
    TokenType::OrAssign,
    TokenType::OrOrAssign,         // ||= (logical or assign)
    TokenType::AndAssign,
    TokenType::AndAndAssign,       // &&= (logical and assign)
    TokenType::MultiplyAssign,
    TokenType::DivideAssign,
    TokenType::LeftShiftAssign,
    TokenType::RightShiftAssign,
    TokenType::XorAssign,
    TokenType::ModuloAssign,
    TokenType::ModuloModuloAssign, // %%= (modulo-modulo assign)
    TokenType::AssignInto          // := (assign into)
};

const std::array<TokenType, 4> DMParser::ComparisonTypes_ = {
    TokenType::Equals,
    TokenType::NotEquals,
    TokenType::TildeEquals,        // ~= (equivalence comparison)
    TokenType::TildeExclamation    // ~! (non-equivalence comparison)
};

const std::array<TokenType, 4> DMParser::LtGtComparisonTypes_ = {
    TokenType::Less,
    TokenType::LessOrEqual,
    TokenType::Greater,
    TokenType::GreaterOrEqual
};

const std::array<TokenType, 2> DMParser::ShiftTypes_ = {
    TokenType::LeftShift,
    TokenType::RightShift
};

const std::array<TokenType, 2> DMParser::PlusMinusTypes_ = {
    TokenType::Plus,
    TokenType::Minus
};

const std::array<TokenType, 4> DMParser::MulDivModTypes_ = {
    TokenType::Multiply,
    TokenType::Divide,
    TokenType::Modulo,
    TokenType::Power
};

const std::array<TokenType, 6> DMParser::DereferenceTypes_ = {
    TokenType::Dot,
    TokenType::Colon,
    TokenType::DoubleColon,
    TokenType::QuestionDot,        // ?. (null-conditional member access)
    TokenType::QuestionColon,      // ?: (null-coalescing)
    TokenType::QuestionBracket     // ?[ (null-conditional index)
};

const std::array<TokenType, 1> DMParser::WhitespaceTypes_ = {
    TokenType::DM_Preproc_Whitespace  // Only skip actual whitespace, NOT Indent/Dedent!
};

const std::array<TokenType, 3> DMParser::IdentifierTypes_ = {
    TokenType::Identifier,
    TokenType::Step,
    TokenType::Proc
};

const std::array<TokenType, 11> DMParser::ValidPathElementTokens_ = {
    TokenType::Identifier,
    TokenType::Var,
    TokenType::Proc,
    TokenType::Step,
    TokenType::Verb,  // Throw doesn't exist - using Verb
    TokenType::Null,
    TokenType::Switch,
    TokenType::New,  // Spawn doesn't exist - using New
    TokenType::Do,
    TokenType::While,
    TokenType::For
};

const std::array<TokenType, 2> DMParser::ForSeparatorTypes_ = {
    TokenType::Semicolon,
    TokenType::Comma
};

DMParser::DMParser(DMCompiler* compiler, DMLexer* lexer)
    : Compiler_(compiler)
    , Lexer_(lexer)
    , CurrentPath_(DreamPath::Root)
    , AllowVarDeclExpression_(false)
    , LastTokenPosition_(0)
    , NoProgressCounter_(0)
    , NestingDepth_(0)
{
    // Initialize by advancing to first token
    Advance();
}

// ============================================================================
// Base Parser Methods
// ============================================================================

Token DMParser::Advance() {
    if (!TokenStack_.empty()) {
        CurrentToken_ = TokenStack_.top();
        TokenStack_.pop();
    } else {
        CurrentToken_ = Lexer_->GetNextToken();
        
        // Skip whitespace tokens (DM_Preproc_Whitespace)
        // The lexer outputs these during preprocessing, but the parser should ignore them
        while (CurrentToken_.Type == TokenType::DM_Preproc_Whitespace) {
            CurrentToken_ = Lexer_->GetNextToken();
        }
        
        // TODO: Handle error and warning tokens if lexer provides them
        // For now, Error and Warning token types don't exist in TokenType enum
        /*
        if (CurrentToken_.Type == TokenType::Error) {
            Emit(WarningCode::BadToken, CurrentToken_.Text);
            return Advance();
        } else if (CurrentToken_.Type == TokenType::Warning) {
            Warning(CurrentToken_.Text);
            return Advance();
        }
        */
    }
    return CurrentToken_;
}

bool DMParser::Check(TokenType type) {
    if (CurrentToken_.Type == type) {
        Advance();
        return true;
    }
    return false;
}

void DMParser::Consume(TokenType type, const std::string& errorMessage) {
    if (!Check(type)) {
        Emit(WarningCode::BadToken, errorMessage);
    }
}

void DMParser::ReuseToken(Token token) {
    TokenStack_.push(CurrentToken_);
    CurrentToken_ = token;
}

void DMParser::Warning(const std::string& message, const Token* token) {
    const Token& t = token ? *token : CurrentToken_;
    // TODO: Call compiler warning method
    // For now, just ignore
    (void)t;  // Suppress unused parameter warning
    (void)message;
}

void DMParser::Emit(WarningCode code, const std::string& message) {
    // TODO: Call compiler emit method
    (void)code;  // Suppress unused parameter warning
    (void)message;
}

Location DMParser::CurrentLocation() const {
    return Current().Loc;
}

size_t DMParser::GetTokenPosition() const {
    const Location& loc = CurrentToken_.Loc;
    // Combine line and column into a single value for comparison
    // Line * 100000 + Column gives a unique position identifier
    return static_cast<size_t>(loc.Line) * 100000 + static_cast<size_t>(loc.Column);
}

bool DMParser::CheckProgress() {
    size_t currentPosition = GetTokenPosition();
    
    if (currentPosition == LastTokenPosition_) {
        NoProgressCounter_++;
        
        if (NoProgressCounter_ >= Limits::MAX_NO_PROGRESS_ITERATIONS) {
            // We're stuck in an infinite loop
            Emit(WarningCode::BadToken, 
                 "Parser stuck at position " + std::to_string(CurrentToken_.Loc.Line) + 
                 ":" + std::to_string(CurrentToken_.Loc.Column) + 
                 " after " + std::to_string(NoProgressCounter_) + " iterations");
            
            // Reset counter and try to advance
            NoProgressCounter_ = 0;
            return false;
        }
    } else {
        // Progress made, reset counter
        LastTokenPosition_ = currentPosition;
        NoProgressCounter_ = 0;
    }
    
    return true;
}

bool DMParser::IsInSet(TokenType type, const std::vector<TokenType>& set) const {
    return std::find(set.begin(), set.end(), type) != set.end();
}

// ============================================================================
// File and Block Parsing
// ============================================================================

std::unique_ptr<DMASTFile> DMParser::ParseFile() {
    Location loc = CurrentLocation();
    std::vector<std::unique_ptr<DMASTStatement>> statements;
    
    Whitespace();
    
    Location lastLoc;
    int stuckCounter = 0;
    
    while (Current().Type != TokenType::EndOfFile) {
        if (Compiler_) {
            std::string progress = "Parsing (Line " + std::to_string(Current().Loc.Line) + ")";
            Compiler_->CheckProgress(progress);
        }
        
        Location currentLoc = CurrentLocation();
        
        // Detect if we're stuck on the same position
        if (currentLoc.Line == lastLoc.Line && currentLoc.Column == lastLoc.Column && currentLoc.SourceFile == lastLoc.SourceFile) {
            stuckCounter++;
            if (stuckCounter > 100) {
                // Force advance to break the loop
                Advance();
                stuckCounter = 0;
                continue;
            }
        } else {
            stuckCounter = 0;
            lastLoc = currentLoc;
        }
        
        try {
            auto stmt = Statement();
            if (stmt) {
                statements.push_back(std::move(stmt));
            } else {
                // If we can't parse a statement, skip to next line to avoid infinite loop
                while (Current().Type != TokenType::Newline && Current().Type != TokenType::EndOfFile) {
                    Advance();
                }
                if (Current().Type == TokenType::Newline) {
                    Advance();
                }
            }
        } catch (const std::exception& e) {
            // Recover from parse error and continue
            RecoverFromError(std::string("Parse error: ") + e.what(), currentLoc);
        }
        
        // Skip statement terminators
        while (Current().Type == TokenType::Semicolon || Current().Type == TokenType::Newline) {
            Advance();
        }
        
        Whitespace();
    }
    
    return std::make_unique<DMASTFile>(loc, std::move(statements));
}

std::vector<std::unique_ptr<DMASTStatement>> DMParser::BlockInner() {
    std::vector<std::unique_ptr<DMASTStatement>> statements;
    
    Whitespace();
    
    // Parse statements until we hit a closing brace, dedent, or EOF
    while (Current().Type != TokenType::RightCurlyBracket && 
           Current().Type != TokenType::Dedent &&
           Current().Type != TokenType::EndOfFile) {
        
        // Skip empty lines
        while (Current().Type == TokenType::Newline || Current().Type == TokenType::Semicolon) {
            Advance();
        }
        
        if (Current().Type == TokenType::RightCurlyBracket || 
            Current().Type == TokenType::Dedent ||
            Current().Type == TokenType::EndOfFile) {
            break;
        }
        
        Location stmtLoc = CurrentLocation();
        try {
            auto stmt = Statement();
            if (stmt) {
                statements.push_back(std::move(stmt));
            } else {
                // If Statement() returns null, skip to next line to avoid infinite loop
                while (Current().Type != TokenType::Newline && 
                       Current().Type != TokenType::Semicolon &&
                       Current().Type != TokenType::RightCurlyBracket &&
                       Current().Type != TokenType::EndOfFile) {
                    Advance();
                }
            }
        } catch (const std::exception& e) {
            // Recover from parse error and continue
            RecoverFromError(std::string("Parse error in block: ") + e.what(), stmtLoc);
        }
        
        // Skip statement terminators
        while (Current().Type == TokenType::Semicolon || Current().Type == TokenType::Newline) {
            Advance();
        }
        
        Whitespace();
    }
    
    return statements;
}

std::unique_ptr<DMASTProcBlockInner> DMParser::ProcBlock() {
    Location loc = CurrentLocation();
    std::vector<std::unique_ptr<DMASTProcStatement>> statements;
    std::vector<std::unique_ptr<DMASTProcStatement>> setStatements;
    
    // ProcBlock parses a proc body and separates set statements for hoisting
    // Set statements in DM are hoisted to the beginning of the proc
    
    // Parse the block using ProcBlockInner
    auto block = ProcBlockInner();
    
    if (block) {
        // Separate set statements from other statements
        for (auto& stmt : block->Statements) {
            if (dynamic_cast<DMASTProcStatementSet*>(stmt.get())) {
                setStatements.push_back(std::move(stmt));
            } else {
                statements.push_back(std::move(stmt));
            }
        }
    }
    
    return std::make_unique<DMASTProcBlockInner>(
        loc,
        std::move(statements),
        std::move(setStatements)
    );
}

// ============================================================================
// Expression Parsing
// ============================================================================

std::unique_ptr<DMASTExpression> DMParser::Expression() {
    // Check for infinite loop (same position for too many iterations)
    if (!CheckProgress()) {
        // Parser is stuck, try to advance and return null
        Advance();
        return nullptr;
    }
    
    // Guard against excessive nesting depth
    RecursionGuard guard(NestingDepth_, Limits::MAX_NESTING_DEPTH, "Expression");
    
    // Expression() calls the lowest precedence level
    // Assignment has very low precedence (only comma operator is lower)
    return AssignmentExpression();
}

std::unique_ptr<DMASTExpression> DMParser::PrimaryExpression() {
    Location loc = CurrentLocation();
    Token token = Current();
    
    // Integer literal
    if (token.Type == TokenType::Number) {
        // Check if it's an integer or float
        if (token.Value.ValueType == Token::TokenValue::Type::Int) {
            Advance();
            return std::make_unique<DMASTConstantInteger>(loc, token.Value.IntValue);
        } else if (token.Value.ValueType == Token::TokenValue::Type::Float) {
            Advance();
            return std::make_unique<DMASTConstantFloat>(loc, token.Value.FloatValue);
        }
    }
    
    // String literal
    if (token.Type == TokenType::String) {
        Advance();
        return std::make_unique<DMASTConstantString>(loc, token.Value.StringValue);
    }

    // Resource literal
    if (token.Type == TokenType::Resource) {
        Advance();
        return std::make_unique<DMASTConstantResource>(loc, token.Value.StringValue);
    }
    
    // Null literal
    if (token.Type == TokenType::Null) {
        Advance();
        return std::make_unique<DMASTConstantNull>(loc);
    }
    
    // Super proc call: ..
    // Check BEFORE path expression to distinguish .. (super) from ../path (upward path)
    if (token.Type == TokenType::DotDot) {
        // Peek ahead to see if it's followed by ( for super call or / for upward path
        Token nextToken = Advance();
        if (nextToken.Type == TokenType::LeftParenthesis) {
            // It's a super call: ..()
            // Return a special identifier that CompileCall will recognize
            return std::make_unique<DMASTIdentifier>(loc, "..");
        } else if (nextToken.Type == TokenType::Divide) {
            // It's an upward path: ../something
            // Backtrack and parse as path
            ReuseToken(token);
            return PathExpression();
        } else {
            // Just .. by itself - treat as super proc reference
            return std::make_unique<DMASTIdentifier>(loc, "..");
        }
    }
    
    // Single dot (.) - proc's implicit return value ("self")
    // Must check BEFORE path expression to distinguish . (self) from ./ (relative path)
    if (token.Type == TokenType::Dot || token.Type == TokenType::Period) {
        // The single dot (.) represents the proc's return value ("self")
        // We need to consume it and return an identifier, letting postfix handle any following operations
        Advance();  // Consume the dot
        return std::make_unique<DMASTIdentifier>(loc, ".");
    }
    
    // Path expression: /mob/player or ../parent or ./child
    // Must check BEFORE identifier to avoid treating / as division
    // Note: Lexer produces Divide token for '/', not Slash
    if (token.Type == TokenType::Divide) {
        return PathExpression();
    }
    
    // New expression: new /type or new type()
    if (token.Type == TokenType::New) {
        return NewExpression();
    }
    
    // List expression: list(1, 2, 3)
    // Note: We need to check if it's the 'list' identifier followed by parenthesis
    if (IsInSet(token.Type, IdentifierTypes_) && token.Text == "list") {
        Location listLoc = loc;
        Advance();
        if (Current().Type == TokenType::LeftParenthesis) {
            return ListExpression(listLoc, false); // false = not associative
        } else {
            // Just an identifier named 'list'
            return std::make_unique<DMASTIdentifier>(listLoc, "list");
        }
    }
    
    // newlist expression: newlist(/obj/item, /mob/player)
    if (IsInSet(token.Type, IdentifierTypes_) && token.Text == "newlist") {
        Location newlistLoc = loc;
        Advance();
        if (Current().Type == TokenType::LeftParenthesis) {
            return NewListExpression(newlistLoc);
        } else {
            // Just an identifier named 'newlist'
            return std::make_unique<DMASTIdentifier>(newlistLoc, "newlist");
        }
    }
    
    // Identifier or path
    if (IsInSet(token.Type, IdentifierTypes_)) {
        std::string identifier = token.Text;
        Advance();
        // For now, just return an identifier expression
        // Later we'll handle paths and member access
        return std::make_unique<DMASTIdentifier>(loc, identifier);
    }
    
    // Parenthesized expression
    if (token.Type == TokenType::LeftParenthesis) {
        Advance();
        auto expr = Expression();
        Consume(TokenType::RightParenthesis, "Expected ')' after expression");
        return expr;
    }
    
    // If we get here, it's an error
    Emit(WarningCode::BadToken, "Expected expression, got '" + token.Text + "' (type: " + std::to_string(static_cast<int>(token.Type)) + ")");
    Advance(); // Skip the bad token
    return std::make_unique<DMASTConstantNull>(loc); // Return null as error recovery
}

std::unique_ptr<DMASTExpression> DMParser::UnaryExpression() {
    Location loc = CurrentLocation();
    Token token = Current();
    
    // Check for unary operators: -, !, ~, ++, --
    if (token.Type == TokenType::Minus || 
        token.Type == TokenType::LogicalNot || 
        token.Type == TokenType::BitwiseNot ||
        token.Type == TokenType::Increment ||
        token.Type == TokenType::Decrement) {
        
        UnaryOperator op = TokenTypeToUnaryOp(token.Type);
        Advance();
        auto operand = UnaryExpression(); // Right-associative
        return std::make_unique<DMASTExpressionUnary>(loc, op, std::move(operand));
    }
    
    // Not a unary operator, parse postfix expression
    return PostfixExpression();
}

std::unique_ptr<DMASTExpression> DMParser::MultiplicationExpression() {
    auto left = UnaryExpression();
    
    while (IsInSet(Current().Type, MulDivModTypes_)) {
        Location loc = CurrentLocation();
        BinaryOperator op = TokenTypeToBinaryOp(Current().Type);
        Advance();
        auto right = UnaryExpression();
        left = std::make_unique<DMASTExpressionBinary>(loc, op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<DMASTExpression> DMParser::AdditionExpression() {
    auto left = MultiplicationExpression();
    
    while (IsInSet(Current().Type, PlusMinusTypes_)) {
        Location loc = CurrentLocation();
        BinaryOperator op = TokenTypeToBinaryOp(Current().Type);
        Advance();
        auto right = MultiplicationExpression();
        left = std::make_unique<DMASTExpressionBinary>(loc, op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<DMASTExpression> DMParser::ShiftExpression() {
    auto left = AdditionExpression();
    
    // Left-associative: keep consuming while we see << or >>
    while (IsInSet(Current().Type, ShiftTypes_)) {
        Location loc = CurrentLocation();
        BinaryOperator op = TokenTypeToBinaryOp(Current().Type);
        Advance();
        auto right = AdditionExpression();
        left = std::make_unique<DMASTExpressionBinary>(loc, op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<DMASTExpression> DMParser::ComparisonExpression() {
    auto left = ShiftExpression();
    
    // Left-associative: keep consuming while we see comparison operators
    // Handles: ==, !=, <, >, <=, >=, in
    while (IsInSet(Current().Type, ComparisonTypes_) || 
           IsInSet(Current().Type, LtGtComparisonTypes_) ||
           Current().Type == TokenType::In) {
        Location loc = CurrentLocation();
        BinaryOperator op = TokenTypeToBinaryOp(Current().Type);
        Advance();
        auto right = ShiftExpression();
        left = std::make_unique<DMASTExpressionBinary>(loc, op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<DMASTExpression> DMParser::BitwiseAndExpression() {
    auto left = ComparisonExpression();
    
    // Left-associative: keep consuming while we see &
    while (Current().Type == TokenType::BitwiseAnd) {
        Location loc = CurrentLocation();
        Advance();
        auto right = ComparisonExpression();
        left = std::make_unique<DMASTExpressionBinary>(loc, BinaryOperator::BitwiseAnd, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<DMASTExpression> DMParser::BitwiseXorExpression() {
    auto left = BitwiseAndExpression();
    
    // Left-associative: keep consuming while we see ^
    while (Current().Type == TokenType::BitwiseXor) {
        Location loc = CurrentLocation();
        Advance();
        auto right = BitwiseAndExpression();
        left = std::make_unique<DMASTExpressionBinary>(loc, BinaryOperator::BitwiseXor, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<DMASTExpression> DMParser::BitwiseOrExpression() {
    auto left = BitwiseXorExpression();
    
    // Left-associative: keep consuming while we see |
    while (Current().Type == TokenType::BitwiseOr) {
        Location loc = CurrentLocation();
        Advance();
        auto right = BitwiseXorExpression();
        left = std::make_unique<DMASTExpressionBinary>(loc, BinaryOperator::BitwiseOr, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<DMASTExpression> DMParser::LogicalAndExpression() {
    auto left = BitwiseOrExpression();
    
    // Left-associative: keep consuming while we see &&
    while (Current().Type == TokenType::LogicalAnd) {
        Location loc = CurrentLocation();
        Advance();
        auto right = BitwiseOrExpression();
        left = std::make_unique<DMASTExpressionBinary>(loc, BinaryOperator::LogicalAnd, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<DMASTExpression> DMParser::LogicalOrExpression() {
    auto left = LogicalAndExpression();
    
    // Left-associative: keep consuming while we see ||
    // OR has lower precedence than AND, so we call LogicalAndExpression()
    while (Current().Type == TokenType::LogicalOr) {
        Location loc = CurrentLocation();
        Advance();
        auto right = LogicalAndExpression();
        left = std::make_unique<DMASTExpressionBinary>(loc, BinaryOperator::LogicalOr, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<DMASTExpression> DMParser::PostfixExpression() {
    // Start with a primary expression
    auto expr = PrimaryExpression();
    
    // Handle postfix operations (left-to-right)
    // These include: member access (.), function calls (), array indexing [], post-increment (++), post-decrement (--)
    // Note: Colon (:) is NOT handled here to avoid conflicts with ternary operator
    while (true) {
        Location loc = CurrentLocation();
        
        // Member access: obj.property
        if (Current().Type == TokenType::Period) {
            Advance();
            
            // Parse the property name (identifier or expression in brackets)
            std::unique_ptr<DMASTExpression> property;
            if (IsInSet(Current().Type, IdentifierTypes_)) {
                Location propLoc = CurrentLocation();
                std::string propName = Current().Text;
                Advance();
                property = std::make_unique<DMASTIdentifier>(propLoc, propName);
            } else if (Current().Type == TokenType::LeftBracket) {
                // Dynamic property access: obj.[expression]
                Advance();
                property = Expression();
                Consume(TokenType::RightBracket, "Expected ']' after dynamic property access");
            } else {
                Emit(WarningCode::BadToken, "Expected property name after '.'");
                break;
            }
            
            expr = std::make_unique<DMASTDereference>(loc, std::move(expr), DereferenceType::Direct, std::move(property));
        }
        // Function call: func(args)
        else if (Current().Type == TokenType::LeftParenthesis) {
            Advance();
            
            // Parse argument list
            std::vector<std::unique_ptr<DMASTCallParameter>> parameters;
            
            if (Current().Type != TokenType::RightParenthesis) {
                do {
                    // Parse argument - handle named parameters (name = value)
                    // We parse up to ternary level to avoid treating = as assignment
                    auto argExpr = TernaryExpression();
                    
                    // Check for named parameter syntax: name = value
                    if (Current().Type == TokenType::Assign) {
                        // This is a named parameter: name = value
                        // The argExpr should be an identifier (the parameter name)
                        Location paramLoc = CurrentLocation();
                        Advance(); // consume '='
                        auto valueExpr = TernaryExpression();
                        
                        // Create a named parameter: key is the name, value is the expression
                        parameters.push_back(std::make_unique<DMASTCallParameter>(paramLoc, std::move(valueExpr), std::move(argExpr)));
                    } else {
                        // Simple positional argument
                        parameters.push_back(std::make_unique<DMASTCallParameter>(argExpr->Location_, std::move(argExpr)));
                    }
                    
                    if (Current().Type == TokenType::Comma) {
                        Advance();
                    } else {
                        break;
                    }
                } while (Current().Type != TokenType::RightParenthesis && !Check(TokenType::EndOfFile));
            }
            
            Consume(TokenType::RightParenthesis, "Expected ')' after function arguments");
            auto call = std::make_unique<DMASTCall>(loc, std::move(expr), std::move(parameters));
            
            // Check if this is an input() call and parse "as type" and "in list" clauses
            if (auto* ident = dynamic_cast<DMASTIdentifier*>(call->Target.get())) {
                if (ident->Identifier == "input") {
                    call->IsInputCall = true;
                    
                    // Parse "as type" clause
                    Whitespace();
                    bool hasAsClause = false;
                    if (Current().Type == TokenType::As) {
                        Advance();
                        Whitespace();
                        
                        // Parse type flags (e.g., "num", "text", "num|text")
                        call->InputTypes = ParseInputTypes();
                        hasAsClause = true;
                    }
                    
                    // Parse "in list" clause
                    Whitespace();
                    if (Current().Type == TokenType::In) {
                        Advance();
                        Whitespace();
                        call->InputList = Expression();
                        
                        // If no explicit "as" clause, default to "as anything" when there's a list
                        if (!hasAsClause) {
                            call->InputTypes = DMValueType::Anything;
                        }
                    } else {
                        // If no "in list" and no explicit "as", default to "as text"
                        if (!hasAsClause) {
                            call->InputTypes = DMValueType::Text;
                        }
                    }
                }
            }
            
            expr = std::move(call);
        }
        // Array indexing: array[index]
        // In DM, this is treated as member access with brackets
        else if (Current().Type == TokenType::LeftBracket) {
            Advance();
            auto index = Expression();
            Consume(TokenType::RightBracket, "Expected ']' after array index");
            
            // Array indexing is represented as dereference with the index as the property
            expr = std::make_unique<DMASTDereference>(loc, std::move(expr), DereferenceType::Index, std::move(index));
        }
        // Post-increment: x++
        else if (Current().Type == TokenType::Increment) {
            Advance();
            expr = std::make_unique<DMASTExpressionUnary>(loc, UnaryOperator::PostIncrement, std::move(expr));
        }
        // Post-decrement: x--
        else if (Current().Type == TokenType::Decrement) {
            Advance();
            expr = std::make_unique<DMASTExpressionUnary>(loc, UnaryOperator::PostDecrement, std::move(expr));
        }
        // No more postfix operations
        else {
            break;
        }
    }
    
    return expr;
}

// Parse path expressions: /mob/player, ../parent, ./child
std::unique_ptr<DMASTExpression> DMParser::PathExpression() {
    Location loc = CurrentLocation();
    
    // Determine path type based on leading token
    DreamPath::PathType pathType = DreamPath::PathType::Relative;
    
    // Note: Lexer produces Divide token for '/', not Slash
    if (Current().Type == TokenType::Divide) {
        Advance();
        // Check for "/.whatever/" which is upward search
        if (Current().Type == TokenType::Dot) {
            pathType = DreamPath::PathType::UpwardSearch;
        } else {
            pathType = DreamPath::PathType::Absolute;
        }
    } else if (Current().Type == TokenType::DotDot) {
        pathType = DreamPath::PathType::UpwardSearch;
        Advance();
        // Expect slash after ..
        if (Current().Type == TokenType::Divide) {
            Advance();
        }
    } else if (Current().Type == TokenType::Dot) {
        pathType = DreamPath::PathType::Relative;
        Advance();
        // Expect slash after .
        if (Current().Type == TokenType::Divide) {
            Advance();
        }
    }
    
    // Parse path elements (identifiers separated by /)
    std::vector<std::string> elements;
    while (IsInSet(Current().Type, IdentifierTypes_) || Current().Type == TokenType::Global) {
        elements.push_back(Current().Text);
        Advance();
        
        if (Current().Type == TokenType::Divide) {
            Advance();
        } else {
            break;
        }
    }
    
    // Create DreamPath from pathType and elements (not from string!)
    DreamPath dreamPath(pathType, elements);
    DMASTPath astPath(loc, dreamPath);
    return std::make_unique<DMASTConstantPath>(loc, astPath);
}

// Parse new expressions: new /type or new type()
std::unique_ptr<DMASTExpression> DMParser::NewExpression() {
    Location loc = CurrentLocation();
    Consume(TokenType::New, "Expected 'new'");
    
    // Parse the type expression
    // We use PrimaryExpression to handle paths, identifiers, parenthesized exprs
    auto typeExpr = PrimaryExpression();
    
    if (!typeExpr) {
        Emit(WarningCode::BadToken, "Expected type expression after 'new'");
        return std::make_unique<DMASTConstantNull>(loc);
    }
    
    // Check for optional constructor call parameters
    std::vector<std::unique_ptr<DMASTCallParameter>> parameters;
    if (Current().Type == TokenType::LeftParenthesis) {
        Advance();
        
        if (Current().Type != TokenType::RightParenthesis) {
            do {
                // Parse argument - handle named parameters (name = value)
                auto argExpr = TernaryExpression();
                
                // Check for named parameter syntax: name = value
                if (Current().Type == TokenType::Assign) {
                    Location paramLoc = CurrentLocation();
                    Advance(); // consume '='
                    auto valueExpr = TernaryExpression();
                    parameters.push_back(std::make_unique<DMASTCallParameter>(paramLoc, std::move(valueExpr), std::move(argExpr)));
                } else {
                    parameters.push_back(std::make_unique<DMASTCallParameter>(argExpr->Location_, std::move(argExpr)));
                }
                
                if (Current().Type == TokenType::Comma) {
                    Advance();
                } else {
                    break;
                }
            } while (Current().Type != TokenType::RightParenthesis && !Check(TokenType::EndOfFile));
        }
        
        Consume(TokenType::RightParenthesis, "Expected ')' after new arguments");
    }
    
    return std::make_unique<DMASTNewPath>(loc, std::move(typeExpr), std::move(parameters));
}

// Parse list expressions: list(1, 2, 3) or list("key" = value, ...)
std::unique_ptr<DMASTExpression> DMParser::ListExpression(const Location& loc, bool isAssociative) {
    // Already consumed 'list' identifier, now expect (
    Consume(TokenType::LeftParenthesis, "Expected '(' after 'list'");
    
    std::vector<std::unique_ptr<DMASTCallParameter>> values;
    
    if (Current().Type != TokenType::RightParenthesis) {
        do {
            // In list literals, "=" is used for key=value association, NOT assignment
            // So we parse up to ternary level (before assignment) and handle = separately
            auto keyExpr = TernaryExpression();
            
            // Check if this is a key = value association
            if (Current().Type == TokenType::Assign) {
                // This is an associative entry: key = value
                Location assocLoc = CurrentLocation();
                Advance(); // consume '='
                auto valueExpr = TernaryExpression();
                
                // Create an association expression (key, value pair)
                // DMASTCallParameter takes (location, value, key)
                values.push_back(std::make_unique<DMASTCallParameter>(assocLoc, std::move(valueExpr), std::move(keyExpr)));
            } else {
                // Simple value
                values.push_back(std::make_unique<DMASTCallParameter>(keyExpr->Location_, std::move(keyExpr)));
            }
            
            if (Current().Type == TokenType::Comma) {
                Advance();
            } else {
                break;
            }
        } while (Current().Type != TokenType::RightParenthesis && !Check(TokenType::EndOfFile));
    }
    
    Consume(TokenType::RightParenthesis, "Expected ')' after list values");
    return std::make_unique<DMASTList>(loc, std::move(values), isAssociative);
}

// Parse newlist expressions: newlist(/obj/item, /mob/player)
std::unique_ptr<DMASTExpression> DMParser::NewListExpression(const Location& loc) {
    // Already consumed 'newlist' identifier, now expect (
    Consume(TokenType::LeftParenthesis, "Expected '(' after 'newlist'");
    
    std::vector<std::unique_ptr<DMASTCallParameter>> parameters;
    
    if (Current().Type != TokenType::RightParenthesis) {
        do {
            // Parse argument - handle named parameters (name = value)
            auto argExpr = TernaryExpression();
            
            // Check for named parameter syntax: name = value
            if (Current().Type == TokenType::Assign) {
                Location paramLoc = CurrentLocation();
                Advance(); // consume '='
                auto valueExpr = TernaryExpression();
                parameters.push_back(std::make_unique<DMASTCallParameter>(paramLoc, std::move(valueExpr), std::move(argExpr)));
            } else {
                parameters.push_back(std::make_unique<DMASTCallParameter>(argExpr->Location_, std::move(argExpr)));
            }
            
            if (Current().Type == TokenType::Comma) {
                Advance();
            } else {
                break;
            }
        } while (Current().Type != TokenType::RightParenthesis && !Check(TokenType::EndOfFile));
    }
    
    Consume(TokenType::RightParenthesis, "Expected ')' after newlist parameters");
    return std::make_unique<DMASTNewList>(loc, std::move(parameters));
}

std::unique_ptr<DMASTExpression> DMParser::TernaryExpression() {
    // Parse condition
    auto condition = LogicalOrExpression();
    
    // Check for ternary operator (? :)
    // Ternary is RIGHT-associative: a ? b : c ? d : e means a ? b : (c ? d : e)
    if (Current().Type == TokenType::Question) {
        Location loc = CurrentLocation();
        Advance();
        
        // Parse true branch
        auto trueExpr = Expression();  // Allow full expressions including assignments in branches
        
        // Expect colon
        if (Current().Type != TokenType::Colon) {
            Emit(WarningCode::BadToken, "Expected ':' in ternary expression");
            return condition;  // Error recovery: return just the condition
        }
        Advance();
        
        // Parse false branch (recursive for right-associativity)
        auto falseExpr = TernaryExpression();
        
        return std::make_unique<DMASTTernary>(loc, std::move(condition), std::move(trueExpr), std::move(falseExpr));
    }
    
    // Not a ternary, just return the expression
    return condition;
}

std::unique_ptr<DMASTExpression> DMParser::AssignmentExpression() {
    // Parse left side (can be any expression for now, validation later)
    // Ternary has higher precedence than assignment
    auto left = TernaryExpression();
    
    // Check if we have an assignment operator
    // Assignment is RIGHT-associative: a = b = c means a = (b = c)
    if (IsInSet(Current().Type, AssignTypes_)) {
        Location loc = CurrentLocation();
        AssignmentOperator op = TokenTypeToAssignmentOp(Current().Type);
        Advance();
        
        // Recursively parse right side (allows chaining: x = y = z)
        auto right = AssignmentExpression();
        
        return std::make_unique<DMASTAssign>(loc, std::move(left), op, std::move(right));
    }
    
    // Not an assignment, just return the expression
    return left;
}

// ============================================================================
// Helper Methods
// ============================================================================

BinaryOperator DMParser::TokenTypeToBinaryOp(TokenType type) {
    switch (type) {
        // Arithmetic operators
        case TokenType::Plus: return BinaryOperator::Add;
        case TokenType::Minus: return BinaryOperator::Subtract;
        case TokenType::Multiply: return BinaryOperator::Multiply;
        case TokenType::Divide: return BinaryOperator::Divide;
        case TokenType::Modulo: return BinaryOperator::Modulo;
        case TokenType::Power: return BinaryOperator::Power;
        
        // Shift operators
        case TokenType::LeftShift: return BinaryOperator::LeftShift;
        case TokenType::RightShift: return BinaryOperator::RightShift;
        
        // Comparison operators
        case TokenType::Equals: return BinaryOperator::Equal;
        case TokenType::NotEquals: return BinaryOperator::NotEqual;
        case TokenType::Less: return BinaryOperator::Less;
        case TokenType::Greater: return BinaryOperator::Greater;
        case TokenType::LessOrEqual: return BinaryOperator::LessOrEqual;
        case TokenType::GreaterOrEqual: return BinaryOperator::GreaterOrEqual;
        
        // Equivalence operators (DM-specific)
        case TokenType::TildeEquals: return BinaryOperator::Equivalent;
        case TokenType::TildeExclamation: return BinaryOperator::NotEquivalent;
        
        // Bitwise operators
        case TokenType::BitwiseAnd: return BinaryOperator::BitwiseAnd;
        case TokenType::BitwiseXor: return BinaryOperator::BitwiseXor;
        case TokenType::BitwiseOr: return BinaryOperator::BitwiseOr;
        
        default:
            return BinaryOperator::Add; // Should never reach here
    }
}

UnaryOperator DMParser::TokenTypeToUnaryOp(TokenType type) {
    switch (type) {
        case TokenType::Minus: return UnaryOperator::Negate;
        case TokenType::LogicalNot: return UnaryOperator::Not;
        case TokenType::BitwiseNot: return UnaryOperator::BitNot;
        case TokenType::Increment: return UnaryOperator::PreIncrement;
        case TokenType::Decrement: return UnaryOperator::PreDecrement;
        default:
            return UnaryOperator::Negate; // Should never reach here
    }
}

AssignmentOperator DMParser::TokenTypeToAssignmentOp(TokenType type) {
    switch (type) {
        case TokenType::Assign: return AssignmentOperator::Assign;
        case TokenType::PlusAssign: return AssignmentOperator::AddAssign;
        case TokenType::MinusAssign: return AssignmentOperator::SubtractAssign;
        case TokenType::MultiplyAssign: return AssignmentOperator::MultiplyAssign;
        case TokenType::DivideAssign: return AssignmentOperator::DivideAssign;
        case TokenType::ModuloAssign: return AssignmentOperator::ModuloAssign;
        case TokenType::AndAssign: return AssignmentOperator::BitwiseAndAssign;
        case TokenType::OrAssign: return AssignmentOperator::BitwiseOrAssign;
        case TokenType::XorAssign: return AssignmentOperator::BitwiseXorAssign;
        case TokenType::LeftShiftAssign: return AssignmentOperator::LeftShiftAssign;
        case TokenType::RightShiftAssign: return AssignmentOperator::RightShiftAssign;
        case TokenType::OrOrAssign: return AssignmentOperator::LogicalOrAssign;
        case TokenType::AndAndAssign: return AssignmentOperator::LogicalAndAssign;
        case TokenType::ModuloModuloAssign: return AssignmentOperator::ModuloModuloAssign;
        case TokenType::AssignInto: return AssignmentOperator::AssignInto;
        default:
            return AssignmentOperator::Assign; // Should never reach here
    }
}

// ============================================================================
// Type Parsing for input() "as type" clause
// ============================================================================

DMValueType DMParser::ParseInputTypes() {
    DMValueType types = DMValueType::Anything;
    
    // Parse first type
    types = ParseSingleInputType();
    
    // Parse additional types separated by |
    while (Current().Type == TokenType::BitwiseOr) {
        Advance();
        Whitespace();
        types = types | ParseSingleInputType();
    }
    
    return types;
}

DMValueType DMParser::ParseSingleInputType() {
    Token typeToken = Current();
    
    if (typeToken.Type != TokenType::Identifier && typeToken.Type != TokenType::Null) {
        Warning("Expected type name in 'as' clause", &typeToken);
        return DMValueType::Anything;
    }
    
    Advance();
    
    const std::string& typeName = typeToken.Text;
    
    if (typeName == "anything") return DMValueType::Anything;
    if (typeName == "null") return DMValueType::Null;
    if (typeName == "text") return DMValueType::Text;
    if (typeName == "obj") return DMValueType::Obj;
    if (typeName == "mob") return DMValueType::Mob;
    if (typeName == "turf") return DMValueType::Turf;
    if (typeName == "num") return DMValueType::Num;
    if (typeName == "message") return DMValueType::Message;
    if (typeName == "area") return DMValueType::Area;
    if (typeName == "color") return DMValueType::Color;
    if (typeName == "file") return DMValueType::File;
    if (typeName == "command_text") return DMValueType::CommandText;
    if (typeName == "sound") return DMValueType::Sound;
    if (typeName == "icon") return DMValueType::Icon;
    if (typeName == "path") return DMValueType::Path;
    
    Warning("Invalid type name '" + typeName + "' in 'as' clause", &typeToken);
    return DMValueType::Anything;
}

// ============================================================================
// Whitespace and Delimiter Handling
// ============================================================================

void DMParser::Whitespace() {
    while (IsInSet(Current().Type, WhitespaceTypes_)) {
        Advance();
    }
}

bool DMParser::Newline() {
    bool hadNewline = false;
    while (Current().Type == TokenType::Newline || 
           Current().Type == TokenType::Skip) {
        if (Current().Type == TokenType::Newline) {
            hadNewline = true;
        }
        Advance();
    }
    return hadNewline;
}

bool DMParser::Delimiter() {
    Whitespace();
    bool hadDelimiter = false;
    
    while (Current().Type == TokenType::Semicolon || 
           Current().Type == TokenType::Newline) {
        hadDelimiter = true;
        Advance();
        Whitespace();
    }
    
    return hadDelimiter;
}

bool DMParser::PeekDelimiter() {
    TokenType type = Current().Type;
    return type == TokenType::Semicolon || 
           type == TokenType::Newline ||
           IsInSet(type, WhitespaceTypes_);
}

void DMParser::LocateNextTopLevel() {
    // Skip tokens until we find a potential statement start
    while (Current().Type != TokenType::EndOfFile) {
        TokenType type = Current().Type;
        
        // Look for path starts or keywords that begin statements
        if (type == TokenType::Slash || 
            type == TokenType::Var ||
            type == TokenType::Proc ||
            type == TokenType::Verb) {
            break;
        }
        
        Advance();
    }
}

// ============================================================================
// Statement and Expression Parsing - TODO: Add implementations
// ============================================================================
// Most parsing methods are temporarily commented out to get a minimal build working.
// These will be added back incrementally once the basic structure compiles.

void DMParser::SkipToNextStatement() {
    // Skip tokens until we find a statement boundary
    // Statement boundaries are: semicolon, newline, dedent, or end of file
    while (Current().Type != TokenType::EndOfFile &&
           Current().Type != TokenType::Semicolon &&
           Current().Type != TokenType::Newline &&
           Current().Type != TokenType::Dedent) {
        Advance();
    }
    // Don't consume the dedent - it's needed for block structure
    if (Current().Type == TokenType::Semicolon || Current().Type == TokenType::Newline) {
        Advance(); // Skip the delimiter
    }
}

void DMParser::RecoverFromError(const std::string& errorMessage, const Location& loc) {
    // Emit warning about the skipped content
    if (Compiler_) {
        std::string message = errorMessage + " at " + loc.ToString();
        message += " (skipping to next statement)";
        Compiler_->ForcedWarning(message);
    }
    
    // Skip to the next valid statement boundary
    SkipToNextStatement();
}

// ============================================================================
// Proc Statement Parsing
// ============================================================================

std::unique_ptr<DMASTProcBlockInner> DMParser::ProcBlockInner(int baseIndent) {
    Location loc = CurrentLocation();
    std::vector<std::unique_ptr<DMASTProcStatement>> statements;
    
    // If baseIndent not provided, use current location's column
    if (baseIndent == -1) {
        baseIndent = loc.Column;
    }
    
    // Parse single statement, braced block, or indented block
    if (Current().Type == TokenType::LeftCurlyBracket) {
        // Curly brace style
        Advance();
        Whitespace();
        
        while (Current().Type != TokenType::RightCurlyBracket && Current().Type != TokenType::EndOfFile) {
            Location stmtLoc = CurrentLocation();
            try {
                auto stmt = ProcStatement();
                if (stmt) {
                    statements.push_back(std::move(stmt));
                }
            } catch (const std::exception& e) {
                // Recover from parse error and continue
                RecoverFromError(std::string("Parse error in proc body: ") + e.what(), stmtLoc);
            }
            
            // Skip statement terminators
            while (Current().Type == TokenType::Semicolon || Current().Type == TokenType::Newline) {
                Advance();
            }
            Whitespace();
        }
        
        Consume(TokenType::RightCurlyBracket, "Expected '}'");
    } else if (Current().Type == TokenType::Newline) {
        // Indentation style - parse multiple indented statements
        Advance(); // Skip newline
        
        Location lastStmtLoc;
        int stuckLoopCount = 0;
        
        // Parse all statements that are indented more than base level
        while (Current().Type != TokenType::EndOfFile) {
            // Skip empty lines
            while (Current().Type == TokenType::Newline) {
                Advance();
            }
            
            if (Current().Type == TokenType::EndOfFile) {
                break;
            }
            
            // Check indentation
            int currentIndent = GetCurrentIndentation();
            
            // If we're back at or before the base indentation, we're done
            if (currentIndent <= baseIndent) {
                break;
            }
            
            // Detect infinite loop
            Location curLoc = CurrentLocation();
            if (curLoc.Line == lastStmtLoc.Line && curLoc.Column == lastStmtLoc.Column) {
                stuckLoopCount++;
                if (stuckLoopCount > 50) {
                    // Force break to prevent hang
                    break;
                }
            } else {
                stuckLoopCount = 0;
                lastStmtLoc = curLoc;
            }
            
            // Parse the statement with error recovery
            try {
                auto stmt = ProcStatement();
                if (stmt) {
                    statements.push_back(std::move(stmt));
                } else {
                    // Skip to next line
                    while (Current().Type != TokenType::Newline && Current().Type != TokenType::EndOfFile) {
                        Advance();
                    }
                }
            } catch (const std::exception& e) {
                // Recover from parse error and continue
                RecoverFromError(std::string("Parse error in proc body: ") + e.what(), curLoc);
            }
            
            // Skip trailing newlines/semicolons
            while (Current().Type == TokenType::Semicolon || Current().Type == TokenType::Newline) {
                Advance();
            }
        }
    } else {
        // Single statement on same line, or just a semicolon (empty body)
        // First, consume any leading semicolons (empty statements)
        while (Current().Type == TokenType::Semicolon) {
            Advance();
        }
        
        // Now try to parse a single statement if there is one
        auto stmt = ProcStatement();
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
    }
    
    return std::make_unique<DMASTProcBlockInner>(loc, std::move(statements));
}

std::unique_ptr<DMASTProcStatement> DMParser::ProcStatement() {
    Location loc = CurrentLocation();
    
    // Null statement (lone semicolon)
    if (Current().Type == TokenType::Semicolon) {
        return nullptr; // Will be handled by caller
    }
    
    // Check for variable declaration
    if (Current().Type == TokenType::Var) {
        return ProcStatementVarDeclaration();
    }
    
    // Check for control flow keywords first
    switch (Current().Type) {
        case TokenType::Return:
            return ProcStatementReturn();
        case TokenType::If:
            return ProcStatementIf();
        case TokenType::While:
            return ProcStatementWhile();
        case TokenType::Do:
            return ProcStatementDoWhile();
        case TokenType::For:
            return ProcStatementFor();
        case TokenType::Switch:
            return ProcStatementSwitch();
        case TokenType::Break:
            return ProcStatementBreak();
        case TokenType::Continue:
            return ProcStatementContinue();
        case TokenType::Goto:
            return ProcStatementGoto();
        case TokenType::Del:
            return ProcStatementDel();
        case TokenType::Spawn:
            return ProcStatementSpawn();
        case TokenType::Try:
            return ProcStatementTryCatch();
        case TokenType::Throw:
            return ProcStatementThrow();
        case TokenType::Set:
            return ProcStatementSet();
        default:
            break;
    }
    
    // Check for labeled statement (identifier:)
    // We need to look ahead to see if there's a colon after an identifier
    if (IsInSet(Current().Type, IdentifierTypes_)) {
        // Save current position in case this isn't a label
        Token identToken = Current();
        Token afterIdent = Advance();  // Advance and save the next token
        
        if (afterIdent.Type == TokenType::Colon) {
            // It's a label! Restore location and parse as label
            ReuseToken(identToken);
            return ProcStatementLabel();
        } else {
            // Not a label, put both tokens back and continue
            // We need to restore to the state before we consumed the identifier
            // Stack order: push afterIdent first, then identToken
            // So when we Advance(), we get identToken first, then afterIdent
            TokenStack_.push(afterIdent);
            CurrentToken_ = identToken;
        }
    }
    
    // Try to parse as expression statement
    auto expr = Expression();
    if (expr) {
        return std::make_unique<DMASTProcStatementExpression>(loc, std::move(expr));
    }
    
    return nullptr;
}

std::unique_ptr<DMASTProcStatement> DMParser::ProcStatementVarDeclaration() {
    Location loc = CurrentLocation();
    Consume(TokenType::Var, "Expected 'var'");
    
    std::vector<DMASTProcStatementVarDeclaration::Decl> decls;
    std::optional<DreamPath> currentTypePath;
    
    // Check for indented block
    if (Current().Type == TokenType::Newline) {
        Advance(); // Consume newline
        int baseIndent = loc.Column;
        
        while (Current().Type != TokenType::EndOfFile) {
            // Skip empty lines
            while (Current().Type == TokenType::Newline) {
                Advance();
            }
            if (Current().Type == TokenType::EndOfFile) break;
            
            // Check for Dedent - signals end of block
            if (Current().Type == TokenType::Dedent) {
                break;
            }

            int currentIndent = GetCurrentIndentation();
            if (currentIndent <= baseIndent) break;
            
            // Parse declarations on this line
            // Reset type path for each new line in a generic var block
            currentTypePath = std::nullopt;
            
            size_t startPos = GetTokenPosition();
            ParseVarDeclarations(decls, currentTypePath);
            
            // Safety check: if we didn't consume anything, force advance to avoid infinite loop
            if (GetTokenPosition() == startPos && Current().Type != TokenType::EndOfFile) {
                // If we're here, ParseVarDeclarations failed to parse anything but didn't consume the bad token
                // We must consume it to proceed
                Advance();
            }
            
            // Expect newline or semicolon at end of line
            while (Current().Type == TokenType::Semicolon || Current().Type == TokenType::Newline) {
                Advance();
            }
        }
    } else {
        // Single line mode
        ParseVarDeclarations(decls, currentTypePath);
    }
    
    return std::make_unique<DMASTProcStatementVarDeclaration>(loc, std::move(decls));
}

void DMParser::ParseVarDeclarations(std::vector<DMASTProcStatementVarDeclaration::Decl>& decls, std::optional<DreamPath>& currentTypePath) {
    do {
        Location declLoc = CurrentLocation();
        
        // Parse path elements
        std::vector<std::string> pathElements;
        bool isAbsolute = false;
        bool hasSlashes = false;
        
        if (Current().Type == TokenType::Divide) {
            Advance();
            isAbsolute = true;
            hasSlashes = true;
        }
        
        while (IsInSet(Current().Type, IdentifierTypes_)) {
            pathElements.push_back(Current().Text);
            Advance();
            
            if (Current().Type == TokenType::Divide) {
                Advance();
                hasSlashes = true;
            } else {
                break;
            }
        }
        
        if (pathElements.empty()) {
            Emit(WarningCode::BadToken, "Expected identifier in var declaration");
            break;
        }
        
        // Analyze path to split Name and Type
        std::string name = pathElements.back();
        std::optional<DreamPath> typePath;
        
        if (pathElements.size() > 1 || isAbsolute) {
            // We have a type component
            if (pathElements.size() > 1) {
                std::vector<std::string> typeElements(pathElements.begin(), pathElements.end() - 1);
                typePath = DreamPath(isAbsolute ? DreamPath::PathType::Absolute : DreamPath::PathType::Relative, typeElements);
                currentTypePath = typePath; // Update current type
            } else {
                // Size is 1 and absolute (/i) -> Type is empty
                currentTypePath = std::nullopt;
            }
        } else {
            // Relative path with 1 element (e.g. `x`).
            // Use inherited type.
            typePath = currentTypePath;
        }
        
        // Check for 'as' type specification
        std::optional<DMComplexValueType> explicitValueType;
        if (Current().Type == TokenType::As) {
            Advance();
            
            // Parse type flags (e.g., "num", "text", "num|text")
            std::string typeStr;
            
            // Collect type tokens until we hit a delimiter
            while (Current().Type != TokenType::Assign &&
                   Current().Type != TokenType::Semicolon &&
                   Current().Type != TokenType::Newline &&
                   Current().Type != TokenType::Comma &&
                   Current().Type != TokenType::EndOfFile) {
                
                if (Current().Type == TokenType::BitwiseOr) {
                    typeStr += "|";
                    Advance();
                } else if (IsInSet(Current().Type, IdentifierTypes_)) {
                    typeStr += Current().Text;
                    Advance();
                } else {
                    break;
                }
            }
            
            DMValueType flags = ParseTypeFlags(typeStr);
            explicitValueType = DMComplexValueType(flags);
        }
        
        // Check for [] suffix (e.g., var/list/L[] - indicates list variable)
        bool isList = false;
        if (Current().Type == TokenType::LeftBracket) {
            Advance(); // consume [
            // Optionally consume expression inside []
            if (Current().Type != TokenType::RightBracket) {
                auto sizeExpr = Expression();
                (void)sizeExpr;
            }
            Consume(TokenType::RightBracket, "Expected ']' after list size");
            isList = true;
        }
        
        // Optional initialization
        std::unique_ptr<DMASTExpression> value;
        if (Current().Type == TokenType::Assign) {
            Advance();
            value = Expression();
        }
        
        decls.emplace_back(declLoc, name, typePath, std::move(value), explicitValueType, isList);
        
        if (Current().Type == TokenType::Comma) {
            Advance();
        } else {
            break;
        }
        
    } while (true);
}

std::unique_ptr<DMASTProcStatement> DMParser::ProcStatementReturn() {
    Location loc = CurrentLocation();
    Consume(TokenType::Return, "Expected 'return'");
    
    // Return value is optional
    std::unique_ptr<DMASTExpression> value;
    if (Current().Type != TokenType::Semicolon && 
        Current().Type != TokenType::Newline && 
        Current().Type != TokenType::EndOfFile) {
        value = Expression();
    }
    
    return std::make_unique<DMASTProcStatementReturn>(loc, std::move(value));
}

std::unique_ptr<DMASTProcStatement> DMParser::ProcStatementIf() {
    Location loc = CurrentLocation();
    Consume(TokenType::If, "Expected 'if'");
    
    // Condition (required, in parentheses)
    Consume(TokenType::LeftParenthesis, "Expected '(' after 'if'");
    auto condition = Expression();
    Consume(TokenType::RightParenthesis, "Expected ')' after condition");
    
    if (!condition) {
        Emit(WarningCode::BadToken, "Expected condition in if statement");
        return nullptr;
    }
    
    // Consume only whitespace before body - let ProcBlockInner handle newlines
    Whitespace();
    
    // Body - pass the if statement's column as base indent for proper indentation handling
    auto body = ProcBlockInner(loc.Column);
    
    // After an indented body, we might be at a newline before the else
    // Consume any newlines/whitespace to check for else
    while (Current().Type == TokenType::Newline || IsInSet(Current().Type, WhitespaceTypes_)) {
        Advance();
    }
    
    // Else clause (optional)
    std::unique_ptr<DMASTProcBlockInner> elseBody;
    if (Current().Type == TokenType::Else) {
        Advance();
        
        // Consume only whitespace before else body - let ProcBlockInner handle newlines
        Whitespace();
        
        // Use if statement's column as base indent (else is at same level as if)
        elseBody = ProcBlockInner(loc.Column);
    }
    
    return std::make_unique<DMASTProcStatementIf>(loc, std::move(condition), 
                                                   std::move(body), std::move(elseBody));
}

std::unique_ptr<DMASTProcStatement> DMParser::ProcStatementWhile() {
    Location loc = CurrentLocation();
    Consume(TokenType::While, "Expected 'while'");
    
    // Condition (required, in parentheses)
    Consume(TokenType::LeftParenthesis, "Expected '(' after 'while'");
    auto condition = Expression();
    Consume(TokenType::RightParenthesis, "Expected ')' after condition");
    
    if (!condition) {
        Emit(WarningCode::BadToken, "Expected condition in while statement");
        return nullptr;
    }
    
    // Consume only whitespace before body - let ProcBlockInner handle newlines
    Whitespace();
    
    // Body - pass the while statement's column as base indent
    auto body = ProcBlockInner(loc.Column);
    
    return std::make_unique<DMASTProcStatementWhile>(loc, std::move(condition), std::move(body));
}

std::unique_ptr<DMASTProcStatement> DMParser::ProcStatementFor() {
    Location loc = CurrentLocation();
    Consume(TokenType::For, "Expected 'for'");
    
    Consume(TokenType::LeftParenthesis, "Expected '(' after 'for'");
    
    // Parse first expression - could be:
    // 1. for(var in list) - for-in loop
    // 2. for(var/mob/M in list) - for-in loop with typed variable
    // 3. for(x in list) - for-in loop with existing variable
    // 4. for(x = 0; ...) - traditional C-style for loop
    
    // Special handling: if we see 'var' followed by '/', we need to parse it as a path
    // expression for for-in loops (e.g., var/mob/M)
    std::unique_ptr<DMASTExpression> firstExpr;
    bool seenVar = false;

    // Check if there's actually an initializer (not empty for())
    if (Current().Type != TokenType::Semicolon && 
        Current().Type != TokenType::Comma &&
        Current().Type != TokenType::RightParenthesis) {
        // Check if this looks like a for-in loop variable declaration
        if (Current().Type == TokenType::Var) {
            seenVar = true;
            // Consume 'var' keyword
            Location varLoc = CurrentLocation();
            Advance();
            
            // Now parse the path (e.g., /mob/M or mob/M or just M)
            // The path should start with / or an identifier
            auto pathExpr = PathExpression();
            
            // Check for initialization: var/i = value
            // This handles: for(var/i = 0, ...) traditional for loop with var declaration
            if (Current().Type == TokenType::Assign) {
                Location assignLoc = CurrentLocation();
                Advance();
                auto initValue = Expression();
                if (initValue) {
                    // Convert path to identifier for the LValue
                    // Extract the variable name from the path
                    std::unique_ptr<DMASTExpression> lvalueExpr;
                    if (auto* pathConst = dynamic_cast<DMASTConstantPath*>(pathExpr.get())) {
                        std::string varName = pathConst->Path.Path.GetLastElement();
                        if (!varName.empty()) {
                            // Create an identifier with 'var:' prefix to signal it's a new variable
                            lvalueExpr = std::make_unique<DMASTIdentifier>(pathConst->Location_, "var:" + varName);
                        }
                    }
                    if (!lvalueExpr) {
                        lvalueExpr = std::move(pathExpr);
                    }
                    
                    // Create an assignment expression: var/i = value
                    firstExpr = std::make_unique<DMASTAssign>(
                        assignLoc, std::move(lvalueExpr), AssignmentOperator::Assign, std::move(initValue));
                }
            } else {
                firstExpr = std::move(pathExpr);
            }
        } else {
            // Parse as normal expression (which handles assignments too)
            firstExpr = Expression();
        }
    }
    
    // Check for optional 'as' keyword (type filter) before 'in'
    // This handles: for(var/mob/M as /mob|mob in world)
    std::string typeFilterStr;
    if (Current().Type == TokenType::As) {
        Advance(); // Consume 'as'
        
        // Parse type filter expression - read tokens until we hit 'in'
        // Type filter can be like /mob or /mob|mob
        while (Current().Type != TokenType::In && 
               Current().Type != TokenType::RightParenthesis && 
               Current().Type != TokenType::EndOfFile) {
            typeFilterStr += Current().Text;
            Advance();
        }
    }
    
    // Check if this is a for-in loop
    bool isForIn = (Current().Type == TokenType::In);
    bool implicitInWorld = false;

    // Check for implicit "in world" loop: for(var/type/name)
    if (!isForIn && seenVar && Current().Type == TokenType::RightParenthesis) {
        if (dynamic_cast<DMASTConstantPath*>(firstExpr.get())) {
            isForIn = true;
            implicitInWorld = true;
        }
    }

    if (isForIn) {
        std::unique_ptr<DMASTExpression> listExpr;
        
        if (implicitInWorld) {
            listExpr = std::make_unique<DMASTIdentifier>(loc, "world");
        } else {
            Advance(); // Consume 'in'
            listExpr = Expression();
        }
        
        if (!listExpr) {
            Emit(WarningCode::BadExpression, "Expected list expression after 'in' in for-in loop");
            return nullptr;
        }
        
        // Extract variable declaration information from firstExpr
        DMASTProcStatementForIn::VariableDeclaration varDecl;
        
        // Check if firstExpr is a path expression (var/mob/M or /mob/M)
        if (auto* pathExpr = dynamic_cast<DMASTConstantPath*>(firstExpr.get())) {
            const DMASTPath& path = pathExpr->Path;
            const std::vector<std::string>& elements = path.Path.GetElements();
            
            if (!elements.empty()) {
                // Last element is the variable name
                varDecl.Name = elements.back();
                varDecl.Loc = pathExpr->Location_;
                
                // If there are more elements, they form the type path
                if (elements.size() > 1) {
                    // Build type path from all but the last element
                    std::vector<std::string> typeElements(elements.begin(), elements.end() - 1);
                    DreamPath typePath(path.Path.GetPathType(), typeElements);
                    varDecl.TypePath = typePath;
                }
            }
        }
        // Check if firstExpr is just an identifier (simple variable)
        else if (auto* identExpr = dynamic_cast<DMASTIdentifier*>(firstExpr.get())) {
            varDecl.Name = identExpr->Identifier;
            varDecl.Loc = identExpr->Location_;
        }
        
        // Use the type filter that was parsed earlier (before 'in')
        if (!typeFilterStr.empty()) {
            varDecl.TypeFilter = typeFilterStr;
        }
        
        Consume(TokenType::RightParenthesis, "Expected ')' after for-in list");
        
        // Consume only whitespace before body - let ProcBlockInner handle newlines
        Whitespace();
        
        // Body - pass the for-in statement's column as base indent
        auto body = ProcBlockInner(loc.Column);
        
        // Create a for-in statement with enhanced variable information
        return std::make_unique<DMASTProcStatementForIn>(loc, std::move(firstExpr), 
                                                         varDecl, std::move(listExpr), std::move(body));
    }
    
    // Not a for-in loop, continue parsing as traditional for loop
    std::unique_ptr<DMASTProcStatement> initializer;
    std::unique_ptr<DMASTExpression> condition;
    std::unique_ptr<DMASTExpression> increment;
    
    if (firstExpr) {
        initializer = std::make_unique<DMASTProcStatementExpression>(loc, std::move(firstExpr));
    }
    
    // DM allows both semicolons (;) and commas (,) as separators in for loops
    // Also allow empty for() - shorthand for for(;;) infinite loop
    if (Current().Type == TokenType::RightParenthesis) {
        // Empty for() - shorthand for infinite loop
        // Skip separator handling, go straight to parsing body
        Advance(); // Consume )
    } else {
        // Normal for loop with initializer, condition, increment
        if (Current().Type != TokenType::Semicolon && Current().Type != TokenType::Comma) {
            Emit(WarningCode::BadToken, "Expected ';' or ',' after for initializer");
        } else {
            Advance();
        }
        
        // Condition (optional)
        if (Current().Type != TokenType::Semicolon && 
            Current().Type != TokenType::Comma &&
            Current().Type != TokenType::RightParenthesis) {
            condition = Expression();
        }
        
        // DM allows both semicolons (;) and commas (,) as separators in for loops
        if (Current().Type != TokenType::Semicolon && Current().Type != TokenType::Comma) {
            // If we already hit ), that's OK - increment is optional
            if (Current().Type != TokenType::RightParenthesis) {
                Emit(WarningCode::BadToken, "Expected ';' or ',' after for condition");
            }
        } else {
            Advance();
        }
        
        // Increment (optional)
        if (Current().Type != TokenType::RightParenthesis) {
            increment = Expression();
        }
        Consume(TokenType::RightParenthesis, "Expected ')' after for clauses");
    }
    
    // Consume only whitespace before body - let ProcBlockInner handle newlines
    Whitespace();
    
    // Body - pass the for statement's column as base indent
    auto body = ProcBlockInner(loc.Column);
    
    return std::make_unique<DMASTProcStatementFor>(loc, std::move(initializer), 
                                                    std::move(condition), std::move(increment), 
                                                    std::move(body));
}

std::unique_ptr<DMASTProcStatement> DMParser::ProcStatementBreak() {
    Location loc = CurrentLocation();
    Consume(TokenType::Break, "Expected 'break'");
    
    // Label is optional (must be identifier if present)
    std::unique_ptr<DMASTIdentifier> label;
    if (IsInSet(Current().Type, IdentifierTypes_) &&
        Current().Type != TokenType::Semicolon && 
        Current().Type != TokenType::Newline && 
        Current().Type != TokenType::EndOfFile) {
        label = std::make_unique<DMASTIdentifier>(CurrentLocation(), Current().Text);
        Advance();
    }
    
    return std::make_unique<DMASTProcStatementBreak>(loc, std::move(label));
}

std::unique_ptr<DMASTProcStatement> DMParser::ProcStatementContinue() {
    Location loc = CurrentLocation();
    Consume(TokenType::Continue, "Expected 'continue'");
    
    // Label is optional (must be identifier if present)
    std::unique_ptr<DMASTIdentifier> label;
    if (IsInSet(Current().Type, IdentifierTypes_) &&
        Current().Type != TokenType::Semicolon && 
        Current().Type != TokenType::Newline && 
        Current().Type != TokenType::EndOfFile) {
        label = std::make_unique<DMASTIdentifier>(CurrentLocation(), Current().Text);
        Advance();
    }
    
    return std::make_unique<DMASTProcStatementContinue>(loc, std::move(label));
}

std::unique_ptr<DMASTProcStatement> DMParser::ProcStatementDoWhile() {
    Location loc = CurrentLocation();
    Consume(TokenType::Do, "Expected 'do'");
    
    // Body - pass the do statement's column as base indent
    auto body = ProcBlockInner(loc.Column);
    
    // While keyword
    Consume(TokenType::While, "Expected 'while' after do-while body");
    
    // Condition
    Consume(TokenType::LeftParenthesis, "Expected '(' after 'while'");
    auto condition = Expression();
    if (!condition) {
        Emit(WarningCode::BadToken, "Expected condition in do-while statement");
        return nullptr;
    }
    Consume(TokenType::RightParenthesis, "Expected ')' after condition");
    
    return std::make_unique<DMASTProcStatementDoWhile>(loc, std::move(body), std::move(condition));
}

std::unique_ptr<DMASTProcStatement> DMParser::ProcStatementSwitch() {
    Location loc = CurrentLocation();
    Consume(TokenType::Switch, "Expected 'switch'");
    
    // Value to switch on
    Consume(TokenType::LeftParenthesis, "Expected '(' after 'switch'");
    auto value = Expression();
    if (!value) {
        Emit(WarningCode::BadToken, "Expected expression in switch statement");
        return nullptr;
    }
    Consume(TokenType::RightParenthesis, "Expected ')' after switch value");
    
    // Switch body with cases
    Consume(TokenType::LeftCurlyBracket, "Expected '{' to start switch body");
    
    std::vector<DMASTProcStatementSwitch::SwitchCase> cases;
    
    while (Current().Type != TokenType::RightCurlyBracket && Current().Type != TokenType::EndOfFile) {
        Whitespace();
        
        if (Current().Type == TokenType::If) {
            // Case with values: if (val1, val2, ...)
            Location caseLoc = CurrentLocation();
            Advance();
            Consume(TokenType::LeftParenthesis, "Expected '(' after 'if' in switch case");
            
            std::vector<std::unique_ptr<DMASTExpression>> values;
            do {
                Whitespace();
                
                auto caseValue = Expression();
                if (caseValue) {
                    Whitespace();
                    
                    // Check for "to" keyword to create a range expression
                    // Note: Expression() should not consume "to" since it's not implemented as a binary operator
                    if (Current().Type == TokenType::To) {
                        Location rangeLoc = CurrentLocation();
                        Advance();  // Consume "to"
                        Whitespace();
                        
                        auto rangeEnd = Expression();
                        if (!rangeEnd) {
                            Warning("Expected upper bound for range in switch case");
                            rangeEnd = std::make_unique<DMASTConstantNull>(rangeLoc);
                        }
                        
                        // Create a DMASTSwitchCaseRange node
                        auto rangeExpr = std::make_unique<DMASTSwitchCaseRange>(
                            rangeLoc,
                            std::move(caseValue),
                            std::move(rangeEnd)
                        );
                        values.push_back(std::move(rangeExpr));
                    } else {
                        values.push_back(std::move(caseValue));
                    }
                }
                
                Whitespace();
                
                if (Current().Type == TokenType::Comma) {
                    Advance();
                } else {
                    break;
                }
            } while (true);
            
            Consume(TokenType::RightParenthesis, "Expected ')' after case values");
            
            // Case body - use case's column as base indent
            auto caseBody = ProcBlockInner(caseLoc.Column);
            
            DMASTProcStatementSwitch::SwitchCase switchCase;
            switchCase.Values = std::move(values);
            switchCase.Body = std::move(caseBody);
            cases.push_back(std::move(switchCase));
        } else if (Current().Type == TokenType::Else) {
            // Default case
            Location elseLoc = CurrentLocation();
            Advance();
            // Default body - use else's column as base indent
            auto defaultBody = ProcBlockInner(elseLoc.Column);
            
            DMASTProcStatementSwitch::SwitchCase defaultCase;
            // Empty values vector means default case
            defaultCase.Body = std::move(defaultBody);
            cases.push_back(std::move(defaultCase));
        } else {
            break;
        }
        
        Whitespace();
    }
    
    Consume(TokenType::RightCurlyBracket, "Expected '}' to end switch body");
    
    return std::make_unique<DMASTProcStatementSwitch>(loc, std::move(value), std::move(cases));
}

std::unique_ptr<DMASTProcStatement> DMParser::ProcStatementDel() {
    Location loc = CurrentLocation();
    Consume(TokenType::Del, "Expected 'del'");
    
    // del takes an expression
    Consume(TokenType::LeftParenthesis, "Expected '(' after 'del'");
    auto value = Expression();
    if (!value) {
        Emit(WarningCode::BadToken, "Expected expression in del statement");
        return nullptr;
    }
    Consume(TokenType::RightParenthesis, "Expected ')' after del value");
    
    return std::make_unique<DMASTProcStatementDel>(loc, std::move(value));
}

std::unique_ptr<DMASTProcStatement> DMParser::ProcStatementSpawn() {
    Location loc = CurrentLocation();
    Consume(TokenType::Spawn, "Expected 'spawn'");
    
    // Delay is optional, can be spawn() or spawn(delay)
    std::unique_ptr<DMASTExpression> delay;
    if (Current().Type == TokenType::LeftParenthesis) {
        Advance();
        if (Current().Type != TokenType::RightParenthesis) {
            delay = Expression();
        }
        Consume(TokenType::RightParenthesis, "Expected ')' after spawn delay");
    }
    
    // Body - pass the spawn statement's column as base indent
    auto body = ProcBlockInner(loc.Column);
    
    return std::make_unique<DMASTProcStatementSpawn>(loc, std::move(delay), std::move(body));
}

std::unique_ptr<DMASTProcStatement> DMParser::ProcStatementTryCatch() {
    Location loc = CurrentLocation();
    Consume(TokenType::Try, "Expected 'try'");
    
    // Try body - pass the try statement's column as base indent
    auto tryBody = ProcBlockInner(loc.Column);
    
    // Catch clause (optional but usually present)
    std::unique_ptr<DMASTIdentifier> catchVariable;
    std::unique_ptr<DMASTProcBlockInner> catchBody;
    
    if (Current().Type == TokenType::Catch) {
        Advance();
        
        // Optional catch variable: catch(e)
        if (Current().Type == TokenType::LeftParenthesis) {
            Advance();
            if (IsInSet(Current().Type, IdentifierTypes_)) {
                catchVariable = std::make_unique<DMASTIdentifier>(CurrentLocation(), Current().Text);
                Advance();
            }
            Consume(TokenType::RightParenthesis, "Expected ')' after catch variable");
        }
        
        // Catch body - use try statement's column as base (catch is at same level as try)
        catchBody = ProcBlockInner(loc.Column);
    }
    
    return std::make_unique<DMASTProcStatementTryCatch>(loc, std::move(tryBody), 
                                                        std::move(catchVariable), std::move(catchBody));
}

std::unique_ptr<DMASTProcStatement> DMParser::ProcStatementThrow() {
    Location loc = CurrentLocation();
    Consume(TokenType::Throw, "Expected 'throw'");
    
    // Throw takes an expression
    auto value = Expression();
    if (!value) {
        Emit(WarningCode::BadToken, "Expected expression in throw statement");
        return nullptr;
    }
    
    return std::make_unique<DMASTProcStatementThrow>(loc, std::move(value));
}

std::unique_ptr<DMASTProcStatement> DMParser::ProcStatementSet() {
    Location loc = CurrentLocation();
    Consume(TokenType::Set, "Expected 'set'");
    
    // Attribute name (identifier)
    if (!IsInSet(Current().Type, IdentifierTypes_)) {
        Emit(WarningCode::BadToken, "Expected attribute name in set statement");
        return nullptr;
    }
    std::string attribute = Current().Text;
    Advance();
    
    // = or 'in' keyword
    if (Current().Type != TokenType::Assign && Current().Type != TokenType::In) {
        Emit(WarningCode::BadToken, "Expected '=' or 'in' in set statement");
        return nullptr;
    }
    Advance();
    
    // Value expression
    auto value = Expression();
    if (!value) {
        Emit(WarningCode::BadToken, "Expected value in set statement");
        return nullptr;
    }
    
    return std::make_unique<DMASTProcStatementSet>(loc, attribute, std::move(value));
}

std::unique_ptr<DMASTProcStatement> DMParser::ProcStatementGoto() {
    Location loc = CurrentLocation();
    Consume(TokenType::Goto, "Expected 'goto'");
    
    // Goto requires a label identifier
    if (!IsInSet(Current().Type, IdentifierTypes_)) {
        Emit(WarningCode::BadToken, "Expected label identifier in goto statement");
        return nullptr;
    }
    
    auto label = std::make_unique<DMASTIdentifier>(CurrentLocation(), Current().Text);
    Advance();
    
    return std::make_unique<DMASTProcStatementGoto>(loc, std::move(label));
}

std::unique_ptr<DMASTProcStatement> DMParser::ProcStatementLabel() {
    Location loc = CurrentLocation();
    
    // Parse label name
    if (!IsInSet(Current().Type, IdentifierTypes_)) {
        Emit(WarningCode::BadToken, "Expected label identifier");
        return nullptr;
    }
    std::string labelName = Current().Text;
    Advance();
    
    // Consume colon
    Consume(TokenType::Colon, "Expected ':' after label name");
    
    // Optional statement after label (can be on same line or next line)
    Whitespace();
    std::unique_ptr<DMASTProcStatement> body;
    if (Current().Type != TokenType::Semicolon && 
        Current().Type != TokenType::Newline && 
        Current().Type != TokenType::EndOfFile &&
        Current().Type != TokenType::RightCurlyBracket) {
        body = ProcStatement();
    }
    
    return std::make_unique<DMASTProcStatementLabel>(loc, labelName, std::move(body));
}

// ============================================================================
// Top-Level and Object Statement Parsing
// ============================================================================

std::unique_ptr<DMASTStatement> DMParser::Statement() {
    Location loc = CurrentLocation();
    Whitespace();
    
    // Check for proc/verb keyword FIRST (before paths)
    if (Current().Type == TokenType::Proc) {
        Token saved = Current();
        Advance();
        Whitespace();
        if (Current().Type == TokenType::Newline || Current().Type == TokenType::LeftCurlyBracket) {
            ReuseToken(saved);
        } else {
            ReuseToken(saved);
            return ObjectProcDefinition(false);
        }
    }
    if (Current().Type == TokenType::Verb) {
        Token saved = Current();
        Advance();
        Whitespace();
        if (Current().Type == TokenType::Newline || Current().Type == TokenType::LeftCurlyBracket) {
            ReuseToken(saved);
        } else {
            ReuseToken(saved);
            return ObjectProcDefinition(true);
        }
    }
    
    // Try to parse a path (could be /obj, /obj/item, New, test, var, etc.)
    // Note: "var" is now handled as a path element, not a special keyword here
    if (Current().Type == TokenType::Divide || 
        IsInSet(Current().Type, IdentifierTypes_) ||
        Current().Type == TokenType::Var ||
        Current().Type == TokenType::Proc ||
        Current().Type == TokenType::Verb) {
        // Save position in case we need to backtrack
        auto path = ParsePath();
        
        
        Whitespace();
        
        // Check if it's a proc definition (path followed by parentheses)
        if (Current().Type == TokenType::LeftParenthesis) {
            // It's a proc! Parse it as a proc definition
            // Need to strip "proc" or "verb" from the path if present
            // Example: /proc/step -> ObjectPath=/, Name=step
            // Example: /mob/proc/test -> ObjectPath=/mob, Name=test
            // Example: /mob/test -> ObjectPath=/mob, Name=test
            
            auto elements = path.Path.GetElements();
            std::string procName;
            std::vector<std::string> objectPathElements;
            bool isVerb = false;
            
            // Find and remove "proc" or "verb" from elements
            bool foundProcVerb = false;
            for (size_t i = 0; i < elements.size(); ++i) {
                if (elements[i] == "proc") {
                    foundProcVerb = true;
                    // Skip this element, everything before goes to object path
                    continue;
                } else if (elements[i] == "verb") {
                    foundProcVerb = true;
                    isVerb = true;
                    // Skip this element, everything before goes to object path
                    continue;
                } else if (foundProcVerb) {
                    // After proc/verb, this is the proc name (should be last element)
                    procName = elements[i];
                } else {
                    // Before proc/verb, add to object path
                    objectPathElements.push_back(elements[i]);
                }
            }
            
            // If no proc/verb found, last element is proc name, rest is object path
            if (!foundProcVerb) {
                if (!elements.empty()) {
                    procName = elements.back();
                    objectPathElements.assign(elements.begin(), elements.end() - 1);
                }
            }
            
            // Parse parameter list
            std::vector<std::unique_ptr<DMASTDefinitionParameter>> parameters;
            Advance(); // consume '('
            
            while (Current().Type != TokenType::RightParenthesis && Current().Type != TokenType::EndOfFile) {
                auto param = ProcParameter();
                if (param) {
                    parameters.push_back(std::move(param));
                }
                
                if (Current().Type == TokenType::Comma) {
                    Advance();
                } else {
                    break;
                }
            }
            
            Consume(TokenType::RightParenthesis, "Expected ')' after parameter list");
            Whitespace();
            
            // Parse return type annotation (as type or as /path)
            if (Current().Type == TokenType::As) {
                Advance(); // Consume 'as'
                Whitespace();
                
                // Parse return type - can be: null, text, num, /path, or combinations with |
                do {
                    // Check for path (/type)
                    if (Current().Type == TokenType::Divide) {
                        ParsePath(); // Consume the path
                    }
                    // Check for built-in type (null, text, num, etc.)
                    else if (IsInSet(Current().Type, IdentifierTypes_)) {
                        Advance(); // Consume the type name
                    }
                    
                    Whitespace();
                    
                    // Check for | separator (multiple return types)
                    if (Current().Type == TokenType::BitwiseOr) {
                        Advance();
                        Whitespace();
                    } else {
                        break;
                    }
                } while (Current().Type != TokenType::Newline && Current().Type != TokenType::EndOfFile);
            }
            
            Whitespace();
            
            // Ensure we consume to end of line if not at newline already
            while (Current().Type != TokenType::Newline && 
                   Current().Type != TokenType::LeftCurlyBracket && 
                   Current().Type != TokenType::EndOfFile) {
                Advance();
            }
            
            // Parse body
            auto body = ProcBlockInner(loc.Column);
            
            // Create DreamPath for the object
            // The objectPath should be relative to CurrentPath_ (the current parsing context)
            // If objectPathElements is empty, use empty relative path (will be combined with CurrentPath_ in compiler)
            DreamPath objectPath;
            if (objectPathElements.empty()) {
                // No object path specified in the proc definition
                // Use empty relative path so it will be combined with CurrentPath_ during compilation
                objectPath = DreamPath(DreamPath::PathType::Relative, objectPathElements);
            } else {
                // Object path specified - use the path type from parsing
                objectPath = DreamPath(path.Path.GetPathType(), objectPathElements);
            }
            
            
            return std::make_unique<DMASTObjectProcDefinition>(loc, objectPath, procName, std::move(parameters),
                                                               std::move(body), isVerb);
        }
        // Check if it's a variable override (simple assignment)
        else if (Current().Type == TokenType::Assign && path.Path.GetElements().size() == 1) {
            // Variable override: name = value
            std::string varName = path.Path.GetLastElement();
            Advance(); // consume '='
            auto value = Expression();
            return std::make_unique<DMASTObjectVarOverride>(loc, varName, std::move(value));
        }
        // Otherwise, it's an object definition
        else {
            // Save old path and update CurrentPath_ for parsing nested content
            DreamPath oldPath = CurrentPath_;
            DreamPath newPath = CurrentPath_.Combine(path.Path);
            CurrentPath_ = newPath;
            
            // Check for body
            std::vector<std::unique_ptr<DMASTObjectStatement>> innerStatements;
            
            if (Current().Type == TokenType::LeftCurlyBracket) {
                // Curly brace style: /obj/item { ... }
                Advance();
                Whitespace();
                
                while (Current().Type != TokenType::RightCurlyBracket && Current().Type != TokenType::EndOfFile) {
                    auto stmt = ObjectStatement();
                    if (stmt) {
                        innerStatements.push_back(std::move(stmt));
                    } else {
                        // Skip to next line
                        while (Current().Type != TokenType::Newline &&
                               Current().Type != TokenType::RightCurlyBracket &&
                               Current().Type != TokenType::EndOfFile) {
                            Advance();
                        }
                        if (Current().Type == TokenType::Newline) {
                            Advance();
                        }
                    }
                    
                    // Skip statement terminators
                    while (Current().Type == TokenType::Semicolon || Current().Type == TokenType::Newline) {
                        Advance();
                    }
                    Whitespace();
                }
                
                Consume(TokenType::RightCurlyBracket, "Expected '}'");
            } else if (Current().Type == TokenType::Newline) {
                // Indentation style: use column-based indentation tracking
                Advance(); // Skip the newline
                
                // Get base indentation level (the object definition line's column)
                int baseIndent = loc.Column;
                
                // Parse indented block using column-based indentation
                innerStatements = ParseIndentedObjectBlock(baseIndent);
            }
            
            // Restore old path
            CurrentPath_ = oldPath;
            
            return std::make_unique<DMASTObjectDefinition>(loc, path, std::move(innerStatements));
        }
    }
    
    return nullptr;
}

std::unique_ptr<DMASTObjectStatement> DMParser::ObjectStatement() {
    Location loc = CurrentLocation();
    Whitespace();
    
    // Check for explicit proc/verb (but only if followed by /)
    // This handles "proc/name()" and "verb/name()" syntax
    if (Current().Type == TokenType::Proc) {
        // Look ahead to see if it's "proc/" or just "proc" (which could be an object path)
        Token savedToken = Current();
        Advance();
        Whitespace();
        if (Current().Type == TokenType::Divide) {
            // It's "proc/", so parse as proc definition
            ReuseToken(savedToken);
            return ObjectProcDefinition(false);
        }
        // It's just "proc", treat as object path
        ReuseToken(savedToken);
    }
    if (Current().Type == TokenType::Verb) {
        // Look ahead to see if it's "verb/" or just "verb" (which could be an object path)
        Token savedToken = Current();
        Advance();
        Whitespace();
        if (Current().Type == TokenType::Divide) {
            // It's "verb/", so parse as verb definition
            ReuseToken(savedToken);
            return ObjectProcDefinition(true);
        }
        // It's just "verb", treat as object path
        ReuseToken(savedToken);
    }
    
    // For identifiers and keywords (including var), we need to look ahead to determine the type
    // Could be:
    //   identifier()     -> proc definition (implicit proc)
    //   identifier = val -> var override OR var definition (if in var block)
    //   identifier { ... -> object definition
    //   identifier\n...  -> object definition
    if (IsInSet(Current().Type, IdentifierTypes_) || 
        Current().Type == TokenType::Var ||
        Current().Type == TokenType::Global) {
        // Save current token to backtrack if needed
        Token savedToken = Current();
        
        // Try to parse a path
        auto path = ParsePath();
        
        // Now check what follows
        Whitespace();
        
        // Check for ( -> it's a proc definition
        if (Current().Type == TokenType::LeftParenthesis) {
            // Backtrack and parse as proc definition
            ReuseToken(savedToken);
            return ObjectProcDefinition(false);
        }
        
        // Check if we're in a var block context (CurrentPath_ contains "var")
        bool inVarBlock = false;
        for (const auto& elem : CurrentPath_.GetElements()) {
            if (elem == "var") {
                inVarBlock = true;
                break;
            }
        }
        
        // Also check if the path itself starts with 'var'
        bool pathStartsVar = !path.Path.GetElements().empty() && path.Path.GetElements()[0] == "var";
        if (pathStartsVar) {
            inVarBlock = true;
        }
        
        // Check for = -> could be var override or var definition
        if (Current().Type == TokenType::Assign) {
            // If in var block, treat as variable definition
            if (inVarBlock) {
                // Parse as variable definition
                // For "Beam/myBeam = value", path is "Beam/myBeam"
                // Variable name is the last element: "myBeam"
                // Variable type is everything before the last element: "Beam"
                auto pathElements = path.Path.GetElements();
                std::string varName = pathElements.empty() ? "" : pathElements.back();
                
                // Extract type path (all elements except the last one)
                std::vector<std::string> typeElements;
                if (pathElements.size() > 1) {
                    typeElements.assign(pathElements.begin(), pathElements.end() - 1);
                }
                
                // If the path started with 'var', remove it from the type elements
                // e.g. var/mob/M -> type is /mob, name is M
                // var/x -> type is empty (implicit), name is x
                if (pathStartsVar && !typeElements.empty() && typeElements[0] == "var") {
                    typeElements.erase(typeElements.begin());
                }
                
                // Create type path (preserve absolute/relative from original path)
                DreamPath typePath(path.Path.GetPathType(), typeElements);
                DMASTPath typeASTPath(loc, typePath, false);
                
                Advance(); // consume =
                Whitespace();
                auto value = Expression();
                
                return std::make_unique<DMASTObjectVarDefinition>(loc, varName, typeASTPath, std::move(value), std::nullopt);
            } else {
                // Parse as variable override
                std::string varName = path.Path.GetElements().empty() ? "" : path.Path.GetLastElement();
                Advance(); // consume =
                Whitespace();
                auto value = Expression();
                return std::make_unique<DMASTObjectVarOverride>(loc, varName, std::move(value));
            }
        }
        
        // Check for [ -> array syntax (e.g., techs[0])
        // In a var block, this is a variable definition with array initialization
        if (Current().Type == TokenType::LeftBracket && inVarBlock) {
            // Parse array size
            Advance(); // consume [
            Whitespace();
            auto arraySize = Expression();
            Whitespace();
            Consume(TokenType::RightBracket, "Expected ']' after array size");
            
            // For "techs[0]", path is "techs"
            // Variable name is "techs"
            // Variable type is empty (will be treated as /list)
            auto pathElements = path.Path.GetElements();
            std::string varName = pathElements.empty() ? "" : pathElements.back();
            
            // Extract type path (all elements except the last one)
            std::vector<std::string> typeElements;
            if (pathElements.size() > 1) {
                typeElements.assign(pathElements.begin(), pathElements.end() - 1);
            }
            
            // Create type path (preserve absolute/relative from original path)
            DreamPath typePath(path.Path.GetPathType(), typeElements);
            DMASTPath typeASTPath(loc, typePath, false);
            
            // For now, treat array syntax as uninitialized (null value)
            // The array size is just a hint, not an initialization value
            // TODO: In the future, we could create a proper list initialization with the size
            
            return std::make_unique<DMASTObjectVarDefinition>(loc, varName, typeASTPath, nullptr, std::nullopt);
        }
        
        // Check for newline/semicolon -> variable definition without initialization (in var block)
        if ((Current().Type == TokenType::Newline || 
             Current().Type == TokenType::Semicolon ||
             Current().Type == TokenType::EndOfFile) && inVarBlock) {
            // Variable definition without initialization
            // For "Beam/myBeam", path is "Beam/myBeam"
            // Variable name is the last element: "myBeam"
            // Variable type is everything before the last element: "Beam"
            auto pathElements = path.Path.GetElements();
            std::string varName = pathElements.empty() ? "" : pathElements.back();
            
            // Extract type path (all elements except the last one)
            std::vector<std::string> typeElements;
            if (pathElements.size() > 1) {
                typeElements.assign(pathElements.begin(), pathElements.end() - 1);
            }
            
            // Create type path (preserve absolute/relative from original path)
            DreamPath typePath(path.Path.GetPathType(), typeElements);
            DMASTPath typeASTPath(loc, typePath, false);
            
            return std::make_unique<DMASTObjectVarDefinition>(loc, varName, typeASTPath, nullptr, std::nullopt);
        }
        
        // Otherwise, backtrack and parse as object definition
        ReuseToken(savedToken);
    }
    
    // Try to parse as object definition
    // ObjectDefinition will handle the rest
    if (Current().Type == TokenType::Divide || 
        IsInSet(Current().Type, IdentifierTypes_) ||
        Current().Type == TokenType::Var ||
        Current().Type == TokenType::Global) {
        auto def = ObjectDefinition();
        return std::unique_ptr<DMASTObjectStatement>(static_cast<DMASTObjectStatement*>(def.release()));
    }
    
    return nullptr;
}

DMASTPath DMParser::ParsePath() {
    Location loc = CurrentLocation();
    std::vector<std::string> elements;
    bool isAbsolute = false;
    
    // Skip leading whitespace
    Whitespace();
    
    
    // Check for absolute path
    if (Current().Type == TokenType::Divide) {
        Advance();
        isAbsolute = true;
        // Skip whitespace after /
        Whitespace();
    }
    
    // Parse path elements (identifiers and certain keywords like 'global', 'var', 'proc', 'verb')
    while (IsInSet(Current().Type, IdentifierTypes_) || 
           Current().Type == TokenType::Global ||
           Current().Type == TokenType::Var ||
           Current().Type == TokenType::Proc ||
           Current().Type == TokenType::Verb) {
        elements.push_back(Current().Text);
        Advance();
        
        // Skip whitespace after identifier
        Whitespace();
        
        if (Current().Type == TokenType::Divide) {
            Advance();
            // Skip whitespace after /
            Whitespace();
        } else {
            break;
        }
    }
    
    DreamPath::PathType pathType = isAbsolute ? DreamPath::PathType::Absolute : DreamPath::PathType::Relative;
    DreamPath dreamPath(pathType, elements);
    return DMASTPath(loc, dreamPath, false);
}

std::unique_ptr<DMASTObjectStatement> DMParser::ObjectProcDefinition(bool isVerb) {
    Location loc = CurrentLocation();
    
    // Check if we have explicit proc/verb keyword
    // DM allows implicit proc definition: just "Name()" at type level is equivalent to "proc/Name()"
    bool hasExplicitKeyword = (Current().Type == TokenType::Proc || Current().Type == TokenType::Verb);
    
    // Consume proc/verb keyword if present
    if (hasExplicitKeyword) {
        if (isVerb) {
            Consume(TokenType::Verb, "Expected 'verb'");
        } else {
            Consume(TokenType::Proc, "Expected 'proc'");
        }
    }
    
    // Check if there's a / after proc/verb keyword
    // If so, this is relative syntax like "proc/test", not absolute like "/proc/test"
    // We need to consume the / but treat the path as relative
    bool hasSlashAfterKeyword = false;
    if (hasExplicitKeyword && Current().Type == TokenType::Divide) {
        hasSlashAfterKeyword = true;
        Advance(); // Consume the /
    }
    
    // Skip any whitespace before parsing the proc name
    Whitespace();
    
    
    // Parse proc path/name and extract object path
    auto path = ParsePath();
    
    // If we consumed a / after the keyword, treat the path as relative
    if (hasSlashAfterKeyword && path.Path.GetPathType() == DreamPath::PathType::Absolute) {
        // Convert absolute path to relative
        path = DMASTPath(path.Location_, DreamPath(DreamPath::PathType::Relative, path.Path.GetElements()), path.IsOperator);
    }
    auto elements = path.Path.GetElements();
    std::string procName;
    std::vector<std::string> objectPathElements;
    
    // Find and remove "proc" or "verb" from elements
    bool foundProcVerb = false;
    for (size_t i = 0; i < elements.size(); ++i) {
        if (elements[i] == "proc" || elements[i] == "verb") {
            foundProcVerb = true;
            continue; // Skip this element
        } else if (foundProcVerb) {
            procName = elements[i]; // After proc/verb, this is the proc name
        } else {
            objectPathElements.push_back(elements[i]); // Before proc/verb, add to object path
        }
    }
    
    // If no proc/verb found, last element is proc name, rest is object path
    if (!foundProcVerb && !elements.empty()) {
        procName = elements.back();
        objectPathElements.assign(elements.begin(), elements.end() - 1);
    }
    
    // Parse parameter list
    std::vector<std::unique_ptr<DMASTDefinitionParameter>> parameters;
    if (Current().Type == TokenType::LeftParenthesis) {
        Advance();
        
        while (Current().Type != TokenType::RightParenthesis && Current().Type != TokenType::EndOfFile) {
            auto param = ProcParameter();
            if (param) {
                parameters.push_back(std::move(param));
            }
            
            if (Current().Type == TokenType::Comma) {
                Advance();
            } else {
                break;
            }
        }
        
        Consume(TokenType::RightParenthesis, "Expected ')' after parameter list");
    }
    
    Whitespace();
    
    // Parse return type annotation (as type or as /path)
    if (Current().Type == TokenType::As) {
        Advance(); // Consume 'as'
        Whitespace();
        
        // Parse return type - can be: null, text, num, /path, or combinations with |
        // For now, we'll consume it but not store it (C++ AST doesn't have ReturnType field yet)
        // This prevents "as text" from being parsed as a separate object definition
        do {
            // Check for path (/type)
            if (Current().Type == TokenType::Divide) {
                ParsePath(); // Consume the path
            }
            // Check for built-in type (null, text, num, etc.)
            else if (IsInSet(Current().Type, IdentifierTypes_)) {
                Advance(); // Consume the type name
            }
            
            Whitespace();
            
            // Check for | separator (multiple return types)
            if (Current().Type == TokenType::BitwiseOr) {
                Advance();
                Whitespace();
            } else {
                break;
            }
        } while (Current().Type != TokenType::Newline && Current().Type != TokenType::EndOfFile);
    }
    
    Whitespace();
    
    // Ensure we consume to end of line if not at newline already
    while (Current().Type != TokenType::Newline && 
           Current().Type != TokenType::LeftCurlyBracket && 
           Current().Type != TokenType::EndOfFile) {
        Advance();
    }
    
    // Parse body - pass the base indentation (column where 'proc' keyword started)
    // so indented blocks can determine when they end
    auto body = ProcBlockInner(loc.Column);
    
    // Create DreamPath for the object
    // The objectPath should be relative to CurrentPath_ (the current parsing context)
    // If objectPathElements is empty, use empty relative path (will be combined with CurrentPath_ in compiler)
    DreamPath objectPath;
    if (objectPathElements.empty()) {
        // No object path specified in the proc definition
        // Use empty relative path so it will be combined with CurrentPath_ during compilation
        objectPath = DreamPath(DreamPath::PathType::Relative, objectPathElements);
    } else {
        // Object path specified - use the path type from parsing
        objectPath = DreamPath(path.Path.GetPathType(), objectPathElements);
    }
    
    return std::make_unique<DMASTObjectProcDefinition>(loc, objectPath, procName, std::move(parameters),
                                                       std::move(body), isVerb);
}

std::unique_ptr<DMASTDefinitionParameter> DMParser::ProcParameter() {
    Location loc = CurrentLocation();
    
    // Parameters can be: name, var/type/name, var/name as type, type/name
    std::string paramName;
    DreamPath typePath;
    bool isList = false;
    std::unique_ptr<DMASTExpression> defaultValue;
    
    // Check for var keyword (optional)
    if (Current().Type == TokenType::Var) {
        Advance();
        
        // Parse type path if present
        if (Current().Type == TokenType::Divide) {
            auto path = ParsePath();
            typePath = path.Path;
            paramName = typePath.GetLastElement();
        } else if (IsInSet(Current().Type, IdentifierTypes_)) {
            paramName = Current().Text;
            Advance();
        }
    } else if (IsInSet(Current().Type, IdentifierTypes_)) {
        // Check if this is a typed parameter (type/name syntax)
        // Look ahead to see if there's a / after the identifier
        Token firstToken = Current();
        Advance();
        
        if (Current().Type == TokenType::Divide) {
            // It's a typed parameter like mob/M
            // Parse as a path to get all elements
            ReuseToken(firstToken);
            auto path = ParsePath();
            
            // Extract type and name from the path
            // For "mob/M", elements = ["mob", "M"]
            // Type = all but last, Name = last
            auto elements = path.Path.GetElements();
            if (elements.size() >= 2) {
                // Type is all elements except the last one
                std::vector<std::string> typeElements(elements.begin(), elements.end() - 1);
                typePath = DreamPath(DreamPath::PathType::Absolute, typeElements);
                paramName = elements.back();
            } else if (elements.size() == 1) {
                // Just a name, no type
                paramName = elements[0];
            }
        } else {
            // Just a simple parameter name
            paramName = firstToken.Text;
        }
    }
    
    // Check for 'as' type specification
    std::optional<DMComplexValueType> explicitValueType;
    if (Current().Type == TokenType::As) {
        Advance();
        
        // Parse type flags (e.g., "num", "text", "num|text")
        std::string typeStr;
        
        // Collect type tokens until we hit a delimiter
        while (Current().Type != TokenType::Comma && 
               Current().Type != TokenType::RightParenthesis &&
               Current().Type != TokenType::Assign &&
               Current().Type != TokenType::Newline &&
               Current().Type != TokenType::EndOfFile) {
            
            if (Current().Type == TokenType::BitwiseOr) {
                typeStr += "|";
                Advance();
            } else if (IsInSet(Current().Type, IdentifierTypes_)) {
                typeStr += Current().Text;
                Advance();
            } else {
                break;
            }
        }
        
        // Convert type string to DMValueType flags
        DMValueType flags = ParseTypeFlags(typeStr);
        explicitValueType = DMComplexValueType(flags);
    }
    
    // Check for default value
    if (Current().Type == TokenType::Assign) {
        Advance();
        defaultValue = Expression();
    }
    
    return std::make_unique<DMASTDefinitionParameter>(loc, paramName, typePath, isList,
                                                      std::move(defaultValue), nullptr, explicitValueType);
}

// ObjectVarDefinition is no longer used - var blocks are now handled through the object definition path
// This function is kept for backwards compatibility but should not be called
std::unique_ptr<DMASTObjectStatement> DMParser::ObjectVarDefinition() {
    Location loc = CurrentLocation();
    Consume(TokenType::Var, "Expected 'var'");
    
    // This should not be reached anymore since var is now handled as part of object paths
    Emit(WarningCode::BadToken, "Unexpected var keyword - this should be handled as an object path");
    return nullptr;
}

std::unique_ptr<DMASTObjectStatement> DMParser::ObjectDefinition() {
    Location loc = CurrentLocation();
    
    // Parse object path
    auto path = ParsePath();
    
    // Save old path and update CurrentPath_
    DreamPath oldPath = CurrentPath_;
    DreamPath newPath = CurrentPath_.Combine(path.Path);
    CurrentPath_ = newPath;
    
    // Check for body
    std::vector<std::unique_ptr<DMASTObjectStatement>> innerStatements;
    
    if (Current().Type == TokenType::LeftCurlyBracket) {
        // Curly brace style: /obj/item { ... }
        Advance();
        Whitespace();
        
        while (Current().Type != TokenType::RightCurlyBracket && Current().Type != TokenType::EndOfFile) {
            auto stmt = ObjectStatement();
            if (stmt) {
                innerStatements.push_back(std::move(stmt));
            } else {
                // If we can't parse a statement, skip to next line to avoid infinite loop
                while (Current().Type != TokenType::Newline &&
                       Current().Type != TokenType::RightCurlyBracket &&
                       Current().Type != TokenType::EndOfFile) {
                    Advance();
                }
                if (Current().Type == TokenType::Newline) {
                    Advance();
                }
            }
            
            // Skip statement terminators
            while (Current().Type == TokenType::Semicolon || Current().Type == TokenType::Newline) {
                Advance();
            }
            Whitespace();
        }
        
        Consume(TokenType::RightCurlyBracket, "Expected '}'");
    } else if (Current().Type == TokenType::Newline) {
        // Indentation style: /obj/item\n\t...
        // Skip the newline after the path
        Advance();
        
        // Get base indentation level (the object definition line)
        int baseIndent = loc.Column;
        
        // Parse indented block
        innerStatements = ParseIndentedObjectBlock(baseIndent);
    }
    
    // Restore old path
    CurrentPath_ = oldPath;
    
    return std::make_unique<DMASTObjectDefinition>(loc, path, std::move(innerStatements));
}

int DMParser::GetCurrentIndentation() {
    // Returns the column position of the current token
    // This represents how many spaces/tabs from the start of the line
    return CurrentLocation().Column;
}

std::vector<std::unique_ptr<DMASTObjectStatement>> DMParser::ParseIndentedObjectBlock(int baseIndent) {
    std::vector<std::unique_ptr<DMASTObjectStatement>> statements;
    
    // Parse all statements that are indented more than the base level
    while (Current().Type != TokenType::EndOfFile) {
        // Skip empty lines
        while (Current().Type == TokenType::Newline) {
            Advance();
        }
        
        if (Current().Type == TokenType::EndOfFile) {
            break;
        }
        
        // Check indentation
        int currentIndent = GetCurrentIndentation();
        
        // If we're back at or before the base indentation, we're done with this block
        if (currentIndent <= baseIndent) {
            break;
        }
        
        // Save the current position's indentation - this is the expected indentation
        // for statements at this nesting level
        int expectedIndent = currentIndent;
        
        // Parse the statement
        auto stmt = ObjectStatement();
        if (stmt) {
            statements.push_back(std::move(stmt));
        } else {
            // Skip to next line to avoid infinite loop
            while (Current().Type != TokenType::Newline && Current().Type != TokenType::EndOfFile) {
                Advance();
            }
            if (Current().Type == TokenType::Newline) {
                Advance();
            }
        }
        
        // Skip trailing semicolons/newlines
        while (Current().Type == TokenType::Semicolon || Current().Type == TokenType::Newline) {
            Advance();
        }
        
        // After parsing a statement (which might have consumed deeply nested content),
        // we need to continue parsing more statements at the same level.
        // Don't break just because the current position has lower indentation -
        // skip lines with lower indentation until we find another statement at our level
        // or determine that the block has truly ended.
    }
    
    return statements;
}

}  // namespace DMCompiler
