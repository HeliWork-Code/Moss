# Versioning

A Moss release has four numeric fields and two optional decorations:

```
<year>.<train>.<minor>[.<hotfix>][-<stage>.<n>]
```

| Field | Meaning | Rhythm |
|---|---|---|
| `year` | the release year | two digits (`26`); four from 2100 on |
| `train` | major release train | about two months |
| `minor` | minor release | one to two weeks |
| `hotfix` | repairs on a released minor | the same day when urgent |
| `stage` | pre-release qualifier | `alpha`, `beta`, `rc` |
| `n` | pre-release number | `1`, `2`, ... |

The compiler version says **when** a build was made. It says nothing about
which language rules it implements; that is the standard, below.

## Examples

| Version | Meaning |
|---|---|
| `26.1.0` | release year 2026, train 1, first minor |
| `26.1.2` | 2026, train 1, third minor |
| `26.1.2.1` | first hotfix for `26.1.2` |
| `26.2.0-beta.3` | third beta of the next train |
| `2100.1.0` | the first release year that needs four digits |

Sorting is plain numeric ordering per field, with `alpha < beta < rc < final`,
so no tool needs a custom comparison rule.

## The language standard

The standard is versioned on its own, as `A.B.C`:

- **A** — breaking epoch. Syntax or semantics that old code relies on changes.
- **B** — additions. New syntax, capabilities or guarantees; old code still
  compiles.
- **C** — clarifications and fixes with no intended semantic change.

A source file declares the standard it is written for:

```moss
moss 1.0;
```

`moss 1000;` is accepted as an alias for standard `1.0`; it was the placeholder
before standards existed. The declaration is optional while only one standard
exists.

The first standard is **1.0**, shipped by compiler `26.1.0`.

## Which standards a compiler supports

One compiler package ships the current standard, the one before it, and every
LTS standard:

| Standard | Status |
|---|---|
| `1.2` | current |
| `1.1` | previous |
| `1.0` | LTS, frozen |

Standards are feature switches in the front end, not copies of it. A standard
that only adds syntax costs almost nothing to keep; a standard that changes the
meaning of existing syntax costs a branch, which is why breaking epochs are rare
by design.

## LTS: a frozen standard, not a maintenance branch

An LTS is a standard that has been declared frozen. It is implemented by the
same mainline compiler, so there is no maintenance branch and nothing to
backport: a user installs the newest compiler and keeps writing `moss 1.0;`,
and still receives every fix.

- Freezing happens once, when a standard has proven itself; it is not a
  calendar event and not every standard becomes one.
- An LTS standard never gains features or syntax.
- Fixes arrive through normal compiler releases. A fix that lives in shared
  code is made once and benefits every supported standard.
- An LTS standard retires on its own schedule — by default three years after it
  was frozen — and may outlive standards published after it: `1.0` LTS can
  still be supported while `1.1` and `1.2` are already retired.
- Retirement is announced at least one release in advance.

The cost of an LTS is a longer test matrix, not a second maintenance line: the
frozen standard's test suite keeps running against every compiler release.

A standard that is neither current, previous nor LTS is retired; it can still be
built with the compiler release of its era, and `moss migrate` moves code
forward.

Within a standard, `.mbc` bytecode stays readable by later compilers that
support that standard, and the capabilities of `core` and the standard library
keep their signatures.

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
