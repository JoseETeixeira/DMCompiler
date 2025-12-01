#include "../include/DMProc.h"
#include "../include/DMObject.h"
#include "../include/DMObjectTree.h"
#include "../include/DMVariable.h"
#include "../include/DreamPath.h"
#include <iostream>
#include <cassert>

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
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected " << (b) << " but got " << (a) << std::endl; \
    } \
} while (0)

#define EXPECT_NE(a, b) do { \
    proc_tests_run++; \
    if ((a) != (b)) { \
        proc_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected values to be different" << std::endl; \
    } \
} while (0)

#define EXPECT_TRUE(x) do { \
    proc_tests_run++; \
    if (x) { \
        proc_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected true" << std::endl; \
    } \
} while (0)

#define EXPECT_FALSE(x) do { \
    proc_tests_run++; \
    if (!(x)) { \
        proc_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected false" << std::endl; \
    } \
} while (0)

#define RUN_TEST(test) do { \
    test(); \
    std::cout << "  " << #test << ": "; \
    if (proc_tests_run == proc_tests_passed) { \
        std::cout << "PASS" << std::endl; \
    } else { \
        std::cout << "FAIL (" << proc_tests_passed << "/" << proc_tests_run << ")" << std::endl; \
    } \
} while (0)

// ========================================
// LocalVariable Tests
// ========================================

TEST(DMProc_LocalVariable_Constructor) {
    LocalVariable var("health", 0, true, std::nullopt, std::nullopt);
    
    EXPECT_EQ(var.Name, "health");
    EXPECT_EQ(var.Id, 0);
    EXPECT_TRUE(var.IsParameter);
    EXPECT_FALSE(var.Type.has_value());
    EXPECT_FALSE(var.ExplicitValueType.has_value());
}

TEST(DMProc_LocalVariable_WithType) {
    DreamPath mobPath = DreamPath::Parse("/mob");
    LocalVariable var("target", 1, false, mobPath, std::nullopt);
    
    EXPECT_EQ(var.Name, "target");
    EXPECT_EQ(var.Id, 1);
    EXPECT_FALSE(var.IsParameter);
    EXPECT_TRUE(var.Type.has_value());
    EXPECT_EQ(var.Type->ToString(), "/mob");
}

TEST(DMProc_LocalConstVariable_Constructor) {
    DreamPath numPath = DreamPath::Parse("/num");
    void* value = reinterpret_cast<void*>(42);
    
    LocalConstVariable constVar("MAX_HEALTH", 2, numPath, value);
    
    EXPECT_EQ(constVar.Name, "MAX_HEALTH");
    EXPECT_EQ(constVar.Id, 2);
    EXPECT_FALSE(constVar.IsParameter); // Const vars are not parameters
    EXPECT_TRUE(constVar.Type.has_value());
    EXPECT_EQ(constVar.ConstValue, value);
}

// ========================================
// DMProc Basic Tests
// ========================================

TEST(DMProc_Constructor_BasicProc) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    
    DMProc proc(1, "New", obj, false, Location());
    
    EXPECT_EQ(proc.Id, 1);
    EXPECT_EQ(proc.Name, "New");
    EXPECT_EQ(proc.OwningObject, obj);
    EXPECT_FALSE(proc.IsVerb);
    EXPECT_FALSE(proc.IsFinal);
    EXPECT_EQ(proc.Attributes, ProcAttributes::None);
    EXPECT_EQ(proc.GetParameterCount(), 0);
    EXPECT_FALSE(proc.IsUnsupported());
}

TEST(DMProc_Constructor_Verb) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    
    DMProc verb(2, "Attack", obj, true, Location());
    
    EXPECT_EQ(verb.Id, 2);
    EXPECT_EQ(verb.Name, "Attack");
    EXPECT_TRUE(verb.IsVerb);
    EXPECT_FALSE(verb.VerbName.has_value());
    EXPECT_FALSE(verb.VerbSource.has_value());
    EXPECT_FALSE(verb.VerbCategory.has_value());
}

// ========================================
// Parameter Tests
// ========================================

TEST(DMProc_AddParameter_Simple) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Attack", obj, false, Location());
    
    LocalVariable* param = proc.AddParameter("target");
    
    EXPECT_NE(param, nullptr);
    EXPECT_EQ(param->Name, "target");
    EXPECT_EQ(param->Id, 0); // First local var ID
    EXPECT_TRUE(param->IsParameter);
    EXPECT_EQ(proc.GetParameterCount(), 1);
}

