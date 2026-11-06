# A blocked boulder makes the Borg dismiss the giant's area attacks

A Rogue sees a storm giant six squares away. The Borg treats the giant's
boulder as unable to reach it, then carries that decision over to the giant's
lightning area attacks. **In the open-path trial, the original stays and loses
54 HP. After the correction, it uses one Phase Door scroll, moves behind a wall
and keeps all 130 HP during the scheduled spell.** Area attacks can reach a
character even when a creature blocks a thrown boulder.

The creature-in-the-path trial shows the limit. With a yellow mold between the
giant and Rogue, the original stays and loses 59 HP. **The correction spends
one scroll, but lands in another exposed square and still loses 54 HP.** This
is not a guarantee of safe teleportation or uniformly better resource use.

The clear-path control also exposed a separate defect: the path checker counts
the caster itself as blocking its own projectile. That is why even the open
path is initially treated as blocked. Fixing that path check alone changes the
open-path decision but leaves the mold-blocked area attack underestimated. The
proposed correction instead assesses each monster spell separately; the two
corrections are tested independently and together. The separate projectile-path
defect remains outside this source proposal.

Canonical report: [issue64 / CBUG-0048](https://github.com/diegoami/neo-angband-tracker/issues/64).
Discovery credit for the mixed-spell finding: borg-rs. The independent
caster-blocking defect was found here through the clear-path control. Priority
is medium, reduced from the imported high label: a changed escape choice,
actual spell damage and resource cost are measured; no death, encounter
frequency or overall survival improvement is claimed.

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/cbug-0048-evidence
git show 0507c24cccd2131710e4e6cd566f901bad94e081:docs/audit/c-reports/CBUG-0048/reproduce.py > /tmp/reproduce-cbug-0048.py
python3 /tmp/reproduce-cbug-0048.py --fixtures-ref 0507c24cccd2131710e4e6cd566f901bad94e081
```

The runner builds four native source versions in a fresh temporary checkout:
original, the separate path correction alone, both corrections together, and
the proposed per-spell correction alone. It runs the same five scenarios on
each, checks the actual results and generates an offline `replay.html` at the
printed path. Your source checkout and character/profile are not used or edited.
The disposable source and binary finish on the proposed per-spell correction
alone; the diagnostic path change is restored away.

Open that file in your browser. Choose **Storm giant: open path**, then **Start**,
**Show escape choice** and **Show scheduled spell**. Both start at 130 HP with
one Phase Door scroll. The original stays and ends at 76 HP. The proposed fix
uses the scroll and moves from (20,20) to (13,24), behind the dividing wall;
the giant no longer has a terrain-clear path, so no spell is scheduled. It
retains 130 HP. The recorded native positions, resources and health are shown.

Select **Storm giant: mold in the path; still-hit counterexample**. The original
ends at 71 HP with its scroll intact. The proposed fix uses the scroll, moves
to (27,15), remains exposed and ends at 76 HP. The gain is five HP in this seeded
recording at a cost of one scroll; it is not complete protection. Different
native actions consume different random draws, so this is not a comparison of
an identical damage roll at two locations.

In **Compare the original with**, choose **Alternative: only correct the
projectile path**. It fixes the open-path decision, but in the mold-blocked case
the Rogue still stays and loses 59 HP. Thus that path correction does not replace
the per-spell correction. **Diagnostic: both corrections together** also makes
the two mixed-attack cases request an escape.

| Source version | Open-path storm giant | Mold-blocked storm giant |
| --- | --- | --- |
| Original | Stays; loses 54 HP | Stays; loses 59 HP |
| Path correction only | Scroll; retains 130 HP | Stays; loses 59 HP |
| Per-spell correction only (proposed) | Scroll; retains 130 HP | Scroll; still loses 54 HP |
| Both corrections (diagnostic) | Scroll; retains 130 HP | Scroll; still loses 54 HP |

Three complete control logs are byte-identical between the original and the
proposed per-spell correction:

- Patriarch with area spells but no preceding boulder: one scroll, 47 HP lost.
- Orc shaman's direct wounding attack before its projectile: native Phase Door
  spell costs two mana; no scheduled spell after the move; no HP lost.
- Hill giant with only a boulder: no escape, 18 HP lost. The separate path bug
  remains visible in this control; unchanged does not mean its estimate is good.

The path-only/both diagnostics intentionally change the hill giant control:
its estimate rises from zero, it spends two mana teleporting, and still loses
29 HP instead of 18 HP in this recording. That independent correction is also
not uniformly safer and is not silently included in this source patch.

Intended-choice checks: original 3/5, proposed correction 5/5. The path-only
alternative still fails the mold-blocked case; its other four cases complete.
The combined diagnostic passes all five. A control checks execution integrity
and comparison stability, not the optimality of its unchanged policy.

To repeat the complete native regression gates as well:

```sh
python3 /tmp/reproduce-cbug-0048.py --fixtures-ref 0507c24cccd2131710e4e6cd566f901bad94e081 --full-tests
```

Actual original and proposed-correction gates: 965/965 unit checks and 5/5
frontend tests, without relevant skips. The path-only and combined diagnostics
run these native scenarios, not separate complete regression gates. The native
unit executable can return zero after failed assertions; the Python runner
checks actual summaries and recorded outcomes.

Prerequisites: Git, Python3, CMake, C compiler/Make, `flock` and native platform
dependencies (X11 development headers/libraries for this Linux configuration).
Optional `--cmake-arg=...` supplies task-local dependencies. This cloud used
GCC 14.2, CMake 4.4.4 and extracted X11 headers. Heavy work coordinates through
`/tmp/angband-validation.lock`.

### Actual output and causal explanation

Open path, original:

```text
PATH clean_bolt=0 native_projectable=1 blocker=0
DANGER current=120 escape_threshold=104
DECISION escape=0 keys=0 sequence=
OUTCOME escape=0 moved=0 distance=6 cast=1 hp_before=130 hp_after=76 hp_lost=54 mana_spent=0 scrolls_spent=0
```

Same setup, proposed per-spell correction:

```text
DANGER current=1890 escape_threshold=104
DECISION escape=1 keys=4 sequence=<Escape><Escape>rc
OUTCOME escape=1 moved=1 distance=13 cast=0 hp_before=130 hp_after=130 hp_lost=0 mana_spent=0 scrolls_spent=1
```

Mold-blocked counterexample, original and proposed correction:

```text
OUTCOME escape=0 moved=0 distance=6 cast=1 hp_before=130 hp_after=71 hp_lost=59 mana_spent=0 scrolls_spent=0
OUTCOME escape=1 moved=1 distance=7 cast=1 hp_before=130 hp_after=76 hp_lost=54 mana_spent=0 scrolls_spent=1
```

The original classifies the earlier boulder as a projectile, rejects its path,
and retains that classification when considering later area spells. Their
ordinary damage contribution is discarded too. Resetting the classification
for each spell restores those area-spell contributions to the existing danger
heuristic. The escape routine then requests a real Phase Door action. The
native engine consumes the scroll and moves the character; the actual new
position determines whether the scheduled spell can still reach it.

The 120/1890 values are internal danger estimates, not predicted HP damage.
This correction does not make the existing damage heuristic accurate: it stops
incorrectly discarding later spells. The native scheduled attack here is a ball
of lightning, not the giant's stronger multi-element storm spell. No native
storm-spell damage is claimed. The escape decision depends on more than the
printed threshold, including checks of potential Phase Door destinations.

### Source explanation and pinned inspection

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`.
Proposed correction: `b223c4f18622883549682f9f997b76fe1f06fc73`.
Only `src/borg/borg-danger.c` changes: move the projectile flag's initialization
from the whole monster's spell calculation into the loop for each spell.

```sh
git fetch --no-tags origin task/c-borg-per-spell-projectile
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-danger.c | sed -n '557,622p;936,980p;1108,1128p;2178,2206p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/list-mon-spells.h | sed -n '35,76p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-flow-kill.c | sed -n '325,343p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:lib/gamedata/monster.txt | sed -n '10862,10888p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-projection.c | sed -n '373,409p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 b223c4f18622883549682f9f997b76fe1f06fc73 -- src/borg/borg-danger.c
```

For an exhaustive scoped inventory of the classification's declaration,
initialization, assignments and read:

```sh
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-danger.c | sed -n '557,2227p' | rg -n '\bbolt\b'
```

There is one declaration initialized false before the loop, multiple assignments
to true for projectile cases and one path-check read that can zero damage.
There is no reset inside the loop. The boulder appears before the lightning
ball and storm in the normal spell enumeration, and stock storm giants have
all those spells. The Borg copies the ordinary enum order into its known spell
list. No fixture reorders, removes or invents monster spells.

Moving the initialization inside the loop gives each spell its own classification.
An area spell no longer inherits a previous boulder's blocked status. The
existing projectile path check itself stays unchanged in the proposed source.
The separate path diagnostic changes only `if (ag->kill)` to
`if (dist && ag->kill)` within `borg_projectable_pure()`, so the starting caster
is not an intervening obstruction. The comparative cases show precisely what
that separate correction can and cannot fix.

### Unchanged conditions, interventions and limits

Native Human Rogue level 20, 130 HP/121 mana, depth 38, seed 1000, lit 40x40 arena.
The fixture constructs experience, maximum base Intelligence, a known +6
Intelligence ring, the known Phase Door spell and one actual scroll. The native
spell has 5% failure in this setup; the shown attempts succeed, not every attempt.
The internal Borg clock is set to 100 to be past its existing anti-summon delay;
it does not simulate 100 world turns.

A wall at column 15 divides the arena, with a passage around its end. The
Rogue starts at (20,20) and caster at (20,14). The blocked case also creates a
native visible yellow mold at (20,18). Native view and Borg observation create
the actual monster records. The three controls substitute the named stock
caster, with the same setup otherwise. Their native spell lists are preserved.

Production `borg_danger()` and `borg_escape()` choose; the adapter validates
and executes their actual spell/book or scroll keys using native commands.
It does not substitute a safer destination, suppress a spell failure or create
a free scroll. After movement it updates native view and checks actual terrain
projection/range. If the caster can still reach the character, the fixture
schedules one valid native spell from that caster's own spell flags: ball of
lightning for storm giant/patriarch, wounding for orc shaman, boulder for hill
giant. It establishes the ordinary active-monster context so native spell power
comes from the real caster. Native code computes damage/saves and item effects.

This deliberately chooses one valid spell effect to measure the consequence;
it does not run monster AI selection, its cast-frequency/failure gate, all
player fallback actions, full UI/energy/world timing, pursuit or a whole fight.
Only one scheduled attack is measured, with no extra enemy-attack or death claim.
The same seed/setup are used across source versions and RNG is not reset between
actions. Different chosen actions naturally consume different random draws.

The blocked example and the hill-giant path diagnostic expose resource/health
tradeoffs. Passing tests do not establish overall survival benefit or owner
acceptance. Source adoption, the separate path correction, downstream translation
and official Angband posting are outside this proposal.
