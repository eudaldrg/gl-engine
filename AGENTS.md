# gl-engine

Agent instructions for this repo. `CLAUDE.md` is a symlink to this file; edit `AGENTS.md`.

## What this is

A public, MIT-licensed real-time 3D engine in C++23 and OpenGL 4.5 core.
`docs/roadmap.md` holds the goals, milestones and non-goals.

## How work is split

- The maintainer owns the design: design docs, ADRs (`docs/adr/`), module boundaries, plan and tasks.
- Each change is one topic on its own branch, proposed as a small PR with tests and a description.
- The maintainer reviews and merges every PR. Nobody merges their own PR.
- Push back on designs that look wrong. Keep PRs small enough to review properly.

## Hard rules

- **Nothing vendored.** Dependencies come through CMake FetchContent at pinned versions.
- **Original code only.** Reimplement ideas from elsewhere; do not copy code into the repo.
- **Public history is for any reader.** Commit messages, PR titles and bodies, review replies,
  issues and docs describe the change and why, not the tooling or the process that produced it: no
  notes addressed to the maintainer, no process asides (a retry, a mistake, a PR that merged early).

## Docs

`docs/adr/` holds the ADRs and `docs/design/` one design doc per milestone. `docs/modules/` (one doc
per component) and `docs/tasks/` (one per procedure) extend this file: they are context for coding
agents, and their front matter (`aliases`, `sources`, `decisions`) is what the agents' knowledge
retrieval matches on.

The `/core:*` commands below come from the maintainer's agent workflows plugin,
[eudaldrg-workflows](https://github.com/eudaldrg/eudaldrg-workflows), which also generates
`.clang-format`, `.clang-tidy` and `.pre-commit-config.yaml` from `.claude/workflows.json`.

## Lint / format / quality gates

**Rule for agents:** code must pass every automated check before it is committed or pushed. Never
bypass a hook (`--no-verify`) and never weaken a check, a lint rule or a test to make something
pass. Adding a check needs no permission; removing or loosening one needs the owner. Silence a
warning only at the line that triggers it (`// NOLINT(<check>)`), with a comment saying why, never
file-wide or project-wide. When a style preference comes up, tighten `.clang-format` / `.clang-tidy`
(through `.claude/workflows.json` and `/core:sync-lint-rules`) rather than writing prose style docs.
Only rules no linter can check go in `docs/style.md`; read it before writing code.

`docs/tasks/testing.md` lists what runs at commit, at push and in CI. Once per clone:

```bash
cmake --preset debug
pre-commit install && pre-commit install --hook-type pre-push
```

`.pre-commit-config.yaml` is generated from `.claude/workflows.json` by `/core:setup-cpp-repo`; edit
the config and regenerate, never the file. The CI workflow (`.github/workflows/ci.yml`) is this
repo's own file and is edited directly.

## Branches and pull requests

`feature/<slug>` → PR into `dev`, merged by the owner after review by the method the PR recommends
(`/core:pull-request`). A release is `dev` →
`main` as a merge commit, then a `vX.Y.Z` tag on `main`, only when the owner says so. Never merge
your own PR.
`docs/tasks/pull-requests.md` has the procedure and its gotchas.