TEST(DMProc_AddParameter_WithType) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Attack", obj, false, Location());
    
    DreamPath mobPath = DreamPath::Parse("/mob");
    LocalVariable* param = proc.AddParameter("target", mobPath);
    
    EXPECT_NE(param, nullptr);
    EXPECT_TRUE(param->Type.has_value());
    EXPECT_EQ(param->Type->ToString(), "/mob");
}

TEST(DMProc_AddParameter_Multiple) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Attack", obj, false, Location());
    
    proc.AddParameter("target");
    proc.AddParameter("damage");
    proc.AddParameter("weapon");
    
    EXPECT_EQ(proc.GetParameterCount(), 3);
    EXPECT_EQ(proc.Parameters[0], "target");
    EXPECT_EQ(proc.Parameters[1], "damage");
    EXPECT_EQ(proc.Parameters[2], "weapon");
}

TEST(DMProc_AddParameter_Duplicate) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Attack", obj, false, Location());
    
    LocalVariable* param1 = proc.AddParameter("target");
    LocalVariable* param2 = proc.AddParameter("target"); // Duplicate
    
    EXPECT_NE(param1, nullptr);
    EXPECT_EQ(param2, nullptr); // Should fail
    EXPECT_EQ(proc.GetParameterCount(), 1); // Only one added
}

TEST(DMProc_HasParameter_True) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Attack", obj, false, Location());
    
    proc.AddParameter("target");
    
    EXPECT_TRUE(proc.HasParameter("target"));
}

TEST(DMProc_HasParameter_False) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Attack", obj, false, Location());
    
    proc.AddParameter("target");
    
    EXPECT_FALSE(proc.HasParameter("damage"));
}

// ========================================
// Local Variable Tests
// ========================================

TEST(DMProc_AddLocalVariable_Simple) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Move", obj, false, Location());
    
    LocalVariable* local = proc.AddLocalVariable("new_loc");
    
    EXPECT_NE(local, nullptr);
    EXPECT_EQ(local->Name, "new_loc");
    EXPECT_FALSE(local->IsParameter);
    EXPECT_EQ(proc.GetLocalVariableCount(), 1);
}

TEST(DMProc_AddLocalVariable_WithType) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Move", obj, false, Location());
    
    DreamPath turfPath = DreamPath::Parse("/turf");
    LocalVariable* local = proc.AddLocalVariable("new_loc", turfPath);
    
    EXPECT_NE(local, nullptr);
    EXPECT_TRUE(local->Type.has_value());
    EXPECT_EQ(local->Type->ToString(), "/turf");
}

TEST(DMProc_AddLocalVariable_Multiple) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Complex", obj, false, Location());
    
    proc.AddParameter("arg1"); // Parameter
    proc.AddLocalVariable("local1");
    proc.AddLocalVariable("local2");
    proc.AddLocalVariable("local3");
    
    EXPECT_EQ(proc.GetParameterCount(), 1);
    EXPECT_EQ(proc.GetLocalVariableCount(), 3); // Only local vars, not params
}

TEST(DMProc_AddLocalVariable_Duplicate) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Test", obj, false, Location());
    
    LocalVariable* local1 = proc.AddLocalVariable("temp");
    LocalVariable* local2 = proc.AddLocalVariable("temp"); // Duplicate
    
    EXPECT_NE(local1, nullptr);
    EXPECT_EQ(local2, nullptr); // Should fail
}

TEST(DMProc_AddLocalConst_Simple) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Test", obj, false, Location());
    
    void* value = reinterpret_cast<void*>(100);
    LocalConstVariable* constVar = proc.AddLocalConst("MAX", std::nullopt, value);
    
    EXPECT_NE(constVar, nullptr);
    EXPECT_EQ(constVar->Name, "MAX");
    EXPECT_FALSE(constVar->IsParameter);
    EXPECT_EQ(constVar->ConstValue, value);
}

// ========================================
// GetLocalVariable Tests
// ========================================

TEST(DMProc_GetLocalVariable_Parameter) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Attack", obj, false, Location());
    
    proc.AddParameter("target");
    
    LocalVariable* var = proc.GetLocalVariable("target");
    EXPECT_NE(var, nullptr);
    EXPECT_EQ(var->Name, "target");
    EXPECT_TRUE(var->IsParameter);
}

TEST(DMProc_GetLocalVariable_Local) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Move", obj, false, Location());
    
    proc.AddLocalVariable("new_loc");
    
    LocalVariable* var = proc.GetLocalVariable("new_loc");
    EXPECT_NE(var, nullptr);
    EXPECT_EQ(var->Name, "new_loc");
    EXPECT_FALSE(var->IsParameter);
}

