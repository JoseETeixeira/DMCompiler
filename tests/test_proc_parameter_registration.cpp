#include "../include/DMCompiler.h"
#include "../include/DMObjectTree.h"
#include "../include/DMProc.h"
#include "../include/DMObject.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cassert>

// Test counter
int param_reg_tests_run = 0;
int param_reg_tests_passed = 0;

#define TEST(name) void name()
#define EXPECT_EQ(a, b) do { \
    param_reg_tests_run++; \
    if ((a) == (b)) { \
        param_reg_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected " << (b) << " but got " << (a) << std::endl; \
    } \
} while (0)

#define EXPECT_NE(a, b) do { \
    param_reg_tests_run++; \
    if ((a) != (b)) { \
        param_reg_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected values to be different" << std::endl; \
    } \
} while (0)

#define EXPECT_TRUE(x) do { \
    param_reg_tests_run++; \
    if (x) { \
        param_reg_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected true" << std::endl; \
    } \
} while (0)

#define EXPECT_FALSE(x) do { \
    param_reg_tests_run++; \
    if (!(x)) { \
        param_reg_tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected false" << std::endl; \
    } \
} while (0)

#define ASSERT_NE(a, b) do { \
    if ((a) == (b)) { \
        std::cout << "FATAL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Assertion failed" << std::endl; \
        return; \
    } \
} while (0)

#define RUN_TEST(test) do { \
    int before_run = param_reg_tests_run; \
    int before_passed = param_reg_tests_passed; \
    test(); \
    std::cout << "  " << #test << ": "; \
    if (param_reg_tests_run == param_reg_tests_passed) { \
        std::cout << "PASS" << std::endl; \
    } else { \
        int failed = (param_reg_tests_run - before_run) - (param_reg_tests_passed - before_passed); \
        std::cout << "FAIL (" << failed << " failures)" << std::endl; \
    } \
} while (0)

// Global compiler instance to keep objects alive
// This is necessary because the returned DMProc* pointer must remain valid
// after the function returns
static std::unique_ptr<DMCompiler::DMCompiler> g_test_compiler;

// Helper function to compile DM code and get the proc
DMCompiler::DMProc* CompileAndGetProc(const std::string& code, const std::string& procPath) {
    // Create a temporary test file
    std::string testFile = "test_param_reg_temp.dme";
    std::ofstream out(testFile);
    out << code;
    out.close();
    
    // Configure compiler settings
    DMCompiler::DMCompilerSettings settings;
    settings.Files.push_back(testFile);
    settings.Verbose = false;
    settings.NoStandard = true;  // Don't load DMStandard for these simple tests
    
    // Create compiler instance (replace any previous one)
    g_test_compiler = std::make_unique<DMCompiler::DMCompiler>();
    
    // Compile
    bool compileResult = g_test_compiler->Compile(settings);
    
    // Clean up test file
    std::filesystem::remove(testFile);
    std::filesystem::remove("test_param_reg_temp.json");
    
    if (!compileResult) {
        std::cout << "ERROR: Compilation failed" << std::endl;
        return nullptr;
    }
    
    // Get the proc from the object tree
    DMCompiler::DMObjectTree* tree = g_test_compiler->GetObjectTree();
    if (!tree) {
        std::cout << "ERROR: No object tree" << std::endl;
        return nullptr;
    }
    
    // Parse the proc path to get proc name
    // Format: "/mob/proc/test" or "/proc/test"
    // We need to find "/proc/" or "/verb/" and split there
    size_t procPos = procPath.find("/proc/");
    size_t verbPos = procPath.find("/verb/");
    
    std::string typePath;
    std::string procName;
    
    if (procPos != std::string::npos) {
        typePath = procPath.substr(0, procPos);
        procName = procPath.substr(procPos + 6);  // Skip "/proc/"
    } else if (verbPos != std::string::npos) {
        typePath = procPath.substr(0, verbPos);
        procName = procPath.substr(verbPos + 6);  // Skip "/verb/"
    } else {
        std::cout << "ERROR: Invalid proc path (must contain /proc/ or /verb/)" << std::endl;
        return nullptr;
    }
    
    // Get the object
    DMCompiler::DMObject* obj = nullptr;
    DMCompiler::DreamPath path(DMCompiler::DreamPath::PathType::Absolute, {});
    
    // Parse the type path
    if (typePath.empty() || typePath == "/") {
        path = DMCompiler::DreamPath::Root;
    } else {
        // Remove leading slash if present
        if (typePath[0] == '/') {
            typePath = typePath.substr(1);
        }
        
        // Split by '/'
        std::vector<std::string> parts;
        size_t start = 0;
        size_t end = typePath.find('/');
        while (end != std::string::npos) {
            parts.push_back(typePath.substr(start, end - start));
            start = end + 1;
            end = typePath.find('/', start);
        }
        parts.push_back(typePath.substr(start));
        
        path = DMCompiler::DreamPath(DMCompiler::DreamPath::PathType::Absolute, parts);
    }
    
    if (!tree->TryGetDMObject(path, &obj)) {
        std::cout << "ERROR: Type not found: " << typePath << std::endl;
        return nullptr;
    }
    
    // Get the proc
    const std::vector<int>* procIds = obj->GetProcs(procName);
    if (!procIds || procIds->empty()) {
        std::cout << "ERROR: Proc not found: " << procName << std::endl;
        return nullptr;
    }
    
    int procId = (*procIds)[0];
    if (procId < 0 || procId >= static_cast<int>(tree->AllProcs.size())) {
        std::cout << "ERROR: Invalid proc ID: " << procId << std::endl;
        return nullptr;
    }
    
    return tree->AllProcs[procId].get();
}

// ========================================
// Basic Parameter Registration Tests
// ========================================

TEST(ProcParameterRegistration_NoParameters) {
    std::string code = R"(
/mob/proc/test()
    return 1
)";
    
    DMCompiler::DMProc* proc = CompileAndGetProc(code, "/mob/proc/test");
    ASSERT_NE(proc, nullptr);
    
    EXPECT_EQ(proc->GetParameterCount(), 0);
}

TEST(ProcParameterRegistration_SingleParameter) {
    std::string code = R"(
/mob/proc/test(a)
    return a
)";
    
    DMCompiler::DMProc* proc = CompileAndGetProc(code, "/mob/proc/test");
    ASSERT_NE(proc, nullptr);
    
    EXPECT_EQ(proc->GetParameterCount(), 1);
    
    DMCompiler::LocalVariable* param = proc->GetLocalVariable("a");
    EXPECT_NE(param, nullptr);
    EXPECT_TRUE(param->IsParameter);
    EXPECT_EQ(param->Name, "a");
}

