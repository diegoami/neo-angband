# The staircase he kept discovering

The "do I already know this up staircase?" scan never stopped at a
match — so a known staircase under the Borg was tracked again.

**`borg_think_stair_scum()`'s up-staircase scan used `continue` where
the matching down-staircase scan uses `break`: a match no longer
stopped the loop, the scan always ended at the end of the array, and
the staircase under the Borg — already tracked, for example from being
seen on the map earlier — was appended to `track_less` again on every
visit.** Measured natively through the production discovery path with
the staircase pre-tracked: the original grows the array from 1 to 2 (a
duplicate per visit, until the array fills and later staircases cannot
be tracked); the correction breaks on the match and leaves it at 1.

Control: a fresh, never-tracked staircase is added exactly once in
both variants (0 to 1), byte-identical logs.

Consequence: `track_less` fills with duplicate entries for the Borg's
own staircase; once full, newly seen up staircases can no longer be
tracked. The measured fact is the array growth; no death or survival
claim.

Canonical report: [issue232](https://github.com/diegoami/neo-angband-tracker/issues/232).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/stairtrack-232-evidence
git show PINSHA:docs/audit/c-reports/stairtrack-232/reproduce.py > /tmp/reproduce-stairtrack-232.py
python3 /tmp/reproduce-stairtrack-232.py --fixtures-ref PINSHA --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the break-on-match,
runs the known-stair and fresh-discovery situations per variant
through the production stair-scum path, checks the fail-before and
pass-after matrix and the byte-identical control, records both full
gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The staircase he kept discovering**: the original re-adds the known
staircase; the correction leaves the array alone.

Actual recorded output, original (`before-known.log`):

```text
GEOMETRY before=1 after=2
RATING before=1 after=2
```

Corrected (`after-known.log`): `RATING before=1 after=1`. Control
(`fresh`): `RATING before=0 after=1` in both variants, byte-identical
logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`8310e9cc9fe90a82f93609f542d5f057b835fbcf`. One keyword in
`src/borg/borg-think-dungeon-util.c`: the up-staircase "already known"
scan breaks on a match, like the down-staircase scan above it.

```sh
git fetch --no-tags origin task/c-borg-stair-track-break
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-think-dungeon-util.c | sed -n '395,412p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 8310e9cc9fe90a82f93609f542d5f057b835fbcf -- src/borg/borg-think-dungeon-util.c
```

### Conditions and limits

Native Human Warrior at depth 2, seed one thousand, standing on a real
up staircase while the map belief is still plain floor (the discovery
moment); the production stair-scum path runs in full. The measured
fact is the tracking array growth. No death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
