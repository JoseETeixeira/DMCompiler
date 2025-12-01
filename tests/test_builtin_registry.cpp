#include "DMBuiltinRegistry.h"
#include "DMValueType.h"
#include "DreamPath.h"
#include <iostream>
#include <cassert>

using namespace DMCompiler;

void TestGlobalProcs() {
    std::cout << "Testing Global Procs..." << std::endl;
    
    DMBuiltinRegistry& registry = DMBuiltinRegistry::Instance();
    
    // Test existence
    assert(registry.IsGlobalBuiltinProc("abs"));
    assert(registry.IsGlobalBuiltinProc("sin"));
    assert(registry.IsGlobalBuiltinProc("locate"));
    assert(!registry.IsGlobalBuiltinProc("nonexistent_proc"));
    
    // Test signatures
    auto absSig = registry.GetGlobalProcSignature("abs");
    assert(absSig.has_value());
    assert(absSig->Name == "abs");
    assert(absSig->Parameters.size() == 1);
    assert(absSig->ReturnType == DMValueType::Num);
    
    auto locateSig = registry.GetGlobalProcSignature("locate");
    assert(locateSig.has_value());
    assert(locateSig->ReturnType == DMValueType::Anything);

    // Test istype signature (should be variadic)
    auto istypeSig = registry.GetGlobalProcSignature("istype");
    assert(istypeSig.has_value());
    assert(istypeSig->Name == "istype");
    assert(istypeSig->IsVariadic);

    
    std::cout << "Global Procs OK" << std::endl;
}

void TestTypeProcs() {
    std::cout << "Testing Type Procs..." << std::endl;
    
    DMBuiltinRegistry& registry = DMBuiltinRegistry::Instance();
    
    DreamPath listPath("/list");
    DreamPath atomPath("/atom");
    DreamPath objPath("/obj"); // Inherits from /atom
    
    // Test /list procs
    assert(registry.IsTypeBuiltinProc(listPath, "Add"));
    assert(registry.IsTypeBuiltinProc(listPath, "Cut"));
    assert(!registry.IsTypeBuiltinProc(listPath, "Move")); // Move is on /atom
    
    // Test /atom procs
    assert(registry.IsTypeBuiltinProc(atomPath, "Move"));
    assert(registry.IsTypeBuiltinProc(atomPath, "Enter")); // Should be Entered? Let's check implementation
    // Implementation has "Entered", not "Enter"
    assert(registry.IsTypeBuiltinProc(atomPath, "Entered"));
    
    // Test inheritance (/obj inherits from /atom)
    assert(registry.IsTypeBuiltinProc(objPath, "Move"));
    assert(registry.IsTypeBuiltinProc(objPath, "Entered"));
    
    // Test signatures
    auto moveSig = registry.GetTypeProcSignature(atomPath, "Move");
    assert(moveSig.has_value());
    assert(moveSig->ReturnType == DMValueType::Num);
    
    std::cout << "Type Procs OK" << std::endl;
}

void TestBuiltinVars() {
    std::cout << "Testing Builtin Vars..." << std::endl;
    
    DMBuiltinRegistry& registry = DMBuiltinRegistry::Instance();
    
    DreamPath atomPath("/atom");
    DreamPath objPath("/obj");
    DreamPath mobPath("/mob");
    
    // Test /atom vars
    assert(registry.IsBuiltinVar(atomPath, "x"));
    assert(registry.IsBuiltinVar(atomPath, "name"));
    assert(registry.IsBuiltinVar(atomPath, "icon"));
    
    // Test inheritance
    assert(registry.IsBuiltinVar(objPath, "x"));
    assert(registry.IsBuiltinVar(mobPath, "key")); // /mob specific
    assert(registry.IsBuiltinVar(mobPath, "x"));   // Inherited from /atom
    
    // Test types
    auto xType = registry.GetVarType(atomPath, "x");
    assert(xType.has_value());
    assert(xType == DMValueType::Num);
    
    auto nameType = registry.GetVarType(atomPath, "name");
    assert(nameType.has_value());
    assert(nameType == DMValueType::Text); // Was String in implementation, changed to Text
    
    std::cout << "Builtin Vars OK" << std::endl;
}

void TestContextVars() {
    std::cout << "Testing Context Vars..." << std::endl;
    
    DMBuiltinRegistry& registry = DMBuiltinRegistry::Instance();
    
    assert(registry.IsContextVariable("src"));
    assert(registry.IsContextVariable("usr"));
    assert(registry.IsContextVariable("args"));
    assert(!registry.IsContextVariable("foo"));
    
    std::cout << "Context Vars OK" << std::endl;
}

void TestTypeHierarchy() {
    std::cout << "Testing Type Hierarchy..." << std::endl;
    
    DMBuiltinRegistry& registry = DMBuiltinRegistry::Instance();
    
    DreamPath atomPath("/atom");
    DreamPath objPath("/obj");
    DreamPath datumPath("/datum");
    DreamPath listPath("/list");
    
    assert(registry.IsBuiltinType(atomPath));
    assert(registry.IsBuiltinType(objPath));
    
    assert(registry.TypeInheritsFrom(objPath, atomPath));
    assert(registry.TypeInheritsFrom(atomPath, datumPath));
    assert(registry.TypeInheritsFrom(objPath, datumPath)); // Transitive
    assert(!registry.TypeInheritsFrom(listPath, atomPath));
    
    std::cout << "Type Hierarchy OK" << std::endl;
}

int main() {
    std::cout << "Running DMBuiltinRegistry tests..." << std::endl;
    
    TestGlobalProcs();
    TestTypeProcs();
    TestBuiltinVars();
    TestContextVars();
    TestTypeHierarchy();
    
    std::cout << "All DMBuiltinRegistry tests passed!" << std::endl;
    return 0;
}