TEST(DMProc_GetLocalVariable_NotFound) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Test", obj, false, Location());
    
    LocalVariable* var = proc.GetLocalVariable("nonexistent");
    EXPECT_EQ(var, nullptr);
}

TEST(DMProc_GetLocalVariable_Const) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Test", obj, false, Location());
    
    void* value = reinterpret_cast<void*>(42);
    proc.AddLocalConst("MAX", std::nullopt, value);
    
    const LocalVariable* var = proc.GetLocalVariable("MAX");
    EXPECT_NE(var, nullptr);
    
    // Can cast to LocalConstVariable
    const LocalConstVariable* constVar = dynamic_cast<const LocalConstVariable*>(var);
    EXPECT_NE(constVar, nullptr);
    EXPECT_EQ(constVar->ConstValue, value);
}

// ========================================
// Attributes Tests
// ========================================

TEST(DMProc_Attributes_SetFlags) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Background", obj, false, Location());
    
    proc.Attributes = ProcAttributes::Hidden | ProcAttributes::Background;
    
    EXPECT_TRUE(HasFlag(proc.Attributes, ProcAttributes::Hidden));
    EXPECT_TRUE(HasFlag(proc.Attributes, ProcAttributes::Background));
    EXPECT_FALSE(HasFlag(proc.Attributes, ProcAttributes::Instant));
}

TEST(DMProc_Attributes_IsFinal) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Final", obj, false, Location());
    
    proc.IsFinal = true;
    
    EXPECT_TRUE(proc.IsFinal);
}

// ========================================
// Unsupported Tests
// ========================================

TEST(DMProc_MarkUnsupported) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Unsupported", obj, false, Location());
    
    EXPECT_FALSE(proc.IsUnsupported());
    
    proc.MarkUnsupported("Uses inline assembly");
    
    EXPECT_TRUE(proc.IsUnsupported());
    EXPECT_TRUE(proc.UnsupportedReason.has_value());
    EXPECT_EQ(proc.UnsupportedReason.value(), "Uses inline assembly");
}

// ========================================
// ToString Tests
// ========================================

TEST(DMProc_ToString_NoParameters) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "New", obj, false, Location());
    
    std::string str = proc.ToString();
    
    EXPECT_EQ(str, "/mob/proc/New()");
}

TEST(DMProc_ToString_WithParameters) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Attack", obj, false, Location());
    
    proc.AddParameter("target");
    proc.AddParameter("damage");
    
    std::string str = proc.ToString();
    
    EXPECT_EQ(str, "/mob/proc/Attack(target, damage)");
}

TEST(DMProc_ToString_Verb) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc verb(1, "Say", obj, true, Location());
    
    verb.AddParameter("message");
    
    std::string str = verb.ToString();
    
    EXPECT_EQ(str, "/mob/verb/Say(message)");
}

TEST(DMProc_ToString_WithTypes) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Attack", obj, false, Location());
    
    DreamPath mobPath = DreamPath::Parse("/mob");
    DreamPath numPath = DreamPath::Parse("/num");
    
    proc.AddParameter("target", mobPath);
    proc.AddParameter("damage", numPath);
    
    std::string str = proc.ToString();
    
    EXPECT_EQ(str, "/mob/proc/Attack(target as /mob, damage as /num)");
}

// ========================================
// Verb-Specific Tests
// ========================================

TEST(DMProc_Verb_Metadata) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc verb(1, "attack", obj, true, Location());
    
    verb.VerbName = "Attack";
    verb.VerbCategory = "Combat";
    verb.VerbDescription = "Attack a target";
    verb.Invisibility = 0;
    
    EXPECT_TRUE(verb.VerbName.has_value());
    EXPECT_EQ(verb.VerbName.value(), "Attack");
    EXPECT_EQ(verb.VerbCategory.value(), "Combat");
    EXPECT_EQ(verb.VerbDescription.value(), "Attack a target");
}

// ========================================
// Complex Integration Tests
// ========================================

