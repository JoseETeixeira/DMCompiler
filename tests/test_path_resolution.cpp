#include "../include/DMCompiler.h"
#include "../include/DMObjectTree.h"
#include "../include/DMObject.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cassert>

// Test counter
int path_resolution_tests_run = 0;
int path_resolution_tests_passed = 0;

#define TEST(name) void name()
#define EXPECT_EQ(a, b) do { \
    path_resolution_tests_run++; \
    if ((a) == (b)) { \
        path_resolution_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected " << (b) << " but got " << (a) << std::endl; \
    } \
} while (0)

#define EXPECT_NE(a, b) do { \
    path_resolution_tests_run++; \
    if ((a) != (b)) { \
        path_resolution_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected values to be different" << std::endl; \
    } \
} while (0)

#define EXPECT_TRUE(x) do { \
    path_resolution_tests_run++; \
    if (x) { \
        path_resolution_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected true" << std::endl; \
    } \
} while (0)

#define EXPECT_FALSE(x) do { \
    path_resolution_tests_run++; \
    if (!(x)) { \
        path_resolution_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected false" << std::endl; \
    } \
} while (0)

// Global compiler instance to keep objects alive
static std::unique_ptr<DMCompiler::DMCompiler> g_test_compiler;

// Helper to compile DM code
DMCompiler::DMCompiler* CompileCode(const std::string& code) {
    // Create a temporary file
    std::string tempFile = "test_path_resolution_temp.dm";
    std::ofstream out(tempFile);
    out << code;
    out.close();

    // Configure compiler settings
    DMCompiler::DMCompilerSettings settings;
    settings.Files.push_back(tempFile);
    settings.Verbose = false;
    settings.NoStandard = true;
    
    // Create compiler instance (replace any previous one)
    g_test_compiler = std::make_unique<DMCompiler::DMCompiler>();
    
    // Compile
    bool success = g_test_compiler->Compile(settings);
    
    // Clean up temp file
    std::filesystem::remove(tempFile);
    std::filesystem::remove("test_path_resolution_temp.json");
    
    if (!success) {
        std::cout << "Compilation failed!" << std::endl;
        return nullptr;
    }
    
    return g_test_compiler.get();
}

TEST(RelativeObjectDefinitionBecomesAbsolute) {
    std::string code = R"(
mob
    var
        HP=500
)";
    
    auto* compiler = CompileCode(code);
    EXPECT_NE(compiler, nullptr);
    
    if (compiler) {
        auto* objectTree = compiler->GetObjectTree();
        EXPECT_NE(objectTree, nullptr);
        
        // Check that /mob exists (absolute path)
        DMCompiler::DMObject* mobObj = nullptr;
        bool foundMob = objectTree->TryGetDMObject(DMCompiler::DreamPath("/mob"), &mobObj);
        EXPECT_TRUE(foundMob);
        EXPECT_NE(mobObj, nullptr);
        
        // Check that .mob does NOT exist (relative path should have been converted)
        DMCompiler::DMObject* relativeMobObj = nullptr;
        bool foundRelativeMob = objectTree->TryGetDMObject(DMCompiler::DreamPath(".mob"), &relativeMobObj);
        EXPECT_FALSE(foundRelativeMob);
        
        // Check that the variable is on /mob
        if (mobObj) {
            EXPECT_TRUE(mobObj->HasLocalVariable("HP"));
        }
    }
}

TEST(VarBlockVariablesOnAbsolutePath) {
    std::string code = R"(
mob
    var
        HP=500
        myVar
)";
    
    auto* compiler = CompileCode(code);
    EXPECT_NE(compiler, nullptr);
    
    if (compiler) {
        auto* objectTree = compiler->GetObjectTree();
        EXPECT_NE(objectTree, nullptr);
        
        DMCompiler::DMObject* mobObj = nullptr;
        bool foundMob = objectTree->TryGetDMObject(DMCompiler::DreamPath("/mob"), &mobObj);
        EXPECT_TRUE(foundMob);
        EXPECT_NE(mobObj, nullptr);
        
        if (mobObj) {
            // Both variables should be on /mob
            EXPECT_TRUE(mobObj->HasLocalVariable("HP"));
            EXPECT_TRUE(mobObj->HasLocalVariable("myVar"));
        }
    }
}

TEST(NestedRelativePathsResolveCorrectly) {
    std::string code = R"(
mob
    player
        var
            level=1
)";
    
    auto* compiler = CompileCode(code);
    EXPECT_NE(compiler, nullptr);
    
    if (compiler) {
        auto* objectTree = compiler->GetObjectTree();
        EXPECT_NE(objectTree, nullptr);
        
        // Check that /mob/player exists (absolute path)
        DMCompiler::DMObject* playerObj = nullptr;
        bool foundPlayer = objectTree->TryGetDMObject(DMCompiler::DreamPath("/mob/player"), &playerObj);
        EXPECT_TRUE(foundPlayer);
        EXPECT_NE(playerObj, nullptr);
        
        // Check that the variable is on /mob/player
        if (playerObj) {
            EXPECT_TRUE(playerObj->HasLocalVariable("level"));
        }
    }
}

TEST(TypedVariableSyntaxWithRelativePath) {
    std::string code = R"(
mob
    var
        Beam/myBeam
)";
    
    auto* compiler = CompileCode(code);
    EXPECT_NE(compiler, nullptr);
    
    if (compiler) {
        auto* objectTree = compiler->GetObjectTree();
        EXPECT_NE(objectTree, nullptr);
        
        DMCompiler::DMObject* mobObj = nullptr;
        bool foundMob = objectTree->TryGetDMObject(DMCompiler::DreamPath("/mob"), &mobObj);
        EXPECT_TRUE(foundMob);
        EXPECT_NE(mobObj, nullptr);
        
        if (mobObj) {
            // myBeam should be on /mob
            EXPECT_TRUE(mobObj->HasLocalVariable("myBeam"));
        }
    }
}

int main() {
    std::cout << "Running path resolution tests..." << std::endl;
    
    RelativeObjectDefinitionBecomesAbsolute();
    VarBlockVariablesOnAbsolutePath();
    NestedRelativePathsResolveCorrectly();
    TypedVariableSyntaxWithRelativePath();
    
    std::cout << "\nPath Resolution Tests: " << path_resolution_tests_passed 
              << "/" << path_resolution_tests_run << " passed" << std::endl;
    
    return (path_resolution_tests_passed == path_resolution_tests_run) ? 0 : 1;
}
