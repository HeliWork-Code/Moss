# Versioning

A Moss release has four numeric fields and two optional decorations:

```
<year>.<train>.<minor>[.<hotfix>][-<stage>.<n>]
```

| Field | Meaning | Rhythm |
|---|---|---|
| `year` | the language standard | two digits (`26`); four from 2100 on |
| `train` | major release train | about two months |
| `minor` | minor release | one to two weeks |
| `hotfix` | repairs on a released minor | the same day when urgent |
| `stage` | pre-release qualifier | `alpha`, `beta`, `rc` |
| `n` | pre-release number | `1`, `2`, ... |

## Examples

| Version | Meaning |
|---|---|
| `26.1.0` | 2026 standard, train 1, first minor |
| `26.1.2` | 2026, train 1, third minor |
| `26.1.2.1` | first hotfix for `26.1.2` |
| `26.2.0-beta.3` | third beta of the next train |
| `2100.1.0` | the first standard that needs four digits |

Sorting is plain numeric ordering per field, with `alpha < beta < rc < final`,
so no tool needs a custom comparison rule.

## The year is the language standard

- Breaking syntax changes are only allowed when the year changes.
- A source file declares the standard it is written for: `moss 2026;`.
  `moss 1000;` is accepted as an alias for the 2026 standard, because it was
  the placeholder before standards existed.
- A compiler supports the standards it was built with, and keeps supporting
  older ones.
- `.mbc` bytecode written under a standard stays readable by later compilers
  that support that standard.

Inside a year, everything is additive: new capabilities, new library types,
new optimisations. A change that would break existing code waits for the next
standard, and ships with `moss migrate` support.

## Feature batches are labels

"Feature 1" is a release title, not a version component:

```
title      Moss 26.1.2 — Feature batch 1
tag        26.1.2
metadata   26.1.2+feature.1
```

Build metadata after `+` does not take part in precedence, so all three
identify the same release.

## Cadence and gates

| Release | Period | Target |
|---|---|---|
| minor | 1–2 weeks | 10–20 changes |
| train | ~2 months | 70–100 features and ~30 changes, or one big overhaul |

**The period is the commitment; the count is the target.** A release ships with
what is green when the period ends. It is never padded to reach a number, and
never delayed for one non-critical item.

A release goes out only when all three test suites pass on Linux and Windows,
both backends agree byte for byte, and no Emergency bug is open.

## Bugs

| Class | Examples | When it is fixed |
|---|---|---|
| **Emergency** | security hole, wrong code generation, compiler crash, syntax hole | a hotfix the same day, backported to the previous train if it is still supported |
| **Minor** | wrong diagnostic, edge-case runtime error, a documented feature that misbehaves | the next minor, or a hotfix if it hits many users |
| **Normal** | cosmetic, hard to reproduce, harmless | the next minor or train, whichever comes first |

A hotfix never adds a feature and never changes syntax; if it would, it is not
a hotfix.
