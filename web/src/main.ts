import "./style.css";
import { compileSource, type CompileResult } from "./compiler";
import { examples } from "./examples";
import { DurinsVM, parseBytecode, type Bytecode } from "./vm";

type TabName = "game" | "diagnostics" | "bytecode" | "debug" | "map";
type PageName = "home" | "docs" | "compiler";

const app = document.querySelector<HTMLDivElement>("#app");
if (!app) throw new Error("Missing #app");

let selectedExample = examples.find((example) => example.name === "04_middle_earth.dc") ?? examples[0];
let activeTab: TabName = "game";
let compileResult: CompileResult | null = null;
let bytecode: Bytecode | null = null;
let vm: DurinsVM | null = null;
let commandInput = "";
let isCompiling = false;
let autoCompileTimer: number | undefined;

app.innerHTML = `
  <main class="shell">
    <nav class="siteNav" aria-label="Primary navigation">
      <a class="brand" href="#/" data-route="home" aria-label="Durin's Code home">
        <span class="ringLogo" aria-hidden="true"></span>
        <span>Durin's Code</span>
      </a>
      <div class="navLinks">
        <a href="#/" data-route="home">Home</a>
        <a href="#/docs" data-route="docs">Documentation</a>
        <a href="#/compiler" data-route="compiler">Compiler</a>
        <a href="https://github.com/amh1k/DurinsCode" target="_blank" rel="noreferrer">GitHub</a>
      </div>
    </nav>

    <section class="page pageHome" data-page="home">
      <div class="heroSection">
        <div class="heroAura" aria-hidden="true">
          <span class="mapLayer contourLayer"></span>
          <span class="mapLayer routeLayer"></span>
          <span class="mapLayer ringSigil"></span>
          <span class="mapNode nodeOne"></span>
          <span class="mapNode nodeTwo"></span>
          <span class="mapNode nodeThree"></span>
          <span class="mapNode nodeFour"></span>
        </div>
        <div class="heroContent">
          <p class="eyebrow">A WebAssembly compiler forged for text adventures</p>
          <h1>
            <span>Build playable quests</span>
            <span>from a tiny fantasy language.</span>
          </h1>
          <p class="heroCopy">
            Durin's Code turns rooms, relics, NPCs, exits, and action handlers into JSON bytecode,
            then runs the adventure directly in your browser.
          </p>
          <div class="heroActions">
            <a class="buttonLink primary" href="#/compiler">Open compiler</a>
            <a class="buttonLink ghost" href="#/docs">Read language docs</a>
          </div>
        </div>

        <div class="heroPreview" aria-label="Compiler preview">
          <div class="previewBar">
            <span class="dot amber"></span>
            <span class="dot green"></span>
            <span class="dot red"></span>
            <span>middle_earth.dc</span>
          </div>
          <div class="previewGrid">
            <pre><code>room "bag_end" {
  description "A golden ring glints."
  item the_ring { power: 100 }
  exit east "buckland"
}

action "destroy ring" {
  if current_room == "mount_doom"
     && player.has_item(the_ring) {
    player.win = true
  }
}</code></pre>
            <div class="previewOutput">
              <span class="chip">WASM compile</span>
              <span class="chip">JSON bytecode</span>
              <span class="chip">Playable VM</span>
              <div class="questCard">
                <strong>Mount Doom</strong>
                <p>The air is thick with ash. The fiery Cracks of Doom loom ahead.</p>
                <span>&gt; destroy ring</span>
                <b>YOU WIN</b>
              </div>
            </div>
          </div>
        </div>

        <div class="homeMetrics" aria-label="Project highlights">
          <article><strong>7</strong><span>compiler phases</span></article>
          <article><strong>49</strong><span>passing tests</span></article>
          <article><strong>0</strong><span>backend servers needed</span></article>
        </div>
      </div>
    </section>

    <section class="page docsPage" data-page="docs">
      <div class="sectionIntro">
        <p class="eyebrow">Complete language and architecture reference</p>
        <h2>Durin's Code, from grammar to bytecode.</h2>
        <p>
          This page condenses the Language Reference Manual, Compiler Architecture Document,
          and Test Report into a web-readable guide for authors and reviewers.
        </p>
      </div>

      <div class="docToc">
        <a href="#/docs" data-doc-target="lexical">Lexical Structure</a>
        <a href="#/docs" data-doc-target="grammar">Grammar</a>
        <a href="#/docs" data-doc-target="semantics">Semantics</a>
        <a href="#/docs" data-doc-target="architecture">Architecture</a>
        <a href="#/docs" data-doc-target="bytecode">Bytecode</a>
        <a href="#/docs" data-doc-target="tests">Tests</a>
      </div>

      <section id="lexical" class="docChapter">
        <div class="chapterText">
          <p class="eyebrow">01 / Lexical Structure</p>
          <h3>Small, strict, and readable.</h3>
          <p>
            Source files are UTF-8 text with Unix or Windows line endings. Comments use
            <code>//</code>. Identifiers are case-sensitive, may start with an underscore, and
            can contain letters, digits, and underscores after the first character.
          </p>
        </div>
        <div class="referenceGrid">
          <article class="docPanel">
            <h4>Keywords</h4>
            <p><code>room</code>, <code>action</code>, <code>item</code>, <code>npc</code>, <code>exit</code>, <code>if</code>, <code>else</code>, <code>print</code>, <code>remove</code>, <code>player</code>, <code>true</code>, <code>false</code>, <code>description</code>, <code>current_room</code></p>
          </article>
          <article class="docPanel">
            <h4>Literals</h4>
            <p>Strings are double-quoted and single-line. Numbers are decimal integers only. Booleans are <code>true</code> and <code>false</code>.</p>
          </article>
          <article class="docPanel">
            <h4>Dot Tokens</h4>
            <p>The lexer reclassifies identifiers after <code>player.</code> or <code>room.</code>, which keeps parser logic simple for attribute access and <code>player.has_item(...)</code>.</p>
          </article>
        </div>
      </section>

      <section id="grammar" class="docChapter">
        <div class="chapterText">
          <p class="eyebrow">02 / Grammar</p>
          <h3>The world is declarative; actions are reactive.</h3>
          <p>
            A program is a list of room and action declarations. Rooms define a directed
            world graph; actions define named handlers that the player can type at runtime.
          </p>
        </div>
        <div class="syntaxBoard">
          <pre><code>program      ::= declaration* EOF
declaration  ::= room_decl | action_decl

room_decl    ::= "room" STRING "{" room_body* "}"
room_body    ::= description | item_decl | npc_decl | exit_decl
item_decl    ::= "item" IDENTIFIER "{" prop ("," prop)* "}"
npc_decl     ::= "npc" IDENTIFIER "{" prop ("," prop)* "}"
exit_decl    ::= "exit" IDENTIFIER STRING

action_decl  ::= "action" STRING "{" statement* "}"
statement    ::= print | remove | assignment | if_else
condition    ::= room_check | has_item | player_compare</code></pre>
        </div>
      </section>

      <section class="docGrid expanded">
        <article class="docPanel">
          <h4>Rooms</h4>
          <p>Rooms contain one description, any number of items/NPCs, and exits to other declared rooms.</p>
          <pre><code>room "village" {
  description "A peaceful village."
  item map { pages: 10 }
  npc wolf { hostile: true }
  exit north "forest"
}</code></pre>
        </article>
        <article class="docPanel">
          <h4>Actions</h4>
          <p>Action names are strings and are typed verbatim by the player. They can print text, mutate player state, or branch on conditions.</p>
          <pre><code>action "take sword" {
  player.inventory += sword
  print "You take the sword."
}</code></pre>
        </article>
        <article class="docPanel">
          <h4>Conditions</h4>
          <p>Conditions can check the current room, inventory, or player attributes. Use <code>&&</code> for reliable compound conditions in this build.</p>
          <pre><code>if current_room == "mount_doom"
   && player.has_item(the_ring) {
  player.win = true
}</code></pre>
        </article>
      </section>

      <section id="semantics" class="docChapter">
        <div class="chapterText">
          <p class="eyebrow">03 / Semantic Rules</p>
          <h3>Static validation keeps the VM simple.</h3>
          <p>
            The analyser registers declarations first, then validates references. This two-pass
            design allows forward room references while still catching broken world graphs.
          </p>
        </div>
        <div class="ruleList">
          <span>Room names must be unique.</span>
          <span>Action names must be unique.</span>
          <span>Exit targets must reference declared rooms.</span>
          <span>One room cannot reuse an exit direction.</span>
          <span><code>remove</code> must target a declared item.</span>
          <span><code>player.has_item(x)</code> must reference a declared item.</span>
          <span><code>current_room == "x"</code> must reference a declared room.</span>
          <span>Player attributes are dynamically typed at runtime.</span>
        </div>
      </section>

      <section id="architecture" class="docChapter">
        <div class="chapterText">
          <p class="eyebrow">04 / Compiler Architecture</p>
          <h3>Seven phases with typed handoff points.</h3>
          <p>
            The architecture document describes a linear pipeline where each phase has one
            clear input and output. That structure made the WebAssembly port straightforward:
            the browser calls the compile pipeline and the TypeScript VM consumes bytecode.
          </p>
        </div>
        <div class="pipelineDiagram" aria-label="Compiler pipeline diagram">
          <div>Source</div><span>-></span><div>Lexer<br><small>tokens</small></div><span>-></span>
          <div>Parser<br><small>AST</small></div><span>-></span><div>Semantic<br><small>symbols</small></div><span>-></span>
          <div>TAC</div><span>-></span><div>Optimiser</div><span>-></span><div>Codegen<br><small>JSON</small></div><span>-></span><div>VM</div>
        </div>
        <div class="architectureGrid">
          <article class="docPanel">
            <h4>Lexer</h4>
            <p>Pull-based scanner over a <code>char*</code> source. Tracks line/column and emits context-sensitive dot tokens.</p>
          </article>
          <article class="docPanel">
            <h4>Parser</h4>
            <p>Hand-written recursive descent parser. Produces an owned AST using <code>unique_ptr</code> nodes.</p>
          </article>
          <article class="docPanel">
            <h4>Semantic Analyser</h4>
            <p>Builds a symbol table for rooms, items, NPCs, actions, and player attributes, then validates references.</p>
          </article>
          <article class="docPanel">
            <h4>TAC + Optimiser</h4>
            <p>Lowers actions into flat instructions, then applies dead-code elimination and redundant-AND folding.</p>
          </article>
          <article class="docPanel">
            <h4>Code Generator</h4>
            <p>Serialises rooms, exits, entities, and action instructions into pretty-printed JSON bytecode.</p>
          </article>
          <article class="docPanel">
            <h4>Virtual Machine</h4>
            <p>Maintains current room, inventory, typed player attributes, branch labels, temps, and win state.</p>
          </article>
        </div>
      </section>

      <section id="bytecode" class="docChapter">
        <div class="chapterText">
          <p class="eyebrow">05 / Runtime and Bytecode</p>
          <h3>JSON is the bridge between compiler and game.</h3>
          <p>
            Bytecode contains the world graph, the starting room, and TAC-like action
            instructions. The browser VM mirrors the C++ VM's command loop without porting
            terminal-specific I/O.
          </p>
        </div>
        <div class="bytecodeLayout">
          <pre><code>{
  "world": [
    {
      "name": "bag_end",
      "description": "The cozy hole of a Hobbit.",
      "items": [{ "name": "the_ring", "properties": { "power": 100 } }],
      "npcs": [],
      "exits": [{ "direction": "east", "target": "buckland" }]
    }
  ],
  "start_room": "bag_end",
  "actions": [
    {
      "name": "take ring",
      "instructions": [{ "op": "CHECK_ROOM", "dest": "t0", "str": "bag_end" }]
    }
  ]
}</code></pre>
          <div class="stateDiagram">
            <div>GameState</div>
            <span>currentRoom</span>
            <span>inventory</span>
            <span>intAttrs</span>
            <span>boolAttrs</span>
            <span>strAttrs</span>
          </div>
        </div>
      </section>

      <section id="tests" class="docChapter">
        <div class="chapterText">
          <p class="eyebrow">06 / Test Coverage</p>
          <h3>49 tests cover the compiler phases.</h3>
          <p>The documented test suite covers lexer, parser, semantic analyser, TAC, codegen, and VM behavior, plus an end-to-end Middle-earth smoke test.</p>
        </div>
        <div class="testMatrix">
          <article><strong>12</strong><span>Lexer</span></article>
          <article><strong>15</strong><span>Parser</span></article>
          <article><strong>10</strong><span>Semantic</span></article>
          <article><strong>4</strong><span>TAC</span></article>
          <article><strong>3</strong><span>Codegen</span></article>
          <article><strong>5</strong><span>VM</span></article>
        </div>
      </section>
    </section>

    <section class="page compilerPage" data-page="compiler">
      <div class="compilerHeader">
        <div>
          <p class="eyebrow">Live compiler</p>
          <h2>Write, compile, inspect, and play.</h2>
          <p>Compile with WebAssembly, inspect generated bytecode/debug output, then run the adventure in the browser VM.</p>
        </div>
        <div class="actions">
          <select id="exampleSelect" aria-label="Example"></select>
          <button id="loadExample">Load</button>
          <button id="compileButton" class="primary">Compile & Play</button>
          <button id="compileOnlyButton">Compile Only</button>
          <button id="resetGame">Reset Game</button>
        </div>
      </div>

      <div class="compilerOptions" aria-label="Compiler options">
        <label><input id="includeDebug" type="checkbox" checked /> include symbol table + TAC debug</label>
        <label><input id="autoCompile" type="checkbox" /> auto-compile after edits</label>
        <button id="copySource">Copy Source</button>
        <button id="downloadSource">Download .dc</button>
        <button id="copyBytecode">Copy Bytecode</button>
        <button id="downloadBytecode">Download JSON</button>
        <button id="clearOutput">Clear Output</button>
      </div>

      <section class="workspace" aria-label="Durin's Code compiler">
        <div class="editorPane">
          <div class="paneHeader">
            <span>source.dc</span>
            <span id="compileStatus">Not compiled</span>
          </div>
          <textarea id="sourceEditor" spellcheck="false"></textarea>
        </div>

        <div class="resultPane">
          <nav class="tabs" aria-label="Output tabs">
            <button data-tab="game" class="active">Game</button>
            <button data-tab="diagnostics">Diagnostics</button>
            <button data-tab="bytecode">Bytecode</button>
            <button data-tab="debug">Debug</button>
            <button data-tab="map">World Map</button>
          </nav>
          <div id="panel"></div>
        </div>
      </section>
    </section>
  </main>
`;

