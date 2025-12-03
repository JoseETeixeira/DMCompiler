#include "DMMParser.h"
#include "DMCompiler.h"
#include "DMObjectTree.h"
#include "DMObject.h"
#include "Token.h"
#include "DreamPath.h"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace DMCompiler {

DMMParser::DMMParser(DMCompiler* compiler, DMLexer* lexer, int zOffset)
    : DMParser(compiler, lexer), ZOffset_(zOffset), CellNameLength_(-1) {
}

std::unique_ptr<DreamMapJson> DMMParser::ParseMap() {
    auto map = std::make_unique<DreamMapJson>();
    
    CellNameLength_ = -1;
    
    bool parsing = true;
    while (parsing) {
        auto cellDefinition = ParseCellDefinition();
        bool foundCell = false;
        if (cellDefinition) {
            foundCell = true;
            if (CellNameLength_ == -1) {
                CellNameLength_ = static_cast<int>(cellDefinition->Name.length());
            }
            
            if (static_cast<int>(cellDefinition->Name.length()) == CellNameLength_) {
                map->CellDefinitions[cellDefinition->Name] = std::move(cellDefinition);
            } else {
                Warning("Invalid cell definition name length '" + cellDefinition->Name + "'");
            }
        }
        
        auto mapBlock = ParseMapBlock();
        bool foundBlock = false;
        if (mapBlock) {
            foundBlock = true;
            int maxX = mapBlock->X + mapBlock->Width - 1;
            int maxY = mapBlock->Y + mapBlock->Height - 1;
            if (map->MaxX < maxX) map->MaxX = maxX;
            if (map->MaxY < maxY) map->MaxY = maxY;
            if (map->MaxZ < mapBlock->Z) map->MaxZ = mapBlock->Z;
            
            map->Blocks.push_back(std::move(mapBlock));
        }
        
        if (!foundCell && !foundBlock) {
            parsing = false;
        }
    }
    
    Consume(TokenType::EndOfFile, "Expected EOF");
    return map;
}

std::unique_ptr<CellDefinitionJson> DMMParser::ParseCellDefinition() {
    // Skip whitespace and newlines
    while (Current().Type == TokenType::Newline || 
           Current().Type == TokenType::Indent || 
           Current().Type == TokenType::Dedent) {
        Advance();
    }
    
    Token currentToken = Current();
    
    if (Check(TokenType::String) || Check(TokenType::Identifier)) {
        Advance(); // Consume the string
        Consume(TokenType::Assign, "Expected '='");
        Consume(TokenType::LeftParenthesis, "Expected '('");
        
        std::string cellName = (currentToken.Type == TokenType::String) ? currentToken.Value.StringValue : currentToken.Text;
        auto cellDefinition = std::make_unique<CellDefinitionJson>(cellName);
        
        auto objectType = PathExpression();
        while (objectType) {
            DMObject* type = nullptr;
            bool foundType = Compiler_->GetObjectTree()->TryGetDMObject(objectType->Path, &type);
            
            if (!foundType && SkippedTypes_.insert(objectType->Path).second) {
                Warning("Skipping type '" + objectType->Path.ToString() + "'");
            }
            
            auto mapObject = std::make_unique<MapObjectJson>(type ? type->Id : -1);
            
            // Check for variable overrides
            if (Check(TokenType::LeftCurlyBracket)) {
                Advance(); // Consume {
                
                while (!Check(TokenType::RightCurlyBracket) && !Check(TokenType::EndOfFile)) {
                    // Skip newlines
                    while (Current().Type == TokenType::Newline) Advance();
                    
                    if (Check(TokenType::Identifier) || Check(TokenType::Var)) {
                        Advance(); // Consume variable name
                        
                        if (Check(TokenType::Assign)) {
                            Advance(); // Consume =
                            
                            // Parse value (constant or path)
                            auto expr = ConstantExpression();
                            if (!expr) {
                                auto path = PathExpression();
                                if (!path) {
                                    // Fallback: consume until semicolon or }
                                    while (!Check(TokenType::Semicolon) && !Check(TokenType::RightCurlyBracket) && !Check(TokenType::EndOfFile)) {
                                        Advance();
                                    }
                                }
                            }
                        }
                    } else if (Check(TokenType::Semicolon)) {
                        Advance();
                    } else {
                        // Unexpected token, skip to avoid infinite loop if not }
                        if (!Check(TokenType::RightCurlyBracket)) {
                            Advance();
                        }
                    }
                }
                
                Consume(TokenType::RightCurlyBracket, "Expected '}'");
            }
            
            // Add the map object to the appropriate list
            if (type != nullptr) {
                if (type->Path.IsDescendantOf(DreamPath::Turf) || type->Path == DreamPath::Turf) {
                    cellDefinition->Turf = std::move(mapObject);
                } else if (type->Path.IsDescendantOf(DreamPath::Area) || type->Path == DreamPath::Area) {
                    cellDefinition->Area = std::move(mapObject);
                } else {
                    cellDefinition->Objects.push_back(std::move(mapObject));
                }
            }
            
            // Check for more objects
            // Skip whitespace/newlines before checking for comma
            while (Current().Type == TokenType::Newline || 
                   Current().Type == TokenType::Indent || 
                   Current().Type == TokenType::Dedent) {
                Advance();
            }

            if (Check(TokenType::Comma)) {
                Advance();
                objectType = PathExpression();
            } else {
                objectType = nullptr;
            }
        }
        
        if (!cellDefinition->Turf) {
            Warning("Cell definition \"" + cellDefinition->Name + "\" is missing a turf");
        }
        
        Consume(TokenType::RightParenthesis, "Expected ')'");
        return cellDefinition;
    }
    
    return nullptr;
}

