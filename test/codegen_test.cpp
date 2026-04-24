#include <gtest/gtest.h>
#include "codegen/codegen.h"
#include "parser/parser.h"
#include "semantic/semantic.h"
#include "tac/tac.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static CodegenResult compileToJSON(const char* src) {
    auto parsed = parseProgram(src);
    auto sem = analyse(*parsed.ast);
    auto tac = generateTAC(*parsed.ast, sem.symbols);
    return generateBytecode(*parsed.ast, sem.symbols, tac);
}

TEST(CodegenTest, ProducesValidJSON) {
    auto result = compileToJSON(R"(
room "bag_end" { description "A hole." item the_ring { power: 100 } exit east "buckland" }
room "buckland" { description "A town." exit west "bag_end" }
action "take ring" { if current_room == "bag_end" { player.inventory += the_ring print "Got it." } }
)");
    EXPECT_FALSE(result.hadError);
    auto j = json::parse(result.json);
    EXPECT_TRUE(j.contains("world"));
    EXPECT_TRUE(j.contains("actions"));
    EXPECT_TRUE(j.contains("start_room"));
}

TEST(CodegenTest, WorldContainsRooms) {
    auto result = compileToJSON(R"(
room "bag_end" { description "A hole." exit east "buckland" }
room "buckland" { description "A town." exit west "bag_end" }
action "look" { print "You look." }
)");
    auto j = json::parse(result.json);
    EXPECT_EQ(j["world"].size(), 2u);
    EXPECT_EQ(j["world"][0]["name"], "bag_end");
    EXPECT_EQ(j["start_room"], "bag_end");
}

TEST(CodegenTest, ActionsContainInstructions) {
    auto result = compileToJSON(R"(action "look" { print "You look around." })");
    auto j = json::parse(result.json);
    ASSERT_EQ(j["actions"].size(), 1u);
    EXPECT_EQ(j["actions"][0]["name"], "look");
    EXPECT_GE(j["actions"][0]["instructions"].size(), 1u);
}