const exampleSelect = document.querySelector<HTMLSelectElement>("#exampleSelect")!;
const sourceEditor = document.querySelector<HTMLTextAreaElement>("#sourceEditor")!;
const compileButton = document.querySelector<HTMLButtonElement>("#compileButton")!;
const compileOnlyButton = document.querySelector<HTMLButtonElement>("#compileOnlyButton")!;
const loadExampleButton = document.querySelector<HTMLButtonElement>("#loadExample")!;
const resetGameButton = document.querySelector<HTMLButtonElement>("#resetGame")!;
const compileStatus = document.querySelector<HTMLSpanElement>("#compileStatus")!;
const includeDebug = document.querySelector<HTMLInputElement>("#includeDebug")!;
const autoCompile = document.querySelector<HTMLInputElement>("#autoCompile")!;
const copySourceButton = document.querySelector<HTMLButtonElement>("#copySource")!;
const downloadSourceButton = document.querySelector<HTMLButtonElement>("#downloadSource")!;
const copyBytecodeButton = document.querySelector<HTMLButtonElement>("#copyBytecode")!;
const downloadBytecodeButton = document.querySelector<HTMLButtonElement>("#downloadBytecode")!;
const clearOutputButton = document.querySelector<HTMLButtonElement>("#clearOutput")!;
const panel = document.querySelector<HTMLDivElement>("#panel")!;

