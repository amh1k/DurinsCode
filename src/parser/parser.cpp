#include <lexer/lexer.h>
#include <parser/parser.h>
#include <iostream>
#include <vector>
struct Parser
{
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
};

Parser parser;
static void errorAt(Token *token, const char *message);
static void errorAtCurrent(const char *message)
{
    errorAt(&parser.current, message);
}

static void errorAt(Token *token, const char *message)
{
    if (parser.panicMode)
        return;
    parser.panicMode = true;
    parser.hadError = true; // ← Also fix: set hadError

    // Improved error format with column
    std::cerr << "Error at line " << token->line
              << ", column " << token->column
              << ": " << message << std::endl;

    if (token->type == TOKEN_EOF)
    {
        std::cerr << "  at end" << std::endl;
    }
    else if (token->type != TOKEN_ERROR)
    {
        std::string lexeme(token->start, token->length);
        std::cerr << "  token: '" << lexeme << "'" << std::endl;
    }
}
static void error(const char *message)
{
    errorAt(&parser.previous, message);
}
static void advance()
{
    parser.previous = parser.current;
    for (;;)
    {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR)
            break;
        errorAtCurrent(parser.current.start);
    }
}
static void synchronize()
{
    // ── STEP 1: Reset panic mode ──────────────────────────────
    // Allow new errors to be reported after recovery
    parser.panicMode = false;

    // ── STEP 2: Scan forward until we find a sync point ───────
    while (parser.current.type != TOKEN_EOF)
    {
        // ── SYNC POINT A: Statement ended with semicolon ──────
        // (Note: Your language doesn't seem to use semicolons,
        //  so this check may never trigger — see ⚠️ below)
        if (parser.previous.type == TOKEN_SEMICOLON)
            return;

        // ── SYNC POINT B: Start of a known declaration/statement ──
        switch (parser.current.type)
        {
        case TOKEN_ROOM:
        case TOKEN_ACTION:
            return;

        // Statements inside blocks
        case TOKEN_IF:
        case TOKEN_PRINT:
        case TOKEN_REMOVE:
        case TOKEN_PLAYER:
            return;

        // Block boundaries - critical for nested recovery
        case TOKEN_RIGHT_BRACE:
            return;

        // Skip everything else
        default:
            advance();
        }
    }
    // ── Reached EOF: Nothing left to sync to ─────────────────
}
static bool consume(TokenType type, const char *message)
{
    if (parser.current.type == type)
    {
        advance();
        return true;
    }
    errorAtCurrent(message);
    synchronize();
    return false;
}
// static void expression()
// {
// }
// static void number()
// {
//     int value = std::stoi(std::string(parser.previous.start, parser.previous.length));
// }
// Recursive descent functions
static bool check(TokenType type)
{
    if (parser.current.type != type)
    {
        return false;
    }
    else
    {
        return true;
    }
}
static bool match(TokenType type)
{
    if (!check(type))
    {
        return false;
    }
    advance();
    return true;
}

// FORWARD DECLARATIONS (Required for recursion)

static std::unique_ptr<ASTNode> parseStatement();
static std::unique_ptr<ConditionNode> parseCondition();
static std::unique_ptr<LiteralNode> parseLiteral();
static std::unique_ptr<PropertyNode> parseProperty();

// LITERALS & PROPERTIES

static std::unique_ptr<LiteralNode> parseLiteral()
{
    auto node = std::make_unique<LiteralNode>();
    node->line = parser.current.line;
    node->type = NodeType::LITERAL;

    if (match(TOKEN_NUMBER))
    {
        node->kind = LiteralKind::NUMBER;
        node->intValue = std::stoi(std::string(parser.previous.start, parser.previous.length));
    }
    else if (match(TOKEN_STRING))
    {
        node->kind = LiteralKind::STRING;
        node->strValue = std::string(parser.previous.start + 1, parser.previous.length - 2);
    }
    else if (match(TOKEN_TRUE))
    {
        node->kind = LiteralKind::BOOL_TRUE;
        node->boolValue = true;
    }
    else if (match(TOKEN_FALSE))
    {
        node->kind = LiteralKind::BOOL_FALSE;
        node->boolValue = false;
    }
    else if (match(TOKEN_IDENTIFIER))
    {
        node->kind = LiteralKind::IDENTIFIER;
        node->strValue = std::string(parser.previous.start, parser.previous.length);
    }
    else
    {
        errorAtCurrent("Expect literal value.");
        return nullptr;
    }
    return node;
}

