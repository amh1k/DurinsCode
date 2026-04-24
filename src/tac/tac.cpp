#include "tac/tac.h"

static int tempCount = 0;
static int labelCount = 0;

static std::string newTemp()  { return "t" + std::to_string(tempCount++); }
static std::string newLabel() { return "L" + std::to_string(labelCount++); }

static void emitCondition(const ConditionNode& cond, const std::string& falseLabel,
                           std::vector<TACInstruction>& out) {
    std::vector<std::string> clauseTemps;
    for (const auto& clause : cond.clauses) {
        std::string t = newTemp();
        if (clause->kind == ConditionKind::ROOM_CHECK) {
            TACInstruction instr;
            instr.op = TACOp::CHECK_ROOM;
            instr.dest = t;
            instr.strValue = clause->value ? clause->value->strValue : "";
            out.push_back(instr);
        } else if (clause->kind == ConditionKind::HAS_ITEM_CHECK) {
            TACInstruction instr;
            instr.op = TACOp::HAS_ITEM;
            instr.dest = t;
            instr.strValue = clause->itemName;
            out.push_back(instr);
        } else if (clause->kind == ConditionKind::PLAYER_ATTR_CHECK) {
            TACInstruction instr;
            switch (clause->op) {
                case CompareOp::EQUAL_EQUAL:   instr.op = TACOp::COMPARE_EQ;  break;
                case CompareOp::GREATER:       instr.op = TACOp::COMPARE_GT;  break;
                case CompareOp::GREATER_EQUAL: instr.op = TACOp::COMPARE_GTE; break;
                case CompareOp::LESS:          instr.op = TACOp::COMPARE_LT;  break;
                case CompareOp::LESS_EQUAL:    instr.op = TACOp::COMPARE_LTE; break;
                default:                       instr.op = TACOp::COMPARE_EQ;  break;
            }
            instr.dest = t;
            instr.src1 = clause->attribute;
            if (clause->value) {
                if (clause->value->kind == LiteralKind::NUMBER)
                    instr.intValue = clause->value->intValue;
                else if (clause->value->kind == LiteralKind::STRING)
                    instr.strValue = clause->value->strValue;
                else if (clause->value->kind == LiteralKind::BOOL_TRUE)
                    instr.boolValue = true;
            }
            out.push_back(instr);
        }
        clauseTemps.push_back(t);
    }

    std::string result = clauseTemps[0];
    for (size_t i = 1; i < clauseTemps.size(); ++i) {
        std::string combined = newTemp();
        TACInstruction andInstr;
        andInstr.op = TACOp::AND;
        andInstr.dest = combined;
        andInstr.src1 = result;
        andInstr.src2 = clauseTemps[i];
        out.push_back(andInstr);
        result = combined;
    }

    TACInstruction jump;
    jump.op = TACOp::JUMP_IF_FALSE;
    jump.src1 = result;
    jump.dest = falseLabel;
    out.push_back(jump);
}

static void emitStatements(const std::vector<std::unique_ptr<ASTNode>>& stmts,
                            std::vector<TACInstruction>& out);

static void emitStatement(const ASTNode& stmt, std::vector<TACInstruction>& out) {
    if (stmt.type == NodeType::PRINT_STMT) {
        const auto& p = static_cast<const PrintStmtNode&>(stmt);
        TACInstruction instr; instr.op = TACOp::PRINT; instr.strValue = p.message;
        out.push_back(instr);
    } else if (stmt.type == NodeType::REMOVE_STMT) {
        const auto& r = static_cast<const RemoveStmtNode&>(stmt);
        TACInstruction instr; instr.op = TACOp::REMOVE_ITEM; instr.strValue = r.itemName;
        out.push_back(instr);
    } else if (stmt.type == NodeType::ASSIGN_STMT) {
        const auto& a = static_cast<const AssignStmtNode&>(stmt);
        TACInstruction instr;
        instr.op = TACOp::SET_PLAYER_ATTR;
        instr.dest = a.attribute;
        if (a.op == AssignOp::PLUS_ASSIGN)       instr.src1 = "+=";
        else if (a.op == AssignOp::MINUS_ASSIGN)  instr.src1 = "-=";
        else                                       instr.src1 = "=";
        if (a.value) {
            if (a.value->kind == LiteralKind::NUMBER)      instr.intValue = a.value->intValue;
            else if (a.value->kind == LiteralKind::STRING)  instr.strValue = a.value->strValue;
            else if (a.value->kind == LiteralKind::BOOL_TRUE)  instr.boolValue = true;
            else if (a.value->kind == LiteralKind::BOOL_FALSE) instr.boolValue = false;
            else if (a.value->kind == LiteralKind::IDENTIFIER) instr.strValue = a.value->strValue;
        }
        out.push_back(instr);
    } else if (stmt.type == NodeType::IF_STMT) {
        const auto& ifStmt = static_cast<const IfStmtNode&>(stmt);
        std::string falseLabel = newLabel();
        std::string endLabel   = newLabel();
        if (ifStmt.condition)
            emitCondition(*ifStmt.condition, falseLabel, out);
        emitStatements(ifStmt.thenBranch, out);
        if (!ifStmt.elseBranch.empty()) {
            TACInstruction jumpEnd; jumpEnd.op = TACOp::JUMP; jumpEnd.dest = endLabel;
            out.push_back(jumpEnd);
        }
        TACInstruction labelFalse; labelFalse.op = TACOp::LABEL; labelFalse.dest = falseLabel;
        out.push_back(labelFalse);
        if (!ifStmt.elseBranch.empty()) {
            emitStatements(ifStmt.elseBranch, out);
            TACInstruction labelEnd; labelEnd.op = TACOp::LABEL; labelEnd.dest = endLabel;
            out.push_back(labelEnd);
        }
    }
}

static void emitStatements(const std::vector<std::unique_ptr<ASTNode>>& stmts,
                            std::vector<TACInstruction>& out) {
    for (const auto& stmt : stmts)
        if (stmt) emitStatement(*stmt, out);
}

TACProgram generateTAC(const ProgramNode& ast, const SymbolTable& symbols) {
    (void)symbols;
    tempCount = 0; labelCount = 0;
    TACProgram prog;
    for (const auto& decl : ast.declarations) {
        if (decl->type == NodeType::ACTION_DECL) {
            const auto& action = static_cast<const ActionDeclNode&>(*decl);
            TACAction tacAction;
            tacAction.name = action.name;
            emitStatements(action.statementList, tacAction.instructions);
            prog.actions.push_back(std::move(tacAction));
        }
    }
    return prog;
}
