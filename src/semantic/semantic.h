#ifndef durin_semantic_h
#define durin_semantic_h

#include <parser/parser.h>
#include <string>
#include <unordered_map>
#include <vector>

struct Diagnostic {
    enum class Severity { WARNING, ERROR };
    Severity severity;
    int line;
    int column;
    std::string message;
};

struct RoomInfo {
    std::string name;
    int line;
    std::vector<std::string> itemNames;
    std::vector<std::string> npcNames;
    std::vector<std::string> exitDirections;
    std::vector<std::string> exitTargets;
};

struct ItemInfo   { std::string name; int line; };
struct NpcInfo    { std::string name; int line; };
struct ActionInfo { std::string name; int line; };

struct SymbolTable {
    std::unordered_map<std::string, RoomInfo>   rooms;
    std::unordered_map<std::string, ItemInfo>   items;
    std::unordered_map<std::string, NpcInfo>    npcs;
    std::unordered_map<std::string, ActionInfo> actions;
    std::unordered_map<std::string, std::string> playerAttrTypes;
};

struct SemanticResult {
    bool hadError = false;
    std::vector<Diagnostic> diagnostics;
    SymbolTable symbols;
};

SemanticResult analyse(const ProgramNode& ast);

#endif
