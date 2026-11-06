# The weapon whose slays vanished

A level thirty-one warrior wields a Broad Sword carrying the animals
slay, with a lit torch in the later light slot, and faces an adjacent
white harpy.

**The slay traits were assigned, not accumulated, for every equipment
slot in the Borg's inventory scan. The last occupied slot — usually the
light or the boots, which carry no slays — therefore erased whatever
the wielded weapon provided, and every melee valuation saw an unslaying
weapon.** The Borg rated the Slay Animal blade exactly like a plain
one: the thrust against the harpy is worth 17, identical to the same
attack with the slay removed by hand. Restoring the accumulation rates
it at 20 — exactly the reading with the slay forced on.

This runs on every `borg_notice` call in real play: every weapon with a
slay or brand was systematically undervalued against the races it
slew, biasing weapon swaps and melee decisions. The correction also
derives the never-set kill traits from the same race flags, where the
race-destroying slays are stored with the largest multiplier.

Control: against a small kobold, which carries no animal flag, all
three readings coincide (19) and the complete log is byte-identical
before and after — the correction changes only the case where the slay
applies. The forced-on reading is identical in both variants, so the
correction restores exactly the value the slay is worth.

Consequence: every slay weapon's melee valuation against matching
races. No death or survival claim is made.

Canonical report: [issue239](https://github.com/diegoami/neo-angband-tracker/issues/239).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/slay-239-evidence
git show PINSHA:docs/audit/c-reports/slay-239/reproduce.py > /tmp/reproduce-slay-239.py
python3 /tmp/reproduce-slay-239.py --fixtures-ref PINSHA --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the accumulating slays
plus the kill-trait derivation, runs two recorded situations per
variant, checks the fail-before and pass-after matrix and the
byte-identical control, records both full gates and writes an offline
`replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**A Slay Animal weapon against an animal**: the original values the
thrust at 17 — the no-slay reading; the correction values it at 20.

Actual recorded output, original (`before-animal.log`):

```text
TRAIT ws_animal=0
RATING auto=17 without=17 with=20
OUTCOME auto=17 without=17 with=20
```

Corrected (`after-animal.log`): `TRAIT ws_animal=2`,
`RATING auto=20 without=17 with=20`. Control (`nonanimal`):
`RATING auto=19 without=19 with=19` in both variants, byte-identical
logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`9ab78ae605ca4463268a669121d69b216285c2f1`. One block in
`src/borg/borg-trait.c`. Inside the equipment loop of
`borg_notice_equipment()`, the eight slay traits were plain
assignments (`borg.trait[BI_WS_ANIMAL] = item->slays[RF_ANIMAL];`), so
each later slot overwrote whatever an earlier slot provided — and
since the weapon slot comes before the light and boots, which have no
slays, the result was always zero. The brand traits directly below
already used the accumulate-safe form (`if (item->brands[...]) trait
= true`). The correction accumulates the slays with `|=` and derives
the kill traits (whose slay-table entries share the race flags but
carry the largest multiplier) for the multiplier ladder in
`borg_thrust_damage_one()` (×2 animal/evil, ×3 other slays and brands,
×5 kills), whose kill branches were dead because the traits were never
set.

```sh
git fetch --no-tags origin task/c-borg-slay-traits
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-trait.c | sed -n '1440,1470p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 9ab78ae605ca4463268a669121d69b216285c2f1 -- src/borg/borg-trait.c
```

### Conditions and limits

Native Human Warrior level thirty-one, depth five, seed one thousand,
lit constructed 40x40 arena, a Broad Sword carrying the animals slay
from the slay table (race flag ANIMAL, the base multiplier of two)
wielded in the weapon slot, the birth torches wielded in the later
light slot, an adjacent awake white harpy (or small kobold in the
control). The measurement calls the public thrust valuation
(`borg_calculate_attack_effectiveness(BF_THRUST)`) with the slay trait
as the inventory scan computed it, then with the trait forced off and
on. This isolates the trait accumulation, not full autonomous play.
No death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
