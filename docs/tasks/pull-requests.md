---
aliases: [pr, pull request, branches, release, tag, merge]
---

# Pull requests

This repo's branch model, merge methods and release steps. How to write and update a PR in general
is in the workflows plugin's `pull-request` skill.

## Branch model

- Work happens on `feature/<slug>` branches cut from the remote `dev`
  (`git fetch origin dev && git checkout -b feature/<slug> origin/dev`).
- Every change reaches `dev` through a PR, one topic per PR. The owner reviews it and merges it by
  the method its description recommends (`/core:pull-request`): rebase for one commit or clean
  steps, squash for review fixups or plan commits. A squash takes the PR title and description as
  its message (a repo setting), so a squash PR's description is written as that commit.
- A release is a PR from `dev` into `main`, merged as a **merge
  commit** (not squashed, so `main` keeps the integration history), then tagged
  `vX.Y.Z` on `main`. Only when the owner says so.
- `main` and `dev` are protected on the forge: PR required, the CI
  checks required, no force push, no deletion. No approval count is required, because the owner
  opens PRs from their own account and cannot approve them; "the owner reviews" is a rule here, not a
  forge setting. Linear history is not required on `main`: it would reject the release
  merge commit.

## Before pushing

Run the push-time checks if the pre-push hook is not installed (`ls .git/hooks/pre-push`):
`pre-commit run --all-files --hook-stage pre-push`.
