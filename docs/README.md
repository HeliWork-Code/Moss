# Moss documentation

| Document | Read it when |
|---|---|
| [Tutorial](tutorial.md) | you are new to Moss and want to write a program |
| [Language reference](language.md) | you need the precise rules: types, statements, expressions, modules |
| [Standard library](stdlib.md) | you want to know which capabilities `core` and `moss` provide |
| [Building Moss](../BUILDING.md) | you want to build the compiler or produce executables |
| [Changelog](../CHANGELOG.md) | you want to know what changed between versions |

Moss is experimental: `26.0.x` releases are pre-releases on the way to
`26.1.0`. Features may change between pre-releases.

## The two ideas behind Moss

1. **Nothing is built in except `pick`.** Input, output, array helpers, even
   `len` are *capabilities* provided by modules. A name that was not picked
   does not exist in your program.
2. **`@` means "mention".** `@output` mentions a capability; `ask @age`
   mentions the value the verb works on.
