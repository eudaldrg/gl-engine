# 0002. Expected failures return `std::expected`; our code never throws

- Status: proposed
- Date: 2026-10-31
- Settled at: M1

## Context

`std::expected` (ADR 0001) gives a way to return errors as values, but it does not settle the rest
of the policy: whether code may also throw, and what happens when an invariant breaks. A codebase that
mixes the two ends up with every call site guarding against both, and the choice spreads into every
fallible signature, so it is expensive to change later.

Two facts constrain it. Exceptions cannot cross C callbacks (GLFW input, the GL debug callback)
without undefined behaviour. And the standard library and some dependencies throw, so exceptions
cannot simply be compiled out without changing what allocation failure does.

## Options

### A. `std::expected` only, exceptions disabled (`-fno-exceptions`)

Every failure is in a signature. But allocation failure and throwing standard-library calls then
terminate the program, and dependencies built with exceptions on need care.

### B. Exceptions for all errors

Short signatures and automatic propagation. Nothing in a signature says what can fail, and
callbacks need a catch at every C boundary.

### C. Split by kind of failure

- A failure the program expects and can handle (missing file, shader compile error, malformed
  PLY) returns `std::expected<T, Error>`.
- A programmer error (a broken invariant, a GL error reported by the debug callback in debug
  builds) stops the program with a message, through an assert that stays on in every build type.
- Code in this repository never throws. Exceptions stay enabled so the standard library and
  dependencies behave normally; `std::bad_alloc` is left to terminate.

## Decision

C, because it keeps every handleable failure visible in a signature, keeps C callbacks safe, and does
not fight the standard library over exceptions.

## Consequences

- A function that can fail in a way the caller should handle returns `std::expected`; a function that
  cannot fail returns its value.
- Exceptions thrown by a dependency are caught at the call into that dependency and turned into an
  `Error`.
- Does not decide what `Error` contains (category, message, source location); that is in the design
  doc and can change freely.
- Does not decide logging.
