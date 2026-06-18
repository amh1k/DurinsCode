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
    <section class="topbar">
      <div>
        <h1>Durin's Code</h1>
        <p>Compile the adventure DSL to WebAssembly-powered bytecode, then play it in the browser.</p>
      </div>
      <div class="actions">
        <select id="exampleSelect" aria-label="Example"></select>
        <button id="loadExample">Load</button>
        <button id="compileButton" class="primary">Compile</button>
        <button id="resetGame">Reset Game</button>
      </div>
    </section>

    <section class="workspace">
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
