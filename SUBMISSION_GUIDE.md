# Submission Guide — Durin's Code
### CS4031 Compiler Construction · Spring 2026
**Team:** Huzaifa Abdul Rehman (23K-0782) · Abdul Moiz Hussain (23K-0553) · Muhammad Abdullah Khan (23K-0607)

---

## I Copied the ZIP to My Windows 10 Laptop — What Now?

Do these steps **in order**. Takes about 10 minutes total.

**Step 1 — Extract the ZIP**
Right-click the ZIP → Extract All → pick any folder (e.g. Desktop).
You should now have a folder called `DurinsCode-main/`.

**Step 2 — Install MSYS2** (only needed once)
1. Go to msys2.org → download the installer → run it → keep all defaults
2. At the end it opens a terminal automatically. If not, open **MSYS2 MINGW64** from Start menu (search "MSYS2 MINGW64" — must be the MINGW64 one, not MSYS2 or UCRT64)

**Step 3 — Install the tools** (paste this into the MSYS2 MINGW64 terminal, press Enter, type `Y` when asked)
```bash
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc mingw-w64-x86_64-gtest
```
Wait for it to finish (~2 minutes).

**Step 4 — Navigate to the project**
In the same MSYS2 MINGW64 terminal (replace `Abdullah` with your actual Windows username):
```bash
cd /c/Users/Abdullah/Desktop/DurinsCode-main
```
> Tip: Windows path `C:\Users\Abdullah\Desktop` becomes `/c/Users/Abdullah/Desktop` in MSYS2.

**Step 5 — Build**
```bash
cmake -S . -B build
cmake --build build
```
Takes ~15 seconds. Last line should say `[100%] Linking CXX executable vm_tests`.

**Step 6 — Run the tests**
```bash
cd build
ctest --output-on-failure
```
Expected: `100% tests passed, 0 tests failed out of 49`

**Step 7 — Play the game**
```bash
./durinsc ../examples/04_middle_earth.dc
```
Then type these commands one by one:
```
take ring
go east
go east
destroy ring
```
You should see the gold `YOU WIN!` banner.

**Done.** Everything works. You can close MSYS2.

---

## What to Submit

Submit the **entire `DurinsCode-main/` folder as a ZIP**, but **delete these before zipping**:

| Delete this | Why |
|---|---|
| `build/` | Compiled binaries — marker builds it themselves |
| `test/build/` | Same |
| `docs/superpowers/` | Internal AI planning notes — not for submission |
| `Suggestions.md` | Internal scratch notes — not for submission |

Everything else goes in. The folder structure the marker should see:

```
DurinsCode-main/
├── CMakeLists.txt              ← build system
├── README.md                   ← setup + usage instructions
├── src/
│   ├── lexer/                  ← Phase 1
│   ├── parser/                 ← Phase 2
│   ├── semantic/               ← Phase 3
│   ├── tac/                    ← Phase 4 + 5 (IR + optimiser)
│   ├── codegen/                ← Phase 6
│   ├── vm/                     ← Phase 7
│   └── main.cpp                ← CLI entry point
├── test/                       ← all 49 GoogleTest unit tests
├── examples/                   ← 5 demo .dc programs
├── third_party/nlohmann/       ← vendored JSON library (required)
└── docs/
    ├── latex/                  ← 4 LaTeX documents (language ref, architecture,
    │                              reflection, test report)
    ├── language-reference.md
    ├── architecture.md
    └── team-reflection.md
```

---

## Prerequisites (install these first)

### Option A — Visual Studio (easiest on Windows)

1. Install **Visual Studio 2022** (Community edition is free) from visualstudio.microsoft.com
2. During install, tick **"Desktop development with C++"** workload — this gives you MSVC + CMake + Ninja all at once
3. Install **GoogleTest** via vcpkg (included with VS2022):
   ```powershell
   vcpkg install gtest:x64-windows
   vcpkg integrate install
   ```

### Option B — MSYS2 / MinGW (closer to Linux feel)

1. Install **MSYS2** from msys2.org
2. Open the **MSYS2 MINGW64** shell and run:
   ```bash
   pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc mingw-w64-x86_64-gtest
   ```
3. Use this shell for all commands below.

### Option C — WSL2 (Ubuntu inside Windows — recommended if you have it)

```bash
sudo apt install cmake libgtest-dev g++
```
Then follow the Linux commands exactly as written below.

---

**Compiler requirement:** C++17 — MSVC 19.14+ (VS2019+), GCC 11+, or Clang 14+.

Check your compiler:
```powershell
# PowerShell / cmd — for MSVC (run inside Developer Command Prompt)
cl

# MSYS2 shell or WSL
g++ --version
```

---

## Build (takes ~15 seconds)

### Visual Studio GUI
1. Open Visual Studio → **Open a Local Folder** → select `DurinsCode-main/`
2. VS detects `CMakeLists.txt` automatically
3. Select build target **durinsc** from the toolbar dropdown → click **Build All**

### Command line (Developer Command Prompt, MSYS2, or WSL)

