# Acid protection can make the Borg stay beside a cold-biting troll

A Rogue has temporary protection from acid, but not cold, while standing beside
a snow troll. **The Borg treats that acid protection as cold protection and
stays. In the test's scheduled attack it loses 21 HP. After the correction it
casts Phase Door, moves away and avoids that attack, spending 2 mana.** Phase
Door is a short teleport.

The reverse case must be considered too. With cold protection alone, the
original spends2 mana escaping; the correction stays and spends no mana, but
takes20 HP in the scheduled attack. Cold protection does not remove the physical
part of bites or the troll's claws. This is a measured HP/mana tradeoff, not a
claim that the correction always improves survival.

The correction makes the Borg check the protection that actually applies to
cold. **Priority is medium**, reduced from the imported high priority: one
controlled escape choice and its consequence are demonstrated, not encounter
frequency, a whole fight, a death or overall survival benefit.

Canonical report: [issue26 / CBUG-0004](https://github.com/diegoami/neo-angband-tracker/issues/26).
Discovery credit: borg-rs. This is independent native C verification; no Rust
repository watch or downstream work was performed.

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/cbug-0004-evidence
git show FETCH_HEAD:docs/audit/c-reports/CBUG-0004/reproduce.py > /tmp/reproduce-cbug-0004.py
python3 /tmp/reproduce-cbug-0004.py
```

The script extracts every required file, creates fresh temporary source/build/
profile directories, builds both versions of the native C game, runs six cases,
verifies their results and generates an offline `replay.html`. It prints the
path: open that file in your browser. No manual patch downloads, placeholder
paths or changes to your existing checkout/profile are needed.

Select **Acid protection only**, then **Start**, **Show decision** and **Show
attack**. Both versions begin with130 HP and121 mana. The original stays beside
the troll and ends at109 HP. The correction teleports10 squares away, ends
at130 HP and has119 mana. The attack is made only when the troll remains next
to the player. HP and positions are recorded from the native game; damage is
not calculated by the replay.

Then select **Cold protection only: show the tradeoff**. The original teleports
and keeps130 HP for2 mana. The correction stays, keeps its mana and ends
at110 HP. This counterexample is deliberately shown prominently.

| Situation | Original choice and consequence | Corrected choice and consequence |
| --- | --- | --- |
| Acid protection only | Stays; loses21 HP, spends0 mana | Teleports; loses0 HP, spends2 mana |
| Cold protection only | Teleports; loses0 HP, spends2 mana | Stays; loses20 HP, spends0 mana |
| Neither protection | Teleports; loses0 HP, spends2 mana | Identical |
| Both protections | Stays; loses20 HP, spends0 mana | Identical |
| Scorpion, acid protection | Uses one Phase Door scroll; loses0 HP | Identical |
| Scorpion, cold protection | Uses one Phase Door scroll; loses0 HP | Identical |

Two changed-choice cases fail the intended-choice assertions before correction;
all six pass afterwards. Passing the cold-only assertion means it recognizes
the existing cold protection, not that its20-HP tradeoff is a combat improvement.
Four controls have byte-identical before/after logs.

To run both complete native regression gates as well:

```sh
python3 /tmp/reproduce-cbug-0004.py --full-tests
```

Both gates report965/965 unit checks and5/5 frontend tests, with no relevant
skips. The native unit executable can return exit0 even when its assertion
fails. The Python runner checks the actual completion/assertion summaries,
changed decisions, HP/mana outcomes and controls, not just exit codes.

Prerequisites: Git, Python3, CMake, a C compiler/build toolchain, util-linux
(`flock`) and native platform dependencies. This Linux configuration uses X11
development headers/libraries. Heavy steps wait on `/tmp/angband-validation.lock`
and print their log paths. Optional `--cmake-arg=...` options support task-local
build dependencies. This cloud used task-local CMake4.4.4 and extracted Debian
X11 development packages because system package installation is unavailable.

### Why the correction changes the choice

Pristine C/official master: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`.
Proposed C source: `d504eb3d272ce96c0de2c00eba757a251b551334`.
Compiler: Debian GCC14.2.0; Debug with Borg and the test frontend enabled.

```sh
git fetch --no-tags origin task/c-borg-cold-melee-resistance
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-danger.c | sed -n '262,276p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-trait.c | sed -n '3017,3024p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-escape.c | sed -n '1000,1022p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 d504eb3d272ce96c0de2c00eba757a251b551334 -- src/borg/borg-danger.c
```

The cold melee case checks `borg.temp.res_acid` before reducing the estimate.
The player-state update separately maps native `TMD_OPP_ACID` to that field
and native `TMD_OPP_COLD` to `borg.temp.res_cold`. They represent different
protections. The one-line change checks `res_cold` in the cold case. It changes
no native damage formula, RNG call, other element or escape threshold.

The actual acid-only danger estimate is52 before and116 afterwards. In this
level20/130-HP state the existing escape condition is danger greater than
`130 * 8 / 10 = 104`.52 does not pass;116 does. The real escape routine checks
landing safety and then requests Phase Door. This explains the changed action;
the separate native attack below establishes the HP consequence. Danger scores
are not HP damage and must not be presented as such.

With cold protection alone, the estimate changes in the opposite direction,
116 to52. The existing policy then accepts staying. We do not equate that
policy result with a measured survival benefit.

### Actual relevant output

Original acid-only case:

```text
DANGER current=52 escape_threshold=104
DECISION escape=0 keys=0 sequence=
ENGINE The snow troll claws you.
ENGINE The snow troll claws you.
ENGINE You are covered with frost!
ENGINE The snow troll bites you.
ENGINE You are covered with frost!
ENGINE The snow troll bites you.
OUTCOME escape=0 moved=0 distance=1 melee=1 hp_before=130 hp_after=109 hp_lost=21 mana_spent=0 scrolls_spent=0
```

Corrected acid-only case:

```text
DANGER current=116 escape_threshold=104
DECISION escape=1 keys=3 sequence=mad
OUTCOME escape=1 moved=1 distance=10 melee=0 hp_before=130 hp_after=130 hp_lost=0 mana_spent=2 scrolls_spent=0
```

These show the actual requested escape and its native execution, followed by
the legally scheduled melee opportunity. The source condition and measured
threshold explain why the choice changes; test names or green summaries alone
would not establish that explanation.

### Unchanged conditions, interventions and limits

All cases use seed1000, a Human Rogue, native level20/130 maximum HP, depth28,
121 native maximum mana, a known Phase Door spell and carried scroll, a legal
+6 Intelligence ring, no missiles and a lit40x40 arena. The fixture grants
experience and raises base Intelligence through native APIs. It constructs
one visible adjacent monster, identifies items, sets the named native timed
resistances to50 and obtains the Borg's traits through its normal update.
The Borg's internal preparation age is constructed as100, beyond the existing
50-step anti-summon cooldown. This models eligibility; it is not a record of
100 simulated world turns. Monster stats and speed are obtained from the
game; the snow troll has native speed111 in this fixed-seed setup.

The same production `borg_escape()` is called with the actual `borg_danger()`
result in both versions. Its requested spell/book keys or scroll keys are
decoded and checked against real inventory/known spells, then executed using
native `do_cmd_cast()` or `do_cmd_read_scroll()`. Two leading Escape keys in
the scroll controls cancel interface state; they do not consume resources.
No spell choice, hit, damage amount, success result or RNG seed is substituted.
Native Phase Door failure chance is5%; this fixed-seed cast succeeds, which is
not a guarantee for every cast in normal play.

After that decision the driver schedules one normal monster melee action
only if it is still adjacent. `make_attack_normal()` determines which blows
hit and the real damage; no damage value is forced. If the Borg escapes,
there is no adjacent melee attack. The remainder of the caution loop, player
fallback actions, monster pursuit and full autonomous/UI turn scheduling are
not run. The21-HP difference is a measured consequence of this isolated choice,
not proof that a complete Borg run loses that amount in every encounter.

Cold protection does not eliminate all attack damage. Native melee handling
uses the larger of the bite's physical and elemental damage, and claws also
remain. Inspect that unchanged rule if the cold-only20-HP result is surprising:

```sh
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/mon-blows.c | sed -n '417,475p;982,985p'
```

The source issue remains open pending upstream acceptance and normal import/
checks. A stored patch or our local PR is not source adoption. Owner filing
material is on issue26; agent posting to angband/angband remains paused.
