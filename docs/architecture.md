# Durin's Code — Compiler Architecture

**CS4031 Compiler Construction · Spring 2026**

---

## 1. High-Level Pipeline

```
  source.dc
      │
      ▼
  ┌─────────┐
  │  Lexer  │  Tokenises source into a flat stream of Tokens
  └────┬────┘
       │  Token stream
       ▼
  ┌─────────┐
  │  Parser │  Recursive-descent; builds a typed AST
  └────┬────┘
       │  ProgramNode (unique_ptr tree)
       ▼
  ┌──────────────────┐
  │ Semantic Analyser│  Two-pass validation; produces SymbolTable
  └────────┬─────────┘
           │  SemanticResult { SymbolTable, Diagnostics }
           ▼
  ┌───────────────┐
  │  TAC Generator│  Lowers AST → Three-Address Code instructions
  └───────┬───────┘
          │  TACProgram
          ▼
  ┌───────────────┐
  │   Optimiser   │  Dead-code elimination + redundant-AND folding
  └───────┬───────┘
          │  TACProgram (optimised)
          ▼
  ┌──────────────┐
  │  Code Generator│  Serialises world + TAC to JSON bytecode
  └──────┬────────┘
         │  JSON string / .json file
         ▼
  ┌──────┐
  │  VM  │  Loads bytecode; runs interactive game loop
  └──────┘
```

---

## 2. Module Descriptions

### 2.1 Lexer (`src/lexer/`)

**Responsibility:** Convert a raw C-string source into a stream of `Token` structs on demand (pull model — `scanToken()` is called once per token).

**Key design decisions:**
- Single global `Lexer` struct (start pointer, current pointer, line, column, lastEmitted, lastBeforeDot) — stateless between files, reset by `initLexer()`.
- Context-sensitive dot handling: when a `.` is scanned, `lastBeforeDot` records the token before it. The next identifier is reclassified as `TOKEN_PLAYER_ATTR` (if `lastBeforeDot == TOKEN_PLAYER`) or `TOKEN_ROOM_ATTR` (if room/identifier). This avoids introducing extra parser lookahead.
- `\r\n` consumed as a single newline; line/column tracking maintained in `advance()`.
- Comment stripping done inside `skipWhiteSpace()` — no separate pre-processing pass.

**Token types:** 35 distinct `TokenType` values defined in `src/lexer/lexer.h`.

---

### 2.2 Parser (`src/parser/`)

**Responsibility:** Consume the token stream and produce a fully typed Abstract Syntax Tree rooted at `ProgramNode`.

**Algorithm:** Hand-written recursive descent. Each grammar production maps 1-to-1 to a `parseXxx()` function.

**AST node hierarchy (base class `ASTNode`):**

```
ASTNode
├── ProgramNode          { vector<unique_ptr<ASTNode>> declarations }
├── RoomDeclNode         { name, description, items, npcs, exits }
├── ActionDeclNode       { name, vector<unique_ptr<StmtNode>> body }
├── ItemDeclNode         { name, properties }
├── NpcDeclNode          { name, properties }
├── ExitDeclNode         { direction, target }
├── StmtNode (abstract)
│   ├── PrintStmtNode    { message }
│   ├── RemoveStmtNode   { itemName }
│   ├── AssignStmtNode   { target (player_attr), op, value }
│   └── IfStmtNode       { condition, thenBranch, elseBranch? }
└── ConditionNode        { type, lhs, rhs, left?, right? }
```

All child ownership is expressed through `unique_ptr` — no manual memory management.

**Error recovery:** `synchronize()` advances past statement-boundary tokens (`}`, `room`, `action`) on parse error, allowing multiple errors to be reported in one pass.

---

### 2.3 Semantic Analyser (`src/semantic/`)

**Responsibility:** Statically validate the AST against the language rules. Produces a `SymbolTable` used by downstream phases.

**Two-pass design:**

