# The Borg hesitates to drink a healing potion after the wrong delay

A wounded, confused Warrior has seven Cure Critical Wounds potions and last
drank one five Borg turns ago. **The original leaves the potions unused, stays
at 321 HP and remains confused. After the correction, it drinks one potion,
heals to 381 HP and clears confusion.** The same problem can block a first-ever
drink, when there was no earlier potion to justify waiting.

The correction applies the existing hesitation rule shortly after a previous
drink instead. **That is a tradeoff:** in the recent-drink example it saves one
potion, but leaves the Warrior at 321 HP and confused on this call; the original
drinks and reaches 381 HP. Emergency callers can bypass that conservation rule,
and that path still drinks. This is not a claim of uniformly safer combat.

The rule is intended to discourage drinking twice in a row. Its elapsed-time
comparison points in the opposite direction. Simply reversing that comparison
fixes the long-delay example but incorrectly blocks a first-ever drink at the
start of the Borg's clock. The complete correction also checks that a previous
drink exists. This simpler alternative is included in the replay.

Canonical source report: [issue77 / CBUG-0061](https://github.com/diegoami/neo-angband-tracker/issues/77).
Discovery credit: borg-rs; independently reproduced here in native C. Priority
is medium, reduced from the imported high label: actual healing and resource
choices are measured, combat harm and deaths are not.

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/cbug-0061-evidence
git show cce1d5d92079e78e939126d3cf35e24121442734:docs/audit/c-reports/CBUG-0061/reproduce.py > /tmp/reproduce-cbug-0061.py
python3 /tmp/reproduce-cbug-0061.py --fixtures-ref cce1d5d92079e78e939126d3cf35e24121442734
```

The runner builds the original, simple reversal and complete correction in a
fresh temporary native C checkout, runs the same nine cases on each, checks the
actual outcomes and generates an offline `replay.html` at the printed path.
It does not edit your source checkout or use your character/profile.

Open that file in your browser. Select **Five turns since the last drink**,
then **Start** and **Show potion decision**. Both recordings start at 321 HP,
seven potions and confusion. The original requests no drink. The correction
uses one real potion: 381 HP, six potions and confusion cleared.

Select **One turn since the last drink: tradeoff** or **Four turns since the
last drink: tradeoff**. The original drinks; the correction conserves the potion
and leaves the injury/confusion untreated on this call. This counterexample is
shown prominently, not hidden in the logs.

Select **Control: first drink at internal turn 1**, then compare **Simple
comparison reversal only**. That alternative does not drink, although there was
no previous drink. Switch to **Complete correction**: the first drink works as
in the original. Select **No previous drink; turn 100** to see the first-use
failure in the original, and its correction.

| Source version | Passing cases | Long delay | First-ever drink at clock 1 |
| --- | ---: | --- | --- |
| Original | 5/9 | Does not drink; remains injured/confused | Drinks |
| Reverse the comparison only | 8/9 | Drinks and heals | Incorrectly refuses |
| Reverse it and require a previous drink | 9/9 | Drinks and heals | Drinks |

Five complete control logs are byte-identical before/after: a first-ever drink
at clock 1, two requests at the same clock value, an explicit emergency bypass,
one potion remaining and no potion remaining. The one-potion reserve policy is
preserved. The zero-elapsed-time exclusion is also preserved; this proposal
does not redesign callers that make two requests at the same clock value.

To repeat the complete native regression gates as well:

```sh
python3 /tmp/reproduce-cbug-0061.py --fixtures-ref cce1d5d92079e78e939126d3cf35e24121442734 --full-tests
```

Actual original and corrected native gates: 965/965 unit checks and 5/5 frontend
tests, without relevant skips. The native test executable may return zero even
when an assertion fails; the Python runner checks actual summaries and recorded
outcomes. The original has four complete expected-choice failures, not crashes.
The simple reversal fails only the first-ever-at-clock-1 case. The disposable
source and binary end on the complete correction after all comparisons.

Prerequisites: Git, Python3, CMake, C compiler/Make, `flock` and native platform
dependencies. This Linux configuration needs X11 development headers/libraries.
Optional `--cmake-arg=...` supplies task-local dependencies. This cloud used
CMake 4.4.4, GCC 14.2 and extracted X11 headers; system installation was unavailable.
Heavy builds/tests coordinate through `/tmp/angband-validation.lock`.

### Why the correction works

Original C source: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`.
Complete correction: `cedf25eb790f2ff7c7c161cd1612b18ed99fb35e`.
One condition in one source file changes; no healing amount, random threshold,
reserve count or emergency caller changes.

```sh
git fetch --no-tags origin task/c-borg-healing-potion-timing
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-item-use.c | rg -n 'when_last_quaff|Avoid drinking'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-item-use.c | sed -n '37,81p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-think-dungeon-util.h | sed -n '29,33p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-caution.c | sed -n '145,166p;369,389p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 cedf25eb790f2ff7c7c161cd1612b18ed99fb35e -- src/borg/borg-item-use.c
```

The exhaustive file search finds one declaration initialized to zero, two
writes after a requested potion (ordinary use and bypass use), and the timing
reads in this condition. There is no separate "has drunk before" flag. The timer
subtracts that stored value from the current Borg decision clock.

Originally, the hesitation applies only when elapsed time is **greater than
four**. The second test, greater than zero, adds nothing in that case. After a
long gap it can reject the potion; during the first four turns it never applies.
Zero also means no previous drink, so at clock 100 the original may hesitate
over an event that never happened.

The correction checks that the stored value is nonzero, then applies hesitation
only during elapsed times 1 through 4. It preserves the existing exclusion of
zero elapsed time. The simple comparison reversal still treats the initial zero
as a real drink at clock 0, so at clock 1 it rejects a first-ever drink. The
additional existence check prevents that measured regression.

The unchanged random test is `randint1(100) < 75`, covering 74 of its 100 possible
integer results. This describes the code's possible rolls, not measured encounter
frequency. The deterministic replay uses seed 1000; it does not claim every call
hesitates. RNG is not reset or overridden between decisions. The corrected
condition naturally changes whether that random draw occurs.

### Relevant actual output

Five-turn gap, original:

```text
MODE long-gap time=105 elapsed=5 available=7
FRAME {"stage":"start","hp":321,"max_hp":421,"potions":7,"confused":10}
DECISION use=0 bypass=0 keys=0 sequence=
OUTCOME used=0 hp_before=321 hp_after=321 healed=0 potions_spent=0 confused=10
```

Same setup, complete correction:

```text
DECISION use=1 bypass=0 keys=2 sequence=qb
OUTCOME used=1 hp_before=321 hp_after=381 healed=60 potions_spent=1 confused=0
```

One-turn gap, original versus complete correction:

```text
OUTCOME used=1 hp_before=321 hp_after=381 healed=60 potions_spent=1 confused=0
OUTCOME used=0 hp_before=321 hp_after=321 healed=0 potions_spent=0 confused=10
```

The real native potion command consumes the selected item, heals the player
and clears confusion. A positive helper return alone is not the proof. The
measured changes in health, potion count and timed confusion show what the
requested action actually does. The recent case exposes the conservation cost.

### Unchanged conditions, interventions and limits

Seed 1000, native Human Warrior level 40, depth 30, maximum 421 HP, known Cure
Critical Wounds potions, lit 40x40 arena. Native APIs create the character,
inventory and injuries. An initial controlled 120-HP injury and an actual
bypass-requested potion establish the earlier drink at clock 100. A controlled
40-HP injury then leaves 321 HP for the second request. The first-use controls
skip that establishing drink and start at 301 HP. Native confusion is set to
10 before the measured request. The reserve/empty controls explicitly reduce
or remove the remaining stack through inventory operations.

The test directly advances the Borg's decision clock to isolate the elapsed-time
rule. These are internal Borg turns, not simulated intervening world turns.
The clock-1 control tests a valid initial/sentinel edge; it is not a claim that
a level-40 character is ordinarily born on turn 1. The fixture calls the shared
production potion helper, not the complete heal/recover/escape ranking. Source
caller context above shows ordinary and emergency uses; a full healing-loop
choice has not been measured here.

The adapter validates the actual queued `q` command and inventory key and calls
native `do_cmd_quaff_potion()` on that selected potion. It supplies no alternate
item, healing amount or cure result. No monster attacks, full world/UI timing,
death or long-term conservation benefit are simulated. The recent-drink
counterexample prevents a claim of uniform safety. This is a C-only proposal,
not source adoption, downstream translation or an official Angband submission.
