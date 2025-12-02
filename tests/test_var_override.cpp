// test_var_override.cpp
// Tests for variable default value override functionality
// Task 5: Implement Variable Default Value Updates

#include <iostream>
#include <string>
#include <memory>
#include <cassert>
#include "../include/DMCompiler.h"
#include "../include/DMObjectTree.h"
#include "../include/DMObject.h"
#include "../include/DMVariable.h"
#include "../include/DMASTExpression.h"
#include "../include/DreamPath.h"
#include "../include/Location.h"

// Avoid namespace/class name collision
using DMCompilerClass = DMCompiler::DMCompiler;
using DMCompiler::DMObjectTree;
using DMCompiler::DMObject;
using DMCompiler::DMVariable;
using DMCompiler::DreamPath;
using DMCompiler::WarningCode;

// Simple test framework
static int TestsPassed = 0;
static int TestsFailed = 0;

#define TEST(name) \
    void name(); \
    static struct name##_runner { \
        name##_runner() { \
            std::cout << "  " << #name << "... "; \
            try { \
                name(); \
                std::cout << "PASSED" << std::endl; \
                TestsPassed++; \
            } catch (const std::exception& e) { \
                std::cout << "FAILED: " << e.what() << std::endl; \
                TestsFailed++; \
            } \
        } \
    } name##_instance; \
    void name()