TEST(ProcParameterRegistration_MultipleParameters) {
    std::string code = R"(
/mob/proc/test(a, b, c)
    return a + b + c
)";
    
    DMCompiler::DMProc* proc = CompileAndGetProc(code, "/mob/proc/test");
    ASSERT_NE(proc, nullptr);
    
    EXPECT_EQ(proc->GetParameterCount(), 3);
    
    DMCompiler::LocalVariable* paramA = proc->GetLocalVariable("a");
    EXPECT_NE(paramA, nullptr);
    EXPECT_TRUE(paramA->IsParameter);
    
    DMCompiler::LocalVariable* paramB = proc->GetLocalVariable("b");
    EXPECT_NE(paramB, nullptr);
    EXPECT_TRUE(paramB->IsParameter);
    
    DMCompiler::LocalVariable* paramC = proc->GetLocalVariable("c");
    EXPECT_NE(paramC, nullptr);
    EXPECT_TRUE(paramC->IsParameter);
}

TEST(ProcParameterRegistration_ParameterOrder) {
    std::string code = R"(
/mob/proc/test(first, second, third)
    return 1
)";
    
    DMCompiler::DMProc* proc = CompileAndGetProc(code, "/mob/proc/test");
    ASSERT_NE(proc, nullptr);
    
    EXPECT_EQ(proc->GetParameterCount(), 3);
    EXPECT_EQ(proc->Parameters[0], "first");
    EXPECT_EQ(proc->Parameters[1], "second");
    EXPECT_EQ(proc->Parameters[2], "third");
}

// ========================================
// Parameter Type Tests
// ========================================

