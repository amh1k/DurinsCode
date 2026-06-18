import "./style.css";
import { compileSource, type CompileResult } from "./compiler";
import { examples } from "./examples";
import { DurinsVM, parseBytecode, type Bytecode } from "./vm";

type TabName = "game" | "diagnostics" | "bytecode" | "debug" | "map";

const app = document.querySelector<HTMLDivElement>("#app");
if (!app) throw new Error("Missing #app");

let selectedExample = examples.find((example) => example.name === "04_middle_earth.dc") ?? examples[0];
let activeTab: TabName = "game";
let compileResult: CompileResult | null = null;
let bytecode: Bytecode | null = null;
let vm: DurinsVM | null = null;
let commandInput = "";
let isCompiling = false;

app.innerHTML = `
  <main class="shell">
    <nav class="siteNav" aria-label="Primary navigation">
      <a class="brand" href="#hero" aria-label="Durin's Code home">
        <span class="ringLogo" aria-hidden="true"></span>
        <span>Durin's Code</span>
      </a>
      <div class="navLinks">
        <a href="#docs">Docs</a>
        <a href="#compiler">Compiler</a>
        <a href="https://github.com/amh1k/DurinsCode" target="_blank" rel="noreferrer">GitHub</a>
      </div>
    </nav>

    <section id="hero" class="heroSection">
      <div class="heroContent">
        <p class="eyebrow">A WebAssembly compiler forged for text adventures</p>
        <h1>Build playable quests from a tiny fantasy language.</h1>
        <p class="heroCopy">
          Durin's Code turns rooms, relics, NPCs, exits, and action handlers into JSON bytecode,
          then runs the adventure directly in your browser.
        </p>
        <div class="heroActions">
          <a class="buttonLink primary" href="#compiler">Open compiler</a>
          <a class="buttonLink ghost" href="#docs">Read language docs</a>
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
    </section>

    <section id="docs" class="docsSection">
      <div class="sectionIntro">
        <p class="eyebrow">Language reference</p>
        <h2>Everything you need to write a Durin's Code adventure.</h2>
        <p>
          The language is intentionally small: declare the world, define player actions,
          compile to bytecode, and let the VM handle exploration.
        </p>
      </div>

      <div class="docGrid">
        <article class="docPanel wide">
          <h3>Compiler Pipeline</h3>
          <p>Source is scanned, parsed into an AST, semantically checked, lowered to TAC, optimised, serialised as JSON bytecode, and executed by the VM.</p>
          <pre><code>source.dc -> lexer -> parser -> semantic analyser
          -> TAC -> optimiser -> JSON bytecode -> VM</code></pre>
        </article>

        <article class="docPanel">
          <h3>Program Shape</h3>
          <p>A program is a list of top-level declarations. Only <code>room</code> and <code>action</code> exist at the top level.</p>
          <pre><code>room "bag_end" { ... }
action "take ring" { ... }</code></pre>
        </article>

        <article class="docPanel">
          <h3>Rooms</h3>
          <p>Rooms are graph nodes. They can contain one description, items, NPCs, and directed exits.</p>
          <pre><code>room "village" {
  description "A peaceful village."
  item map { pages: 10 }
  npc wolf { hostile: true }
  exit north "forest"
}</code></pre>
        </article>

        <article class="docPanel">
          <h3>Items and NPCs</h3>
          <p>Entities use property maps with integer, string, or boolean values. Item names become globally referenceable in actions.</p>
          <pre><code>item sword { damage: 50, type: "weapon" }
npc king { health: 100, hostile: false }</code></pre>
        </article>

        <article class="docPanel">
          <h3>Actions</h3>
          <p>Actions are commands the player types verbatim. They contain print statements, inventory updates, removals, assignments, and conditionals.</p>
          <pre><code>action "take sword" {
  player.inventory += sword
  print "You take the sword."
}</code></pre>
        </article>

        <article class="docPanel">
          <h3>Conditions</h3>
          <p>Branch on the current room, inventory, or player attributes. Use <code>&&</code> for reliable compound checks in the current build.</p>
          <pre><code>if current_room == "armoury"
   && player.has_item(sword) {
  print "Ready."
}</code></pre>
        </article>

        <article class="docPanel">
          <h3>Player State</h3>
          <p>The VM tracks inventory plus integer, boolean, and string attributes. <code>player.win = true</code> completes the quest.</p>
          <pre><code>player.inventory += the_ring
player.gold = 500
player.win = true</code></pre>
        </article>

        <article class="docPanel">
          <h3>Static Checks</h3>
          <p>The semantic analyser catches duplicate rooms/actions, invalid exits, duplicate exit directions, and references to undeclared items or rooms.</p>
          <pre><code>exit east "missing_room"  // compile error
remove phantom_item      // compile error</code></pre>
        </article>

        <article class="docPanel">
          <h3>Runtime Commands</h3>
          <p>The browser VM supports the same core player flow as the terminal version.</p>
          <pre><code>look
go east
inventory
take ring
destroy ring</code></pre>
        </article>
      </div>
    </section>

    <section id="compiler" class="compilerSection">
      <div class="compilerHeader">
        <div>
          <p class="eyebrow">Live compiler</p>
          <h2>Write, compile, inspect, and play.</h2>
        </div>
        <div class="actions">
          <select id="exampleSelect" aria-label="Example"></select>
          <button id="loadExample">Load</button>
          <button id="compileButton" class="primary">Compile</button>
          <button id="resetGame">Reset Game</button>
        </div>
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
const loadExampleButton = document.querySelector<HTMLButtonElement>("#loadExample")!;
const resetGameButton = document.querySelector<HTMLButtonElement>("#resetGame")!;
const compileStatus = document.querySelector<HTMLSpanElement>("#compileStatus")!;
const panel = document.querySelector<HTMLDivElement>("#panel")!;

function init(): void {
  exampleSelect.innerHTML = examples
    .map((example) => `<option value="${example.name}">${example.name}</option>`)
    .join("");
  exampleSelect.value = selectedExample.name;
  sourceEditor.value = selectedExample.source;

  exampleSelect.addEventListener("change", () => {
    selectedExample = examples.find((example) => example.name === exampleSelect.value) ?? examples[0];
  });

  loadExampleButton.addEventListener("click", () => {
    sourceEditor.value = selectedExample.source;
    compileResult = null;
    bytecode = null;
    vm = null;
    setStatus("Example loaded");
    renderPanel();
  });

  compileButton.addEventListener("click", () => {
    void compileCurrentSource();
  });

  resetGameButton.addEventListener("click", () => {
    resetGame();
    renderPanel();
  });

  document.querySelectorAll<HTMLButtonElement>("[data-tab]").forEach((button) => {
    button.addEventListener("click", () => {
      activeTab = button.dataset.tab as TabName;
      document.querySelectorAll<HTMLButtonElement>("[data-tab]").forEach((tab) => {
        tab.classList.toggle("active", tab === button);
      });
      renderPanel();
    });
  });

  renderPanel();
}

async function compileCurrentSource(): Promise<void> {
  isCompiling = true;
  setStatus("Compiling...");
  renderPanel();

  compileResult = await compileSource(sourceEditor.value, true);
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
    activeTab = "game";
    setActiveTabButton();
    setStatus("Compiled successfully");
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
  const debug = compileResult?.debug || "No debug output yet.";
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
