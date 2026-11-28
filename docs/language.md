# Language reference

This document describes standard `1.0` of Moss. Moss is experimental; details
may still change, but breaking changes only happen with a new standard.

## Source files

A Moss program is one `.ms` file. The first statement declares the language
standard it is written for:

```moss
moss 1.0;
```

`moss 1000;` is accepted as an alias for standard `1.0`; it was the placeholder
before standards existed. The declaration is optional while only one standard
exists, but a compiler that supports several of them refuses one it does not
implement.

## Lexical structure

- Comments: `// line` and `/* block */`.
- Identifiers: `[A-Za-z_][A-Za-z0-9_]*`.
- Integer literals: decimal, non-negative (`-` is a unary operator).
- Boolean literals: `true`, `false`.
- String literals: `"..."` with the escapes `\n`, `\t`, `\\`, `\"`, `\0`.
  Strings are byte sequences; no encoding is assumed by the compiler.

## Types

| Type | Notes |
|---|---|
| `int` | 64-bit signed |
| `bool` | `true` / `false` |
| `string` | immutable byte string; `+` concatenates, `==` / `!=` compare |
| `void` | only as a function result |
| `Point`, `...` | a struct declared in this file |
| `array T` | an array of `T`; `T` may be `int`, `bool` or `string` |
There are no implicit conversions: `int` and `bool` never mix, and an `int`
expression cannot be used as a condition.

## Declarations

```moss
int a, b;          // both default to 0
int c = 3, d;      // c is 3, d is 0
const int n = 10;  // must be initialized; cannot be assigned
```

