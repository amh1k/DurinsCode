#ifndef durin_parser_h // ← FIX: was "durin_paser_h" (typo)
#define durin_parser_h

#include <lexer/lexer.h>
#include <memory>
#include <string>
#include <vector>

// ============================================================
// AST Node Types
// ============================================================
enum class NodeType
{
    PROGRAM,

    // Declarations
    ROOM_DECL,
    ACTION_DECL,
    ITEM_DECL,
    NPC_DECL,
    EXIT_DECL,
    DESCRIPTION_STMT, // ← Now used for description as proper node

    // Statements
    PRINT_STMT,
    REMOVE_STMT,
    IF_STMT,
    ASSIGN_STMT,

    // Conditions
    CONDITION,
    CONDITION_PRIMARY,

    // Expressions/Literals
    LITERAL,
    PROPERTY,
};

enum class LiteralKind
{
    NUMBER,
    STRING,
    BOOL_TRUE,
    BOOL_FALSE,
    IDENTIFIER,
};

enum class CompareOp
{
    NONE,
    EQUAL_EQUAL,
    GREATER,
    GREATER_EQUAL,
    LESS,
    LESS_EQUAL,
};

enum class AssignOp
{
    ASSIGN,
    PLUS_ASSIGN,
    MINUS_ASSIGN, // ← ADDED: support for -= operator
};

enum class ConditionKind
{
    HAS_ITEM_CHECK,    // player.has_item(x)
    PLAYER_ATTR_CHECK, // player.attr OP value
    ROOM_CHECK,        // current_room == "x"
    ROOM_ATTR_CHECK,   // room.attr OP value (future)
};

// ============================================================
// Base AST Node
// ============================================================
struct ASTNode
{
    NodeType type;
    int line;
    ASTNode(NodeType t, int l) : type(t), line(l) {}

    virtual ~ASTNode() = default; // Enable polymorphic deletion
};

// ============================================================
// Literal & Property Nodes
// ============================================================
struct LiteralNode : public ASTNode
{
    LiteralKind kind;

    // Union-style storage (only one is valid based on 'kind')
    int intValue;
    std::string strValue;
    bool boolValue;

    LiteralNode() : ASTNode{NodeType::LITERAL, 0}, kind{LiteralKind::NUMBER},
                    intValue{0}, boolValue{false} {}
};

struct PropertyNode : public ASTNode
{
    std::string key;
    std::unique_ptr<LiteralNode> value; // ← FIX: unique_ptr for ownership

    PropertyNode() : ASTNode{NodeType::PROPERTY, 0} {}
};

// ============================================================
// Condition Nodes
// ============================================================
struct ConditionPrimaryNode : public ASTNode
{
    ConditionKind kind;
    CompareOp op;
    std::string attribute;              // for player.attr, room.attr, current_room
    std::string itemName;               // for has_item(x)
    std::unique_ptr<LiteralNode> value; // ← FIX: unique_ptr

    ConditionPrimaryNode() : ASTNode{NodeType::CONDITION_PRIMARY, 0},
                             kind{ConditionKind::HAS_ITEM_CHECK},
                             op{CompareOp::NONE} {}
};

struct ConditionNode : public ASTNode
{
    std::vector<std::unique_ptr<ConditionPrimaryNode>> clauses; // ← FIX: unique_ptr vector

    ConditionNode() : ASTNode{NodeType::CONDITION, 0} {}
};

// ============================================================
// Statement Nodes
// ============================================================
struct PrintStmtNode : public ASTNode
{
    std::string message;

    PrintStmtNode() : ASTNode{NodeType::PRINT_STMT, 0} {}
};

struct RemoveStmtNode : public ASTNode
{
    std::string itemName;

    RemoveStmtNode() : ASTNode{NodeType::REMOVE_STMT, 0} {}
};

struct IfStmtNode : public ASTNode
{
    std::unique_ptr<ConditionNode> condition; // ← FIX: unique_ptr
    std::vector<std::unique_ptr<ASTNode>> thenBranch;
    std::vector<std::unique_ptr<ASTNode>> elseBranch;

    IfStmtNode() : ASTNode{NodeType::IF_STMT, 0} {}
};

struct AssignStmtNode : public ASTNode
{
    std::string attribute;
    AssignOp op;
    std::unique_ptr<LiteralNode> value; // ← FIX: unique_ptr

    AssignStmtNode() : ASTNode{NodeType::ASSIGN_STMT, 0}, op{AssignOp::ASSIGN} {}
};

// ============================================================
// Declaration Nodes
// ============================================================
struct ExitDeclNode : public ASTNode
{
    std::string direction;
    std::string targetRoom;

    ExitDeclNode() : ASTNode{NodeType::EXIT_DECL, 0} {}
};

struct ItemDeclNode : public ASTNode
{
    std::string name;
    std::vector<std::unique_ptr<PropertyNode>> properties; // ← FIX: unique_ptr

    ItemDeclNode() : ASTNode{NodeType::ITEM_DECL, 0} {}
};

struct NpcDeclNode : public ASTNode
{
    std::string name;
    std::vector<std::unique_ptr<PropertyNode>> properties; // ← FIX: unique_ptr

    NpcDeclNode() : ASTNode{NodeType::NPC_DECL, 0} {}
};

// ← FIX: Description as proper AST node instead of bare string
struct DescriptionStmtNode : public ASTNode
{
    std::string text;

    DescriptionStmtNode() : ASTNode{NodeType::DESCRIPTION_STMT, 0} {}
};

struct RoomDeclNode : public ASTNode
{
    std::string name;
    std::unique_ptr<DescriptionStmtNode> description; // ← FIX: proper node
    std::vector<std::unique_ptr<ItemDeclNode>> items;
    std::vector<std::unique_ptr<NpcDeclNode>> npcs;
    std::vector<std::unique_ptr<ExitDeclNode>> exits;

    RoomDeclNode() : ASTNode{NodeType::ROOM_DECL, 0} {}
};

struct ActionDeclNode : public ASTNode
{
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> statementList; // ← FIX: unique_ptr

    ActionDeclNode() : ASTNode{NodeType::ACTION_DECL, 0} {}
};

// ============================================================
// Program Root
// ============================================================
struct ProgramNode : public ASTNode
{
    std::vector<std::unique_ptr<ASTNode>> declarations; // ← FIX: unique_ptr

    ProgramNode() : ASTNode{NodeType::PROGRAM, 0} {}
};

// ============================================================
// Parse Result (for error reporting)
// ============================================================
struct ParseResult
{
    std::unique_ptr<ProgramNode> ast;
    bool hadError;

    ParseResult() : hadError{false} {}
};

// ============================================================
// Parser Interface
// ============================================================
ParseResult parseProgram(const char *source); // ← FIX: unified init + parse

#endif