# Agent entry: C Borg, Angband game and Neo Angband engine fixes

This session owns **diegoami/neo-angband-tracker**. Start here, then read [RULES.md](RULES.md),
[PLAN.md](PLAN.md), [session.json](session.json), [the session map](docs/SESSION.md)
and the latest comments on [Neo engine checkpoint #25](https://github.com/diegoami/neo-angband-tracker/issues/25).
Native C uses its separately owned [checkpoint #207](https://github.com/diegoami/neo-angband-tracker/issues/207)
(`checkpoint` + `session:c`); do not select a lane by the generic checkpoint label alone:

```sh
git status --short
git remote -v
gh issue view 25 -R diegoami/neo-angband-tracker --comments
node tools/check-session.mjs
# Run upstream-watch.mjs only after verifying selected Threads exclude Rust refs.
node tools/c-upstream-watch.mjs
```

borg-rs reports directly to this repository. Triage those posted reports under
[the intake contract](docs/BORG-RS-INTAKE.md); do not proactively fetch/poll
borg-rs source, docs/c-bugs.md or its external issue board. Its old watch tool
and snapshots are legacy/manual evidence, not startup work. Keep Rust
provenance in Source/discovery fields rather than watched Threads references;
no routine Rust polling includes indirect monitor follows. No Rust worker,
adoption task or new reference clone is managed here.

Use this repository's issue board, not Mandos's former upstream queue.
Preserve transferred history and link component/integration work across repos.
The default branch contains our session docs; upstream source references and
fix branches remain separate. Follow the source-base rules in PLAN before
editing. Implementation uses isolated task worktrees; do not disturb the old
running lab or pristine reference checkouts. Owner scope and posting limits
in RULES take precedence over historical upstream roadmap/reference prose.

[Upstream technical README](docs/upstream-reference/README.md) is reference
material, not this fork's task queue or autonomous feature authorization.
