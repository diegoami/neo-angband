# The Phial that shone twice as bright

Noticing the equipment added an item's light modifier to the Borg's
light radius twice — once before and once after the flag adjustments.

**`borg_notice_equipment()` adds `modifiers[OBJ_MOD_LIGHT]` to
`BI_LIGHT` on both sides of the `LIGHT_2`/`LIGHT_3` and Necromancer
unlight adjustments, so any item with a real light modifier — the
artifact lights — lit twice as much in the Borg's mind as in the
dungeon.** Measured natively with the Phial of Galadriel (LIGHT[4]):
the original reports light radius 8; the correction counts the
modifier once and reports 4.

Control: a stock Wooden Torch draws its radius from the `LIGHT_2`
flag, not a modifier — light 2 in both variants, byte-identical logs.

Consequence: the Borg overestimates its light radius by the modifier,
feeding every darkness and out-of-light decision with an inflated
value. No crash; no death or survival claim.

Canonical report: [issue238](https://github.com/diegoami/neo-angband-tracker/issues/238).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/light-238-evidence
git show PINSHA:docs/audit/c-reports/light-238/reproduce.py > /tmp/reproduce-light-238.py
python3 /tmp/reproduce-light-238.py --fixtures-ref PINSHA --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the removal of the
doubled add, runs the Phial and torch situations per variant through
the production equipment notice, checks the fail-before and pass-after
matrix and the byte-identical control, records both full gates and
writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The Phial that shone twice as bright**: the original counts the
modifier twice (radius 8 for LIGHT[4]); the correction counts it once
(4).

Actual recorded output, original (`before-phial.log`):

```text
GEOMETRY light_mod=4 bi_light=8
RATING bi_light=8
```

Corrected (`after-phial.log`): `RATING bi_light=4`. Control
(`torch`): `RATING bi_light=2` in both variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`66b183340a567811051a5745c8d3a53792963860`. Two deleted lines in
`src/borg/borg-trait.c`: the stray second
`borg.trait[BI_LIGHT] += item->modifiers[OBJ_MOD_LIGHT];` after the
flag and unlight adjustments.

```sh
git fetch --no-tags origin task/c-borg-light-mod-once
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-trait.c | sed -n '1508,1530p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 66b183340a567811051a5745c8d3a53792963860 -- src/borg/borg-trait.c
```

### Conditions and limits

Native Human Warrior on a lit arena at depth 30, seed one thousand,
wearing the Phial of Galadriel (created through the artifact data) or
a stock Wooden Torch; the Borg's equipment notice runs in full. The
measured value is the Borg's own light-radius trait; the consequence
is an inflated radius in darkness and out-of-light decisions, not a
crash — no death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
