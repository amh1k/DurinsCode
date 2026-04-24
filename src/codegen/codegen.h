#ifndef durin_codegen_h
#define durin_codegen_h

#include <parser/parser.h>
#include <semantic/semantic.h>
#include <tac/tac.h>
#include <string>

struct CodegenResult {
    std::string json;
    bool hadError = false;
};

CodegenResult generateBytecode(const ProgramNode& ast,
                                const SymbolTable& symbols,
                                const TACProgram& tac);

bool writeBytecodeFile(const std::string& path, const CodegenResult& result);

#endif
