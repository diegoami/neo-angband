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
Current fixture templates by scenario class (evidence dirs are now named by
slug, `docs/audit/c-reports/<slug>-<issue>/`): `staletarget-322` (full-game
arena fixture: warrior, `borg_update` entrypoint, keypress drain) and
`vampire-257` (`borg_attack(false)` acting mode with `known_item()`).

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
- `player->body.slots[]` is indexed by **body slot** — look it up with
  `slot_by_name(player, "light")` etc. Using the INVEN_* constant as the index
  (INVEN_LIGHT) is out of bounds and gives a heap-use-after-free in the
  fixture itself.
- Stock light sources (torch, lantern) carry LIGHT_2/LIGHT_3 **flags** with a
  zero OBJ_MOD_LIGHT modifier; only artifact lights carry the modifier. To get
  a light with a modifier in a fixture, instantiate from the object's artifact
  pointer and `copy_artifact_data`.
- The borg unique scan re-derives `borg_race_death` from `r_info[].max_num` on
  every pass and only runs on the new-level branch — flip
  `borg.status.old_depth` to re-trigger it.
- Borg messages queue via `borg_react` and are processed across several
  use-gated passes inside `borg_update`; a single update call may not surface
  a queued message. Run several passes before asserting on message state.
- Run observer binaries with cwd=`build/game` — the `lib/` tree is staged
  there, and **only by a full build**. Build the whole tree once (never just
  the observer target); the same applies to the ASAN build dir.
- `grep -c` exits 1 on zero matches: never chain a build with `&&` after a
  `grep -c` that may legitimately print 0.
- `reproduce.py` already takes `/tmp/angband-validation.lock` internally per
  command — don't wrap the runner itself in `flock`.
- When one LSan report shows several leaks, attribute them by allocation
  stack signature, not by size alone (e.g. three formula leaks split by
  `borg_array_add ← parse_calculation_line` vs `← adjust_order_ops_depth_block`
  vs `← tokenize_math`), and fix/verify them one at a time.
- ASAN reports **global-buffer-overflow** for static arrays (the borg glow
  arrays, issue #311) as well as heap-buffer-overflow; a scenario that
  only exercises heap objects can miss it.
- Evidence-branch iteration: rebuild the branch from the maintenance base as
  a **single new commit**, never amend after push; iterate worktrees are
  disposable.

## 7. Triage dispositions for leads that don't become fixes

Not every reported lead should get a fixture. This session's working taxonomy
(posted as a comment on the issue, with file:line evidence, never just an
assertion):

- **latent** — the defect exists in code but no stock configuration reaches
  it. Verify across the *whole* stock dataset before claiming it (e.g. all 94
  stock uniques enumerated in depth order; no race in p_race.txt carrying
  RES_FIRE/RES_ACID). Comment only, no promotion.
- **unreachable** — no code path reaches it (e.g. the only timed-and-
  unwearable items are rods, and rods are handled elsewhere).
- **masked** — another check already decides the case; the suspect value
  never influences an action. Trace the full decision path, not just the
  first function (the leaked value may be dominated by a later branch's
  clause or return).
- **consequence-free / dead code** — no post-scan consumer reads the value;
  or the variable is never read after the suspect write.
- **owner-intent** — comment-vs-code disagreement, or a possible deliberate
  omission (damage formula, caster glove penalties). Ask the owner with the
  candidate interpretations spelled out; do not guess a fix.

A triage comment must carry the same standard of evidence as a fix:
file:line traces and stock-data checks that a maintainer can re-run.
