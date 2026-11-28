# Moss

[![CI](https://github.com/HeliWork-Code/Moss/actions/workflows/ci.yml/badge.svg)](https://github.com/HeliWork-Code/Moss/actions/workflows/ci.yml)

> a little lang,not so hard.enjoy this =).

A small, statically typed language built around **capabilities**: nothing is
built in except the ability to `pick` what you need.

> Status: experimental. Versions are `[year].[major].[pre-release]`, so
> `26.0.x` are pre-releases on the way to `26.1.0`. See
> [CHANGELOG.md](CHANGELOG.md).

```
moss 1000;

pick head {
  core: @read_int as input;
  moss: @output as show;
}

fn int add(int a, int b, int c) {
  return a + b + c;
}

fn int main() {
  int a, b, c;
  input @a;
  input @b;
  input @c;
  show @add(a, b, c);
  return 0;
}
```

## The idea

- **Capabilities, not libraries.** `input` and `show` above are not compiler
  magic. They are capabilities declared in `moss.msh`, which itself is written
  in Moss on top of the native `core` module. `pick` brings exactly the names
  you ask for into scope; anything you did not pick simply does not exist.
- **`@` means "mention".** `@output` mentions a capability; `ask @age` mentions
  the thing the verb acts on. A run of mentions forms an argument list, so
  `read_int_sep @a @"," @b;` reads two numbers separated by a comma.
- **`ask` asks.** It prints an optional prompt and then reads:
  `ask "Age? " @age;`

## Getting started

Requirements:

- a C++17 compiler (`g++` or `clang++`) to build the compiler itself
- `gcc` on `PATH` (or `MOSS_CC` set) only if you want native executables

See [BUILDING.md](BUILDING.md) for a detailed, platform-by-platform guide.

```bash
bash build.sh                 # builds moss.exe and mossrt.exe

./moss.exe examples/sum.ms                # compile and run
echo "1 3 5" | ./moss.exe examples/sum.ms

./moss.exe build examples/sum.ms -o sum.exe   # native executable (needs gcc)
./moss.exe build --emit-c examples/sum.ms -o sum.c   # just show the C
./moss.exe pack examples/sum.ms -o sum.exe    # embedded-VM executable
```

## Documentation

- [Tutorial](docs/tutorial.md) — start here
- [Language reference](docs/language.md) — types, statements, modules
- [Standard library](docs/stdlib.md) — every capability `core` and `moss` provide
- [Building](BUILDING.md) — toolchain requirements, native builds
- [Changelog](CHANGELOG.md)

## Language tour

```moss
moss 1000;

pick head {
  core: @read_int as input;
  moss: @output as show;
}

struct Point {
  int x;
  int y;
}

fn int sum(Point p) {
  return p.x + p.y;
}

fn int main() {
  // declarations are mutable and default to zero
  int a, b;
  a = 3;
  b = 4;

  // constants must be initialized and cannot be assigned
  const int limit = 10;

  // three loop forms
  for (int i = 1; i <= 3; i = i + 1) {
    show @i;
  }

  while (a < b) {
    a = a + 1;
  }

  // filtered iteration; `ex` is implicit
  __go (1 ~ 5) when (ex % 2 == 0) {
    show @ex;
  }

  // structs are references
  Point p;
  p.x = 3;
  p.y = 4;
  show @sum(p);
  return 0;
}
```

Arrays default to zero and are bounds-checked. A length range makes an array
growable:

```moss
array int a[10];        // fixed length 10
array int b[2~5];       // starts at 2, may grow to 5
array int c[~];         // empty, unbounded
array string d[1~];     // starts at 1, unbounded

a[3] = 7;
show @a[3];

grow @b 2;              // now 4 elements
shrink @b 1;
show @len(b);           // 3
```

Structs can declare methods; fields are in scope directly and `self` is the
receiver:

```moss
struct Point {
  int x;
  int y;
  fn int sum() { return x + y; }
}

Point p;
p.x = 3;
p.y = 4;
show @p.sum();   // 7
```

`for ... in` walks a range or an array:

```moss
for i in 1~3 {
  show @i;
}

array int a[3];
for v in a {
  show @v;
}
```

## Two backends

| Command | Output |
|---|---|
| `moss prog.ms` | compile to bytecode and run on the built-in VM |
| `moss -c prog.ms -o prog.mbc` | bytecode file |
| `moss run prog.mbc` | run a bytecode file |
| `moss build prog.ms -o prog.exe` | native executable via C and gcc |
| `moss pack prog.ms -o prog.exe` | executable with the VM embedded |

Both backends are held to the same test suite:

```bash
bash tests/run.sh                      # interpreter
bash tests/native.sh                   # native backend
bash tests/packaging.sh                # bytecode + packaged executables
```

`main` may take the command line as one string; `args` is an optional marker:

```moss
fn int main(args string argv) {
  show @argv;      // "one two" when run as: prog.exe one two
  return 0;
}
```

## Project layout

```
src/
  token.h lexer.*        lexer
  ast.*                  syntax tree
  parser.*               recursive-descent parser
  loader.cpp             pick resolution, module loading, scopes
  typeck.*               type checker and call resolution
  codegen.*              AST -> bytecode
  bytecode.*             instruction set, value representation
  vm.*                   stack VM
  c_backend.*            AST -> C
  runtime_c.cpp          the C runtime embedded into native builds
  serialize.*            bytecode container (.mbc)
  main.cpp               command line driver
  rt/rt_main.cpp         standalone VM runtime (mossrt)
lib/moss.msh             the standard module, written in Moss
tests/                   language tests and backend comparisons
examples/                small programs
```

## Status and limitations

Works today: `int`, `bool`, `string`, `void`, `const` bindings and constant
arrays, structs (reference semantics, methods with implicit `self`, fields that
hold arrays or other structs), arrays with fixed, runtime or growable lengths
plus bounds and capacity checks, array literals and whole-array assignment,
`len`/`grow`/`shrink`, functions, `if`/`else`, `while`, `for`,
`for ... in` over ranges and arrays, `__go`, capabilities with `pick`,
aliasing, overloads, `mut` parameters, `@` mentions, string concatenation
and comparison, `ask`/`input`/`show` style I/O.

Not yet: collections (lists, maps), first-class functions, enums and pattern
matching, generics, struct literals, structs in modules, error handling,
arrays of structs, 2D arrays.

Generated C leaks strings (no deallocator yet); programs are meant to be
short-lived.

## License

MIT — see [LICENSE](LICENSE). Copyright (c) 2026 HeliWork Studios.

---

## 中文简介

Moss 是一门围绕"能力"构建的静态类型小语言：语言核心什么都不会，除了 `pick`
——你挑什么能力，语言就有什么；没挑的东西根本不存在。`@` 表示"提及"，`ask`
既能打印提示又能读入，结构体采用引用语义。编译器自带字节码虚拟机，并能把程序
翻译成 C 再交给 gcc 生成真正的机器码。协议为 MIT。
