# The staircase the Borg was not standing on

A young Borg in munchkin mode stands on plain floor and remembers a down
staircase one diagonal step to the north-west — the grid the
adjacent-monster scan examines last. A second, seen stair is sealed
inside a permanent-wall pocket so every stair flow fails and the
decision reaches the take-the-stair tests.

**The adjacent-monster scan reuses `ag`, the pointer to the Borg's own
grid, for each neighbour, so after the scan it points at the last
in-bounds neighbour. The take-the-stair tests that follow read that
neighbour's feature: the Borg queues the take-stair key while standing
on plain floor.** In the game that keypress fails without taking a
turn, so the Borg can repeat the same decision indefinitely.

The correction re-acquires the Borg's own grid after the scan; the
phantom keypress disappears and the dungeon brain moves on to its next
goal.

Control: standing on a real, seen down staircase, both versions queue
the take-stair key and the complete log is byte-identical before and
after — legitimate stair-taking is handled before the scan (the
power-diving section) and is untouched by the correction. This also
documents why the missed-stair direction of the report is shadowed in
practice: a Borg standing on a staircase is served by the earlier
sections, which read the pointer before the scan clobbers it.

Consequence: with the munchkin configuration enabled (a supported
`borg.txt` option, off in the stock configuration) and a young Borg
(max character level under 12), a remembered stair on the last-scanned
neighbour produces a phantom stair keypress on floor — the seed of a
repeat-forever stuck state. No death or survival claim is made.

Canonical report: [issue244](https://github.com/diegoami/neo-angband-tracker/issues/244).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/munchkin-244-evidence
git show PINSHA:docs/audit/c-reports/munchkin-244/reproduce.py > /tmp/reproduce-munchkin-244.py
python3 /tmp/reproduce-munchkin-244.py --fixtures-ref PINSHA --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the own-grid
re-acquire, runs two recorded situations per variant (the fixture
profile enables `borg_munchkin_start`), checks the fail-before and
pass-after matrix and the byte-identical control, records both full
gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**A staircase the Borg was not standing on**: the original queues
`>` while standing on floor; the correction queues no stair key.

Actual recorded output, original (`before-phantom.log`):

```text
MODE phantom
GEOMETRY player=(11,11) own_feat=1 nw_feat=6 far_stair=(30,30) track_more=1
RATING acted=1 keys=">" stair_key=62
OUTCOME acted=1 stair_key=62 own_is_stair=0
```

Corrected (`after-phantom.log`): `RATING acted=1 keys="" stair_key=0`,
`OUTCOME acted=1 stair_key=0 own_is_stair=0`. Control (`on-stair`):
`keys=">"` with `own_is_stair=1` in both variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`5b31e13d1023a64bf4f8613352e738757b5f56ec`. Five lines in
`src/borg/borg-think-dungeon.c`. The adjacent-monster scan in
`borg_think_dungeon_munchkin()` assigns `ag = &borg_grids[y][x]` for
each of the eight neighbours, so after the scan `ag` points at the
last in-bounds neighbour instead of the Borg's own grid; the
"Take the Up Stair" and "Take the Stair" tests that follow compare
`ag->feat` against the staircase features. The correction re-points
`ag` at the Borg's own grid between the scan and the tests. The
power-climbing section above the scan already uses a correct local
re-acquire (`tmp_ag`) — the same pattern, applied to the section below
the scan.

```sh
git fetch --no-tags origin task/c-borg-munchkin-stair-grid
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-think-dungeon.c | sed -n '836,920p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 5b31e13d1023a64bf4f8613352e738757b5f56ec -- src/borg/borg-think-dungeon.c
```

### Conditions and limits

Native Human Warrior level 1, depth 30, seed 1000, lit constructed
40x40 arena, birth equipment only (rapier, torches wielded so nothing
in the pack is worth wearing), munchkin mode enabled in the Borg
configuration with the dispatch gate it requires (max character level
under 12). One seen stair is sealed inside a permanent-wall pocket so
every stair flow fails and the decision reaches the take-the-stair
tests; a remembered-but-untracked down stair sits on the north-west
neighbour, the grid the scan examines last. The scenario calls the
public `borg_think_dungeon()` and collects every keypress it queues.
This isolates the stale-pointer test, not full autonomous play; the
measured claim is the phantom keypress itself, not the full loop it
can seed. No death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration; the runner enables the munchkin
option when staging it), `result.json`/`receipt.json` (outcomes and
hashes), and the captured phase logs. [Full
receipt](receipt.json).
