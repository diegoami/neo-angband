# B94: cancelling a Borg map dump crashes the C game

Local proposal; **no upstream C submission**.
[Our draft C fix PR](https://github.com/diegoami/neo-angband/pull/1) targets
our own c-borg-reference branch; the owner has paused C submissions.
Tracker: [#94](https://github.com/diegoami/mandos/issues/94).
Lead credited to [borg-rs, catalogue group 13](https://github.com/diegoami/borg-rs/blob/66dced95ba5c61b95fb011b01c3d214b7ab2c4a6/docs/c-bugs.md).

## Human consequence

A user asks the C Borg for a diagnostic map, then presses Escape at the
filename prompt. The running game crashes instead of returning to play.
A failed file open reaches the same unchecked-file path by source inspection,
but the runtime scenario below specifically tests cancellation.
This can risk progress since the last save; actual lost progress was **not**
measured. It is a user-invoked diagnostic path, not an ordinary combat death.

## Source and scenario

- Current official Angband C reference:
  `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`.
- Native C-engine/C-Borg harness from borg-rs
  `613227bc53c4f0e77ec457d5721febe9cf8bfe03`.
  The unmodified `src/borg/borg-log.c` is byte-identical to current C:
  SHA-256 `72cb87cb08062ba26bd90e061917a441e6d47cfe824d6b3321bda621a2020b42`.
- CMake Release, `SUPPORT_BORG_HARNESS=ON`, `BORG_IMPL=c`.
  The native harness supplies a headless terminal, deterministic seeding and
  run limits. It does not link the Rust Borg.
- Normal birth: seed **1000**, Human (race 0), Warrior (class 0);
  fixed harness `borg.txt`, swaps enabled and `borg_save_death = 1`.
  No cheat-death flag. At the diagnostic call: **turn 1, 20 HP, depth 0**.
  No synthetic player, monster, inventory or grid mutation.
- A diagnostic callback, after the first C Borg key, temporarily suspends
  `inkey_hack` and supplies Escape to the **real** `get_string` prompt
  invoked by `borg_write_map(true)`. It then restores the hook if returned.
  A second mode sends Enter to accept the normal writable filename.
  This calls the dump function directly in the running C game; it does not
  exercise the top-level Ctrl-Z command dispatch. The normal dump command
  also refreshes inventory first; the callback does not reproduce that
  extra refresh. The crash depends on the cancelled file, not item content.
- No RNG draws or game-state edits are added by the callback. See
  `observation.patch` for the complete intervention.

This is **running C-game** evidence, not an extracted-function fixture,
and not a claim of an uninstrumented desktop reproduction.

## Before/after and controls

| Native run | Observed result |
| --- | --- |
| Unfixed, Escape | Signal 11 / shell status 139 at turn 1; no “returned” marker or completed summary. |
| Unfixed, Enter | Returns; writes a 23,865-byte map; plays to game turn 609, 83 logged Borg keys, alive. |
| Corrected, Escape | Returns; no map created; plays to turn 609, 83 logged Borg keys, alive. |
| Corrected, Enter | Returns; writes a 23,865-byte map; same endpoint and 83 keys. |

The unfixed/corrected Enter key logs are byte-identical. Corrected Escape
and Enter key logs are also byte-identical. The cap is a **game-turn cap
600**, not 600 controller decisions; the harness stops at the next key,
turn 609. Wall timings are not equality evidence.

The correction returns immediately if no file was opened. No combat,
inventory, scoring or RNG logic changes. Proposed patch:
[`patches/upstream-c/B94-map-dump-cancel.patch`](../../../../patches/upstream-c/B94-map-dump-cancel.patch).
Storing this patch does not apply it to the TypeScript Borg/engine and does
not make the issue `carried`.

## Reproduction

From the pinned borg-rs checkout, apply `observation.patch`, then:

```sh
BORG_IMPL=c bash harness/build.sh
MANDOS_B94_PROMPT=cancel bash harness/run.sh /tmp/b94-before-cancel 1000 0 0 600
MANDOS_B94_PROMPT=accept bash harness/run.sh /tmp/b94-before-accept 1000 0 0 600
# Apply the proposed src/borg/borg-log.c guard, then rebuild.
BORG_IMPL=c bash harness/build.sh
MANDOS_B94_PROMPT=cancel bash harness/run.sh /tmp/b94-after-cancel 1000 0 0 600
MANDOS_B94_PROMPT=accept bash harness/run.sh /tmp/b94-after-accept 1000 0 0 600
cmp /tmp/b94-before-accept/keys.log /tmp/b94-after-accept/keys.log
cmp /tmp/b94-after-cancel/keys.log /tmp/b94-after-accept/keys.log
```

Output directories must be disposable: the upstream harness recreates its
named output. CMake was supplied by the already installed local toolchain.
No user profile or saved character was used.

## Duplicate review and proposed delivery

Eight searches, each including open and closed records: Borg map, cancel,
dump and file, separately for issues and PRs. All returned pages fit within
100 results. Result metadata retained in `duplicate-search.json`.
No exact matching cancellation/NULL-file report was found in those results.
That is a dated search result, not a guarantee of absence from every comment,
forum or unindexed report.

Relevant hits reviewed: PR #5934 fixes a **different** map-dump crash,
a trait-array heap overflow; #6261 formats map text; #6048 fixes dumped
quantities. Issue #6362 is a base-game charge-combination divide by zero,
not this file path. PR #6504 moved debug commands; verify its patch before
external submission if it changes how users reach the prompt.

The local proposal is **diegoami/neo-angband#1**, targeting our own
**c-borg-reference** branch pinned at the C revision above. Its one-file
four-line guard is the canonical patch linked here. It remains a draft,
unapplied to a maintained C consumer. No angband/angband submission is made.

Proposed concise PR text:

> Cancelling the Borg map-file prompt leaves the file pointer NULL, but the
> dump continues and crashes. Return when no file was opened, covering both
> cancellation and an unsuccessful open.
>
> Reproduced with the C game and C Borg: Escape crashes before the guard and
> returns safely after it; accepting a writable filename still produces the
> map and the same subsequent key trace. Found while checking borg-rs's C bug
> catalogue.

The quoted draft is our proposed text. Retain this detailed internal receipt
rather than putting the whole investigation into our local proposal PR.
