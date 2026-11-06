# The fear spell that promised confusion

A level fifty necromancer stands surrounded by four young red dragons
and considers its blind-creature maneuver — which casts Frighten.

**The maneuver was scored with the confusion attack type: the Borg's
danger model suppresses a confused dragon massively (the probe records
a confusion delta of 408 against a fear delta of zero), so a fear
spell was valued at the confusion it cannot deliver — 955, high enough
to win the chooser and be cast.** The correction scores the cast as
the fear effect it is (`BORG_ATTACK_TURN_ALL`, the type the dedicated
Frighten maneuver already uses): the maneuver values −5, **exactly what
the dedicated Frighten maneuver values** — the corrected contract the
scenario asserts.

Control: the dedicated Frighten maneuver, already scored as fear,
values −5 before and after with byte-identical logs — the fix changes
only the mislabeled maneuver.

Consequence: in exactly the situations where confusion is tactically
precious — dangerous adjacent monsters — the Borg could cast Frighten
believing it confuses, then act on a model of a confused monster that
is actually afraid. No death or survival claim is made.

Canonical report: [issue261](https://github.com/diegoami/neo-angband-tracker/issues/261).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/blind-261-evidence
git show PINSHA:docs/audit/c-reports/blind-261/reproduce.py > /tmp/reproduce-blind-261.py
python3 /tmp/reproduce-blind-261.py --fixtures-ref PINSHA --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the fear-scoring token,
runs two recorded situations per variant, checks the fail-before and
pass-after matrix and the byte-identical control, records both full
gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally).
Choose **The fear spell that promised confusion**: the original values
the maneuver at 955; the correction at −5, matching the dedicated
maneuver.

Actual recorded output, original (`before-blind.log`):

```text
FEARPROBE p1=488 p2=488 fear_delta=0 conf_delta=408
RATING blind=955 frighten=-5
OUTCOME blind=955 frighten=-5
```

Corrected (`after-blind.log`): `RATING blind=-5 frighten=-5`. Control
(`control`): `RATING frighten=-5` in both variants, byte-identical
logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`a49b7a6c15f6353cd427af28903355fe258af4d5`. One token plus a comment
in `src/borg/borg-fight-attack.c`. The `BF_SPELL_BLIND_CREATURE` case
casts FRIGHTEN — a fear effect — but passed `BORG_ATTACK_OLD_CONF` to
the launcher, whose damage table then computed the confusion
suppression instead of the fear suppression. The dedicated
`BF_SPELL_FRIGHTEN` case (casting the same spell) already uses
`BORG_ATTACK_TURN_ALL`; the correction matches it.

```sh
git fetch --no-tags origin task/c-borg-frighten-scored-as-fear
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-attack.c | sed -n '3924,3932p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 a49b7a6c15f6353cd427af28903355fe258af4d5 -- src/borg/borg-fight-attack.c
```

### Conditions and limits

Native Human Necromancer level fifty, depth thirty, seed one thousand,
lit constructed 40x40 arena, every class book carried, four awake
young red dragons adjacent, avoidance tuned low. The probe records the
danger model deltas directly: confusion suppresses each dragon by 408
while fear suppresses nothing here. The measurement calls the public
maneuver chooser with `borg_simulate` set. This isolates the
attack-type token, not full autonomous play. No death or survival
claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