static std::unique_ptr<PropertyNode> parseProperty()
{
    if (!consume(TOKEN_IDENTIFIER, "Expect property name."))
    {
        return nullptr;
    }
    std::string key(parser.previous.start, parser.previous.length);
    if (!consume(TOKEN_COLON, "Expect ':' after property name."))
    {
        return nullptr;
    }

    auto node = std::make_unique<PropertyNode>();
    node->type = NodeType::PROPERTY;
    node->key = key;
    node->value = parseLiteral();
    return node;
}
static CompareOp parseCompareOp()
{
    if (match(TOKEN_EQUAL_EQUAL))
    {
        return CompareOp::EQUAL_EQUAL;
    }
    if (match(TOKEN_GREATER_EQUAL))
    {
        return CompareOp::GREATER_EQUAL;
    }
    if (match(TOKEN_GREATER))
    {
        return CompareOp::GREATER;
    }
    if (match(TOKEN_LESS_EQUAL))
    {
        return CompareOp::LESS_EQUAL;
    }
    if (match(TOKEN_LESS))
    {
        return CompareOp::LESS;
    }

    errorAtCurrent("Expect comparison operator (==, >, >=, <, <=) in condition.");
    return CompareOp::NONE;
}
static std::unique_ptr<ConditionPrimaryNode> parseConditionPrimary()
{
    auto node = std::make_unique<ConditionPrimaryNode>();
    // node->type = NodeType::CONDITION_PRIMARY;
    node->line = parser.current.line;
    // node->value = nullptr;
    // node->op = CompareOp::NONE;

    // ── Branch 1: player.something ───────────────────────────────────────
    // Handles both:
    //   player.has_item(the_ring)          → HAS_ITEM_CHECK
    //   player.health >= 50                → PLAYER_ATTR_CHECK
    //   player.win == true                 → PLAYER_ATTR_CHECK

    if (match(TOKEN_PLAYER))
    {
        if (!consume(TOKEN_DOT, "Expect '.' after 'player'."))
        {
            return nullptr;
        }

        // Sub-branch A: player.has_item(identifier)
        // TOKEN_PLAYER_ATTR will match "has_item" since it comes after player.
        // We distinguish it by checking the lexeme itself.
        if (check(TOKEN_PLAYER_ATTR))
        {
            std::string attrName(parser.current.start, parser.current.length);

            if (attrName == "has_item")
            {
                advance(); // consume has_item
                node->kind = ConditionKind::HAS_ITEM_CHECK;
                node->op = CompareOp::NONE; // no operator for has_item

                if (!consume(TOKEN_LEFT_PAREN, "Expect '(' after 'has_item'."))
                    return nullptr;
                if (!consume(TOKEN_IDENTIFIER, "Expect item name inside has_item(...)."))
                    return nullptr;
                node->itemName = std::string(parser.previous.start,
                                             parser.previous.length);
                if (!consume(TOKEN_RIGHT_PAREN, "Expect ')' after item name in has_item."))
                    return nullptr;

                return node;
            }

            // Sub-branch B: player.attribute <op> literal
            // e.g.  player.health >= 50
            //       player.win == true
            //       player.location == "bag_end"
            advance(); // consume the TOKEN_PLAYER_ATTR
            node->kind = ConditionKind::PLAYER_ATTR_CHECK;
            node->attribute = std::string(parser.previous.start,
                                          parser.previous.length);
            node->op = parseCompareOp();
            node->value = parseLiteral();

            return node;
        }

        errorAtCurrent("Expect a player attribute or 'has_item' after 'player.'.");

        return nullptr;
    }

    // ── Branch 2: current_room == "roomname" ─────────────────────────────
    // current_room is scanned as TOKEN_CURRENT_ROOM by the lexer

    if (match(TOKEN_CURRENT_ROOM))
    {
        node->kind = ConditionKind::ROOM_CHECK;
        node->attribute = "current_room";

        // current_room only makes sense with ==
        // but we call the generic helper so the error message is consistent
        node->op = parseCompareOp();

        // rhs must be a string room name, but parseLiteral handles that
        node->value = parseLiteral();
        if (node->op != CompareOp::EQUAL_EQUAL)
        {
            errorAtCurrent("current_room only supports == comparison.");
            return nullptr;
        }
        if (!node->value || node->value->kind != LiteralKind::STRING)
        {
            errorAtCurrent("current_room must be compared to a string room name.");
            return nullptr;
        }

        return node;
    }

    // ── Branch 3: room.ROOM_ATTR <op> literal  (future use) ──────────────
    // e.g.  room.enemy_count > 0
    //       room.locked == false

    if (match(TOKEN_ROOM))
    {
        if (!consume(TOKEN_DOT, "Expect '.' after 'room'."))
            return nullptr;

        if (check(TOKEN_ROOM_ATTR))
        {
            advance(); // consume TOKEN_ROOM_ATTR
            node->kind = ConditionKind::ROOM_ATTR_CHECK;
            node->attribute = std::string(parser.previous.start,
                                          parser.previous.length);
            node->op = parseCompareOp();
            node->value = parseLiteral();

            return node;
        }

        errorAtCurrent("Expect a room attribute after 'room.'.");
        return nullptr;
    }

    // ── No valid condition start found ────────────────────────────────────
    errorAtCurrent("Expect 'player', 'current_room', or 'room' to start a condition.");
    advance();
    return nullptr;
}

