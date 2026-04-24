# Durin's Code — 8-Bit Adventure DSL Compiler

**CS4031 Compiler Construction · Spring 2026**

| Member | ID |
|---|---|
| Abdul Moiz Hussain | 23K-0553 |
| Muhammed Abdullah Khan | 23K-0607 |
| Huzaifa Abdul Rehman | 23K-0782 |

---

## Overview

Durin's Code (`dc`) is a domain-specific language and compiler for authoring interactive text-adventure games. A `.dc` source file declares rooms, items, NPCs, and scripted actions; the compiler validates the world, generates Three-Address Code (TAC), optimises it, serialises it to JSON bytecode, and runs it inside a terminal Virtual Machine.

```
source.dc  →  Lexer  →  Parser  →  Semantic  →  TAC  →  Optimiser  →  JSON Bytecode  →  VM
```

---

## Prerequisites

| Tool | Version |
|---|---|
| CMake | ≥ 3.23 |
| C++ compiler | C++17 (Clang 14+ / GCC 11+) |
| GoogleTest | system-installed (`libgtest-dev`) |

GoogleTest install (Ubuntu/macOS):
```bash
# Ubuntu
sudo apt install libgtest-dev

# macOS (Homebrew)
brew install googletest
```

---

## Build

```bash
cd "Compiler construction project/DurinsCode-main"
cmake -S . -B build
cmake --build build
```

Binaries produced in `build/`:

| Binary | Purpose |
|---|---|
| `durinsc` | Main compiler + runtime CLI |
| `lexer_tests` | Lexer test suite |
| `parser_tests` | Parser test suite |
| `semantic_tests` | Semantic analyser test suite |
| `tac_tests` | TAC/IR test suite |
| `codegen_tests` | Code generation test suite |
| `vm_tests` | Virtual machine test suite |

---

## Running Tests

```bash
cd build
ctest --output-on-failure
```

Or run individual suites:
```bash
./lexer_tests
./parser_tests
./semantic_tests
./tac_tests
./codegen_tests
./vm_tests
```

---

## CLI Usage

### Compile and run immediately
```bash
./durinsc game.dc
```

### Compile to bytecode file
```bash
./durinsc game.dc -o game.json
```

### Run from pre-compiled bytecode
```bash
./durinsc --run game.json
```

### Debug mode (symbol table + TAC dump)
```bash
./durinsc game.dc --debug
```

### Interactive REPL
```bash
./durinsc --interactive
```

---

## Game Runtime Commands

Once the VM starts, the player types commands at the `> ` prompt:

| Command | Effect |
|---|---|
| `look` | Describe the current room |
| `inventory` | List items the player is carrying |
| `go <direction>` | Move to a connected room (e.g. `go east`) |
| `quit` | Exit the game |
| any action name | Execute a scripted action (e.g. `take ring`) |

---

## Language Quick Reference

```dc
// Single-line comment

room "room_id" {
    description "Room description text."
    item item_name { key: value, key: value }
    npc  npc_name  { health: 100, hostile: false }
    exit north "other_room_id"
}

action "action name" {
    if current_room == "room_id" && player.has_item(item_name) {
        remove item_name
        player.inventory += other_item
        player.win = true
        print "Message to the player."
    } else {
        print "Fallback message."
    }
}
```

See [docs/language-reference.md](docs/language-reference.md) for the full grammar.

---

## Project Structure

```
src/
  lexer/      Tokeniser (hand-written)
  parser/     Recursive-descent parser → typed AST
  semantic/   Two-pass semantic analyser + symbol table
  tac/        TAC IR generator + optimiser
  codegen/    JSON bytecode serialiser (nlohmann/json)
  vm/         Virtual machine + game loop
  main.cpp    CLI entry point
test/         GoogleTest suites (one per phase)
third_party/  nlohmann/json v3.11.3 (header-only, vendored)
docs/         Language reference, architecture document
```

---

## Compiler Phases

1. **Lexer** — tokenises `.dc` source; handles keywords, strings, numbers, operators, `//` comments, `\r\n` line endings, context-sensitive dot tokens (`TOKEN_PLAYER_ATTR`, `TOKEN_ROOM_ATTR`).
2. **Parser** — recursive-descent; produces a fully typed AST using `unique_ptr` nodes.
3. **Semantic Analyser** — two passes: (1) register all declarations, detect duplicates; (2) validate exit targets, remove-statement references, condition references.
4. **TAC Generator** — flattens AST into Three-Address Code; 18 opcodes; `t0`, `t1`, … temporaries for complex conditions.
5. **Optimiser** — two passes: dead-code elimination (unreachable instructions after unconditional JUMP) and redundant-AND folding (collapse `AND t, t, t` → alias).
6. **Code Generator** — serialises world graph and optimised TAC to structured JSON bytecode.
7. **Virtual Machine** — loads JSON bytecode; builds label→index jump table; executes instructions; runs interactive game loop.
