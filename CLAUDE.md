# byteshiftlabs conventions for AI coding agents

Canonical rules for commits, PRs, issues, and docs across all byteshiftlabs
repositories.

Copied from `byteshiftlabs/.github`, `docs/CLAUDE-CONVENTIONS.md`. That repo is
the source of truth; `@`-imports only resolve local paths, so the file cannot be
referenced remotely and is duplicated here instead. Update it there first.

## Commits

- Past tense, describing what the commit did: `Added MBC2 cartridge support`,
  `Fixed timer overflow on DIV reset`. Not `Add`, not `Fix:`, not `WIP`,
  `fix`, `update`, or `misc`.
- One logical change per commit. Prefer several small commits over one that
  mixes unrelated fixes.
- Branch naming: `label/brief-description` — `feature/mbc2-support`,
  `fix/timer-overflow`, `refactor/ppu-cleanup`, `docs/memory-map`.
- **No AI attribution.** No `Co-Authored-By: Claude ...` trailer, no
  "Generated with Claude Code" line, anywhere in a commit or a PR body.
  Authorship reads as the repo owner's.

## Pull requests

Use these three sections, in this order — don't invent a different structure
per PR:

```markdown
## Summary

What changed, in a sentence or two.

## Changes

One line per change, grouped by theme or file.

## Testing

What was actually run.
```

- **State what changed. Nothing else.** No reasoning for the change, no
  narration of how a problem was found, no justification of the approach, no
  explanation of what a tool does or why an alternative was rejected.
  Explaining the why is the developer's job, not the agent's.
- Facts a reviewer needs — a corrected value, a removed file — belong in.
  Sentences beginning "because", "this means", "the cause was", or recounting
  what was checked and in what order, stay out.
- **Leave out numbers that only measure the size of the work.** How many lines
  a file went from and to, how many symbols, directives, call sites or files
  were touched, how many warnings a tool printed: these will probably never be
  of any interest to anyone. Write "generated `api.rst` from the sources",
  not "132 hand-written symbols to 1,085, 1,491 lines to 216".
- A number is worth writing when the number *is* the fact: a value the
  documentation stated wrongly and now states correctly, a version, a size a
  reader has to match. A corrected-figures table is exactly this.
- A before/after table beats a paragraph explaining a discrepancy.
- Organize by theme or file, never as a narration of the work session.
- Keep length proportional to the change. A one-file bugfix doesn't need a
  multi-section report.
- **Verification is a claim.** Say what was actually run, and mark anything
  established by reading the code alone. Never write a check as though it
  were executed when it was inferred.
- Don't pad with impact analysis or downstream reasoning beyond what was
  established. Speculative? One line saying so.
- No hedging about unrelated platforms, toolchains, or use cases the PR
  doesn't touch.

## Formatting text posted to GitHub

Applies to issue, pull request, and discussion bodies and comments.

- **Don't hard-wrap paragraphs.** Write each paragraph and each list item as
  one long line and let the browser reflow it. GitHub renders these fields with
  soft line breaks enabled, so a paragraph wrapped at 76 columns becomes a
  stack of 76-column lines and can never use the full width of the page.
- Tables, code fences, and list markers stay on their own lines as normal.

## Issues

Use these three sections, in this order:

```markdown
## Summary

What's wrong, and exactly where. Link the specific line with a permalink
pinned to a commit SHA, not a branch — branch links rot as lines move.

## Context

The supporting facts, including what was actually checked and how.

## Options

Possible resolutions, laid out rather than prescribed.
```

- Say in Context how it was found — read the code, reproduced, hit in
  production. It tells the reader how much to trust the rest.
- Never write reproduction steps that weren't run. Say what would settle it
  instead.
- Offer options rather than dictating one fix. Say if you'll implement it.
- State the mechanism ("an unchecked index into a buffer sized from input"),
  not an untested story about what it could lead to.
- Describe the class, not one arbitrary member of it.

## Documentation (README / CONTRIBUTING / ROADMAP / SECURITY)

- **Stick to what is implemented.** Describe the code as it is, not as it
  could be or should be. Do not include anything that is not in the
  implementation without the creator's approval or supervision. This covers
  invented sections as much as invented sentences: a "recommended learning
  path", a "what and why" rationale, a suggested reading order, a design a
  subsystem does not have.
- Name the platform and toolchain actually developed and tested on, then
  stop. No "other platforms may work", no invitations to users on unlisted
  ones.
- Check every version number, tool requirement, and coverage claim against
  the repo before writing it or leaving it standing. Stale claims already in
  the file are not exempt.
- Re-verify "not yet covered" and "known gap" statements on every edit that
  touches them. They rot silently.
- Keep shared information (dependency lists, versions) in one file and link
  to it. Duplication is how these drift.

## Third-party material

- Don't reference copyrighted or trademarked material where a neutral
  alternative works. Examples, placeholders, sample data and test fixtures
  get generic names — `game.gb`, not `tetris.gb`.
- Naming a real product is fine when it states a technical fact the reader
  needs ("the header must match the Nintendo logo", "MBC5 was used by
  Pokémon Crystal"). That's descriptive, and removing it would make the
  documentation wrong. It's arbitrary filler that causes trouble.
- Same test for media: no screenshots or recordings of commercial games in
  a README. Use homebrew with a known license, or the project's own UI.

## General

- When a claim in this file or in repo docs turns out to be wrong or
  unverifiable, fix it immediately rather than leaving it for later — treat
  documentation debt the same as code debt.
