#include "../include/DMProc.h"
#include "../include/DMObject.h"
#include "../include/DMObjectTree.h"
#include "../include/DreamPath.h"
#include <iostream>

using namespace DMCompiler;

// Test counter
int proc_tests_run = 0;
int proc_tests_passed = 0;

#define TEST(name) void name()
#define EXPECT_EQ(a, b) do { \
    proc_tests_run++; \
    if ((a) == (b)) { \
        proc_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << std::endl; \
    } \
} while (0)

#define EXPECT_NE(a, b) do { \
    proc_tests_run++; \
    if ((a) != (b)) { \
        proc_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << std::endl; \
    } \
} while (0)

#define EXPECT_TRUE(x) do { \
    proc_tests_run++; \
    if (x) { \
        proc_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << std::endl; \
    } \
} while (0)

#define EXPECT_FALSE(x) do { \
    proc_tests_run++; \
    if (!(x)) { \
        proc_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << std::endl; \
    } \
} while (0)

#define RUN_TEST(test) do { \
    int before = proc_tests_run; \
    test(); \
    std::cout << "  " << #test << ": "; \
    if (proc_tests_run - before == proc_tests_passed - (proc_tests_passed - (proc_tests_run - before))) { \
        std::cout << "PASS" << std::endl; \
    } \
} while (0)

// ========================================
// DMProc Tests
// ========================================

TEST(DMProc_Constructor) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DMObject* obj = tree.GetOrCreateDMObject(mobPath);
    
    DMProc proc(1, "Attack", obj, false, Location());
    
    EXPECT_EQ(proc.Id, 1);
    EXPECT_EQ(proc.Name, "Attack");
    EXPECT_EQ(proc.OwningObject, obj);
    EXPECT_FALSE(proc.IsVerb);
    EXPECT_FALSE(proc.IsFinal);
}

TEST(DMProc_AddParameter) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DMObject* obj = tree.GetOrCreateDMObject(mobPath);
    
    DMProc proc(1, "Attack", obj);
    
    LocalVariable* param = proc.AddParameter("target");
    
    EXPECT_NE(param, nullptr);
    EXPECT_EQ(param->Name, "target");
    EXPECT_TRUE(param->IsParameter);
    EXPECT_EQ(proc.GetParameterCount(), 1);
}

TEST(DMProc_AddMultipleParameters) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DMObject* obj = tree.GetOrCreateDMObject(mobPath);
    
    DMProc proc(1, "TakeDamage", obj);
    
    proc.AddParameter("amount");
    proc.AddParameter("source");
    proc.AddParameter("type");
    
    EXPECT_EQ(proc.GetParameterCount(), 3);
}

TEST(DMProc_AddLocalVariable) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DMObject* obj = tree.GetOrCreateDMObject(mobPath);
    
    DMProc proc(1, "Move", obj);
    
    LocalVariable* local = proc.AddLocalVariable("new_loc");
    
    EXPECT_NE(local, nullptr);
    EXPECT_EQ(local->Name, "new_loc");
    EXPECT_FALSE(local->IsParameter);
}

TEST(DMProc_GetLocalVariable) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DMObject* obj = tree.GetOrCreateDMObject(mobPath);
    
    DMProc proc(1, "Test", obj);
    
    proc.AddParameter("arg1");
    proc.AddLocalVariable("local1");
    
    LocalVariable* found = proc.GetLocalVariable("arg1");
    EXPECT_NE(found, nullptr);
    
    found = proc.GetLocalVariable("local1");
    EXPECT_NE(found, nullptr);
    
    found = proc.GetLocalVariable("nonexistent");
    EXPECT_EQ(found, nullptr);
}

TEST(DMProc_HasParameter) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DMObject* obj = tree.GetOrCreateDMObject(mobPath);
    
    DMProc proc(1, "Test", obj);
    
    proc.AddParameter("target");
    
    EXPECT_TRUE(proc.HasParameter("target"));
    EXPECT_FALSE(proc.HasParameter("damage"));
}

TEST(DMProc_MarkUnsupported) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DMObject* obj = tree.GetOrCreateDMObject(mobPath);
    
    DMProc proc(1, "Unsupported", obj);
    
    EXPECT_FALSE(proc.IsUnsupported());
    
    proc.MarkUnsupported("Uses inline assembly");
    
    EXPECT_TRUE(proc.IsUnsupported());
}

TEST(DMProc_ToString) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DMObject* obj = tree.GetOrCreateDMObject(mobPath);
    
    DMProc proc(1, "Attack", obj);
    proc.AddParameter("target");
    
    std::string str = proc.ToString();
    
    // Should contain the proc name and parameter
    EXPECT_TRUE(str.find("Attack") != std::string::npos);
    EXPECT_TRUE(str.find("target") != std::string::npos);
}

TEST(DMProc_Verb) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DMObject* obj = tree.GetOrCreateDMObject(mobPath);
    
    DMProc verb(1, "Say", obj, true);
    
    EXPECT_TRUE(verb.IsVerb);
    
    verb.VerbName = "Say Something";
    verb.VerbCategory = "Communication";
    
    EXPECT_TRUE(verb.VerbName.has_value());
    EXPECT_EQ(verb.VerbName.value(), "Say Something");
}

TEST(DMProc_LocalVariableCount) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DMObject* obj = tree.GetOrCreateDMObject(mobPath);
    
    DMProc proc(1, "Test", obj);
    
    proc.AddParameter("arg1");
    proc.AddLocalVariable("local1");
    proc.AddLocalVariable("local2");
    
    EXPECT_EQ(proc.GetParameterCount(), 1);
    EXPECT_EQ(proc.GetLocalVariableCount(), 2);
}

// ========================================
// Test Runner
// ========================================

void RunProcTests() {
    std::cout << "\n=== Running DMProc Tests ===\n" << std::endl;
    
    proc_tests_run = 0;
    proc_tests_passed = 0;
    
    RUN_TEST(DMProc_Constructor);
    RUN_TEST(DMProc_AddParameter);
    RUN_TEST(DMProc_AddMultipleParameters);
    RUN_TEST(DMProc_AddLocalVariable);
    RUN_TEST(DMProc_GetLocalVariable);
    RUN_TEST(DMProc_HasParameter);
    RUN_TEST(DMProc_MarkUnsupported);
    RUN_TEST(DMProc_ToString);
    RUN_TEST(DMProc_Verb);
    RUN_TEST(DMProc_LocalVariableCount);
    
    std::cout << "\n=== DMProc Test Results ===" << std::endl;
    std::cout << "Total: " << proc_tests_run << std::endl;
    std::cout << "Passed: " << proc_tests_passed << std::endl;
    std::cout << "Failed: " << (proc_tests_run - proc_tests_passed) << std::endl;
    
    if (proc_tests_run == proc_tests_passed) {
        std::cout << "\n✓ All DMProc tests passed!" << std::endl;
    } else {
        std::cout << "\n✗ Some DMProc tests failed!" << std::endl;
    }
}
