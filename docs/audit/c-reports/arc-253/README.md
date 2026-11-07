# An arc that hit everything in range

A Mage considers Acid Spray, a sixty-degree arc, at a kobold four
grids north. Another kobold stands four grids south — inside the
spell's ten-grid range but one hundred eighty degrees off the spray.

**The angle test that delineates the arc reoriented the spell's target
grid instead of the grid the blast scan was looking at, so every grid
received the same angle difference: the arc counted every monster in
range and line of sight, or none.** In practice the constant passed,
and the spray was valued as if it hit everything around the caster:
17 with the southern kobold counted, against a reference of 6 with
that kobold hidden from the Borg's knowledge. The correction measures
each scanned grid; the off-arc bystander adds nothing and the value
returns to 6.

Control: with two monsters standing inside the arc, both versions
count both and the complete log is byte-identical before and after —
the correction removes only off-angle counting, not legitimate
coverage.

Consequence: arc spells were over-valued in exactly the crowded,
surrounded situations where the Borg relies on the angle to choose
them. No death or survival claim is made.

Canonical report: [issue253](https://github.com/diegoami/neo-angband-tracker/issues/253).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/arc-253-evidence
git show 4af016461e556cae771c4870e5dffad27c1a3077:docs/audit/c-reports/arc-253/reproduce.py > /tmp/reproduce-arc-253.py
python3 /tmp/reproduce-arc-253.py --fixtures-ref 4af016461e556cae771c4870e5dffad27c1a3077 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the scanned-grid
orientation, runs two recorded situations per variant, checks the
fail-before/pass-after matrix and the byte-identical control, records
both full gates and writes an offline `replay.html`. The native
verification ran with these generators except the replay generator's
final parenthesis, completed for the captured package; no native log,
source variant or gate differs.

Open [replay.html](replay.html) (download raw and open locally). Choose
**A bystander behind the spray**: the original values the spray at 17
with a reference of 6; the correction values it at 6. The control
shows both in-arc monsters counted identically.

Actual recorded output, original (`before-arc-bystander.log`):

```text
GEOMETRY front=(11,7) behind=(11,15) player=(11,11) temp_n=2
RATING reference=6 value=17
OUTCOME reference=6 value=17
```

Corrected (`after-arc-bystander.log`): `RATING reference=6 value=6`,
`OUTCOME reference=6 value=6`. Control (`arc-both`): value 44 in both
variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`7f33e9fbf83059a0a1ed5e530f463124ce1e4be0`. Two tokens in
`src/borg/borg-fight-attack.c`. Inside the blast scan
`for (ry ...) for (rx ...)` of `borg_launch_arc_at_location()`, the
table lookup for the current grid was built from the function's target
parameters (`y - y1 + 20`, `x - x1 + 20`) — a constant for the whole
scan — instead of the scanned grid (`ry - y1 + 20`, `rx - x1 + 20`).
The comparison therefore evaluated the same angle difference for every
grid.

```sh
git fetch --no-tags origin task/c-borg-arc-grid-angle
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-attack.c | sed -n '1930,1985p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 7f33e9fbf83059a0a1ed5e530f463124ce1e4be0 -- src/borg/borg-fight-attack.c
```

### Conditions and limits

Native Human Mage level 26, depth 30, seed 1000, lit constructed
40x40 arena, the Attacks and Knowledge magic book, Acid Spray learned
as the only spell, full mana, a torch. The production valuation call
and its damage expression are used unchanged; the reference value
hides the bystander from the Borg's knowledge for one measurement and
restores it (kill record save/restore). Damage contributions fall with
distance. This isolates the angle test, not full autonomous play. No
death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
