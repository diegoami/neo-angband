# Half out of lunal mode

The lunal mode — the Borg's fast, reckless rush — is entered and left
as a pair of flags. The stair-scum bail cleared only one of them.

**`borg_think_stair_scum()`'s town-or-weak bail announced "Leaving
Scumming Mode" and cleared `borg.lunal_mode`, but left the companion
flag `borg.goal.fleeing_lunal` set — the caution phase and the
flow-recovery rest check kept treating the Borg as lunal while the
mode flag said otherwise.** Measured natively with both lunal flags
set through the production bail in town: the original leaves
`lunal_mode=0 fleeing_lunal=1` (split); the correction clears the
pair, as the normal lunal exit does.

Control: the full-pack bail returns before any flag is touched — both
halves stay set in both variants, byte-identical logs.

Consequence: the Borg behaves half-lunal after bailing — the caution
phase still skips its caution while the mode flag says the mode is
over. The measured fact is the flag state; the downstream behaviour is
the report's mechanism, not claimed here. No death or survival claim.

Canonical report: [issue314](https://github.com/diegoami/neo-angband-tracker/issues/314).
Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/lunal-314-evidence
git show 0995f346e3b7ff050576376836b4bd625ddd9f00:docs/audit/c-reports/lunal-314/reproduce.py > /tmp/reproduce-lunal-314.py
python3 /tmp/reproduce-lunal-314.py --fixtures-ref 0995f346e3b7ff050576376836b4bd625ddd9f00 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the pair-clearing bail,
runs the town bail and the full-pack control per variant through the
production stair-scum routine, checks the fail-before and pass-after
matrix and the byte-identical control, records both full gates and
writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**Half out of lunal mode**: the original leaves the state split; the
correction clears the pair.

Actual recorded output, original (`before-town.log`):

```text
GEOMETRY lunal_mode=0 fleeing_lunal=1
RATING lunal_mode=0 fleeing_lunal=1
```

Corrected (`after-town.log`): `RATING lunal_mode=0 fleeing_lunal=0`.
Control (`pack`): `RATING lunal_mode=1 fleeing_lunal=1` in both
variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`a26da54d61be83236c2c5859141b801521d6c6d5`. One line in
`src/borg/borg-think-dungeon-util.c`: the town-or-weak bail also
clears `borg.goal.fleeing_lunal`, matching the paired lunal exit in
`borg-think-dungeon.c`.

```sh
git fetch --no-tags origin task/c-borg-scum-exit-clears-pair
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-think-dungeon-util.c | sed -n '360,370p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 a26da54d61be83236c2c5859141b801521d6c6d5 -- src/borg/borg-think-dungeon-util.c
```

### Conditions and limits

Native Human Warrior, seed one thousand, with both lunal flags set,
running the production stair-scum bail in town (or the full-pack bail
at depth 2). The measured fact is the flag state after the bail. No
death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
