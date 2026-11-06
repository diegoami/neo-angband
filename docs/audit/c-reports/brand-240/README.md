# The brand need that counted twice

A level fifty druid wields a long bow — shooter power three — wears a
firebrand ring, and carries five identified plain arrows.

**The brand-the-ammo test was pasted twice into the inventory scan:
once inside the good-shooter block and once after it, with the same
condition. Every qualifying stack therefore added twenty to the
need-brand trait instead of ten.** Measured: the single stack yields
`need_brand=20` in the original and `10` in the correction.

The correction removes the duplicate, keeping the copy that sits with
the enchant logic inside the good-shooter block.

Control: a stack of two arrows — below the five the test requires —
leaves the need at zero in both variants with byte-identical logs.

Consequence: the Borg's brand-need trait ran double wherever it was
consumed. The shipped configuration formulas do not reference this
trait, so the measured impact is the trait itself and whatever custom
configurations make of it. No death or survival claim is made.

Canonical report: [issue240](https://github.com/diegoami/neo-angband-tracker/issues/240).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/brand-240-evidence
git show PINSHA:docs/audit/c-reports/brand-240/reproduce.py > /tmp/reproduce-brand-240.py
python3 /tmp/reproduce-brand-240.py --fixtures-ref PINSHA --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the removed duplicate
test, runs two recorded situations per variant, checks the fail-before
and pass-after matrix and the byte-identical control, records both
full gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally).
Choose **The brand need that counted twice**: the original counts the
single stack at 20; the correction at 10.

Actual recorded output, original (`before-brand.log`):

```text
SETUP seed=1000 class=Druid level=50 ammo_power=3 ammo_tval=3 brand_legal=1
RATING need_brand=20
OUTCOME need_brand=20
```

Corrected (`after-brand.log`): `RATING need_brand=10`. Control
(`control`): `RATING need_brand=0` in both variants, byte-identical
logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`b30afa1a67436d59b070e3509a7e92b932d01f75`. Fifteen deleted lines in
`src/borg/borg-trait.c`. In `borg_notice_ammo()`, the
brand-the-ammo test (firebrand or the brand spell, five or more shots,
no ego or artifact, the shooter's ammo type) appears once inside the
`BI_AMMO_POWER >= 3` block and once more after the `< 3` return, with
identical conditions; both add ten to `BI_NEED_BRAND_WEAPON`. The
correction deletes the second copy.

```sh
git fetch --no-tags origin task/c-borg-brand-need-single
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-trait.c | sed -n '1155,1210p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 b30afa1a67436d59b070e3509a7e92b932d01f75 -- src/borg/borg-trait.c
```

### Conditions and limits

Native Human Druid level fifty, depth thirty, seed one thousand, lit
constructed 40x40 arena, every class book carried, a long bow wielded
(shooter power three), a firebrand ring worn as the brand ability,
five (or two, in the control) identified plain arrows. The measurement
reads the need-brand trait after one inventory scan. This isolates the
duplicated test, not full autonomous play. No death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
