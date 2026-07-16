#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "parser/parser.h"
#include "semantic/semantic.h"
#include "tac/tac.h"
#include "codegen/codegen.h"
#include "vm/vm.h"

static void printUsage(const char *prog)
{
    std::cerr << "Usage:\n"
              << "  " << prog << " <source.dc>               compile + run\n"
              << "  " << prog << " <source.dc> -o <out.json>  compile to bytecode\n"
              << "  " << prog << " --run <bytecode.json>       run from bytecode\n"
              << "  " << prog << " <source.dc> --debug         show symbol table + TAC\n"
              << "  " << prog << " --interactive               REPL mode\n";
}

static void runREPL()
{
    std::cout << "=== Durin's Code REPL ===\n"
              << "Type a Durin snippet and press Enter twice to compile & run.\n"
              << "Type 'quit' at any time to exit.\n\n";

    std::string source;
    std::string line;
    bool prevEmpty = false;

    while (true)
    {
        std::cout << "dc> ";
        std::cout.flush();
        if (!std::getline(std::cin, line))
            break;

        if (line == "quit" || line == "exit")
            break;

        if (line.empty())
        {
            if (prevEmpty && !source.empty())
            {
                // ── Compile & Hand off to VM ─────────────────────
                auto parsed = parseProgram(source.c_str());
                if (!parsed.hadError)
                {
                    auto sem = analyse(*parsed.ast);
                    if (!sem.hadError)
                    {
                        auto tac = generateTAC(*parsed.ast, sem.symbols);
                        auto opt = optimizeTAC(tac);
                        auto cg = generateBytecode(*parsed.ast, sem.symbols, opt);

                        VM vm;
                        if (vm.loadBytecodeFromString(cg.json))
                        {
                            // VM handles ALL input, movement, hints, actions & win state
                            runGameLoop(vm);
                        }
                        else
                        {
                            std::cout << "Failed to load bytecode.\n";
                        }
                    }
                }
                // Reset for next snippet
                source.clear();
                prevEmpty = false;
                std::cout << "\n";
            }
            else
            {
                prevEmpty = true;
            }
        }
        else
        {
            source += line + "\n";
            prevEmpty = false;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printUsage(argv[0]);
        return 1;
    }

    std::string arg1 = argv[1];

    if (arg1 == "--interactive")
    {
        runREPL();
        return 0;
    }

    if (arg1 == "--run")
    {
        if (argc < 3)
        {
            std::cerr << "Error: --run needs a file.\n";
            return 1;
        }
        VM vm;
        if (!vm.loadBytecode(argv[2]))
            return 1;
        runGameLoop(vm);
        return 0;
    }

    std::ifstream srcFile(arg1);
    if (!srcFile.is_open())
    {
        std::cerr << "Error: cannot open: " << arg1 << "\n";
        return 1;
    }
    std::stringstream ss;
    ss << srcFile.rdbuf();
    std::string source = ss.str();

    auto parsed = parseProgram(source.c_str());
    if (parsed.hadError)
    {
        std::cerr << "Parse failed.\n";
        return 1;
    }

    auto sem = analyse(*parsed.ast);
    if (sem.hadError)
    {
        std::cerr << "Compilation failed: " << sem.diagnostics.size() << " error(s).\n";
        return 1;
    }

    auto tac = generateTAC(*parsed.ast, sem.symbols);
    auto opt = optimizeTAC(tac);

    bool debug = false;
    for (int i = 2; i < argc; ++i)
        if (std::string(argv[i]) == "--debug")
            debug = true;

    if (debug)
    {
        std::cout << "=== Symbol Table ===\n";
        for (const auto &[name, _] : sem.symbols.rooms)
            std::cout << "  room:   " << name << "\n";
        for (const auto &[name, _] : sem.symbols.items)
            std::cout << "  item:   " << name << "\n";
        for (const auto &[name, _] : sem.symbols.actions)
            std::cout << "  action: " << name << "\n";
        std::cout << "\n=== TAC (optimized) ===\n";
        for (const auto &action : opt.actions)
        {
            std::cout << "action \"" << action.name << "\":\n";
            for (const auto &instr : action.instructions)
                std::cout << "  " << instr.dest << " <- [" << instr.src1 << "] " << instr.strValue << "\n";
        }
        std::cout << "\n";
    }

    auto cg = generateBytecode(*parsed.ast, sem.symbols, opt);

    std::string outFile;
    for (int i = 2; i < argc - 1; ++i)
        if (std::string(argv[i]) == "-o")
            outFile = argv[i + 1];

    if (!outFile.empty())
    {
        if (!writeBytecodeFile(outFile, cg))
        {
            std::cerr << "Error writing " << outFile << "\n";
            return 1;
        }
        std::cout << "Bytecode written to " << outFile << "\n";
        return 0;
    }

    VM vm;
    if (!vm.loadBytecodeFromString(cg.json))
        return 1;
    runGameLoop(vm);
    return 0;
}
