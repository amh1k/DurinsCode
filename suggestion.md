# Durin's Code Web Showcase Suggestions

## Project Snapshot

Durin's Code is a C++17 compiler and runtime for a small domain-specific language that authors 8-bit style text-adventure games. A `.dc` program describes a world as rooms, items, NPCs, exits, and named player actions. The runtime turns those declarations into an interactive adventure with movement, inventory, scripted commands, conditions, and player state.

The current compiler pipeline is:

```text
source.dc
  -> lexer
  -> parser / AST
  -> semantic analyser / symbol table
  -> TAC generator
  -> optimiser
  -> JSON bytecode
  -> VM game loop
```

The project already has a good showcase story because it is not just a parser. It demonstrates a complete compilation pipeline, intermediate representation, optimisation, bytecode generation, and a playable virtual machine.

Current CLI capabilities:

- `durinsc <source.dc>` compiles and immediately runs a game.
- `durinsc <source.dc> -o <out.json>` writes compiled JSON bytecode.
- `durinsc --run <bytecode.json>` runs precompiled bytecode.
- `durinsc <source.dc> --debug` prints the symbol table and optimised TAC.
- `durinsc --interactive` starts a terminal REPL.

The documented and locally verified test suite passes all 49 tests with:

```bash
ctest --test-dir build --output-on-failure
```

## Recommended Showcase: Browser Playground

The strongest way to present Durin's Code on the web is as an online playground where visitors can edit, compile, inspect, and play a `.dc` adventure directly in the browser.

Recommended first screen:

- Left panel: Durin's Code editor with syntax highlighting.
- Top toolbar: example selector, compile button, reset button, download bytecode button.
- Right panel tabs: game, diagnostics, bytecode, TAC/debug, world map.
- Game panel: interactive text-adventure UI with command input and clickable command hints.

This makes the compiler visible as a real product: users can change source code, see compiler output, and immediately experience the resulting game.

## Best Deployment Architecture

The best long-term approach is a static web app with the compiler compiled to WebAssembly.

Use Emscripten to compile the non-interactive compiler pipeline to Wasm:

```text
browser editor
  -> Wasm compileSourceToBytecode(source)
  -> JSON bytecode
  -> TypeScript game runtime
  -> playable web adventure
```

Why this is the best option:

- No backend server is required for normal use.
- It can deploy cheaply on Vercel, Netlify, or GitHub Pages.
- Visitors can compile code instantly in the browser.
- The existing C++ compiler remains the source of truth.
- The generated JSON bytecode is already a clean boundary between compiler and runtime.

Do not port the terminal `runGameLoop()` directly to the browser. It is tightly coupled to `std::cin`, `std::cout`, ANSI terminal colors, and a blocking input loop. Instead, write a TypeScript runtime that interprets the existing JSON bytecode shape:

```text
world[]      rooms, descriptions, items, NPCs, exits
start_room   initial room name
actions[]    TAC-like bytecode instructions
```

Use the current C++ VM as the reference for behavior:

- `look` describes the current room.
- `go <direction>` moves through exits.
- `inventory` / `inv` shows carried items.
- action names execute matching bytecode actions.
- `player.win = true` ends the game with a win state.

## Alternative Deployment Options

### Option 1: Docker Backend API

Wrap the existing `durinsc` executable in a small backend service.

Flow:

```text
browser editor
  -> POST /compile
  -> backend writes temporary .dc file
  -> backend runs durinsc
  -> backend returns bytecode, diagnostics, or output
```

Good for:

- Fastest reuse of the existing CLI.
- Avoiding Emscripten setup.
- Keeping C++ runtime behavior exactly as-is.

Tradeoffs:

- Requires hosting a backend.
- Needs sandboxing, timeouts, and temp-file cleanup.
- Harder to deploy than a static site.

### Option 2: Terminal-In-Browser Demo

Use `xterm.js` in the browser and run `durinsc` on a server-side pseudo-terminal.

Good for:

- Showing the existing terminal experience almost unchanged.
- Preserving colors, prompts, and CLI behavior.
- A course demo where authenticity matters more than polished web UX.

