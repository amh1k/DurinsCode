#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <memory>
#include "lexer/lexer.h"
#include "parser/parser.h"

// Helper: Extract raw pointer from unique_ptr for dynamic_cast (read-only)
template <typename T>
static T *cast(ASTNode *node)
{
    return dynamic_cast<T *>(node);
}

TEST(ParserTest, ParsesFullWorldScript)
{
    const char *source = R"(
room "bag_end" {
description "The cozy hole of a Hobbit. A golden ring glints on the table."
item the_ring { power: 100, type: "artifact" }
exit east "buckland"
}
room "mount_doom" {
description "The air is thick with ash. The fiery Cracks of Doom loom ahead."
npc sauron { health: 999, hostile: true }
exit west "buckland"
}
action "take ring" {
if current_room == "bag_end" {
player.inventory += the_ring
print "The Precious is yours."
}
}
action "destroy ring" {
if current_room == "mount_doom" && player.has_item(the_ring) {
remove the_ring
print "The ring is consumed by fire! Middle-earth is saved."
player.win = true
} else {
print "You cannot do that here."
}
}
)";

    // ← NEW API: Unified parseProgram(source) returns ParseResult
    ParseResult result = parseProgram(source);

    EXPECT_FALSE(result.hadError); // ← Check for parse errors
    ASSERT_NE(result.ast, nullptr);
    ProgramNode *program = result.ast.get(); // ← Get raw pointer for inspection

    ASSERT_EQ(program->declarations.size(), 4u);

    // ── Room 1: bag_end ──────────────────────────────────────
    auto *room1 = cast<RoomDeclNode>(program->declarations[0].get());
    ASSERT_NE(room1, nullptr);
    EXPECT_EQ(room1->name, "bag_end");

    // ← FIX: description is now a DescriptionStmtNode*, not bare string
    ASSERT_NE(room1->description, nullptr);
    EXPECT_EQ(room1->description->text, "The cozy hole of a Hobbit. A golden ring glints on the table.");

    ASSERT_EQ(room1->items.size(), 1u);
    ASSERT_EQ(room1->exits.size(), 1u);

    auto *item = room1->items[0].get(); // ← unique_ptr → .get()
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->name, "the_ring");
    ASSERT_EQ(item->properties.size(), 2u);

    EXPECT_EQ(item->properties[0]->key, "power");
    EXPECT_EQ(item->properties[0]->value->kind, LiteralKind::NUMBER);
    EXPECT_EQ(item->properties[0]->value->intValue, 100);

    EXPECT_EQ(item->properties[1]->key, "type");
    EXPECT_EQ(item->properties[1]->value->kind, LiteralKind::STRING);
    EXPECT_EQ(item->properties[1]->value->strValue, "artifact");

    // ── Room 2: mount_doom ───────────────────────────────────
    auto *room2 = cast<RoomDeclNode>(program->declarations[1].get());
    ASSERT_NE(room2, nullptr);
    EXPECT_EQ(room2->name, "mount_doom");

    ASSERT_NE(room2->description, nullptr);
    EXPECT_EQ(room2->description->text, "The air is thick with ash. The fiery Cracks of Doom loom ahead.");

    ASSERT_EQ(room2->npcs.size(), 1u);
    ASSERT_EQ(room2->exits.size(), 1u);

    auto *npc = room2->npcs[0].get();
    EXPECT_EQ(npc->name, "sauron");
    ASSERT_EQ(npc->properties.size(), 2u);

    // ── Action 1: "take ring" ────────────────────────────────
    auto *action1 = cast<ActionDeclNode>(program->declarations[2].get());
    ASSERT_NE(action1, nullptr);
    EXPECT_EQ(action1->name, "take ring");
    ASSERT_EQ(action1->statementList.size(), 1u);

    auto *if1 = cast<IfStmtNode>(action1->statementList[0].get());
    ASSERT_NE(if1, nullptr);
    ASSERT_NE(if1->condition, nullptr);
    ASSERT_EQ(if1->condition->clauses.size(), 1u);
    EXPECT_EQ(if1->thenBranch.size(), 2u); // print + assignment

    // ── Action 2: "destroy ring" ─────────────────────────────
    auto *action2 = cast<ActionDeclNode>(program->declarations[3].get());
    ASSERT_NE(action2, nullptr);
    EXPECT_EQ(action2->name, "destroy ring");
    ASSERT_EQ(action2->statementList.size(), 1u);

    auto *if2 = cast<IfStmtNode>(action2->statementList[0].get());
    ASSERT_NE(if2, nullptr);
    ASSERT_EQ(if2->condition->clauses.size(), 2u); // current_room && has_item
    EXPECT_EQ(if2->elseBranch.size(), 1u);         // print "cannot do that"
}

