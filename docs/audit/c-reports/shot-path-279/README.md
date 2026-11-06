# The Borg counts the throwing giant as an obstacle to its own boulder

A Rogue sees a hill giant six squares away, with no creature or wall between
them. **The original path check stops on the throwing giant itself and rates
its boulder at zero danger. A scheduled native boulder still hits for 18 HP.**
After the correction, the Borg recognizes the clear shot and considers the
boulder when deciding whether to escape.

**The hill-giant result is an unfavorable counterexample:** the correction spends
two mana on Phase Door, remains exposed and takes 29 HP instead of 18 HP in this
seeded recording. Correctly recognizing a shot does not guarantee a safer
teleport. In the clear-path storm-giant case, the correction uses one scroll,
moves behind a wall and retains 130 HP instead of losing 54 HP. Both outcomes
are shown; there is no uniform safety, death or frequency claim.

The correction ignores the caster at the starting square, while preserving
actual intervening creatures and walls. A yellow mold really blocking a hill
giant's boulder still prevents native damage to the Rogue. Player-origin paths
and valid endpoints retain their previous results.

Canonical source report: [issue279](https://github.com/diegoami/neo-angband-tracker/issues/279).
This native C defect was independently found in a clear-path control while
validating borg-rs [CBUG-0048 / issue64](https://github.com/diegoami/neo-angband-tracker/issues/64).
That separate report concerns carrying one spell's classification into later
spells. The source corrections are independent. Priority is medium: a false
zero-threat assessment, actual hits and changed choices are measured; net
combat benefit is not established.

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/shot-path-279-evidence
git show cef0b118e02976a3384d4c348ba4cd0210c1f492:docs/audit/c-reports/shot-path-279/reproduce.py > /tmp/reproduce-shot-path-279.py
python3 /tmp/reproduce-shot-path-279.py --fixtures-ref cef0b118e02976a3384d4c348ba4cd0210c1f492
```

The runner builds four native versions in fresh temporary source/build/profile
directories: original, the independent per-spell correction alone, both
corrections, and the proposed caster-origin helper correction alone. It runs
the same six scenarios, checks actual results and creates an offline
`replay.html` at the printed path. Your checkout and character/profile are not
edited. The disposable source/binary finish on the helper-only correction;
the independent per-spell change is restored away.

Open that replay. Select **Hill giant: correct path, unfavorable teleport
outcome**, then **Start**, **Show escape choice** and **Show scheduled spell**.
The original marks the shot blocked, stays and ends at 112 HP. The correction
marks it clear, spends two mana teleporting and ends at 101 HP. Resource and
health costs remain visible beside the corrected recognition.

Choose **Storm giant: clear shot and successful escape**. The original stays and
ends at 76 HP. The correction uses one Phase Door scroll and moves behind the
wall to (13,24), retaining all 130 HP during the scheduled attack. This is one
bounded native example, not a guaranteed teleport result.

Choose **Control: actual mold blocks the boulder**. Both versions mark that shot
blocked, request no escape and retain all 130 HP: the actual intervening mold
still stops the native boulder. The correction removes only the false obstacle
at the starting square.

Use **Alternative: only assess each spell separately**. It can change the
storm giant's area-spell choice, but still incorrectly marks the caster-origin
path blocked and leaves the pure hill-giant boulder at zero danger. The two
fixes address different causes. The combined diagnostic is also available;
that combination is not silently included in this proposed source patch.

### Controls and actual outcomes

| Situation | Original | Helper-only correction |
| --- | --- | --- |
| Hill giant, unobstructed | No escape; loses 18 HP | Two mana; teleports but still loses 29 HP |
| Storm giant, unobstructed | No escape; loses 54 HP | One scroll; retains 130 HP behind wall |
| Hill giant, real mold in the way | No escape; no player damage | Same |
| Storm giant, real mold in the way | No escape; loses 59 HP | Same; separate mixed-spell defect remains |
| Patriarch area spell | One scroll; loses 47 HP | Same |
| Orc shaman direct spell | Two mana; no scheduled spell/no HP lost | Same |

The two actual-intermediate-monster cases have identical **complete logs**
before/after. All four lower table rows have identical native decision,
frame and outcome records. Some path-query/estimate fields correctly change in
the patriarch/orc cases even though their chosen actions do not, so their entire
logs are not claimed identical.

Every scenario also checks player-to-caster paths, a target at the starting
square, a wall target and a target beyond the wall. Those results are unchanged
before/after. In the two real-mold scenarios, a projectile aimed at the mold
itself remains a valid endpoint, even though a projectile aimed beyond it is
blocked. A missing mold is marked `blocker_endpoint=-1` (not tested), not reported
as a fabricated successful endpoint query.

Intended path checks: original 2/6, independent per-spell-only 2/6, helper-only
6/6, both 6/6. The four original failures occur at the recorded caster-origin
path assertion, after full native actions/outcomes are captured. They are not
crashes or missing results. Unchanged outcome controls do not assert that the
original policies are optimal.

To repeat both full native regression gates:

```sh
python3 /tmp/reproduce-shot-path-279.py --fixtures-ref cef0b118e02976a3384d4c348ba4cd0210c1f492 --full-tests
```

Actual original and helper-only gates: 965/965 unit checks and 5/5 frontend tests,
without relevant skips. The two diagnostic alternatives run the scenarios,
not separate complete regression gates. Native test exit zero alone is not
sufficient; the Python runner checks actual summaries and outcome fields.

Prerequisites: Git, Python3, CMake, C compiler/Make, `flock` and native platform
dependencies (X11 development headers/libraries on this Linux configuration).
Optional `--cmake-arg=...` supplies task-local dependencies. This cloud used
GCC 14.2, CMake 4.4.4 and extracted X11 headers. Heavy work coordinates through
`/tmp/angband-validation.lock`.

### Relevant actual output and why it supports the claim

Unobstructed hill giant, original:

```text
PATH clean_bolt=0 native_projectable=1 blocker=0
GEOMETRY caster_clear=0 player_clear=1 same_start=1 wall=0 through_wall=0 blocker_endpoint=-1 expected_clear=1
DANGER current=0 escape_threshold=104
DECISION escape=0 keys=0 sequence=
OUTCOME escape=0 moved=0 distance=6 cast=1 hp_before=130 hp_after=112 hp_lost=18 mana_spent=0 scrolls_spent=0
```

Same setup, helper-only correction:

```text
PATH clean_bolt=1 native_projectable=1 blocker=0
GEOMETRY caster_clear=1 player_clear=1 same_start=1 wall=0 through_wall=0 blocker_endpoint=-1 expected_clear=1
DANGER current=192 escape_threshold=104
OUTCOME escape=1 moved=1 distance=11 cast=1 hp_before=130 hp_after=101 hp_lost=29 mana_spent=2 scrolls_spent=0
```

The native terrain path is clear in both versions. The original Borg rejects
only the caster-origin direction, while accepting the reverse player-origin
path to the giant. The starting grid already contains the giant's known
monster record. Skipping that starting record corrects the asymmetric result;
real intermediate creatures still stop the scan. The actual native hit
contradicts the original zero-danger assessment, but the corrected escape's
health/resource result is deliberately not presented as an improvement.

The 192 is the existing danger heuristic, not a prediction of 192 HP damage.
Different chosen actions naturally consume different random draws; the 18/29
native damage figures are not an identical damage roll replayed at two positions.

Real mold blocking the boulder, both versions:

```text
GEOMETRY caster_clear=0 player_clear=0 same_start=1 wall=0 through_wall=0 blocker_endpoint=1 expected_clear=0
OUTCOME escape=0 moved=0 distance=6 cast=1 hp_before=130 hp_after=130 hp_lost=0 mana_spent=0 scrolls_spent=0
```

This separates ignoring the caster from incorrectly allowing all occupied
squares. The boulder is executed natively and does not hurt the player beyond
the actual blocker.

### Source explanation and pinned inspection

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`.
Helper-only correction: `1bce07d8080b520589c292c793cc0b95ef9df182`.
Independent per-spell diagnostic: `b223c4f18622883549682f9f997b76fe1f06fc73`.
The proposed source changes one comparison in `src/borg/borg-projection.c`.

```sh
git fetch --no-tags origin task/c-borg-caster-shot-origin
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-projection.c | sed -n '365,409p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-danger.c | sed -n '2180,2189p'
git grep -n 'borg_projectable_pure(' e473720ed88a0f0c7ca66815cef3000eb4a38bc3 -- src/borg
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-attack.c | sed -n '1108,1126p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-defend.c | sed -n '2881,2942p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 1bce07d8080b520589c292c793cc0b95ef9df182 -- src/borg/borg-projection.c
```

The exhaustive caller search finds the definition, one attack caller, four
light-beam defense callers and the danger caller. The attack/defense paths start
at the player, whose grid has no monster record. The danger path starts at the
monster, whose starting grid does have one.

The loop starts at distance zero on its source square. It tests arrival first,
then unconditionally stops at `ag->kill`. For a different destination, that
stops immediately at the caster. The correction changes the obstruction test
to `if (dist && ag->kill)`: only a monster after the starting square can block
the path. Arrival remains before obstruction, so a monster at the final target
is still allowed. Unknown/wall handling and the range bound stay unchanged.

The native geometric and attack cases above test the contract from both caller
directions and retain genuine interior obstacles. The independent per-spell
alternative does not repair this contract, even where it changes an area-spell
choice. Conversely, the helper correction alone leaves the separate mixed-spell
error visible behind a real blocker; it is not claimed to fix all danger logic.

### Unchanged conditions, interventions and limits

Native Rogue level 20, 130 HP/121 mana, depth 38, seed 1000, lit 40x40 arena,
known Phase Door and one actual scroll. The fixture constructs experience,
maximum base Intelligence/+6 ring, mapped terrain and stock native monsters.
A wall at column 15 divides the arena with a passage around its end. The Rogue
starts at (20,20), caster at (20,14); the actual-mold cases add a yellow mold at
(20,18). Native visibility/Borg observation populate the real monster records.

Production danger/escape code chooses. Exact queued spell/book or scroll keys
are validated and executed with native commands. Native Phase Door has 5%
failure here; the shown attempts succeed. The internal decision clock is set
to 100 to be past the existing anti-summon delay; 100 world turns are not run.

After the chosen move, actual native terrain projection and range determine
whether to schedule one valid spell from that caster's own unchanged spell
flags: boulder for hill giant, ball of lightning for storm giant/patriarch,
wounding for orc shaman. The real active-monster context supplies native spell
power. Native code computes hit, saves, damage, movement and resource use.
The helper/path queries consume no random numbers; RNG is not reset between
actions. The unchanged seed/setup do not mean identical later RNG draws after
a changed action.

This selects a valid attack for the experiment, not monster AI spell selection,
cast-frequency/failure gates, all fallback player actions, full UI/energy/world
timing, pursuit or a whole fight. No extra enemy attack, death, encounter
frequency or overall survival benefit is claimed. The unfavorable hill-giant
case remains prominent for review. This is a source proposal, not adoption,
downstream translation or an official Angband submission.