TEST(ProcParameterRegistration_TypedParameter) {
    std::string code = R"(
/mob/proc/test(mob/target)
    return target
)";
    
    DMCompiler::DMProc* proc = CompileAndGetProc(code, "/mob/proc/test");
    ASSERT_NE(proc, nullptr);
    
    EXPECT_EQ(proc->GetParameterCount(), 1);
    
    DMCompiler::LocalVariable* param = proc->GetLocalVariable("target");
    EXPECT_NE(param, nullptr);
    EXPECT_TRUE(param->IsParameter);
    EXPECT_TRUE(param->Type.has_value());
    EXPECT_EQ(param->Type->ToString(), "/mob");
}

TEST(ProcParameterRegistration_MixedTypedParameters) {
    std::string code = R"(
/mob/proc/attack(mob/target, damage, obj/weapon)
    return 1
)";
    
    DMCompiler::DMProc* proc = CompileAndGetProc(code, "/mob/proc/attack");
    ASSERT_NE(proc, nullptr);
    
    EXPECT_EQ(proc->GetParameterCount(), 3);
    
    DMCompiler::LocalVariable* target = proc->GetLocalVariable("target");
    EXPECT_NE(target, nullptr);
    EXPECT_TRUE(target->Type.has_value());
    EXPECT_EQ(target->Type->ToString(), "/mob");
    
    DMCompiler::LocalVariable* damage = proc->GetLocalVariable("damage");
    EXPECT_NE(damage, nullptr);
    EXPECT_FALSE(damage->Type.has_value());
    
    DMCompiler::LocalVariable* weapon = proc->GetLocalVariable("weapon");
    EXPECT_NE(weapon, nullptr);
    EXPECT_TRUE(weapon->Type.has_value());
    EXPECT_EQ(weapon->Type->ToString(), "/obj");
}

// ========================================
// Parameter with Default Values Tests
// ========================================

TEST(ProcParameterRegistration_ParameterWithDefault) {
    std::string code = R"(
/mob/proc/test(a = 10)
    return a
)";
    
    DMCompiler::DMProc* proc = CompileAndGetProc(code, "/mob/proc/test");
    ASSERT_NE(proc, nullptr);
    
    EXPECT_EQ(proc->GetParameterCount(), 1);
    
    DMCompiler::LocalVariable* param = proc->GetLocalVariable("a");
    EXPECT_NE(param, nullptr);
    EXPECT_TRUE(param->IsParameter);
}

TEST(ProcParameterRegistration_MixedDefaultParameters) {
    std::string code = R"(
/mob/proc/test(a, b = 5, c = 10)
    return a + b + c
)";
    
    DMCompiler::DMProc* proc = CompileAndGetProc(code, "/mob/proc/test");
    ASSERT_NE(proc, nullptr);
    
    EXPECT_EQ(proc->GetParameterCount(), 3);
    
    EXPECT_NE(proc->GetLocalVariable("a"), nullptr);
    EXPECT_NE(proc->GetLocalVariable("b"), nullptr);
    EXPECT_NE(proc->GetLocalVariable("c"), nullptr);
}

// ========================================
// IsParameter Flag Tests
// ========================================

TEST(ProcParameterRegistration_IsParameterFlag) {
    std::string code = R"(
/mob/proc/test(param1, param2)
    var/local1 = 1
    var/local2 = 2
    return param1 + param2 + local1 + local2
)";
    
    DMCompiler::DMProc* proc = CompileAndGetProc(code, "/mob/proc/test");
    ASSERT_NE(proc, nullptr);
    
    // Parameters should have IsParameter = true
    DMCompiler::LocalVariable* param1 = proc->GetLocalVariable("param1");
    EXPECT_NE(param1, nullptr);
    EXPECT_TRUE(param1->IsParameter);
    
    DMCompiler::LocalVariable* param2 = proc->GetLocalVariable("param2");
    EXPECT_NE(param2, nullptr);
    EXPECT_TRUE(param2->IsParameter);
    
    // Note: Local variables are added during compilation, not during parsing
    // So we can't test local1 and local2 here unless we trigger full compilation
}

// ========================================
// Real-World Example Tests
// ========================================

