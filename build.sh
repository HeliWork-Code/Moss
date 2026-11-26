#!/usr/bin/env bash
# Builds the Moss compiler (moss) and the standalone VM runtime (mossrt).
#
# Requires a C++17 compiler. Set CXX to choose one; it defaults to g++.
set -eu
cd "$(dirname "$0")"

# Optional: point MOSS_MINGW at the directory holding gcc/g++ when they are
# not already on PATH.
if [ -n "${MOSS_MINGW:-}" ]; then
  export PATH="$MOSS_MINGW:$PATH"
fi

CXX="${CXX:-g++}"
if ! command -v "$CXX" >/dev/null 2>&1; then
  echo "error: '$CXX' not found. Install a C++17 compiler or set CXX." >&2
  exit 1
fi

FLAGS="-std=c++17 -O2 -Wall -Wextra -Isrc"

# Static linking keeps the produced executables free of the toolchain's
# runtime DLLs on Windows. Drop these flags if your platform does not
# support them.
STATIC="-static -static-libgcc -static-libstdc++"

"$CXX" $FLAGS $STATIC -o moss.exe src/*.cpp
"$CXX" $FLAGS $STATIC -o mossrt.exe \
  src/bytecode.cpp src/serialize.cpp src/vm.cpp src/rt/rt_main.cpp

echo "built: $(pwd)/moss.exe"
echo "built: $(pwd)/mossrt.exe"
