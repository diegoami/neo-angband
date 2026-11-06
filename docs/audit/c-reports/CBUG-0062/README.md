# The Borg overlooks a charged fire ring on its left hand

A Warrior has a charged Ring of Flames on the left hand and an ogre in range.
**The original ring-attack check requests no activation, leaving the ogre at
70 HP. After the correction, it activates the left ring and kills the ogre,
removing its 70 HP.** A Ring of Flames launches a fireball and grants temporary
fire protection. The same ring already works on the right hand.

The correction checks both hands when finding a ring and when activating it.
Changing only one of those steps is insufficient: both partial corrections
were tested and still miss the left-ring attack. The existing preference for
the right ring, and rejection of rings that are recharging, stay unchanged.

This demonstrates a missing available attack, not a death or an overall
survival improvement. The test isolates the fire-ring attack; it does not
compare every other attack the Borg might choose in a whole turn. Priority is
medium, reduced from the imported high label.

Canonical source report: [issue78 / CBUG-0062](https://github.com/diegoami/neo-angband-tracker/issues/78).
Discovery credit: borg-rs; independently reproduced here in native C.

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/cbug-0062-evidence
git show bfe3fc402e8ae6cdec74d8b35fa8f50e612f2a8e:docs/audit/c-reports/CBUG-0062/reproduce.py > /tmp/reproduce-cbug-0062.py
python3 /tmp/reproduce-cbug-0062.py --fixtures-ref bfe3fc402e8ae6cdec74d8b35fa8f50e612f2a8e
```

The script creates fresh temporary source/build/profile directories, builds
four versions of the native game, runs all seven cases on each, checks the
actual results and produces an offline `replay.html`. It prints the path: open
that file in your browser. No manual patch downloads, placeholder paths or
changes to your existing checkout/profile are needed.

Choose **Charged fire ring on the left**, then **Start** and **Show ring attack**.
The ogre starts with70 HP in both recordings. The original requests no ring
activation and leaves70 HP. The complete correction activates the left ring,
kills the ogre and starts the ring's recharge period. It also applies the
native temporary fire protection. The Warrior's health is unchanged.

Select **Right fire ring recharging; left ready** to see the other affected
case. The correction uses the ready left ring instead of overlooking it because
the right ring is recharging.

Use **Compare the original with** to select each partial fix. **Only find either
ring** finds a target but does not activate the left ring. **Only extend
activation** still fails to find the left ring in the first place. The ogre
retains70 HP in both partial versions. This is why both loops must change.

| Production version | Passing cases | Charged left ring |
| --- | ---: | --- |
| Original | 5/7 | No activation; ogre keeps70 HP |
| Extend only ring lookup | 5/7 | Target selected, no activation; ogre keeps70 HP |
| Extend only activation lookup | 5/7 | Ring not found; ogre keeps70 HP |
| Extend both lookups | 7/7 | Left ring activates; ogre is killed |

The same native test, seed, inventory, monster, perception and assertions are
used for all four versions; only the production ring loops change. The five
controls have identical complete logs across all four versions:

- A ready fire ring on the right works as before.
- With two ready fire rings, the right ring remains preferred.
- A left fire ring that is still recharging is not used.
- With no fire ring equipped, no fire-ring attack is requested.
- A blind character does not make the ring attack.

To also run both complete native regression gates:

```sh
python3 /tmp/reproduce-cbug-0062.py --fixtures-ref bfe3fc402e8ae6cdec74d8b35fa8f50e612f2a8e --full-tests
```

Expected gates:965/965 unit checks and5/5 frontend tests on original and complete
correction, without relevant skips. The native unit executable can return exit0
after an assertion failure; the Python runner inspects actual summaries and
outcomes instead of trusting exit0. On success, the disposable source and binary
are left with the complete correction, not one of the partial experiments.

Prerequisites: Git, Python3, CMake, a C compiler/build toolchain, `flock` and
native platform dependencies (X11 development headers/libraries for this Linux
configuration). Builds wait on the shared validation lock and print log paths.
Optional `--cmake-arg=...` flags support task-local dependencies; this cloud used
CMake4.4.4 and extracted X11 headers because system package installation is
unavailable.

### Why the correction works

Source base: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`.
Complete source correction: `46c14b2828ed751ff8be7ac67df93b28cdc15f87`.
The complete diff changes two comparisons in one source file.

```sh
git fetch --no-tags origin task/c-borg-both-ring-slots
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-item.h | sed -n '132,137p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-item-use.c | sed -n '726,776p;781,821p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-attack.c | sed -n '3092,3128p;4924,4929p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 46c14b2828ed751ff8be7ac67df93b28cdc15f87 -- src/borg/borg-item-use.c
```

The right ring is slot `pack_size+2`; the left ring is the next slot,
`pack_size+3`. Both loops start at the right ring but stop before the left ring:

```c
for (i = INVEN_RIGHT; i < INVEN_LEFT; i++)
```

Each therefore examines only the right hand. The correction includes the final
left-ring slot:

```c
for (i = INVEN_RIGHT; i <= INVEN_LEFT; i++)
```

The attack first calls `borg_equips_ring()` to check the ring is known, ready
and usable. It later calls `borg_activate_ring()` to request the equipment key.
Only extending the first loop leaves the activation lookup broken; the measured
partial experiment proves that finding a target is not the same as casting the
fireball. Only extending the second loop cannot help when the first rejects the
left ring before activation. All existing awareness, identification, type,
recharge and device-skill checks remain in place.

### Relevant actual output

Original, fire ring on the left:

```text
DECISION score=0 keys=0 goal_x=0 goal_y=0 sequence=
OUTCOME activated=0 selected_slot=-1 enemy_before=70 enemy_after=70 damage=0 killed=0 right_timeout=0 left_timeout=0 fire_protection=0
```

Only the first loop corrected:

```text
DECISION score=160 keys=10 goal_x=20 goal_y=14 sequence=*p88888855
OUTCOME activated=0 selected_slot=-1 enemy_before=70 enemy_after=70 damage=0 killed=0 right_timeout=0 left_timeout=0 fire_protection=0
```

Complete correction:

```text
DECISION score=160 keys=12 goal_x=20 goal_y=14 sequence=*p8888885Ad5
ENGINE You feel resistant to fire!
OUTCOME activated=1 selected_slot=3 enemy_before=70 enemy_after=0 damage=70 killed=1 right_timeout=0 left_timeout=98 fire_protection=37
```

`A` requests item activation and the following `d` selects the left ring.
The partial fix's target-selection keys contain no activation request. Native
monster death, left-ring recharge and temporary fire protection show that the
complete correction executes the actual item effect, rather than merely
reporting a positive estimate. The score160 is an internal attack score, not
damage. The70 recorded HP removed is capped by the ogre's remaining health;
it is not the uncapped fireball damage.

### Unchanged conditions, interventions and limits

Seed1000, Human Warrior level40, native421 HP, depth30, device skill51, lit40x40
arena, visible ogre six squares away. The test constructs experience, maximum
base Intelligence, known rings, ring placements and recharge states. Native
game APIs handle birth, stats, equipment and monster creation. A charged left
fire ring normally has an ordinary Strength ring on the right. The fallback
case has two fire rings with the right ring still charging.

The fixture supplies the native-observed target to the production fire-ring
attack stage and validates visibility. It calls that stage's real execution
path, not a copied attack calculation. It does not run the full attack ranking
or its occasional simulation-stage random rejection. Target/equipment keys are
decoded, validated against actual gear and the Borg's selected target, then
executed through native `do_cmd_activate()`. The adapter bypasses interactive
targeting UI but does not choose another ring or target. Native code computes
device success, damage, death, recharge and protection; none is substituted.

Native device failure is49/1000 in this setup. The shown fixed-seed activation
succeeds; this does not guarantee every activation succeeds. No monster/world
turns or alternative player attacks are run. The experiment demonstrates a
missed ring attack and its restored execution, not how a full autonomous fight
ends, encounter frequency, player damage or death. There is no source adoption,
downstream translation, Rust watch or official Angband post.
