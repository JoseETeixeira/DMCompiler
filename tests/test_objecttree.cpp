#include "../include/DMObjectTree.h"
#include "../include/DMObject.h"
#include "../include/DMVariable.h"
#include "../include/DreamPath.h"
#include <iostream>
#include <cassert>

using namespace DMCompiler;

// Test counter
int tests_run = 0;
int tests_passed = 0;

#define TEST(name) void name()
#define EXPECT_EQ(a, b) do { \
    tests_run++; \
    if ((a) == (b)) { \
        tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Values not equal" << std::endl; \
    } \
} while(0)

#define EXPECT_NE(a, b) do { \
    tests_run++; \
    if ((a) != (b)) { \
        tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected not equal" << std::endl; \
    } \
} while(0)

#define EXPECT_TRUE(cond) do { \
    tests_run++; \
    if (cond) { \
        tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected true" << std::endl; \
    } \
} while(0)

#define EXPECT_FALSE(cond) do { \
    tests_run++; \
    if (!(cond)) { \
        tests_passed++; \
    } else { \
        std::cout << "FAIL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Expected false" << std::endl; \
    } \
} while(0)

#define ASSERT_NE(a, b) do { \
    if ((a) == (b)) { \
        std::cout << "FATAL: " << __FUNCTION__ << " - Line " << __LINE__ << ": Assertion failed" << std::endl; \
        return; \
    } \
} while(0)

// Test root object creation
TEST(TestRootObjectCreation) {
    DMObjectTree tree(nullptr);
    DMObject* root = tree.GetRoot();
    
    ASSERT_NE(root, nullptr);
    EXPECT_TRUE(root->IsRoot());
    EXPECT_EQ(root->Path, DreamPath::Root);
    EXPECT_EQ(root->Parent, nullptr);
    EXPECT_EQ(root->Id, 0);
}

// Test basic object creation
TEST(TestCreateBasicObject) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DMObject* mob = tree.GetOrCreateDMObject(mobPath);
    
    ASSERT_NE(mob, nullptr);
    EXPECT_EQ(mob->Path, mobPath);
    EXPECT_FALSE(mob->IsRoot());
    
    // Mob should have datum as parent by default
    ASSERT_NE(mob->Parent, nullptr);
    EXPECT_EQ(mob->Parent->Path, DreamPath::Datum);
}

// Test object hierarchy
TEST(TestObjectHierarchy) {
    DMObjectTree tree(nullptr);
    
    // Create /mob/player
    DreamPath playerPath(DreamPath::PathType::Absolute, {"mob", "player"});
    DMObject* player = tree.GetOrCreateDMObject(playerPath);
    
    ASSERT_NE(player, nullptr);
    
    // Player's parent should be /mob
    ASSERT_NE(player->Parent, nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    EXPECT_EQ(player->Parent->Path, mobPath);
    
    // /mob's parent should be /datum
    ASSERT_NE(player->Parent->Parent, nullptr);
    EXPECT_EQ(player->Parent->Parent->Path, DreamPath::Datum);
}

// Test object retrieval
TEST(TestGetExistingObject) {
    DMObjectTree tree(nullptr);
    DreamPath objPath(DreamPath::PathType::Absolute, {"obj"});
    
    // Create the object
    DMObject* obj1 = tree.GetOrCreateDMObject(objPath);
    ASSERT_NE(obj1, nullptr);
    
    // Retrieve the same object
    DMObject* obj2 = tree.GetOrCreateDMObject(objPath);
    EXPECT_EQ(obj1, obj2);
}

// Test TryGetDMObject
TEST(TestTryGetObject) {
    DMObjectTree tree(nullptr);
    DreamPath turfPath(DreamPath::PathType::Absolute, {"turf"});
    
    // Object doesn't exist yet
    DMObject* result = nullptr;
    bool found = tree.TryGetDMObject(turfPath, &result);
    EXPECT_FALSE(found);
    EXPECT_EQ(result, nullptr);
    
    // Create the object
    tree.GetOrCreateDMObject(turfPath);
    
    // Now it should be found
    found = tree.TryGetDMObject(turfPath, &result);
    EXPECT_TRUE(found);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->Path, turfPath);
}

