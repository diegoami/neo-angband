# Preparation should queue only the chosen spell

Native C Borg source issue: [#110](https://github.com/diegoami/neo-angband-tracker/issues/110),
CBUG-0094. Discovery credit: borg-rs, retained on the canonical issue and in the
unchanged historical import record. This is independent native C verification.

When choosing a persistent preparation spell, the Borg casts Rapid Regeneration
while merely evaluating it. It then queues the selected spell. If regeneration
wins, it is cast twice. If Haste Self wins, regeneration is still cast first.
The correction returns regeneration's score during simulation and queues it
only when selected, just like the neighboring preparation scorers.

The native fixture measures an extra 20 mana and 100 action energy before the
correction in both cases. This is a medium-priority resource/action-selection
defect. The duplicate cast extends regeneration, so these results do not imply
that its entire effect is useless. In the competing-spell case, it spends mana
and queues an additional action before the higher-priority haste preparation.
No combat damage, death, autonomous encounter frequency or lost progress is claimed.

## Pins and source reasoning

- Pristine C / official master at investigation: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`.
- Proposed C source: `31a042d00581ed914d30f17cca1ed0e70e99d21f`.
- Maintenance base: `5928146b85cc498ca596da16571b6ee28396ef4c`.
- Compiler: Debian GCC 14.2.0; CMake 4.4.4, Debug, `SUPPORT_BORG=ON`,
  `SUPPORT_TEST_FRONTEND=ON`, X11 frontend. Headless observer links the native
  game/core and unit support; there is no Rust or TypeScript runtime involved.

From this repository, inspect the exhaustive named helper/caller search and
the actual scheduling and queue-writing code:

```sh
git fetch --no-tags origin task/c-borg-regen-simulation
git grep -n -w borg_perma_aux_regen e473720ed88a0f0c7ca66815cef3000eb4a38bc3 -- src/borg
git grep -n -w borg_perma_spell e473720ed88a0f0c7ca66815cef3000eb4a38bc3 -- src/borg
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-perm.c | sed -n '428,470p;747,805p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-magic.c | sed -n '596,634p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 31a042d00581ed914d30f17cca1ed0e70e99d21f -- src/borg/borg-fight-perm.c
```

The helper search finds exactly its definition at line 428 and its dispatch
at line 730. The public selector search finds its declaration, definition and
normal dungeon call at `borg-think-dungeon.c:1619`.
The selector sets `borg_simulate = true`, evaluates the preparation choices,
selects the greatest score, sets `borg_simulate = false` and calls the winner.
The baseline regeneration helper reaches `borg_spell(RAPID_REGENERATION)` in
both passes. `borg_spell()` checks affordability and the book, then queues
`m`, the inventory book key and the spell offset; it does not check simulation.
Thus a single selector invocation can enqueue two commands without waiting
for the native player state to show regeneration as active.

The complete source difference adds four lines, before the existing cast:

```c
    /* Simulation */
    if (borg_simulate)
        return 1;
```

Score 1 matches the existing executed return value and other low-priority
preparations. Eligibility, mana/failure thresholds, real execution and the
6000 no-rest assignment remain at their existing points. Moving the gate
ahead of eligibility or returning zero would change selection behavior.

## Verification: repeat the native scenarios

On Debian/Ubuntu with package-install privileges, prerequisites are:

```sh
sudo apt-get update
sudo apt-get install -y git python3 cmake build-essential util-linux libx11-dev
```

Run from an evidence checkout at the immutable publication commit linked in
issue110 and the source PR. `--fixtures-ref HEAD` pins all fixture reads to
that checkout's commit; the source pins above are hardcoded in the runner.
The issue and PR include commands using the full evidence commit explicitly.

```sh
python3 docs/audit/c-reports/CBUG-0094/reproduce.py --fixtures-ref HEAD --full-tests
```

The runner creates a new disposable detached C worktree under `/tmp`, adds only
an observer build target and isolated test/profile paths, builds baseline and
corrected variants and runs the same nine cases in separate processes. It
checks the fixed source bytes against the source proposal. No old workstation
path or save is needed. It prints the evidence directory containing individual
logs, aggregate JSON and both full-gate logs. Heavy operations use
`flock /tmp/angband-validation.lock`.

The cloud used a task-local CMake wheel and extracted Debian X11 development
packages because it has no sudo and system package installation is unavailable.
Those package prefixes affect building, not scenario logic. The runner accepts
extra `--cmake-arg=...` arguments for such environments. Standard installed
headers need no extra options.

Actual scenario results, initial capture and independently repeated by the
portable runner:

| Case | Before | After |
| --- | --- | --- |
| Regeneration wins | 2 regeneration commands; 40 mana; 200 action energy | 1 regeneration; 20 mana; 100 energy |
| Haste wins | regeneration then haste; 32 mana; 200 action energy | haste only; 12 mana; 100 energy |
| Haste only known | 1 haste; 12 mana; 100 energy | identical |
| Regeneration already active | no commands or mana spent | identical |
| Regeneration unlearned | no commands or mana spent | identical |
| Blind | no commands or mana spent | identical |
| Confused | no commands or mana spent | identical |
| Town | no commands or mana spent | identical |
| Half mana | no commands or mana spent | identical |

Both affected baseline cases finish the native casts before failing the
expected-single-command assertion. All nine corrected cases pass. The seven
control logs match byte for byte. Both full native gates report **965/965 unit
checks and 5/5 frontend tests**, without relevant skips. Inspect summaries:
the native unit executable returns zero even when its assertion fails. The
Python runner checks assertions, completion, outcome deltas and controls.

Example actual baseline output:

```text
SETUP seed=1000 class=Druid level=50 depth=50 hp=326/326 sp=401/401 wisdom=208 amulet_wis=6
ELIGIBILITY learned=1 native_fail=3 borg_fail=3 cost=20 okay=1
DECISION result=1 keys=6 no_rest_prep=6000 sequence=mbdmbd
CAST spell=Rapid Regeneration sp_before=401 sp_after=381 energy=100 heal=8 fast=0
CAST spell=Rapid Regeneration sp_before=381 sp_after=361 energy=100 heal=14 fast=0
OUTCOME commands=2 regen=2 haste=0 mana_spent=40 action_energy=200 heal=14 fast=0
```

Corrected regeneration case:

```text
DECISION result=1 keys=3 no_rest_prep=6000 sequence=mbd
CAST spell=Rapid Regeneration sp_before=401 sp_after=381 energy=100 heal=8 fast=0
OUTCOME commands=1 regen=1 haste=0 mana_spent=20 action_energy=100 heal=8 fast=0
```

The competing haste case has the same 401 starting mana and real 3% failure
rate. Baseline spends 20 then 12; corrected spends 12. Both obtain haste.
The differing duration (55 versus 68) comes from the extra baseline spell's
RNG draws; it is not a separate measured benefit.

## Interventions and limits

This is a synthetic native integration fixture, not unattended desktop play.
It births a Human Druid at seed1000, constructs a lit arena at depth50, grants
experience through the native API to level50, increases base Wisdom through
the stat-increase API to its normal cap, creates a legal +6 Wisdom amulet and
Nature Craft book, learns the relevant spells and starts at full native HP/SP.
The +6 amulet is the kind's maximum allowed bonus, obtained with MAXIMISE.
The game computes 401 maximum mana and 3% spell failure; neither is substituted.
Only regeneration is learned in the first case; adding Haste Self tests the
competing action. Controls alter the named condition through normal state/APIs.

The production `borg_perma_spell()` chooses and queues actions, with unchanged
eligibility/score logic. The adapter decodes its actual `m/book/offset` triples,
validates the carried book and learned spell, and calls native `do_cmd_cast()`
with that spell argument. This bypasses interactive UI selection but does not
substitute a spell choice, mana cost, success result or effect. Native spell
casting consumes mana and sets `energy_use` and the real timed effects.
No spell failure is suppressed and no seed is changed to obtain success.

There is no world-loop/monster processing between these diagnostic casts, so
200 versus 100 is the **sum of native command energy costs**, not elapsed
world turns or measured extra monster attacks. The direct selector call
isolates the preparation stage; it does not establish how often a complete
autonomous Borg reaches this setup. Haste/regeneration durations accumulate
without per-turn decrement in this fixture. These limits do not change the
observed extra queued command and native mana consumption.

## Triage and lifecycle

The resume first checked upstream6808/6809 and local226/227/211/222 reviews.
No accepted upstream source correction was available. Source refresh confirmed
official C still at e473720. Our board was searched for open C reports, older
CBUG reports, high-priority reports and crashes; this was a bounded shortlist,
not a complete independent validation of the catalogue. Map-prompt13 already
has a proposal. Device-division76 and no-target254 need their own legal native
reachability proof; shoot/scoot100 needs broader tactical outcome controls.
Cosmetic reports and code-only scoring anomalies remain behind measurable harm.
0094 offered a compact native resource/action consequence and unchanged controls,
so it was selected without promoting those other leads or importing their labels
as proof. Exact native/fork and official searches for regeneration/perma-spell
reports found only canonical110 and unrelated historical results; this bounded
search does not claim exhaustive historical duplicate coverage.

The source issue remains open pending upstream acceptance and normal fork
import/checks. The local source PR is a draft, C-only proposal against unchanged
`c-borg-reference`. This evidence branch targets the separate maintenance lane;
neither merge is source adoption. No official Angband post, Rust watch/change,
engine fix, downstream translation, lab change, deployment or release occurs.
Readiness for owner filing remains subject to human assessment.
