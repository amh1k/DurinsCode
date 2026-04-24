#ifndef durin_vm_h
#define durin_vm_h

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <nlohmann/json.hpp>

struct GameState {
    std::string currentRoom;
    std::unordered_set<std::string> inventory;
    std::unordered_map<std::string, int> intAttrs;
    std::unordered_map<std::string, bool> boolAttrs;
    std::unordered_map<std::string, std::string> strAttrs;
};

class VM {
public:
    bool loadBytecode(const std::string& path);
    bool loadBytecodeFromString(const std::string& jsonStr);
    bool executeAction(const std::string& actionName, GameState& state) const;
    void describeRoom(const std::string& roomName) const;
    std::vector<std::string> exitsFrom(const std::string& roomName) const;
    const nlohmann::json& bytecode() const { return bytecode_; }

private:
    nlohmann::json bytecode_;
    bool loaded_ = false;
    void executeInstructions(const nlohmann::json& instructions, GameState& state) const;
};

void runGameLoop(VM& vm);

#endif
