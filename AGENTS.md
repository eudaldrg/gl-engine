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
- **Public history is for any reader.** Commit messages, PR titles and
  bodies, review replies, issues and docs say what changed and why, nothing else. No notes
  addressed to the maintainer, no process asides (a retry, a mistake, a PR that merged early), no tool or
  AI attribution.

## Docs

`docs/adr/` holds the ADRs. `docs/modules/` holds one doc per component, `docs/tasks/` one per
procedure.
