# Durin's Code Web Playground

This folder is a standalone web showcase for Durin's Code. It compiles the existing C++ compiler pipeline to WebAssembly with Emscripten and runs the generated JSON bytecode in a browser TypeScript VM.

The main compiler source under `../src`, top-level CMake files, tests, and examples are not modified by this web app.

## Requirements

- Node.js and npm
- Emscripten SDK with `emcc` available on `PATH`

If `emcc` is installed but not visible in the terminal, load it first:

```bash
source ~/emsdk/emsdk_env.sh
```

## Install

```bash
cd web
npm install
```

## Build Wasm

```bash
npm run build:wasm
```

This writes:

- `public/wasm/durins.js`
- `public/wasm/durins.wasm`

## Run Locally

```bash
npm run dev
```

Open the printed local URL and use the example picker. The recommended demo is `04_middle_earth.dc` with:

```text
take ring
go east
go east
destroy ring
```

## Production Build

```bash
npm run build
npm run preview
```

The final static site can be deployed to Vercel, Netlify, or GitHub Pages.

## Notes

- The browser runtime intentionally reimplements the VM in TypeScript instead of porting the terminal `runGameLoop()`.
- The compiler Wasm build excludes `src/main.cpp` and `src/vm/vm.cpp`.
- The current language has a known `||` caveat in TAC generation, so the showcased examples avoid OR-heavy logic.
