#include <gtest/gtest.h>
#include "tac/tac.h"
#include "parser/parser.h"
#include "semantic/semantic.h"

static TACProgram compile(const char* src) {
    auto parsed = parseProgram(src);
    auto sem = analyse(*parsed.ast);
    return generateTAC(*parsed.ast, sem.symbols);
}

TEST(TACTest, PrintStatementEmitsPrintOp) {
    auto prog = compile(R"(action "look" { print "You look around." })");
    ASSERT_EQ(prog.actions.size(), 1u);
    bool hasPrint = false;
    for (const auto& i : prog.actions[0].instructions)
        if (i.op == TACOp::PRINT) hasPrint = true;
    EXPECT_TRUE(hasPrint);
}

TEST(TACTest, IfConditionEmitsJumpIfFalse) {
    auto prog = compile(R"(
room "bag_end" { description "A hole." }
action "check" { if current_room == "bag_end" { print "Home." } }
)");
    bool hasJump = false;
    for (const auto& i : prog.actions[0].instructions)
        if (i.op == TACOp::JUMP_IF_FALSE) hasJump = true;
    EXPECT_TRUE(hasJump);
}

TEST(TACTest, AssignStatementEmitsSetPlayerAttr) {
    auto prog = compile(R"(action "win" { player.win = true })");
    bool hasSet = false;
    for (const auto& i : prog.actions[0].instructions)
        if (i.op == TACOp::SET_PLAYER_ATTR && i.dest == "win") hasSet = true;
    EXPECT_TRUE(hasSet);
}

TEST(TACTest, RemoveEmitsRemoveItemOp) {
    auto prog = compile(R"(
room "x" { description "X." item the_ring { power: 1 } }
action "drop" { remove the_ring }
)");
    bool hasRemove = false;
    for (const auto& i : prog.actions[0].instructions)
        if (i.op == TACOp::REMOVE_ITEM) hasRemove = true;
    EXPECT_TRUE(hasRemove);
}
