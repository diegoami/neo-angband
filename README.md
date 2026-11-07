# Angband and Neo Angband repair tracker

This is diegoami's shared engine/C repair fork. It is a **tracker and
evidence store, not a game release**. Neo Angband builds come from
[neostryder/neo-angband](https://github.com/neostryder/neo-angband/releases).
The Neo source on this repository's `master` is the reference that engine fixes
start from, plus our session documentation.

## What this fork is for

1. **Finding and proving bugs in original Angband, above all its C Borg.**
   Each defect in the C Borg or the C game has one canonical issue here, with
   source lines, evidence level and any local fix proposal. The C source is
   pinned at angband/angband `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`.
2. **Repairing the Neo Angband engine** (TypeScript), with its own source tree
   and regression tests.
3. **Understanding the C Borg well enough to build a better one.** The C
   Borg's specification and the plan for a tunable Borg live here. So do the
   ideas for early-game experiments and quick gains, filed as enhancement
   issues. Other repositories build on this:
   - [better-borg-c](https://github.com/diegoami/better-borg-c): a native C
     Angband/Borg product that carries the fixes verified here;
   - [borg-rs](https://github.com/diegoami/borg-rs): a Rust port of the C Borg;
   - [Mandos](https://github.com/diegoami/mandos): a TypeScript derivative that
     translates applicable C fixes for itself.

What it does not own: the Neo Borg TypeScript port
([neo-angband-borg-mod-tracker](https://github.com/diegoami/neo-angband-borg-mod-tracker)),
Mandos product work, and any Rust work. No angband/angband issues, PRs or
comments are submitted from here under the standing owner pause.

## Find your way

| I want to... | Go to |
| --- | --- |
| Understand how the C Borg works | [The C Borg: purpose and architecture](docs/c-borg/BORG_SPEC.md) and the [C Borg documents index](docs/c-borg/README.md) |
| See the C Borg's known bugs | [C findings and candidates](docs/C-BUGS.md), or the [open findings on the board](https://github.com/diegoami/neo-angband-tracker/issues?q=is%3Aissue+label%3Aupstream-c+label%3Afinding+is%3Aopen) |
| See the plan for a tunable, better Borg | [Tunable Borg Plan](docs/c-borg/TUNABLE-BORG-PLAN.md) and its [review prompt](docs/c-borg/TUNABLE-BORG-PLAN-REVIEW-PROMPT.md) |
| See quick gains and experiment ideas | [Open enhancement issues](https://github.com/diegoami/neo-angband-tracker/issues?q=is%3Aissue+is%3Aopen+label%3Aenhancement) |
| Check how the specification and the findings were verified | [Source-level check programs](docs/audit/c-reports/spec-checks/README.md), the [re-check record](docs/audit/c-reports/spec-checks/RECHECK.md) and the [findings' provenance](docs/audit/c-reports/spec-checks/PROVENANCE.md) |
| Reuse one of our C fixes | [Reusing a local C fix](docs/c-fix-handoff.md) and the [proposal patches](patches) |
| Report a C lead from borg-rs | [borg-rs intake contract](docs/BORG-RS-INTAKE.md) |
| Start or resume a session | [AGENTS](AGENTS.md), [RULES](RULES.md), [PLAN](PLAN.md), the [session map](docs/SESSION.md) and the checkpoints below |
| Read the Neo engine's own documentation | [Neo Angband documentation](docs/README.md) and the [upstream README](docs/upstream-reference/README.md) |

## Two responsibilities in this one fork

One Angband fork means C and Neo engine work share this repository; their
source trees, build commands and proposal bases remain separate.

1. **Original Angband:** the C game and especially its C Borg. Use the native
   C source/worktree, C build and native before/after/control evidence.
2. **Neo Angband:** the TypeScript game engine and engine APIs. Use the Neo
   source tree/toolchain and its own engine regression tests.

Read [AGENTS](AGENTS.md), [RULES](RULES.md), [PLAN](PLAN.md) and the latest
[Neo engine checkpoint #25](https://github.com/diegoami/neo-angband-tracker/issues/25)
to start a Neo session. The independently owned native C lane uses
[checkpoint #207](https://github.com/diegoami/neo-angband-tracker/issues/207)
(`checkpoint` + `session:c`). [Session map](docs/SESSION.md).

- Default master: Neo engine source plus our session documentation.
- c-borg-reference: separately pinned original C source, unchanged as a base.
- workbench/c-borg: documented native C workspace at .worktrees/angband-c.
- Fix branches start from current upstream/master (Neo) or angband/master
  (C), excluding session-documentation commits from upstream proposals.

[Known C bugs/candidates](docs/C-BUGS.md) distinguish reports from independently
verified defects. Metadata is split by stable finding ID; live issue status
belongs on our issue board. [Reuse a C proposal](docs/c-fix-handoff.md).

Neo source/toolchain: Node26 (the .nvmrc line), pnpm12.3.4 from package.json; pnpm install
--frozen-lockfile, pnpm typecheck/build/test as appropriate. C uses its own
CMake/Make build and native regression scenarios; never run Neo pnpm commands
in the C tree. Documentation/catalogue gates: node tools/check-session.mjs,
node tools/c-bugs.mjs --check, node --test tools/c-bugs-source.node-test.mjs.
See PLAN for exact baseline/consumer and proposal checks.

Original upstream credits, licence and legal policies remain in their files.
[Upstream README](docs/upstream-reference/README.md) retains technical context.

## Current Neo source and build profiles

[Engine integration372](https://github.com/diegoami/neo-angband-tracker/issues/372)
imports exact upstream06a2321ce30a414df20d1b20d486446e7688171c, the source of
GitHub desktop `v1.23.0-edge.218`. Registry Core/SDK/content still publish
`next=1.23.0-beta.3` (`latest=1.22.0`); edge.218 is not an npm package release.
Source package manifests also retain beta.3 versions, so exact source and
artifact hashes, not their version strings, identify the new compatibility
profile. [Pinned integration and consumer recipe](docs/runs/2026-11-07-engine-edge218/README.md).

The accepted input-flush, truthful seed/birth and load-resume corrections are
now upstream. Optional replay is not made a runtime prerequisite. Public
Worker controllers and the documented in-process developer opt-in have
separate capability contracts: Core/harness success does not by itself prove
stock desktop installation or first-party profile/management access. Consumers
verify their own named package/plugin/harness profiles; the retained lab is
not deployed by an engine source merge.

## C fix consumers and borg-rs intake

[borg-rs](https://github.com/diegoami/borg-rs) posts C defect leads directly to
this repository's issue board. Triage and deduplicate those reports here;
verify C-versus-Rust attribution and evidence before a native defect claim.
Do not proactively fetch/poll its repository, docs/c-bugs.md or external
issue board. Old snapshots, ../borg-rs-reference and tools/borg-rs-watch.mjs
remain legacy/manual evidence only, with no automatic startup/end sweep.
No Rust worker, adoption task or new managed clone belongs to this workspace.
[Intake contract](docs/BORG-RS-INTAKE.md).

This session proposes/proves the native C correction. Mandos decides whether
it has a TS counterpart, translates it there with scenario controls and
carries it under a linked consumer issue. Any borg-rs Rust translation/carry
is independently managed outside this workspace. The Neo Borg mod tracker
restores C fidelity and is not asked to translate proposed C corrections as a separate repair service.

## Fresh clone setup

```sh
git remote add upstream https://github.com/neostryder/neo-angband.git
git remote add angband https://github.com/angband/angband.git
git fetch upstream master
git fetch angband master
git fetch origin workbench/c-borg
git worktree add -b workbench/c-borg .worktrees/angband-c origin/workbench/c-borg
```

Skip remotes/branches already present. Start the session from this master
workspace; the C worktree shares this repository's issue board/rules. Source
fix task branches still use pristine upstream/master or angband/master.

## Neo desktop build/install reference

For a locally tested or upstream unsigned macOS build, use System Settings →
**Privacy & Security** → **Open Anyway** after the initial launch refusal.
Apple Silicon machines need the **arm64** build; Intel machines use x64.
See [the installation guide](docs/INSTALL.md) and the retained upstream README
for the full supported procedure. This tracker does not publish a new desktop
release merely by documenting/testing an upstream build.
