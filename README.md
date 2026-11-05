# Angband and Neo Angband repair tracker

This is diegoami's shared engine/C repair fork. It owns the C bug catalogue,
canonical C/game/Neo engine trackers, native evidence, patches and isolated
fix proposals. It does not own the Neo Borg TypeScript port or Mandos product
work. One Angband fork means C and Neo engine work share this repository;
their source trees, build commands and proposal bases remain separate.

## Two responsibilities in this one fork

1. **Original Angband:** the C game and especially its C Borg. Use the native
   C source/worktree, C build and native before/after/control evidence.
2. **Neo Angband:** the TypeScript game engine and engine APIs. Use the Neo
   source tree/toolchain and its own engine regression tests.

The Neo Borg TypeScript mod is a different repository/session:
[neo-angband-borg-mod-tracker](https://github.com/diegoami/neo-angband-borg-mod-tracker).
Mandos product development and downstream adoption are also separate.

Read [AGENTS](AGENTS.md), [RULES](RULES.md), [PLAN](PLAN.md) and the latest
checkpoint to start a session. [Session map](docs/SESSION.md).

- Default master: Neo engine source plus our session documentation.
- c-borg-reference: separately pinned original C source, unchanged as a base.
- workbench/c-borg: documented native C workspace at .worktrees/angband-c.
- Fix branches start from current upstream/master (Neo) or angband/master
  (C), excluding session-documentation commits from upstream proposals.

[Known C bugs/candidates](docs/C-BUGS.md) distinguish reports from independently
verified defects. Metadata is split by stable finding ID; live issue status
belongs on our issue board. [Reuse a C proposal](docs/c-fix-handoff.md).
No angband/angband submissions from us under the standing owner pause.

Neo source/toolchain: Node22+, pnpm version from package.json; pnpm install
--frozen-lockfile, pnpm typecheck/build/test as appropriate. C uses its own
CMake/Make build and native regression scenarios; never run Neo pnpm commands
in the C tree. Documentation/catalogue gates: node tools/check-session.mjs,
node tools/c-bugs.mjs --check, node --test tools/c-bugs-source.node-test.mjs.
See PLAN for exact baseline/consumer and proposal checks.

Original upstream credits, licence and legal policies remain in their files.
[Upstream README](docs/upstream-reference/README.md) retains technical context.

## C fix consumers and borg-rs intake

Track [borg-rs](https://github.com/diegoami/borg-rs) as both a source of C leads
and an independent consumer of C corrections. It does not depend on Neo.
Use `node tools/borg-rs-watch.mjs` to inspect the current file and issue feed;
read-only source checkout: ../borg-rs-reference. A file-to-issues transition
must not drop leads. [Intake contract](docs/BORG-RS-INTAKE.md).

This session proposes/proves the native C correction. Mandos decides whether
it has a TS counterpart, translates it there with scenario controls and
carries it under a linked consumer issue. borg-rs handles its own Rust
translation/carry. The Neo Borg mod tracker restores C fidelity and is not
asked to translate proposed C corrections as a separate repair service.
