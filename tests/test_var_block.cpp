#include <iostream>
#include <cassert>
#include <memory>
#include "../include/DMParser.h"
#include "../include/DMLexer.h"
#include "../include/DMCompiler.h"
#include "../include/DMASTStatement.h"

// Helper function to parse a statement from a string
std::unique_ptr<DMCompiler::DMASTProcStatement> ParseStatementForVarBlock(const std::string& code) {
    DMCompiler::DMCompiler compiler;
    DMCompiler::DMLexer lexer("test.dm", code);
    DMCompiler::DMParser parser(&compiler, &lexer);
    return parser.ProcStatement();
}

bool TestMultiLineVarBlock() {
    std::cout << "  TestMultiLineVarBlock..." << std::endl;
    
    // Note: The lexer/parser handles indentation via newlines and tabs/spaces.
    // We simulate this with explicit newlines and indentation.
    std::string code = "var\n\tx = 1\n\ty = 2\n\tlist/L";
    
    auto stmt = ParseStatementForVarBlock(code);

    if (!stmt) {
        std::cerr << "    FAILED: Parse returned null" << std::endl;
        return false;
    }
    
    auto* varDecl = dynamic_cast<DMCompiler::DMASTProcStatementVarDeclaration*>(stmt.get());
    if (!varDecl) {
        std::cerr << "    FAILED: Not a var declaration" << std::endl;
        return false;
    }
    
    if (varDecl->Decls.size() != 3) {
        std::cerr << "    FAILED: Expected 3 declarations, got " << varDecl->Decls.size() << std::endl;
        return false;
    }
    
    // Check x
    if (varDecl->Decls[0].Name != "x") {
        std::cerr << "    FAILED: First var should be x, got " << varDecl->Decls[0].Name << std::endl;
        return false;
    }
    if (!varDecl->Decls[0].Value) {
        std::cerr << "    FAILED: x should have value" << std::endl;
        return false;
    }
    
    // Check y
    if (varDecl->Decls[1].Name != "y") {
        std::cerr << "    FAILED: Second var should be y, got " << varDecl->Decls[1].Name << std::endl;
        return false;
    }
    if (!varDecl->Decls[1].Value) {
        std::cerr << "    FAILED: y should have value" << std::endl;
        return false;
    }
    
    // Check L
    if (varDecl->Decls[2].Name != "L") {
        std::cerr << "    FAILED: Third var should be L, got " << varDecl->Decls[2].Name << std::endl;
        return false;
    }
    
    std::string typeStr = varDecl->Decls[2].TypePath ? varDecl->Decls[2].TypePath->ToString() : "null";
    // Accept /list (absolute) or .list (relative)
    if (typeStr != "/list" && typeStr != ".list") {
        std::cerr << "    FAILED: L should be type /list or .list, got " << typeStr << std::endl;
        return false;
    }
    
    std::cout << "    PASSED" << std::endl;
    return true;
}

int RunMultiLineVarBlockTests() {
    bool success = true;
    if (!TestMultiLineVarBlock()) success = false;
    
    if (success) {
        std::cout << "MultiLineVarBlockTests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "MultiLineVarBlockTests failed." << std::endl;
        return 1;
    }
}
