# The penalty that counted twice

A level twenty-one priest with every class book faces three vampire
bats, its mana tuned so that casting Dispel Undead leaves five points —
inside the final-teleport band of every class reserve.

**The Borg holds back mana for a final teleport, and casting a dispel
from that reserve is punished with a heavy penalty on the maneuver
value — but two back-to-back blocks applied it: one keyed on the class
reserve (six for a Mage, eight for a Priest, up to twenty-two for a
Ranger) and a second keyed on a fixed six. The second is a subset of
the first for every class, since every class reserve is six or more,
so the penalty simply doubled.** Measured: the dispel is valued at
**−21127** with both blocks and **−10627** with one — exactly one
penalty block of 750 × the spell's cost apart.

The correction removes the fixed-six block, keeping the class-based
one.

Control: with twenty points to spare after the cast — outside the
teleport band — neither block applies, and both variants value the
dispel identically at −127 with byte-identical logs: the correction
removes only the duplicated penalty.

Consequence: dispel maneuvers were penalized twice for the same
reserve, biasing the Borg away from its area attacks exactly when its
mana was low. No death or survival claim is made.

Canonical report: [issue259](https://github.com/diegoami/neo-angband-tracker/issues/259).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/dispel-259-evidence
git show b0b58d3209d8248d2103ab202ba69e2de61f133d:docs/audit/c-reports/dispel-259/reproduce.py > /tmp/reproduce-dispel-259.py
python3 /tmp/reproduce-dispel-259.py --fixtures-ref b0b58d3209d8248d2103ab202ba69e2de61f133d --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the removed duplicate
block, runs two recorded situations per variant, checks the
fail-before and pass-after matrix and the byte-identical control,
records both full gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**Deep in the teleport reserve**: the original values the dispel at
−21127; the correction at −10627.

Actual recorded output, original (`before-reserve.log`):

```text
MANA dispel_power=14 cursp=19 maxsp=116 ok=1
GEOMETRY player=(11,11) bats=3 temp_n=3 cursp=19 maxsp=116
RATING value=-21127
OUTCOME value=-21127
```

Corrected (`after-reserve.log`): `RATING value=-10627`. Control
(`clear`): `RATING value=-127` in both variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`e5da34177368960c9707df6c7b8d55e6c4ab081a`. Five deleted lines in
`src/borg/borg-fight-attack.c`. In `borg_attack_aux_spell_dispel()`,
the mana-needed-for-final-teleport penalty appears twice: first keyed
on a per-class `penalty` (Mage 6, Ranger 22, Rogue 20, Priest 8,
Paladin 20, Necromancer 10), then again — with a comment noting it
duplicates the Mage value — keyed on a fixed `6`. Because the fixed
test `CURSP − spell_power < 6` can only fire when the class test
`CURSP − spell_power < penalty` already has (every class reserve is at
least six), the second block doubled the penalty for every class. The
correction deletes it.

```sh
git fetch --no-tags origin task/c-borg-dispel-single-reserve
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-attack.c | sed -n '2680,2720p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 e5da34177368960c9707df6c7b8d55e6c4ab081a -- src/borg/borg-fight-attack.c
```

### Conditions and limits

Native Human Priest level twenty-one, depth five, seed one thousand,
lit constructed 40x40 arena, every class book carried, wisdom
maximized, three awake vampire bats in line of sight, current mana
tuned to five points (or twenty in the control) above the dispel cost
after casting. The measurement calls the public maneuver chooser with
`borg_simulate` set; the raw dispel launch value in this fixture is the
beam-path quirk seen elsewhere (−1), which the penalty arithmetic
dwarfs and does not affect. This isolates the penalty blocks, not full
autonomous play. No death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
