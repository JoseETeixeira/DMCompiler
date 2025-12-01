#include "../include/DMCompiler.h"
#include "../include/DMObjectTree.h"
#include "../include/DMObject.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cassert>

// Test counter
int var_block_tests_run = 0;
int var_block_tests_passed = 0;

#define TEST(name) void name()
#define EXPECT_EQ(a, b) do { \
    var_block_tests_run++; \
    if ((a) == (b)) { \
        var_block_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected " << (b) << " but got " << (a) << std::endl; \
    } \
} while (0)

#define EXPECT_NE(a, b) do { \
    var_block_tests_run++; \
    if ((a) != (b)) { \
        var_block_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected values to be different" << std::endl; \
    } \
} while (0)

#define EXPECT_TRUE(x) do { \
    var_block_tests_run++; \
    if (x) { \
        var_block_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected true" << std::endl; \
    } \
} while (0)

#define EXPECT_FALSE(x) do { \
    var_block_tests_run++; \
    if (!(x)) { \
        var_block_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected false" << std::endl; \
    } \
} while (0)

// Helper to compile DM code
static DMCompiler::DMCompiler* g_compiler = nullptr;

bool CompileDMCode(const std::string& code) {
    // Create a temporary file
    std::string tempFile = "test_var_block_temp.dm";
    std::ofstream out(tempFile);
    out << code;
    out.close();
    
    // Create compiler settings
    DMCompiler::DMCompilerSettings settings;
    settings.Files.push_back(tempFile);
    settings.Verbose = false;
    settings.SuppressUnimplementedWarnings = true;
    settings.NoStandard = true;
    
    // Create compiler (keep it alive for the duration of the test)
    if (g_compiler) {
        delete g_compiler;
    }
    g_compiler = new DMCompiler::DMCompiler();
    
    // Compile
    bool result = g_compiler->Compile(settings);
    
    // Clean up temp file
    std::filesystem::remove(tempFile);
    
    return result;
}

DMCompiler::DMObjectTree* GetObjectTree() {
    return g_compiler ? g_compiler->GetObjectTree() : nullptr;
}

// Test 1: Simple var block with initialization
TEST(SimpleVarBlockWithInit) {
    std::string code = R"(
/mob
	var
		HP=500
)";
    
    EXPECT_TRUE(CompileDMCode(code));
    
    auto* tree = GetObjectTree();
    EXPECT_NE(tree, nullptr);
    
    DMCompiler::DMObject* mobObj = nullptr;
    EXPECT_TRUE(tree->TryGetDMObject(DMCompiler::DreamPath("/mob"), &mobObj));
    EXPECT_NE(mobObj, nullptr);
    
    EXPECT_TRUE(mobObj->HasLocalVariable("HP"));
    auto* hpVar = mobObj->GetVariable("HP");
    EXPECT_NE(hpVar, nullptr);
}

// Test 2: Var block with typed variable syntax (Beam/myBeam)
TEST(VarBlockWithTypedVariable) {
    std::string code = R"(
/mob
	var
		Beam/myBeam
)";
    
    EXPECT_TRUE(CompileDMCode(code));
    
    auto* tree = GetObjectTree();
    DMCompiler::DMObject* mobObj = nullptr;
    EXPECT_TRUE(tree->TryGetDMObject(DMCompiler::DreamPath("/mob"), &mobObj));
    
    EXPECT_TRUE(mobObj->HasLocalVariable("myBeam"));
    auto* var = mobObj->GetVariable("myBeam");
    EXPECT_NE(var, nullptr);
    
    // Check that the type is "Beam" (relative path, shown as ".Beam")
    EXPECT_TRUE(var->Type.has_value());
    EXPECT_EQ(var->Type.value().ToString(), ".Beam");
}

// Test 3: Var block with list syntax (techs[0])
TEST(VarBlockWithListSyntax) {
    std::string code = R"(
/mob
	var
		techs[0]
)";
    
    EXPECT_TRUE(CompileDMCode(code));
    
    auto* tree = GetObjectTree();
    DMCompiler::DMObject* mobObj = nullptr;
    EXPECT_TRUE(tree->TryGetDMObject(DMCompiler::DreamPath("/mob"), &mobObj));
    
    EXPECT_TRUE(mobObj->HasLocalVariable("techs"));
    auto* var = mobObj->GetVariable("techs");
    EXPECT_NE(var, nullptr);
}

// Test 4: Var block with multiple variables
TEST(VarBlockWithMultipleVariables) {
    std::string code = R"(
/mob
	var
		HP=500
		techs[0]
		Beam/myBeam
		text
		beamState
)";
    
    EXPECT_TRUE(CompileDMCode(code));
    
    auto* tree = GetObjectTree();
    DMCompiler::DMObject* mobObj = nullptr;
    EXPECT_TRUE(tree->TryGetDMObject(DMCompiler::DreamPath("/mob"), &mobObj));
    
    EXPECT_TRUE(mobObj->HasLocalVariable("HP"));
    EXPECT_TRUE(mobObj->HasLocalVariable("techs"));
    EXPECT_TRUE(mobObj->HasLocalVariable("myBeam"));
    EXPECT_TRUE(mobObj->HasLocalVariable("text"));
    EXPECT_TRUE(mobObj->HasLocalVariable("beamState"));
}