TEST(ParserTest, ParsesSingleRoomDeclaration)
{
    const char *source = R"(
room "bag_end" {
description "A cozy hole."
}
)";

    ParseResult result = parseProgram(source);

    EXPECT_FALSE(result.hadError);
    ASSERT_NE(result.ast, nullptr);
    ProgramNode *program = result.ast.get();

    ASSERT_EQ(program->declarations.size(), 1u);

    auto *room = cast<RoomDeclNode>(program->declarations[0].get());
    ASSERT_NE(room, nullptr);
    EXPECT_EQ(room->name, "bag_end");

    ASSERT_NE(room->description, nullptr); // ← Now a node
    EXPECT_EQ(room->description->text, "A cozy hole.");
}

TEST(ParserTest, ParsesItemProperties)
{
    const char *source = R"(
room "x" {
item ring { power: 100, type: "artifact", cursed: true }
}
)";

    ParseResult result = parseProgram(source);
    EXPECT_FALSE(result.hadError);

    ProgramNode *program = result.ast.get();
    auto *room = cast<RoomDeclNode>(program->declarations[0].get());

    ASSERT_EQ(room->items.size(), 1u);
    auto *item = room->items[0].get();

    ASSERT_EQ(item->properties.size(), 3u);

    EXPECT_EQ(item->properties[0]->key, "power");
    EXPECT_EQ(item->properties[0]->value->kind, LiteralKind::NUMBER);
    EXPECT_EQ(item->properties[0]->value->intValue, 100);

    EXPECT_EQ(item->properties[1]->key, "type");
    EXPECT_EQ(item->properties[1]->value->kind, LiteralKind::STRING);
    EXPECT_EQ(item->properties[1]->value->strValue, "artifact");

    EXPECT_EQ(item->properties[2]->key, "cursed");
    EXPECT_EQ(item->properties[2]->value->kind, LiteralKind::BOOL_TRUE);
    EXPECT_EQ(item->properties[2]->value->boolValue, true);
}

TEST(ParserTest, ParsesNpcDeclaration)
{
    const char *source = R"(
room "mount_doom" {
npc sauron { health: 999, hostile: true }
}
)";

    ParseResult result = parseProgram(source);
    EXPECT_FALSE(result.hadError);

    ProgramNode *program = result.ast.get();
    auto *room = cast<RoomDeclNode>(program->declarations[0].get());

    ASSERT_EQ(room->npcs.size(), 1u);
    auto *npc = room->npcs[0].get();

    EXPECT_EQ(npc->name, "sauron");
    ASSERT_EQ(npc->properties.size(), 2u);
    EXPECT_EQ(npc->properties[0]->key, "health");
    EXPECT_EQ(npc->properties[1]->key, "hostile");
}

TEST(ParserTest, ParsesExitDeclaration)
{
    const char *source = R"(
room "bag_end" {
exit east "buckland"
}
)";

    ParseResult result = parseProgram(source);
    EXPECT_FALSE(result.hadError);

    ProgramNode *program = result.ast.get();
    auto *room = cast<RoomDeclNode>(program->declarations[0].get());

    ASSERT_EQ(room->exits.size(), 1u);
    auto *ex = room->exits[0].get();

    EXPECT_EQ(ex->direction, "east");
    EXPECT_EQ(ex->targetRoom, "buckland");
}

TEST(ParserTest, ParsesCurrentRoomCondition)
{
    const char *source = R"(
action "take ring" {
if current_room == "bag_end" {
print "ok"
}
}
)";

    ParseResult result = parseProgram(source);
    EXPECT_FALSE(result.hadError);

    ProgramNode *program = result.ast.get();
    auto *action = cast<ActionDeclNode>(program->declarations[0].get());
    auto *ifStmt = cast<IfStmtNode>(action->statementList[0].get());

    ASSERT_EQ(ifStmt->condition->clauses.size(), 1u);
    auto *clause = ifStmt->condition->clauses[0].get();

    EXPECT_EQ(clause->kind, ConditionKind::ROOM_CHECK);
    EXPECT_EQ(clause->attribute, "current_room");
    EXPECT_EQ(clause->op, CompareOp::EQUAL_EQUAL);

    ASSERT_NE(clause->value, nullptr);
    EXPECT_EQ(clause->value->kind, LiteralKind::STRING);
    EXPECT_EQ(clause->value->strValue, "bag_end");
}

TEST(ParserTest, ParsesPlayerAssignment)
{
    const char *source = R"(
action "x" {
player.win = true
}
)";

    ParseResult result = parseProgram(source);
    EXPECT_FALSE(result.hadError);

    ProgramNode *program = result.ast.get();
    auto *action = cast<ActionDeclNode>(program->declarations[0].get());

    ASSERT_EQ(action->statementList.size(), 1u);
    auto *assign = cast<AssignStmtNode>(action->statementList[0].get());

    ASSERT_NE(assign, nullptr);
    EXPECT_EQ(assign->attribute, "win");
    EXPECT_EQ(assign->op, AssignOp::ASSIGN);

    ASSERT_NE(assign->value, nullptr);
    EXPECT_EQ(assign->value->kind, LiteralKind::BOOL_TRUE);
    EXPECT_EQ(assign->value->boolValue, true);
}

