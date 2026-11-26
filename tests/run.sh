#!/usr/bin/env bash
# Moss test runner.
#   tests/NAME.ms + tests/NAME.expected  -> output must match
#   optional tests/NAME.in               -> fed to stdin
#   tests/err_*.ms                       -> must fail with a compile error
#
# MOSS_BACKEND=vm (default) runs the interpreter; MOSS_BACKEND=native builds a
# real executable with the C backend and runs that instead.
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# Optional: point MOSS_MINGW at the directory holding gcc/g++ when they are
# not already on PATH.
if [ -n "${MOSS_MINGW:-}" ]; then
  export PATH="$MOSS_MINGW:$PATH"
fi
MOSS="$ROOT/moss.exe"
BACKEND="${MOSS_BACKEND:-vm}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

pass=0
fail=0

for f in "$ROOT"/tests/*.ms; do
  name="$(basename "$f" .ms)"
  out="$TMP/out.txt"
  err="$TMP/err.txt"

  if [[ "$name" == err_* ]]; then
    if [ "$BACKEND" = native ]; then
      "$MOSS" build "$f" -o "$TMP/errprog.exe" > "$out" 2> "$err"
      rc=$?
    else
      "$MOSS" "$f" > "$out" 2> "$err"
      rc=$?
    fi
    if [ "$rc" -ne 0 ] && grep -q "error" "$err"; then
      echo "ok   $name (rejected: $(head -1 "$err" | cut -c1-70))"
      pass=$((pass + 1))
    else
      echo "FAIL $name: expected a compile error, got rc=$rc"
      fail=$((fail + 1))
    fi
    continue
  fi

  if [ "$BACKEND" = native ]; then
    "$MOSS" build "$f" -o "$TMP/prog.exe" > /dev/null 2> "$err"
    if [ $? -ne 0 ]; then
      echo "FAIL $name: native build failed: $(head -1 "$err")"
      fail=$((fail + 1))
      continue
    fi
    if [ -f "$ROOT/tests/$name.in" ]; then
      "$TMP/prog.exe" < "$ROOT/tests/$name.in" > "$out" 2> "$err"
    else
      "$TMP/prog.exe" > "$out" 2> "$err"
    fi
    rc=$?
  else
    if [ -f "$ROOT/tests/$name.in" ]; then
      "$MOSS" "$f" < "$ROOT/tests/$name.in" > "$out" 2> "$err"
    else
      "$MOSS" "$f" > "$out" 2> "$err"
    fi
    rc=$?
  fi

  if [ "$rc" -ne 0 ]; then
    echo "FAIL $name: runtime failure (rc=$rc): $(head -1 "$err")"
    fail=$((fail + 1))
    continue
  fi

  if diff -q <(sed 's/\r$//' "$out") "$ROOT/tests/$name.expected" > /dev/null; then
    echo "ok   $name"
    pass=$((pass + 1))
  else
    echo "FAIL $name: output differs"
    diff <(sed 's/\r$//' "$out") "$ROOT/tests/$name.expected" | head -10 | sed 's/^/     /'
    fail=$((fail + 1))
  fi
done

echo "----------------------------------------"
echo "[$BACKEND] $pass passed, $fail failed"
[ "$fail" -eq 0 ]
