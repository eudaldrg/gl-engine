---
aliases: [error, errors, result, logging, log, file watcher, hot reload]
sources: [src/base/**, tests/base/**]
decisions: [docs/adr/0002-errors-as-values-no-throw.md]
---

# base

Foundations every other library uses, with no GL: `Error` and `Result<T>`, the logger, and
`FileWatcher`.

## Error and Result

`Result<T>` is `std::expected<T, Error>`. One `Error` type serves the whole engine: a code
(`kIo`, `kParse`, `kGl`, `kUnsupported`), a human-readable message, and the `std::source_location`
where it was built. The location is a default member initialiser, so `Error{ErrorCode::kIo, "..."}`
records the caller's line without a macro. `ToString(error)` gives `io: <message> (file.cpp:42)`.

A module that needs richer errors defines its own type that converts to `Error`; it does not derive
from it, because `std::expected` stores the error by value and a derived error would be sliced.

Broken invariants are not `Error`s: they abort (ADR 0002).

## Logging

A process-wide level filter (default `kInfo`) and one sink (default stderr). `SetSink` replaces the
sink; an empty function restores stderr. Messages below the level are dropped before formatting, but
the arguments are still evaluated at the call site. Tests replace the sink to capture messages and
must restore it in `TearDown`, since the logger is shared by every test in the binary.

## FileWatcher

Callers register paths and get a callback when a file changes, appears after being missing, or
disappears. Callbacks run only from `Poll()`, on the caller's thread; the host calls it once per frame.
Detection compares modification times at most once per interval (250 ms by default, so an edit shows
within that), which keeps per-frame cost to one clock read. The interval is a constructor parameter so
tests can poll every time.

Polling is an implementation detail: OS notifications can replace it without changing callers, and a
later script binding can register on the same watcher. Unwatching from inside a callback is safe.

Coarse file-system timestamps can make two quick writes look identical; the tests move modification
times forward explicitly rather than sleeping.