| Pass | Function | What it does |
|---|---|---|
| 1 | `registerDeclarations()` | Walks all top-level declarations; inserts rooms, items, NPCs, actions into `SymbolTable`. Reports duplicates. |
| 2 | `validateDeclarations()` | For each room: checks exit targets exist, no duplicate exit directions. For each action: validates `remove` references, `has_item` references, `current_room` string comparisons. |

**SymbolTable structure:**
```cpp
struct SymbolTable {
    unordered_map<string, RoomInfo>   rooms;
    unordered_map<string, ItemInfo>   items;
    unordered_map<string, NpcInfo>    npcs;
    unordered_map<string, ActionInfo> actions;
    unordered_map<string, string>     playerAttrTypes;
};
```

Diagnostics are collected into a `vector<Diagnostic>` with severity (`WARNING` / `ERROR`), line, column, and message. All are printed to `stderr`. If any `ERROR` severity diagnostic exists, `hadError = true` and compilation halts.

---

### 2.4 TAC IR Generator (`src/tac/tac.cpp`)

**Responsibility:** Lower the AST into a flat sequence of Three-Address Code instructions — one `TACAction` per action declaration.

**Instruction format:**
```cpp
struct TACInstruction {
    TACOp  op;
    string dest, src1, src2;
    int    intValue;
    bool   boolValue;
    string strValue;
};
```

**Opcode set (18 opcodes):**

| Opcode | Semantics |
|---|---|
| `ASSIGN` | `dest = src1` |
| `ADD` / `SUB` | arithmetic |
| `COMPARE_EQ/GT/GTE/LT/LTE` | comparison into temp bool |
| `AND` / `OR` | logical combine of two temp bools |
| `JUMP_IF_FALSE` | conditional branch to label |
| `JUMP` | unconditional branch |
| `LABEL` | branch target |
| `PRINT` | print `strValue` |
| `REMOVE_ITEM` | remove `src1` from world |
| `SET_PLAYER_ATTR` | set player attribute |
| `HAS_ITEM` | check inventory → temp bool |
| `CHECK_ROOM` | check current room → temp bool |

**Temporary naming:** `t0`, `t1`, `t2`, … per action, allocated by a counter reset at the start of each action.

**Condition lowering example** — `current_room == "bag_end" && player.has_item(the_ring)`:
```
t0  ← CHECK_ROOM   "bag_end"
t1  ← HAS_ITEM     the_ring
t2  ← AND          t0  t1
      JUMP_IF_FALSE t2  L_else_0
```

---

### 2.5 Optimiser (`src/tac/optimizer.cpp`)

**Responsibility:** Apply machine-independent optimisations to the TAC program before code generation.

**Pass 1 — Dead Code Elimination:**
After an unconditional `JUMP` instruction, all instructions up to (but not including) the next `LABEL` are unreachable. They are removed from the instruction list.

Example:
```
JUMP  L_end          ← unconditional
PRINT "dead code"    ← REMOVED
LABEL L_end          ← kept
```

**Pass 2 — Redundant AND Folding:**
When `AND t2, t0, t0` (both operands identical), `t2` is an alias of `t0`. The AND instruction is removed and subsequent uses of `t2` are rewritten to `t0`. This commonly arises from single-condition `if` statements that go through the general condition-combining path.

Both passes run on each `TACAction` independently.

---

### 2.6 Code Generator (`src/codegen/`)

**Responsibility:** Serialise the semantic symbol table and optimised TAC program into a JSON bytecode string/file.

**Output format:**
```json
{
  "world": [
    {
      "id": "bag_end",
      "description": "...",
      "items": [{"name": "the_ring", "props": {"power": 100}}],
      "npcs":  [],
      "exits": [{"direction": "east", "target": "buckland"}]
    }
  ],
  "start_room": "bag_end",
  "actions": [
    {
      "name": "take ring",
      "instructions": [
        {"op": "CHECK_ROOM", "dest": "t0", "strValue": "bag_end"},
        {"op": "JUMP_IF_FALSE", "src1": "t0", "strValue": "L_else_0"},
        {"op": "SET_PLAYER_ATTR", "dest": "inventory", "src1": "the_ring", "strValue": "+="},
        {"op": "PRINT", "strValue": "The Precious is yours."},
        {"op": "LABEL", "strValue": "L_end_0"}
      ]
    }
  ]
}
```