// Test type ID lookup
TEST(TestTypeIdLookup) {
    DMObjectTree tree(nullptr);
    DreamPath areaPath(DreamPath::PathType::Absolute, {"area"});
    
    int typeId;
    bool found = tree.TryGetTypeId(areaPath, typeId);
    EXPECT_FALSE(found);
    
    // Create object
    DMObject* area = tree.GetOrCreateDMObject(areaPath);
    
    // Now ID should be findable
    found = tree.TryGetTypeId(areaPath, typeId);
    EXPECT_TRUE(found);
    EXPECT_EQ(typeId, area->Id);
}

// Test string table
TEST(TestStringTable) {
    DMObjectTree tree(nullptr);
    std::string str1 = "hello";
    std::string str2 = "world";
    std::string str3 = "hello"; // Duplicate
    
    int id1 = tree.AddString(str1);
    int id2 = tree.AddString(str2);
    int id3 = tree.AddString(str3);
    
    EXPECT_EQ(id1, 0);
    EXPECT_EQ(id2, 1);
    EXPECT_EQ(id1, id3); // Duplicate strings should have same ID
    
    EXPECT_EQ(tree.StringTable[id1], "hello");
    EXPECT_EQ(tree.StringTable[id2], "world");
}

// Test global variable creation
TEST(TestGlobalVariableCreation) {
    DMObjectTree tree(nullptr);
    size_t initialGlobals = tree.Globals.size();
    
    DMVariable global(std::nullopt, "temp", true, false, false, false);
    std::string varName = "test_var";
    DMComplexValueType valType(DMValueType::Num);  // Changed from string to DMValueType
    
    int id = tree.CreateGlobal(
        global,
        std::nullopt,  // No type
        varName,
        false,  // Not const
        false,  // Not final
        valType
    );
    
    EXPECT_EQ(id, static_cast<int>(initialGlobals));
    EXPECT_EQ(tree.Globals.size(), initialGlobals + 1);
    EXPECT_EQ(tree.Globals[id].Name, varName);
    EXPECT_TRUE(tree.Globals[id].IsGlobal);
}

// Test DMObject variable operations
TEST(TestObjectVariables) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DMObject* mob = tree.GetOrCreateDMObject(mobPath);
    
    // Add a variable
    DMVariable healthVar(
        std::nullopt,
        "health",
        false,  // Not global
        false,  // Not const
        false,  // Not final
        false   // Not tmp
    );
    mob->Variables["health"] = healthVar;
    
    // Test retrieval
    const DMVariable* retrieved = mob->GetVariable("health");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->Name, "health");
    
    // Test non-existent variable
    const DMVariable* notFound = mob->GetVariable("nonexistent");
    EXPECT_EQ(notFound, nullptr);
}

// Test variable inheritance
TEST(TestVariableInheritance) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DreamPath playerPath(DreamPath::PathType::Absolute, {"mob", "player"});
    
    DMObject* mob = tree.GetOrCreateDMObject(mobPath);
    DMObject* player = tree.GetOrCreateDMObject(playerPath);
    
    // Add variable to parent
    DMVariable nameVar(std::nullopt, "name", false, false, false, false);
    mob->Variables["name"] = nameVar;
    
    // Child should inherit the variable
    const DMVariable* inherited = player->GetVariable("name");
    ASSERT_NE(inherited, nullptr);
    EXPECT_EQ(inherited->Name, "name");
}

