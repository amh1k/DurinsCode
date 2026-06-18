export type CompileResult = {
  ok: boolean;
  bytecode: string;
  diagnostics: string;
  debug: string;
  stdout?: string;
};

type DurinsModule = {
  ccall: (
    ident: string,
    returnType: string | null,
    argTypes: string[],
    args: unknown[]
  ) => number | string | null;
  UTF8ToString: (ptr: number) => string;
};

type DurinsFactory = {
  default: (options?: Record<string, unknown>) => Promise<DurinsModule>;
};

let modulePromise: Promise<DurinsModule> | null = null;

async function loadModule(): Promise<DurinsModule> {
  if (!modulePromise) {
    const baseUrl = import.meta.env.BASE_URL;
    const importPublicModule = new Function("path", "return import(path)") as (
      path: string
    ) => Promise<DurinsFactory>;
    modulePromise = importPublicModule(`${baseUrl}wasm/durins.js`).then((factory) =>
      factory.default({
        locateFile: (path: string) =>
          path.endsWith(".wasm") ? `${baseUrl}wasm/${path}` : path
      })
    );
  }
  return modulePromise;
}

function fallbackError(message: string): CompileResult {
  return {
    ok: false,
    bytecode: "",
    diagnostics: message,
    debug: ""
  };
}

export async function compileSource(
  source: string,
  includeDebug = true
): Promise<CompileResult> {
  let mod: DurinsModule;
  try {
    mod = await loadModule();
  } catch (error) {
    return fallbackError(
      `Could not load Durin's Code WebAssembly module.\n\nRun:\ncd web\nnpm run build:wasm\n\n${String(error)}`
    );
  }

  const fn = includeDebug ? "compile_debug" : "compile_source";
  const ptr = mod.ccall(fn, "number", ["string"], [source]) as number;
  if (!ptr) {
    return fallbackError("Compiler returned an empty result pointer.");
  }

  try {
    const raw = mod.UTF8ToString(ptr);
    const parsed = JSON.parse(raw) as CompileResult;
    return {
      ok: Boolean(parsed.ok),
      bytecode: parsed.bytecode || "",
      diagnostics: parsed.diagnostics || "",
      debug: parsed.debug || "",
      stdout: parsed.stdout
    };
  } catch (error) {
    return fallbackError(`Could not parse compiler response: ${String(error)}`);
  } finally {
    mod.ccall("free_string", null, ["number"], [ptr]);
  }
}
