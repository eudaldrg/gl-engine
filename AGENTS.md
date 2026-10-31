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

- **Nothing vendored.** Dependencies come through CPM.cmake or FetchContent at pinned versions.
- **No code from the author's UOC coursework.** Reimplement the ideas; don't port the code.
- **Public history is for any reader, not a channel to Eudald.** Commit messages, PR titles and
  bodies, review replies, issues and docs say what changed and why, nothing else. No notes
  addressed to Eudald, no process asides (a retry, a mistake, a PR that merged early), no tool or
  AI attribution. Anything meant for Eudald goes in the chat.

## Docs

`docs/adr/` holds the ADRs. `docs/modules/` holds one doc per component, `docs/tasks/` one per
procedure (the eudaldrg-workflows knowledge convention).
