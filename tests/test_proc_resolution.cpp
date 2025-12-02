#include "../include/DMObjectTree.h"
#include "../include/DMObject.h"
#include "../include/DMProc.h"
#include "../include/DreamPath.h"
#include <iostream>
#include <cassert>

using namespace DMCompiler;

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

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

// Test basic proc lookup
TEST(TestBasicProcLookup) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DMObject* mob = tree.GetOrCreateDMObject(mobPath);
    
    // Create a proc for mob
    DMProc* proc = tree.CreateProc("Attack", mob, false, Location());
    mob->AddProc(proc->Id, "Attack");
    
    // Lookup should find the proc
    DMProc* found = tree.GetProc(mob, "Attack");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->Name, "Attack");
    EXPECT_EQ(found->Id, proc->Id);
}

// Test proc inheritance - child should find parent's proc
TEST(TestProcInheritanceLookup) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DreamPath playerPath(DreamPath::PathType::Absolute, {"mob", "player"});
    
    DMObject* mob = tree.GetOrCreateDMObject(mobPath);
    DMObject* player = tree.GetOrCreateDMObject(playerPath);
    
    // Create a proc on the parent (mob)
    DMProc* moveProc = tree.CreateProc("Move", mob, false, Location());
    mob->AddProc(moveProc->Id, "Move");
    
    // Child should find parent's proc via GetProc
    DMProc* found = tree.GetProc(player, "Move");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->Name, "Move");
    EXPECT_EQ(found->Id, moveProc->Id);
}

// Test proc override - child's proc should be found first
TEST(TestProcOverride) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DreamPath playerPath(DreamPath::PathType::Absolute, {"mob", "player"});
    
    DMObject* mob = tree.GetOrCreateDMObject(mobPath);
    DMObject* player = tree.GetOrCreateDMObject(playerPath);
    
    // Create Move on parent
    DMProc* parentMove = tree.CreateProc("Move", mob, false, Location());
    mob->AddProc(parentMove->Id, "Move");
    
    // Create Move on child (override)
    DMProc* childMove = tree.CreateProc("Move", player, false, Location());
    player->AddProc(childMove->Id, "Move");
    
    // When looking up from player, should find child's version
    DMProc* found = tree.GetProc(player, "Move");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->Id, childMove->Id);
    
    // When looking up from mob, should find parent's version
    DMProc* parentFound = tree.GetProc(mob, "Move");
    ASSERT_NE(parentFound, nullptr);
    EXPECT_EQ(parentFound->Id, parentMove->Id);
}

// Test global proc lookup
TEST(TestGlobalProcLookup) {
    DMObjectTree tree(nullptr);
    DMObject* root = tree.GetRoot();
    
    // Create a global proc
    DMProc* globalProc = tree.CreateProc("sleep", root, false, Location());
    tree.RegisterGlobalProc("sleep", globalProc->Id);
    
    // Should be findable with null object
    DMProc* found = tree.GetProc(nullptr, "sleep");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->Name, "sleep");
    
    // Should also be findable from any object via fallback
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DMObject* mob = tree.GetOrCreateDMObject(mobPath);
    
    DMProc* foundFromMob = tree.GetProc(mob, "sleep");
    ASSERT_NE(foundFromMob, nullptr);
    EXPECT_EQ(foundFromMob->Name, "sleep");
}

// Test global proc fallback - when proc not found in hierarchy
TEST(TestGlobalProcFallback) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DMObject* mob = tree.GetOrCreateDMObject(mobPath);
    DMObject* root = tree.GetRoot();
    
    // Create a local proc on mob
    DMProc* localProc = tree.CreateProc("Attack", mob, false, Location());
    mob->AddProc(localProc->Id, "Attack");
    
    // Create a global proc
    DMProc* globalProc = tree.CreateProc("world_proc", root, false, Location());
    tree.RegisterGlobalProc("world_proc", globalProc->Id);
    
    // Attack should be found locally
    DMProc* attackFound = tree.GetProc(mob, "Attack");
    ASSERT_NE(attackFound, nullptr);
    EXPECT_EQ(attackFound->Id, localProc->Id);
    
    // world_proc should be found via global fallback
    DMProc* globalFound = tree.GetProc(mob, "world_proc");
    ASSERT_NE(globalFound, nullptr);
    EXPECT_EQ(globalFound->Id, globalProc->Id);
}

// Test GetAllProcsForObject - collects all procs including inherited
TEST(TestGetAllProcsForObject) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DreamPath playerPath(DreamPath::PathType::Absolute, {"mob", "player"});
    
    DMObject* mob = tree.GetOrCreateDMObject(mobPath);
    DMObject* player = tree.GetOrCreateDMObject(playerPath);
    
    // Create procs on parent
    DMProc* moveProc = tree.CreateProc("Move", mob, false, Location());
    mob->AddProc(moveProc->Id, "Move");
    
    DMProc* attackProc = tree.CreateProc("Attack", mob, false, Location());
    mob->AddProc(attackProc->Id, "Attack");
    
    // Create override on child
    DMProc* childMove = tree.CreateProc("Move", player, false, Location());
    player->AddProc(childMove->Id, "Move");
    
    // Create new proc on child
    DMProc* specialProc = tree.CreateProc("Special", player, false, Location());
    player->AddProc(specialProc->Id, "Special");
    
    // Get all procs for player
    auto allProcs = tree.GetAllProcsForObject(player);
    
    // Should have 3 procs: Move (child version), Attack (from parent), Special
    EXPECT_TRUE(allProcs.find("Move") != allProcs.end());
    EXPECT_TRUE(allProcs.find("Attack") != allProcs.end());
    EXPECT_TRUE(allProcs.find("Special") != allProcs.end());
    
    // Move should be child's version
    EXPECT_EQ(allProcs["Move"]->Id, childMove->Id);
    
    // Attack should be parent's version
    EXPECT_EQ(allProcs["Attack"]->Id, attackProc->Id);
    
    // Special should be child's version
    EXPECT_EQ(allProcs["Special"]->Id, specialProc->Id);
}

