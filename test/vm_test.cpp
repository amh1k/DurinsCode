#include <gtest/gtest.h>
#include "vm/vm.h"
#include "codegen/codegen.h"
#include "parser/parser.h"
#include "semantic/semantic.h"
#include "tac/tac.h"

static std::string compileStr(const char* src) {
    auto parsed = parseProgram(src);
    auto sem = analyse(*parsed.ast);
    auto tac = generateTAC(*parsed.ast, sem.symbols);
    return generateBytecode(*parsed.ast, sem.symbols, tac).json;
}

TEST(VMTest, LoadsBytecodeFromString) {
    VM vm;
    EXPECT_TRUE(vm.loadBytecodeFromString(compileStr(R"(
room "bag_end" { description "A hole." exit east "buckland" }
room "buckland" { description "A town." exit west "bag_end" }
action "look" { print "You look." }
)")));
}

TEST(VMTest, ExecutesPrintAction) {
    VM vm;
    vm.loadBytecodeFromString(compileStr(R"(action "look" { print "You look around." })"));
    GameState state;
    EXPECT_TRUE(vm.executeAction("look", state));
}

TEST(VMTest, ActionNotFoundReturnsFalse) {
    VM vm;
    vm.loadBytecodeFromString(compileStr(R"(action "look" { print "You look." })"));
    GameState state;
    EXPECT_FALSE(vm.executeAction("fly", state));
}

TEST(VMTest, ConditionalActionAddsToInventory) {
    VM vm;
    vm.loadBytecodeFromString(compileStr(R"(
room "bag_end" { description "A hole." item the_ring { power: 100 } exit east "buckland" }
room "buckland" { description "A town." exit west "bag_end" }
action "take ring" {
    if current_room == "bag_end" {
        player.inventory += the_ring
        print "Got the ring."
    }
}
)"));
    GameState state; state.currentRoom = "bag_end";
    vm.executeAction("take ring", state);
    EXPECT_TRUE(state.inventory.count("the_ring"));
}

TEST(VMTest, ConditionalSkippedWhenRoomWrong) {
    VM vm;
    vm.loadBytecodeFromString(compileStr(R"(
room "bag_end" { description "A hole." item the_ring { power: 100 } exit east "buckland" }
room "buckland" { description "A town." exit west "bag_end" }
action "take ring" {
    if current_room == "bag_end" {
        player.inventory += the_ring
        print "Got the ring."
    }
}
)"));
    GameState state; state.currentRoom = "buckland";
    vm.executeAction("take ring", state);
    EXPECT_FALSE(state.inventory.count("the_ring"));
}