std::unique_ptr<MapBlockJson> DMMParser::ParseMapBlock() {
    // Skip whitespace and newlines
    while (Current().Type == TokenType::Newline || 
           Current().Type == TokenType::Indent || 
           Current().Type == TokenType::Dedent) {
        Advance();
    }
    
    auto coordinates = ParseCoordinates();
    
    if (coordinates) {
        auto mapBlock = std::make_unique<MapBlockJson>(
            coordinates->X, 
            coordinates->Y, 
            coordinates->Z
        );
        
        Consume(TokenType::Assign, "Expected '='");
        Token blockStringToken = Current();
        Consume(TokenType::String, "Expected a constant string");
        
        std::string blockString = blockStringToken.Value.StringValue;
        
        // Split by newlines
        std::vector<std::string> lines;
        std::stringstream ss(blockString);
        std::string line;
        while (std::getline(ss, line)) {
            // Trim whitespace
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            if (!line.empty()) {
                lines.push_back(line);
            }
        }
        
        mapBlock->Height = static_cast<int>(lines.size());
        for (int y = 1; y <= static_cast<int>(lines.size()); y++) {
            const std::string& currentLine = lines[y - 1];
            int width = static_cast<int>(currentLine.length()) / CellNameLength_;
            
            if (mapBlock->Width < width) {
                mapBlock->Width = width;
            }
            
            if ((static_cast<int>(currentLine.length()) % CellNameLength_) != 0) {
                Warning("Invalid map block row");
                return nullptr;
            }
            
            for (int x = 1; x <= width; x++) {
                std::string cell = currentLine.substr((x - 1) * CellNameLength_, CellNameLength_);
                mapBlock->Cells.push_back(cell);
            }
        }
        
        return mapBlock;
    }
    
    return nullptr;
}

std::unique_ptr<DMMParser::Coordinates> DMMParser::ParseCoordinates() {
    // Skip whitespace and newlines
    while (Current().Type == TokenType::Newline || 
           Current().Type == TokenType::Indent || 
           Current().Type == TokenType::Dedent) {
        Advance();
    }
    
    if (!Check(TokenType::LeftParenthesis)) {
        return nullptr;
    }
    
    Advance(); // Consume (
    
    auto xExpr = ConstantExpression();
    auto xConst = dynamic_cast<DMASTConstantInteger*>(xExpr.get());
    if (!xConst) {
        Warning("Expected an integer for X coordinate");
        return nullptr;
    }
    
    Consume(TokenType::Comma, "Expected ','");
    
    auto yExpr = ConstantExpression();
    auto yConst = dynamic_cast<DMASTConstantInteger*>(yExpr.get());
    if (!yConst) {
        Warning("Expected an integer for Y coordinate");
        return nullptr;
    }
    
    Consume(TokenType::Comma, "Expected ','");
    
    auto zExpr = ConstantExpression();
    auto zConst = dynamic_cast<DMASTConstantInteger*>(zExpr.get());
    if (!zConst) {
        Warning("Expected an integer for Z coordinate");
        return nullptr;
    }
    
    Consume(TokenType::RightParenthesis, "Expected ')'");
    
    auto coords = std::make_unique<Coordinates>();
    coords->X = static_cast<int>(xConst->Value);
    coords->Y = static_cast<int>(yConst->Value);
    coords->Z = static_cast<int>(zConst->Value) + ZOffset_;
    
    return coords;
}

std::unique_ptr<DMASTPath> DMMParser::PathExpression() {
    // Skip whitespace and newlines
    while (Current().Type == TokenType::Newline || 
           Current().Type == TokenType::Indent || 
           Current().Type == TokenType::Dedent) {
        Advance();
    }
    
    // Check if we have a path starting with /
    if (Current().Type != TokenType::Slash && Current().Type != TokenType::Divide) {
        return nullptr;
    }
    
    // Use the inherited ParsePath() method
    DMASTPath path = ParsePath();
    return std::make_unique<DMASTPath>(path);
}

std::unique_ptr<DMASTExpression> DMMParser::ConstantExpression() {
    // Skip whitespace and newlines
    while (Current().Type == TokenType::Newline || 
           Current().Type == TokenType::Indent || 
           Current().Type == TokenType::Dedent) {
        Advance();
    }
    
    Token token = Current();
    
    if (token.Type == TokenType::Number) {
        Advance();
        if (token.Value.ValueType == Token::TokenValue::Type::Int) {
            return std::make_unique<DMASTConstantInteger>(token.Loc, token.Value.IntValue);
        } else if (token.Value.ValueType == Token::TokenValue::Type::Float) {
            return std::make_unique<DMASTConstantFloat>(token.Loc, token.Value.FloatValue);
        }
    } else if (token.Type == TokenType::String) {
        Advance();
        return std::make_unique<DMASTConstantString>(token.Loc, token.Value.StringValue);
    } else if (token.Type == TokenType::Null) {
        Advance();
        return std::make_unique<DMASTConstantNull>(token.Loc);
    }
    
    return nullptr;
}

} // namespace DMCompiler
