# Teleport Other skips a monster it is not going to move

A level-30 Mage on depth 100 considers Teleport Other with a kobold
beside it and a hill giant to the other side. The Borg first runs its
special Morgoth-depth Teleport Other evaluation, which — with Morgoth
assumed present on depth 100 and little mana left — stores each listed
monster twice in its skip list, writing the second slot. The regular
Teleport Other evaluation then targets the kobold, whose candidate list
holds one entry.

**The danger check for that candidate walks one entry past the list and
also suppresses the hill giant left over in the second slot. The cast is
valued at 144 — as if it removed the kobold and the giant — when the
targeted candidate removes only the kobold (16).** The corrected read
stops at the listed entries and values the cast at the winning
candidate's own list, 128 (the side giant is the better target).

Measured transition: original 144, corrected 128. Two controls stay
byte-identical before and after: without the Morgoth-depth pass the
second slot is never written, and when only one monster exists the
leftover slot holds that same monster, so a duplicate is harmless.

Consequence: the Borg over-values Teleport Other (and any decision
priced from that danger), expecting relief from a monster the cast does
not remove. No death, frequency or survival claim is made.

Canonical report: [issue67 / CBUG-0051](https://github.com/diegoami/neo-angband-tracker/issues/67).
Lead supplied by borg-rs; independently validated and reproduced
natively here. The separate unbounded-writer report
[issue250](https://github.com/diegoami/neo-angband-tracker/issues/250)
is triaged unreachable in native C (kill table capped at 256; see its
triage comment). Source proposal [PR285](https://github.com/diegoami/neo-angband-tracker/pull/285).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/cbug-0051-evidence
git show EVSHA:docs/audit/c-reports/CBUG-0051/reproduce.py > /tmp/reproduce-cbug-0051.py
python3 /tmp/reproduce-cbug-0051.py --fixtures-ref EVSHA --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the one bounded read,
runs three recorded situations per variant, checks the fail-before/
pass-after matrix and byte-identical controls, records both full
regression gates and writes an offline `replay.html`. Your checkout and
character/profile are not used or edited.

Open [replay.html](replay.html) (download raw and open locally). Choose
**Morgoth-depth pass leaves a stale skip entry**, then **Danger of each
monster**, **Morgoth-depth pass** and **Regular evaluation**. The
original values the cast at 144 with the leftover giant suppressed; the
correction values it at 128. The other two situations are controls
whose logs match byte for byte.

Actual recorded output, original (`before-two-phase.log`):

```text
KILL index=1 pos=(11,12) awake=1 danger=16
KILL index=2 pos=(12,11) awake=1 danger=128
DANGER p1=144 strong=128 weak=16
TEMP phase1 n=2
MORGOTH p2=150 slots_after slot0=2 slot1=2 n=0
TEMP phase2 n=2
LAUNCH p2=144
OUTCOME p2=144 expected_best=128 strong=128 weak=16 tp_other_n=0
```

Corrected (`after-two-phase.log`): `LAUNCH p2=128`,
`OUTCOME p2=128 ...`; every other line identical.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`25f8a43828204dd1226d5bf5c3b36547b4a07a40`. One comparison in
`src/borg/borg-danger.c`. The skip-list check iterated
`ii <= borg_tp_other_n`; valid entries are 0..n-1. Entry n is whatever
an earlier target candidate left there: the Morgoth-depth evaluation
(`BORG_ATTACK_AWAY_ALL_MORGOTH` in borg-fight-attack.c) can store the
same monster twice per candidate (Morgoth-present and low-mana
branches), so the second slot is written and survives the per-candidate
reset of the count. A later, shorter candidate then suppresses that
leftover monster as if it were about to be teleported away. The
correction reads only the listed entries.

### Conditions and limits

Native Human Mage level 31, depth 100, seed 1000, lit constructed
40x40 arena, Teleport Other known, a kobold and a hill giant awake and
observed, full mana for the regular evaluation and 30 mana during the
Morgoth-depth pass (the low-mana branch needs at most 35). The Borg's
own depth-100 assumption that Morgoth is present (borg-update.c sets
`near.morgoth` there) is set directly to match. Both launch calls are
the production calls from the two Teleport Other defenses; the target
list is filled the way the defense fills it. This isolates the
skip-list read, not full autonomous play or monster AI casting. Danger
figures are the Borg's heuristic, not HP predictions. No death or
survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
