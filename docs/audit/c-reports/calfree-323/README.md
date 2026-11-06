# The free that left the token list behind

Releasing a parsed calculation — as a borg reinitialize does for every
dynamic formula — freed every token and the token-list struct, but not
the list's backing array.

**`calc_free()` freed each token and the `token_array` struct, but not
`token_array->items`, which `borg_array_add()` had allocated
separately — releasing a parsed calculation lost its backing pointer
array.** Measured natively on parse-and-free of "1 + 2": both variants
free identically on the surface (the count is 0 either way,
byte-identical logs), but LeakSanitizer finds the token list's 80-byte
backing array lost in the original (160 bytes in 2 allocations) and
freed with the correction (80 bytes in 1 — the remainder being the
operator-block scratch array, the separate lead in issue 324).

Control: a parse that is never freed never reaches the free path —
both variants evaluate 42 with one calculation and byte-identical
logs.

Consequence: 80 bytes lost per freed calculation — a bounded leak per
reinitialize, not a crash; no death or survival claim.

Canonical report: [issue323](https://github.com/diegoami/neo-angband-tracker/issues/323).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/calfree-323-evidence
git show 9c13d0adccf11567bfa588873572c452bbbb004f:docs/audit/c-reports/calfree-323/reproduce.py > /tmp/reproduce-calfree-323.py
python3 /tmp/reproduce-calfree-323.py --fixtures-ref 9c13d0adccf11567bfa588873572c452bbbb004f --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720` (a normal build and an AddressSanitizer build each), verifies
the source diff is exactly the token-array release, runs the
parse-and-free and the unfreed control per variant, runs the free path
under LeakSanitizer, checks the disappearing token-array leak, the
surviving sibling leak and the byte-identical control and functional
logs, records both full gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The free that left the token list behind**: both variants free
identically; the original loses the token list's backing array, the
correction frees it.

Actual recorded output (`before-free.log` and `after-free.log`,
byte-identical):

```text
GEOMETRY freed count=0
RATING freed_value=3 freed_count=0
```

LeakSanitizer on the same free (`before-asan-free.log` /
`after-asan-free.log`): `160 byte(s) leaked in 2 allocation(s)` —
including the token list's backing array, allocated in `borg_array_add`
from `tokenize_math` — down to `80 byte(s) leaked in 1 allocation(s)`
with that block gone; the remaining operator-block leak (issue 324)
appears in both. Control (`control`): `RATING control_value=42
control_count=1` in both variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`2b98c831ba7b3f53b05ddcf620b8b2c54f5ed6a3`. One block in
`src/borg/borg-formulas-calc.c`. `calc_free()` freed every token and
the `token_array` struct, but `borg_array_add()` had allocated the
list's backing array separately; the correction frees it along with
the struct.

```sh
git fetch --no-tags origin task/c-borg-calc-token-array-free
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-formulas-calc.c | sed -n '548,560p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 2b98c831ba7b3f53b05ddcf620b8b2c54f5ed6a3 -- src/borg/borg-formulas-calc.c
```

### Conditions and limits

Native calculation machinery only (no game state needed): parse the
constant formula "1 + 2", evaluate it (3), and free the calculations
as a reinitialize does; the leak proof is the same parse-and-free
built with AddressSanitizer and run under LeakSanitizer. The lost
allocation is 80 bytes per freed calculation — a bounded leak per
reinitialize, not a crash; no death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
