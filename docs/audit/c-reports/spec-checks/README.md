# Source-level checks for the C Borg findings

These are the standalone C programs used to check the branch, counter and
formula claims in the C Borg findings filed as
[#310](https://github.com/diegoami/neo-angband-tracker/issues/310) through
[#316](https://github.com/diegoami/neo-angband-tracker/issues/316).

Each one **transcribes the relevant code verbatim** from the upstream C
reference (`angband/angband` `master` at
`e473720ed88a0f0c7ca66815cef3000eb4a38bc3`) and runs it over synthetic
inputs.

They are **source-level checks, not game runs**. A program here proves that
the transcribed code does what the finding says; it does not prove that the
game reaches that state. Each finding says which of the two it has, and
carries the label that records it.

## Running them

```sh
cd docs/audit/c-reports/spec-checks
for f in *.c; do
    gcc -O0 -Wall -Wextra -o "/tmp/${f%.c}" "$f" && "/tmp/${f%.c}"
done
```

## What each one checks

| Program | Finding | What it shows |
| --- | --- | --- |
| `recall_branch_test.c` | #310 | the branch whose comment says "Do reset Depth" queues `n` |
| `glow_bound_test.c` | #311 | the map panel is larger than the 1536-entry glow arrays |
| `glow_overflow_probe.c` | #311 | the overflow swaps the two arrays' coordinates and yields 212 off-map flow targets |
| `surrounded_test.c` | #312 | the trapped-wall double count flips the surrounded verdict |
| `blows_test.c` | #313 | the stack weight changes the blows (2 where the engine gives 3) |
| `recharge_test.c` | #316 | an unwearable pack wand is selected and the routine gives up |
| `teststuff_test.c`, `teststuff_test3.c` | #315 | the identify-priority score overwrites the artifact bonus |

### `glow_overflow_probe.c` and the layout it depends on

This one depends on the **link order** of the declarations, because that is
what makes the two arrays contiguous in `.bss`:

```c
int16_t borg_glow_n = 0;
uint8_t borg_glow_x[AUTO_LIGHT_MAX];
uint8_t borg_glow_y[AUTO_LIGHT_MAX];
```

The probe declares them in the same order as
`src/borg/borg-cave-light.c:42-45`, so the compiler lays them out the same
way, and it reproduces the interleaved write of `borg_flow_light()` and its
read-back loop. The layout can be confirmed against the real binary:

```sh
nm -S game/angband | grep -E ' borg_glow| borg_light'
```

which shows `borg_glow_y` ending exactly where `borg_glow_x` begins.

## How the transcriptions were checked

Each transcription was compared against the upstream source line by line,
and the data tables and class parameters were taken from the pinned tree
rather than from memory:

* `blows_test.c` uses `adj_str_blow` and `blows_table` verbatim from
  `src/player-calcs.c:548` and `:889`, and the Warrior's parameters from
  `lib/gamedata/class.txt`.
* `glow_bound_test.c` uses `row_top_map`, `row_bottom_map` and `col_map`
  verbatim from `src/ui-term.c:305-307`, and `AUTO_LIGHT_MAX` from
  `src/borg/borg-cave-light.h:30`.

## The runtime side

The runtime evidence is not in this directory. It comes from the native C
game built from the upstream commit with `SUPPORT_BORG=ON` and the headless
deterministic Borg harness (`SUPPORT_BORG_HARNESS=ON`), which logs the
Borg's keypress stream and its full message/decision trace. The
specification's §18 gives the exact commands and the isolation arrangement
that keeps a run off the shared user directory.
