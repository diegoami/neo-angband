# Test the simpler comparison too

These tests call the actual native `borg_backup_swap()` with the same real
inventory, monster and danger setup. The assertions are unchanged. Only the
production comparison is changed between the three tested variants.

| Native case | Original comparison | Simple reversal | Guarded fix |
| --- | --- | --- | --- |
| Cold shield helps | Fail | Pass | Pass |
| Frost weapon helps | Fail | Pass | Pass |
| Only cold shield available | Pass | **Fail: selects missing weapon** | Pass |
| Only frost weapon available | Pass | **Fail: selects missing armour** | Pass |
| Equal scores | Pass | Pass | Pass |
| Neither item improves protection | Pass | Pass | Pass |
| Fire shield helps | Fail | Pass | Pass |
| Lightning weapon helps | Fail | Pass | Pass |
| Both help; shield protects against both attackers | Fail | Pass | Pass |
| **Total** | **4/9** | **7/9** | **9/9** |

The simple reversal is:

```c
if (b_p2 <= b_p1) {
```

It fixes the two-item choice. But the missing item's uncomputed score stays
zero, making it look safer than the real item's score49. The branch then
queues `w` followed by an invalid inventory selection in each single-item case.
The native tests catch that before attempting to apply an invalid wield.

Actual output from **both** single-item cases with the simple reversal:

```text
DECISION swapped=1 command=119 letter=0 pack_index=-1
    native-c/CBUG-0066:236: requirement 'index>=0' failed
native-c/CBUG-0066 finished: 0/1 passed
```

`command=119` is the character `w` (wear/wield). `letter=0` is not an inventory
letter, and `pack_index=-1` means the decoder could not identify a real item.
This establishes an invalid equipment command, not a claim that the full game
crashed. The `DANGER ...=-1` printed earlier in the observer output is merely
its display marker for an absent item; the production function's absent-item
score starts at **0**.

The guarded version avoids comparing absent candidates:

```c
if (weapon_swap && (!armour_swap || b_p2 <= b_p1)) {
```

## Repeat the comparison

From a clone of this repository:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/cbug-0066-evidence
git show FETCH_HEAD:docs/audit/c-reports/CBUG-0066/expanded/reproduce.py > /tmp/reproduce-cbug-0066.py
python3 /tmp/reproduce-cbug-0066.py --compare-simple
```

This runs the ordinary before/after comparison and then tests the simple
reversal in the disposable source tree. It expects exactly the two single-item
regressions, writes `simple-reversal-result.json` and the per-case native logs,
and restores/rebuilds the guarded version. Add `--full-tests` to also run the
general native regression gates for the original and guarded variants.

The new comparison helper itself was exercised against the retained fresh
replay build: seven cases pass, the two expected index assertions fail, and
the corrected source/binary are restored. Its nine native case logs match the
independent direct comparison run exactly. The source proposal was not changed
by this experiment. Captured results are in `simple-reversal-result.json`,
`simple-reversal-expanded.json` and the nine `simple-reversal-*.log` files.