TEST(ParserTest, ParsesPlayerPlusAssign)
{
    const char *source = R"(
action "x" {
player.inventory += the_ring
}
)";

    ParseResult result = parseProgram(source);
    EXPECT_FALSE(result.hadError);

    ProgramNode *program = result.ast.get();
    auto *action = cast<ActionDeclNode>(program->declarations[0].get());
    auto *assign = cast<AssignStmtNode>(action->statementList[0].get());

    EXPECT_EQ(assign->attribute, "inventory");
    EXPECT_EQ(assign->op, AssignOp::PLUS_ASSIGN);

    EXPECT_EQ(assign->value->kind, LiteralKind::IDENTIFIER);
    EXPECT_EQ(assign->value->strValue, "the_ring");
}

TEST(ParserTest, ParsesPlayerMinusAssign) // ← NEW: Test -= operator
{
    const char *source = R"(
action "use potion" {
player.health -= 10
}
)";

    ParseResult result = parseProgram(source);
    EXPECT_FALSE(result.hadError);

    ProgramNode *program = result.ast.get();
    auto *action = cast<ActionDeclNode>(program->declarations[0].get());
    auto *assign = cast<AssignStmtNode>(action->statementList[0].get());

    EXPECT_EQ(assign->attribute, "health");
    EXPECT_EQ(assign->op, AssignOp::MINUS_ASSIGN); // ← Verify new operator
    EXPECT_EQ(assign->value->kind, LiteralKind::NUMBER);
    EXPECT_EQ(assign->value->intValue, 10);
}

TEST(ParserTest, ParsesLogicalOrInCondition) // ← NEW: Test || operator
{
    const char *source = R"(
action "escape" {
if current_room == "danger" || player.health < 10 {
print "fleeing"
}
}
)";

    ParseResult result = parseProgram(source);
    EXPECT_FALSE(result.hadError);

    ProgramNode *program = result.ast.get();
    auto *action = cast<ActionDeclNode>(program->declarations[0].get());
    auto *ifStmt = cast<IfStmtNode>(action->statementList[0].get());

    // Should have two clauses joined by ||
    ASSERT_EQ(ifStmt->condition->clauses.size(), 2u);
    EXPECT_EQ(ifStmt->condition->clauses[0]->kind, ConditionKind::ROOM_CHECK);
    EXPECT_EQ(ifStmt->condition->clauses[1]->kind, ConditionKind::PLAYER_ATTR_CHECK);
}

TEST(ParserTest, FailsOnMissingRoomBrace)
{
    const char *source = R"(
room "bag_end" {
description "x"
)";

    // ← NEW API: parseProgram handles lexer init internally
    ParseResult result = parseProgram(source);

    EXPECT_TRUE(result.hadError);   // ← Should report error
    EXPECT_NE(result.ast, nullptr); // ← Should still return partial AST
}

TEST(ParserTest, ReportsErrorOnFloatLiteral) // ← NEW: Lexer rejects floats
{
    const char *source = R"(
action "test" {
player.health = 3.14
}
)";

    ParseResult result = parseProgram(source);
    EXPECT_TRUE(result.hadError); // Lexer should reject 3.14
}

TEST(ParserTest, AcceptsUnderscoreIdentifiers) // ← NEW: _leading IDs
{
    const char *source = R"(
room "test" {
description "A room"
item _secret { power: 50 }
}
)";

    ParseResult result = parseProgram(source);
    EXPECT_FALSE(result.hadError);

    ProgramNode *program = result.ast.get();
    auto *room = cast<RoomDeclNode>(program->declarations[0].get());

    ASSERT_EQ(room->items.size(), 1u);
    EXPECT_EQ(room->items[0]->name, "_secret"); // ← Underscore preserved
}

TEST(ParserTest, EmptyActionBodyIsValid) // ← NEW: Edge case
{
    const char *source = R"(
action "do nothing" {
}
)";

    ParseResult result = parseProgram(source);
    EXPECT_FALSE(result.hadError);

    ProgramNode *program = result.ast.get();
    auto *action = cast<ActionDeclNode>(program->declarations[0].get());

    EXPECT_EQ(action->statementList.size(), 0u); // Empty but valid
}

TEST(ParserTest, RoomWithNoBodyElementsIsValid) // ← NEW: Edge case
{
    const char *source = R"(
room "empty" {
}
)";

    ParseResult result = parseProgram(source);
    EXPECT_FALSE(result.hadError);

    ProgramNode *program = result.ast.get();
    auto *room = cast<RoomDeclNode>(program->declarations[0].get());

    EXPECT_EQ(room->name, "empty");
    EXPECT_EQ(room->items.size(), 0u);
    EXPECT_EQ(room->npcs.size(), 0u);
    EXPECT_EQ(room->exits.size(), 0u);
    EXPECT_EQ(room->description, nullptr); // No description = nullptr
}