```powershell
cd DurinsCode-main
cmake -S . -B build
cmake --build build
```

If it works you'll see `[100%] Linking CXX executable vm_tests` at the end with no errors.

> **MSVC tip:** add `--config Release` to the build command if you get debug-mode link errors:
> `cmake --build build --config Release`

---

## Run All Tests (49 tests, all pass)

```powershell
cd build
ctest --output-on-failure
```

Expected last line:
```
100% tests passed, 0 tests failed out of 49
```

To run one suite at a time:

**PowerShell / cmd (MSVC build):**
```powershell
.\lexer_tests.exe
.\parser_tests.exe
.\semantic_tests.exe
.\tac_tests.exe
.\codegen_tests.exe
.\vm_tests.exe
```

**MSYS2 or WSL:**
```bash
./lexer_tests
./parser_tests
./semantic_tests
./tac_tests
./codegen_tests
./vm_tests
```

---

## Run the Game (demo for the marker)

**PowerShell / cmd:**
```powershell
cd build
.\durinsc.exe ..\examples\04_middle_earth.dc
```

**MSYS2 or WSL:**
```bash
cd build
./durinsc ../examples/04_middle_earth.dc
```

**Win path to show the marker:**
```
> take ring          ← picks up the_ring in bag_end
> go east            ← move to buckland
> go east            ← move to mount_doom
> destroy ring       ← triggers win condition
```

Other commands in the game: `look`, `inventory`, `go <direction>`, `quit`

> **Colour note:** ANSI colours render correctly in Windows Terminal and WSL. In the old `cmd.exe` they may show as plain text — use Windows Terminal for the best look.

---

## Other CLI Modes

**PowerShell / cmd:**
```powershell
# Show symbol table + TAC dump (good for showing the marker)
.\durinsc.exe ..\examples\04_middle_earth.dc --debug

# Export compiled bytecode to a file
.\durinsc.exe ..\examples\04_middle_earth.dc -o game.json

# Run from bytecode file (shows the VM is separate from compiler)
.\durinsc.exe --run game.json

# Interactive REPL
.\durinsc.exe --interactive
```

**MSYS2 or WSL:** same but with `./durinsc` and forward slashes.

---

## The 5 Example Programs

| File | What it shows |
|---|---|
| `examples/01_hello_world.dc` | Simplest valid program |
| `examples/02_inventory.dc` | `+=` / `-=` inventory, `has_item` condition |
| `examples/03_multiroom.dc` | 4-room connected world graph |
| `examples/04_middle_earth.dc` | **Full demo — use this for the marker** |
| `examples/05_error_demo.dc` | Intentionally broken — shows 5 semantic errors |

Run the error demo to show the semantic checker works:

```powershell
.\durinsc.exe ..\examples\05_error_demo.dc
# Expected: "Compilation failed: 5 error(s)."
```

---

## Documentation (for the docs marks)

All 4 LaTeX source files are in `docs/latex/`. To compile them to PDF:

**Option A — Overleaf (easiest, no install needed):**
1. Go to overleaf.com → New Project → Upload
2. Upload `preamble.tex` + whichever `.tex` you want
3. Hit Compile

**Option B — local (install MiKTeX from miktex.org first):**
```powershell
cd docs\latex
latexmk -pdf 01_language_reference.tex
latexmk -pdf 02_architecture.tex
latexmk -pdf 03_team_reflection.tex
latexmk -pdf 04_test_report.tex
```

| LaTeX file | Document |
|---|---|
| `01_language_reference.tex` | Language Reference Manual (grammar, tokens, EBNF) |
| `02_architecture.tex` | Compiler Architecture (all 7 phases, diagrams) |
| `03_team_reflection.tex` | Team Reflection (challenges, lessons learned) |
| `04_test_report.tex` | Test Report (all 49 tests documented) |

---

## Handwritten Diagrams (do these on paper before submitting)

Three things the prof expects drawn by hand:
1. **DFA diagram** — state machine for the lexer (identifiers, `==`, `&&`, strings)
2. **Two parse trees** — e.g. for `if current_room == "bag_end" { ... }` and `player.inventory += the_ring`
3. **Symbol table** — hand-fill the table after parsing `04_middle_earth.dc`

Scan or photo these and include them in your submission folder.

---

## Quick Sanity Check Before Submitting

**WSL or MSYS2:**
```bash
cd DurinsCode-main
cmake -S . -B build && cmake --build build
cd build
ctest --output-on-failure
./durinsc ../examples/04_middle_earth.dc --debug 2>&1 | head -8
printf "take ring\ngo east\ngo east\ndestroy ring\nquit\n" | ./durinsc ../examples/04_middle_earth.dc
```

**PowerShell (MSVC build):**
```powershell
cd DurinsCode-main
cmake -S . -B build; cmake --build build --config Release
cd build
ctest --output-on-failure
.\durinsc.exe ..\examples\04_middle_earth.dc --debug
# Then run the game manually and type: take ring → go east → go east → destroy ring
```

All steps must complete with no errors and the game must print `YOU WIN!`.
