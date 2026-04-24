#include "tac/tac.h"
#include <unordered_map>
#include <unordered_set>

// Optimization 1: Dead code elimination
// Remove instructions that appear after an unconditional JUMP and before the next LABEL.
// These are unreachable and can be safely deleted.
static std::vector<TACInstruction> eliminateDeadCode(const std::vector<TACInstruction>& instrs) {
    std::vector<TACInstruction> result;
    bool dead = false;
    for (const auto& instr : instrs) {
        if (instr.op == TACOp::LABEL) {
            dead = false; // labels are reachable via jumps
        }
        if (!dead) {
            result.push_back(instr);
        }
        if (instr.op == TACOp::JUMP) {
            dead = true; // instructions after unconditional jump are dead
        }
    }
    return result;
}

// Optimization 2: Redundant AND elimination (constant folding for booleans)
// If an AND instruction combines two temps that are both set by the same prior instruction
// (e.g., t0 AND t0), collapse it to just use t0.
// More practically: if src1 == src2 in an AND, replace dest with src1 throughout.
static std::vector<TACInstruction> foldRedundantAnds(const std::vector<TACInstruction>& instrs) {
    std::vector<TACInstruction> result;
    std::unordered_map<std::string, std::string> aliases; // dest -> canonical temp

    auto resolve = [&](const std::string& t) -> std::string {
        auto it = aliases.find(t);
        return (it != aliases.end()) ? it->second : t;
    };

    for (auto instr : instrs) {
        // Resolve aliases in operands
        instr.src1 = resolve(instr.src1);
        instr.src2 = resolve(instr.src2);

        if (instr.op == TACOp::AND && instr.src1 == instr.src2) {
            // AND of a temp with itself is just that temp
            aliases[instr.dest] = instr.src1;
            // Skip emitting this instruction entirely
            continue;
        }
        result.push_back(instr);
    }
    return result;
}

TACProgram optimizeTAC(const TACProgram& prog) {
    TACProgram optimized;
    for (const auto& action : prog.actions) {
        TACAction opt;
        opt.name = action.name;
        opt.instructions = eliminateDeadCode(action.instructions);
        opt.instructions = foldRedundantAnds(opt.instructions);
        optimized.actions.push_back(std::move(opt));
    }
    return optimized;
}
