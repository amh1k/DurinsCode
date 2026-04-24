# Durin's Code — Team Reflection

**CS4031 Compiler Construction · Spring 2026**

**Team Members:** Abdul Moiz Hussain (23K-0553), Muhammed Abdullah Khan (23K-0607), Huzaifa Abdul Rehman (23K-0782)

---

## 1. Project Summary

Durin's Code is a complete compiler for a Tolkien-themed text-adventure DSL. Over the course of the project we built every stage of the compilation pipeline from scratch in C++17: a hand-written lexer, a recursive-descent parser producing a typed AST, a two-pass semantic analyser with a global symbol table, a Three-Address Code (TAC) intermediate representation generator, a two-pass optimiser, a JSON bytecode serialiser, and a virtual machine with an interactive game loop. The final system compiles and runs `.dc` programs entirely from the terminal with no external runtime dependencies beyond the vendored nlohmann/json library.

---

## 2. What Went Well

**Clear phase boundaries.** Organising the project as one CMake static library per phase (`durins_lexer`, `durins_parser`, …) meant each team member could own a phase independently and changes in one phase rarely broke another. GoogleTest suites gave immediate feedback whenever an interface changed.

**Typed AST design.** Using `unique_ptr` throughout the AST eliminated entire classes of memory bugs from the start. The ownership model was strict but paid dividends — there were no use-after-free or double-free issues in the parser or downstream passes.

**Context-sensitive lexing for the dot operator.** Rather than adding lookahead to the parser, we encoded the `player.` vs `room.` distinction directly in the lexer by tracking `lastEmitted` and `lastBeforeDot`. This kept the parser simpler and the token stream richer.

**TAC as a stable interface.** Once the 18-opcode instruction set was agreed upon, both the generator and the VM could be developed independently. The JSON bytecode acted as a clean boundary that also gave us the `-o` bytecode-export feature for free.

---

## 3. Challenges We Faced

**Parser priming bug.** The original parser never called `advance()` after `initLexer()`, so `parser.current` was zero-initialised. This caused "Expect 'room' or 'action' at line 0 col 0" on every input. The fix was a single `advance()` call in `parseProgram()`, but diagnosing it required careful comparison of the parser's mental model against the actual token stream.

**`unique_ptr` copy vs move.** Several `push_back(ptr)` calls on `vector<unique_ptr<T>>` compiled silently under GCC but were rejected by Clang. Systematically replacing every `push_back(ptr)` with `push_back(std::move(ptr))` resolved the issue and surfaced the correct ownership semantics.

**Lexer line/column tracking.** The `advance()` function incremented the line counter on `\n`, but the original `skipWhiteSpace()` also incremented it separately, causing double-counting. Windows `\r\n` line endings additionally counted as two lines. Fixing both required rethinking which function owns line-increment responsibility — ultimately `advance()` alone.

**Two-pass semantic analysis ordering.** Exits reference rooms that may be declared later in the file. A single-pass approach would incorrectly report "room not found" for forward references. The two-pass design (register everything first, validate second) resolved this cleanly but required careful separation of the registration and validation concerns.

**Condition lowering to TAC.** Complex conditions (`&&`, `||`, nested `has_item`) require temporary boolean registers and careful label placement for `JUMP_IF_FALSE`. Getting the else/end label numbering correct with nested `if` statements required several iterations, especially for the case where no `else` branch exists.

---

## 4. What We Would Do Differently

**More incremental testing.** We wrote test suites per phase, but the integration between phases (e.g., semantic analyser feeding correct types to TAC generator) was only tested through full end-to-end runs. Dedicated integration tests between adjacent phases would have caught interface mismatches earlier.

**Richer type system.** The current type system is implicit — property types are inferred from their literal values at parse time and not enforced later. A formal type environment passed through the pipeline would enable better error messages for assignments like `player.health = "text"`.

**Better REPL experience.** The current `--interactive` REPL compiles a complete snippet on double-Enter. A proper REPL would support incremental room and action definition with persistent state between snippets, making it more useful for testing small language fragments.

---

## 5. Division of Work

| Phase | Primary Owner |
|---|---|
| Lexer + Lexer tests | Abdul Moiz Hussain |
| Parser + Parser tests | Abdul Moiz Hussain |
| Semantic Analyser + tests | Muhammed Abdullah Khan |
| TAC Generator + Optimiser + tests | Muhammed Abdullah Khan |
| Code Generator + VM + tests | Huzaifa Abdul Rehman |
| CLI (`main.cpp`) + Documentation | Huzaifa Abdul Rehman |
| Integration testing + example programs | All three members |

---

## 6. Lessons Learned

- **Compiler phases are natural unit boundaries.** Each phase has a well-defined input and output type. This made parallel development practical and made it easy to substitute one implementation for another.
- **Static validation pays dividends at runtime.** Every semantic check we added to the compiler saved us from writing equivalent runtime checks in the VM. The VM could be written assuming valid input, making it significantly simpler.
- **Intermediate representations decouple work.** The TAC program and JSON bytecode are both explicit, inspectable intermediate forms. The `--debug` flag that dumps them was invaluable during development. Build your IR to be human-readable.
- **Test suites are not optional.** The 49 tests across six suites caught regressions immediately whenever we changed a shared data structure. Without them, cross-phase changes would have been far more risky.
