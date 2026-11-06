# CBUG-0002: knockout leaves C Borg reactions suppressed

Source issue: [diegoami/neo-angband-tracker#14](https://github.com/diegoami/neo-angband-tracker/issues/14).
Local draft proposal: [#210](https://github.com/diegoami/neo-angband-tracker/pull/210),
commit `32ab6823c6a993d94ce382b72f8a855a20f7911c`, against
`c-borg-reference` at pristine C `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`.
Discovery credit: borg-rs, retained CBUG-0002/group 14 provenance on the source issue.
No official Angband submission or consumer application.

## Independently observed consequence

A Human Warrior recovers from knockout and kills a visible kobold using
ordinary movement/attack commands. The game emits the death message at turn
519, but the uncorrected Borg queues **zero** kill reactions. Correcting the
reset queues **one** kill reaction. Thus later combat information is discarded
from the Borg's reaction input after recovery. This does not demonstrate a
later tactical decision change, player death, or loss of saved progress.

`borg-messages.c:441` sets global `borg_dont_react` for the current knockout
message; `borg-messages-react.c:45` reads that global. The input boundary in
`borg.c:312` instead resets the distinct, unused struct field. The proposal
resets the global after `borg_parse(NULL)`, preserving suppression while the
message batch is parsed. It changes only that assignment and adds a FORK
source marker; no gameplay rules or RNG calls change.

The current paralysis message is **"You are paralysed!"**
(`lib/gamedata/player_timed.txt:213`); the parser checks **"You are paralyzed"**.
The paralysis control therefore does not enter this global gate. The imported
report's paralysis trigger is not promoted to an independently observed fault.
The compatible knockout text is at `player_timed.txt:272`.

## Native scenario and interventions

- Original C game and C Borg, pinned above; GCC 15.2.0, CMake 3.31.10, Debug,
  `SUPPORT_BORG=ON`, `SUPPORT_TEST_FRONTEND=ON`. No Rust or Neo build.
- Full game initialization and normal Human Warrior birth; RNG state seeded
  to **1000** before birth, Borg local seed **1000**, birth HP **20**, depth **0**.
  Shipped [borg.txt](borg.txt); cheat-death and cheat-live both checked false.
- Synthetic entry calls the normal checked `player_inc_timed()` API with
  STUN **151**, STUN **100**, PARALYZED **5**, or no effect, depending on mode.
  No enemy-inflicted knockout or uninstrumented desktop reproduction is claimed.
  `run_game_loop()` performs forced sleep/recovery; ordinary hold commands let
  remaining stun expire. HP remains 20 at full recovery.
- A synthetic adjacent kobold is placed using the native monster-placement API.
  Visibility is checked with the real perception state before each command.
  The driver chooses ordinary `CMD_WALK` directions toward the visible creature;
  the native combat engine supplies hit/miss/fear/death messages and RNG. The
  kobold can flee; the driver follows its currently visible location.
- [scenario.c](scenario.c) replaces message rendering with a headless terminal
  adapter: it draws each **engine-generated** message, calls the real Borg input
  hook/screen scraper, clears the line, then drains a queued Escape through the
  real input boundary. This deliberately avoids an autonomous thinking loop.
  It does not synthesize parser strings, set the suppression global, edit
  reaction counts, force death/damage, or replace the production reset logic.
- Synthetic timing, placement and renderer/input scheduling are test
  interventions. This is a native game integration scenario with ordinary
  combat commands, not an assertion of the complete desktop UI event schedule.
- All writable observer paths are explicitly redirected to
  `/tmp/native-c-14-evidence/profile` **before** game initialization. Upstream
  tests additionally use `PRIVATE_USER_PATH=/tmp/native-c-14-tests`; their shell
  cleanup path is redirected to that same disposable root. See the complete
  [observation.patch](observation.patch). These harness changes are excluded from
  the C proposal. Final comparison begins after these interventions were frozen.

## Before/after and controls

| Mode | Recovery turn | Unfixed kill reactions | Corrected kill reactions | Result |
| --- | ---: | ---: | ---: | --- |
| Knockout, STUN 151 | 511 | 0 | 1 | Fails before, passes after |
| Heavy stun, STUN 100 | 341 | 1 | 1 | Logs byte-identical |
| Paralysis, duration 5 | 51 | 1 | 1 | Logs byte-identical |
| No effect | 1 | 1 | 1 | Logs byte-identical |

Both native `alltests` gates pass **965/965 unit checks** through
`allunittests`, plus **5/5 frontend tests**. No relevant skips or assertion/seed
changes. Existing compiler warnings in `datafile.c:339` and `obj-init.c:90`
discard const qualifiers; they precede this correction. No new warning was
introduced by the reset. CTest is not used.

The upstream unit executable returns status 0 even when a case fails.
[run-modes.py](run-modes.py) therefore records the actual `finished: 1/1 passed`
summary; the decisive failing case also records the completed kill and
`KILL_REACTION delta=0`. Exit status alone is not a passing receipt.
[comparison.json](comparison.json), [receipt.json](receipt.json), all eight
scenario logs and both full-gate logs retain the observations and hashes.

## Reproduce

Use fresh disposable paths, or preserve existing evidence before reusing these
exact names. Copy this report's `scenario.c`, `run-modes.py` and `borg.txt` into
`/tmp/native-c-14-evidence` (the configuration goes in `profile/borg.txt`). The
observation patch deliberately uses those explicit disposable paths.

From a clone of this fork:

```sh
git worktree add --detach /tmp/angband-c-reaction-reset e473720ed88a0f0c7ca66815cef3000eb4a38bc3
git -C /tmp/angband-c-reaction-reset apply /PATH/TO/observation.patch
flock /tmp/angband-validation.lock cmake -S /tmp/angband-c-reaction-reset -B /tmp/angband-c-reaction-build -DSUPPORT_BORG=ON -DSUPPORT_TEST_FRONTEND=ON -DCMAKE_BUILD_TYPE=Debug
flock /tmp/angband-validation.lock cmake --build /tmp/angband-c-reaction-build --parallel 2
flock /tmp/angband-validation.lock python3 /tmp/native-c-14-evidence/run-modes.py before /tmp/angband-c-reaction-build/game/CBUG0002Observer
flock /tmp/angband-validation.lock cmake --build /tmp/angband-c-reaction-build --target alltests --parallel 2
# Apply only the proposed C source hunk; keep the observer, seed and controls unchanged.
git -C /tmp/angband-c-reaction-reset apply /PATH/TO/patches/upstream-c/CBUG-0002-reaction-reset.patch
flock /tmp/angband-validation.lock cmake --build /tmp/angband-c-reaction-build --parallel 2
flock /tmp/angband-validation.lock python3 /tmp/native-c-14-evidence/run-modes.py after /tmp/angband-c-reaction-build/game/CBUG0002Observer
flock /tmp/angband-validation.lock cmake --build /tmp/angband-c-reaction-build --target alltests --parallel 2
```

Inspect each mode's pass summary and `KILL_REACTION`, and compare each control's
before/after log with `cmp`. The knockout must fail only before correction.
The full-gate logs preserve the actual commands' native target outcomes.

## Reconciliation and proposal state

Official master was fetched and still equals the recorded C pin. The bounded
open/closed searches in [duplicate-search.json](duplicate-search.json) found no
exact knockout/reset correction. Search indexing and unsearched comments/forums
limit that conclusion. Inspected related PRs 6753 (respawn reaction cleanup),
6228 (Wonder/banishment prompt handling) and 6132 (throwing inventory selection)
change different paths; their closure is not evidence this fault was fixed.
The pin itself still contains the wrong reset.

The source issue stays open. Proposal #210 is a local draft, not an applied
correction or consumer-ready adoption claim. Native C checkpoint #207 owns the
next review action; the parent routes any later consumer work independently.

## Corrected copy/paste replay

The original manual block above assumes files already placed at placeholder
paths and omits the native scenario's `game` working directory. Use the
[complete replay instructions](REPRODUCE.md) and [runner](reproduce.py), added
without changing the original run records. They fetch/extract every fixture,
use fresh temporary directories, apply the clean one-line correction, and
verify both native gates and the before/after controls.