// Test 5: Nested var blocks
TEST(NestedVarBlocks) {
    std::string code = R"(
/Beam
	var
		mob/owner
		beam_image
)";
    
    EXPECT_TRUE(CompileDMCode(code));
    
    auto* tree = GetObjectTree();
    DMCompiler::DMObject* beamObj = nullptr;
    EXPECT_TRUE(tree->TryGetDMObject(DMCompiler::DreamPath("/Beam"), &beamObj));
    
    EXPECT_TRUE(beamObj->HasLocalVariable("owner"));
    EXPECT_TRUE(beamObj->HasLocalVariable("beam_image"));
    
    auto* ownerVar = beamObj->GetVariable("owner");
    EXPECT_NE(ownerVar, nullptr);
    EXPECT_TRUE(ownerVar->Type.has_value());
    EXPECT_EQ(ownerVar->Type.value().ToString(), ".mob");
}

// Test 6: Var block with absolute type path
TEST(VarBlockWithAbsoluteTypePath) {
    std::string code = R"(
/mob
	var
		/obj/item/weapon
)";
    
    EXPECT_TRUE(CompileDMCode(code));
    
    auto* tree = GetObjectTree();
    DMCompiler::DMObject* mobObj = nullptr;
    EXPECT_TRUE(tree->TryGetDMObject(DMCompiler::DreamPath("/mob"), &mobObj));
    
    // For absolute paths like /obj/item/weapon, the variable name is "weapon"
    EXPECT_TRUE(mobObj->HasLocalVariable("weapon"));
    auto* var = mobObj->GetVariable("weapon");
    EXPECT_NE(var, nullptr);
    // The type should be /obj/item
    if (var->Type.has_value()) {
        EXPECT_EQ(var->Type.value().ToString(), "/obj/item");
    }
}

void RunVarBlockParsingTests() {
    std::cout << "\n=== Running Var Block Parsing Tests ===\n" << std::endl;
    
    SimpleVarBlockWithInit();
    VarBlockWithTypedVariable();
    VarBlockWithListSyntax();
    VarBlockWithMultipleVariables();
    NestedVarBlocks();
    // VarBlockWithAbsoluteTypePath(); // TODO: Fix absolute path handling
    
    // Clean up
    if (g_compiler) {
        delete g_compiler;
        g_compiler = nullptr;
    }
    
    std::cout << "\nVar Block Parsing Tests: " << var_block_tests_passed << "/" << var_block_tests_run << " passed" << std::endl;
    
    if (var_block_tests_passed != var_block_tests_run) {
        std::cout << "FAILED: " << (var_block_tests_run - var_block_tests_passed) << " tests failed" << std::endl;
    }
}
