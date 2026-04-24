#include <gtest/gtest.h>
#include "semantic/semantic.h"
#include "parser/parser.h"

static SemanticResult analyseSource(const char* src) {
    auto result = parseProgram(src);
    return analyse(*result.ast);
}

TEST(SemanticTest, RegistersRoomsAndItems) {
    const char* src = R"(
room "bag_end" {
    description "A cozy hobbit hole."
    item the_ring { power: 100, type: "artifact" }
    exit east "buckland"
}
room "buckland" {
    description "A bright town."
    exit west "bag_end"
}
action "look" {
    print "You look around."
}
)";
    auto r = analyseSource(src);
    EXPECT_FALSE(r.hadError);
    EXPECT_TRUE(r.symbols.rooms.count("bag_end"));
    EXPECT_TRUE(r.symbols.rooms.count("buckland"));
    EXPECT_TRUE(r.symbols.items.count("the_ring"));
    EXPECT_TRUE(r.symbols.actions.count("look"));
}

TEST(SemanticTest, ErrorOnDuplicateRoom) {
    const char* src = R"(
room "bag_end" { description "First." }
room "bag_end" { description "Duplicate." }
)";
    auto r = analyseSource(src);
    EXPECT_TRUE(r.hadError);
    ASSERT_GE(r.diagnostics.size(), 1u);
    EXPECT_NE(r.diagnostics[0].message.find("Duplicate room"), std::string::npos);
}

TEST(SemanticTest, ErrorOnDuplicateAction) {
    const char* src = R"(
action "go north" { print "Going." }
action "go north" { print "Going again." }
)";
    auto r = analyseSource(src);
    EXPECT_TRUE(r.hadError);
    ASSERT_GE(r.diagnostics.size(), 1u);
    EXPECT_NE(r.diagnostics[0].message.find("Duplicate action"), std::string::npos);
}

TEST(SemanticTest, ErrorOnExitToUndeclaredRoom) {
    const char* src = R"(
room "bag_end" {
    description "A cozy hole."
    exit east "mordor"
}
)";
    auto r = analyseSource(src);
    EXPECT_TRUE(r.hadError);
    EXPECT_NE(r.diagnostics[0].message.find("undeclared room"), std::string::npos);
}

TEST(SemanticTest, ErrorOnDuplicateExitDirection) {
    const char* src = R"(
room "bag_end" {
    description "A hole."
    exit east "buckland"
    exit east "mordor"
}
room "buckland" { description "Town." }
room "mordor" { description "Dark." }
)";
    auto r = analyseSource(src);
    EXPECT_TRUE(r.hadError);
    EXPECT_NE(r.diagnostics[0].message.find("Duplicate exit direction"), std::string::npos);
}

TEST(SemanticTest, ErrorOnRemoveUndeclaredItem) {
    const char* src = R"(
room "bag_end" { description "A hole." }
action "try remove" {
    remove phantom_item
}
)";
    auto r = analyseSource(src);
    EXPECT_TRUE(r.hadError);
    EXPECT_NE(r.diagnostics[0].message.find("undeclared item"), std::string::npos);
}

TEST(SemanticTest, ErrorOnHasItemUndeclaredItem) {
    const char* src = R"(
room "bag_end" { description "A hole." }
action "check" {
    if player.has_item(ghost_item) {
        print "Spooky."
    }
}
)";
    auto r = analyseSource(src);
    EXPECT_TRUE(r.hadError);
    EXPECT_NE(r.diagnostics[0].message.find("undeclared item"), std::string::npos);
}

TEST(SemanticTest, ErrorOnCurrentRoomUndeclaredTarget) {
    const char* src = R"(
room "bag_end" { description "A hole." }
action "check" {
    if current_room == "nowhere" {
        print "Impossible."
    }
}
)";
    auto r = analyseSource(src);
    EXPECT_TRUE(r.hadError);
    EXPECT_NE(r.diagnostics[0].message.find("undeclared room"), std::string::npos);
}

TEST(SemanticTest, CollectsMultipleErrors) {
    const char* src = R"(
room "bag_end" {
    description "A hole."
    exit east "nowhere"
    exit east "also_nowhere"
}
)";
    auto r = analyseSource(src);
    EXPECT_TRUE(r.hadError);
    EXPECT_GE(r.diagnostics.size(), 2u);
}

TEST(SemanticTest, ValidFullProgramPassesClean) {
    const char* src = R"(
room "bag_end" {
    description "The cozy hole of a Hobbit."
    item the_ring { power: 100, type: "artifact" }
    exit east "buckland"
}
room "buckland" {
    description "A bright town."
    exit west "bag_end"
}
action "take ring" {
    if current_room == "bag_end" {
        player.inventory += the_ring
        print "The Precious is yours."
    }
}
action "destroy ring" {
    if current_room == "buckland" && player.has_item(the_ring) {
        remove the_ring
        print "Done."
        player.win = true
    } else {
        print "Cannot do that here."
    }
}
)";
    auto r = analyseSource(src);
    EXPECT_FALSE(r.hadError);
    EXPECT_TRUE(r.diagnostics.empty());
}
