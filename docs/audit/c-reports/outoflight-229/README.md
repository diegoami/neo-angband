# A light about to burn out never started the out-of-light maneuvers

A Warrior at depth 30 wears a lantern with three turns of fuel left
and carries a fresh torch in the pack. The Borg keeps a routine for
running low on light — swap to a spare, refuel, recall, or take
stairs — and this is exactly the situation it exists for.

**The routine's entry test read "no light radius, AND (no light radius
or fuel within three turns)". Once the first clause holds, the second
is always true, so the about-to-burn-out clause never fired on its
own: the lit lantern three turns from dying was left to go dark with a
fresh torch in the pack.** Measured on the exact situation (seed 1000,
production `borg_think_dungeon_light()`): the original does nothing
(no keys). The corrected test — "no light, OR (fueled light within
three turns of dying)" — starts the maneuvers and swaps to the fresh
torch (keys `w e`).

The correction deliberately excludes fuel-less (everburning-style)
lights, whose timeout stays permanently at zero: without that guard
the new clause would false-trigger every turn on a healthy everburning
light and push the Borg into its recall and stairs maneuvers for no
reason. Three controls stay byte-identical: plenty of fuel (skip in
both variants), a fuel-less light (skip in both variants), and a fully
burned-out lantern (both versions run the same swap). No survival
claim beyond the routine starting is made.

Canonical report: [issue229](https://github.com/diegoami/neo-angband-tracker/issues/229).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal: PR (see the issue).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/outoflight-229-evidence
git show e1638569ea292812dd6633e3968b99f78c18c2f4:docs/audit/c-reports/outoflight-229/reproduce.py > /tmp/reproduce-outoflight-229.py
python3 /tmp/reproduce-outoflight-229.py --fixtures-ref e1638569ea292812dd6633e3968b99f78c18c2f4 --full-tests
```

The runner builds two variants (original, corrected) from pristine C
`e473720` in fresh temporary directories, verifies the source diff is
exactly the dying-light gate, runs four recorded situations on each,
compares controls and gates, and writes `replay.html` for offline
review. Your checkout and character/profile are not used or edited.
The disposable source and binary finish on the correction.

The native run recorded in this directory's logs executed at fixtures
pin `e1638569ea292812dd6633e3968b99f78c18c2f4`. The captured replay,
its data and this guide were then added on top; no generator change of
any kind followed the run (commit list: pin, then one artifact
commit).

Open [replay.html](replay.html) (download raw and open locally).
Choose **The lantern is lit but three turns from dying**, then **The
light** and **The routine**: the original does nothing, the corrected
starts the maneuvers. Choose the three controls: each matches byte for
byte between versions.

Actual recorded output, original (`before-dying.log`; the scenario
asserts the corrected start, so the original fails it — that is the
fail-before contract):

```text
SETUP light=3 timeout=3 nofuel=0 flasks=1
DECISION acted=0 keys=0 sequence=
native-c/outoflight-229 finished: 0/1 passed
```

Corrected (`after-dying.log`):

```text
DECISION acted=1 keys=2 sequence=w e
native-c/outoflight-229 finished: 1/1 passed
```

Controls (`before-lit.log` / `after-lit.log`,
`before-everburn.log` / `after-everburn.log`, `before-dead.log` /
`after-dead.log` — byte-identical pairs):

```text
DECISION acted=0 keys=0 sequence=          (lit: plenty of fuel, skip)
DECISION acted=0 keys=0 sequence=          (everburn: fuel-less, skip)
DECISION acted=1 keys=2 sequence=w e       (dead: same swap in both variants)
```

Full native gates on both variants: 965/965 unit checks and 5/5
frontend tests (`before-alltests.log`, `after-alltests.log`).

### Conditions and limits

Native Human Warrior, depth 30, seed 1000, glowing constructed arena,
a worn lantern whose timeout is set per situation (3 / 1500 / 0 with a
fuel-less flag / 0), a flask of oil and the starting torch in the
pack. The production out-of-light routine runs directly; its queued
keys are shown, not executed. This isolates the entry gate, not full
autonomous play.

### File inventory

- `scenario.c` — the native observer (four modes via `OL229_MODE`).
- `run-cases.py` — runs each mode, strips ANSI, writes per-mode logs
  and `*-results.json`.
- `make_replay.py` — builds `replay.html` + `replay-data.json`.
- `reproduce.py` — the pinned runner described above.
- `borg.txt` — Borg config for the disposable profile.
- `before-dying.log`, `after-dying.log`, `before-lit.log`,
  `after-lit.log`, `before-everburn.log`, `after-everburn.log`,
  `before-dead.log`, `after-dead.log` — captured scenario logs (the
  control pairs are byte-identical).
- `before-results.json`, `after-results.json` — parsed per-mode rows.
- `configure.log`, `before-build.log`, `after-build.log`,
  `before-scenarios.log`, `after-scenarios.log`,
  `before-alltests.log`, `after-alltests.log` — run-root logs.
- `result.json` — pins, per-case results, identical controls, gates,
  fixed-source sha256.
- `replay.html`, `replay-data.json` — the offline demo.
- `receipt.json` — sha256 map of every file in this directory.
