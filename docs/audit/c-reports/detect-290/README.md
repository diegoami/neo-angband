# The detection that outlived its cave

The Borg updates on the second panel row of a large cave, queues the
self-reaction of a trap detection — the same call the spell casting path
uses — and then the level changes to a cave exactly one panel high: the
height where the old-offset reset, which compares strictly, lets equal
offsets through while the panel count yields a single row pair.

**The queued reaction still carries the old panel offsets, and its
four-cell mark is indexed with them after the update resized the
detection arrays for the new cave.** The mark reads its row pointer one
row past the outer array and writes through it: the recorded native run
of the original **crashes** (exit 139, matching the supplied harness
runs of this scenario), and the AddressSanitizer variant names the
out-of-bounds row-pointer read in `borg_handle_self()`
(borg-update.c:1396, reached from the update's message loop).

The correction drops the marks of reactions whose panel lies outside
the current arrays — in `borg_handle_self()` and in the trap-creation
reset inside `borg_update()` that indexes with the same old offsets.
The stale reaction is dropped and the small cave's fresh detection
stamps stay clear.

Control: when the reaction is queued *after* the level change, with the
current offsets, it marks the panel legitimately in both variants
(`last_row_mark=1`) and the complete log is byte-identical — the
correction drops only reactions whose panel lies outside the current
arrays.

Consequence: a crash of the whole game in the turn after a level change
following a self-detection. No death or survival claim is made.

Canonical report: [issue290](https://github.com/diegoami/neo-angband-tracker/issues/290).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/detect-290-evidence
git show bab03cb2204389ddad6ce017ae84614fdd39fc66:docs/audit/c-reports/detect-290/reproduce.py > /tmp/reproduce-detect-290.py
python3 /tmp/reproduce-detect-290.py --fixtures-ref bab03cb2204389ddad6ce017ae84614fdd39fc66 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720` in a normal and an AddressSanitizer build, verifies the
source diff is exactly the panel-bounds guards, runs two recorded
situations per variant per build, checks the crash/pass matrix, the
sanitizer report, the byte-identical control and both full gates, and
writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**A reaction queued for the previous, larger cave**: the original
crashes (the sanitizer names the row-pointer read past the arrays); the
correction drops the stale mark and completes.

Actual recorded output, original (`normal/before-stale.log`): the
process dies with `exit=139` (segmentation fault) after
`MODE stale`/`QUEUED`. The sanitizer variant (`asan/before-stale.log`):

```text
ERROR: AddressSanitizer: heap-buffer-overflow
    #0 ... in borg_handle_self src/borg/borg-update.c:1396
    #1 ... in borg_update src/borg/borg-update.c:1676
... is located 0 bytes after 16-byte region
```

Corrected (`normal/after-stale.log`): `GEOMETRY new_cave=28x28
panels=1,1 last_row_mark=0`, `Passed`. Control (`fresh`):
`last_row_mark=1` in both variants, byte-identical logs.

Full native gates (original and corrected, normal build,
`before-alltests.log` and `after-alltests.log`): **965/965 unit checks
and 5/5 frontend tests**, no relevant skips, checked from actual run
summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`02fb00af6abdb165fc1b445c42dc85d2263a3cdd`. One file,
`src/borg/borg-update.c`. `borg_handle_self()` computes the panel from
the old offsets `o_w_x`/`o_w_y` — saved at the end of the previous
update — and every detection branch writes the four cells
`[q_y..q_y+1][q_x..q_x+1]`. After a level change,
`borg_update()` resizes the arrays for the new cave
(`borg_alloc_detection()`, sized `panels + 1` per axis) and processes
the still-queued reaction with the stale offsets. The only guard,
`if (o_w_x > cave->width || o_w_y > cave->height)`, compares strictly:
offsets equal to the new cave's dimensions survive it while the derived
panel index already lands one row past the arrays. The correction
computes `detect_panel_ok` from the panel indices against the current
panel counts and guards every detection branch with it, and applies the
same bound to the trap-creation reset inside `borg_update()` that
indexes with the same old offsets.

```sh
git fetch --no-tags origin task/c-borg-detect-panel-bounds
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-update.c | sed -n '1316,1340p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 02fb00af6abdb165fc1b445c42dc85d2263a3cdd -- src/borg/borg-update.c
```

### Conditions and limits

Native Human Warrior level one, seed one thousand, a lit arena sized
from the panel geometry of an eighty-by-thirty term (panel height
twenty-eight): a large cave spanning several panel rows with the view
pushed to the second row, then a cave exactly one panel high. The
reaction is queued with the same call the spell casting path uses, the
level change replaces the cave as the game does, and the scenario reads
the resized arrays afterwards. The AddressSanitizer variant names the
out-of-bounds row-pointer read in `borg_handle_self`; the plain variant
records the crash. This isolates the queued-reaction bounds, not full
autonomous play. No death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs (with the sanitizer
logs). [Full receipt](receipt.json).
