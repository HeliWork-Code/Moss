#!/usr/bin/env bash
# Verifies the packaging pipeline: bytecode files and standalone executables.
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# Optional: point MOSS_MINGW at the directory holding gcc/g++ when they are
# not already on PATH.
if [ -n "${MOSS_MINGW:-}" ]; then
  export PATH="$MOSS_MINGW:$PATH"
fi
MOSS="$ROOT/moss.exe"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

pass=0
fail=0
check() {  # check <name> <expected> <actual>
  if [ "$2" = "$3" ]; then
    echo "ok   $1"
    pass=$((pass + 1))
  else
    echo "FAIL $1: expected [$2] got [$3]"
    fail=$((fail + 1))
  fi
}

# 1. Compile to bytecode and run it through the interpreter.
"$MOSS" -c "$ROOT/tests/hello.ms" -o "$TMP/hello.mbc" > /dev/null
check "bytecode round-trip" "Hello World from Moss!" "$("$MOSS" run "$TMP/hello.mbc")"

# 2. Build a standalone executable and run it.
"$MOSS" pack "$ROOT/tests/hello.ms" -o "$TMP/hello.exe" > /dev/null
check "standalone hello" "Hello World from Moss!" "$("$TMP/hello.exe")"

# 3. A standalone executable that reads stdin.
"$MOSS" pack "$ROOT/tests/input_sep.ms" -o "$TMP/sum.exe" > /dev/null
check "standalone with input" "4" "$(echo "1,3" | "$TMP/sum.exe")"

echo "----------------------------------------"
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
