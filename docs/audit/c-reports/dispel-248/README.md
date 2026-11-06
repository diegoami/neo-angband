# Dispel Life was rated against nobody

A Necromancer considers Dispel Life against a hill giant five grids
away. The game's drain hits every living monster in line of sight, and a
native cast removes 66 HP from the giant.

**The Borg's rating kept damage only for monsters flagged both
nonliving and undead — the exact inverse of the game's rule — and since
no monster in the game data carries the nonliving flag, the spell was
rated below doing nothing against every enemy.** The original values it
at −21 (the launch failure minus the spell's mana cost); the corrected
filter, which zeroes damage for undead or nonliving monsters like the
game does, values it at 19.

Two situations stay unchanged: an explicitly undead vampire bat is
immune in the game (a native cast removes nothing) and both versions
refuse the cast; and the class's primary nether bolt, rated against the
same giant, produces a byte-identical complete log before and after the
correction.

Consequence: the Borg never considered casting a spell that works, and
mis-ranked it in every comparison that included it. No death or
survival claim is made.

Canonical report: [issue248](https://github.com/diegoami/neo-angband-tracker/issues/248).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/dispel-248-evidence
git show 77bddc19421eaff33c70f6cebc2588c8ca632ce8:docs/audit/c-reports/dispel-248/reproduce.py > /tmp/reproduce-dispel-248.py
python3 /tmp/reproduce-dispel-248.py --fixtures-ref 77bddc19421eaff33c70f6cebc2588c8ca632ce8 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the living-monster
filter, runs three recorded situations per variant, checks the
fail-before/pass-after matrix and the byte-identical control, records
both full gates and writes an offline `replay.html`. The native
verification ran with these exact generators except the replay
generator's syntax, which was completed afterwards; no native log,
source variant or gate differs.

Open [replay.html](replay.html) (download raw and open locally). Choose
**A living giant the spell really hurts**, then **The rating** and **The
native cast**. The original values the spell at −21 while the recorded
native cast removes 66 HP; the correction values it at 19. The other
two situations are controls.

Actual recorded output, original (`before-living.log`):

```text
GATE spell_okay=1 legal=1 temp_n=1
LAUNCH direct=-1
RATING value=-21 expected_average=39 clevel=26
OUTCOME hp_before=225 hp_after=159 hp_lost=66 undead=0
```

Corrected (`after-living.log`): `LAUNCH direct=39`,
`RATING value=19 ...`; the native cast line is unchanged (the game is
untouched by the fix). Undead control: value ≤ 0 in both versions and
`hp_lost=0`. Bolt control: byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`6912b85cb7b902616ad786dfc20afe3b4cf28b10`. One case in
`src/borg/borg-fight-attack.c`. The original zeroed the damage twice —
for monsters without the NONLIVING flag and again for those without
UNDEAD — leaving damage only for monsters with both. The game's drain
(project-mon.c MON_DRAIN with monster_is_nonliving = undead or
nonliving) damages each living monster; the correction zeroes the
damage for undead or nonliving monsters, matching it.

```sh
git fetch --no-tags origin task/c-borg-dispel-life-filter
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-attack.c | sed -n '580,596p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/mon-predicate.c | sed -n '35,55p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 6912b85cb7b902616ad786dfc20afe3b4cf28b10 -- src/borg/borg-fight-attack.c
```

### Conditions and limits

Native Human Necromancer level 26, depth 30, seed 1000, lit constructed
40x40 arena, the Fear and Torment shadow book, Dispel Life and Nether
Bolt learned, full mana, a torch, one awake monster. The production
rating call and its damage expression are used unchanged; the native
cast goes through the game's own command and its damage is measured on
the monster's hit points. Damage rolls are random; the rating is the
average expression. This isolates the rating filter, not full
autonomous play. No death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
