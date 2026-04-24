#include "vm/vm.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cctype>

using json = nlohmann::json;

// Convert internal snake_case ids to "Title Case" for display
static std::string pretty(const std::string& id) {
    std::string out;
    bool cap = true;
    for (char c : id) {
        if (c == '_') { out += ' '; cap = true; }
        else { out += cap ? (char)std::toupper(c) : c; cap = false; }
    }
    return out;
}

// ── ANSI colour helpers ───────────────────────────────────────
// Palette: amber (room/banner), green (exits/ok), red (danger/error),
//          everything else is neutral white or dim gray.
namespace C {
    static const char* RESET   = "\033[0m";
    static const char* BOLD    = "\033[1m";
    static const char* DIM     = "\033[2m";
    static const char* AMBER   = "\033[1;33m";   // room name, banner, items
    static const char* WHITE   = "\033[0;97m";   // description, narration
    static const char* GREEN   = "\033[0;32m";   // exits, success (muted, not bright)
    static const char* RED     = "\033[0;31m";   // NPCs, errors, danger (muted)
    static const char* GRAY    = "\033[0;90m";   // separators, hints, prompt
}

bool VM::loadBytecode(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) { std::cerr << "Error: cannot open: " << path << "\n"; return false; }
    std::stringstream ss; ss << f.rdbuf();
    return loadBytecodeFromString(ss.str());
}

bool VM::loadBytecodeFromString(const std::string& jsonStr) {
    try { bytecode_ = json::parse(jsonStr); loaded_ = true; return true; }
    catch (const std::exception& e) { std::cerr << "Bad JSON: " << e.what() << "\n"; return false; }
}

void VM::describeRoom(const std::string& roomName) const {
    for (const auto& room : bytecode_["world"]) {
        if (room["name"] != roomName) continue;

        std::cout << "\n"
                  << C::GRAY << "  ─────────────────────────────────────\n" << C::RESET
                  << C::AMBER << C::BOLD << "  " << pretty(roomName) << C::RESET << "\n"
                  << C::GRAY << "  ─────────────────────────────────────\n" << C::RESET
                  << C::WHITE << "  " << room["description"].get<std::string>() << C::RESET << "\n";

        if (!room["items"].empty()) {
            std::cout << C::GRAY << "  You see  " << C::RESET;
            for (size_t i = 0; i < room["items"].size(); ++i) {
                if (i) std::cout << C::GRAY << ",  " << C::RESET;
                std::cout << C::WHITE << pretty(room["items"][i]["name"].get<std::string>()) << C::RESET;
            }
            std::cout << "\n";
        }
        if (!room["npcs"].empty()) {
            std::cout << C::RED << "  ⚠ Beware  " << C::RESET;
            for (size_t i = 0; i < room["npcs"].size(); ++i) {
                if (i) std::cout << C::GRAY << ",  " << C::RESET;
                std::cout << C::RED << pretty(room["npcs"][i]["name"].get<std::string>()) << C::RESET;
            }
            std::cout << "\n";
        }
        bool first = true;
        for (const auto& exit : room["exits"]) {
            std::cout << C::GRAY << (first ? "  " : "  ") << C::RESET
                      << C::GREEN << "go " << exit["direction"].get<std::string>() << C::RESET
                      << C::GRAY  << " → " << pretty(exit["target"].get<std::string>()) << C::RESET
                      << "\n";
            first = false;
        }
        return;
    }
    std::cout << C::RED << "  (Unknown room: " << pretty(roomName) << ")" << C::RESET << "\n";
}

std::vector<std::string> VM::exitsFrom(const std::string& roomName) const {
    std::vector<std::string> exits;
    for (const auto& room : bytecode_["world"])
        if (room["name"] == roomName)
            for (const auto& exit : room["exits"])
                exits.push_back(exit["direction"].get<std::string>());
    return exits;
}

bool VM::executeAction(const std::string& actionName, GameState& state) const {
    for (const auto& action : bytecode_["actions"])
        if (action["name"] == actionName) { executeInstructions(action["instructions"], state); return true; }
    return false;
}

