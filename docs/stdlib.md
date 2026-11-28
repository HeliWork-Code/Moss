# Standard library

Moss has no built-in functions. Everything below is a *capability* declared in
a module, and a program only sees what it picks.

## The `core` module

`core` is provided by the compiler and is the bottom of the capability stack:
its capabilities have no Moss body and are implemented by the runtime (the VM
for `moss prog.ms`, the generated C runtime for `moss build`).

### Output

| Capability | Effect |
|---|---|
| `print_int(int x): void` | prints an integer and a newline |
| `print_bool(bool b): void` | prints `true` or `false` and a newline |
| `print_str(string s): void` | prints a string and a newline |

### Input

| Capability | Effect |
|---|---|
| `read_int(mut int x): void` | reads one whitespace-separated integer |
| `read_bool(mut bool x): void` | reads `true`/`false`/`1`/`0` |
| `read_str(mut string s): void` | reads one whitespace-separated word |
| `read_int_sep(mut int a, string sep, mut int b): void` | reads an integer, consumes `sep`, reads another integer |

### `ask`

`ask` prints an optional prompt and then reads. It is the language's "问":

| Capability | Effect |
|---|---|
| `ask(mut int x): void` | reads an integer |
| `ask(string prompt, mut int x): void` | prints `prompt`, then reads |
| ... the same two forms for `bool` and `string` | |

The prompt is printed without a newline.

### Arrays

| Capability | Effect |
|---|---|
| `len(array int a): int` | number of elements |
| `len(array bool a): int` | |
| `len(array string a): int` | |
| `grow(mut array T a, int n): void` | adds `n` elements (default values) |
| `shrink(mut array T a, int n): void` | removes `n` elements from the end |

`grow` and `shrink` take their array as a `mut` parameter, so the argument must
be a writable place: a variable, a struct field or an array element. A `const`
array is rejected at compile time.

`grow` fails at runtime when the array declares a maximum length and the new
length would exceed it. Both fail on negative counts, and `shrink` cannot go
below zero.

## The `moss` module

`lib/moss.msh` is written in Moss on top of `core`:

| Capability | Effect |
|---|---|
| `output(int x): void` | prints an integer and a newline |
| `output(bool b): void` | prints a boolean and a newline |
| `output(string s): void` | prints a string and a newline |
| `pair(int a, int b): void` | prints two integers, one per line |

Typical import:

```moss
pick head {
  core: @ask, @len, @grow;
  moss: @output as show;
}
```

## Writing your own module

A module is a `.msh` file. It may pick from other modules and declare
capabilities; capabilities with a body are written in Moss:

```moss
// math.msh
moss 1000;

cap abs(int x): int {
  if (x < 0) {
    return -x;
  }
  return x;
}

cap max(int a, int b): int {
  if (a > b) {
    return a;
  }
  return b;
}
```

Use it from a program:

```moss
pick head {
  core: @print_int;
  math: @abs, @max as bigger;
}

fn int main() {
  print_int(abs(-7));      // 7
  print_int(bigger(3, 9)); // 9
  return 0;
}
```

Rules:

- Module `m` is looked up as `m.msh` next to the source file, then in the
  source directory's `lib/`, then in the executable's `lib/`.
- Only picked names are visible; within a module, all of its own capabilities
  are visible to each other.
- Overloads share a name; a call must match exactly one signature.
- Duplicate imported names and circular imports are compile errors.
- Capabilities without a body are *native* and may only appear in `core`.

## Aliases

`as` renames a capability locally, which is how the examples turn `output`
into `show` and `read_int` into `input`:

```moss
pick head {
  core: @read_int as input;
  moss: @output as show;
}
```

Aliases are also useful to shorten long names, or to resolve two modules that
provide the same capability name.
