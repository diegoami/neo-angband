# Source-level checks for the C Borg findings

These are the standalone C programs used to check the branch, counter and
formula claims in the C Borg findings. Each one **transcribes the relevant
code verbatim** from the pinned C reference
(`angband/angband` at `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`) and runs
it over synthetic inputs.

They are **source-level checks, not game runs**. A program here proves that
the transcribed code does what the finding says; it does not prove that the
game reaches that state. The findings say which of the two they have.

## Running them

```sh
cd docs/c-borg/checks
for f in *.c; do
    gcc -O0 -Wall -Wextra -o "/tmp/${f%.c}" "$f" && "/tmp/${f%.c}"
done
```

## What each one checks

| Program | Finding | What it shows |
| --- | --- | --- |
| `recall_branch_test.c` | #310 | the branch whose comment says "Do reset Depth" queues `n` |
| `glow_bound_test.c` | #311 | the map panel is larger than the 1536-entry glow arrays |
| `surrounded_test.c` | #312 | the trapped-wall double count flips the surrounded verdict |
| `blows_test.c` | #313 | the stack weight changes the blows (2 where the engine gives 3) |
| `recharge_test.c` | #316 | an unwearable pack wand is selected and the routine gives up |
| `teststuff_test.c`, `teststuff_test3.c` | #315 | the identify-priority score overwrites the artifact bonus |

## How the transcriptions were checked

Each transcription was compared against the pinned source line by line, and
the data tables and class parameters were taken from the pinned tree rather
than from memory:

* `blows_test.c` uses `adj_str_blow` and `blows_table` verbatim from
  `src/player-calcs.c:548` and `:889`, and the Warrior's parameters from
  `lib/gamedata/class.txt`.
* `glow_bound_test.c` uses `row_top_map`, `row_bottom_map` and `col_map`
  verbatim from `src/ui-term.c:305-307`, and `AUTO_LIGHT_MAX` from
  `src/borg/borg-cave-light.h:30`.

## The runtime side

The runtime evidence is not in this directory. It comes from the native C
game built from the pinned tree with `SUPPORT_BORG=ON` and the headless
deterministic Borg harness (`SUPPORT_BORG_HARNESS=ON`), which logs the
Borg's keypress stream and its full message/decision trace. See
[BORG_SPEC.md §18](../BORG_SPEC.md) for the exact commands and the
isolation arrangement that keeps the run off the shared user directory.
