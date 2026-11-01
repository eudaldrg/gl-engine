# 0001. C++23, with GCC 14 / Clang 19 / MSVC 17.8 as the floor

- Status: proposed
- Date: 2026-10-31
- Settled at: M1

## Context

The language standard decides which idioms can spread through the codebase. Error handling is the
one that matters most: the plan is to return `std::expected<T, Error>` instead of bare bool or
error-code returns that lose the reason for a failure, and that type only exists from C++23. Once
dozens of functions return it, changing the standard means rewriting their signatures.

The standard also fixes which compilers can build the project. The project builds on Linux and
Windows, in CI on GitHub-hosted runners (Ubuntu 24.04, Windows Server 2022/2025) and locally on
Ubuntu 26.04 (GCC 15, Clang 21) and Windows. macOS is a non-goal, so Apple Clang is not a constraint.

## Options

### A. C++20

Supported by every compiler on every runner. Error handling would need either a third-party
`expected` (tl::expected: one more dependency, swapped out later) or exceptions. Moving up to C++23
later is cheap; the cost is what gets written in the meantime.

### B. C++23, floor GCC 14 / Clang 19 / MSVC 17.8

Gives `std::expected`, `std::print`, `std::to_underlying`, deducing `this`, and
`std::ranges::to`. All three compilers at this floor ship them; GCC 14 and Clang 19 install from the
Ubuntu 24.04 archive on the runner, and current Visual Studio 2022 is past 17.8. Anyone cloning needs
a compiler from 2024 or later, which is a mild ask for a project that is new in 2026.

### C. C++26

Brings contracts, static reflection and `std::execution`. Compiler support is the blocker: at the
time of writing, contracts are implemented only in GCC 16 (behind `-fcontracts`); Clang has them
only in an experimental fork and MSVC not at all. Choosing C++26 now would mean either a GCC-only
build or a feature list that changes per compiler. Revisit when the three compilers ship the
features the project would use.

## Decision

B, because `std::expected` is the error-handling idiom the codebase is built on, the floor
is met by every machine and runner the project uses with no extra dependency, and C++26 is not yet
implemented across the three compilers.

The floor is the oldest release of each compiler that implements every standard feature the code
uses; raising it is free. Today that is GCC 14 / Clang 19 / MSVC 17.8.

Clang 18 is not enough on Linux, where Clang uses GCC's standard library: libstdc++ declares
`std::expected` only when `__cpp_concepts >= 202002L`, and Clang 18 still reports `201907L`. Clang 19
is the first release that reports the newer value.

## Consequences

- Error-returning functions use `std::expected` directly; no polyfill dependency.
- CI must pin compilers at or above the floor, not take the runner default (Ubuntu 24.04 defaults to
  GCC 13).
- Raising the floor later is cheap. Lowering it below C++23 is the expensive direction.
- Does not decide how errors are modelled (error type, when to throw, when to abort); that is its
  own decision.
- Does not decide whether to move to C++26 later; revisit when all three compilers support the
  features the project would use.
