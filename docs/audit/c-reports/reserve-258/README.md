# The emergency bolt that never fired

A level-fourteen mage with no mana — the emergency premise: the reserve
bolt is only considered when the mage can no longer cast anything the
normal way — faces a kobold two grids away, while a young red dragon
stands far across the arena, observed first so it owns the lower kill
index.

**The monster scan behind the emergency magic missile broke at the
first live monster.** It examined whichever monster happened to own the
lowest kill index — here the distant dragon — and the too-many-hit-
points veto applied to it: **a distant tough monster vetoed the
emergency bolt for a nearby weak one, and the maneuver was valued at
zero.** Meanwhile `near_monsters` could never exceed one, so the
only-be-one-near-monster test that guards against fainting never
failed.

The correction scans every monster and applies the hit-points veto to
the near ones it describes: the distant dragon is skipped, the kobold
qualifies, and the maneuver is valued at 40.

Control: with only the kobold present, the first live monster is the
right one, and both variants value the maneuver identically at fifty
with byte-identical logs — the fix changes only which monsters the scan
examines.

Consequence: the young mage's last-resort attack was systematically
miscued by monster index order — either vetoed by an irrelevant distant
monster, or blind to a second nearby one. No death or survival claim is
made.

Canonical report: [issue258](https://github.com/diegoami/neo-angband-tracker/issues/258).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/reserve-258-evidence
git show PINSHA:docs/audit/c-reports/reserve-258/reproduce.py > /tmp/reproduce-reserve-258.py
python3 /tmp/reproduce-reserve-258.py --fixtures-ref PINSHA --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the scan-all loop, runs
two recorded situations per variant, checks the fail-before and
pass-after matrix and the byte-identical control, records both full
gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The distant dragon that vetoed the emergency bolt**: the original
values the maneuver at 0; the correction at 40.

Actual recorded output, original (`before-veto.log`):

```text
GEOMETRY player=(11,11) weakling=(11,13) bully=(11,25) danger=544 avoidance=4 temp_n=2
RATING value=0 danger_gate=1
OUTCOME value=0
```

Corrected (`after-veto.log`): `RATING value=40 danger_gate=1`. Control
(`control`): `RATING value=50 danger_gate=1` in both variants,
byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`7b4ecd6b888a1d87b526f273c769449eb013ad9b`. One loop in
`src/borg/borg-fight-attack.c`. In
`borg_attack_aux_spell_bolt_reserve()`, the monster loop counted
`near_monsters` for grids closer than seven, applied the
too-many-hit-points veto, and then `break`ed — unconditionally — after
the first live monster in the kill list. The correction removes the
`break` and scopes the hit-points veto (and the town check that shared
its block) to the near monsters the veto's comment describes; the
fainting guard then counts every nearby monster.

```sh
git fetch --no-tags origin task/c-borg-reserve-near-monsters
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-attack.c | sed -n '2554,2600p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 7b4ecd6b888a1d87b526f273c769449eb013ad9b -- src/borg/borg-fight-attack.c
```

### Conditions and limits

Native Human Mage level fourteen, depth five, seed one thousand, lit
constructed 40x40 arena, zero current mana with a full maximum (the
emergency premise the maneuver requires), avoidance tuned low so the
pair of monsters clears the must-be-dangerous gate. The kobold stands
two grids away; the young red dragon fourteen grids away, observed
first so it owns the lower kill index. The measurement calls the public
maneuver chooser with `borg_simulate` set. This isolates the monster
scan, not full autonomous play. No death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