function init(): void {
  exampleSelect.innerHTML = examples
    .map((example) => `<option value="${example.name}">${example.name}</option>`)
    .join("");
  exampleSelect.value = selectedExample.name;
  sourceEditor.value = selectedExample.source;

  window.addEventListener("hashchange", renderRoute);
  renderRoute();

  document.querySelectorAll<HTMLAnchorElement>("[data-doc-target]").forEach((link) => {
    link.addEventListener("click", (event) => {
      event.preventDefault();
      const target = link.dataset.docTarget;
      if (!target) return;
      window.location.hash = "#/docs";
      renderRoute();
      window.setTimeout(() => {
        document.getElementById(target)?.scrollIntoView({ behavior: "smooth", block: "start" });
      }, 0);
    });
  });

  exampleSelect.addEventListener("change", () => {
    selectedExample = examples.find((example) => example.name === exampleSelect.value) ?? examples[0];
  });

  loadExampleButton.addEventListener("click", () => {
    sourceEditor.value = selectedExample.source;
    clearCompileState("Example loaded");
    renderPanel();
    if (autoCompile.checked) scheduleAutoCompile();
  });

  sourceEditor.addEventListener("input", () => {
    if (autoCompile.checked) scheduleAutoCompile();
  });

  compileButton.addEventListener("click", () => {
    void compileCurrentSource({ playAfterCompile: true });
  });

  compileOnlyButton.addEventListener("click", () => {
    void compileCurrentSource({ playAfterCompile: false });
  });

  resetGameButton.addEventListener("click", () => {
    resetGame();
    renderPanel();
  });

  copySourceButton.addEventListener("click", () => {
    void copyText(sourceEditor.value, "Source copied");
  });

  downloadSourceButton.addEventListener("click", () => {
    downloadText(selectedExample.name || "adventure.dc", sourceEditor.value, "text/plain");
  });

  copyBytecodeButton.addEventListener("click", () => {
    void copyText(compileResult?.bytecode || "", "Bytecode copied");
  });

  downloadBytecodeButton.addEventListener("click", () => {
    downloadText("durins-bytecode.json", compileResult?.bytecode || "{}", "application/json");
  });

  clearOutputButton.addEventListener("click", () => {
    clearCompileState("Output cleared");
    renderPanel();
  });

  document.querySelectorAll<HTMLButtonElement>("[data-tab]").forEach((button) => {
    button.addEventListener("click", () => {
      activeTab = button.dataset.tab as TabName;
      setActiveTabButton();
      renderPanel();
    });
  });

  renderPanel();
}

