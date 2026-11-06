# The phantom in his own boots

After a teleport plus a blindness transition, a tracked monster entry
can still point at the grid the player now occupies — and the melee
chooser accepted it.

**`borg_attack_aux_thrust()` accepted candidate monsters at distance
zero as "adjacent". No real monster can share the player's grid, so
such an entry is stale tracking — and selecting it queued the
center-square key '5' for the attack direction, which is not a
direction and trips the unexpected-direction abort that stops the
Borg.** Measured natively through the production attack chooser in
acting mode: with the tracked entry on the player's own grid the
original attacks (`acted=1 keys="+5"`); the correction requires
distance exactly one, queues nothing, and the run continues.

Control: a genuinely adjacent kobold one grid south is attacked
identically in both variants (`+2`, byte-identical logs) — the
correction rejects only the impossible own-grid case.

Consequence: the recorded in-game abort — the Borg stopped after a
teleport with a stale monster on its grid (the supplied extended-run
evidence). The fixture measures the decision that produces it, not
the full replay.

Canonical report: [issue322](https://github.com/diegoami/neo-angband-tracker/issues/322).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/staletarget-322-evidence
git show PINSHA:docs/audit/c-reports/staletarget-322/reproduce.py > /tmp/reproduce-staletarget-322.py
python3 /tmp/reproduce-staletarget-322.py --fixtures-ref PINSHA --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the adjacency check,
runs the stale-target and control situations per variant through the
production attack chooser in acting mode, checks the fail-before and
pass-after matrix and the byte-identical control, records both full
gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The phantom in his own boots**: the original queues +5 — an attack on
the player's own grid; the correction queues nothing.

Actual recorded output, original (`before-stale.log`):

```text
GEOMETRY target=(11,11) player=(11,11) dist=0
RATING acted=1 keys="+5" center_attack=1
```

Corrected (`after-stale.log`): `RATING acted=0 keys=""
center_attack=0`. Control (`control`): `RATING acted=1 keys="+2"
center_attack=0` in both variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`1ab30ef0bc9a12caa5b1cf48e60c204ef564e9de`. One check in
`src/borg/borg-fight-attack.c`. The candidate filter required distance
at most one, accepting zero; `borg_extract_dir()` maps a same-grid
target to the center square 5, which is not an attack direction. The
correction requires distance exactly one.

```sh
git fetch --no-tags origin task/c-borg-thrust-adjacent
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-attack.c | sed -n '288,292p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 1ab30ef0bc9a12caa5b1cf48e60c204ef564e9de -- src/borg/borg-fight-attack.c
```

### Conditions and limits

Native Human Warrior on a lit arena at depth 30, seed one thousand, a
Broad Sword wielded, one observed awake Kobold. The stale situation
moves the tracked entry onto the player's grid — exactly the
post-teleport state the supplied run evidence recorded; the abort
itself happens later, when the empty direction prompt is answered, so
this fixture measures the decision that produces it. No death claim;
the recorded consequence is the stopped Borg.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
