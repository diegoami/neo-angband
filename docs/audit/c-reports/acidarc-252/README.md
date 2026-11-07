# Ball and arc blasts never counted their south and east edges

A level-50 Mage with Acid Spray — a player-centred acid cone reaching
ten grids — faces an awake hill giant standing exactly ten grids south,
straight in the cone, with nothing else alive in sight. Acid Spray is
the only live attack: the pack carries one book and 20 mana blocks its
Frost Bolt (cost 40) while allowing Acid Spray (cost 5).

**The Borg's blast loops ran from centre minus radius up to but
excluding centre plus radius — `ry < y2 + rad`, likewise `rx`, and
`y1 < y1 + max` for the player-centred arcs — so grids exactly radius
grids south or east never entered the scan.** The giant on that edge
contributed nothing: measured on the exact situation (seed 1000,
production `borg_attack(false)` chooser), the original finds nothing
worth casting and queues no keys. The corrected loops are inclusive
(the existing `r > rad` distance check already keeps grids beyond the
radius out) and the same situation sprays the giant — keys
`* p 2 2 2 2 2 2 2 2 2 2 5 m b f 5`, the target cursor walked ten
grids south and the Acid Spray cast queued.

The same two loops score every ball and arc evaluation in play, so any
monster resting on the south or east rim of a blast was invisible to
the Borg's targeting and to its value comparisons between attacks.
Two controls stay byte-identical: the giant eight grids south (inside
the loop either way — both versions cast with identical targeting) and
an empty arena (both refuse). No damage or survival claim beyond this
chooser rating is made.

Canonical report: [issue252](https://github.com/diegoami/neo-angband-tracker/issues/252).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal: PR (see the issue).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/acidarc-252-evidence
git show e40c642a0a7906d8d9caadb803ed6a5adb70bbc5:docs/audit/c-reports/acidarc-252/reproduce.py > /tmp/reproduce-acidarc-252.py
python3 /tmp/reproduce-acidarc-252.py --fixtures-ref e40c642a0a7906d8d9caadb803ed6a5adb70bbc5 --full-tests
```

The runner builds two variants (original, corrected) from pristine C
`e473720` in fresh temporary directories, verifies the source diff is
exactly the two inclusive blast loops, runs three recorded situations
on each, compares controls and gates, and writes `replay.html` for
offline review. Your checkout and character/profile are not used or
edited. The disposable source and binary finish on the correction.

The native run recorded in this directory's logs executed at fixtures
pin `e40c642a0a7906d8d9caadb803ed6a5adb70bbc5`. The captured replay,
its data and this guide were then added on top; no generator change of
any kind followed the run (commit list: pin, then one artifact
commit).

Open [replay.html](replay.html) (download raw and open locally).
Choose **The giant stands exactly ten grids south**, then **The
situation** and **The decision**: the original refuses, the corrected
sprays. Choose **Control: the giant eight grids south** and
**Control: nothing in sight**: both logs match byte for byte between
versions.

Actual recorded output, original (`before-edge.log`; the scenario
asserts the corrected cast, so the original fails it — that is the
fail-before contract):

```text
SETUP maxsp=20 csp=20 failacid=2 poweracid=5 dist=10
DECISION acted=0 keys=0 sequence=
native-c/acidarc-252 finished: 0/1 passed
```

Corrected (`after-edge.log`):

```text
DECISION acted=1 keys=17 sequence=* p 2 2 2 2 2 2 2 2 2 2 5 m b f 5
native-c/acidarc-252 finished: 1/1 passed
```

Controls (`before-near.log` / `after-near.log`, `before-empty.log` /
`after-empty.log` — byte-identical pairs):

```text
DECISION acted=1 keys=15 sequence=* p 2 2 2 2 2 2 2 2 5 m b f 5   (near: giant at eight grids)
DECISION acted=0 keys=0 sequence=                                 (empty: nothing to do)
```

Key reading: `* p` opens target mode, the `2` run walks the target
cursor south (eight for the near control, ten to reach the giant on
the edge), `5` selects, and `m b f` is the cast command, the Attacks
and Knowledge book (pack slot b) and the Acid Spray letter; the final
`5` confirms the cast direction.

Full native gates on both variants: 965/965 unit checks and 5/5
frontend tests (`before-alltests.log`, `after-alltests.log`).

### Conditions and limits

Native Human Mage level 50, depth 30, seed 1000, glowing constructed
arena, the Attacks and Knowledge book carried, Acid Spray learned,
declared mana 20/20 — enough for Acid Spray (cost 5, the chooser's
only live attack) and below Frost Bolt (cost 40), which keeps the
comparison single-attack; the chooser's 2% simulation paranoia roll
was checked across fifteen seeds and none hits at this call position.
The production attack chooser acts on its own filled target list; the
cast is shown by its queued keys, not executed. This isolates the
blast-radius scan, not full autonomous play.

### File inventory

- `scenario.c` — the native observer (three modes via `AC252_MODE`).
- `run-cases.py` — runs each mode, strips ANSI, writes per-mode logs
  and `*-results.json`.
- `make_replay.py` — builds `replay.html` + `replay-data.json`.
- `reproduce.py` — the pinned runner described above.
- `borg.txt` — Borg config for the disposable profile.
- `before-edge.log`, `after-edge.log`, `before-near.log`,
  `after-near.log`, `before-empty.log`, `after-empty.log` — captured
  scenario logs (the control pairs are byte-identical).
- `before-results.json`, `after-results.json` — parsed per-mode rows.
- `configure.log`, `before-build.log`, `after-build.log`,
  `before-scenarios.log`, `after-scenarios.log`,
  `before-alltests.log`, `after-alltests.log` — run-root logs.
- `result.json` — pins, per-case results, identical controls, gates,
  fixed-source sha256.
- `replay.html`, `replay-data.json` — the offline demo.
- `receipt.json` — sha256 map of every file in this directory.