void VM::executeInstructions(const json& instructions, GameState& state) const {
    std::unordered_map<std::string, size_t> labelMap;
    for (size_t i = 0; i < instructions.size(); ++i)
        if (instructions[i]["op"] == "LABEL")
            labelMap[instructions[i]["dest"].get<std::string>()] = i;

    std::unordered_map<std::string, bool> temps;
    size_t ip = 0;
    while (ip < instructions.size()) {
        const auto& instr = instructions[ip];
        std::string op   = instr["op"].get<std::string>();
        std::string dest = instr["dest"].get<std::string>();
        std::string src1 = instr["src1"].get<std::string>();
        std::string str  = instr["str"].get<std::string>();
        bool bval        = instr["bool"].get<bool>();
        int  ival        = instr["int"].get<int>();

        if (op == "PRINT") {
            std::cout << C::WHITE << "  " << str << C::RESET << "\n";
        } else if (op == "REMOVE_ITEM") {
            state.inventory.erase(str);
        } else if (op == "SET_PLAYER_ATTR") {
            if (src1 == "=") {
                if (!str.empty())
                    state.strAttrs[dest] = str;
                else if (ival != 0)
                    state.intAttrs[dest] = ival;
                else
                    state.boolAttrs[dest] = bval;
            } else if (src1 == "+=") {
                if (!str.empty()) state.inventory.insert(str);
                else              state.intAttrs[dest] += ival;
            } else if (src1 == "-=") {
                state.intAttrs[dest] -= ival;
            }
        } else if (op == "CHECK_ROOM") {
            temps[dest] = (state.currentRoom == str);
        } else if (op == "HAS_ITEM") {
            temps[dest] = (state.inventory.count(str) > 0);
        } else if (op == "COMPARE_EQ") {
            if (state.intAttrs.count(src1))
                temps[dest] = (state.intAttrs.at(src1) == ival);
            else if (state.boolAttrs.count(src1))
                temps[dest] = (state.boolAttrs.at(src1) == bval);
            else
                temps[dest] = false;
        } else if (op == "COMPARE_GT")  { temps[dest] = state.intAttrs.count(src1) && state.intAttrs.at(src1) >  ival; }
          else if (op == "COMPARE_GTE") { temps[dest] = state.intAttrs.count(src1) && state.intAttrs.at(src1) >= ival; }
          else if (op == "COMPARE_LT")  { temps[dest] = state.intAttrs.count(src1) && state.intAttrs.at(src1) <  ival; }
          else if (op == "COMPARE_LTE") { temps[dest] = state.intAttrs.count(src1) && state.intAttrs.at(src1) <= ival; }
          else if (op == "AND") {
            std::string s2 = instr["src2"].get<std::string>();
            temps[dest] = (temps.count(src1) && temps[src1]) && (temps.count(s2) && temps[s2]);
        } else if (op == "OR") {
            std::string s2 = instr["src2"].get<std::string>();
            temps[dest] = (temps.count(src1) && temps[src1]) || (temps.count(s2) && temps[s2]);
        } else if (op == "JUMP_IF_FALSE") {
            bool cond = temps.count(src1) ? temps[src1] : false;
            if (!cond && labelMap.count(dest)) { ip = labelMap[dest]; continue; }
        } else if (op == "JUMP") {
            if (labelMap.count(dest)) { ip = labelMap[dest]; continue; }
        }
        ++ip;
    }
}

static void printHints(const json& bc, const GameState& state) {
    std::vector<std::string> cmds;

    // real exit commands first  e.g. "go east"
    for (const auto& room : bc["world"]) {
        if (room["name"] != state.currentRoom) continue;
        for (const auto& exit : room["exits"])
            cmds.push_back("go " + exit["direction"].get<std::string>());
    }

    // available actions for this room / inventory state
    for (const auto& action : bc["actions"]) {
        bool roomOk = true, hasRoomCheck = false, hasItemCheck = false, itemPresent = true;
        for (const auto& instr : action["instructions"]) {
            std::string op = instr["op"].get<std::string>();
            if (op == "CHECK_ROOM") {
                hasRoomCheck = true;
                roomOk = (instr["str"].get<std::string>() == state.currentRoom);
                break;
            }
            if (op == "HAS_ITEM" && !hasItemCheck) {
                hasItemCheck = true;
                itemPresent = state.inventory.count(instr["str"].get<std::string>()) > 0;
            }
        }
        if (!hasRoomCheck && hasItemCheck && !itemPresent) continue;
        if (roomOk) cmds.push_back(action["name"].get<std::string>());
    }

    // always-available commands
    cmds.push_back("look");
    cmds.push_back("inventory");

    std::cout << C::GRAY << "  ·";
    for (const auto& c : cmds)
        std::cout << "  " << c << "  ·";
    std::cout << "  quit" << C::RESET << "\n";
}

