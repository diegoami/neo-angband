# The light list that outgrew its shelf

When the Borg is in the dark it flows back towards remembered light,
listing every perma-lit grid on the screen into two fixed-size arrays
— with no bound.

**`borg_flow_light()` appends one entry per glowing grid on the
visible panel to `borg_glow_y`/`borg_glow_x`, which hold
`AUTO_LIGHT_MAX` (1536) entries; an ordinary panel can show more
glowing grids than that.** Measured natively on a fully lit arena
panel (1600 glowing grids): the original collects all 1600 —
`overflow=64` — and AddressSanitizer catches the overflowing write as
a `global-buffer-overflow` (`WRITE of size 1`) in the scan itself; the
correction stops collecting at the bound (`glow_n=1536`, no overflow)
and the flow still runs.

Control: a panel with a thousand glowing grids fits the arrays in both
variants — `glow_n=1000`, byte-identical logs.

Consequence: past the bound the writes land in the neighbouring arrays
(the tail of `borg_glow_y` in `borg_glow_x` and vice versa), so the
read-back loop enqueues swapped and off-panel coordinates as flow
targets — the report's source-derived mechanism. The measured facts
here are the unbounded write and the sanitizer-caught overflow; the
live out-of-light trigger is the supplied evidence, not claimed here.
No death or survival claim.

Canonical report: [issue311](https://github.com/diegoami/neo-angband-tracker/issues/311).
Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/glowoverflow-311-evidence
git show 83a131b3187b125521dcc76a099807c78167dc54:docs/audit/c-reports/glowoverflow-311/reproduce.py > /tmp/reproduce-glowoverflow-311.py
python3 /tmp/reproduce-glowoverflow-311.py --fixtures-ref 83a131b3187b125521dcc76a099807c78167dc54 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720` (a normal build and an AddressSanitizer build each), verifies
the source diff is exactly the glow-array bound, runs the overflowing
and control situations per variant through the production out-of-light
flow, runs the overflow under AddressSanitizer, checks the
fail-before and pass-after matrix, the sanitizer report and the
byte-identical control, records both full gates and writes an offline
`replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The light list that outgrew its shelf**: the original writes 1600
entries into arrays of 1536; the correction stops at the bound.

Actual recorded output, original (`before-glow.log`):

```text
GEOMETRY glow_n=1600 limit=1536
RATING glow_n=1600 overflow=64
```

Corrected (`after-glow.log`): `RATING glow_n=1536 overflow=0`.
AddressSanitizer on the same run (`before-asan-glow.log`):
`WRITE of size 1` — `SUMMARY: AddressSanitizer:
global-buffer-overflow ... in borg_flow_light`; the corrected run
passes with no sanitizer report. Control (`control`): `RATING
glow_n=1000 overflow=0` in both variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`d4dcb983c9623757b523a7fae861b624d858e23d`. One check in
`src/borg/borg-flow-misc.c`. The scan appended every glowing grid on
the panel with no bound; the correction stops collecting once the
arrays are full.

```sh
git fetch --no-tags origin task/c-borg-glow-array-bound
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-flow-misc.c | sed -n '833,850p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 d4dcb983c9623757b523a7fae861b624d858e23d -- src/borg/borg-flow-misc.c
```

### Conditions and limits

Native Human Warrior on a fully perma-lit arena (the Borg map marked
`BORG_GLOW` throughout), seed one thousand, terminal sized so the panel
sits inside the map; the production out-of-light flow runs in full.
The memory-safety proof is the same run built with AddressSanitizer
(the strcmp interceptor is disabled — the pristine base trips an
unrelated interceptor read while sorting monster names at init). No
death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
