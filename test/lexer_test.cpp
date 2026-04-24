#include <gtest/gtest.h>
#include <vector>
#include <string>
#include "lexer/lexer.h"

static std::vector<Token> lexAll(const char *source)
{
    initLexer(source);
    std::vector<Token> tokens;
    while (true)
    {
        Token t = scanToken();
        tokens.push_back(t);
        if (t.type == TOKEN_EOF || t.type == TOKEN_ERROR)
            break;
    }
    return tokens;
}

static std::string lexeme(const Token &t)
{
    return std::string(t.start, t.length);
}

static void expectToken(const Token &t, TokenType type, const std::string &text)
{
    EXPECT_EQ(t.type, type);
    EXPECT_EQ(lexeme(t), text);
}
TEST(LexerTest, TokenizesSampleWorldScript)
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

    auto tokens = lexAll(source);

    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens.back().type, TOKEN_EOF);

    expectToken(tokens[0], TOKEN_ROOM, "room");
    expectToken(tokens[1], TOKEN_STRING, "\"bag_end\"");
    expectToken(tokens[2], TOKEN_LEFT_BRACE, "{");
    expectToken(tokens[3], TOKEN_DESCRIPTION, "description");
    expectToken(tokens[4], TOKEN_STRING, "\"The cozy hole of a Hobbit. A golden ring glints on the table.\"");
    expectToken(tokens[5], TOKEN_ITEM, "item");
    expectToken(tokens[6], TOKEN_IDENTIFIER, "the_ring");
    expectToken(tokens[7], TOKEN_LEFT_BRACE, "{");
    expectToken(tokens[8], TOKEN_IDENTIFIER, "power");
    expectToken(tokens[9], TOKEN_COLON, ":");
    expectToken(tokens[10], TOKEN_NUMBER, "100");
    expectToken(tokens[11], TOKEN_COMMA, ",");
    expectToken(tokens[12], TOKEN_IDENTIFIER, "type");
    expectToken(tokens[13], TOKEN_COLON, ":");
    expectToken(tokens[14], TOKEN_STRING, "\"artifact\"");
}
TEST(LexerTest, RecognizesKeywordsAndPunctuation)
{
    const char *source = R"(
room action item npc exit if else print remove player true false description current_room
{}():,.; 
)";

    auto tokens = lexAll(source);

    EXPECT_EQ(tokens[0].type, TOKEN_ROOM);
    EXPECT_EQ(tokens[1].type, TOKEN_ACTION);
    EXPECT_EQ(tokens[2].type, TOKEN_ITEM);
    EXPECT_EQ(tokens[3].type, TOKEN_NPC);
    EXPECT_EQ(tokens[4].type, TOKEN_EXIT);
    EXPECT_EQ(tokens[5].type, TOKEN_IF);
    EXPECT_EQ(tokens[6].type, TOKEN_ELSE);
    EXPECT_EQ(tokens[7].type, TOKEN_PRINT);
    EXPECT_EQ(tokens[8].type, TOKEN_REMOVE);
    EXPECT_EQ(tokens[9].type, TOKEN_PLAYER);
    EXPECT_EQ(tokens[10].type, TOKEN_TRUE);
    EXPECT_EQ(tokens[11].type, TOKEN_FALSE);
    EXPECT_EQ(tokens[12].type, TOKEN_DESCRIPTION);
    EXPECT_EQ(tokens[13].type, TOKEN_CURRENT_ROOM);
}

TEST(LexerTest, RecognizesStringAndNumber)
{
    const char *source = R"(
"hello world"
12345
)";
    auto tokens = lexAll(source);

    EXPECT_EQ(tokens[0].type, TOKEN_STRING);
    EXPECT_EQ(lexeme(tokens[0]), "\"hello world\"");
    EXPECT_EQ(tokens[1].type, TOKEN_NUMBER);
    EXPECT_EQ(lexeme(tokens[1]), "12345");
}

TEST(LexerTest, RecognizesCurrentRoomCondition)
{
    const char *source = R"(
if current_room == "bag_end" {
}
)";
    auto tokens = lexAll(source);

    EXPECT_EQ(tokens[0].type, TOKEN_IF);
    EXPECT_EQ(tokens[1].type, TOKEN_CURRENT_ROOM);
    EXPECT_EQ(tokens[2].type, TOKEN_EQUAL_EQUAL);
    EXPECT_EQ(tokens[3].type, TOKEN_STRING);
    EXPECT_EQ(tokens[4].type, TOKEN_LEFT_BRACE);
}

