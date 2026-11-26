#!/usr/bin/env bash
# One-click build for Moss.
#
#   ./mossbuild.sh program.ms             -> program.exe next to the source
#   ./mossbuild.sh program.ms -o app.exe  -> explicit output path
#
# With gcc available this produces a native executable (moss build).
# Without gcc it falls back to the embedded-VM executable (moss pack).
# Use -p/--pack to force the embedded-VM form.
set -u

HERE="$(cd "$(dirname "$0")" && pwd)"
MOSS="$HERE/moss.exe"

usage() {
  cat <<'EOF'
Moss one-click build

  mossbuild.sh <file.ms> [-o out.exe] [-p]

With gcc: native executable. Without gcc: embedded-VM executable.
-p / --pack forces the embedded-VM form.
Without -o the output is placed next to the source file.
EOF
}

src=""
out=""
pack_only=0
while [ $# -gt 0 ]; do
  case "$1" in
    -h|--help) usage; exit 0 ;;
    -p|--pack) pack_only=1; shift ;;
    -o)
      shift
      if [ $# -eq 0 ]; then
        echo "[mossbuild] error: -o needs a file name" >&2
        exit 1
      fi
      out="$1"; shift ;;
    -*)
      echo "[mossbuild] error: unknown option '$1'" >&2; exit 1 ;;
    *)
      if [ -n "$src" ]; then
        echo "[mossbuild] error: more than one source file" >&2; exit 1
      fi
      src="$1"; shift ;;
  esac
done

if [ -z "$src" ]; then
  usage
  exit 1
fi

if [ ! -f "$MOSS" ]; then
  echo "[mossbuild] error: moss.exe not found next to this script" >&2
  exit 1
fi
if [ ! -f "$src" ]; then
  echo "[mossbuild] error: source file not found: $src" >&2
  exit 1
fi

if [ -z "$out" ]; then
  out="${src%.*}.exe"
fi

have_cc=0
if [ -n "${MOSS_CC:-}" ] || command -v gcc >/dev/null 2>&1; then
  have_cc=1
fi

if [ "$pack_only" -eq 1 ] || [ "$have_cc" -eq 0 ]; then
  if [ "$pack_only" -eq 0 ]; then
    echo "[mossbuild] gcc not found; using the embedded VM (moss pack)." >&2
    echo "[mossbuild]   Install MinGW-w64/MSYS2 or set MOSS_CC for native executables." >&2
  fi
  "$MOSS" pack "$src" -o "$out"
else
  "$MOSS" build "$src" -o "$out"
fi

rc=$?
if [ $rc -ne 0 ]; then
  echo "[mossbuild] build failed." >&2
  exit $rc
fi
echo "[mossbuild] done: $out"
