#include "codegen/codegen.h"
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

static json serializeRoom(const RoomDeclNode& room) {
    json j;
    j["name"] = room.name;
    j["description"] = room.description ? room.description->text : "";
    json items = json::array();
    for (const auto& item : room.items) {
        json ji; ji["name"] = item->name;
        json props = json::object();
        for (const auto& prop : item->properties) {
            if (!prop || !prop->value) continue;
            if (prop->value->kind == LiteralKind::NUMBER)      props[prop->key] = prop->value->intValue;
            else if (prop->value->kind == LiteralKind::STRING)  props[prop->key] = prop->value->strValue;
            else if (prop->value->kind == LiteralKind::BOOL_TRUE) props[prop->key] = true;
            else if (prop->value->kind == LiteralKind::BOOL_FALSE) props[prop->key] = false;
            else props[prop->key] = prop->value->strValue;
        }
        ji["properties"] = props; items.push_back(ji);
    }
    j["items"] = items;
    json npcs = json::array();
    for (const auto& npc : room.npcs) {
        json jn; jn["name"] = npc->name;
        json props = json::object();
        for (const auto& prop : npc->properties) {
            if (!prop || !prop->value) continue;
            if (prop->value->kind == LiteralKind::NUMBER)      props[prop->key] = prop->value->intValue;
            else if (prop->value->kind == LiteralKind::STRING)  props[prop->key] = prop->value->strValue;
            else if (prop->value->kind == LiteralKind::BOOL_TRUE) props[prop->key] = true;
            else if (prop->value->kind == LiteralKind::BOOL_FALSE) props[prop->key] = false;
        }
        jn["properties"] = props; npcs.push_back(jn);
    }
    j["npcs"] = npcs;
    json exits = json::array();
    for (const auto& exit : room.exits) {
        json je; je["direction"] = exit->direction; je["target"] = exit->targetRoom;
        exits.push_back(je);
    }
    j["exits"] = exits;
    return j;
}

static std::string opName(TACOp op) {
    switch (op) {
        case TACOp::ASSIGN:          return "ASSIGN";
        case TACOp::ADD:             return "ADD";
        case TACOp::SUB:             return "SUB";
        case TACOp::COMPARE_EQ:      return "COMPARE_EQ";
        case TACOp::COMPARE_GT:      return "COMPARE_GT";
        case TACOp::COMPARE_GTE:     return "COMPARE_GTE";
        case TACOp::COMPARE_LT:      return "COMPARE_LT";
        case TACOp::COMPARE_LTE:     return "COMPARE_LTE";
        case TACOp::JUMP_IF_FALSE:   return "JUMP_IF_FALSE";
        case TACOp::JUMP:            return "JUMP";
        case TACOp::LABEL:           return "LABEL";
        case TACOp::PRINT:           return "PRINT";
        case TACOp::REMOVE_ITEM:     return "REMOVE_ITEM";
        case TACOp::SET_PLAYER_ATTR: return "SET_PLAYER_ATTR";
        case TACOp::HAS_ITEM:        return "HAS_ITEM";
        case TACOp::CHECK_ROOM:      return "CHECK_ROOM";
        case TACOp::AND:             return "AND";
        case TACOp::OR:              return "OR";
        default:                     return "UNKNOWN";
    }
}

static json serializeInstr(const TACInstruction& instr) {
    json j;
    j["op"] = opName(instr.op); j["dest"] = instr.dest;
    j["src1"] = instr.src1; j["src2"] = instr.src2;
    j["int"] = instr.intValue; j["bool"] = instr.boolValue; j["str"] = instr.strValue;
    return j;
}

CodegenResult generateBytecode(const ProgramNode& ast,
                                const SymbolTable& symbols,
                                const TACProgram& tac) {
    (void)symbols;
    json bytecode;
    json rooms = json::array();
    for (const auto& decl : ast.declarations)
        if (decl->type == NodeType::ROOM_DECL)
            rooms.push_back(serializeRoom(static_cast<const RoomDeclNode&>(*decl)));
    bytecode["world"] = rooms;
    bytecode["start_room"] = rooms.empty() ? "" : rooms[0]["name"];
    json actions = json::array();
    for (const auto& ta : tac.actions) {
        json ja; ja["name"] = ta.name;
        json instrs = json::array();
        for (const auto& i : ta.instructions) instrs.push_back(serializeInstr(i));
        ja["instructions"] = instrs;
        actions.push_back(ja);
    }
    bytecode["actions"] = actions;
    CodegenResult result;
    result.json = bytecode.dump(2);
    return result;
}

bool writeBytecodeFile(const std::string& path, const CodegenResult& result) {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << result.json;
    return true;
}