TEST(DMProc_Integration_ComplexProc) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob/living"));
    DMProc proc(1, "TakeDamage", obj, false, Location());
    
    // Add parameters
    DreamPath numPath = DreamPath::Parse("/num");
    DreamPath mobPath = DreamPath::Parse("/mob");
    
    proc.AddParameter("amount", numPath);
    proc.AddParameter("source", mobPath);
    proc.AddParameter("damage_type");
    
    // Add local variables
    proc.AddLocalVariable("new_health", numPath);
    proc.AddLocalVariable("is_dead");
    
    // Add const
    void* maxValue = reinterpret_cast<void*>(100);
    proc.AddLocalConst("MAX_HEALTH", numPath, maxValue);
    
    // Verify counts
    EXPECT_EQ(proc.GetParameterCount(), 3);
    EXPECT_EQ(proc.GetLocalVariableCount(), 3); // 2 locals + 1 const
    
    // Verify lookup
    EXPECT_NE(proc.GetLocalVariable("amount"), nullptr);
    EXPECT_NE(proc.GetLocalVariable("new_health"), nullptr);
    EXPECT_NE(proc.GetLocalVariable("MAX_HEALTH"), nullptr);
    EXPECT_EQ(proc.GetLocalVariable("nonexistent"), nullptr);
    
    // Verify ToString
    std::string expected = "/mob/living/proc/TakeDamage(amount as /num, source as /mob, damage_type)";
    EXPECT_EQ(proc.ToString(), expected);
}

TEST(DMProc_Integration_LocalVariableIds) {
    DMObjectTree tree;
    DMObject* obj = tree.GetOrCreateObject(DreamPath::Parse("/mob"));
    DMProc proc(1, "Test", obj, false, Location());
    
    auto* param1 = proc.AddParameter("a");
    auto* param2 = proc.AddParameter("b");
    auto* local1 = proc.AddLocalVariable("x");
    auto* local2 = proc.AddLocalVariable("y");
    
    // IDs should increment sequentially
    EXPECT_EQ(param1->Id, 0);
    EXPECT_EQ(param2->Id, 1);
    EXPECT_EQ(local1->Id, 2);
    EXPECT_EQ(local2->Id, 3);
}

// ========================================
// Test Runner
// ========================================

void RunProcTests() {
    std::cout << "\n=== Running DMProc Tests ===\n" << std::endl;
    
    proc_tests_run = 0;
    proc_tests_passed = 0;
    
    // LocalVariable tests
    RUN_TEST(DMProc_LocalVariable_Constructor);
    RUN_TEST(DMProc_LocalVariable_WithType);
    RUN_TEST(DMProc_LocalConstVariable_Constructor);
    
    // Basic DMProc tests
    RUN_TEST(DMProc_Constructor_BasicProc);
    RUN_TEST(DMProc_Constructor_Verb);
    
    // Parameter tests
    RUN_TEST(DMProc_AddParameter_Simple);
    RUN_TEST(DMProc_AddParameter_WithType);
    RUN_TEST(DMProc_AddParameter_Multiple);
    RUN_TEST(DMProc_AddParameter_Duplicate);
    RUN_TEST(DMProc_HasParameter_True);
    RUN_TEST(DMProc_HasParameter_False);
    
    // Local variable tests
    RUN_TEST(DMProc_AddLocalVariable_Simple);
    RUN_TEST(DMProc_AddLocalVariable_WithType);
    RUN_TEST(DMProc_AddLocalVariable_Multiple);
    RUN_TEST(DMProc_AddLocalVariable_Duplicate);
    RUN_TEST(DMProc_AddLocalConst_Simple);
    
    // GetLocalVariable tests
    RUN_TEST(DMProc_GetLocalVariable_Parameter);
    RUN_TEST(DMProc_GetLocalVariable_Local);
    RUN_TEST(DMProc_GetLocalVariable_NotFound);
    RUN_TEST(DMProc_GetLocalVariable_Const);
    
    // Attributes tests
    RUN_TEST(DMProc_Attributes_SetFlags);
    RUN_TEST(DMProc_Attributes_IsFinal);
    
    // Unsupported tests
    RUN_TEST(DMProc_MarkUnsupported);
    
    // ToString tests
    RUN_TEST(DMProc_ToString_NoParameters);
    RUN_TEST(DMProc_ToString_WithParameters);
    RUN_TEST(DMProc_ToString_Verb);
    RUN_TEST(DMProc_ToString_WithTypes);
    
    // Verb tests
    RUN_TEST(DMProc_Verb_Metadata);
    
    // Integration tests
    RUN_TEST(DMProc_Integration_ComplexProc);
    RUN_TEST(DMProc_Integration_LocalVariableIds);
    
    int total_tests = proc_tests_run;
    int passed_tests = proc_tests_passed;
    int failed_tests = total_tests - passed_tests;
    
    std::cout << "\n=== DMProc Test Results ===" << std::endl;
    std::cout << "Total: " << total_tests << std::endl;
    std::cout << "Passed: " << passed_tests << std::endl;
    std::cout << "Failed: " << failed_tests << std::endl;
    
    if (failed_tests == 0) {
        std::cout << "\n✓ All DMProc tests passed!" << std::endl;
    } else {
        std::cout << "\n✗ Some DMProc tests failed!" << std::endl;
    }
}
