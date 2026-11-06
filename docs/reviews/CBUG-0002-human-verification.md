# Human verification: CBUG-0002 / source issue #14 / local proposal #210

When the Borg recovers from being knocked out, it should resume handling fight
messages such as "You hit the kobold" and "You have slain the kobold". Instead,
a flag remains stuck on "ignore these messages". The Borg still reads the screen
and player statistics and can send commands; its entire input is not cut off.

The cause is a reset of the wrong variable: `borg.dont_react` is a struct member
that nothing reads, while the fight-message handler checks the separate global
`borg_dont_react`. The fix resets the global at the existing reset point.

The demonstrated problem is lost fight information after recovery. Lost messages
could leave the Borg with stale information about enemies, but the existing run
does not demonstrate a worse decision, a player death or lost progress. Other
screen/stat updates still exist. **Priority is medium, reduced from high:** the
persistent message-processing fault is real, but high practical severity has not
been established. Further investigation is parked behind bugs with demonstrated
crashes, stuck behavior, resource waste or harmful survival/combat decisions.

Here is how a human can check the cause and the measured effect without trusting
that explanation or a green test.

Run the source commands from any clone of diegoami/neo-angband-tracker containing
these commits. They inspect immutable Git objects, so your checkout can remain
on its current branch. No game, build, profile change or network posting is
needed for steps 1–4. Fetch a missing object from our fork if necessary.

```sh
c_base=e473720ed88a0f0c7ca66815cef3000eb4a38bc3
c_fix=32ab6823c6a993d94ce382b72f8a855a20f7911c
c_evidence=3de1e3123915587483395dd40c4d2e75704e65f4
```

## 1. Check whether anything reads the struct field

```sh
git grep -n -w dont_react "$c_base" -- .
```

Actual complete result (each line starts with the full `c_base` hash):

```text
e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg.c:312:    borg.dont_react = false;
e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg.h:233:    bool dont_react; /* don't react to messages, just queue them */
```

This searches **all tracked text at the pinned C revision**, rather than a
selected file or today's possibly different checkout. `-w` keeps the field
identifier separate from `borg_dont_react`, whose underscore is a word character.
There are exactly two named occurrences: a member declaration and a write to
that member. There is **no named read of this field** in this source tree.
Its comment describes an intention; it is not evidence that the code uses it.
This search alone does not prove any gameplay consequence.

## 2. Find the state the reaction code actually uses

```sh
git grep -n -w borg_dont_react "$c_base" -- .
```

Actual complete result:

```text
e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-caution.c:920:    if (borg_dont_react) {
e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-messages-react.c:34:bool borg_dont_react = false;
e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-messages-react.c:45:    if (borg_dont_react || borg.trait[BI_ISPARALYZED]) {
e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-messages-react.h:29:extern bool borg_dont_react;
e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-messages.c:443:        borg_dont_react = true;
e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-messages.c:447:        borg_dont_react = true;
```

Classify these instead of just counting matches:

| Site | Meaning |
| --- | --- |
| messages-react.c:34 | Defines a separate global, initially false at program startup |
| messages-react.h:29 | Declares that global for other translation units |
| messages.c:443 and :447 | Two runtime writes, both setting it true |
| messages-react.c:45 | Reads the global to decide whether to discard a reaction |
| caution.c:920 | Reads the global only to print an ignoring-messages note |

There is no runtime assignment of false to this global at the base revision.
`borg.dont_react` and `borg_dont_react` are separately declared C objects;
assigning the former cannot reset the latter. Check the surrounding code next
so that a matching identifier is not mistaken for a behavioral proof.

## 3. Read the decisive context and establish the reachable trigger

```sh
git show "$c_base":src/borg/borg-messages.c | sed -n '441,448p'
git show "$c_base":src/borg/borg-messages-react.c | sed -n '40,49p'
git show "$c_base":src/borg/borg.c | sed -n '304,323p'
git show "$c_base":lib/gamedata/player_timed.txt | sed -n '211,215p;267,274p'
```

The first command shows:

```c
    if (streq(msg, "You have been knocked out.")) {
        borg_note("Ignoring Messages While KO'd");
        borg_dont_react = true;
    }
    if (streq(msg, "You are paralyzed")) {
        borg_note("Ignoring Messages While Paralyzed");
        borg_dont_react = true;
    }
```

The second shows `borg_react()` immediately returning when the global is true:

```c
void borg_react(const char *msg, const char *buf)
{
    int len;

    if (borg_dont_react || borg.trait[BI_ISPARALYZED]) {
        borg_note("# Ignoring messages.");
        return;
    }
```

The third shows the existing batch boundary:

```c
    if (borg_get_messages(&key, cursor))
        return key;

    /* Flush messages */
    borg_parse(NULL);
    borg.dont_react = false;
```

This makes the causal chain inspectable: knockout sets the global, reaction
handling tests the global, and the intended reset after parsing changes only
the unused member. The fourth command checks the actual messages emitted by
the native game's timed-effect definitions:

