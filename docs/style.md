# Code style beyond the linters

`.clang-format` and `.clang-tidy` enforce most of the style. This file lists only the rules they
cannot check. When a rule here becomes checkable, move it into the lint config and delete it here.

## Names and namespaces

- Refer to engine types through their module namespace: `base::Error`, not `Error` after
  `using base::Error;`. A namespace exists to be read. Namespace aliases that only shorten the
  `gl_engine::` prefix outside the engine (`namespace base = gl_engine::base;`) are fine.

## Scope

- A variable used only by an `if` lives in its init-statement:
  `if (auto current = Query(); current != last) { ... }`.

## Comments and docs

- Never refer to a source line by number (`renderer.cpp:120`). Line numbers go stale with the next
  edit. Name the function, type or constant instead.

## Variants

- `std::visit` over an event or message variant dispatches each alternative to a handler of its
  own, named after it: `OnMouseMove(const MouseMove&)`. No mix of inline logic and named handlers.
