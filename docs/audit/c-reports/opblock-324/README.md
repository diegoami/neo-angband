# The precedence pass that kept its scratch array

Every dynamic formula the Borg parses goes through a precedence pass
that builds a temporary operator list for each block — and the list's
backing array was never freed.

**`adjust_order_ops_depth_block()` freed the temporary operator tokens
and the `operators` struct, but not the separately allocated `items`
backing array — every populated operator block of every parsed formula
lost that allocation, starting with the full formula file at
startup.** Measured natively on the parse of "1 + 2 * 3": both
variants apply precedence identically (the value is 7 either way,
byte-identical logs), but LeakSanitizer finds the two lost backing
arrays in the original (160 bytes in 2 allocations) and no leaks at
all with the correction.

Control: a bare constant builds no operator list — both variants
evaluate 42 with one calculation and byte-identical logs.

Consequence: 80 bytes lost per populated operator block per parse —
the full dynamic formula file parses at every borg startup and
reinitialize, so the leak recurs on the startup path; bounded, no
corruption or crash, no death or survival claim.

Canonical report: [issue324](https://github.com/diegoami/neo-angband-tracker/issues/324).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/opblock-324-evidence
git show b6eac1ed1f7b93bc42ecc5877e235bc92b19ffdf:docs/audit/c-reports/opblock-324/reproduce.py > /tmp/reproduce-opblock-324.py
python3 /tmp/reproduce-opblock-324.py --fixtures-ref b6eac1ed1f7b93bc42ecc5877e235bc92b19ffdf --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720` (a normal build and an AddressSanitizer build each), verifies
the source diff is exactly the operator-array release, parses the
operator formula and the constant control per variant, runs the
operator parse under LeakSanitizer, checks the disappearing leak, the
identical precedence results and the byte-identical control and
functional logs, records both full gates and writes an offline
`replay.html`.

Open [replay.html](replay.html) (download raw and open locally).
Choose **The precedence pass that kept its scratch array**: both
variants compute 7; the original loses 160 bytes in 2 backing-array
allocations on the way, the correction none.

Actual recorded output (`before-operators.log` and
`after-operators.log`, byte-identical):

```text
RATING formula_value=7 calc_count=1
OUTCOME formula_value=7 calc_count=1
```

LeakSanitizer on the same parse (`before-asan-operators.log` /
`after-asan-operators.log`): `160 byte(s) leaked in 2 allocation(s)`
allocated in `borg_array_add` from `adjust_order_ops_depth_block` —
down to no leaks reported. Control (`control`): `RATING
control_value=42 control_count=1` in both variants, byte-identical
logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`02be7739db306897f2441562ee8d27dddc15d36c`. One block in
`src/borg/borg-formulas-calc.c`. The temporary operator list's items
and struct were freed, but `borg_array_add()` had allocated the list's
backing array separately; the correction frees it along with the
struct.

```sh
git fetch --no-tags origin task/c-borg-operator-block-free
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-formulas-calc.c | sed -n '426,436p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 02be7739db306897f2441562ee8d27dddc15d36c -- src/borg/borg-formulas-calc.c
```

### Conditions and limits

Native calculation machinery only (no game state needed): parse the
constant-operator formula "1 + 2 * 3" and evaluate it; the leak proof
is the same parse built with AddressSanitizer and run under
LeakSanitizer. The lost allocations are 80 bytes per populated
operator block per parse — a bounded leak at startup and per
reinitialize, not a crash; no death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
