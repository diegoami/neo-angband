# Source-level checks for the C Borg findings

These are the standalone C programs used to check the branch, counter and
formula claims in the C Borg findings filed as
[#310](https://github.com/diegoami/neo-angband-tracker/issues/310) through
[#316](https://github.com/diegoami/neo-angband-tracker/issues/316), and the
corresponding examples in the specification
([`docs/c-borg/BORG_SPEC.md`](../../../c-borg/BORG_SPEC.md) §10.5, §11).

Each one **transcribes the relevant code** from the upstream C reference
(`angband/angband` at `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`) — the
lines it transcribes are named at the top of each file — and runs it over
synthetic inputs.

They are **source-level checks, not game runs**. A program here shows what
the transcribed code does with the inputs it is given; it does not show that
the game reaches those inputs. Where an input is one the game does not
produce in ordinary play, the program says so (`SYNTHETIC`, or a note in
its output). [RECHECK.md](RECHECK.md) records the last full run.

## Running them

```sh
cd docs/audit/c-reports/spec-checks
for f in *.c; do
    gcc -O0 -Wall -Wextra -o "/tmp/${f%.c}" "$f" && "/tmp/${f%.c}"
done
```

All eight compile without warnings at `-O0 -Wall -Wextra`.

## What each one checks

| Program | Finding | What it shows |
| --- | --- | --- |
| `recall_branch_test.c` | #310 | in the endgame branch the comment says "Do reset Depth" while the key is `'n'` and the note is "Not Resetting recall depth"; every control reaches the branch it is labelled with |
| `glow_bound_test.c` | #311 | the map panel can hold more grids than the 1536-entry glow arrays, by terminal size and sidebar mode (capacity only) |
| `glow_overflow_probe.c` | #311 | for **one named build** (RelWithDebInfo at the pin), what the overflow corrupts, simulated from that build's `nm -S` layout and code: wrong `x` coordinates near the start of the list, none off the panel, and `borg_light_y[]` overwritten |
| `surrounded_test.c` | #312 | a grid that satisfies the chain and the separate trap test is counted twice; the ordinary case is a monster on a known trap |
| `blows_test.c` | #313 | weighing a home stack instead of one weapon lowers the blow count (1 where the engine gives 2, in the table's example) |
| `recharge_test.c` | #316 | the last item with a timeout wins; the give-up path needs a synthetic input (a pack wand with a timeout) |
| `teststuff_test.c` | #315 | the `+20000` assignment fires for every ordinary identify candidate, so the artifact and ego bonuses do not survive; the exception is a cursed non-artifact in the weapon or bow slot (value 0, as `borg_item_analyze()` sets it); an artifact that is also an ego is marked `SYNTHETIC` (ordinary generation never makes one) |
| `teststuff_test3.c` | #315 | loop 1 changes loop 2's choice only through a tie (the equipment item is kept) or through the cursed-slot exception |

### `glow_overflow_probe.c` and layouts

What an out-of-bounds write corrupts depends on the build. The probe does
**not** rely on the layout its own compiler chooses: it simulates, in a
byte arena, the layout reported by `nm -S` for one build of the pinned tree
and the code that build generates (the counter is held in a register during
the scan and stored once after it). The build, the `nm` output and the
`objdump` evidence are in the probe's header comment and in
[RECHECK.md](RECHECK.md). A different build can give a different result;
the specification relies only on the overflow itself.

## How the transcriptions were checked

Each transcription was compared with the pinned source line by line
(`origin/c-borg-reference`), and the tables and class parameters come from
the pinned tree:

* `blows_test.c`: `adj_str_blow` (first 12 entries) and `blows_table` from
  `src/player-calcs.c`; `blows_table` is identical to `borg_blows_table` in
  `src/borg/borg-trait.c`; the Warrior's `max-attacks:6`, `min-weight:30`,
  `strength-multiplier:5` from `lib/gamedata/class.txt`.
* `glow_bound_test.c`: `row_top_map`, `row_bottom_map`, `col_map` from
  `src/ui-term.c`, `SIDEBAR_*` and the `SCREEN_*` macros from
  `src/ui-term.h`, and `AUTO_LIGHT_MAX` from `src/borg/borg-cave-light.h`.
* `teststuff_test.c`: `borg_item_note_needs_id()` and `borg_item_worth_id()`
  are transcribed, not taken as inputs, because their results follow from
  `ident`/`needs_ident`.

[`recheck/`](recheck/) holds the scripts that re-check the specification
(identifiers, citations, transcriptions) and `adversarial_fixture.txt`,
deliberately false material used to test what those scripts catch.

## The runtime side

The runtime evidence is not in this directory. The harness commands and the
isolation arrangement are in [PROVENANCE.md](PROVENANCE.md).