```text
grade:r:10000:Paralyzed!:You are paralysed!
grade:r:10000:Knocked Out:You have been knocked out.
```

The knockout text matches exactly. The paralysis text does **not** match the
parser's spelling/punctuation. Thus this proof targets **knockout**; it does
not repeat the imported report's paralysis claim as established fact.

## 4. Inspect exactly what the proposed change does

```sh
git diff "$c_base" "$c_fix" --stat
git diff "$c_base" "$c_fix" -- src/borg/borg.c
```

The complete source difference is one file, two insertions and one deletion:

```diff
     /* Flush messages */
     borg_parse(NULL);
-    borg.dont_react = false;
+    /* FORK (CBUG-0002): reset the message gate read by borg_react(). */
+    borg_dont_react = false;
```

The reset remains **after** `borg_parse(NULL)`. It does not enable reactions in
the middle of the batch being parsed, remove the separate paralysis-trait
guard, alter RNG calls, or change an attack/monster rule. It fixes the target
of the already-existing reset. This establishes the patch's mechanism, not its
runtime result.

## 5. Inspect the observed native result and the controls

```sh
for phase in before after; do
    git show "$c_evidence":docs/audit/c-reports/CBUG-0002/"$phase"-knockout.log |
        rg 'RECOVERED|You have slain|KILL_REACTION|OUTCOME|finished:'
done
```

Actual output, first unfixed and then corrected:

```text
RECOVERED turn=511 hp=20 global_suppression=1 knockout_messages=1
ENGINE turn=519 stun=0 paralysis=0: You have slain the kobold.
KILL_REACTION delta=0
OUTCOME deaths=1 reactions_before=0 reactions_after=0 delta=0
native-c/CBUG-0002 finished: 0/1 passed
RECOVERED turn=511 hp=20 global_suppression=0 knockout_messages=1
ENGINE turn=519 stun=0 paralysis=0: You have slain the kobold.
KILL_REACTION delta=1
OUTCOME deaths=1 reactions_before=1 reactions_after=7 delta=6
native-c/CBUG-0002 finished: 1/1 passed
```

The **kill-specific** delta is 0 then 1. The total queue delta after correction
is 6 because hit/miss reactions are also restored; it must not be misreported
as six kill reactions. Both runs produce the same game kill at turn 519.
The change is in the Borg's received reaction information, not whether that
externally driven fight kills the monster.

For each unaffected control, compare the entire retained output in Bash:

```sh
for mode in heavy-stun paralysis none; do
    cmp <(git show "$c_evidence":docs/audit/c-reports/CBUG-0002/before-"$mode".log) \
        <(git show "$c_evidence":docs/audit/c-reports/CBUG-0002/after-"$mode".log) &&
        printf '%s: identical\n' "$mode"
done
```

Actual result:

```text
heavy-stun: identical
paralysis: identical
none: identical
```

These unchanged controls argue against a general change to parsing or combat.
Both full native gates also pass 965/965 unit checks and 5/5 frontend tests;
that is supporting regression coverage, not the reason to believe this bug.
The upstream unit executable returns exit 0 even on the failing case, so the
actual failure/pass summaries above and `KILL_REACTION` must be inspected.

## 6. Decide whether the scenario is persuasive, then rerun if wanted

[Immutable full recipe and interventions](https://github.com/diegoami/neo-angband-tracker/blob/3de1e3123915587483395dd40c4d2e75704e65f4/docs/audit/c-reports/CBUG-0002/README.md)
include copyable configure/build/run commands, the observer source, configuration,
observation patch, raw logs and hashes. To inspect the instrumentation first:

```sh
git show "$c_evidence":docs/audit/c-reports/CBUG-0002/scenario.c
git show "$c_evidence":docs/audit/c-reports/CBUG-0002/observation.patch
git show "$c_evidence":docs/audit/c-reports/CBUG-0002/run-modes.py
```

This is a fixed-seed **synthetic native integration scenario**, not a report
that an enemy naturally knocked out an autonomous Borg during desktop play.
It uses Human Warrior seed 1000, 20 birth HP and depth 0; enters checked stun
151 through the real timed API, lets the native game loop recover, places an
adjacent legal kobold, checks visibility, and drives normal movement/attack
commands. A headless renderer draws each engine message and drives the real
Borg screen scraper/input boundary. Those interventions are visible in the
source. The observer does not set the suppression flag/reaction counts,
synthesize combat message strings or force damage/death. All writable paths
in the final comparison are disposable; assertions, seeds and controls stay
unchanged across the actual before/after comparison.

The convincing claim is therefore deliberately specific: **a reachable knockout
message sets a gate that has no runtime reset; the native scenario then shows
lost post-recovery combat reactions, and correcting only the reset restores
them while the three controls remain identical.** No subsequent autonomous
strategy change, player death or lost save is demonstrated. If a human needs
those stronger consequences before filing, this evidence does not establish
them. The local proposal remains subject to the owner's judgment; it has not
been filed upstream or applied to a consumer.