Tradeoffs:

- Requires a backend process per session.
- Needs careful resource limits.
- Feels less like a modern language playground.

### Option 3: Static Precompiled Examples

Precompile the example `.dc` files into JSON bytecode and ship only a frontend runtime.

Good for:

- Very fast deployment.
- No Wasm or backend required.
- Great backup plan for a deadline.

Tradeoffs:

- Users cannot compile arbitrary Durin's Code.
- It showcases the VM more than the compiler.
- Diagnostics, TAC, and symbol table views would be static or unavailable.

## Feature Roadmap

### Version 1: Solid Demo

- Add a Monaco or CodeMirror editor for `.dc` source.
- Load examples from `examples/*.dc`.
- Compile source to JSON bytecode.
- Show compile success or failure.
- Run the generated game in a web play panel.
- Include commands for `look`, `go <direction>`, `inventory`, and scripted action names.
- Deploy as a static site if using the Wasm approach.

### Version 2: Compiler Showcase

- Add a diagnostics panel with line and column information.
- Add a bytecode viewer with formatted JSON.
- Add a debug panel for symbol table and optimised TAC.
- Add a world graph view generated from rooms and exits.
- Add download buttons for `.dc` source and `.json` bytecode.
- Add a "copy share link" feature that stores source in the URL or a small gist-like backend.

### Version 3: Polished Language Playground

- Add Durin's Code syntax highlighting.
- Add autocomplete for keywords such as `room`, `action`, `description`, `item`, `npc`, `exit`, `player`, `current_room`, and `print`.
- Add hover help for language constructs.
- Add inline diagnostics in the editor.
- Add a visual inventory and current-room panel beside the command log.
- Add a generated minimap from the room graph.

## Suggested Implementation Path

1. Create a separate `web/` frontend app after this suggestion document is accepted.
2. Start with a TypeScript implementation of the bytecode VM using the C++ VM as the behavior reference.
3. Use precompiled example bytecode first to validate the game panel and world map quickly.
4. Add a small C++ binding layer for the compiler pipeline, for example:

```cpp
std::string compileSourceToBytecode(const std::string& source);
std::string compileSourceDebug(const std::string& source);
```

5. Compile the compiler libraries to WebAssembly with Emscripten.
6. Connect the browser editor to the Wasm compile function.
7. Add diagnostics, bytecode, TAC, and symbol-table panels.
8. Deploy the static build to Vercel, Netlify, or GitHub Pages.

## Pre-Web Polish Needed

Before heavily promoting the web playground, fix or document these caveats:

- Avoid advertising `||` heavily until it is fixed. The lexer and parser recognize logical OR, but the current TAC generation combines condition clauses as `AND`, so OR behavior is not reliable.
- Use known-passing examples for demos, especially `examples/04_middle_earth.dc`.
- If browser diagnostics are a core feature, refactor parser and semantic errors away from direct `stderr` printing into structured results that the UI can display.
- Keep the bytecode schema aligned with the actual generated JSON. The current code uses fields such as `name`, `description`, `items`, `npcs`, `exits`, `start_room`, and `actions`.
- Consider adding a compile-only API that does not start the VM, because browser compilation should return data rather than enter an interactive loop.

## Recommended Final Demo Script

For a strong reviewer demo:

1. Open the web playground with `examples/04_middle_earth.dc` loaded.
2. Point out the language constructs: rooms, exits, item declaration, action handlers, `current_room`, inventory checks, and `player.win`.
3. Press Compile.
4. Show the generated bytecode tab.
5. Show the world graph from `bag_end` to `buckland` to `mount_doom`.
6. Play the win path:

```text
take ring
go east
go east
destroy ring
```

7. Open `examples/05_error_demo.dc` and compile it to show semantic validation.
8. Show the diagnostics panel catching duplicate rooms, bad exits, duplicate exit directions, and undeclared items.

This demo highlights the full value of the project: language design, compiler phases, static analysis, bytecode generation, runtime execution, and a usable web experience.

