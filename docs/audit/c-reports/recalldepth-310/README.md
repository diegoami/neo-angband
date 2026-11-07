# The endgame recall answer contradicted its own comment

A Warrior hunting the last uniques before Morgoth recalls from depth 65
toward the unique it is hunting there. Its deepest level reached is 99
and fewer than five uniques are still alive. On a Word of Recall read
inside the dungeon the game asks "Set recall depth to current depth?"
— and how the Borg answers decides where later recalls land for the
Morgoth push.

**The branch deciding this carries the comment "Do reset Depth,
cleaning up last uniques to hit Morgoth" — and then answers 'n' with
the note "# Not Resetting recall depth."** Measured on the exact
situation (seed 1000, production `borg_recall()`): the original queues
`<Escape> <Escape> r d n <Escape>` — the 'n' answers against its own
comment, so later recalls keep landing wherever the depth was last set
instead of at the depth where the remaining uniques are hunted. The
corrected branch answers 'y' (`<Escape> <Escape> r d y <Escape>`) and
notes "# Resetting recall depth.", as the comment describes; the stale
half-comment about keeping the potion stock — the rationale that came
in with the changed key — is removed.

Two controls keep their answers and stay byte-identical: the generic
reset branch (deepest level 60: answers 'y' in both variants) and the
hunted-depth mismatch branch (answers 'n' in both variants). Owner
intent recorded on the issue 2026-11-07: reset depth (key 'y'). No
survival or progress claim beyond the queued answer is made.

Canonical report: [issue310](https://github.com/diegoami/neo-angband-tracker/issues/310).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal: PR (see the issue).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/recalldepth-310-evidence
git show 9ccd191c2601d30285d2e9983f5eabdfd1cbf165:docs/audit/c-reports/recalldepth-310/reproduce.py > /tmp/reproduce-recalldepth-310.py
python3 /tmp/reproduce-recalldepth-310.py --fixtures-ref 9ccd191c2601d30285d2e9983f5eabdfd1cbf165 --full-tests
```

The runner builds two variants (original, corrected) from pristine C
`e473720` in fresh temporary directories, verifies the source diff is
exactly the endgame answer change, runs three recorded situations on
each, compares controls and gates, and writes `replay.html` for
offline review. Your checkout and character/profile are not used or
edited. The disposable source and binary finish on the correction.

The native run recorded in this directory's logs executed at fixtures
pin `9ccd191c2601d30285d2e9983f5eabdfd1cbf165`. The captured replay,
its data and this guide were then added on top; no generator change of
any kind followed the run (commit list: pin, then one artifact commit;
the earlier pin `bbb95ca4860…` differed only by a results-field the
replay needed and was re-run in full before any capture).

Open [replay.html](replay.html) (download raw and open locally).
Choose **Endgame: last uniques, recalling from depth 65**, then **The
situation** and **The answer**: the original answers n, the corrected
answers y. Choose **Control: generic reset branch** and **Control:
hunted unique far from here**: both logs match byte for byte between
versions.

Actual recorded output, original (`before-endgame.log`; the scenario
asserts the corrected answer, so the original fails it — that is the
fail-before contract):

```text
SETUP maxdepth=99 curdepth=65 hunted=65 live_uniques=2
DECISION acted=1 keys=6 sequence=<Escape> <Escape> r d n <Escape>
OUTCOME acted=1 keys=6 reset_y=0 keep_n=1
native-c/recalldepth-310 finished: 0/1 passed
```

Corrected (`after-endgame.log`):

```text
DECISION acted=1 keys=6 sequence=<Escape> <Escape> r d y <Escape>
OUTCOME acted=1 keys=6 reset_y=1 keep_n=0
native-c/recalldepth-310 finished: 1/1 passed
```

Controls (`before-generic.log` / `after-generic.log`,
`before-mismatch.log` / `after-mismatch.log` — byte-identical pairs):

```text
DECISION acted=1 keys=6 sequence=<Escape> <Escape> r d y <Escape>   (generic: y in both variants)
DECISION acted=1 keys=6 sequence=<Escape> <Escape> r d n <Escape>   (mismatch: n in both variants)
```

Key reading: the two `<Escape>` and `r d` are the scroll read, the
fifth key is the game's prompt answer (`y` = set recall depth to the
current depth, `n` = leave it), and the final `<Escape>` closes any
lingering prompt.

Full native gates on both variants: 965/965 unit checks and 5/5
frontend tests (`before-alltests.log`, `after-alltests.log`).

### Conditions and limits

Native Human Warrior, seed 1000, glowing constructed arena, a known
Word of Recall scroll in the pack, full hp. The endgame shape is set
directly on the borg's declared traits (deepest level 99, current 65)
and unique-tracking state (hunted depth 65, two live uniques), the
same values the branch tests; the production recall attempt then runs
unmodified and its keys are shown, not executed in the game. This
isolates the endgame branch answer, not full autonomous play.

### File inventory

- `scenario.c` — the native observer (three modes via `RD310_MODE`).
- `run-cases.py` — runs each mode, strips ANSI, writes per-mode logs
  and `*-results.json`.
- `make_replay.py` — builds `replay.html` + `replay-data.json`.
- `reproduce.py` — the pinned runner described above.
- `borg.txt` — Borg config for the disposable profile.
- `before-endgame.log`, `after-endgame.log`, `before-generic.log`,
  `after-generic.log`, `before-mismatch.log`, `after-mismatch.log` —
  captured scenario logs (the control pairs are byte-identical).
- `before-results.json`, `after-results.json` — parsed per-mode rows.
- `configure.log`, `before-build.log`, `after-build.log`,
  `before-scenarios.log`, `after-scenarios.log`,
  `before-alltests.log`, `after-alltests.log` — run-root logs.
- `result.json` — pins, per-case results, identical controls, gates,
  fixed-source sha256.
- `replay.html`, `replay-data.json` — the offline demo.
- `receipt.json` — sha256 map of every file in this directory.
