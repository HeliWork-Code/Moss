# Moss documentation

| Document | Read it when |
|---|---|
| [Tutorial](tutorial.md) | you are new to Moss and want to write a program |
| [Language reference](language.md) | you need the precise rules: types, statements, expressions, modules |
| [Standard library](stdlib.md) | you want to know which capabilities `core` and `moss` provide |
| [Versioning](versioning.md) | you want to know how releases are numbered and when things may break |
| [Openness](openness.md) | you want to know what belongs in the compiler and what belongs in a library |
| [Building Moss](../BUILDING.md) | you want to build the compiler or produce executables |
| [Changelog](../CHANGELOG.md) | you want to know what changed between versions |

Moss is experimental. Compiler releases carry the year; the language standard is
versioned separately (`1.0` is the first), and breaking changes only happen with
a new standard. `26.0.x` used the previous numbering; `26.0.3` was its last
release.

## The two ideas behind Moss

1. **Nothing is built in except `pick`.** Input, output, array helpers, even
   `len` are *capabilities* provided by modules. A name that was not picked
   does not exist in your program.
2. **`@` means "mention".** `@output` mentions a capability; `ask @age`
   mentions the value the verb works on.