Uses the vendored `nlohmann/json` v3.11.3 header-only library. Output is pretty-printed with 2-space indentation.

---

### 2.7 Virtual Machine (`src/vm/`)

**Responsibility:** Load JSON bytecode and execute the game interactively.

**Game state:**
```cpp
struct GameState {
    string               currentRoom;
    unordered_set<string> inventory;
    unordered_map<string,int>    intAttrs;
    unordered_map<string,bool>   boolAttrs;
    unordered_map<string,string> strAttrs;
};
```

**Execution model:**
1. `loadBytecode()` parses JSON, populates world map and action instruction lists.
2. `executeAction()` builds a `label → instruction index` map, then runs an instruction-pointer (ip) loop.
3. All 18 opcodes implemented with a switch statement; results stored in a `unordered_map<string,Value>` temporary register file.

**Game loop (`runGameLoop()`):**
- Starts player in the first declared room.
- Prints room description.
- Reads player input line.
- Built-in commands: `look`, `inventory`, `go <dir>`, `quit`.
- Any other input is matched against action names; if found, executes it.
- After each action, checks `state.boolAttrs["win"]` — if true, prints win message and exits.

---

## 3. Data Flow Diagram

```
                    ┌──────────────┐
 source.dc ────────►│    Lexer     │ Token stream (lazy pull)
                    └──────┬───────┘
                           │
                    ┌──────▼───────┐
                    │    Parser    │ ProgramNode AST
                    └──────┬───────┘
                           │
                    ┌──────▼────────────┐
                    │ Semantic Analyser │ SymbolTable + Diagnostics
                    └──────┬────────────┘
                           │
                    ┌──────▼────────┐
                    │ TAC Generator │ TACProgram (raw)
                    └──────┬────────┘
                           │
                    ┌──────▼────────┐
                    │  Optimiser    │ TACProgram (optimised)
                    └──────┬────────┘
                           │
                    ┌──────▼────────────┐
                    │  Code Generator   │ JSON bytecode string
                    └──────┬────────────┘
                           │
              ┌────────────┴────────────┐
              │                         │
       ┌──────▼──────┐          ┌───────▼──────┐
       │  Write file │          │     VM        │
       │  (*.json)   │          │  game loop   │
       └─────────────┘          └──────────────┘
```

---

## 4. Error Handling Strategy

| Phase | Strategy |
|---|---|
| Lexer | Returns `TOKEN_ERROR` token with message; scanning stops at first lexer error. |
| Parser | Panic-mode recovery via `synchronize()`; multiple parse errors reported per run. |
| Semantic | Collect-all-errors model; all diagnostics reported before halting. `hadError` flag. |
| TAC/Codegen | Assume valid input (semantic gate ensures this); no additional error paths. |
| VM | Runtime errors (bad action name, missing room) printed to `stderr`; game continues. |

---

## 5. Build System

CMake 3.23+ with one static library per phase:

```
durins_lexer  ←  durins_parser  ←  durins_semantic  ←  durins_tac  ←  durins_codegen  ←  durins_vm  ←  durinsc
```

Each library exposes its `src/` subtree as a public include directory so downstream targets find headers without specifying paths.

Test executables link only the libraries they test, keeping compilation units small and test feedback fast.

---

## 6. Third-Party Dependencies

| Library | Version | Location | Purpose |
|---|---|---|---|
| nlohmann/json | 3.11.3 | `third_party/nlohmann/json.hpp` | JSON serialisation in codegen + VM |
| GoogleTest | system | installed via package manager | Unit testing |

nlohmann/json is vendored (single header, no build step) to eliminate network dependency at build time.
