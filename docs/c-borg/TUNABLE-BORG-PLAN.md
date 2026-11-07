*better-borg-c and borg-rs · evaluation and plan · 2026-11-07*

> This file is the reviewable source of the plan. A rendered copy was published as a private page for the owner. Live state (issues, labels, carries) belongs to the boards of neo-angband-tracker, better-borg-c and borg-rs, not to this file.

# Tunable Borg Plan

How to get a Borg that we can tune and improve by experiment, without throwing away what the C Borg knows and without being bound to every decision it made. Two repositories share that goal: better-borg-c (native C) and borg-rs (Rust).

> **Recommendation.** Keep the C Borg as the working engine and grow a policy layer on top of it, in three steps that each pay off on their own:
>
> 1. **Expose parameters with C-Borg defaults.** Move the decision constants that matter for survival into configuration. With every parameter at its default, the build must reproduce the pinned Borg's trace byte for byte.
> 2. **Tune by racing on a frozen seed population.** Use the deterministic native runner that better-borg-c already has, run in parallel, with survival statistics and held-out seeds.
> 3. **Replace modules one at a time** (caution, dive policy, fight-or-flee) behind switches. A replacement is kept only when it beats the original on the same harness. The original module stays as the reference.
>
> Do not rewrite the Borg from scratch, and do not start with machine learning. Both throw away tested behaviour before we can measure what it is worth.
>
> **First step: bring the fixes into borg-rs.** Before any tuning, borg-rs carries better-borg-c's verified fixes, one per commit. It is accepted when its key stream equals the matching better-borg-c build's on the existing suites, checked once at the end. From then on the two repositories start from the same Borg, and every later change can be checked in both.
>
> **Two repositories, one contract.** Then run steps 1 and 2 in **better-borg-c**, which has the measurement runner and death dossiers. Prepare **borg-rs** to take over step 3: its explicit state and Rust traits make module replacement and state snapshots far easier than in C. Both use the same parameter names, profiles, seed sets and outcome definitions. At the same profile, the C and Rust Borgs must produce the same key stream, which turns borg-rs into a differential test of every parameter and fix. Decide later, from measurements, which one becomes the long-term base.

## What we already have

### neo-angband-tracker and better-borg-c

