# The Borg recognizes Enlightenment but never drinks it

A Warrior carries a known Enlightenment potion in the dungeon, with stairs
beyond a wall still unknown. **The original useful-item routine leaves the
potion unused and those stairs unknown. After the correction, it drinks one
potion and the native game reveals the stairs.** Enlightenment reveals the
level's layout; the Borg already recognizes this potion but omits the drink.

The existing rule against drinking Enlightenment in town remains. Unknown
potions and unrelated True Seeing potions are still left alone. Augmentation,
which raises character stats, still works in both town and dungeon.

This is a utility fix, with priority reduced from the imported high label to
low. The experiment demonstrates missing map information and its restoration,
not a faster escape, avoided attack, injury or death. The correction consumes
one potion to obtain that information; it does not preserve the potion for a
later level or judge whether a particular level is worth mapping.

Canonical report: [issue81 / CBUG-0065](https://github.com/diegoami/neo-angband-tracker/issues/81).
Discovery credit: borg-rs; independently reproduced here in native C.

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/cbug-0065-evidence
git show 8eb4b9951cdd8a3cc3a702ce0109643dba1f98f1:docs/audit/c-reports/CBUG-0065/reproduce.py > /tmp/reproduce-cbug-0065.py
python3 /tmp/reproduce-cbug-0065.py --fixtures-ref 8eb4b9951cdd8a3cc3a702ce0109643dba1f98f1
```

The runner builds original and corrected native C in a fresh temporary checkout,
runs the same six cases and checks the actual outcomes. It prints the path to
an offline `replay.html`; open that file in your browser. It uses its own
constructed character/profile and does not edit your source checkout.

Select **Dungeon: recognized Enlightenment potion**, then **Start** and **Show
item use**. Both recordings start with two potions and unknown stairs beyond
a wall. The original requests no drink. The correction drinks one potion and
shows **Down stairs discovered**. Both retain 421 HP. The display comes from
the native player's actual map memory, not an invented successful result.

Select the five controls:

| Situation | Original and corrected result |
| --- | --- |
| Enlightenment in town | Keep the potion; stairs remain unknown |
| Unknown potion flavor | Keep the potion; stairs remain unknown |
| Unrelated True Seeing potion | Keep the potion; stairs remain unknown |
| Augmentation in dungeon | Drink Augmentation; stairs remain unknown |
| Augmentation in town | Drink Augmentation; stairs remain unknown |

All five complete control logs are byte-identical before/after. They distinguish
the missing Enlightenment action from an accidental change to all potions or
an overbroad town restriction. No separate partial correction is required for
the missing call; these adjacent policy controls test the plausible mistakes.

Actual intended-choice results: original 5/6, corrected 6/6. The original dungeon
case completes with no drink and unknown stairs before failing its expectation.
It is not a crash or a skipped test.

To run both complete native regression gates too:

```sh
python3 /tmp/reproduce-cbug-0065.py --fixtures-ref 8eb4b9951cdd8a3cc3a702ce0109643dba1f98f1 --full-tests
```

Actual gates: 965/965 unit checks and 5/5 frontend tests on original and correction,
without relevant skips. The runner checks actual summaries and outcome fields;
the native test executable can return zero after failed assertions.

Prerequisites: Git, Python3, CMake, C compiler/Make, `flock` and native platform
dependencies (X11 development headers/libraries on this Linux configuration).
Optional `--cmake-arg=...` supplies task-local dependencies. This cloud used
GCC 14.2, CMake 4.4.4 and extracted X11 headers. Heavy commands coordinate through
`/tmp/angband-validation.lock`.

### Relevant actual output and why it supports the claim

Original:

```text
SETUP seed=1000 class=Warrior level=40 depth=30 hp=421 potion=Enlightenment aware=1 light=2 food=4 safe=1
STAIRS x=30 y=20 native_feat=6 visible=0 known=0
DECISION used=0 keys=0 sequence=
OUTCOME used=0 potions_spent=0 stairs_known=0 stairs_feat=0 native_stairs=6 hp=421
```

Corrected, unchanged setup:

```text
DECISION used=1 keys=2 sequence=qc
ENGINE An image of your surroundings forms in your mind...
ENGINE You have a Potion of Enlightenment (c).
OUTCOME used=1 potions_spent=1 stairs_known=1 stairs_feat=6 native_stairs=6 hp=421
```

The queued `q` command selects the actual Enlightenment potion. The native
potion handler consumes one item and invokes the real level-revelation effect.
The player's remembered stair changes from unknown (`0`) to the same down-stair
terrain (`6`) that already existed in the actual level. Thus the experiment
shows both the missing choice and a concrete information consequence, rather
than merely a changed return value or a passing assertion. No map knowledge is
written by the observer after the choice.

### Source explanation and pinned inspection

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`.
Correction: `526906c0f367cd690bdd92b99ac1e114fd84b1a2`.
One missing potion request is added inside the existing dungeon-only branch;
the adjacent comment is corrected from “scroll” to “potion”.

```sh
git fetch --no-tags origin task/c-borg-use-enlightenment
git grep -n sv_potion_enlightenment e473720ed88a0f0c7ca66815cef3000eb4a38bc3 -- src/borg
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-item-use.c | sed -n '1105,1156p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-think-dungeon.c | sed -n '190,214p;1807,1818p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:lib/gamedata/object.txt | sed -n '3387,3398p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/effect-handler-general.c | sed -n '3017,3027p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 526906c0f367cd690bdd92b99ac1e114fd84b1a2 -- src/borg/borg-item-use.c
```

The exhaustive symbol search in `src/borg` finds the subtype declaration,
its initialization by the native potion name, and this recognition condition.
It finds no other dedicated use of the Enlightenment subtype. In the decisive
branch, town exits immediately. In the dungeon, the original reaches the end
of the branch without calling the potion helper, then skips the mutually
exclusive Augmentation branch and exits the switch. It never queues the drink.

The correction calls the existing potion helper after the town guard and
returns success when it queues the drink. Awareness checks remain outside the
switch. Augmentation remains in its existing separate branch, retaining town
use. The ordinary dungeon routine already calls this utility helper at a safe
location; that caller is not changed.

The native item data specifies `LIGHT_LEVEL`, and the effect handler calls
`wiz_light()`. Native map code memorizes non-floor terrain, including the distant
stairs. This explains the recorded knowledge change without treating an
internal action score as a gameplay effect.

### Unchanged conditions, interventions and limits

Seed 1000, native Human Warrior level 40, 421 HP, depth 30, a lit 40x40 arena.
A wall at column 25 blocks sight of stairs at (30,20), ten squares east of the
player at (20,20). The wall has a passage around its end, so this is ordinary
reachable terrain. Native view and player-map checks both confirm the stairs
are unseen/unknown before the action. A fueled wooden torch and native food
inventory provide light and four counted food supplies. The actual Borg safety
predicate returns true. There are no monsters or active injuries.

The fixture constructs experience, known or unknown potion flavor, inventory
and terrain with native APIs. Town controls use this same constructed arena at
depth 0; they test the utility routine's town gate, not a generated town/store
visit. The chosen potion stack contains two items; the native command consumes
one. Augmentation controls execute native stat-gain effects but do not reveal
the stairs.

The observer calls production `borg_use_things()`, validates the queued `q`
command and exact inventory object, then executes native `do_cmd_quaff_potion()`.
It does not select a replacement item, force a return value or copy map knowledge
into the player after the decision. This isolates item use, not the whole
exploration/caution loop, map navigation, world turns or a complete dungeon run.

The corrected routine follows its existing immediate-use policy even on a
partly explored level. Whether it should save Enlightenment for a later level
is a separate strategy question. No travel-time, resource-efficiency, combat or
survival improvement is claimed. This is a C-only proposal, not source adoption,
downstream translation or an official Angband submission.
