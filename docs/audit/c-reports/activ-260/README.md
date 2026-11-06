# The activation that never counted

A mage wears a ring carrying the ACID_BOLT3 activation — the greater
acid bolt, twelve dice — and faces an adjacent kobold.

**The ACID_BOLT3 maneuver looks up the ACID_BOLT2 activation id, so the
Borg checks whether it carries the lesser bolt: an item with the
greater bolt is never considered, and the maneuver is valued at zero.**
The correction looks up the id the maneuver's damage and effect already
describe (twelve dice, `act_acid_bolt3`), and the ring is valued at 52 —
exactly like the correctly-wired lesser bolt in the control. The
borrowing direction was real too: with the lesser bolt worn, the
greater-bolt maneuver used to fire it at the greater bolt's damage.

The correction also fixes the companion token reported with it: the
DISPEL_ALL maneuver looked up the dispel-undead activation, so an item
with the all-monsters dispel was never considered either. Both ids are
properly initialized lookups; the swap is the same one-line shape. The
native demo below covers the bolt half; during development the dispel
half's original behavior was measured the same way (its item never
considered, zero), and the corrected dispel maneuver now reaches its
item — its valuation against this fixture's kobold runs into a
separate quirk of the dispel bolt-walk machinery (a small negative
return through the beam path) that is outside this fix's scope and not
asserted here.

Control: the correctly-wired lesser-bolt maneuver values its ring at 52
identically before and after, with byte-identical logs — the fix
changes only the miswired lookups, not the valuation machinery.

Reachability: in stock data these activations attach to randomly
generated artifacts, so the affected games are randart games; the
lookup defect itself is measured directly.

Canonical report: [issue260](https://github.com/diegoami/neo-angband-tracker/issues/260).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/activ-260-evidence
git show 460ca1570b2801fed38cc853e82599cd87e8acd2:docs/audit/c-reports/activ-260/reproduce.py > /tmp/reproduce-activ-260.py
python3 /tmp/reproduce-activ-260.py --fixtures-ref 460ca1570b2801fed38cc853e82599cd87e8acd2 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the two lookup ids, runs
two recorded situations per variant, checks the fail-before and
pass-after matrix and the byte-identical control, records both full
gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The greater acid bolt that never counted**: the original values the
maneuver at 0; the correction values it at 52.

Actual recorded output, original (`before-miswired.log`):

```text
RATING bolt3=0 bolt2=0
OUTCOME bolt3=0 bolt2=0
```

Corrected (`after-miswired.log`): `RATING bolt3=52 bolt2=0`. Control
(`control`): `RATING bolt2=52` in both variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`891d0cb9cc9057a2004dfeba7b21b19913f5a2b6`. Two tokens in
`src/borg/borg-fight-attack.c`. In
`borg_calculate_attack_effectiveness()`, the `BF_ACT_ACID_BOLT3` case
passes `act_acid_bolt2` to `borg_attack_aux_activation()` — whose
equips check (`borg_equips_item(activation, true)`) then looks for the
lesser bolt's id on the worn gear — and the `BF_ACT_DISPEL_ALL` case
passes `act_dispel_undead` while its damage and effect
(`BORG_ATTACK_DISP_ALL`) describe the all-monsters dispel. Both
looked-up ids exist and are initialized (`borg_findact("ACID_BOLT3")`,
`borg_findact("DISPEL_ALL")`); the cases simply pass the neighbours.

```sh
git fetch --no-tags origin task/c-borg-activation-lookups
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-attack.c | sed -n '4624,4640p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 891d0cb9cc9057a2004dfeba7b21b19913f5a2b6 -- src/borg/borg-fight-attack.c
```

### Conditions and limits

Native Human Mage level forty-one, depth thirty, seed one thousand,
lit constructed 40x40 arena, a worn ring whose activation is set to the
table's ACID_BOLT3 entry (or ACID_BOLT2 in the control), charged, and
an adjacent awake kobold. The measurement calls the public maneuver
chooser with `borg_simulate` set. In stock data these activations
attach to randomly generated artifacts, so the reachability is
randart-games; the lookup defect itself is measured directly. The
dispel token of the correction is the same one-line shape and is
verified by reading; its maneuver reaches its item after the
correction. This isolates the lookup ids, not full autonomous play. No
death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
