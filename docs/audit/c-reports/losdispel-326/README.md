# The dispel that stopped at nothing

A level fifty priest stands near the west edge of a lit arena with
three vampire bats to its east, full mana, and considers Dispel Undead.

**The LOS-dispel beam walk (the Borg's radius-ten convention, used by
the dispel prayers, the dispel and staff-holiness activations and the
tap spells) had no arrival break: it ran past its target to the maximum
range and exited through the excessive-distance guard, discarding
everything it had counted.** The blast scan that values the effect over
every monster in sight — the whole point of a dispel — ran only by
accident, when the walk happened to break at the cave edge first. In
this fixture every walk runs to the maximum range, the launcher returns
its initial minus one instead of zero, and the dispel is valued at
**−15**: the leaked −1 minus the spell cost.

The correction is three tokens: break the walk at the target like a
ball does, take the path damage for bolts and beams only (where it
belongs — counting the path as well would double-count every monster
between the Borg and its target), and return zero when no candidate
survived. The blast scan then runs as written and values the bats at
**534**.

This explains the minus-one raw launch values recorded during the
dispel-259 and activ-260 work, and it affects every radius-ten
valuation in the game: in open sight lines dispels were worth the
leaked −1 (or zero), and behind walls they were worth only the path
sum — the area-of-sight model never ran.

Control: the holy orb ball, whose walk already breaks at its target,
is valued identically at 540 before and after with byte-identical
logs — the correction changes only the LOS-dispel walk and the
launcher leak, not the bolt, beam or ball machinery.

Consequence: the Borg's entire LOS-dispel family was systematically
miscued. No death or survival claim is made.

Canonical report: [issue326](https://github.com/diegoami/neo-angband-tracker/issues/326)
(native finding, discovered and measured during the dispel-259 and
activ-260 work on this board). Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/losdispel-326-evidence
git show 6a46c46bd29192e1ccc17c5212748ed29be04020:docs/audit/c-reports/losdispel-326/reproduce.py > /tmp/reproduce-losdispel-326.py
python3 /tmp/reproduce-losdispel-326.py --fixtures-ref 6a46c46bd29192e1ccc17c5212748ed29be04020 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the three tokens (the
arrival break, the path-damage scoping and the launcher leak guard),
runs two recorded situations per variant, checks the fail-before and
pass-after matrix and the byte-identical control, records both full
gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The dispel that stopped at nothing**: the original values the dispel
at −15; the correction at 534.

Actual recorded output, original (`before-storm.log`):

```text
GEOMETRY player=(5,20) bats=(10,20)(11,19)(11,21) temp_n=3
RATING dispel=-15
OUTCOME dispel=-15
```

Corrected (`after-storm.log`): `RATING dispel=534`. Control
(`control`): `RATING orb=540` in both variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`53ef84c743768fdde9abaebc27bd8526b4423de9`. Three tokens in
`src/borg/borg-fight-attack.c`. In `borg_launch_bolt_at_location()`,
the beam walk's arrival break excluded radius ten
(`if ((rad != -1 && rad != 10) && arrival) break;`), so a dispel walk
never stopped at its target; the path-damage collection included
radius ten (`if (rad <= 0 || rad == 10)`) although the dispel damage
belongs to the blast scan alone; and in `borg_launch_bolt()` the
no-survivor tail returned the initial `b_n` of −1 instead of zero (the
missile launcher has that guard). Instrumented probes recorded the
excessive-distance exit discarding path sums of 187, 222 and 3996 in a
single fixture run — the walk counts the effect and then throws it
away.

```sh
git fetch --no-tags origin task/c-borg-dispel-los-walk
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-attack.c | sed -n '1495,1510p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 53ef84c743768fdde9abaebc27bd8526b4423de9 -- src/borg/borg-fight-attack.c
```

### Conditions and limits

Native Human Priest level fifty, depth thirty, seed one thousand, lit
constructed 40x40 arena, every prayer book carried, wisdom maximized,
full mana, three awake vampire bats east of the priest near the west
edge so that every beam walk runs to the maximum range rather than the
cave edge (the accidental path by which the blast scan could run
before). The measurement calls the public maneuver chooser with
`borg_simulate` set. This isolates the radius-ten walk and the
launcher leak, not full autonomous play. No death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
