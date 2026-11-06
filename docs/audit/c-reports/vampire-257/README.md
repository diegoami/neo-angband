# Vampire Strike was judged by a bystander's distance

A Necromancer with Vampire Strike faces a kobold five grids away while
a hill giant stands twenty grids away. The Borg's strike routine scans
its target list for the closest monster and picks the kobold — but its
"require relatively close" test afterwards reads a distance variable
that the scan leaves holding the last entry it examined, including
entries it skipped.

**One distant bystander anywhere in the target list kept the spell
from ever being cast, even with a perfect target five grids away.**
Since the chosen entry is always the minimum distance, the mistaken
test only ever over-restricted the spell. The correction judges the
chosen monster's distance; the strike is cast (the chooser queues the
real spell keys).

Measured transition: with the kobold at five and the giant at twenty,
the original attack chooser does nothing (acted 0, no keys); the
corrected chooser casts Vampire Strike ('m' plus book and spell keys).
Two situations stay byte-identical before and after: with both
monsters close, both versions cast; with only the distant monster,
both refuse — the chosen distance really is twenty.

Consequence: a working attack spell went unused in exactly the
situations it is made for. No damage, death or survival claim beyond
the recorded decision.

Canonical report: [issue257](https://github.com/diegoami/neo-angband-tracker/issues/257).
Lead supplied by borg-rs; independently validated and reproduced
natively here through the full production attack chooser. Source
proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/vampire-257-evidence
git show 545213726d6d0156ea1cea931b212a9895c5b77b:docs/audit/c-reports/vampire-257/reproduce.py > /tmp/reproduce-vampire-257.py
python3 /tmp/reproduce-vampire-257.py --fixtures-ref 545213726d6d0156ea1cea931b212a9895c5b77b --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the one-token guard,
runs three recorded situations per variant, checks the fail-before/
pass-after matrix and byte-identical controls, records both full gates
and writes an offline `replay.html`. The native verification ran with
these generators except the replay generator's syntax and wording,
completed for the captured package; no native log, source variant or
gate differs.

Open [replay.html](replay.html) (download raw and open locally). Choose
**A close target and a distant bystander**, then **The target list** and
**The decision**. The original refuses the cast; the correction casts.
The other two situations are controls whose logs match byte for byte.

Actual recorded output, original (`before-near-far.log`):

```text
GEOMETRY near=5 far=20 max_range=20
DECISION acted=0 keys=0 sequence=
OUTCOME acted=0 keys=0 cast_spell=0
```

Corrected (`after-near-far.log`): `DECISION acted=1 keys=3
sequence=mbb`, `OUTCOME acted=1 keys=3 cast_spell=1`.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`0119d5daee9b8a07a8096331eea78dbbb6c26050`. One token in
`src/borg/borg-fight-attack.c`. The closest-target scan assigns `dist`
for every entry before deciding to skip it, so after the loop `dist`
holds the distance of the last entry examined — not the chosen one
(`b_i`, whose distance is tracked in `best_dist`). The guard
`if (dist >= 20) return 0;` therefore tested an arbitrary bystander.
The correction tests `best_dist`, the chosen monster's distance.

```sh
git fetch --no-tags origin task/c-borg-vampire-strike-distance
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-attack.c | sed -n '2331,2350p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 0119d5daee9b8a07a8096331eea78dbbb6c26050 -- src/borg/borg-fight-attack.c
```

### Conditions and limits

Native Human Necromancer level 26, depth 30, seed 1000, lit
constructed 40x40 arena, the Fear and Torment shadow book, Vampire
Strike learned as the only spell, full mana, a torch. The production
attack chooser acts on its own filled target list; the cast is shown
by its queued keys, not executed. The bystander sits at distance
twenty, the closeness limit itself, within the spell's range. This
isolates the closeness guard, not full autonomous play. No damage or
survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