TEST(ProcParameterRegistration_HeapsortExample) {
    // Simplified version of heapsort proc signature
    std::string code = R"(
/proc/heapsort(list/L, cmp, start, end)
    return L
)";
    
    DMCompiler::DMProc* proc = CompileAndGetProc(code, "/proc/heapsort");
    ASSERT_NE(proc, nullptr);
    
    EXPECT_EQ(proc->GetParameterCount(), 4);
    
    // Verify all parameters are registered
    EXPECT_NE(proc->GetLocalVariable("L"), nullptr);
    EXPECT_NE(proc->GetLocalVariable("cmp"), nullptr);
    EXPECT_NE(proc->GetLocalVariable("start"), nullptr);
    EXPECT_NE(proc->GetLocalVariable("end"), nullptr);
    
    // Verify IsParameter flag
    EXPECT_TRUE(proc->GetLocalVariable("L")->IsParameter);
    EXPECT_TRUE(proc->GetLocalVariable("cmp")->IsParameter);
    EXPECT_TRUE(proc->GetLocalVariable("start")->IsParameter);
    EXPECT_TRUE(proc->GetLocalVariable("end")->IsParameter);
}

TEST(ProcParameterRegistration_QuicksortExample) {
    // Simplified version of quicksort proc signature
    std::string code = R"(
/proc/quicksort(list/L, cmp, l, r, upper)
    return L
)";
    
    DMCompiler::DMProc* proc = CompileAndGetProc(code, "/proc/quicksort");
    ASSERT_NE(proc, nullptr);
    
    EXPECT_EQ(proc->GetParameterCount(), 5);
    
    // Verify all parameters are registered
    EXPECT_NE(proc->GetLocalVariable("L"), nullptr);
    EXPECT_NE(proc->GetLocalVariable("cmp"), nullptr);
    EXPECT_NE(proc->GetLocalVariable("l"), nullptr);
    EXPECT_NE(proc->GetLocalVariable("r"), nullptr);
    EXPECT_NE(proc->GetLocalVariable("upper"), nullptr);
    
    // Verify IsParameter flag
    EXPECT_TRUE(proc->GetLocalVariable("L")->IsParameter);
    EXPECT_TRUE(proc->GetLocalVariable("cmp")->IsParameter);
    EXPECT_TRUE(proc->GetLocalVariable("l")->IsParameter);
    EXPECT_TRUE(proc->GetLocalVariable("r")->IsParameter);
    EXPECT_TRUE(proc->GetLocalVariable("upper")->IsParameter);
}

// ========================================
// Edge Cases
// ========================================

TEST(ProcParameterRegistration_LongParameterName) {
    std::string code = R"(
/mob/proc/test(very_long_parameter_name_that_is_descriptive)
    return very_long_parameter_name_that_is_descriptive
)";
    
    DMCompiler::DMProc* proc = CompileAndGetProc(code, "/mob/proc/test");
    ASSERT_NE(proc, nullptr);
    
    EXPECT_EQ(proc->GetParameterCount(), 1);
    
    DMCompiler::LocalVariable* param = proc->GetLocalVariable("very_long_parameter_name_that_is_descriptive");
    EXPECT_NE(param, nullptr);
    EXPECT_TRUE(param->IsParameter);
}

TEST(ProcParameterRegistration_ParameterNamedSrc) {
    // 'src' is a special identifier, but can also be a parameter name
    std::string code = R"(
/proc/test(src)
    return src
)";
    
    DMCompiler::DMProc* proc = CompileAndGetProc(code, "/proc/test");
    ASSERT_NE(proc, nullptr);
    
    EXPECT_EQ(proc->GetParameterCount(), 1);
    
    DMCompiler::LocalVariable* param = proc->GetLocalVariable("src");
    EXPECT_NE(param, nullptr);
    EXPECT_TRUE(param->IsParameter);
}

TEST(ProcParameterRegistration_ManyParameters) {
    std::string code = R"(
/mob/proc/test(a, b, c, d, e, f, g, h, i, j)
    return a + b + c + d + e + f + g + h + i + j
)";
    
    DMCompiler::DMProc* proc = CompileAndGetProc(code, "/mob/proc/test");
    ASSERT_NE(proc, nullptr);
    
    EXPECT_EQ(proc->GetParameterCount(), 10);
    
    // Verify all parameters are registered
    EXPECT_NE(proc->GetLocalVariable("a"), nullptr);
    EXPECT_NE(proc->GetLocalVariable("b"), nullptr);
    EXPECT_NE(proc->GetLocalVariable("c"), nullptr);
    EXPECT_NE(proc->GetLocalVariable("d"), nullptr);
    EXPECT_NE(proc->GetLocalVariable("e"), nullptr);
    EXPECT_NE(proc->GetLocalVariable("f"), nullptr);
    EXPECT_NE(proc->GetLocalVariable("g"), nullptr);
    EXPECT_NE(proc->GetLocalVariable("h"), nullptr);
    EXPECT_NE(proc->GetLocalVariable("i"), nullptr);
    EXPECT_NE(proc->GetLocalVariable("j"), nullptr);
}

