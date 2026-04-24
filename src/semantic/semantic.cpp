#include "semantic/semantic.h"
#include <iostream>

static void addError(SemanticResult& result, int line, int col, const std::string& msg) {
    result.diagnostics.push_back({Diagnostic::Severity::ERROR, line, col, msg});
    result.hadError = true;
}

static void registerDeclarations(const ProgramNode& ast, SemanticResult& result) {
    for (const auto& decl : ast.declarations) {
        if (decl->type == NodeType::ROOM_DECL) {
            const auto& room = static_cast<const RoomDeclNode&>(*decl);
            if (result.symbols.rooms.count(room.name)) {
                addError(result, room.line, 0, "Duplicate room: \"" + room.name + "\"");
            } else {
                RoomInfo info;
                info.name = room.name;
                info.line = room.line;
                for (const auto& item : room.items) {
                    info.itemNames.push_back(item->name);
                    if (!result.symbols.items.count(item->name))
                        result.symbols.items[item->name] = {item->name, item->line};
                }
                for (const auto& npc : room.npcs) {
                    info.npcNames.push_back(npc->name);
                    if (!result.symbols.npcs.count(npc->name))
                        result.symbols.npcs[npc->name] = {npc->name, npc->line};
                }
                for (const auto& exit : room.exits) {
                    info.exitDirections.push_back(exit->direction);
                    info.exitTargets.push_back(exit->targetRoom);
                }
                result.symbols.rooms[room.name] = info;
            }
        } else if (decl->type == NodeType::ACTION_DECL) {
            const auto& action = static_cast<const ActionDeclNode&>(*decl);
            if (result.symbols.actions.count(action.name)) {
                addError(result, action.line, 0, "Duplicate action: \"" + action.name + "\"");
            } else {
                result.symbols.actions[action.name] = {action.name, action.line};
            }
        }
    }
}

static void validateCondition(const ConditionNode& cond, const SymbolTable& syms, SemanticResult& result) {
    for (const auto& clause : cond.clauses) {
        if (clause->kind == ConditionKind::HAS_ITEM_CHECK) {
            if (!syms.items.count(clause->itemName))
                addError(result, clause->line, 0,
                    "has_item references undeclared item: \"" + clause->itemName + "\"");
        } else if (clause->kind == ConditionKind::ROOM_CHECK) {
            if (clause->value && clause->value->kind == LiteralKind::STRING) {
                const std::string& target = clause->value->strValue;
                if (!syms.rooms.count(target))
                    addError(result, clause->line, 0,
                        "current_room compared to undeclared room: \"" + target + "\"");
            }
        }
    }
}

static void validateStatements(const std::vector<std::unique_ptr<ASTNode>>& stmts,
                                const SymbolTable& syms, SemanticResult& result) {
    for (const auto& stmt : stmts) {
        if (stmt->type == NodeType::REMOVE_STMT) {
            const auto& rem = static_cast<const RemoveStmtNode&>(*stmt);
            if (!syms.items.count(rem.itemName))
                addError(result, rem.line, 0,
                    "remove references undeclared item: \"" + rem.itemName + "\"");
        } else if (stmt->type == NodeType::IF_STMT) {
            const auto& ifStmt = static_cast<const IfStmtNode&>(*stmt);
            if (ifStmt.condition)
                validateCondition(*ifStmt.condition, syms, result);
            validateStatements(ifStmt.thenBranch, syms, result);
            validateStatements(ifStmt.elseBranch, syms, result);
        }
    }
}

static void validateDeclarations(const ProgramNode& ast, SemanticResult& result) {
    for (const auto& decl : ast.declarations) {
        if (decl->type == NodeType::ROOM_DECL) {
            const auto& room = static_cast<const RoomDeclNode&>(*decl);
            std::unordered_map<std::string, int> seenDirections;
            for (const auto& exit : room.exits) {
                if (seenDirections.count(exit->direction))
                    addError(result, exit->line, 0,
                        "Duplicate exit direction \"" + exit->direction + "\" in room \"" + room.name + "\"");
                else
                    seenDirections[exit->direction] = exit->line;
                if (!result.symbols.rooms.count(exit->targetRoom))
                    addError(result, exit->line, 0,
                        "Exit leads to undeclared room: \"" + exit->targetRoom + "\"");
            }
        } else if (decl->type == NodeType::ACTION_DECL) {
            const auto& action = static_cast<const ActionDeclNode&>(*decl);
            validateStatements(action.statementList, result.symbols, result);
        }
    }
}

SemanticResult analyse(const ProgramNode& ast) {
    SemanticResult result;
    registerDeclarations(ast, result);
    validateDeclarations(ast, result);
    for (const auto& d : result.diagnostics)
        std::cerr << (d.severity == Diagnostic::Severity::ERROR ? "Error" : "Warning")
                  << " at line " << d.line << ": " << d.message << "\n";
    return result;
}
