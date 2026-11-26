# Changelog

All notable changes to Moss are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## Versioning

Versions are `[year].[major].[pre-release]`:

- `26.0.1`, `26.0.2`, ... are pre-releases published in 2026 on the way to
  the stable `26.1.0`.
- `26.1.0` is the stable release of that line; the last number is `0` for
  stable releases.
- Pre-releases may change behaviour or syntax without notice; stable
  releases keep compatibility within their major version.

## [26.0.2] — unreleased

### Added

- Dynamic arrays with capacity limits:
  `array int a[10]` (fixed), `a[2~5]` (grows to 5), `a[~]` (empty,
  unbounded), `a[1~]` (starts at 1, unbounded), `a[~10]` (empty, at most 10).
- `len`, `grow` and `shrink` as core capabilities, with one overload set per
  element type (`int`, `bool`, `string`).
- Continuous integration: the compiler is built and the full test suite runs
  on Linux and Windows for every push and pull request.

### Changed

- `build.sh` honours `MOSS_STATIC=0`, for platforms without a static
  libstdc++.

### Fixed

- `moss pack` now marks the packaged program executable on POSIX systems;
  before, the produced file could not be run on Linux.

## [26.0.1] — 2026-11-26

First pre-release towards `26.1.0`; supersedes the initial `26.0.0` upload.

### Added

- Language basics: `int`, `bool`, `string`, `void`, mutable variables with
  default values, functions, `if`/`else`, `while`, C-style `for`, and
  `__go (a ~ b) when (cond)` filtered iteration with the implicit `ex`.
- Capabilities: `pick head { module: @cap as alias; }`, `cap` definitions in
  `.msh` modules, overloading, `mut` parameters, and the built-in native
  `core` module.
- `@` as the mention marker, including mention lists
  (`ask @a @"," @b`) and aliases (`@output as show`).
- `ask` / `input` / `show` I/O, string concatenation and comparison.
- Structs with reference semantics and field access.
- Static arrays with bounds checks, lengths from constants or variables,
  and array parameters.
- Two backends: a bytecode VM with a `.mbc` container, and a native C
  backend that drives gcc.
- `moss build` (native executable), `moss pack` (executable with the VM
  embedded), `moss -c` / `moss run` (bytecode).
- `mossbuild.sh` / `mossbuild.bat` one-click wrappers, which fall back to
  `moss pack` when no C compiler is available.
- Documentation: `README.md`, `BUILDING.md`.

### Known limitations

- No collections beyond arrays, no generics, no first-class functions,
  no enums or pattern matching, no methods, no error handling.
- Generated C leaks strings; programs are meant to be short-lived.