void runGameLoop(VM& vm) {
    const auto& bc = vm.bytecode();
    GameState state;
    state.currentRoom = bc["start_room"].get<std::string>();

    // ── Title banner ─────────────────────────────────────────
    std::cout << "\n"
              << C::AMBER
              << "  ╔══════════════════════════════════════╗\n"
              << "  ║       D U R I N ' S   C O D E        ║\n"
              << "  ║        8-Bit Adventure Engine        ║\n"
              << "  ╚══════════════════════════════════════╝\n"
              << C::RESET << "\n"
              << C::GRAY
              << "  Read the room. Type any command shown at the bottom.\n"
              << C::RESET << "\n";

    vm.describeRoom(state.currentRoom);
    printHints(bc, state);

    std::string input;
    while (true) {
        std::cout << "\n" << C::AMBER << "> " << C::RESET;
        std::cout.flush();
        if (!std::getline(std::cin, input)) break;
        if (input.empty()) continue;

        // ── built-in: help ─────────────────────────────────
        if (input == "help" || input == "?") {
            std::cout << C::GRAY
                      << "  look            — describe current room\n"
                      << "  go <direction>  — move to another room\n"
                      << "  inventory       — show your bag\n"
                      << "  quit            — leave the game\n"
                      << "  Read the room description carefully for clues.\n"
                      << C::RESET;
            continue;
        }

        // ── built-in: quit ─────────────────────────────────
        if (input == "quit" || input == "exit") {
            std::cout << C::GRAY << "\n  Farewell, adventurer.\n" << C::RESET;
            break;
        }

        // ── built-in: go ───────────────────────────────────
        if (input.rfind("go ", 0) == 0) {
            std::string dir = input.substr(3);
            bool moved = false;
            for (const auto& room : bc["world"]) {
                if (room["name"] != state.currentRoom) continue;
                for (const auto& exit : room["exits"]) {
                    if (exit["direction"] == dir) {
                        state.currentRoom = exit["target"].get<std::string>();
                        vm.describeRoom(state.currentRoom);
                        printHints(bc, state);
                        moved = true;
                    }
                }
            }
            if (!moved) {
                std::cout << C::RED << "  Can't go \"" << dir << "\" from here." << C::RESET
                          << C::GRAY << "  You can go: ";
                bool f = true;
                for (const auto& room : bc["world"]) {
                    if (room["name"] != state.currentRoom) continue;
                    for (const auto& exit : room["exits"]) {
                        if (!f) std::cout << ",  ";
                        std::cout << exit["direction"].get<std::string>();
                        f = false;
                    }
                }
                std::cout << C::RESET << "\n";
            }
            continue;
        }

        // ── built-in: look ─────────────────────────────────
        if (input == "look") {
            vm.describeRoom(state.currentRoom);
            printHints(bc, state);
            continue;
        }

        // ── built-in: inventory ────────────────────────────
        if (input == "inventory" || input == "inv") {
            if (state.inventory.empty()) {
                std::cout << C::GRAY << "  bag  " << C::RESET
                          << C::GRAY << "empty\n" << C::RESET;
            } else {
                std::cout << C::GRAY << "  bag  " << C::RESET;
                bool f = true;
                for (auto& item : state.inventory) {
                    if (!f) std::cout << C::GRAY << "  " << C::RESET;
                    std::cout << C::WHITE << item << C::RESET;
                    f = false;
                }
                std::cout << "\n";
            }
            continue;
        }

        // ── scripted action ────────────────────────────────
        if (!vm.executeAction(input, state)) {
            std::cout << C::RED << "  \"" << input << "\" — not a valid command here." << C::RESET << "\n";
            printHints(bc, state);
        }

        // ── win check ──────────────────────────────────────
        if (state.boolAttrs.count("win") && state.boolAttrs["win"]) {
            std::cout << "\n"
                      << C::AMBER << C::BOLD
                      << "  ╔══════════════════════════════════════╗\n"
                      << "  ║   YOU WIN!  The quest is complete.   ║\n"
                      << "  ╚══════════════════════════════════════╝\n"
                      << C::RESET << "\n";
            break;
        }
    }
}
