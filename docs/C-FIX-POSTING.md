# Posting a native C bug fix: the evidence-package standard

This is the mechanical checklist for promoting a reported lead to a verified
native C fix and posting it, as practiced on CBUG-0066 through CBUG-0102.
The prose standard for every report and PR (situation, wrong action, measured
consequence, corrected action before technical detail) is RULES 15-17; this
page records what to build and where it goes so a future session does not
have to reverse-engineer it from old PR bodies.

Worked examples: [CBUG-0102 package](https://github.com/diegoami/neo-angband-tracker/tree/task/cbug-0102-evidence/docs/audit/c-reports/CBUG-0102)
(small, complete, includes asan), [CBUG-0048 package](https://github.com/diegoami/neo-angband-tracker/tree/task/cbug-0048-evidence/docs/audit/c-reports/CBUG-0048)
(four source variants), [CBUG-0066](https://github.com/diegoami/neo-angband-tracker/pull/226) (the owner-accepted reasoning example).

## 1. Branches

- **Source branch** `task/c-borg-<slug>` from pristine `c-borg-reference`
  (currently `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`). One defect, minimal
  diff; confirm scope with `git diff --name-only BASE FIX`. Never base a C
  proposal on Neo master or the workbench branch.
- **Evidence branch** `task/cbug-<ID>-evidence` from owned master (or the
  current maintenance base). Everything lives under
  `docs/audit/c-reports/<CBUG-ID>/`.

Both are pushed and opened as **draft PRs**: source against `c-borg-reference`,
evidence against `master`. No self-merge; hosted checks are watched, not
assumed green.

## 2. Evidence directory contents

Generators (written first, iterated locally until the matrix passes):

- `scenario.c` — native observer built as a unit-test executable. Constructs
  the character/arena/items natively, runs **production Borg decision code**,
  validates the exact requested keys/inventory letters, then executes the
  real native commands (`do_cmd_*`). Game messages are captured with an
  `EVENT_MESSAGE` handler that renders each real message to the terminal and
  lets the Borg's own `inkey_hack` scrape and parse it — **never replay a
  hand-written message as gameplay evidence, never manufacture counts or
  state**. Modes are selected by env var. Print marker lines (`DECISION`,
  `TRACK`, `OUTCOME`, `DANGER`, `ENGINE`) that the runner parses.
- `run-cases.py` — runs each mode, strips ANSI codes, writes per-mode logs
  and a results JSON with pass/asan signatures.
- `make_replay.py` — builds the offline `replay.html` (scenario dropdown,
  step buttons, before/corrected cards populated from recorded values, raw
  logs). The demo must show real recorded numbers, not claims.
- `reproduce.py` — the pinned runner: fetches BASE/FIX commits if missing,
  **verifies the source diff is exactly the expected hunk**, configures one
  normal and (for memory-safety defects) one asan build dir, toggles source
  variants, rebuilds, runs scenarios per phase under
  `flock /tmp/angband-validation.lock`, asserts the full verification matrix,
  records gates with `--full-tests`, writes `result.json`, generates the
  replay, and restores the disposable source to the correction.
- `borg.txt` — Borg config for the disposable profile.

Captured artifacts from the verified run, **committed to the evidence branch**
(the step most easily forgotten):

- `replay.html` + `replay-data.json` — the demo, "download raw and open
  locally".
- `README.md` — the immutable readable guide (same opening as the issue, the
  three pinned commands, the replay walkthrough with what to click and what
  changes, actual output blocks, conditions and limits, file inventory).
- `receipt.json` — `base`, `fix`, `fixtures_ref`, per-case results, identical
  controls, gate totals, compiler/cmake versions, `browser_verification`, and
  a `sha256` map of every evidence file so committed bytes can be re-checked.
- All phase logs: `{before,after}-{mode}.log`, asan variants
  (`asan-{before,after}-{mode}.log`, named so the crash report is kept),
  results JSONs, plus `configure*.log`, `*-build.log`, `*-scenarios.log`,
  `*-alltests.log` and `result.json` from the run root.

## 3. Verification matrix (all required before posting)

1. **Fail-before / pass-after** on the corrected contract: the original must
   fail the scenario's final assertions, the correction must pass, and the
   failure must be the defect's observable, not a crash.
2. **Controls byte-identical**: at least one unchanged-situation control whose
   complete log is identical before/after. Changed-case action/outcome lines
   are compared field-by-field.
3. **Full native gates on both variants** (`--full-tests`): currently
   965/965 unit checks and 5/5 frontend tests, checked from actual run
   summaries (the native unit runner can exit zero after failed assertions).
4. **Memory-safety defects: asan variant**. gcc's AddressSanitizer reports
   `heap-buffer-overflow` for both directions (clang says `underflow`) —
   detect the `WRITE|READ of size` line, the function and the allocation
   stack. Scenario runs may set `ASAN_OPTIONS=detect_leaks=0:intercept_strcmp=0`
   when the pristine base trips unrelated interceptor noise; record why.
5. **Replay render check**: run headless Chrome on the exact committed
   `replay.html` (`--headless=new --dump-dom --virtual-time-budget`), assert
   the scenario labels, panel values against `replay-data.json`, no
   `undefined` fields, zero console errors; record the check count in the
   receipt. Assert against the **rendered DOM before the `<script>` tag** —
   matching the script source itself gives false passes when the JS is
   syntactically broken (this bit three replays: incomplete ternaries,
   Python-escaped apostrophes, stray parens). Likewise run `node --check`
   against the **generated** replay's extracted script, not the Python
   template: Python processes `\'` escapes the JS source never sees, so a
   template that passes can still generate broken JS (bit calfree-323).

## 4. Pinning discipline

The three reproduce commands pin an **immutable fixtures commit sha**. If
artifacts or generator fixes are appended after the verified run (CBUG-0061
precedent), the branch head moves past the pin — state the exact relationship
in the README ("the native run executed at pin X; the only later generator
change is Y; no native log differs"). Never amend or force-push a published
head; append commits instead.

## 5. Posting checklist

- Canonical issue: maintainer-readable opening near the top (RULES 15-17),
  `## Owner upstream filing kit` with the `angband/angband/compare/master...`
  link (opening it posts nothing; agent posting stays paused) plus copyable
  title/description, `## Verification` with the three pinned commands,
  threads line, relabel `bug` + `evidence:c-game-reproduced`.
- Close duplicate reports with a cross-link to the canonical issue.
- Draft source PR (base `c-borg-reference`) and draft evidence PR (base
  `master`), linking README/replay/receipt at the end of the evidence PR body.
- Receipt comment on checkpoint #207: exact heads, actual outcomes, gates,
  controls, fixture interventions, pending hosted checks, and the next exact
  action.
- Source issues stay OPEN pending accepted upstream correction/import;
  proposals and diagnostics are not adoption.

## 6. Fixture pitfalls seen so far

- Flavor awareness is **per kind**: a known and an unknown item of the same
  kind cannot coexist; teach the flavor through the emergency read itself and
  grant the now-known item afterward.
- `borg_caution()` has many preemption branches: a starting Word of Recall
  scroll gets read when `goal.fleeing` is set; `ready_morgoth`, retreat
  timers and stair flows all matter. Keep `borg.time.now = 0` to keep the
  retreat/back-away branches disabled, and drop the recall scroll if the
  scenario needs the unknown-item emergency.
- `player_make_simple` starting gear is unaware; `all_letters_nohjkl` skips
  h/j/k/l; `borg_read_scroll` queues two leading ESCAPEs; the unit-test `eq`
  macro returns on failure, so record values and assert at the end.
- Monsters with `base:` inheritance can carry INVISIBLE (quylthulg); check
  `monster_is_invisible` before building an observation scenario.