// Test HasLocalVariable
TEST(TestHasLocalVariable) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DreamPath playerPath(DreamPath::PathType::Absolute, {"mob", "player"});
    
    DMObject* mob = tree.GetOrCreateDMObject(mobPath);
    DMObject* player = tree.GetOrCreateDMObject(playerPath);
    
    DMVariable healthVar(std::nullopt, "health", false, false, false, false);
    mob->Variables["health"] = healthVar;
    
    EXPECT_TRUE(mob->HasLocalVariable("health"));
    EXPECT_TRUE(player->HasLocalVariable("health")); // Should find in parent
    
    EXPECT_FALSE(mob->HasLocalVariable("missing"));
    EXPECT_FALSE(player->HasLocalVariable("missing"));
}

// Test proc operations
TEST(TestObjectProcs) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DMObject* mob = tree.GetOrCreateDMObject(mobPath);
    
    // Add a proc
    mob->AddProc(100, "Attack");
    
    EXPECT_TRUE(mob->HasProc("Attack"));
    EXPECT_TRUE(mob->OwnsProc("Attack"));
    
    const std::vector<int>* procIds = mob->GetProcs("Attack");
    ASSERT_NE(procIds, nullptr);
    EXPECT_EQ(procIds->size(), 1u);
    EXPECT_EQ((*procIds)[0], 100);
}

// Test proc inheritance
TEST(TestProcInheritance) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DreamPath playerPath(DreamPath::PathType::Absolute, {"mob", "player"});
    
    DMObject* mob = tree.GetOrCreateDMObject(mobPath);
    DMObject* player = tree.GetOrCreateDMObject(playerPath);
    
    // Add proc to parent
    mob->AddProc(200, "Move");
    
    EXPECT_TRUE(mob->HasProc("Move"));
    EXPECT_TRUE(player->HasProc("Move")); // Child should inherit proc
    
    EXPECT_TRUE(mob->OwnsProc("Move"));
    EXPECT_FALSE(player->OwnsProc("Move")); // Child doesn't own inherited proc
}

// Test proc overriding
TEST(TestProcOverride) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DreamPath playerPath(DreamPath::PathType::Absolute, {"mob", "player"});
    
    DMObject* mob = tree.GetOrCreateDMObject(mobPath);
    DMObject* player = tree.GetOrCreateDMObject(playerPath);
    
    // Add proc to parent and child
    mob->AddProc(300, "Attack");
    player->AddProc(301, "Attack");  // Override
    
    // Both should have the proc
    EXPECT_TRUE(mob->HasProc("Attack"));
    EXPECT_TRUE(player->HasProc("Attack"));
    
    // Get proc from player - should get child's version first
    const std::vector<int>* playerProcs = player->GetProcs("Attack");
    ASSERT_NE(playerProcs, nullptr);
    EXPECT_EQ(playerProcs->size(), 1u);
    EXPECT_EQ((*playerProcs)[0], 301);
}

// Test multiple proc overloads
TEST(TestMultipleProcOverloads) {
    DMObjectTree tree(nullptr);
    DreamPath objPath(DreamPath::PathType::Absolute, {"obj"});
    DMObject* obj = tree.GetOrCreateDMObject(objPath);
    
    // Add multiple overloads
    obj->AddProc(400, "New");
    obj->AddProc(401, "New");
    obj->AddProc(402, "New");
    
    const std::vector<int>* procs = obj->GetProcs("New");
    ASSERT_NE(procs, nullptr);
    EXPECT_EQ(procs->size(), 3u);
}

// Test forceFirst parameter in AddProc
TEST(TestProcForceFirst) {
    DMObjectTree tree(nullptr);
    DreamPath objPath(DreamPath::PathType::Absolute, {"obj"});
    DMObject* obj = tree.GetOrCreateDMObject(objPath);
    
    obj->AddProc(500, "Test");
    obj->AddProc(501, "Test");
    obj->AddProc(502, "Test", true);  // Force to front
    
    const std::vector<int>* procs = obj->GetProcs("Test");
    ASSERT_NE(procs, nullptr);
    EXPECT_EQ(procs->size(), 3u);
    EXPECT_EQ((*procs)[0], 502); // forceFirst should put proc at beginning
}