- Defaults: `0`, `false`, `""`.
- `const` bindings reject assignment, including through `mut` parameters.
- `const array` bindings are arrays; see [Arrays](#arrays).

Array declarations carry a length, or a length range:

```moss
array int a[10];     // fixed length 10
array int b[2~5];    // starts at 2, grows up to 5
array int c[10~];    // starts at 10, unbounded
array int d[~];      // starts empty, unbounded
array int e[~10];    // starts empty, at most 10
```

An array declaration may be initialized with a literal. The literal may be
shorter than the declared length; the remaining elements keep their default.
A literal longer than the capacity is a compile error, and the length and
capacity must be constant when a literal is present.

```moss
array int a[5] = {1, 2, 3};    // {1, 2, 3, 0, 0}
array int g[2~5] = {7, 8, 9};  // grows to length 3
array int h[3~] = {1};         // length 3: {1, 0, 0}
array int e[~10] = {};         // empty

const array int digits[5] = {1, 2, 3};  // cannot be written afterwards
```

Assigning a literal to an array variable replaces its contents: the array
grows when the literal is longer than the current length, and the tail is
zero-filled when it is shorter.

```moss
array int a[5] = {1, 2, 3, 4, 5};
a = {9, 8};          // {9, 8, 0, 0, 0}
```

A write can also be limited to the head of the array:

```moss
array int a[5] = {1, 2, 3, 4, 5};
a[3] = {10, 20, 30};   // {10, 20, 30, 4, 5}
a[3] = {1, \null, 3};  // slot 1 keeps whatever it held
```

- `X` is the number of leading positions written and must be a constant.
- The literal may be shorter than `X`; the positions it does not reach are set
  to the element default.
- `\null` is a hole: that slot is skipped instead of written, and it still
  counts as one of the `X` positions.
- When `X` is greater than the current length, the array grows first if its
  declaration allows it.

`__roll` in front of such a write handles arrays that cannot grow: the values
are written in a ring instead, at index `i % len`, and the length does not
change.

```moss
array int a[8] = {1, 2, 3, 4, 5, 6, 7, 8};
__roll a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
// {9, 0, 3, 4, 5, 6, 7, 8}
```

`__roll` needs exactly `X` values. An unbounded array can always grow, so it
never wraps there; a true carousel needs a bounded declaration.

## Expressions

Precedence, loosest first:

| Level | Operators |
|---|---|
| 1 | `\|\|` |
| 2 | `&&` |
| 3 | `==` `!=` |
| 4 | `<` `<=` `>` `>=` |
| 5 | `+` `-` |
| 6 | `*` `/` `%` |
| 7 | unary `-` `!` `@` |
| 8 | postfix: call `f(...)`, method call `x.m(...)`, member `x.f`, index `x[i]` |

- `+` concatenates strings when both operands are strings.
- `==` / `!=` work on `int`, `bool` and `string`; structs cannot be compared.
- `/` truncates toward zero, `%` takes the sign of the left operand.
- Division by zero is a runtime error in both backends.
- `&&` and `||` short-circuit.
- `@expr` is a *mention*: it is type-transparent and only meaningful in
  capability arguments and `pick` lists.

Functions and capabilities are called with parentheses. A capability that was
imported can also be called in verb form (see below).

## Statements

```moss
{ ... }                                  // block, introduces a scope
if (cond) stmt [else stmt]
while (cond) stmt
for (init; cond; update) stmt             // any clause may be empty
for x in arrayExpr stmt
for i in from~to stmt
__go (from ~ to) [when (cond)] stmt
return [expr];
target = expr;                            // target: variable, field, element
target = literal;                         // whole-array assignment
target[X] = literal;                      // rewrite the first X elements
__roll target[X] = literal;               // grow if possible, else wrap
expr;
```

- `for` follows C: the loop variable declared in `init` is scoped to the loop.
- `for i in a~b` iterates inclusive, ascending, step 1.
- `for x in a` iterates an array's elements in order.
- `__go` also iterates inclusive ascending; `ex` is the implicit loop variable
  and `when` filters iterations. The body runs only when `when` is true.
- Assignment is a statement, not an expression.
- Conditions must be `bool`.

## Functions

```moss
fn int add(int a, int b) {
  return a + b;
}

fn void greet(string name) {
  show("Hello, " + name);
}
```

- The return type comes after `fn`, before the name.
- Parameters are written `type name`, separated by commas.
- A non-`void` function must return on every path.
- `main` must be in the main file, take no parameters or one `string`, and
  return `int` or `void`. A fall-through in `main` returns 0.
- `fn int main(args string argv)` receives the command line joined by spaces;
  `args` is an optional marker with no other meaning.
- `mut` parameters are only allowed on capabilities, not on `fn`.

## Structs and methods

```moss
struct Point {
  int x;
  int y;

  fn int sum() { return x + y; }
  fn void move(int dx, int dy) { x = x + dx; y = y + dy; }
}
```

- Structs are declared in the main file (not in `.msh` modules yet).
- Fields may be `int`, `bool`, `string`, another struct, or an array of
  `int`/`bool`/`string`. An array field starts as a fresh empty array, which
  `grow` can extend; a struct field starts as a fresh zeroed object.
- A struct may not contain itself, directly or through other fields.
- Methods are declared with `fn` inside the body. Their implicit first
  parameter is `self`; fields are directly visible inside a method.
- Methods are resolved statically by receiver type and exact argument types.
  There is no dynamic dispatch.
- Structs have **reference semantics**: assignment and argument passing share
  the same object.
- There are no struct literals and no struct equality yet.

## Arrays

- Arrays have **reference semantics**: passing one to a function lets the
  callee modify the caller's elements.
- Elements are `int`, `bool` or `string`.
- Every index is bounds-checked; out of range is a runtime error.
- Array literals may initialize a declaration or replace a whole array.
- `const` arrays reject assignment, element writes and `grow`/`shrink`.
- `len`, `grow` and `shrink` are capabilities provided by `core`.
- Growing past the declared maximum is a runtime error.

## Capabilities and modules

A `cap` declares a capability in a `.msh` module:

```moss
// lib/io.msh
moss 1000;

pick head {
  core: @print_int, @print_bool, @print_str;
}

cap output(int x): void { print_int(x); }
cap output(b: bool): void { print_bool(b); }
cap output(string s): void { print_str(s); }
```

- Capabilities with a body are written in Moss.
- Capabilities without a body are *native* and provided by the runtime. Only
  the built-in `core` module may declare them.
- Overloading by parameter types is allowed; a call must match exactly one
  overload.
- A module may `pick` from other modules; cycles are an error.

A program picks what it needs:

```moss
pick head {
  core: @ask, @len, @grow, @shrink;
  moss: @output as show;
}
```

- `@name` mentions a capability; `as` renames it locally.
- Only picked names exist in the program: using an unpicked capability is a
  compile error.
- A module name `m` is resolved to `m.msh` next to the source, in the source
  directory's `lib/`, or in the executable's `lib/` (in that order).
- Importing the same name twice is an error.

### Verb calls

If a statement starts with an imported capability name and the next token is
not `(` or `;`, the statement is a *verb call*: the capability receives the
expressions up to `;` as its arguments.

```moss
ask "Age? " @age;      // ask("Age? ", age)
input @a;              // input(a)
show @add(a, b, c);    // show(add(a, b, c))
grow @a 10;            // grow(a, 10)
```

### `mut` parameters

A capability can declare a parameter `mut`; the callee may write to it.

```moss
cap read_int(mut int x): void;
```

At the call site the argument must be a mutable variable. Constants and
literals are rejected. `@` is optional but conventional.

## Grammar

A compact EBNF; whitespace and comments are insignificant.

```
program   := [ "moss" INT ";" ] pick* struct* function*
pick      := "pick" "head" "{" ( ident ":" cap ("," cap)* ";" )* "}"
cap       := "@" ident [ "as" ident ]
struct    := "struct" ident "{" ( type ident ";" | function )* "}" [";"]
function  := "fn" type ident "(" [ "args" ] params ")" block
params    := [ param ("," param)* ]
param     := [ "mut" ] type ident
block     := "{" statement* "}"
statement := block
           | "const" type ident "=" expr ("," ident "=" expr)* ";"
           | "const" "array" type ident "[" length "]" "=" literal
             ("," ident "[" length "]" "=" literal)* ";"
           | "array" type ident "[" length "]" ["=" literal]
             ("," ident "[" length "]" ["=" literal])* ";"
           | type ident ["=" expr] ("," ident ["=" expr])* ";"
           | "if" "(" expr ")" statement ["else" statement]
           | "while" "(" expr ")" statement
           | "for" "(" [simple] ";" [expr] ";" [simple] ")" statement
           | "for" ident "in" expr ("~" expr)? statement
           | "__go" "(" expr "~" expr ")" ["when" "(" expr ")"] statement
           | "return" [expr] ";"
           | lvalue "=" expr ";"
           | lvalue "=" literal ";"
           | lvalue "[" expr "]" "=" literal ";"
           | "__roll" lvalue "[" expr "]" "=" literal ";"
           | ident expr* ";"                 (* verb call *)
           | expr ";"
expr      := binary expression with the precedence in the table above
lvalue    := ident | lvalue "." ident | lvalue "[" expr "]"
literal   := "{" [ elem ("," elem)* ] "}"
elem      := expr | "\null"
length    := [expr] ["~" [expr]]
```

## Current limitations

- No collections other than arrays, no generics, no first-class functions,
  no enums or pattern matching, no error handling, no 2D arrays.
- Arrays cannot hold structs yet, and arrays cannot hold arrays.
- A prefix write (`a[X] = {...}`) and `__roll` need a constant `X`, and their
  target must be a plain array variable.
- Structs and `const` arrays have the restrictions listed above.
- Generated C leaks strings; programs are meant to be short-lived.

## Bytecode files

`.mbc` files carry a format version. 26.0.3 writes version 3; files from
26.0.2 and earlier are rejected with `unsupported bytecode version`.