static std::unique_ptr<ConditionNode> parseCondition()
{
    auto node = std::make_unique<ConditionNode>();
    node->type = NodeType::CONDITION;

    auto primary = parseConditionPrimary();
    if (!primary)
    {

        return nullptr;
    }
    node->clauses.push_back(std::move(primary));

    while (match(TOKEN_AND_AND) || match(TOKEN_OR_OR)) // ← ADDED: support ||
    {
        auto clause = parseConditionPrimary();
        if (!clause)
            break; // Stop on error, but keep valid clauses collected so far
        node->clauses.push_back(std::move(clause));
    }
    return node;
}

// STATEMENTS

static std::unique_ptr<ASTNode> parseStatement()
{
    if (match(TOKEN_PRINT))
    {
        auto node = std::make_unique<PrintStmtNode>();
        node->line = parser.previous.line;

        if (!consume(TOKEN_STRING, "Expect string message to print."))
            return nullptr;
        node->message = std::string(parser.previous.start + 1, parser.previous.length - 2);
        return std::move(node);
    }

    if (match(TOKEN_IF))
    {
        auto node = std::make_unique<IfStmtNode>();
        node->line = parser.previous.line;

        node->condition = parseCondition();
        if (!node->condition)
            return nullptr;

        if (!consume(TOKEN_LEFT_BRACE, "Expect '{' to start if block."))
            return nullptr;

        // Parse then-branch statements
        while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
        {
            if (parser.panicMode)
                break;

            // ← SAFETY: Stop if we hit a top-level declaration token
            if (check(TOKEN_ROOM) || check(TOKEN_ACTION))
                break;

            auto stmt = parseStatement();
            if (stmt)
                node->thenBranch.push_back(std::move(stmt));
        }

        if (!consume(TOKEN_RIGHT_BRACE, "Expect '}' after if block."))
            return nullptr;

        // Parse optional else-branch
        if (match(TOKEN_ELSE))
        {
            if (!consume(TOKEN_LEFT_BRACE, "Expect '{' to start else block."))
                return nullptr;

            while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
            {
                if (parser.panicMode)
                    break;
                if (check(TOKEN_ROOM) || check(TOKEN_ACTION))
                    break;

                auto stmt = parseStatement();
                if (stmt)
                    node->elseBranch.push_back(std::move(stmt));
            }
            if (!consume(TOKEN_RIGHT_BRACE, "Expect '}' after else block."))
                return nullptr;
        }
        return std::move(node);
    }

    //  FIXED: Handle TOKEN_PLAYER_ATTR for player assignments
    if (match(TOKEN_PLAYER))
    {
        auto node = std::make_unique<AssignStmtNode>();
        node->line = parser.previous.line;

        if (!consume(TOKEN_DOT, "Expect '.' after 'player'."))
            return nullptr;

        // Accept context-sensitive TOKEN_PLAYER_ATTR or fallback IDENTIFIER
        if (match(TOKEN_PLAYER_ATTR) || match(TOKEN_IDENTIFIER))
        {
            node->attribute = std::string(parser.previous.start, parser.previous.length);
        }
        else
        {
            errorAtCurrent("Expect player attribute name after 'player.'.");
            synchronize();
            return nullptr;
        }

        // Parse assignment operator: check compound ops BEFORE simple =
        if (match(TOKEN_PLUS_EQUAL))
        {
            node->op = AssignOp::PLUS_ASSIGN;
        }
        else if (match(TOKEN_MINUS_EQUAL)) // ← ADDED: support -=
        {
            node->op = AssignOp::MINUS_ASSIGN;
        }
        else if (match(TOKEN_EQUAL))
        {
            node->op = AssignOp::ASSIGN;
        }
        else
        {
            errorAtCurrent("Expect '=', '+=', or '-=' for assignment.");
            synchronize();
            return nullptr;
        }

        node->value = parseLiteral();
        if (!node->value)
            return nullptr;

        return std::move(node);
    }

    errorAtCurrent("Expect statement.");
    advance();
    return nullptr;
}

