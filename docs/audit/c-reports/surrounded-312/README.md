# Surrounded by nobody

The "am I about to be surrounded?" check counted some of the Borg's
neighbours twice — and the verdict can fire with no monsters nearby.

**`borg_surrounded()` tested the trap flag with a separate `if` after
the wall/unknown/monster/shop `else if` chain, so a wall that also
carried the trap flag counted as two non-safe grids. Enough
double-counted neighbours push the safe-square count below zero, and
"monsters > safe squares" then fires with zero monsters.** Measured
natively through the production verdict with the report's board —
three trapped walls, three trap-only floors, no monsters: the original
answers surrounded; the correction makes the trap test the fifth
branch of the chain and answers not surrounded.

Control: eight plain walls with no traps never touch the trap test —
not surrounded in both variants, byte-identical logs.

Consequence: the phantom verdict makes the Borg abandon a fight it is
not in, or flee the level. The measured fact is the wrong verdict; the
live in-game abandonment is the report's mechanism, not claimed here.
No death or survival claim.

Canonical report: [issue312](https://github.com/diegoami/neo-angband-tracker/issues/312).
Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/surrounded-312-evidence
git show PINSHA:docs/audit/c-reports/surrounded-312/reproduce.py > /tmp/reproduce-surrounded-312.py
python3 /tmp/reproduce-surrounded-312.py --fixtures-ref PINSHA --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the chained trap test,
runs the trap-board and plain-wall situations per variant through the
production surrounded check, checks the fail-before and pass-after
matrix and the byte-identical control, records both full gates and
writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**Surrounded by nobody**: with zero monsters nearby the original
answers surrounded; the correction answers no.

Actual recorded output, original (`before-traps.log`):

```text
GEOMETRY mode=traps kills_cnt=0
RATING surrounded=1
```

Corrected (`after-traps.log`): `RATING surrounded=0`. Control
(`walls`): `RATING surrounded=0` in both variants, byte-identical
logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`d366ef4e75e40f17fb293135c05fa74524f2e184`. One keyword in
`src/borg/borg-escape.c`: the trap test becomes the fifth `else if` of
the non-safe-grid chain, so each non-safe neighbour counts once.

```sh
git fetch --no-tags origin task/c-borg-surrounded-elseif
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-escape.c | sed -n '206,237p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 d366ef4e75e40f17fb293135c05fa74524f2e184 -- src/borg/borg-escape.c
```

### Conditions and limits

Native Human Warrior on an empty arena at depth 30, seed one thousand,
with the eight neighbouring grids arranged as the report describes;
the production `borg_surrounded()` verdict is read directly. No death
or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
