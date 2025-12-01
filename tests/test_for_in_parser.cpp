#include <gtest/gtest.h>
#include "DMParser.h"
#include "DMLexer.h"
#include "DMPreprocessor.h"
#include "DMASTStatement.h"
#include "DMASTExpression.h"
#include <sstream>

using namespace DMCompiler;

// Helper function to parse a proc body
std::unique_ptr<DMASTProcBlockInner> ParseProcBody(const std::string& code) {
    std::istringstream input(code);
    DMPreprocessor preprocessor(input, "test.dm");
    DMLexer lexer(preprocessor);
    DMParser parser(lexer);
    
    // Parse as a proc block
    return parser.ProcBlockInner();
}

TEST(ForInParserTest, ParseSimpleTypedVariable) {
    std::string code = R"(
    for(var/atom/movable/A in src)
        if(A)
            return A
)";
    
    auto block = ParseProcBody(code);
    ASSERT_NE(block, nullptr);
    ASSERT_EQ(block->Statements.size(), 1);
    
    auto* forInStmt = dynamic_cast<DMASTProcStatementForIn*>(block->Statements[0].get());
    ASSERT_NE(forInStmt, nullptr);
    
    // Check variable declaration info
    EXPECT_EQ(forInStmt->VarDecl.Name, "A");
    EXPECT_TRUE(forInStmt->VarDecl.TypePath.has_value());
    
    if (forInStmt->VarDecl.TypePath.has_value()) {
        std::string typePath = forInStmt->VarDecl.TypePath->ToString();
        EXPECT_EQ(typePath, "/atom/movable");
    }
    
    EXPECT_FALSE(forInStmt->VarDecl.TypeFilter.has_value());
}

TEST(ForInParserTest, ParseTypedVariableWithFilter) {
    std::string code = R"(
    for(var/mob/M as /mob|mob in world)
        if(M.key == "test")
            break
)";
    
    auto block = ParseProcBody(code);
    ASSERT_NE(block, nullptr);
    ASSERT_EQ(block->Statements.size(), 1);
    
    auto* forInStmt = dynamic_cast<DMASTProcStatementForIn*>(block->Statements[0].get());
    ASSERT_NE(forInStmt, nullptr);
    
    // Check variable declaration info
    EXPECT_EQ(forInStmt->VarDecl.Name, "M");
    EXPECT_TRUE(forInStmt->VarDecl.TypePath.has_value());
    
    if (forInStmt->VarDecl.TypePath.has_value()) {
        std::string typePath = forInStmt->VarDecl.TypePath->ToString();
        EXPECT_EQ(typePath, "/mob");
    }
    
    EXPECT_TRUE(forInStmt->VarDecl.TypeFilter.has_value());
    if (forInStmt->VarDecl.TypeFilter.has_value()) {
        EXPECT_EQ(forInStmt->VarDecl.TypeFilter.value(), "/mob|mob");
    }
}

TEST(ForInParserTest, ParseSimpleIdentifier) {
    std::string code = R"(
    for(x in list(1, 2, 3))
        if(x == 2)
            continue
)";
    
    auto block = ParseProcBody(code);
    ASSERT_NE(block, nullptr);
    ASSERT_EQ(block->Statements.size(), 1);
    
    auto* forInStmt = dynamic_cast<DMASTProcStatementForIn*>(block->Statements[0].get());
    ASSERT_NE(forInStmt, nullptr);
    
    // Check variable declaration info
    EXPECT_EQ(forInStmt->VarDecl.Name, "x");
    EXPECT_FALSE(forInStmt->VarDecl.TypePath.has_value());
    EXPECT_FALSE(forInStmt->VarDecl.TypeFilter.has_value());
}