// Test GetAllProcsForObject with deep hierarchy
TEST(TestGetAllProcsDeepHierarchy) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DreamPath playerPath(DreamPath::PathType::Absolute, {"mob", "player"});
    DreamPath adminPath(DreamPath::PathType::Absolute, {"mob", "player", "admin"});
    
    DMObject* mob = tree.GetOrCreateDMObject(mobPath);
    DMObject* player = tree.GetOrCreateDMObject(playerPath);
    DMObject* admin = tree.GetOrCreateDMObject(adminPath);
    
    // Create proc on each level
    DMProc* mobNew = tree.CreateProc("New", mob, false, Location());
    mob->AddProc(mobNew->Id, "New");
    
    DMProc* playerNew = tree.CreateProc("New", player, false, Location());
    player->AddProc(playerNew->Id, "New");
    
    DMProc* adminNew = tree.CreateProc("New", admin, false, Location());
    admin->AddProc(adminNew->Id, "New");
    
    // Admin should get its own New proc
    auto adminProcs = tree.GetAllProcsForObject(admin);
    EXPECT_EQ(adminProcs["New"]->Id, adminNew->Id);
    
    // Player should get its own New proc
    auto playerProcs = tree.GetAllProcsForObject(player);
    EXPECT_EQ(playerProcs["New"]->Id, playerNew->Id);
    
    // Mob should get its own New proc
    auto mobProcs = tree.GetAllProcsForObject(mob);
    EXPECT_EQ(mobProcs["New"]->Id, mobNew->Id);
}

// Test UpwardSearch with proc support
TEST(TestUpwardSearchWithProc) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DreamPath playerPath(DreamPath::PathType::Absolute, {"mob", "player"});
    
    DMObject* mob = tree.GetOrCreateDMObject(mobPath);
    DMObject* player = tree.GetOrCreateDMObject(playerPath);
    
    // Create proc on mob
    DMProc* attackProc = tree.CreateProc("Attack", mob, false, Location());
    mob->AddProc(attackProc->Id, "Attack");
    
    // Search from player path for a type with Attack proc
    DreamPath searchPath(DreamPath::PathType::Relative, {"mob"});
    auto result = tree.UpwardSearch(playerPath, searchPath, "Attack");
    
    // Should find mob since it has the Attack proc
    EXPECT_TRUE(result.has_value());
    if (result.has_value()) {
        EXPECT_EQ(result.value(), mobPath);
    }
}

// Test UpwardSearch finds global proc
TEST(TestUpwardSearchGlobalProc) {
    DMObjectTree tree(nullptr);
    DMObject* root = tree.GetRoot();
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    tree.GetOrCreateDMObject(mobPath);
    
    // Create a global proc
    DMProc* globalProc = tree.CreateProc("world_func", root, false, Location());
    tree.RegisterGlobalProc("world_func", globalProc->Id);
    
    // Search from mob path for type with world_func proc
    DreamPath searchPath(DreamPath::PathType::Relative, {});
    auto result = tree.UpwardSearch(mobPath, DreamPath::Root, "world_func");
    
    // Should find root since global procs are there
    EXPECT_TRUE(result.has_value());
}

// Test proc not found returns nullptr
TEST(TestProcNotFound) {
    DMObjectTree tree(nullptr);
    DreamPath mobPath(DreamPath::PathType::Absolute, {"mob"});
    DMObject* mob = tree.GetOrCreateDMObject(mobPath);
    
    // No procs defined, should return nullptr
    DMProc* found = tree.GetProc(mob, "NonExistent");
    EXPECT_EQ(found, nullptr);
}

// Test GetAllProcsForObject with null object
TEST(TestGetAllProcsNullObject) {
    DMObjectTree tree(nullptr);
    
    // Should return empty map for null object
    auto procs = tree.GetAllProcsForObject(nullptr);
    EXPECT_TRUE(procs.empty());
}

void RunProcResolutionTests() {
    std::cout << "\n=== Running Proc Resolution Tests ===" << std::endl;
    
    TestBasicProcLookup();
    TestProcInheritanceLookup();
    TestProcOverride();
    TestGlobalProcLookup();
    TestGlobalProcFallback();
    TestGetAllProcsForObject();
    TestGetAllProcsDeepHierarchy();
    TestUpwardSearchWithProc();
    TestUpwardSearchGlobalProc();
    TestProcNotFound();
    TestGetAllProcsNullObject();
    
    std::cout << "\nProc Resolution Tests: " << tests_passed << "/" << tests_run << " passed" << std::endl;
    if (tests_passed == tests_run) {
        std::cout << "✓ All Proc Resolution tests passed!" << std::endl;
    }
}

int main() {
    RunProcResolutionTests();
    return (tests_passed == tests_run) ? 0 : 1;
}
