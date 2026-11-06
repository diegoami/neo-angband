# Watch the extra spell and its mana cost

The Borg should compare preparation spells, choose one and cast it once.
Instead, considering **Rapid Regeneration** already adds a cast to its actions.
It then adds the spell it actually chose. Regeneration makes health recover
faster; **Haste Self** makes the character faster.

This produces either two regeneration casts, or regeneration before a chosen
haste spell. [PR264](https://github.com/diegoami/neo-angband-tracker/pull/264)
changes the game code to cast only the chosen preparation.
[PR265](https://github.com/diegoami/neo-angband-tracker/pull/265) holds the test
recipe and results for that same bug. They are not two different fixes.

## Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/cbug-0094-evidence
git show FETCH_HEAD:docs/audit/c-reports/CBUG-0094/review/reproduce.py > /tmp/reproduce-cbug-0094.py
python3 /tmp/reproduce-cbug-0094.py
```

The script extracts the required files, creates fresh temporary source/build/
profile directories, builds the native C game before and after the correction,
runs the nine cases, verifies their outcomes and generates an offline
`replay.html`. It prints the path: open that file in your browser. No manual
patch downloads, placeholder paths or changes to your checkout/profile are
needed. Git fetch updates only the fetched objects.

In the replay choose **Haste is chosen**, then click **Start**, **Show first
cast** and **Show final result**. Both versions begin with **401 mana** and
full health. The original casts regeneration first, then haste, ending at
**369 mana**. The corrected version casts haste only and ends at **389 mana**.
The chosen haste spell still works; the extra regeneration cast is removed.

Then choose **Regeneration is chosen**. The original casts it twice and ends
at **361 mana**; the correction casts it once and ends at **381 mana**.

| Spell chosen | Original actions and mana left | Corrected actions and mana left |
| --- | --- | --- |
| Regeneration | Regeneration, regeneration; **361** | Regeneration; **381** |
| Haste Self | Regeneration, haste; **369** | Haste; **389** |

In both situations the correction removes **one extra cast costing 20 mana**.
The page uses the native game's recorded spell names, mana values and messages.
It is not an invented mana calculation, combat animation or live game window.
The raw captured output is available on the page if you want to inspect it.

The other seven choices in the selector are controls: haste without regeneration
learned, regeneration already active, regeneration unlearned, blindness,
confusion, town and insufficient spare mana. Their before/after logs are
identical. Both affected cases fail before the correction; all nine pass after.

To also run both complete native regression gates:

```sh
python3 /tmp/reproduce-cbug-0094.py --full-tests
```

Expected full-gate result: **965/965 unit checks and 5/5 frontend tests** before
and after. Inspect the actual pass/fail summaries: the native test executable
can return exit0 even when its assertion fails. The Python runner checks the
summaries and outcomes rather than trusting that exit code.

Prerequisites: Git, Python3, CMake, a C compiler/build toolchain, `flock` and
native platform dependencies (this Linux configuration needs X11 development
headers/libraries). Builds wait on the shared validation lock and print their
log paths. The runner accepts additional CMake arguments for task-local build
dependencies, just like the original reproduction runner.

## What this establishes, and what it does not

The old code pays for an extra cast that the decision did not select. The
correction makes its commands match the selected spell. That is the measured
resource/action consequence. Priority remains medium.

A second regeneration cast extends its duration. These results therefore do
**not** establish that the extra cast is always harmful or that the corrected
character wins a fight it otherwise loses. There is no demonstrated death,
encounter frequency or full autonomous run. This evidence is narrower than
the HP-damage evidence on the owner-accepted swap report226/82.

The fixture constructs a Human Druid with the relevant book, learned spells,
full native HP/mana, high Wisdom and a legal Wisdom amulet. The real Borg
preparation code selects and requests the actions. The adapter decodes the
requested book/spell keys and invokes native casting commands. The real game
computes success, mana use and spell effects. World turns and monsters are not
advanced between commands; counted casts are not measured extra enemy attacks.

The original [full technical verification](README.md), source proposal,
recorded logs and receipt remain unchanged. The source correction is still
`31a042d00581ed914d30f17cca1ed0e70e99d21f`, based on pristine C
`e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. The original evidence publication
is `a19c8733c0dfa8139b04eecd8117429ec5f55cd6`. This addition changes how the
evidence is presented and replayed, not the game fix or the historical results.
