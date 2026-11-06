# The ten rapiers that weighed as one

Totalling up the home, the Borg asked how many blows a stack of
weapons gives — and weighed the whole stack as if it were one weapon.

**`borg_calc_blows()` multiplied the weapon's weight by the stack
size, while the engine's `calc_blows()` weighs a single weapon
(`object_weight_one`). For the wielded weapon the two agree (quantity
one), but a stack of light weapons in the home was treated as one very
heavy weapon and under-counted its blows.** Measured natively with a
stack of ten rapiers (strength 17, dexterity 16): the original
computes 1 blow where the engine gives 2 per weapon; the correction
weighs one weapon and both say 2.

Control: a single rapier (stack size one) agrees in both variants —
2 blows, byte-identical logs.

Consequence: the home-damage total is wrong for stacked weapons,
biasing the Borg's home comparisons. The measured fact is the
divergence itself; what it does to a home decision is not claimed. No
crash; no death or survival claim.

Canonical report: [issue313](https://github.com/diegoami/neo-angband-tracker/issues/313).
Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/homeblows-313-evidence
git show PINSHA:docs/audit/c-reports/homeblows-313/reproduce.py > /tmp/reproduce-homeblows-313.py
python3 /tmp/reproduce-homeblows-313.py --fixtures-ref PINSHA --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the single-weapon
weight, parses a ten-rapier stack and a single rapier through the
production item analyzer, computes the Borg and engine blows side by
side per variant, checks the fail-before and pass-after matrix and the
byte-identical control, records both full gates and writes an offline
`replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The ten rapiers that weighed as one**: the original weighs the whole
stack (1 blow against the engine's 2); the correction weighs one
weapon (2 equals 2).

Actual recorded output, original (`before-stack.log`):

```text
GEOMETRY iqty=10 weight=40 weight_one=40
RATING borg_blows=1 engine_blows=2
```

Corrected (`after-stack.log`): `RATING borg_blows=2 engine_blows=2`.
Control (`single`): `RATING borg_blows=2 engine_blows=2` in both
variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`d7b42db02cc74473bc175d2c37dd6feba1a77811`. One line in
`src/borg/borg-trait.c`: `item->weight * item->iqty` becomes
`item->weight`, matching the engine's single-weapon weight.

```sh
git fetch --no-tags origin task/c-borg-blows-single-weight
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-trait.c | sed -n '1035,1048p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 d7b42db02cc74473bc175d2c37dd6feba1a77811 -- src/borg/borg-trait.c
```

### Conditions and limits

Native Human Warrior (strength 17, dexterity 16) on an arena at depth
30, seed one thousand, with a stack of ten identified rapiers in the
pack; the item is parsed through the production borg item analyzer and
the blows are computed by the production borg formula and the engine
formula side by side. No death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
