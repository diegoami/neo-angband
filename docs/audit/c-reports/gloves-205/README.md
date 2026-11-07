# Gloves could never be chosen as the armour swap

A Warrior past depth 50 carries a pair of gauntlets of Free Action
while having no free action of its own — exactly the situation the
situational armour swap exists for: the main gear lacks something the
depth needs, so the Borg scores every wearable pack item and carries
the best one for a quick change.

**The scoring switch in `borg_notice_armour_swap()` covered rings,
amulets, boots, helms, crowns, shields, cloaks and the three
body-armour tvals — but not `TV_GLOVES`.** A carried pair of
free-action gauntlets was therefore never eligible, however badly the
Borg needed free action (paralysis is the classic killer at depth).
Measured on the exact situation (seed 1000, declared deepest level
60, free action granted to the fixture's gauntlets the way an ego or
artifact pair carries it): the original tracks `armour_swap=0` — no
swap at all — and records no free-action swap flags; the corrected
scan picks the gauntlets (`armour_swap=5`, the gauntlets' pack slot
plus one) with value 10609, of which 10000 is the free-action reward.

Two controls keep the scan empty and stay byte-identical: plain
gauntlets with no free action are not worth a swap in either variant
(value stays below the 1000 threshold), and a carried free-action
pair stays a non-swap when free action is already worn (the
free-action reward is gated on lacking it). Owner intent recorded on
the issue 2026-11-07: the omission is a defect; add `TV_GLOVES`.
Class-sensitive glove concerns (caster mana penalties) are already
handled by the main-gear glove logic; the swap is only worn
situationally. No survival claim beyond the tracked swap is made.

Canonical report: [issue205](https://github.com/diegoami/neo-angband-tracker/issues/205).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal: PR (see the issue).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/gloves-205-evidence
git show 9a6927a4f4025d05ec6ec78866c607d56acebe03:docs/audit/c-reports/gloves-205/reproduce.py > /tmp/reproduce-gloves-205.py
python3 /tmp/reproduce-gloves-205.py --fixtures-ref 9a6927a4f4025d05ec6ec78866c607d56acebe03 --full-tests
```

The runner builds two variants (original, corrected) from pristine C
`e473720` in fresh temporary directories, verifies the source diff is
exactly the `case TV_GLOVES:` addition, runs three recorded
situations on each, compares controls and gates, and writes
`replay.html` for offline review. Your checkout and character/profile
are not used or edited. The disposable source and binary finish on
the correction.

The native run recorded in this directory's logs executed at fixtures
pin `9a6927a4f4025d05ec6ec78866c607d56acebe03`. The captured replay,
its data and this guide were then added on top; no generator change
of any kind followed the run (commit list: pin, then one artifact
commit).

Open [replay.html](replay.html) (download raw and open locally).
Choose **Free-action gauntlets in the pack, none worn**, then **The
pack** and **The armour-swap scan**: the original picks no swap, the
corrected picks the gauntlets and records free action. Choose
**Control: plain gauntlets** and **Control: free action already
worn**: both logs match byte for byte between versions.

Actual recorded output, original (`before-glove-fa.log`; the scenario
asserts the corrected tracking, so the original fails it — that is
the fail-before contract):

```text
SETUP gloveslot=4 borg_fa=1 game_fa=1 game_known_fa=1
SWAPS armour_swap=0 value=-1 free_act=0
native-c/gloves-205 finished: 0/1 passed
```

Corrected (`after-glove-fa.log`):

```text
SWAPS armour_swap=5 value=10609 free_act=1
native-c/gloves-205 finished: 1/1 passed
```

Controls (`before-plain.log` / `after-plain.log`,
`before-have-fa.log` / `after-have-fa.log` — byte-identical pairs):

```text
SWAPS armour_swap=0 value=-1 free_act=0
```

Full native gates on both variants: 965/965 unit checks and 5/5
frontend tests (`before-alltests.log`, `after-alltests.log`).

### Conditions and limits

Native Human Warrior, seed 1000, glowing constructed arena, declared
deepest level 60 (the swap gate is 50), full hp. Free Action was
granted to the fixture's gauntlets by flag on both the object and its
known copy, matching what an ego or artifact pair carries; the
production armour-swap scan then runs directly and its tracked swap
and flags are read back — nothing is worn or executed in the game.
This isolates the swap eligibility, not full autonomous play.

### File inventory

- `scenario.c` — the native observer (three modes via `GL205_MODE`).
- `run-cases.py` — runs each mode, strips ANSI, writes per-mode logs
  and `*-results.json`.
- `make_replay.py` — builds `replay.html` + `replay-data.json`.
- `reproduce.py` — the pinned runner described above.
- `borg.txt` — Borg config for the disposable profile.
- `before-glove-fa.log`, `after-glove-fa.log`, `before-plain.log`,
  `after-plain.log`, `before-have-fa.log`, `after-have-fa.log` —
  captured scenario logs (the control pairs are byte-identical).
- `before-results.json`, `after-results.json` — parsed per-mode rows.
- `configure.log`, `before-build.log`, `after-build.log`,
  `before-scenarios.log`, `after-scenarios.log`,
  `before-alltests.log`, `after-alltests.log` — run-root logs.
- `result.json` — pins, per-case results, identical controls, gates,
  fixed-source sha256.
- `replay.html`, `replay-data.json` — the offline demo.
- `receipt.json` — sha256 map of every file in this directory.