// DECLARATIONS (Room, Item, NPC, Exit)

static bool parseRoomBody(RoomDeclNode *room)
{
    if (match(TOKEN_DESCRIPTION))
    {
        if (!consume(TOKEN_STRING, "Expect description text."))
            return false;
        auto desc = std::make_unique<DescriptionStmtNode>();
        desc->text = std::string(parser.previous.start + 1, parser.previous.length - 2);
        room->description = std::move(desc);
        return true;
    }
    else if (match(TOKEN_ITEM))
    {
        if (!consume(TOKEN_IDENTIFIER, "Expect item name."))
            return false;

        auto item = std::make_unique<ItemDeclNode>();
        item->name = std::string(parser.previous.start, parser.previous.length);
        item->type = NodeType::ITEM_DECL;
        // item->name = std::string(parser.previous.start, parser.previous.length);

        if (!consume(TOKEN_LEFT_BRACE, "Expect '{'."))
        {

            return false;
        }

        if (!check(TOKEN_RIGHT_BRACE))
        {
            do
            {
                auto prop = parseProperty();
                if (prop) //  Only push valid properties
                    item->properties.push_back(std::move(prop));
                if (parser.panicMode)
                    break;
            } while (match(TOKEN_COMMA));
        }

        if (!consume(TOKEN_RIGHT_BRACE, "Expect '}'."))
        {

            return false;
        }
        room->items.push_back(std::move(item));
        return true;
    }
    else if (match(TOKEN_NPC))
    {
        if (!consume(TOKEN_IDENTIFIER, "Expect NPC name."))
            return false;

        auto npc = std::make_unique<NpcDeclNode>();
        npc->type = NodeType::NPC_DECL;
        npc->name = std::string(parser.previous.start, parser.previous.length);

        if (!consume(TOKEN_LEFT_BRACE, "Expect '{'."))
        {

            return false;
        }

        if (!check(TOKEN_RIGHT_BRACE))
        {
            do
            {
                auto prop = parseProperty();
                if (prop)
                    npc->properties.push_back(std::move(prop));
                if (parser.panicMode)
                    break;
            } while (match(TOKEN_COMMA));
        }

        if (!consume(TOKEN_RIGHT_BRACE, "Expect '}'."))
        {

            return false;
        }
        room->npcs.push_back(std::move(npc));
        return true;
    }
    else if (match(TOKEN_EXIT))
    {
        auto exit = std::make_unique<ExitDeclNode>();
        exit->type = NodeType::EXIT_DECL;

        // Direction is an identifier (north/south/east/west)
        if (!match(TOKEN_IDENTIFIER))
        {
            errorAtCurrent("Expect direction (north/south/east/west).");

            synchronize();
            return false;
        }
        exit->direction = std::string(parser.previous.start, parser.previous.length);

        if (!consume(TOKEN_STRING, "Expect target room name."))
        {

            return false;
        }
        exit->targetRoom = std::string(parser.previous.start + 1, parser.previous.length - 2);
        room->exits.push_back(std::move(exit));
        return true;
    }

    errorAtCurrent("Expect room body item, npc, exit, or description.");
    advance();
    synchronize();
    return false;
}

