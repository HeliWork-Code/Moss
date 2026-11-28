# Openness

Moss wants to be the most open language in the sense that matters to library
authors: **anything that can be a library is a library.**

## Rule 1 — the compiler translates

The compiler owns only what cannot be written in Moss:

- the grammar and the type checker;
- the two backends (bytecode VM, C);
- the `.mbc` container;
- the native `core` capabilities that touch the operating system.

Everything else is a module. The compiler translates code; it does not build
the language's features for you.

## Rule 2 — anything expressible in Moss is a library

If a proposal can be written as a `.msh` module, it is rejected from the
compiler, however convenient it would be there. Standard library collections,
string helpers, math, file formats: all modules.

## What the type system must allow

Library authors must be able to define **types**, not just functions:

- a struct field may declare its capacity range: `array int slots[8~16];`
- an array may hold a user-defined type: `array Point pts[2~5];`
- a type may live in a `.msh` module, so a library can publish it;
- later, generic types, so a library can ship `Vec<T>` and `Map<K, V>`.

## What openness does not mean

Openness is not "no rules". The smaller and more precise the core is, the more
the libraries can do without asking permission. `core` stays the author's
responsibility; everything above it is fair game for anyone.
