# CBUG-0066: Borg rejects protective backup gear and takes extra frost damage

Source: [issue #82](https://github.com/diegoami/neo-angband-tracker/issues/82).
Original discovery: borg-rs, retained CBUG-0066 provenance on the issue.
C base: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`.
Proposed C correction: `66e1012113fd7db634b19ffe86a470b58a7f7967`.

The injured Borg has a shield that resists cold in its pack. Its emergency
swap routine calculates that wearing the shield is safer, but then considers
the fire-branded dagger instead. Because that dagger does not help against
frost, the Borg decides to change nothing. In this native scenario the next
frost breath costs **79 HP**. After correcting the comparison, the Borg puts
on the cold-resistant shield and the same breath costs **26 HP**: **53 HP less**.

The mirrored case also fails before correction: a cold-resistant weapon is
ignored in favor of considering an unhelpful fire-resistant shield. This is a
wrong equipment decision with a measured protection/HP consequence. It was
selected ahead of diagnostic/color issues during impact triage. It does not
establish how frequently complete autonomous runs reach this state or prove a
player death. The test isolates the emergency-swap stage; it does not run all
other healing/defense/escape decisions around it.

## Verification: inspect the decision

These commands read the exact uncorrected source without changing a checkout:

```sh
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-item-wear.c | sed -n '602,606p;667,671p;702,723p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-caution.c | sed -n '1025,1031p'
```

The first two slices show `b_p1 = borg_danger(...)` after simulating armour,
and `b_p2 = borg_danger(...)` after simulating the weapon. The decisive code is:

```c
    /* Pass on the swap which yields the best result */
    if (b_p1 <= b_p2) {
        b_p  = b_p2;
        swap = weapon_swap - 1;
    } else {
        b_p  = b_p1;
        swap = armour_swap - 1;
    }
```

Read this with the measured numbers: armour **47**, weapon **99**. The condition
`47 <= 99` is true, so it selects **99**, the worse result. The next guard requires
`p > b_p`; current danger is also **99**, so that guard fails. The useful armour
at **47** is never chosen. The 47/99 numbers are danger scores, not measured HP.
The independent native attack below measures the HP cost.

The caller in `borg_caution()` checks `pos_danger > borg.avoidance / 3` before
calling this routine. Our state satisfies that normal threshold: **99 > 263/3**.
The better swap also satisfies the existing acceptance threshold:
**47 <= 263/2**. Thus the failure is not a fixture demanding a swap the caller
or safety threshold would reject.

Inspect the complete proposed source difference:

```sh
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 66e1012113fd7db634b19ffe86a470b58a7f7967 -- src/borg/borg-item-wear.c
```

It changes only the comment and selection condition:

```c
    /* FORK (CBUG-0066): prefer the lower danger among available swaps. */
    if (weapon_swap && (!armour_swap || b_p2 <= b_p1)) {
```

Choose the weapon when it exists and either armour is absent or weapon danger
is no higher. Otherwise choose armour. Availability matters because an absent
candidate leaves a zero-initialized score; merely reversing the old comparison
would break the single-item cases. Equal danger still prefers the weapon,
preserving the existing tie behavior. No damage formula or safety threshold changes.
Related issue #83's zero-danger/missing-candidate report remains separate; this
receipt does not claim its particular zero-danger case was independently reproduced.

## Native results

| Mode | Before | After | HP lost before / after |
| --- | --- | --- | --- |
| Cold shield vs fire dagger | Refuses useful swap | Wears cold shield | 79 / 26 |
| Fire shield vs cold dagger | Refuses useful swap | Wields cold dagger | 79 / 26 |
| Only cold shield | Wears shield | Same | 28 / 28 |
| Only cold dagger | Wields dagger | Same | 28 / 28 |
| Both resist cold, equal danger | Wields dagger | Same | 26 / 26 |
| Neither helps against cold | No swap | Same | 79 / 79 |

Actual relevant output in `before-armour-better.log`:

```text
DANGER initial=99 avoidance=263
DANGER armour=47 weapon=99
DECISION swapped=0 command=0 letter=0 pack_index=-1
PROTECTION cold=0 fire=0
ENGINE: The young white dragon breathes frost.
OUTCOME hp_before=263 hp_after=184 hp_lost=79
native-c/CBUG-0066 finished: 0/1 passed
```

The matching corrected output in `after-armour-better.log`:

```text
DANGER initial=99 avoidance=263
DANGER armour=47 weapon=99
DECISION swapped=1 command=119 letter=101 pack_index=4
SELECTED cold_shield=1 fire_weapon=0
ENGINE: You are wearing a Small Metal Shield of Resist Cold [5,+0] (m).
PROTECTION cold=1 fire=0
ENGINE: The young white dragon breathes frost.
OUTCOME hp_before=263 hp_after=237 hp_lost=26
native-c/CBUG-0066 finished: 1/1 passed
```

All four unaffected control logs are byte-identical before/after. Both native
`alltests` gates pass **965/965 unit checks** (via `allunittests`) and **5/5 frontend
tests**, without relevant skips. The unit runner returns exit 0 on an assertion
failure, so `run-modes.py` checks the actual `finished: 1/1 passed` summary.
`comparison.json`, per-mode logs and `receipt.json` retain the observations and
input/output hashes. Existing GCC15 const-qualifier warnings in datafile.c and
obj-init.c precede this correction; the observer's initial setup/compiler debugging
is excluded from the frozen comparison.

## Setup, interventions and limits

- Full original C game/C Borg, GCC15.2.0, CMake3.31.10, Debug,
  `SUPPORT_BORG=ON`, `SUPPORT_TEST_FRONTEND=ON`. No Rust or Neo code.
- Normal Human Warrior birth with RNG state initialized to **1000**; synthetic
  experience **1,000,000** gives level **40** and **421 maximum HP**. Full health
  is established before combat. Current/maximum depth is **50**, where the
  production Borg enables armour/weapon backups. These are explicit scenario
  initial conditions, not a claim of a naturally played save reaching them.
- Synthetic 24x24 illuminated floor arena; real `player_place()` and monster
  placement, visibility and production monster tracking. The young white dragon
  is awake, visible three grids away, with **239 HP** in the main two-item modes.
- Real generated, identified items: worn acid-resistant small metal shield,
  worn leather boots of Speed (**+6**), and carried cold shield/fire dagger (or
  the corresponding control pair). Existing birth equipment remains. The native
  item/ego APIs generate their legal properties; rune knowledge is established.
  Production inventory analysis and swap selection choose the candidates;
  the fixture does not assign their indices or substitute danger scores.
- Two controlled calls to the dragon's real `do_mon_spell(RSF_BR_COLD, ...)`
  hurt the character before the decision: **421 -> 342 -> 263 HP** in the main
  modes. No direct HP reduction is injected. One-item modes have different
  generation RNG consumption and start the decision at **251 HP**; compare
  each mode only against its unchanged before/after counterpart.
- The observer independently prints each candidate's native danger estimate by
  temporarily trying the Borg's inventory mirror and restoring it, then calls
  **unmodified production `borg_backup_swap()`** before the patch. A positive
  decision queues `w` and the selected inventory letter. The observer decodes
  that real letter and applies the corresponding native `inven_wield()` action;
  it does not replace the selection with an expected item.
- The next cold breath is controlled through the same native spell routine,
  not hand-calculated or directly subtracted. Damage, resistances and inventory
  side effects are real. Scheduling the spell and applying the selected wield
  action directly isolates protection; it does not reproduce full UI key dispatch,
  an autonomous caution loop, or all alternative actions/turn timing. The result
  establishes a harmful choice at this decision stage, not a complete-run death.
- Assertions, seeds, item construction and modes are unchanged across the final
  before/after comparison. `scenario.c` is the complete frozen observer.
- Observer profile/save/score/archive paths are explicitly redirected before
  initialization to `/tmp/native-c-82-evidence/profile`. Upstream tests use
  `/tmp/native-c-82-tests`, including their cleanup path. No owner profile or
  lab data is needed. All heavy gates used `/tmp/angband-validation.lock`.

## Reproduce

Use fresh disposable paths, preserving existing evidence. Copy this directory's
`scenario.c` and `run-modes.py` to `/tmp/native-c-82-evidence`, and `borg.txt` to
`/tmp/native-c-82-evidence/profile/borg.txt` (create the directories first).
The retained observation patch uses those explicit paths.

From a clone containing the base and proposed source commits:

```sh
git worktree add --detach /tmp/angband-c-safer-swap e473720ed88a0f0c7ca66815cef3000eb4a38bc3
git -C /tmp/angband-c-safer-swap apply /PATH/TO/observation.patch
flock /tmp/angband-validation.lock cmake -S /tmp/angband-c-safer-swap -B /tmp/angband-c-safer-swap-build -DSUPPORT_BORG=ON -DSUPPORT_TEST_FRONTEND=ON -DCMAKE_BUILD_TYPE=Debug
flock /tmp/angband-validation.lock cmake --build /tmp/angband-c-safer-swap-build --parallel 2
flock /tmp/angband-validation.lock python3 /tmp/native-c-82-evidence/run-modes.py before /tmp/angband-c-safer-swap-build/game/CBUG0066Observer
flock /tmp/angband-validation.lock cmake --build /tmp/angband-c-safer-swap-build --target alltests --parallel 2
# Apply only the proposed C patch, keeping the scenario and assertions unchanged.
git -C /tmp/angband-c-safer-swap apply /PATH/TO/patches/upstream-c/CBUG-0066-safer-swap.patch
flock /tmp/angband-validation.lock cmake --build /tmp/angband-c-safer-swap-build --parallel 2
flock /tmp/angband-validation.lock python3 /tmp/native-c-82-evidence/run-modes.py after /tmp/angband-c-safer-swap-build/game/CBUG0066Observer
flock /tmp/angband-validation.lock cmake --build /tmp/angband-c-safer-swap-build --target alltests --parallel 2
```

Read each result summary and `OUTCOME`. Compare the armour-only, weapon-only,
tie and no-benefit logs with `cmp`; all four should match. The first two modes
fail only before correction. The source diff remains one file; the observation
CMake/test-path changes are excluded from it and retained in `observation.patch`.

## Reconciliation and delivery

Official C was fetched and still equals the recorded base. The bounded open/
closed searches in `duplicate-search.json` found no exact emergency-comparison
correction. Related official PR5945 changes curse-removal guards; PR6041 changes
ring refresh/recharge paths. Their closure does not establish this fix. Search
indexing and unsearched comments/forums limit any duplicate-absence conclusion.

This is a local C proposal/stored patch. Source issue #82 remains open. No
upstream submission, consumer translation/application, engine merge, lab change,
publication or deployment is performed here. The owner can use the direct
comparison and copyable proposal text on the canonical issue when satisfied.
