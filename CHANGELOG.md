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

## [26.0.3] — 2026-11-28

### Added

- Array initializer lists: `array int a[5] = {1, 2, 3};` writes the values and
  leaves the remaining elements at their default. A literal longer than the
  capacity is a compile error, the length and capacity must be constant when a
  literal is present, and every element is type-checked.
- Whole-array assignment: `a = {9, 8};` replaces the contents, growing the
  array when the literal is longer than the current length and zero-filling the
  tail when it is shorter.
- Prefix assignment: `a[X] = {...};` rewrites the first `X` elements and leaves
  the rest of the array alone. The literal may be shorter than `X` (the tail is
  zero-filled), `\null` leaves its slot untouched, and `X` must be a constant.
- `__roll a[X] = {...};` grows the array when its declared capacity allows it
  and otherwise writes the values in a ring, at index `i % len`.
- Constant arrays: `const array int digits[5] = {1, 2, 3};`. The initializer is
  required, and reassigning the name, assigning an element, or passing the
  array to `grow`/`shrink` is a compile error.
- Struct fields may hold arrays and other structs. An array field starts as a
  fresh empty array, a struct field as a fresh zeroed object.
- `mut` parameters accept a field or an element as well as a variable, which
  is what makes `grow @buffer.data 3` work.

### Changed

- The `.mbc` container version is 3. A struct field is described by a full
  layout instead of a single tag byte, so bytecode written by 26.0.2 or earlier
  cannot be read by 26.0.3.
- `grow` and `shrink` declare their array parameter `mut`, so a constant array
  is rejected at compile time instead of at run time.

### Fixed

- A struct that contains itself, directly or through other fields, is rejected
  instead of recursing forever while its default value is built.
- An array declared in a `for` initializer keeps its declared length in the
  native backend; it used to be built empty there while the VM built it with
  the right length.

## [26.0.2] — 2026-11-27

### Added

- Dynamic arrays with capacity limits:
  `array int a[10]` (fixed), `a[2~5]` (grows to 5), `a[~]` (empty,
  unbounded), `a[1~]` (starts at 1, unbounded), `a[~10]` (empty, at most 10).
- `len`, `grow` and `shrink` as core capabilities, with one overload set per
  element type (`int`, `bool`, `string`).
- `const` bindings: `const int n = 1;` must be initialized and cannot be
  assigned; `mut` parameters also reject constants.
- Methods declared inside `struct` bodies: fields are in scope directly and
  the receiver is the implicit `self` parameter.
- `for x in a` over arrays and `for i in a~b` over integer ranges.
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
