# Existing input-flush correction in the owned engine fork

Canonical source: [diegoami/neo-angband-tracker#5](https://github.com/diegoami/neo-angband-tracker/issues/5).
Existing upstream proposal: [neostryder/neo-angband#432](https://github.com/neostryder/neo-angband/pull/432),
`e8729a285474325ca25a03534db14f8c10808794`, still OPEN/unmerged when reconciled.
Upstream source issue [neostryder/neo-angband#414](https://github.com/neostryder/neo-angband/issues/414)
is also OPEN. No duplicate upstream proposal, nudge or broad source scan occurred.

## Source and correction

Verified owned master `f9a03f0564dbbae9b2b5e60404a80436ad1fc70d` lacked the
correction: `disturb()` cancelled running/rest/queued continuations and returned
without signalling the already declared/grantable event. A subscribed mod
therefore heard no disturbance through `ctx.events`.

The exact existing proposal was imported as
`b02adb897e2bc11caf393cf0c71a6c39f5dcaa69` with its original author/date, source/test/doc diff intact. Its commit
message qualifies the original upstream reference rather than accidentally
addressing an unrelated fork issue. FORK provenance beside the signal is
`94f2d1f61c4f65b2d9f09b440bb194b5986b4b75`. The preserved
[exact proposal patch](../../../patches/upstream-neo/432-disturb-input-flush.patch)
uses zero context (as the existing noteSpot patch does) and has SHA256 `076b0f261f6a9fa027cb13a1b27fea148ffe38124ce4548ec16ad1e9fd3d23fa`.

The event is emitted once at the end, outside every cancellation branch, with
no payload; optional-bus hosts still work, and listener faults go to
`onEventFault` rather than escaping into the caller. The existing proposal's
CHANGELOG, MOD_SEAMS and matching generated SDK documentation travel with it.
No SDK/content version change, lockfile change, strategy or new API seam is
introduced. Source6/PR433 remains inventory only, unapplied. The already
imported noteSpot correction and its earlier immutable package recipe stay
unchanged; no Mandos patch is removed.

## Human verification and observed controls

From a clone containing the pins above, inspect the actual change first:

```sh
git show f9a03f0564dbbae9b2b5e60404a80436ad1fc70d:packages/core/src/game/player-path.ts |
  sed -n '90,110p'
git diff f9a03f0564dbbae9b2b5e60404a80436ad1fc70d \
  94f2d1f61c4f65b2d9f09b440bb194b5986b4b75 -- packages/core/src/game/player-path.ts
```

The first source ends after clearing `cmdQueue`. The second adds only the
existing proposal's unconditional guarded signal/fault handoff plus FORK
comment, leaving run/rest/queue cancellation order intact. Compare the exact
stored proposal with its source commit using `git diff --unified=0 e8729a28^ e8729a28`. Use
`git apply --unidiff-zero` when applying the stored zero-context patch.

Run the unchanged proposal tests before applying its production correction,
using disposable worktrees and Node26.13.0 / pnpm12.3.4 / TS5.9.3 / Vitest4.1.11:

```sh
scratch=$(mktemp -d)
git worktree add --detach "$scratch/before" f9a03f0564dbbae9b2b5e60404a80436ad1fc70d
git diff e8729a28^ e8729a28 -- packages/core/src/game/player-path.test.ts \
  packages/web/src/input-flush-event.test.ts | git -C "$scratch/before" apply
(cd "$scratch/before" && ELECTRON_SKIP_BINARY_DOWNLOAD=1 pnpm install --frozen-lockfile --offline)
(cd "$scratch/before" && CI=1 flock /tmp/angband-validation.lock pnpm build)
(cd "$scratch/before" && CI=1 flock /tmp/angband-validation.lock pnpm exec vitest run \
  packages/core/src/game/player-path.test.ts packages/web/src/input-flush-event.test.ts --reporter=verbose)
# The last command is expected to exit 1; inspect the failure summaries.
git worktree add --detach "$scratch/after" 94f2d1f61c4f65b2d9f09b440bb194b5986b4b75
(cd "$scratch/after" && ELECTRON_SKIP_BINARY_DOWNLOAD=1 pnpm install --frozen-lockfile --offline)
(cd "$scratch/after" && CI=1 flock /tmp/angband-validation.lock pnpm build)
(cd "$scratch/after" && CI=1 flock /tmp/angband-validation.lock pnpm exec vitest run \
  packages/core/src/game/player-path.test.ts packages/web/src/input-flush-event.test.ts --reporter=verbose)
```

Offline installation assumes the frozen-lock packages are cached. Populate
that cache without changing the lock on a new machine. Build before both
runs: the web test imports the actual workspace core package's compiled entry,
so source-only testing without rebuilding would grade stale JavaScript.

Observed baseline: build exits0; **4 failed / 29 passed / zero skips**, exit1.
The three new bus/fault assertions fail: heard0 instead of2, heard0 instead
of1, faults[] instead of[input-flush]. The seeded real-game mod test fails
because the granted subscription hears0 events rather than at least1. The
29 existing run/path/queue controls pass. The new test's no-bus and no-grant
checks execute before the failing event assertions; they are not counted as
separate passing test cases.

Observed corrected source: build exits0; **33/33 pass / zero skips**, exit0,
using unchanged tests/assertions/seed/timeouts. A bus listener hears one event
per disturbance even with no work to cancel, and a throwing listener is handed
to the fault hook. The web case boots the shipped complete content at seed3,
depth0, full birth HP, subscribes through the real capability wrapper and issues
a legal `REST_ALL_POINTS` rest via the registry/runGameLoop. Completion cancels
the rest and emits at least one event; a mod without the grant has no events
facade. Town visibility can add disturbances, so the existing count assertion
is a lower bound, not an invented exact event count.

This proves engine event delivery and the grant/control boundary. It does not
claim every mod's multi-step strategy, a production desktop takeover, player
death or lab deployment. Baseline and corrected logs retain actual output;
only terminal colours/task path spelling are normalised for durable records.

## Full validation and retained failed run

Build/typecheck, lint and production bundle pass at the tested product head
94f2d1f6. The first normal `pnpm test` run **failed**: 14149 passed / 5 failed /
12 existing skips, 713 files passed / 5 failed / 3 skipped, 359.08s. All five
failures report their existing timeouts, not expected-value assertion failures:
msg.upstream many_repeat (5s), knowledge-recall (5s), mod-install archive limit
(15s), generation greater-vault case (5s), and object-info seed123 Human Warrior
(30s). [Failed output](default-full-failure.log) and
[stage commands/exits](default-full-gates.json) are preserved; this run is not
a passing full gate and its cause is not claimed proved.

A bounded diagnostic runs those same five files, with **no source, assertion,
seed, skip or timeout change**, using `--maxWorkers=2`: **306/306 passed**, all
five files passed / zero skips, 58.40s. [Diagnostic output](timeout-files-bounded.log)
and [command/exit receipt](timeout-files-bounded.json) retain that evidence.
This is not a substitute for full coverage or proof of the first run's cause.
The shared lock was released after this diagnostic to let queued native C/port
work proceed fairly before another full run.

The required remaining local sequence is the entire `vitest run` with bounded
worker scheduling, CLI golden scenarios, all publishable package checks, MCP
transport smoke, desktop build, both smoke-filter self-checks and the11-step
Electron/Xvfb play smoke, sequentially under the shared lock. The equivalent
normal full hosted CI (default scheduling) is required on the published code
head; metadata-only CI cannot satisfy this source change. Final actual run
outcomes and parent review/merge disposition are recorded on the canonical
source5 issue and owned integration PR, rather than creating CI feedback-loop
commits for changing status.

Local tools are Node26.13.0, pnpm12.3.4, TypeScript5.9.3, Vitest4.1.11 and
npm11.20.0. The local live-game gate uses an existing same-pinned Electron43.2.0
binary read-only via ELECTRON_OVERRIDE_DIST_PATH with the disposable task's
bundle/profile; it does not reuse the running lab's app/profile/data. Hosted
CI retains its normal Electron install and npm packaging toolchain.

## Consumer boundary

Source5 remains OPEN while upstream432 is unresolved. This owned-fork carry is
not upstream resolution and receives no merged-upstream or consumer-state tag.
[Mandos128](https://github.com/diegoami/mandos/issues/128) independently owns its
existing package/lab backport, rebuilding/adoption and consumer checks. No
package is published, no release/tag created and no owner's lab/saves/profiles
or data changed. Parent reviews the owned-fork code PR before merge. No next
bug/feature queue is started by this task.
