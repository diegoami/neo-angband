# Repeat the knockout comparison

The earlier README left `/PATH/TO` placeholders and assumed several files were
already copied into a fixed temporary directory. Those steps were incomplete.
The runner below extracts all required files and uses a fresh directory each
time, preserving existing worktrees, profiles and previous runs.

From a clone of **diegoami/neo-angband-tracker**, run these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/cbug-0002-evidence
git show FETCH_HEAD:docs/audit/c-reports/CBUG-0002/reproduce.py > /tmp/reproduce-cbug-0002.py
python3 /tmp/reproduce-cbug-0002.py
```

The first command fetches the evidence branch. The second extracts the replay
script from the revision just fetched (`FETCH_HEAD`); it does not change your
checkout. The third runs it. You need Python3, Git, CMake, a C compiler/build
toolchain, `flock`, and the native platform build dependencies (on Linux, this
CMake configuration enables X11 and needs its development headers/libraries).
No manual patch download or editing of placeholder paths is required.

The runner prints its fresh `/tmp/angband-cbug0002-...` directory immediately.
Each build/gate prints its log path and waits for the shared validation lock.
Allow the native builds and both test gates to complete. Output is retained
there, including `result.json`; the source worktree is retained for inspection.
It runs native C only, without Neo or Rust builds.

Expected final lines:

```text
VERIFIED: knockout fails before and passes after; kill reactions 0 -> 1.
VERIFIED: three controls identical; both native gates 965/965 + 5/5.
```

An assertion failure in the **before knockout** case is expected. The old unit
runner misleadingly returns exit0 for that failure, so this runner checks the
reported result and the actual kill-reaction marker. The three controls must
pass before and after, and their logs must match exactly. All four after cases
must pass. Both full native test gates must pass. Unexpected results cause the
replay script to fail instead of printing VERIFIED.

## Where observation.patch and the other files come from

All fixtures are committed beside the original README at evidence revision
`3de1e3123915587483395dd40c4d2e75704e65f4`:

- [observation.patch](https://github.com/diegoami/neo-angband-tracker/blob/3de1e3123915587483395dd40c4d2e75704e65f4/docs/audit/c-reports/CBUG-0002/observation.patch): adds the diagnostic build target and redirects upstream test output/cleanup into disposable directories. This is instrumentation, **not** the proposed Borg correction.
- [scenario.c](https://github.com/diegoami/neo-angband-tracker/blob/3de1e3123915587483395dd40c4d2e75704e65f4/docs/audit/c-reports/CBUG-0002/scenario.c): the native game scenario/observer.
- [run-modes.py](https://github.com/diegoami/neo-angband-tracker/blob/3de1e3123915587483395dd40c4d2e75704e65f4/docs/audit/c-reports/CBUG-0002/run-modes.py): runs knockout, heavy-stun, paralysis and no-effect modes.
- [borg.txt](https://github.com/diegoami/neo-angband-tracker/blob/3de1e3123915587483395dd40c4d2e75704e65f4/docs/audit/c-reports/CBUG-0002/borg.txt): the Borg configuration used in the recorded comparison.

`reproduce.py` extracts these exact committed files with `git show`, rather
than downloading whatever happens to be on a moving branch. It changes only
their temporary paths so separate runs do not collide. It also runs the native
scenario from the build's `game` directory, where the test library can find
`./lib`; that working-directory requirement was missing from the old commands.

The proposed C correction is separate. After the before run, the runner applies
this one-line change, without internal tracking comments:

```diff
-    borg.dont_react = false;
+    borg_dont_react = false;
```

It checks the corrected C file against the SHA256 of upstream PR6808's clean
source head `6f463d01d753089ab695537495ab27f3bc928dfa`. Seeds, assertions, game
conditions and negative controls remain unchanged. The native C base is the
original recorded revision `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`; this replays
that comparison rather than silently testing a future upstream revision.

The scenario's original synthetic interventions and limits still apply:
[recorded explanation and verification](README.md). The runner makes the
reproduction complete; it does not expand what the scenario proves.
