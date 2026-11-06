# The reload that forgot its array

Reinitializing the Borg frees the dynamic calculations and parses them
again — but the free emptied the list while keeping its backing pointer
array.

**`calculations_free()` reset the calculation count to zero but
retained the backing pointer array; `borg_array_add()` allocates a
fresh array whenever the count is zero, so the next parse after a
reload overwrote the retained pointer and lost the previous
allocation.** Measured natively: the original still holds the array
after the free (`retained=1`) and LeakSanitizer finds the overwritten
80-byte allocation lost (320 bytes in 4 allocations on the reload
path); the correction releases the array on free (`retained=0`), the
overwritten-array leak is gone (240 bytes in 3), and reloading still
works — the reparsed formula evaluates to 7 in both variants.

Control: a single parse without a reload never touches the free path —
both variants evaluate the formula to 3 with one calculation and
byte-identical logs.

Consequence: every borg reinitialize leaked the previous calculation
backing array — 80 bytes per reload, a slow leak on a manual command,
not a crash; no death or survival claim is made. The two further leaks
visible in the same report (temporary operator arrays, token pointer
arrays) are the separate reported leads in issues 324 and 323, present
in both variants and untouched here.

Canonical report: [issue325](https://github.com/diegoami/neo-angband-tracker/issues/325).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/calcreload-325-evidence
git show PINSHA:docs/audit/c-reports/calcreload-325/reproduce.py > /tmp/reproduce-calcreload-325.py
python3 /tmp/reproduce-calcreload-325.py --fixtures-ref PINSHA --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720` (a normal build and an AddressSanitizer build each), verifies
the source diff is exactly the array release, runs the reload and
control situations per variant, runs the reload path under
LeakSanitizer, checks the fail-before and pass-after matrix, the
disappearing backing-array leak, the surviving sibling leaks and the
byte-identical control, records both full gates and writes an offline
`replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The reload that forgot its array**: the original retains the array
across the free and loses the previous allocation; the correction
releases it.

Actual recorded output, original (`before-reload.log`):

```text
GEOMETRY freed retained=1 count=0
RATING retained=1 reload_value=7 reload_count=1
```

Corrected (`after-reload.log`): `RATING retained=0 reload_value=7
reload_count=1`. LeakSanitizer on the reload path
(`before-asan-reload.log` / `after-asan-reload.log`): `320 byte(s)
leaked in 4 allocation(s)` — including the overwritten backing array,
allocated in `borg_array_add` directly from `parse_calculation_line` —
down to `240 byte(s) leaked in 3 allocation(s)` with that block gone;
the remaining operator-block and token-array leaks (issues 324 and 323)
appear in both. Control (`control`): `RATING control_value=3
control_count=1` in both variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`253ab11468bd91f47f2a458ecc3f333a00eaa677`. One block in
`src/borg/borg-formulas-calc.c`. `calculations_free()` freed each
calculation and zeroed the count but kept `calculations.items`;
`borg_array_add()` treats a zero count as unallocated and allocates a
new array, overwriting the retained pointer. The correction frees the
array and clears the pointer and bound so the reload starts from a
clean empty array.

```sh
git fetch --no-tags origin task/c-borg-calc-array-reload
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-formulas-calc.c | sed -n '700,712p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 253ab11468bd91f47f2a458ecc3f333a00eaa677 -- src/borg/borg-formulas-calc.c
```

### Conditions and limits

Native calculation machinery only (no game state needed): parse the
constant formula "1 + 2", free the calculations as a reinitialize does,
parse "3 + 4" and evaluate it. The retained-pointer check reads the
global array state directly; the leak proof is the same reload path
built with AddressSanitizer and run under LeakSanitizer. The lost
allocation is 80 bytes per reload — a slow leak on a manual
reinitialize, not a crash; no death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
