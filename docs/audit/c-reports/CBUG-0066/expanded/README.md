# See and repeat the equipment-swap scenarios

This is **CBUG-0066 / issue82**, the emergency equipment selection bug. The
native Borg's old comparison considers the item with the **higher danger**.
That can make it miss a useful swap or actually equip the less protective item.

## What to watch

| Added scenario | What the unfixed Borg does | HP lost before / after |
| --- | --- | ---: |
| Fire dragon | Leaves the fire-resistant shield in its pack because it considers the frost dagger instead | **88 / 29** |
| Lightning dragon | Leaves the lightning-resistant dagger in its pack because it considers the cold shield instead | **79 / 26** |
| Frost and fire dragons together | Equips the frost dagger, although the spare shield protects against both attacks | **113 / 59** |

The last case demonstrates a **worse item actually equipped**, not only a missed
swap. Both alternatives help and both satisfy the existing acceptance threshold:
current danger239, shield120, dagger174, threshold225. The old code chooses174.
The corrected code chooses120. Each version then receives one native frost
breath and one native fire breath. Frost costs32HP in both; fire costs81HP with
the dagger alone and27HP with the shield. Total damage is113 versus59HP.

The original two frost cases remain79/26HP. Four controls remain identical:
armour-only28/28, weapon-only28/28, equal danger26/26, neither item helps79/79.
In total, five affected cases fail before/pass after; all nine cases pass after.

## Open the visible replay

[replay.html](replay.html) is a self-contained offline page. Download the raw
file and open it in your browser, or generate it yourself with the commands
below. It needs no server or internet connection once saved.

Choose a scenario, then use **Play**, **Next**, **Show decision** or **Show damage**.
The panels show the native map, equipped items, resistances, danger estimates,
actual game messages and HP at each captured step. Start with **Two threats;
both backups help** and click **Show decision**: the old version equips the
frost dagger; the corrected version equips the resistance shield. Click
**Show damage** to compare337HP remaining against391HP.

The map is captured with the native game's `map_info()` and
`grid_data_as_text()` routines. This is a replay of real native C run data,
not a live autonomous game window. The page's labels and controls explain the
data; they do not simulate damage or choose an item. Full native output and
setup limits are expandable beneath the panels, and the captured data can be
downloaded from the page.

## Repeat it yourself

From your clone of **diegoami/neo-angband-tracker**:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/cbug-0066-evidence
git show FETCH_HEAD:docs/audit/c-reports/CBUG-0066/expanded/reproduce.py > /tmp/reproduce-cbug-0066.py
python3 /tmp/reproduce-cbug-0066.py
```

The fetch retrieves the evidence branch; `FETCH_HEAD` is the revision just
fetched. The script extracts the fixture and renderer from that exact revision,
creates a fresh source/build/profile tree, builds both C versions, runs all
nine cases, verifies the regression/control results and writes `replay.html`.
It prints the complete path to open. No `/PATH/TO` placeholders or manually
prepared files are needed. Previous worktrees/profiles/runs are preserved.

Prerequisites: Git, Python3, CMake, a native C compiler/build toolchain, `flock`
and platform dependencies. This Linux CMake configuration enables X11 and
needs its development headers/libraries. All heavy commands wait on the
shared validation lock and print their log paths.

To additionally run both full native regression gates:

```sh
python3 /tmp/reproduce-cbug-0066.py --full-tests
```

The default always runs all nine relevant scenario cases and checks the four
controls. `--full-tests` adds the general `alltests` target for both variants,
including `allunittests` and the frontend tests; it does not enable otherwise
skipped scenario cases. Each invocation uses new temporary directories.
The upstream unit executable can return exit0 for a failed test, so the runner
checks actual case summaries, completed outcomes and damage differences.

## Setup and what the comparison establishes

The original C pin remains `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`;
the corrected source matches clean proposal head
`017ba47ebc8731fa0a61361a6f6777759a748d2a`. Only its selection condition changes:

```diff
-    if (b_p1 <= b_p2) {
+    if (weapon_swap && (!armour_swap || b_p2 <= b_p1)) {
```

The first six modes retain the original scenario's construction and measured
results. The fire and lightning cases use the corresponding real dragon,
spell and legal ego items. Like the original, each has two controlled
preparation breaths before the equipment decision. They are level40 Warriors
at depth/maxdepth50, where the production Borg enables backup gear.

The two-threat case starts at full health: a level43 Warrior with450HP, obtained
through2,000,000 experience rather than the original1,000,000. There is one
visible white dragon and one visible red dragon three grids away. The pack
contains an ordinary small metal shield of Resistance and dagger of Frost.
The production Borg selects the backup indices and calculates danger; no
candidate scores or selection are substituted by the observer. It actually
queues an inventory letter, which is decoded and applied with native
`inven_wield()` before the controlled native spells resolve damage. No rare
artifact, injected hit-point loss, knockout state or custom damage formula is
needed in any added case.

As in the original fixture, birth progression/depth, item construction and
identification, illuminated arena, spell scheduling and the direct wield
application are synthetic interventions. Seeds, conditions and assertions
remain identical between each before/after pair. This isolates the emergency
swap stage. It demonstrates worse protection and extra native damage, without
claiming complete autonomous caution-loop/UI timing, encounter frequency or
player death. Other healing/escape alternatives are outside this test.

All frames and raw case output are retained in the before/after JSON/logs.
The original committed evidence is unchanged. The extended observer adds only
read-only map/status capture; the original six case results still match, and
all four control logs are byte-identical between variants.

## Checked end to end

The committed bundle was replayed in fresh temporary directories with
`--full-tests`: five expected baseline failures, nine corrected passes, four
byte-identical controls, and both full native gates965/965 +5/5. All18 fresh
case logs match the captured replay exactly. END-TO-END-RESULT.json records
the source/fixture pins, helper-code hashes and full-gate log hashes. The nine
replay modes and decision/damage controls were also checked in a browser with
no JavaScript errors. The C proposal itself is unchanged.

## Why not simply reverse the comparison?

[The same native tests run against all three conditions](SIMPLE-COMPARISON.md)
show that a simple reversal passes7/9 but selects an absent item in both
single-backup cases. The guarded version passes9/9. Use `--compare-simple`
to repeat that additional check; the disposable source/binary are restored
to the guarded version afterwards.