- **A specification of the C Borg** (`docs/c-borg/BORG_SPEC.md` in neo-angband-tracker, PR #317). It covers the perception channels, the dungeon decision ladder, flow, danger, caution, depth preparation and modes, and every claim is cited at the upstream pin `e473720`. This is the map of where the decisions live.
- **A bug-fixed C product.** better-borg-c carries 15 verified fixes (`patches/0001`–`0015`), each with its own provenance and removal unit.
- **A deterministic native runner** (`tools/measure-native.py`). It plays a normal birth with the normal game loop, logs both RNG states, and classifies each run as death, cap, stall, crash or harness failure. Replays from the same seed are byte-identical. A 400,000-engine-turn run takes about 95–113 wall seconds; 20,000 turns take 8–15 s.
- **Death dossiers** (`docs/deaths/`) with T-minus snapshots, plus a death-analysis plan.
- **An existing external tuning language.** `borg.txt` has a formula section, about 790 lines, with *Power*, *Depth Requirement* and *Restock Requirement* blocks. With `borg_uses_dynamic_calcs` set it replaces `borg_power()`, the depth preparation checks and the restock checks without recompiling.

What is missing is scale and a tuning surface for everything else. So far the cohorts have three seeds. Danger, caution, escape, healing, flow fear and the ladder order are hard-coded: the Borg's `.c` files contain about 4,600 numeric literals of two or more digits (713 in `borg-power.c`, 376 in `borg-danger.c`, 179 in `borg-caution.c`).

### borg-rs

- **A faithful Rust port of all 59 modules** (about 73,000 lines; milestones M1 deterministic C harness, M2 c2rust transpile and M3 safe idiomatic Rust are done). `unsafe` appears only in the engine boundary (`engine.rs` and the engine declarations). It links as a static library behind the same `inkey_hack` seam and drives the unmodified C engine.
- **Equivalence by key stream.** For the same seed, the Rust Borg's key log is byte-identical to the C Borg's on every suite: smoke, full (45 runs), deep (9), classes (16) and config (7, munchkin and lunal). Its `harness/diff` reports the first divergence by seed, turn and key. Each build records a `BUILD_ID`.
- **A headless, parallel harness**: the `borgh` frontend (`-DSUPPORT_BORG_HARNESS=ON`, hooks under `#ifdef BORG_HARNESS`), with scripted birth, a fixed seed, a fixed holiday date and a fixed 80×24 terminal. The 45-run, 100,000-turn suite takes about 138 s with 4 jobs on its 4-core machine.
- **All Borg state in one place.** Every former C global lives in `BorgState`, with one sub-struct per module. That is the precondition for serialising the Borg's memory (which the C dossiers cannot restore on load), for cloning state for what-if evaluation, and for replacing a module behind a trait.
- **Narrower scope today.** C bugs are kept on purpose (`docs/c-bugs.md`) and reported to neo-angband-tracker; none of better-borg-c's 15 carried fixes is in it yet. Verification covers the suites' seeds and turn limits only: nothing deeper than dungeon level 27, no Morgoth, no self-lunal. "No new tactics" is its current rule.

### How the two compare for this goal

|  | better-borg-c | borg-rs |
| --- | --- | --- |
| Bug fixes | 15 verified carries | none yet (faithful to the pin) |
| Tracks upstream C | directly (same tree) | by porting each upstream Borg change |
| Measurement | native runner, outcome classes, death dossiers, frozen cohorts | seed suites, key-stream identity, build identity |
| Adding parameters | C edits across many files | fields on `BorgState` sub-structs; easy to type and document |
| Replacing a module | preprocessor or function-pointer switches | Rust traits; the compiler checks the interface |
| Saving and cloning Borg memory | hard (scattered globals) | feasible (one struct) |
| Correctness oracle | pristine C trace | the C Borg's key log, and later better-borg-c's at the same profile |

> **Scope decisions needed first.** better-borg-c's RULES.md (rule 12) and AGENTS.md exclude new gameplay strategies; its scope is bug carries and measurement. borg-rs's rules make it a faithful port with no new tactics and no translated C fixes. This plan needs an explicit owner decision for each. Keep strategy changes labelled and stored apart from bug carries in both, so the carries stay reusable and their removal units stay clean.

## Approaches compared

| Approach | Keeps C Borg knowledge | Freedom to change decisions | Cost and risk |
| --- | --- | --- | --- |
| **A. Tune existing knobs only** (35 `borg.txt` settings plus the formula section) | all | Low. Power, depth preparation and restock only; danger, caution and the ladder stay fixed. | Cheapest; can start this week. Hits a ceiling quickly. |
| **B. Parameterise in place** (constants to config, defaults = pin) | all | Medium. Any threshold or weight can move, but the structure stays. | Moderate C work. Trace identity at the defaults makes regressions detectable. |
| **C. Module replacement on B** (switchable variants per subsystem) | as reference | High within a module. Rung order and new rules are possible. | Each module is its own project; it is measured, and stays reversible. |
| **D. Rewrite from the specification** | mostly lost | Total | Very high. Years of edge-case handling (prompts, stores, flow) would have to be rediscovered before it plays as well. |
| **E. Learned policy** (reinforcement or imitation learning) | indirect | Total in principle | Very long horizons, sparse death signal, huge state; compute far beyond one workstation. At most later, for one narrow decision trained on harness logs. |

The plan is **A, then B, then C**: a gradual replacement. The C Borg keeps doing perception, flow, items, stores and key generation; we take over decisions one surface at a time, and only where measurement says we did better.

borg-rs adds a sixth option that sits between C and D: **module replacement in the Rust port**. It keeps all the C Borg's knowledge, because the port is key-for-key identical, and it adds safe refactoring, traits and serialisable state. Its cost is that the bug fixes must be translated first, and every later upstream change must be ported.

## Architecture

- **Substrate (unchanged C Borg).** Perception (`borg_update()`, the direct reads), memory, flow (`borg_flow_*`), item handling, stores, the key queue, and prompt answering. Bug carries go here, as today.
- **Parameter layer.** One table of named parameters, read from `borg.txt` (or a sibling file) at start. Each entry has a default equal to the pinned constant, a valid range, a source location and a unit. The table is dumped into every run record. The `CONFIG` value type the formula language already has can read the same table.
- **Module switches.** A small number of seams where a variant can replace the original's decision: `borg_caution()` steps, `borg_escape()`, `borg_heal()`, `borg_prepared()`/dive policy, and the order of rungs in `borg_think_dungeon()`. Variant 0 is always the original code path.
- **Profiles.** A named profile is a complete parameter set plus module choices. `c-borg-1.0` is all defaults, and is the control in every comparison.

**The defining invariant:** profile `c-borg-1.0` reproduces the pinned Borg's full native trace byte for byte on the frozen seeds. The existing runner already compares traces, so this is cheap to enforce. It makes the parameterisation itself zero-risk, and it lets every later experiment be read as a delta against the C Borg.

### The shared contract between the two repositories

- **One parameter table:** the same names, units, ranges and defaults in both, read from the same `borg.txt` syntax, so a profile file works unchanged in C and in Rust.
- **One harness contract:** the same seed sets, outcome classes (death, cap, stall, crash, harness failure), engine-turn caps and result-record schema. The quickest route is to agree on one headless frontend for population runs (borg-rs's `borgh`) and keep better-borg-c's observer for death dossiers. Both repositories should ship the same frontend patch, so their runs are comparable.
- **Differential identity:** at any profile both repositories implement, C and Rust must emit the same key stream on the frozen seeds. A divergence means a wrong parameter wiring or a wrong fix translation in one of them, and the diff tool names the seed, turn and key where it starts.
- **One results store and report,** keyed by repository, build ID, profile hash and seed, so the two repositories' measurements can be read side by side.

## The first tuning surface

These are the decision points the specification identifies as the main levers for survival and progress. Each becomes a handful of named parameters.

| Group | Where (pin) | Current values | What it trades |
| --- | --- | --- | --- |
| Flow fear thresholds | `borg_flow_spread()`, `borg_flow_enqueue_grid()` | `avoidance` × 3/10, 5/10 at level 50, ×2 scary, ×3, ×5 ignoring, ×25 after 5000 turns, ×100 starving | Routes through danger vs. detours and stalling |
| Bravery boosts | `borg_think_dungeon()` | 2×`BI_CURHP`, 4×`BI_MAXHP`, 30000 | Getting unstuck vs. walking into lethal grids |
| Retreat and flight | `borg_caution()` | retreat at danger > avoidance/3; flee after more than 3 escapes; desperate dashes at stair distance 3 / 2 / 20 | Early disengagement vs. lost experience and loot |
| Escape choice | `borg_escape()` | allowed failure 10 / 25 / 35 %; `risky_boost` 0 or 3 | Teleporting too late vs. wasting escapes |
| Healing | `borg_heal()` | hit-point-loss bands vs. chance thresholds; +5 when risky | Potions spent vs. deaths with unused cures |
| Depth preparation | `borg_prepared_aux()`, Depth section | per-band requirements; "character level < depth" rule | Dive speed vs. readiness. Probably the biggest single lever. |
| Restock | `borg_restock()`, Restock section | food, light, cures, escapes per depth | Trips to town vs. running dry |
| Time on level | boredom (10000), `borg_time_to_stay_on_level()`, stair leash (`BI_CLEVEL`×3+14) | as coded | Farming experience vs. exposure |
| Anti-bounce | `borg_check_bounciness()`, `borg_danger()` | 200 / 300 / 500 / 700 | Loop breaking vs. discarding danger knowledge |
| Power weights | Power section | weights as in `borg.txt` | What kit the Borg values |
| Mana reserve | `borg_spell_okay()` | per class, from character level 35 | Escape mana vs. offence |
| Existing switches | `borg.txt` | `borg_plays_risky`, `borg_kills_uniques`, `borg_worships_*`, `borg_uses_swaps`, munchkin, self-lunal | Style presets |

That is about 40–60 parameters to start. The rest of the roughly 4,600 constants stay in code until screening shows a reason to move them.

## What "best" means

"Survivability" alone is a trap: a Borg that never leaves dungeon level 1 never dies. The objective has to reward progress while it penalises death. Use two primary measures, read together on a Pareto front:

- **Survival:** the share of runs alive at the engine-turn cap, and the restricted mean survival time in engine turns. Runs that hit the cap are censored, not counted as wins; use Kaplan–Meier curves.
- **Progress:** the share of runs that reach dungeon level *D* alive by turn *T* (for example dlvl 10 by 400,000 turns, dlvl 30 by 4,000,000), and the maximum depth and character level reached.

Track as guardrails: stalls, wall-clock timeouts, money-scum loops and town idling. A profile that gains survival by stalling is rejected.

### How many runs a decision needs

Deaths are noisy yes/no outcomes. With a two-sided test at α = 0.05 and 80 % power, the runs needed *per profile* are:

| Death rate, control → variant | Runs per profile | Core-hours at 400k turns |
| --- | --- | --- |
| 50 % → 40 % | ~390 | ~12 |
| 30 % → 20 % | ~290 | ~9 |
| 30 % → 25 % | ~1,250 | ~38 |

Core-hours assume about 110 s per 400,000-turn run, and count both profiles of a comparison. On the 20-core workstation, a 30 % → 20 % comparison takes about half an hour of wall time, if the shared validation lock allows that much parallel load. Paired seeds help: run every profile on the same seeds and compare with McNemar's test. Pairing helps less than it seems, because the RNG streams diverge after the first different decision. The cohort of three used so far cannot detect any realistic difference. Deeper caps cost roughly ten times more per run.

## Test harness

The harness has four levels, from fast and exact to slow and statistical:

1. **Identity test.** Profile `c-borg-1.0` must reproduce the pinned trace byte for byte on the frozen seeds. This runs on every change to the parameter layer and every bug carry.
2. **Decision fixtures.** Hand-built boards that call a single decision function and check its output, as the existing native scenarios already do: "with this danger and these items, does the Borg phase, heal or fight?" Every module variant comes with fixtures that show where its decision differs from the original's.
3. **Situation suite.** Saves taken from the death dossiers (the T-minus-*n* snapshots), with a fresh Borg started from each, scored by survival over the next *N* turns. The dossiers record that the Borg's in-memory state is not restored on load, so this suite tests "can this profile handle this position", not an exact replay. It is cheap and aimed at the actual causes of death.
4. **Population runs.** Full games from birth on frozen seed sets: a *training* set for search, a *validation* set for choosing between candidates, and a *test* set opened once per milestone. Extend to more races and classes once Warrior results stabilise, so that tuning does not overfit one class.

**Runner changes needed:** parallel workers with isolated directories (one run per core), parameter injection through `borg.txt` so no rebuild is needed per profile, an append-only results store (JSONL or SQLite) keyed by build hash, profile hash and seed, and a summary report with survival curves and paired comparisons. Keep generated runs out of Git, as the current measurement rules require.

## Search strategy

- **Screening first.** Run one-at-a-time sweeps and Morris screening on short caps (20,000–100,000 turns) to find the 5–10 parameters that actually move the outcome. Most will not.
- **Racing for tuning.** Use irace-style racing or successive halving. Start many candidate profiles on a few seeds, drop the clearly worse ones, and promote the survivors to more seeds and longer caps. This is built for noisy, expensive evaluations, which is exactly what game runs are. Use CMA-ES or Bayesian optimisation within a racing round if the space is continuous and smooth.
- **Confirm on held-out seeds,** always against `c-borg-1.0`, before a profile is called better.
- **Death-driven module work.** Rank causes of death from the dossiers and the population runs, pick the dominant one, design a module variant that addresses it, and prove it on fixtures, then on the situation suite, then on the population.

## Plan

**Order:** R1 (fixes into borg-rs) comes first. Phase 0 can be prepared meanwhile, but its baseline cohort is run only after R1, so that both repositories measure the same Borg. After that, the C phases and the borg-rs track run side by side, as below.
- **R1 · first.** **Carry the fixes into borg-rs.** Translate better-borg-c's carried fixes (`patches/0001`–`0015` today, plus any carried meanwhile) into Rust, in the same order, one per commit, each linking its better-borg-c adoption issue. Check once, at the end: the existing borg-rs suites against a better-borg-c build of the same carries; on a divergence, bisect over the fix commits. No per-fix native scenarios, because better-borg-c already proved the fixes and borg-rs only has to show a faithful translation. For fixes the suites do not reach (for example the deep-level swap), compare the Rust change with the C patch side by side. Exit: identical key logs on the suites, and a side-by-side review of each unreached fix. Needs the owner's scope change for borg-rs, which today keeps C bugs on purpose.
- **Phase 0.** **Decide and prepare.** Owner scope change for a strategy track in better-borg-c. Freeze the metric definitions and three seed sets. Parallelise the runner and add the results store. Baseline cohort of about 300 seeds at 400k turns on the current carries build (about 9 core-hours). Exit: baseline survival curve and death-cause table.
- **Phase 1.** **Existing knobs.** Sweep the formula section and the `borg.txt` switches (approach A) on the training seeds. No C changes. Exit: a first measured profile, or a documented null result.
- **Phase 2.** **Parameter layer.** Move the first 40–60 constants into the parameter table with pin defaults. Prove the identity test on every frozen seed. Exit: `c-borg-1.0` byte-identical; parameters dumped in every run record.
- **Phase 3.** **Screen and race.** Morris screening on short caps (around 6,000 runs, about 10 hours on 20 cores), then racing on the top parameters at 400k turns. Confirm on validation seeds. Exit: a Pareto set of profiles that beats `c-borg-1.0` with stated confidence.
- **Phase 4.** **Module variants.** Build the situation suite from the dossiers. Make the first module replacements where the death analysis points, each behind a switch with its own fixtures. Exit: each kept variant has fixture, situation and population evidence.
- **Phase 5.** **Generalise and go deeper.** More races and classes, longer caps, and re-tuning of combined profiles. Open the test seed set once. Exit: a milestone review package, in the style of better-borg-c's existing ones.

### borg-rs track (runs alongside)

- **R1b.** **Keep the carries in step.** After the first catch-up (above), new better-borg-c carries are ported in batches, with the same single suite comparison per batch, before either repository's next measurement batch.
- **R2.** **Shared harness and deeper verification.** Adopt the shared seed sets and result schema. Extend identity checks beyond dungeon level 27 with long caps, to cover the code that today rests on reading. Exit: identity on the population seeds Phase 0 freezes.
- **R3.** **Same parameter table (M5).** Implement Phase 2's parameter table as typed fields on `BorgState`. For several non-default profiles, C and Rust must produce the same keys. Exit: differential identity at default and non-default profiles. Follows Phase 2.
- **R4.** **Serialisable Borg memory.** Save and restore `BorgState` next to the game save. This makes exact mid-game continuation possible, and turns the situation suite (harness level 3) from "fresh Borg in an old position" into "this Borg in its exact position". Exit: a resumed run's keys equal the uninterrupted run's.
- **R5.** **Module variants behind traits (M6).** Phase 4's module replacements are developed here first. Variant 0 is the ported original. Measure with the shared harness, and port winners back to C only if C remains the delivery build. Exit: each kept variant has the same evidence Phase 4 requires.
- **Decide.** **Choose the long-term base** after R5 and Phase 4, from data: per-run speed, how hard it was to add a variant, how much it costs to follow upstream, and whether keeping two codebases in step still pays for itself as a cross-check. Until then, every profile or fix exists in both, or is marked as existing in one only.

## Risks

- **Gaming the metric:** survival bought by stalling or idling. Guard with the progress measure and the stall classification.
- **Overfitting:** to Warrior, to the training seeds, or to the turn cap. Guard with held-out seeds, more classes, and occasional longer caps.
- **Upstream drift:** the parameter layer touches many files. Keep it mechanical (constant becomes parameter, nothing else), carry it as its own removal unit, and re-run the identity test after each upstream import.
- **Mixing provenance:** strategy changes must never enter bug carries or upstream proposals. Keep separate labels, patches and issues.
- **Compute contention:** large cohorts share the workstation with other sessions' validation gates. Schedule batches, or use a separate runner machine. borg-rs reports a 4-core machine, which suits identity suites but is too small for tuning cohorts.
- **Two codebases drifting apart:** a fix or parameter added to one but not the other. Guard with the differential identity check, and record in both ledgers which items are present in only one.
- **Upstream changes cost twice:** an upstream Borg change is a normal import in better-borg-c and a manual port in borg-rs. Upstream's Borg changes rarely, but budget for it.

## Decisions for the owner

1. Approve a strategy track in better-borg-c, which changes RULES 12 and the AGENTS scope.
2. Approve R1 in borg-rs now: carrying the fixes changes its rule of keeping C bugs on purpose. Later, approve R3–R5 (parameters, state serialisation, module variants), which change its "no new tactics" scope.
3. Agree the shared contract: one parameter table, one harness frontend, one result schema. Decide which repository owns the shared definitions.
4. Choose the primary objective: the dive target *D* by turn *T*, and how survival and progress are weighed against each other.
5. Set the compute budget, and say whether a dedicated runner machine is available.
6. Decide whether tuned profiles may differ by class, or must be one profile for all classes.

Sources: `BORG_SPEC.md` at `684321538` (PR #317); borg-rs `README.md`, `CLAUDE.md`, `docs/REVIEW.md`, `docs/STATUS.md`, `docs/harness.md` and `harness/suites/` at `74fc879` (local checkout, read only); better-borg-c `README.md`, `PLAN.md`, `RULES.md`, `docs/measurement/` (BASELINE-V1, DEEP-IMPACT-V2, IMPACT-MILESTONE2) on `main` at `8a7eba1ad`; `src/borg/borg.txt` and literal counts from `src/borg/*.c` at `e473720`. Sample sizes come from the normal approximation for two proportions.