function routeFromHash(): PageName {
  const hash = window.location.hash.replace(/^#\/?/, "");
  if (hash === "docs") return "docs";
  if (hash === "compiler") return "compiler";
  return "home";
}

function renderRoute(): void {
  const page = routeFromHash();
  document.querySelectorAll<HTMLElement>("[data-page]").forEach((section) => {
    section.hidden = section.dataset.page !== page;
  });
  document.querySelectorAll<HTMLAnchorElement>("[data-route]").forEach((link) => {
    link.classList.toggle("active", link.dataset.route === page);
  });
  window.scrollTo({ top: 0, behavior: "auto" });
}

function scheduleAutoCompile(): void {
  window.clearTimeout(autoCompileTimer);
  autoCompileTimer = window.setTimeout(() => {
    void compileCurrentSource({ playAfterCompile: false, quiet: true });
  }, 700);
}

async function compileCurrentSource(options: { playAfterCompile: boolean; quiet?: boolean }): Promise<void> {
  isCompiling = true;
  setStatus("Compiling...");
  renderPanel();

  compileResult = await compileSource(sourceEditor.value, includeDebug.checked);
  isCompiling = false;

  if (!compileResult.ok) {
    bytecode = null;
    vm = null;
    activeTab = "diagnostics";
    setActiveTabButton();
    setStatus("Compilation failed");
    renderPanel();
    return;
  }

  try {
    bytecode = parseBytecode(compileResult.bytecode);
    vm = new DurinsVM(bytecode);
    activeTab = options.playAfterCompile ? "game" : "bytecode";
    setActiveTabButton();
    setStatus(options.quiet ? "Auto-compiled" : "Compiled successfully");
  } catch (error) {
    bytecode = null;
    vm = null;
    activeTab = "diagnostics";
    setActiveTabButton();
    setStatus("Bad bytecode");
    compileResult = {
      ...compileResult,
      ok: false,
      diagnostics: `Compiler returned invalid bytecode: ${String(error)}`
    };
  }

  renderPanel();
}

function clearCompileState(status: string): void {
  compileResult = null;
  bytecode = null;
  vm = null;
  commandInput = "";
  setStatus(status);
}

function setActiveTabButton(): void {
  document.querySelectorAll<HTMLButtonElement>("[data-tab]").forEach((button) => {
    button.classList.toggle("active", button.dataset.tab === activeTab);
  });
}

function setStatus(status: string): void {
  compileStatus.textContent = status;
}

function resetGame(): void {
  if (bytecode) {
    vm = new DurinsVM(bytecode);
    activeTab = "game";
    setActiveTabButton();
    setStatus("Game reset");
  }
}

function renderPanel(): void {
  if (isCompiling) {
    panel.innerHTML = `<div class="empty">Compiling with WebAssembly...</div>`;
    return;
  }

  if (activeTab === "game") renderGamePanel();
  else if (activeTab === "diagnostics") renderDiagnosticsPanel();
  else if (activeTab === "bytecode") renderBytecodePanel();
  else if (activeTab === "debug") renderDebugPanel();
  else renderMapPanel();
}

function renderGamePanel(): void {
  if (!vm) {
    panel.innerHTML = `<div class="empty">Compile a valid adventure to start playing.</div>`;
    return;
  }

  const snapshot = vm.snapshot();
  panel.innerHTML = `
    <div class="gameLayout">
      <pre class="gameLog">${escapeHtml(snapshot.log.join("\n"))}</pre>
      <div class="commandBar">
        <input id="commandInput" value="${escapeAttr(commandInput)}" placeholder="type a command" />
        <button id="sendCommand" class="primary">Send</button>
      </div>
      <div class="hintRow">
        ${snapshot.hints.map((hint) => `<button class="hint" data-command="${escapeAttr(hint)}">${escapeHtml(hint)}</button>`).join("")}
      </div>
      <div class="stateStrip">
        <span>Room: ${escapeHtml(snapshot.currentRoom || "none")}</span>
        <span>Inventory: ${escapeHtml(snapshot.inventory.join(", ") || "empty")}</span>
        <span>${snapshot.won ? "Win state reached" : "Quest active"}</span>
      </div>
    </div>
  `;

  const input = document.querySelector<HTMLInputElement>("#commandInput")!;
  const send = document.querySelector<HTMLButtonElement>("#sendCommand")!;
  input.focus();
  input.setSelectionRange(input.value.length, input.value.length);
  input.addEventListener("input", () => {
    commandInput = input.value;
  });
  input.addEventListener("keydown", (event) => {
    if (event.key === "Enter") submitCommand(input.value);
  });
  send.addEventListener("click", () => submitCommand(input.value));
  document.querySelectorAll<HTMLButtonElement>("[data-command]").forEach((button) => {
    button.addEventListener("click", () => submitCommand(button.dataset.command || ""));
  });
}

function submitCommand(command: string): void {
  if (!vm) return;
  vm.submit(command);
  commandInput = "";
  renderGamePanel();
}

function renderDiagnosticsPanel(): void {
  const diagnostics = compileResult?.diagnostics || "No diagnostics yet.";
  panel.innerHTML = `<pre class="codeBlock">${escapeHtml(diagnostics)}</pre>`;
}

function renderBytecodePanel(): void {
  const text = compileResult?.bytecode
    ? JSON.stringify(JSON.parse(compileResult.bytecode), null, 2)
    : "No bytecode yet.";
  panel.innerHTML = `<pre class="codeBlock">${escapeHtml(text)}</pre>`;
}

function renderDebugPanel(): void {
  const debug = compileResult?.debug || "No debug output yet. Enable debug output and compile again.";
  panel.innerHTML = `<pre class="codeBlock">${escapeHtml(debug)}</pre>`;
}

function renderMapPanel(): void {
  if (!bytecode) {
    panel.innerHTML = `<div class="empty">Compile a valid adventure to see the room graph.</div>`;
    return;
  }

  panel.innerHTML = `
    <div class="mapGrid">
      ${bytecode.world
        .map(
          (room) => `
            <article class="roomCard">
              <h2>${escapeHtml(room.name)}</h2>
              <p>${escapeHtml(room.description)}</p>
              <div class="edgeList">
                ${room.exits.length === 0 ? "<span>No exits</span>" : room.exits.map((exit) => `<span>${escapeHtml(exit.direction)} -> ${escapeHtml(exit.target)}</span>`).join("")}
              </div>
            </article>
          `
        )
        .join("")}
    </div>
  `;
}

async function copyText(value: string, successMessage: string): Promise<void> {
  if (!value) {
    setStatus("Nothing to copy");
    return;
  }
  try {
    await navigator.clipboard.writeText(value);
    setStatus(successMessage);
  } catch {
    setStatus("Clipboard permission unavailable");
  }
}

function downloadText(filename: string, value: string, type: string): void {
  const blob = new Blob([value], { type });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = filename;
  link.click();
  URL.revokeObjectURL(url);
}

function escapeHtml(value: string): string {
  return value
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}

function escapeAttr(value: string): string {
  return escapeHtml(value);
}

init();
