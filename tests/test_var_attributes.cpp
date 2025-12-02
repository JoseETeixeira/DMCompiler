/// @file test_var_attributes.cpp
/// @brief Unit tests for variable attribute extraction (const, final, tmp, global, static)

#include "../include/DMCompiler.h"
#include "../include/DMObjectTree.h"
#include "../include/DMObject.h"
#include "../include/DMVariable.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cassert>

// Test counter
int var_attr_tests_run = 0;
int var_attr_tests_passed = 0;

#define TEST(name) void name()
#define EXPECT_EQ(a, b) do { \
    var_attr_tests_run++; \
    if ((a) == (b)) { \
        var_attr_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected " << (b) << " but got " << (a) << std::endl; \
    } \
} while (0)

#define EXPECT_NE(a, b) do { \
    var_attr_tests_run++; \
    if ((a) != (b)) { \
        var_attr_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected values to be different" << std::endl; \
    } \
} while (0)

#define EXPECT_TRUE(x) do { \
    var_attr_tests_run++; \
    if (x) { \
        var_attr_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected true" << std::endl; \
    } \
} while (0)

#define EXPECT_FALSE(x) do { \
    var_attr_tests_run++; \
    if (!(x)) { \
        var_attr_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected false" << std::endl; \
    } \
} while (0)

// Helper to compile DM code
static DMCompiler::DMCompiler* g_compiler = nullptr;

bool CompileDMCode(const std::string& code) {
    // Create a temporary file
    std::string tempFile = "test_var_attributes_temp.dm";
    std::ofstream out(tempFile);
    out << code;
    out.close();
    
    // Create compiler settings
    DMCompiler::DMCompilerSettings settings;
    settings.Files.push_back(tempFile);
    settings.Verbose = false; // Enable verbose for debugging
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

// ============================================================================
// VarModifiers::Parse() Unit Tests
// ============================================================================

TEST(VarModifiersParse_NoModifiers) {
    // Test parsing a simple variable path with no modifiers
    DMCompiler::DreamPath path("/var/mob");
    auto mods = DMCompiler::VarModifiers::Parse(path);
    
    EXPECT_FALSE(mods.IsConst);
    EXPECT_FALSE(mods.IsFinal);
    EXPECT_FALSE(mods.IsGlobal);
    EXPECT_FALSE(mods.IsStatic);
    EXPECT_FALSE(mods.IsTmp);
    EXPECT_TRUE(mods.TypePath.has_value());
    EXPECT_EQ(mods.TypePath.value().ToString(), "/mob");
}

TEST(VarModifiersParse_ConstModifier) {
    // Test parsing var/const/x
    DMCompiler::DreamPath path("/var/const");
    auto mods = DMCompiler::VarModifiers::Parse(path);
    
    EXPECT_TRUE(mods.IsConst);
    EXPECT_FALSE(mods.IsFinal);
    EXPECT_FALSE(mods.IsGlobal);
    EXPECT_FALSE(mods.IsTmp);
    EXPECT_FALSE(mods.TypePath.has_value());
}

TEST(VarModifiersParse_TmpModifier) {
    // Test parsing var/tmp/x
    DMCompiler::DreamPath path("/var/tmp");
    auto mods = DMCompiler::VarModifiers::Parse(path);
    
    EXPECT_FALSE(mods.IsConst);
    EXPECT_FALSE(mods.IsFinal);
    EXPECT_FALSE(mods.IsGlobal);
    EXPECT_TRUE(mods.IsTmp);
    EXPECT_FALSE(mods.TypePath.has_value());
}

TEST(VarModifiersParse_GlobalModifier) {
    // Test parsing var/global/x
    DMCompiler::DreamPath path("/var/global");
    auto mods = DMCompiler::VarModifiers::Parse(path);
    
    EXPECT_FALSE(mods.IsConst);
    EXPECT_FALSE(mods.IsFinal);
    EXPECT_TRUE(mods.IsGlobal);
    EXPECT_FALSE(mods.IsTmp);
    EXPECT_FALSE(mods.TypePath.has_value());
}

TEST(VarModifiersParse_StaticModifier) {
    // Test parsing var/static/x (static is alias for global)
    DMCompiler::DreamPath path("/var/static");
    auto mods = DMCompiler::VarModifiers::Parse(path);
    
    EXPECT_FALSE(mods.IsConst);
    EXPECT_FALSE(mods.IsFinal);
    EXPECT_TRUE(mods.IsGlobal);  // static sets IsGlobal
    EXPECT_TRUE(mods.IsStatic);
    EXPECT_FALSE(mods.IsTmp);
    EXPECT_FALSE(mods.TypePath.has_value());
}

TEST(VarModifiersParse_FinalModifier) {
    // Test parsing var/final/x
    DMCompiler::DreamPath path("/var/final");
    auto mods = DMCompiler::VarModifiers::Parse(path);
    
    EXPECT_FALSE(mods.IsConst);
    EXPECT_TRUE(mods.IsFinal);
    EXPECT_FALSE(mods.IsGlobal);
    EXPECT_FALSE(mods.IsTmp);
    EXPECT_FALSE(mods.TypePath.has_value());
}

TEST(VarModifiersParse_MultipleModifiers) {
    // Test parsing var/const/global/mob - multiple modifiers with type
    DMCompiler::DreamPath path("/var/const/global/mob");
    auto mods = DMCompiler::VarModifiers::Parse(path);
    
    EXPECT_TRUE(mods.IsConst);
    EXPECT_FALSE(mods.IsFinal);
    EXPECT_TRUE(mods.IsGlobal);
    EXPECT_FALSE(mods.IsTmp);
    EXPECT_TRUE(mods.TypePath.has_value());
    EXPECT_EQ(mods.TypePath.value().ToString(), "/mob");
}

TEST(VarModifiersParse_TmpWithType) {
    // Test parsing var/tmp/list - tmp with a type
    DMCompiler::DreamPath path("/var/tmp/list");
    auto mods = DMCompiler::VarModifiers::Parse(path);
    
    EXPECT_FALSE(mods.IsConst);
    EXPECT_FALSE(mods.IsFinal);
    EXPECT_FALSE(mods.IsGlobal);
    EXPECT_TRUE(mods.IsTmp);
    EXPECT_TRUE(mods.TypePath.has_value());
    EXPECT_EQ(mods.TypePath.value().ToString(), "/list");
}

// ============================================================================
// Full Compilation Tests for Variable Attributes
// ============================================================================

TEST(CompileConstVar) {
    std::string code = R"(
/datum/test
    var/const/MY_CONST = 5
)";
    
    EXPECT_TRUE(CompileDMCode(code));
    
    auto* tree = GetObjectTree();
    EXPECT_NE(tree, nullptr);
    
    DMCompiler::DMObject* testObj = nullptr;
    EXPECT_TRUE(tree->TryGetDMObject(DMCompiler::DreamPath("/datum/test"), &testObj));
    EXPECT_NE(testObj, nullptr);
    
    // Check that MY_CONST has IsConst = true
    auto it = testObj->Variables.find("MY_CONST");
    EXPECT_TRUE(it != testObj->Variables.end());
    if (it != testObj->Variables.end()) {
        EXPECT_TRUE(it->second.IsConst);
        EXPECT_FALSE(it->second.IsTmp);
        EXPECT_FALSE(it->second.IsGlobal);
        EXPECT_FALSE(it->second.IsFinal);
    }
}

TEST(CompileTmpVar) {
    std::string code = R"(
/datum/test
    var/tmp/temp_data
)";
    
    EXPECT_TRUE(CompileDMCode(code));
    
    auto* tree = GetObjectTree();
    EXPECT_NE(tree, nullptr);
    
    DMCompiler::DMObject* testObj = nullptr;
    EXPECT_TRUE(tree->TryGetDMObject(DMCompiler::DreamPath("/datum/test"), &testObj));
    EXPECT_NE(testObj, nullptr);
    
    // Check that temp_data has IsTmp = true
    auto it = testObj->Variables.find("temp_data");
    EXPECT_TRUE(it != testObj->Variables.end());
    if (it != testObj->Variables.end()) {
        EXPECT_FALSE(it->second.IsConst);
        EXPECT_TRUE(it->second.IsTmp);
        EXPECT_FALSE(it->second.IsGlobal);
        EXPECT_FALSE(it->second.IsFinal);
    }
}

TEST(CompileGlobalVar) {
    std::string code = R"(
/datum/test
    var/global/shared_counter = 0
)";
    
    EXPECT_TRUE(CompileDMCode(code));
    
    auto* tree = GetObjectTree();
    EXPECT_NE(tree, nullptr);
    
    DMCompiler::DMObject* testObj = nullptr;
    EXPECT_TRUE(tree->TryGetDMObject(DMCompiler::DreamPath("/datum/test"), &testObj));
    EXPECT_NE(testObj, nullptr);
    
    // Check that shared_counter has IsGlobal = true
    auto it = testObj->Variables.find("shared_counter");
    EXPECT_TRUE(it != testObj->Variables.end());
    if (it != testObj->Variables.end()) {
        EXPECT_FALSE(it->second.IsConst);
        EXPECT_FALSE(it->second.IsTmp);
        EXPECT_TRUE(it->second.IsGlobal);
        EXPECT_FALSE(it->second.IsFinal);
    }
}

TEST(CompileStaticVar) {
    std::string code = R"(
/datum/test
    var/static/instance_count = 0
)";
    
    EXPECT_TRUE(CompileDMCode(code));
    
    auto* tree = GetObjectTree();
    EXPECT_NE(tree, nullptr);
    
    DMCompiler::DMObject* testObj = nullptr;
    EXPECT_TRUE(tree->TryGetDMObject(DMCompiler::DreamPath("/datum/test"), &testObj));
    EXPECT_NE(testObj, nullptr);
    
    // Check that instance_count has IsGlobal = true (static is alias for global)
    auto it = testObj->Variables.find("instance_count");
    EXPECT_TRUE(it != testObj->Variables.end());
    if (it != testObj->Variables.end()) {
        EXPECT_FALSE(it->second.IsConst);
        EXPECT_FALSE(it->second.IsTmp);
        EXPECT_TRUE(it->second.IsGlobal);  // static sets IsGlobal
        EXPECT_FALSE(it->second.IsFinal);
    }
}

TEST(CompileTypedConstVar) {
    std::string code = R"(
/datum/test
    var/const/mob/owner
)";
    
    EXPECT_TRUE(CompileDMCode(code));
    
    auto* tree = GetObjectTree();
    EXPECT_NE(tree, nullptr);
    
    DMCompiler::DMObject* testObj = nullptr;
    EXPECT_TRUE(tree->TryGetDMObject(DMCompiler::DreamPath("/datum/test"), &testObj));
    EXPECT_NE(testObj, nullptr);
    
    // Check that owner has IsConst = true and Type = /mob
    auto it = testObj->Variables.find("owner");
    EXPECT_TRUE(it != testObj->Variables.end());
    if (it != testObj->Variables.end()) {
        EXPECT_TRUE(it->second.IsConst);
        EXPECT_TRUE(it->second.Type.has_value());
        if (it->second.Type.has_value()) {
            EXPECT_EQ(it->second.Type.value().ToString(), "/mob");
        }
    }
}

