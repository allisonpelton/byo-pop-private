#!/usr/bin/env bash
# Rebuild the port (build/native/typershark.exe) with the tool paths of tools/build.sh.
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
export PATH="${APPDATA:-}/Python/Python312/Scripts:$PATH"
cmake --build "$ROOT/build/native" --target typershark 2>&1 | tail -${1:-6}
