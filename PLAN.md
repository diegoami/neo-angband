# Engine/C session plan

1. Read the lane checkpoint (Neo engine #25; native C #207, session:c) and
   open issues here. Review official C and Neo engine boards/source and our linked proposals first. Preserve existing
   linked Mandos consumer receipts; no issue closure by upstream title alone.
2. Triage borg-rs reports posted directly to this tracker. Follow
   docs/BORG-RS-INTAKE.md; deduplicate against canonical issues, source URLs
   and CBUG IDs. Separate reported C leads from verified native faults,
   Rust-only issues and progress notes. Do not proactively fetch/poll the
   borg-rs repository, docs/c-bugs.md or its external issue board. Retained
   snapshots/watch tooling are legacy/manual evidence, not a live intake queue.
   No Rust worker/adoption task or new managed clone is created here. Keep
   Rust provenance in Source/discovery fields, not watched Threads references;
   do not run monitors that would indirectly poll Rust status.
3. Choose the source tree before implementation. Neo tasks use pristine
   upstream/master from neostryder/neo-angband. C tasks use angband/master at
   the recorded C pin, via the same fork's native C worktree. Read that tree's
   bootstrap docs. c-borg-reference stays pinned; never replace it with Neo.
4. Implement one defect in an isolated branch from the correct upstream base,
   not our documentation-bearing master/workbench branch. Compare the branch
   diff before opening a proposal. Keep upstream legal/credit policy intact.
5. Neo validation: build references (pnpm build/typecheck), relevant unit tests
   and required repository CI. C validation: native C build/tests and the
   saved/synthetic scenario under ordinary rules, before/after plus controls.
   Record actual commands, pins and limitations; parser/unit evidence is not
   automatically a full-game consequence. Coordinate heavy gates by flock.
6. C proposals/patches stay here, labelled c-proposal/postponed when parked;
   no official C submissions. Neo engine offers go to the Neo engine repo
   after duplicate/provenance checks. Do not offer a C behavioral departure
   to the Neo Borg port as parity. Route port omissions to the mod fork.
7. Keep patch/index/watch receipts current, then link the delivered correction
   to a distinct Mandos adoption/translation task if applicable. Mandos
   translates C corrections into TS only in its derivative, with its own
   package/plugin/harness/lab scenario proof. Any borg-rs translation/adoption
   is external; do not dispatch or manage Rust work from this plan. Do not
   mutate or deploy the running lab from this session.
8. Checkpoint at merge/stop. Keep proposals, source fixes and linked consumer adoption distinct. Gameplay features remain parked.

Source refresh before a task:

```sh
git fetch upstream master   # Neo engine source reference
git fetch angband master    # original C source reference
```

Use the appropriate remote reference as the isolated fix branch base.
Owned master/workbench documentation commits are not source proposal bases.