// ========================================
// Verb Tests
// ========================================

TEST(ProcParameterRegistration_VerbWithParameters) {
    std::string code = R"(
/mob/verb/say(msg as text)
    world << msg
)";
    
    DMCompiler::DMProc* proc = CompileAndGetProc(code, "/mob/verb/say");
    ASSERT_NE(proc, nullptr);
    
    EXPECT_TRUE(proc->IsVerb);
    EXPECT_EQ(proc->GetParameterCount(), 1);
    
    DMCompiler::LocalVariable* param = proc->GetLocalVariable("msg");
    EXPECT_NE(param, nullptr);
    EXPECT_TRUE(param->IsParameter);
}

// ========================================
// HasParameter Tests
// ========================================

TEST(ProcParameterRegistration_HasParameter) {
    std::string code = R"(
/mob/proc/test(a, b, c)
    return a + b + c
)";
    
    DMCompiler::DMProc* proc = CompileAndGetProc(code, "/mob/proc/test");
    ASSERT_NE(proc, nullptr);
    
    EXPECT_TRUE(proc->HasParameter("a"));
    EXPECT_TRUE(proc->HasParameter("b"));
    EXPECT_TRUE(proc->HasParameter("c"));
    EXPECT_FALSE(proc->HasParameter("d"));
    EXPECT_FALSE(proc->HasParameter("nonexistent"));
}

// ========================================
// Test Runner
// ========================================

void RunProcParameterRegistrationTests() {
    std::cout << "\n=== Running Proc Parameter Registration Tests ===\n" << std::endl;
    
    param_reg_tests_run = 0;
    param_reg_tests_passed = 0;
    
    // Basic tests
    RUN_TEST(ProcParameterRegistration_NoParameters);
    RUN_TEST(ProcParameterRegistration_SingleParameter);
    RUN_TEST(ProcParameterRegistration_MultipleParameters);
    RUN_TEST(ProcParameterRegistration_ParameterOrder);
    
    // Type tests
    RUN_TEST(ProcParameterRegistration_TypedParameter);
    RUN_TEST(ProcParameterRegistration_MixedTypedParameters);
    
    // Default value tests
    RUN_TEST(ProcParameterRegistration_ParameterWithDefault);
    RUN_TEST(ProcParameterRegistration_MixedDefaultParameters);
    
    // IsParameter flag tests
    RUN_TEST(ProcParameterRegistration_IsParameterFlag);
    
    // Real-world examples
    RUN_TEST(ProcParameterRegistration_HeapsortExample);
    RUN_TEST(ProcParameterRegistration_QuicksortExample);
    
    // Edge cases
    RUN_TEST(ProcParameterRegistration_LongParameterName);
    RUN_TEST(ProcParameterRegistration_ParameterNamedSrc);
    RUN_TEST(ProcParameterRegistration_ManyParameters);
    
    // Verb tests
    RUN_TEST(ProcParameterRegistration_VerbWithParameters);
    
    // HasParameter tests
    RUN_TEST(ProcParameterRegistration_HasParameter);
    
    int total_tests = param_reg_tests_run;
    int passed_tests = param_reg_tests_passed;
    int failed_tests = total_tests - passed_tests;
    
    std::cout << "\n=== Proc Parameter Registration Test Results ===" << std::endl;
    std::cout << "Total: " << total_tests << std::endl;
    std::cout << "Passed: " << passed_tests << std::endl;
    std::cout << "Failed: " << failed_tests << std::endl;
    
    if (failed_tests == 0) {
        std::cout << "\n✓ All Proc Parameter Registration tests passed!" << std::endl;
    } else {
        std::cout << "\n✗ Some Proc Parameter Registration tests failed!" << std::endl;
    }
}
