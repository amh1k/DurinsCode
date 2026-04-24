#ifndef durin_tac_h
#define durin_tac_h

#include <parser/parser.h>
#include <semantic/semantic.h>
#include <string>
#include <vector>

enum class TACOp {
    ASSIGN, ADD, SUB,
    COMPARE_EQ, COMPARE_GT, COMPARE_GTE, COMPARE_LT, COMPARE_LTE,
    JUMP_IF_FALSE, JUMP, LABEL,
    PRINT, REMOVE_ITEM, SET_PLAYER_ATTR,
    HAS_ITEM, CHECK_ROOM,
    AND, OR,
};

struct TACInstruction {
    TACOp op;
    std::string dest;
    std::string src1;
    std::string src2;
    int intValue = 0;
    bool boolValue = false;
    std::string strValue;
};

struct TACAction {
    std::string name;
    std::vector<TACInstruction> instructions;
};

struct TACProgram {
    std::vector<TACAction> actions;
};

TACProgram generateTAC(const ProgramNode& ast, const SymbolTable& symbols);

// Optimization pass — run after generateTAC, before codegen
TACProgram optimizeTAC(const TACProgram& prog);

#endif