TEST(LexerTest, RecognizesPlayerHasItemAsContextSensitiveAttribute)
{
    const char *source = R"(
if player.has_item(the_ring) {
}
)";
    auto tokens = lexAll(source);

    EXPECT_EQ(tokens[0].type, TOKEN_IF);
    EXPECT_EQ(tokens[1].type, TOKEN_PLAYER);
    EXPECT_EQ(tokens[2].type, TOKEN_DOT);
    EXPECT_EQ(tokens[3].type, TOKEN_PLAYER_ATTR);
    EXPECT_EQ(lexeme(tokens[3]), "has_item");
    EXPECT_EQ(tokens[4].type, TOKEN_LEFT_PAREN);
    EXPECT_EQ(tokens[5].type, TOKEN_IDENTIFIER);
    EXPECT_EQ(lexeme(tokens[5]), "the_ring");
    EXPECT_EQ(tokens[6].type, TOKEN_RIGHT_PAREN);
}
TEST(LexerTest, RecognizesPlayerAttributeAndPlusEqual)
{
    const char *source = R"(
player.inventory += the_ring
)";
    auto tokens = lexAll(source);

    EXPECT_EQ(tokens[0].type, TOKEN_PLAYER);
    EXPECT_EQ(tokens[1].type, TOKEN_DOT);
    EXPECT_EQ(tokens[2].type, TOKEN_PLAYER_ATTR);
    EXPECT_EQ(lexeme(tokens[2]), "inventory");
    EXPECT_EQ(tokens[3].type, TOKEN_PLUS_EQUAL);
    EXPECT_EQ(tokens[4].type, TOKEN_IDENTIFIER);
}

TEST(LexerTest, ReportsUnexpectedCharacter)
{
    const char *source = "@";
    auto tokens = lexAll(source);

    EXPECT_EQ(tokens[0].type, TOKEN_ERROR);
    EXPECT_NE(std::string(tokens[0].start, tokens[0].length).find("Unexpected character"), std::string::npos);
}

TEST(LexerTest, AcceptsUnderscoreLeadingIdentifiers)
{
    const char *source = "_private __magic _123";
    initLexer(source);

    auto t1 = scanToken();
    EXPECT_EQ(t1.type, TOKEN_IDENTIFIER);
    EXPECT_EQ(std::string(t1.start, t1.length), "_private");
    EXPECT_EQ(t1.line, 1);
    EXPECT_EQ(t1.column, 1); // ← Column tracking test

    auto t2 = scanToken();
    EXPECT_EQ(t2.type, TOKEN_IDENTIFIER);
    EXPECT_EQ(std::string(t2.start, t2.length), "__magic");
    EXPECT_EQ(t2.column, 10);

    auto t3 = scanToken();
    EXPECT_EQ(t3.type, TOKEN_IDENTIFIER);
    EXPECT_EQ(std::string(t3.start, t3.length), "_123");
}

TEST(LexerTest, RejectsFloatingPointNumbers)
{
    const char *source = "3.14";
    initLexer(source);

    auto t = scanToken();
    EXPECT_EQ(t.type, TOKEN_ERROR);
    EXPECT_NE(std::string(t.start, t.length).find("Floating-point"), std::string::npos);
}

TEST(LexerTest, RecognizesLogicalOrOperator)
{
    const char *source = "||";
    initLexer(source);

    auto t = scanToken();
    EXPECT_EQ(t.type, TOKEN_OR_OR);
}

TEST(LexerTest, ReportsColumnNumbers)
{
    const char *source = "room\n  @\naction";
    initLexer(source);

    scanToken(); // TOKEN_ROOM (skipWhiteSpace handles newline internally)

    auto err = scanToken(); // Should hit @
    EXPECT_EQ(err.type, TOKEN_ERROR);
    EXPECT_EQ(err.line, 2);
    EXPECT_EQ(err.column, 3); // @ is at column 3 (after two spaces)
}

TEST(LexerTest, HandlesWindowsLineEndings)
{
    const char *source = "room\r\n\"test\"";
    initLexer(source);

    auto t1 = scanToken();
    EXPECT_EQ(t1.type, TOKEN_ROOM);
    EXPECT_EQ(t1.line, 1);

    auto t2 = scanToken();
    EXPECT_EQ(t2.type, TOKEN_STRING);
    EXPECT_EQ(t2.line, 2); // Should be on line 2 after \r\n
}