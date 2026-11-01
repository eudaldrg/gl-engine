# gl-engine

Agent instructions for this repo. `CLAUDE.md` is a symlink to this file; edit `AGENTS.md`.

## What this is

A public, MIT-licensed real-time 3D engine in C++23 and OpenGL 4.5 core, and a portfolio piece.
`docs/context.md` holds the goals, suggested stack, stages and non-goals.

## How work is split

- Eudald owns the design: design docs, ADRs (`docs/adr/`), module boundaries, plan and tickets.
- Claude implements one ticket per branch, as a small focused PR with tests and a clear description.
- Eudald reviews every PR. Claude never merges its own work.
- Push back on designs that look wrong. Keep PRs small enough to review properly.

The repo history is part of the showcase: design, then tickets, then reviewed PRs.

## Hard rules

- **Nothing vendored.** Dependencies come through CMake FetchContent at pinned versions.
- **No code from the author's UOC coursework.** Reimplement the ideas; don't port the code.
- **Public history is for any reader, not a channel to Eudald.** Commit messages, PR titles and
  bodies, review replies, issues and docs say what changed and why, nothing else. No notes
  addressed to Eudald, no process asides (a retry, a mistake, a PR that merged early), no tool or
  AI attribution. Anything meant for Eudald goes in the chat.

## Docs

`docs/adr/` holds the ADRs. `docs/modules/` holds one doc per component, `docs/tasks/` one per
procedure (the eudaldrg-workflows knowledge convention).

## Lint / format / quality gates

**Rule for agents:** code must pass every automated check before it is committed or pushed. Never
bypass a hook (`--no-verify`) and never weaken a check, a lint rule or a test to make something
pass. Adding a check needs no permission; removing or loosening one needs the owner. Silence a
warning only at the line that triggers it (`// NOLINT(<check>)`), with a comment saying why, never
file-wide or project-wide. When a style preference comes up, tighten `.clang-format` / `.clang-tidy`
(through `.claude/workflows.json` and `/core:sync-lint-rules`) rather than writing prose style docs.

`docs/tasks/testing.md` lists what runs at commit, at push and in CI. Once per clone:

```bash
cmake --preset debug
pre-commit install && pre-commit install --hook-type pre-push
```

`.pre-commit-config.yaml` is generated from `.claude/workflows.json` by `/core:setup-cpp-repo`; edit
the config and regenerate, never the file. The CI workflow (`.github/workflows/ci.yml`) is this
repo's own file and is edited directly.

## Branches and pull requests

`feature/<slug>` → PR into `dev`, squash-merged by the owner after review. A release is `dev` →
`main` as a merge commit, then a `vX.Y.Z` tag on `main`, only when the owner says so. Never merge
your own PR.
`docs/tasks/pull-requests.md` has the procedure and its gotchas.
