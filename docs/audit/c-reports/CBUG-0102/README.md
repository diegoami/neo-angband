# A failed rune creation drives the glyph tracker below zero

A dying level-1 Warrior stands on an open doorway with a mage summoner
beside it. To survive, the Borg reads an unknown scroll from its pack —
a legal emergency identify-by-use action. The scroll is a Rune of
Protection, and a doorway cannot hold a rune: the game reports
"There is no clear floor on which to cast the spell."

**The Borg's handler for that message removes a glyph that was never
tracked. The count goes to -1, and the next commanded rune creation
stores its position four bytes in front of the two tracking arrays.**
Under AddressSanitizer the original aborts at that creation with a
heap-buffer-overflow WRITE of size 4 at `borg-fight-defend.c:1607`
(`borg_defend_aux_glyph`), on the array allocated by `borg_init_track`
via `borg_init_flow_glyph`. The corrected build runs the same scenario
cleanly and records the creation at index 0.

Measured transition: original counts -1 after the message and 0 after
the creation (the index -1 write); corrected counts 0 and 1. Two
controls stay byte-identical before and after the correction: the same
emergency read on ordinary floor (the rune is created, no failure
message exists) and an ordinary tracked creation from a known scroll
with no emergency at all.

With the tracker list full (200 entries) the same handler also wrote
one entry past the end of each array; the guard removes that write too.
While the count is negative, glyphs seen on screen silently fail to
track (the `i == num` test can no longer match). Consequence measured:
an out-of-bounds heap write (memory corruption) with a sanitizer abort,
plus broken glyph tracking until the count is repaired. No crash in
every configuration, death or combat-outcome claim is made.

Canonical report: [issue118 / CBUG-0102](https://github.com/diegoami/neo-angband-tracker/issues/118).
Lead supplied by borg-rs (docs/c-bugs.md section 29); independently
validated and reproduced natively here. Duplicate report
[issue195](https://github.com/diegoami/neo-angband-tracker/issues/195)
(same handler, full-list overflow and negative count) is closed in its
favor. Source proposal [PR283](https://github.com/diegoami/neo-angband-tracker/pull/283).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/cbug-0102-evidence
git show f1f53106526412cfe44fa01c73d00e7dbf4dbeed:docs/audit/c-reports/CBUG-0102/reproduce.py > /tmp/reproduce-cbug-0102.py
python3 /tmp/reproduce-cbug-0102.py --fixtures-ref f1f53106526412cfe44fa01c73d00e7dbf4dbeed --full-tests
```

The runner builds two normal variants (original, corrected) and two
AddressSanitizer variants from pristine C `e473720` in fresh temporary
directories, verifies the source diff is exactly the guarded removal,
runs three recorded situations on each, compares controls and gates,
and writes `replay.html` for offline review. Your checkout and
character/profile are not used or edited. The disposable source and
binary finish on the correction.

The native run recorded in this directory's logs executed at fixtures
pin `f1f53106526412cfe44fa01c73d00e7dbf4dbeed`. The captured replay,
its data and this guide were then added on top; the only later change
to the generators is a replay-side text fix (a removed
`grid_feat_marked` field rendered as `undefined`); no native log,
source variant or gate differs between the pin and this head.

Open [replay.html](replay.html) (download raw and open locally). Choose
**Emergency unknown scroll read on a doorway**, then **Emergency read**,
**Tracker after the message** and **Next glyph creation**. The original
records tracker count -1 after the real failure message and 0 after the
next creation; the sanitizer build aborts that creation with the
out-of-bounds write. The corrected records 0 and 1 and wards the native
grid. Choose **Control: the same read on open floor** and **Control:
ordinary tracked creation**: both logs match byte for byte between
versions.

Actual recorded output, original (`before-stairs.log`):

```text
DECISION phaseA acted=1 keys=3 sequence=rc<Escape>
ENGINE turn=1 hp=8: There is no clear floor on which to cast the spell.
TRACK phaseA num=-1
DANGER phaseB p1=154 p2=88 avoidance=180
DECISION phaseB acted=1 keys=4 sequence=<Escape><Escape>rd
TRACK phaseB_acted num=0
OUTCOME glyph_created=1 warded_grid=(10,10) hp=8 num_final=0
```

Corrected (`after-stairs.log`): `TRACK phaseA num=0`,
`TRACK phaseB_acted num=1`, `num_final=1`; every other decision/outcome
line identical.

ASAN original (`asan-before-stairs.log`): `heap-buffer-overflow WRITE
of size 4`, `borg_defend_aux_glyph` at `borg-fight-defend.c:1607`,
allocation stack `borg_init_track`/`borg_init_flow_glyph`. Corrected
asan build: 1/1 passed, no sanitizer report. Scenario runs disable the
strcmp interceptor and leak check: the pristine base otherwise reports
an unrelated strcmp overread while sorting monster names during init
(`borg-util.c`); the tracked defect is a compiler-instrumented write
and needs no interceptors. That overread is recorded as an observation,
not a claim.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`ec172531ec38fdd3ac2783d21e3faf6dc1e2b3f3` (PR283). One guarded block
in `src/borg/borg-messages.c`. The failed-glyph handler wrote through
the current count and then decremented: with nothing tracked the count
became -1 and the next creation in `borg_defend_aux_glyph()` or
`borg_flow_glyph()` passed `count < size` and wrote at index -1; with
the list full the same write landed past the end. The correction
decrements only when an entry exists and clears the actually-tracked
entry, keeping every write inside the arrays.

Legal path: `borg-caution.c`'s unknown-item emergency (danger 154 >
four times 8 HP, level 1 < 20, no spells) → `borg_read_unknown()` →
real native scroll read on a non-trappable grid (open doorway) →
`effect_handler_GLYPH`'s message → the real terminal-scrape parse. The
defense chooser then commands a rune creation on clear floor with a
known scroll. "The object resists the spell" stays in the match though
current C no longer emits it.

### Conditions and limits

Native Human Warrior level 1, depth 5, seed 1000, wounded to 8 HP, lit
constructed 40x40 arena, one awake mage (depth 28, bolts plus
S_MONSTER), torch with fuel, one unidentified Rune of Protection as the
last unknown scroll. The starting Word of Recall scroll is dropped
(fixture narrative: used to reach the dungeon); the mid-scenario step
off the doorway and the grant of a second, now-identified rune scroll
are fixture interventions (flavor awareness is per kind). This isolates
the count transition and the guarded write, not full autonomous play,
monster AI casting or world timing. Danger figures are the Borg's
heuristic, not HP predictions. No death, frequency or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs (normal and asan
variants). [Full receipt](receipt.json).
