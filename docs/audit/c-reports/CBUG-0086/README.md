# The Borg mistakes visible Itangast for an unseen “It”

A Warrior already sees and tracks Itangast the Fire Drake. When that name is
passed to the Borg's message-matching routine, **the original treats its first
two letters as the unseen-monster pronoun “It”. Its defense chooser then drinks
one True Seeing potion. After the correction, it recognizes Itangast and keeps
both potions.** A True Seeing potion grants temporary sight of invisible
monsters; Itangast is already visible in this experiment.

This measures one unnecessary potion consumed in response to a visible dragon.
The same code still requests detection for a genuinely invisible ghost. No
extra enemy attack, player injury or death is demonstrated. The extra sight
could help against a later unseen monster; that future benefit is not tested.

The correction requires the complete word “It”, while still ignoring case.
It does not change how real unseen attackers are handled. Priority is medium,
reduced from the imported high label: a resource cost is demonstrated, combat
harm is not.

Canonical report: [issue102 / CBUG-0086](https://github.com/diegoami/neo-angband-tracker/issues/102).
Discovery credit: borg-rs; independently reproduced here in native C.

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/cbug-0086-evidence
git show b61f6b611600fdbc4c6370fff3ac2835ee77017f:docs/audit/c-reports/CBUG-0086/reproduce.py > /tmp/reproduce-cbug-0086.py
python3 /tmp/reproduce-cbug-0086.py --fixtures-ref b61f6b611600fdbc4c6370fff3ac2835ee77017f
```

The runner builds original and corrected native C in a fresh temporary checkout,
runs the same five cases, checks actual results and generates an offline
`replay.html` at the printed path. It does not use your character/profile or
edit your source checkout.

Open that file in your browser. Choose **Visible Itangast: unnecessary potion**,
then **Start** and **Show defense decision**. Both recordings start with two
True Seeing potions and 421 HP. The original drinks one and gains temporary
sight of invisible monsters. The correction requests no defense and retains
both potions. Both stay at 421 HP; no enemy attack is scheduled.

Select the four controls. A visible ogre requests no potion in either version.
An unseen ghost generates the native descriptor **Something**, **It** or **it**;
each still requests and consumes one True Seeing potion. These names come from
the native monster-description API, not replacement strings invented by the
adapter. All four complete control logs are byte-identical before/after.

Actual intended-choice results: original 4/5, correction 5/5. The original named
dragon case completes and records the actual potion use before failing its
recognition assertion. It is not a crash or a missing test result.

To repeat both full native regression gates as well:

```sh
python3 /tmp/reproduce-cbug-0086.py --fixtures-ref b61f6b611600fdbc4c6370fff3ac2835ee77017f --full-tests
```

Actual gates: 965/965 unit checks and 5/5 frontend tests on both original and
correction, without relevant skips. The native test executable can return zero
after failed assertions; the runner inspects actual summaries and outcomes.

Prerequisites: Git, Python3, CMake, C compiler/Make, `flock` and native platform
dependencies (X11 development headers/libraries in this Linux configuration).
Optional `--cmake-arg=...` supports task-local dependencies. This cloud used
GCC 14.2, CMake 4.4.4 and extracted X11 headers. Heavy work coordinates through
`/tmp/angband-validation.lock`.

### Relevant actual output and why it matters

Original:

```text
SETUP seed=1000 class=Warrior level=40 depth=52 hp=421 monster=Itangast the Fire Drake visible=1 known_index=1
NATIVE_NAME Itangast the Fire Drake
MESSAGE located=0 stamp_before=0 stamp_after=100 danger=488 limit=842
DECISION defend=1 keys=2 sequence=qc
ENGINE Your eyes feel very sensitive!
ENGINE You have a Potion of True Seeing (c).
OUTCOME located=0 defend=1 potions_spent=1 see_invisible=60 stamp=100 hp=421
```

Corrected, same starting character, inventory, dragon, seed and name:

```text
NATIVE_NAME Itangast the Fire Drake
MESSAGE located=1 stamp_before=0 stamp_after=0 danger=488 limit=842
DECISION defend=0 keys=0 sequence=
OUTCOME located=1 defend=0 potions_spent=0 see_invisible=0 stamp=0 hp=421
```

The original returns “unseen” despite a known, visible monster. It records a
recent unseen attacker, making detection eligible. The full defense chooser
then requests the actual potion. The native game consumes it and grants 60
units of temporary sight; this is more than an internal flag or positive score.

The correction returns the known monster's index and creates no false unseen
sighting. The danger estimate remains 488, so a changed estimate is not the
reason the potion is saved. The item, eligibility limit of 842 and all other
defense options are unchanged. The only source change is the complete-word
comparison.

The matched native control names are `The Ogre`, `Something`, `It` and `it`.
The three unseen controls still consume a potion and gain 61 units of native
temporary sight. Their entire output matches across the two source versions.
The different duration from the dragon case reflects different native setup/RNG
history; no duration is substituted or normalized.

### Source explanation and pinned inspection

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`.
Correction: `cad765806eed6431642ab02b53240d7938470c21`.
One comparison in one file changes.

```sh
git fetch --no-tags origin task/c-borg-exact-unseen-name
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-flow-kill.c | sed -n '1274,1332p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-defend.c | sed -n '2659,2737p;3536,3625p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-update.c | sed -n '1915,1945p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:lib/gamedata/monster.txt | sed -n '11437,11451p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 cad765806eed6431642ab02b53240d7938470c21 -- src/borg/borg-flow-kill.c
```

`my_strnicmp(who, "It", 2)` ignores case and compares just two characters. It
therefore accepts both the pronoun `It` and the real unique name `Itangast the
Fire Drake`. That branch returns before normal named-monster matching and
records the current time as an unseen sighting. `my_stricmp(who, "It")` instead
requires the whole word while preserving case-insensitive matching. The
separate existing `Something` branch stays unchanged.

For an exhaustive scoped search of the unseen-sighting timestamp:

```sh
for f in borg.h borg-flow-kill.c borg-fight-defend.c borg-recover.c borg-update.c borg-commands.c; do
  git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/$f | rg -n 'need_see_invis'
done
```

The declaration is an internal clock value. The name matcher records a new
sighting; new-level update clears it; recovery also records time to avoid loops.
Monster creation reads it while clearing old regional fear. Two defense checks
read elapsed time, and the diagnostic command prints it. In the decisive
detection path, a recent sighting is required, as are suitable light/state,
an available resource and danger no greater than twice the character's
avoidance value. The fixture uses the actual danger calculation and full
production defense selection; no detection score is forced.

### Unchanged conditions, interventions and limits

Native Human Warrior level 40, depth 52, seed 1000, 421 HP, lit 40x40 arena,
known True Seeing potions. Native game APIs create the character, items and
monsters. Itangast stands six squares away, is natively visible and already
observed in the Borg's monster list. The ghost controls are natively invisible
and are not inserted as visible monsters. The fixture obtains each name through
`monster_desc()` with the relevant standard/pronoun/capitalization mode.

The character wears a known Ring of Flames and has 50 units of native temporary
fire protection. This matters: detection is suppressed if danger exceeds its
limit. Here the computed 488 is below 842. The initial exploratory unprotected
setup reached 2000 and requested no defense despite the wrong name match; the
mistake does not always consume a potion. The committed reproduction holds the
protected setup unchanged in both source versions. It does not assert that
every Itangast encounter has this consequence.

The fixture passes the game-generated name to production `borg_locate_kill()`,
then calls production `borg_defend()` with the actual danger. It isolates that
message-name/defense sequence; it does not run the full message parser, schedule
an actual monster attack or run all caution/attack alternatives. The adapter
validates the exact queued potion and inventory key before native
`do_cmd_quaff_potion()`; it does not choose a replacement item or invent an
effect. Native code consumes the item and computes its duration.

No monster turns, damage, deaths or long-term potion savings are measured.
A later invisible enemy might make the extra sight useful; no such enemy is
present in the dragon case and that future benefit is not tested. This remains
a C-only proposal, not source adoption or an official Angband submission.