static std::unique_ptr<RoomDeclNode> parseRoomDecl()
{
    if (!consume(TOKEN_STRING, "Expect room name."))
        return nullptr;

    auto node = std::make_unique<RoomDeclNode>();
    node->type = NodeType::ROOM_DECL;
    node->name = std::string(parser.previous.start + 1, parser.previous.length - 2);

    if (!consume(TOKEN_LEFT_BRACE, "Expect '{' before room body."))
    {

        return nullptr;
    }

    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
    {
        if (parser.panicMode)
            break;
        if (!parseRoomBody(node.get()))
        {
            // parseRoomBody already called synchronize() on error
            // Continue trying to parse more body items
        }
    }

    if (!consume(TOKEN_RIGHT_BRACE, "Expect '}' after room body."))
    {

        return nullptr;
    }
    return node;
}

static std::unique_ptr<ActionDeclNode> parseActionDecl()
{
    if (!consume(TOKEN_STRING, "Expect action name."))
        return nullptr;

    auto node = std::make_unique<ActionDeclNode>();
    node->type = NodeType::ACTION_DECL;
    node->name = std::string(parser.previous.start + 1, parser.previous.length - 2);
    if (!consume(TOKEN_LEFT_BRACE, "Expect '{' before action statements."))
    {

        return nullptr;
    }

    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
    {
        if (parser.panicMode)
            break;

        auto stmt = parseStatement();
        if (stmt)
            node->statementList.push_back(std::move(stmt));
        // If stmt is nullptr, skip it and continue (error already reported)
    }

    if (!consume(TOKEN_RIGHT_BRACE, "Expect '}' after action statements."))
    {

        return nullptr;
    }
    return node;
}

// TOP LEVEL ENTRY

static void initParser()
{
    parser.current = {};
    parser.previous = {};
    parser.hadError = false;
    parser.panicMode = false;
}
ParseResult parseProgram(const char *source)
{
    initParser();
    initLexer(source);
    ParseResult result;
    result.ast = std::make_unique<ProgramNode>();
    // ProgramNode *program = new ProgramNode();
    // program->type = NodeType::PROGRAM;

    while (!check(TOKEN_EOF))
    {
        if (parser.panicMode && !check(TOKEN_ROOM) && !check(TOKEN_ACTION))
        {
            synchronize();
            continue;
        }

        if (match(TOKEN_ROOM))
        {
            auto room = parseRoomDecl();
            if (room) //  Only push valid declarations
                result.ast->declarations.push_back(std::move(room));
        }
        else if (match(TOKEN_ACTION))
        {
            auto action = parseActionDecl();
            if (action)
                result.ast->declarations.push_back(std::move(action));
        }
        else
        {
            errorAtCurrent("Expect 'room' or 'action' declaration.");
            advance();
            synchronize();
        }
    }
    result.hadError = parser.hadError;
    return result;
}