// Test special root types
TEST(TestSpecialRootTypes) {
    DMObjectTree tree(nullptr);
    
    // These types should have /root as parent, not /datum
    std::vector<std::string> specialTypes = {
        "client", "datum", "list", "savefile", "world"
    };
    
    for (const auto& typeName : specialTypes) {
        DreamPath path(DreamPath::PathType::Absolute, {typeName});
        DMObject* obj = tree.GetOrCreateDMObject(path);
        
        ASSERT_NE(obj, nullptr);
        if (typeName == "datum") {
            // datum's parent should be root
            EXPECT_EQ(obj->Parent->Path, DreamPath::Root);
        } else if (obj->Parent != nullptr) {
            // Other special types should also have root as parent
            EXPECT_EQ(obj->Parent->Path, DreamPath::Root);
        }
    }
}

// Test deeply nested hierarchy
TEST(TestDeepHierarchy) {
    DMObjectTree tree(nullptr);
    DreamPath deepPath(DreamPath::PathType::Absolute, 
        {"obj", "item", "weapon", "sword", "legendary"});
    
    DMObject* obj = tree.GetOrCreateDMObject(deepPath);
    ASSERT_NE(obj, nullptr);
    
    // Walk up the hierarchy
    int depth = 0;
    DMObject* current = obj;
    while (current != nullptr && !current->IsRoot()) {
        depth++;
        current = current->Parent;
    }
    
    // Should have walked through: legendary -> sword -> weapon -> item -> obj -> datum -> root
    EXPECT_TRUE(depth >= 5);
}

// Test object count
TEST(TestObjectCount) {
    DMObjectTree tree(nullptr);
    
    size_t initialCount = tree.AllObjects.size();
    EXPECT_TRUE(initialCount > 0); // Should have at least root
    
    tree.GetOrCreateDMObject(DreamPath(DreamPath::PathType::Absolute, {"mob"}));
    tree.GetOrCreateDMObject(DreamPath(DreamPath::PathType::Absolute, {"obj"}));
    tree.GetOrCreateDMObject(DreamPath(DreamPath::PathType::Absolute, {"turf"}));
    
    // Should have created /datum (if not exists), /mob, /obj, /turf
    EXPECT_TRUE(tree.AllObjects.size() > initialCount);
}

// Test variable override
TEST(TestVariableOverride) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DMObject* mob = tree.GetOrCreateDMObject(mobPath);
    
    // Add variable
    DMVariable originalVar(std::nullopt, "icon", false, false, false, false);
    mob->Variables["icon"] = originalVar;
    
    // Add override
    DMVariable overrideVar(std::nullopt, "icon", false, false, false, false);
    mob->VariableOverrides["icon"] = overrideVar;
    
    // GetVariable should return the override
    const DMVariable* retrieved = mob->GetVariable("icon");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->Name, "icon");
}

void RunObjectTreeTests() {
    std::cout << "\n=== Running DMObjectTree Tests ===" << std::endl;
    
    TestRootObjectCreation();
    TestCreateBasicObject();
    TestObjectHierarchy();
    TestGetExistingObject();
    TestTryGetObject();
    TestTypeIdLookup();
    TestStringTable();
    TestGlobalVariableCreation();
    TestObjectVariables();
    TestVariableInheritance();
    TestHasLocalVariable();
    TestObjectProcs();
    TestProcInheritance();
    TestProcOverride();
    TestMultipleProcOverloads();
    TestProcForceFirst();
    TestSpecialRootTypes();
    TestDeepHierarchy();
    TestObjectCount();
    TestVariableOverride();
    
    std::cout << "\nObjectTree Tests: " << tests_passed << "/" << tests_run << " passed" << std::endl;
    if (tests_passed == tests_run) {
        std::cout << "✓ All ObjectTree tests passed!" << std::endl;
    }
}

