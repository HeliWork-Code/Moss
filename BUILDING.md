# Building Moss

This document covers building the Moss toolchain from source, building Moss
programs, and running the test suite.

## What gets built

| Binary | Built from | Purpose |
|---|---|---|
| `moss.exe` | all of `src/*.cpp` | the compiler: front end, type checker, bytecode backend, C backend, and the built-in VM |
| `mossrt.exe` | `src/bytecode.cpp`, `src/serialize.cpp`, `src/vm.cpp`, `src/rt/rt_main.cpp` | the standalone runtime used by `moss pack` to produce self-contained executables |

Both are linked statically on Windows, so they run without the toolchain's
runtime DLLs.

## Requirements

| Requirement | Needed for | Notes |
|---|---|---|
| A C++17 compiler (`g++` or `clang++`) | building `moss.exe` and `mossrt.exe` | required |
| `gcc` (any C11 compiler) | `moss build` (native executables) | **optional**; without it, use `moss pack` |
| `bash` | `build.sh` and the test scripts | on Windows: Git Bash or MSYS2 |

Moss programs can be compiled and run **without** `gcc`; only the native
backend needs a C compiler.

## Quick start

### Windows — MSYS2 (recommended)

1. Install [MSYS2](https://www.msys2.org/) and open the **UCRT64** shell.
2. Install the toolchain:

   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc
   ```

3. Make sure the toolchain is on `PATH` (the UCRT64 shell does this for you):

   ```bash
   export PATH=/ucrt64/bin:$PATH
   ```

4. Build:

   ```bash
   cd /path/to/moss
   bash build.sh
   ```

### Windows — WinLibs (single download, no package manager)

1. Download a WinLibs GCC build and unzip it, e.g. to `C:\mingw64`.
2. Either add `C:\mingw64\bin` to `PATH`, or pass the location explicitly:

   ```bash
   MOSS_MINGW=/c/mingw64/bin bash build.sh
   ```

### Windows — Git Bash with another MinGW

If `g++` is not on `PATH`, point `CXX` at it:

```bash
CXX="/c/msys64/ucrt64/bin/g++.exe" bash build.sh
```

### Linux (Debian/Ubuntu)

```bash
sudo apt install g++
bash build.sh
```

`build.sh` passes `-static`; if your distribution does not provide static
libstdc++, run `MOSS_STATIC=0 bash build.sh` (or build manually, below).

### macOS

```bash
xcode-select --install
bash build.sh
```

The `-static` flags in `build.sh` are not supported by the macOS toolchain.
Disable them:

```bash
MOSS_STATIC=0 bash build.sh
```

or build manually:

```bash
g++ -std=c++17 -O2 -Isrc -o moss.exe src/*.cpp
g++ -std=c++17 -O2 -Isrc -o mossrt.exe \
  src/bytecode.cpp src/serialize.cpp src/vm.cpp src/rt/rt_main.cpp
```

(`g++` on macOS is an alias for `clang++`; the binaries keep the `.exe`
suffix because the scripts look for those names.)

## Building manually

If you would rather not use `build.sh`:

```bash
# the compiler
g++ -std=c++17 -O2 -Isrc -o moss.exe src/*.cpp

# the standalone runtime
g++ -std=c++17 -O2 -Isrc -o mossrt.exe \
  src/bytecode.cpp src/serialize.cpp src/vm.cpp src/rt/rt_main.cpp
```

Add `-static -static-libgcc -static-libstdc++` on Windows if you want
self-contained executables.

Verify the build:

```bash
./moss.exe --help
echo "1 3 5" | ./moss.exe examples/sum.ms      # prints 9
```

## Environment variables

| Variable | Used by | Meaning |
|---|---|---|
| `CXX` | `build.sh` | C++ compiler to use (default `g++`) |
| `MOSS_MINGW` | `build.sh`, test scripts, `mossbuild.sh` | directory containing `gcc`/`g++`, prepended to `PATH` |
| `MOSS_CC` | `moss build`, `mossbuild.*` | C compiler for native builds (default: `gcc` on `PATH`) |
| `MOSS_BACKEND` | `tests/run.sh` | `vm` (default) or `native` |
| `MOSS_STATIC` | `build.sh` | `0` disables static linking (needed on distributions without static libstdc++) |

When `MOSS_CC` is a full path, `moss` adds its directory to the child
process `PATH` automatically, so the compiler's helper processes can find
their DLLs.

## Building Moss programs

| Command | Needs `gcc`? | Result |
|---|---|---|
| `moss prog.ms` | no | compile to bytecode and run on the VM |
| `moss -c prog.ms -o prog.mbc` | no | bytecode file |
| `moss run prog.mbc` | no | run a bytecode file |
| `moss pack prog.ms -o prog.exe` | no | self-contained executable (VM embedded, ~2.9 MB) |
| `moss build prog.ms -o prog.exe` | yes | native executable (~80 KB) |
| `moss build --emit-c prog.ms -o prog.c` | no | write the generated C and stop |

One-click wrappers, which pick `build` when a compiler is available and fall
back to `pack` otherwise:

```bash
./mossbuild.sh prog.ms              # -> prog.exe next to the source
./mossbuild.sh prog.ms -o app.exe   # explicit output
./mossbuild.sh prog.ms -p           # force the embedded-VM build
```

```bat
mossbuild.bat prog.ms
mossbuild.bat prog.ms -o app.exe
mossbuild.bat prog.ms -p
```

## Running the tests

```bash
bash tests/run.sh          # interpreter backend
bash tests/native.sh       # native C backend (needs gcc)
bash tests/packaging.sh    # bytecode files and packaged executables (needs gcc)
```

Both backends are run against the same programs, so a difference between them
is a bug.

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| `cc1plus.exe: error while loading shared libraries: libssp-0.dll` | `g++` found but its directory is not on `PATH` | add the MinGW `bin` directory to `PATH`, or use `MOSS_MINGW` |
| `'gcc' is not recognized as an internal or external command` | no C compiler on `PATH` while running `moss build` | `set MOSS_CC=C:\msys64\ucrt64\bin\gcc.exe`, or use `moss pack` |
| `C compiler failed; ... The generated C is at X.c` | the C compiler rejected the generated code | inspect the `.c` file; check `MOSS_CC` points at a C11 compiler |
| `standalone runtime 'mossrt.exe' not found next to moss` | `moss pack` cannot find `mossrt.exe` | build it with `build.sh`, or put it next to `moss.exe` |
| `cannot find module 'moss.msh'` | `lib/` is missing next to the binary | keep `lib/moss.msh` next to `moss.exe` when distributing |
| A path with spaces fails to compile | unquoted path | quote it: `mossbuild.bat "C:\My Programs\a.ms"` |
| `-static` link errors (Linux/macOS) | no static libstdc++ | `MOSS_STATIC=0 bash build.sh`, or build manually without the flags |
