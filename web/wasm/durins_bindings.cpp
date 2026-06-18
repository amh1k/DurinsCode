#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include "codegen/codegen.h"
#include "parser/parser.h"
#include "semantic/semantic.h"
#include "tac/tac.h"

using json = nlohmann::json;

static char *copyString(const std::string &value)
{
    char *out = static_cast<char *>(std::malloc(value.size() + 1));
    if (!out)
        return nullptr;
    std::memcpy(out, value.c_str(), value.size() + 1);
    return out;
}

static std::string tacToText(const TACProgram &prog)
{
    std::ostringstream out;
    out << "=== TAC (optimized) ===\n";
    for (const auto &action : prog.actions)
    {
        out << "action \"" << action.name << "\":\n";
        for (const auto &instr : action.instructions)
        {
            out << "  " << instr.dest << " <- [" << instr.src1 << "]";
            if (!instr.src2.empty())
                out << " [" << instr.src2 << "]";
            if (!instr.strValue.empty())
                out << " " << instr.strValue;
            if (instr.intValue != 0)
                out << " " << instr.intValue;
            if (instr.boolValue)
                out << " true";
            out << "\n";
        }
    }
    return out.str();
}

static std::string symbolsToText(const SymbolTable &symbols)
{
    std::ostringstream out;
    out << "=== Symbol Table ===\n";
    for (const auto &[name, _] : symbols.rooms)
        out << "  room:   " << name << "\n";
    for (const auto &[name, _] : symbols.items)
        out << "  item:   " << name << "\n";
    for (const auto &[name, _] : symbols.npcs)
        out << "  npc:    " << name << "\n";
    for (const auto &[name, _] : symbols.actions)
        out << "  action: " << name << "\n";
    return out.str();
}

static std::string compileInternal(const char *source, bool includeDebug)
{
    std::ostringstream capturedStdout;
    std::ostringstream capturedStderr;
    auto *oldCout = std::cout.rdbuf(capturedStdout.rdbuf());
    auto *oldCerr = std::cerr.rdbuf(capturedStderr.rdbuf());

    json response;
    response["ok"] = false;
    response["bytecode"] = "";
    response["diagnostics"] = "";
    response["debug"] = "";

    try
    {
        auto parsed = parseProgram(source ? source : "");
        if (parsed.hadError)
        {
            response["diagnostics"] = capturedStderr.str() + "Parse failed.";
        }
        else
        {
            auto sem = analyse(*parsed.ast);
            if (sem.hadError)
            {
                response["diagnostics"] = capturedStderr.str() +
                                          "Compilation failed: " +
                                          std::to_string(sem.diagnostics.size()) +
                                          " error(s).";
            }
            else
            {
                auto tac = generateTAC(*parsed.ast, sem.symbols);
                auto opt = optimizeTAC(tac);
                auto cg = generateBytecode(*parsed.ast, sem.symbols, opt);

                response["ok"] = true;
                response["bytecode"] = cg.json;
                response["diagnostics"] = capturedStderr.str();
                if (includeDebug)
                    response["debug"] = symbolsToText(sem.symbols) + "\n" + tacToText(opt);
            }
        }
    }
    catch (const std::exception &e)
    {
        response["diagnostics"] = capturedStderr.str() + std::string("Internal compiler error: ") + e.what();
    }
    catch (...)
    {
        response["diagnostics"] = capturedStderr.str() + "Internal compiler error.";
    }

    std::cout.rdbuf(oldCout);
    std::cerr.rdbuf(oldCerr);

    std::string stdoutText = capturedStdout.str();
    if (!stdoutText.empty())
        response["stdout"] = stdoutText;

    return response.dump();
}

extern "C"
{
    char *compile_source(const char *source)
    {
        return copyString(compileInternal(source, false));
    }

    char *compile_debug(const char *source)
    {
        return copyString(compileInternal(source, true));
    }

    void free_string(char *ptr)
    {
        std::free(ptr);
    }
}
