# A stale Teleport Other entry erased an adjacent giant's danger

A Warrior stands next to an awake hill giant (measured danger 179 at
one turn) while a Teleport Other evaluation is under way. The Borg
keeps a list of monsters it plans to banish so their danger is not
counted twice; this time the list holds one real entry that is not the
giant, plus the leftover a longer earlier evaluation wrote one past
the count — and that leftover happens to name the giant.

**The exclusion loop in `borg_danger_one_kill()` ran to
`ii <= borg_tp_other_n`, reading the slot one past the entries the
count describes. The giant's index in that stale slot matched, and the
whole danger check returned 0: an adjacent giant was rated harmless
while the Borg scored whether to teleport away.** Measured on the
exact situation (seed 1000, production `borg_danger_one_kill()`): the
original scores danger 0 for the adjacent giant; the corrected loop
reads only the real entries and scores 179.

Two controls stay byte-identical: with the giant genuinely in the
list, both variants exclude its danger (that exclusion is correct and
is kept), and with an empty list both count 179. The writer side of
the same list (appending past its 255 entries) is the other half of
issue 250 and is handled separately; this fix is the reader slip (C
bug 17 of the borg-rs import). No damage or survival claim beyond
this one score is made.

Canonical report: [issue250](https://github.com/diegoami/neo-angband-tracker/issues/250).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal: PR (see the issue).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/tpreader-250-evidence
git show 6f6710ccc3a3dc1b035544338444048b2f7cbced:docs/audit/c-reports/tpreader-250/reproduce.py > /tmp/reproduce-tpreader-250.py
python3 /tmp/reproduce-tpreader-250.py --fixtures-ref 6f6710ccc3a3dc1b035544338444048b2f7cbced --full-tests
```

The runner builds two variants (original, corrected) from pristine C
`e473720` in fresh temporary directories, verifies the source diff is
exactly the bounded exclusion loop, runs three recorded situations on
each, compares controls and gates, and writes `replay.html` for
offline review. Your checkout and character/profile are not used or
edited. The disposable source and binary finish on the correction.

The native run recorded in this directory's logs executed at fixtures
pin `6f6710ccc3a3dc1b035544338444048b2f7cbced`. The captured replay,
its data and this guide were then added on top; no generator change of
any kind followed the run (commit list: pin, then one artifact
commit).

Open [replay.html](replay.html) (download raw and open locally).
Choose **A stale entry one past the count names the giant**, then
**The list** and **The danger score**: the original rates the giant
harmless, the corrected counts 179. Choose **Control: the giant
genuinely is in the list** and **Control: empty list**: both logs
match byte for byte between versions.

Actual recorded output, original (`before-stale-hit.log`; the
scenario asserts the counted danger, so the original fails it — that
is the fail-before contract):

```text
SETUP n=1 idx0=0 idx1=1
DANGER danger=0
native-c/tpreader-250 finished: 0/1 passed
```

Corrected (`after-stale-hit.log`):

```text
DANGER danger=179
native-c/tpreader-250 finished: 1/1 passed
```

Controls (`before-in-list.log` / `after-in-list.log`,
`before-empty.log` / `after-empty.log` — byte-identical pairs):

```text
DANGER danger=0     (in-list: the genuine exclusion is kept)
DANGER danger=179   (empty: no exclusions, full danger)
```

Full native gates on both variants: 965/965 unit checks and 5/5
frontend tests (`before-alltests.log`, `after-alltests.log`).

### Conditions and limits

Native Human Warrior, seed 1000, glowing constructed arena, lit
torch, an awake hill giant adjacent to the Borg. The exclusion list
is set directly in the state a longer earlier Teleport Other
evaluation leaves behind (one real entry, one stale leftover past the
count) — recorded as a fixture intervention; the production
per-monster danger routine then runs unmodified. This isolates the
stale-read, not full autonomous play.

### File inventory

- `scenario.c` — the native observer (three modes via `TP250_MODE`).
- `run-cases.py` — runs each mode, strips ANSI, writes per-mode logs
  and `*-results.json`.
- `make_replay.py` — builds `replay.html` + `replay-data.json`.
- `reproduce.py` — the pinned runner described above.
- `borg.txt` — Borg config for the disposable profile.
- `before-stale-hit.log`, `after-stale-hit.log`,
  `before-in-list.log`, `after-in-list.log`, `before-empty.log`,
  `after-empty.log` — captured scenario logs (the control pairs are
  byte-identical).
- `before-results.json`, `after-results.json` — parsed per-mode rows.
- `configure.log`, `before-build.log`, `after-build.log`,
  `before-scenarios.log`, `after-scenarios.log`,
  `before-alltests.log`, `after-alltests.log` — run-root logs.
- `result.json` — pins, per-case results, identical controls, gates,
  fixed-source sha256.
- `replay.html`, `replay-data.json` — the offline demo.
- `receipt.json` — sha256 map of every file in this directory.