TEST(CompileRegularVarNotMarked) {
    std::string code = R"(
/datum/test
    var/regular_var = 10
)";
    
    EXPECT_TRUE(CompileDMCode(code));
    
    auto* tree = GetObjectTree();
    EXPECT_NE(tree, nullptr);
    
    DMCompiler::DMObject* testObj = nullptr;
    EXPECT_TRUE(tree->TryGetDMObject(DMCompiler::DreamPath("/datum/test"), &testObj));
    EXPECT_NE(testObj, nullptr);
    
    // Check that regular_var has all modifiers = false
    auto it = testObj->Variables.find("regular_var");
    EXPECT_TRUE(it != testObj->Variables.end());
    if (it != testObj->Variables.end()) {
        EXPECT_FALSE(it->second.IsConst);
        EXPECT_FALSE(it->second.IsTmp);
        EXPECT_FALSE(it->second.IsGlobal);
        EXPECT_FALSE(it->second.IsFinal);
    }
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "Running Variable Attributes Tests..." << std::endl;
    std::cout << "========================================" << std::endl;
    
    // VarModifiers::Parse() unit tests
    std::cout << "\nVarModifiers::Parse() Tests:" << std::endl;
    VarModifiersParse_NoModifiers();
    VarModifiersParse_ConstModifier();
    VarModifiersParse_TmpModifier();
    VarModifiersParse_GlobalModifier();
    VarModifiersParse_StaticModifier();
    VarModifiersParse_FinalModifier();
    VarModifiersParse_MultipleModifiers();
    VarModifiersParse_TmpWithType();
    
    // Full compilation tests
    std::cout << "\nFull Compilation Tests:" << std::endl;
    CompileConstVar();
    CompileTmpVar();
    CompileGlobalVar();
    CompileStaticVar();
    CompileTypedConstVar();
    CompileRegularVarNotMarked();
    
    // Cleanup
    if (g_compiler) {
        delete g_compiler;
        g_compiler = nullptr;
    }
    
    // Summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "Variable Attributes Tests: " << var_attr_tests_passed << "/" << var_attr_tests_run << " passed" << std::endl;
    
    if (var_attr_tests_passed == var_attr_tests_run) {
        std::cout << "All tests PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "Some tests FAILED!" << std::endl;
        return 1;
    }
}
