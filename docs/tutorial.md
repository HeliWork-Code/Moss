# Tutorial

This walkthrough goes from a fresh checkout to a program that reads input,
uses arrays, and defines a struct with methods. It assumes you have built
the compiler (see [BUILDING.md](../BUILDING.md)).

## 1. Hello, world

Create `hello.ms`:

```moss
moss 1000;

pick head {
  moss: @output as show;
}

fn int main() {
  show("Hello, world!");
  return 0;
}
```

Run it:

```bash
./moss.exe hello.ms
```

Three things are worth noticing:

- `moss 1000;` is the optional version line; it must be the first statement.
- `pick head { ... }` brings capabilities into scope. `show` is not a keyword:
  it is the alias *we chose* for the `output` capability of the `moss` module.
- `main` returns an `int`, which becomes the program's exit status.

## 2. Asking the user

```moss
moss 1000;

pick head {
  core: @ask;
  moss: @output as show;
}

fn int main() {
  int age;
  ask "How old are you? " @age;
  show @(age + 1);
  return 0;
}
```

`ask` prints its prompt and then reads into the mentioned variable. The `@`
marks the thing being mentioned; it is required for `ask`, and optional
elsewhere.

## 3. Variables and constants

```moss
int a, b;             // both default to 0
a = 3;
b = a * 2;

const int limit = 5;  // const needs an initializer
// limit = 6;          // compile error: cannot assign to a constant
```

Operators, from loosest to tightest binding:

```
||
&&
==  !=
<   <=  >   >=
+   -
*   /   %
-  !        (unary)
()  []  .   (calls, indexing, fields)
```

`int` is 64-bit. `/` truncates toward zero and `%` follows the sign of the
left operand, as in C. Strings support `+` (concatenation) and `==` / `!=`;
there is no string ordering or indexing yet.

## 4. Branching and loops

```moss
if (a > b) {
  show("bigger");
} else if (a == b) {
  show("equal");
} else {
  show("smaller");
}

while (a < 10) {
  a = a + 1;
}

for (int i = 0; i < 3; i = i + 1) {
  show @i;
}

for i in 1~3 {          // inclusive integer range
  show @i;
}

array int xs[3];
for x in xs {           // elements of an array
  show @x;
}

__go (1 ~ 10) when (ex % 2 == 0) {
  show @ex;             // ex is the implicit loop variable
}
```

`__go` is filtered iteration: it walks the range and runs the body only where
the `when` condition holds.

## 5. Arrays

```moss
array int fixed[10];      // exactly 10 elements
array int growing[2~5];   // starts at 2, may grow to 5
array int open[~];        // empty, no upper bound

// A literal fills the array; the tail keeps the default value.
array int squares[4] = {0, 1, 4, 9};
const array int primes[3] = {2, 3, 5};   // read-only

// A prefix rewrites only the leading elements.
squares[2] = {7, 7};     // {7, 7, 4, 9}

fixed[3] = 7;
show @fixed[3];

grow @growing 2;          // now 4 elements
shrink @growing 1;        // now 3
show @len(growing);       // 3
```

Elements default to `0`, `false` or `""`. All indexing is bounds-checked, and
growing past the declared maximum is a runtime error. Arrays are references:
passing one to a function lets the callee write to the same elements.

```moss
fn void fill(array int a, int n) {
  for (int i = 0; i < n; i = i + 1) {
    a[i] = i * i;
  }
}
```

## 6. Structs and methods

```moss
struct Point {
  int x;
  int y;

  fn int sum() {
    return x + y;          // fields are in scope; `self` is the receiver
  }

  fn void move(int dx, int dy) {
    x = x + dx;
    y = y + dy;
  }
}

fn int main() {
  Point p;
  p.x = 3;
  p.y = 4;
  show @p.sum();           // 7
  p.move(1, 1);
  show @p.sum();           // 9
  return 0;
}
```

Structs are references too: `q = p` makes both names point at the same
object. Fields may be `int`, `bool`, `string`, another struct, or an array of
`int`/`bool`/`string`; an array field starts empty and can be grown.

## 7. Building executables

```bash
./moss.exe prog.ms                      # run on the built-in VM
./moss.exe build prog.ms -o prog.exe    # native executable (needs gcc)
./moss.exe pack prog.ms -o prog.exe     # embedded-VM executable (no gcc)
```

Or use the wrappers, which choose for you:

```bash
./mossbuild.sh prog.ms        # native if gcc is available, else packed
```

## 8. Where to go next

- [Language reference](language.md) for the exact rules.
- [Standard library](stdlib.md) for every capability `core` and `moss`
  provide.
- `examples/` and `tests/` in the repository are full of small programs.
- `moss.exe build --emit-c prog.ms -o prog.c` shows the C your program turns
  into.