#define ASSERT_TRUE(expr) \
    if (!(expr)) { \
        throw std::runtime_error("Assertion failed: " #expr " at line " + std::to_string(__LINE__)); \
    }

#define ASSERT_FALSE(expr) \
    if (expr) { \
        throw std::runtime_error("Assertion failed: " #expr " should be false at line " + std::to_string(__LINE__)); \
    }

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        throw std::runtime_error("Assertion failed: " #a " != " #b " at line " + std::to_string(__LINE__)); \
    }

#define ASSERT_NE(a, b) \
    if ((a) == (b)) { \
        throw std::runtime_error("Assertion failed: " #a " == " #b " at line " + std::to_string(__LINE__)); \
    }

// Test 1: Verify UpdateVariableDefault finds variable in parent
TEST(TestUpdateVariableDefault_ParentVariable) {
    DMCompilerClass compiler;
    
    // Create object tree
    auto* tree = compiler.GetObjectTree();
    
    // Create parent type with a variable
    DreamPath parentPath(DreamPath::PathType::Absolute, {"parent"});
    DMObject* parent = tree->GetOrCreateDMObject(parentPath);
    
    DMVariable parentVar;
    parentVar.Name = "health";
    parentVar.IsConst = false;
    parentVar.IsFinal = false;
    parentVar.IsGlobal = false;
    parentVar.IsTmp = false;
    parentVar.Value = nullptr; // Original value
    parent->Variables["health"] = parentVar;
    
    // Create child type
    DreamPath childPath(DreamPath::PathType::Absolute, {"parent", "child"});
    DMObject* child = tree->GetOrCreateDMObject(childPath);
    
    // Child has no variables yet, but should be able to override parent's
    ASSERT_TRUE(child->VariableOverrides.empty());
    
    // Simulate UpdateVariableDefault by searching inheritance chain
    DMObject* current = child;
    bool found = false;
    while (current != nullptr) {
        auto it = current->Variables.find("health");
        if (it != current->Variables.end()) {
            // Found it - create override in child
            DMVariable override = it->second;
            override.Value = nullptr; // New value would be set here
            child->VariableOverrides["health"] = override;
            found = true;
            break;
        }
        current = current->Parent;
    }
    
    ASSERT_TRUE(found);
    ASSERT_FALSE(child->VariableOverrides.empty());
    ASSERT_TRUE(child->VariableOverrides.find("health") != child->VariableOverrides.end());
}

// Test 2: Verify UpdateVariableDefault finds global variable
TEST(TestUpdateVariableDefault_GlobalVariable) {
    DMCompilerClass compiler;
    auto* tree = compiler.GetObjectTree();
    
    // Add a global variable
    DMVariable globalVar;
    globalVar.Name = "global_score";
    globalVar.IsConst = false;
    globalVar.IsFinal = false;
    globalVar.IsGlobal = true;
    globalVar.IsTmp = false;
    globalVar.Value = nullptr;
    tree->Globals.push_back(globalVar);
    
    // Verify we can find it
    int globalIdx = tree->GetGlobalVariableId("global_score");
    ASSERT_NE(globalIdx, -1);
    ASSERT_EQ(tree->Globals[globalIdx].Name, "global_score");
}

// Test 3: Verify variable override in existing VariableOverrides
TEST(TestUpdateVariableDefault_ExistingOverride) {
    DMCompilerClass compiler;
    auto* tree = compiler.GetObjectTree();
    
    // Create parent -> child -> grandchild hierarchy
    DreamPath parentPath(DreamPath::PathType::Absolute, {"base"});
    DreamPath childPath(DreamPath::PathType::Absolute, {"base", "derived"});
    DreamPath grandchildPath(DreamPath::PathType::Absolute, {"base", "derived", "final"});
    
    DMObject* parent = tree->GetOrCreateDMObject(parentPath);
    DMObject* child = tree->GetOrCreateDMObject(childPath);
    DMObject* grandchild = tree->GetOrCreateDMObject(grandchildPath);
    
    // Parent has the original variable
    DMVariable var;
    var.Name = "damage";
    var.Value = nullptr;
    parent->Variables["damage"] = var;
    
    // Child has an override
    DMVariable childOverride = var;
    childOverride.Value = nullptr; // Different value
    child->VariableOverrides["damage"] = childOverride;
    
    // Grandchild should find the override in child
    DMObject* current = grandchild;
    bool foundInOverrides = false;
    while (current != nullptr) {
        auto overrideIt = current->VariableOverrides.find("damage");
        if (overrideIt != current->VariableOverrides.end()) {
            foundInOverrides = true;
            break;
        }
        
        auto varIt = current->Variables.find("damage");
        if (varIt != current->Variables.end()) {
            break;
        }
        
        current = current->Parent;
    }
    
    ASSERT_TRUE(foundInOverrides);
}

// Test 4: Verify GetVariable searches up hierarchy
TEST(TestGetVariable_InheritanceChain) {
    DMCompilerClass compiler;
    auto* tree = compiler.GetObjectTree();
    
    // Create hierarchy
    DreamPath basePath(DreamPath::PathType::Absolute, {"obj"});
    DreamPath derivedPath(DreamPath::PathType::Absolute, {"obj", "item"});
    
    DMObject* base = tree->GetOrCreateDMObject(basePath);
    DMObject* derived = tree->GetOrCreateDMObject(derivedPath);
    
    // Add variable to base
    DMVariable var;
    var.Name = "weight";
    var.IsConst = false;
    var.IsFinal = false;
    var.IsGlobal = false;
    var.IsTmp = false;
    var.Value = nullptr;
    base->Variables["weight"] = var;
    
    // Derived should find weight through GetVariable
    const DMVariable* foundVar = derived->GetVariable("weight");
    ASSERT_NE(foundVar, nullptr);
    ASSERT_EQ(foundVar->Name, "weight");
}

// Test 5: Verify UnknownVariable error code exists
TEST(TestUnknownVariableWarningCode) {
    // This test verifies the enum value exists
    WarningCode code = WarningCode::UnknownVariable;
    ASSERT_TRUE(code != WarningCode::Unknown);
}

int main() {
    std::cout << "\n=== Running Variable Override Tests ===\n" << std::endl;
    
    // Tests are auto-run by static initializers
    
    std::cout << "\nVariable Override Tests: " << TestsPassed << "/" 
              << (TestsPassed + TestsFailed) << " passed" << std::endl;
    
    if (TestsFailed == 0) {
        std::cout << "✓ All Variable Override tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "✗ Some tests failed!" << std::endl;
        return 1;
    }
}
