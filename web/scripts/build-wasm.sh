#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WEB_DIR="$ROOT_DIR/web"
OUT_DIR="$WEB_DIR/public/wasm"

if ! command -v emcc >/dev/null 2>&1; then
  if [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
    # shellcheck source=/dev/null
    source "$HOME/emsdk/emsdk_env.sh" >/dev/null
  fi
fi

if ! command -v emcc >/dev/null 2>&1; then
  echo "emcc was not found. Run: source ~/emsdk/emsdk_env.sh" >&2
  exit 1
fi

export EM_CACHE="$WEB_DIR/.emcache"
mkdir -p "$OUT_DIR" "$EM_CACHE"

emcc \
  "$WEB_DIR/wasm/durins_bindings.cpp" \
  "$ROOT_DIR/src/lexer/lexer.cpp" \
  "$ROOT_DIR/src/parser/parser.cpp" \
  "$ROOT_DIR/src/semantic/semantic.cpp" \
  "$ROOT_DIR/src/tac/tac.cpp" \
  "$ROOT_DIR/src/tac/optimizer.cpp" \
  "$ROOT_DIR/src/codegen/codegen.cpp" \
  -std=c++17 \
  -O2 \
  -I"$ROOT_DIR/src" \
  -I"$ROOT_DIR/third_party" \
  -s MODULARIZE=1 \
  -s EXPORT_ES6=1 \
  -s ENVIRONMENT=web \
  -s EXPORTED_FUNCTIONS='["_compile_source","_compile_debug","_free_string"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall","UTF8ToString"]' \
  -s ALLOW_MEMORY_GROWTH=1 \
  -o "$OUT_DIR/durins.js"

echo "Built $OUT_DIR/durins.js and $OUT_DIR/durins.wasm"
