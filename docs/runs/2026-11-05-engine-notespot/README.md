# Engine noteSpot integration, 2026-11-05

Canonical source defect: [engine issue 4](https://github.com/diegoami/neo-angband-tracker/issues/4).
Proposal: [neostryder/neo-angband PR 429](https://github.com/neostryder/neo-angband/pull/429),
`2553cb496da46692fd64eade5f089acecdc5088e`. Upstream issue 428 and PR 429
remain open; there are no check runs on that proposal head. This is an explicitly
owner-authorized integration into our fork, independent of upstream acceptance.

## Reconciliation and source

Read AGENTS.md, RULES.md, PLAN.md, session.json, docs/SESSION.md and the latest
[checkpoint](https://github.com/diegoami/neo-angband-tracker/issues/25#issuecomment-6670747127).
Fresh fetches confirmed upstream engine `0cc75b4afcbe6f5128730e4b910408c7be4ac5c5`
and current C `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Owned engine default
was `313ecb0784c0dea097b323d48ba3c3305b977e34`; its actual CI run
[42113910504](https://github.com/diegoami/neo-angband-tracker/actions/runs/42113910504)
passed both test and live-game jobs. The independent reorganization review
remains open; this task does not change its verdict.

The source/issue monitor completed: 202 source/consumer trackers, 75 references,
no audit problems or observed movement. The C monitor still matches its reference;
borg-rs stays `c375a82456741f7e4d4d410b27390ba68077ba9c`, 41 groups, no issue feed.
The live upstream open-issue and PR boards were inspected, including 428/429/468;
429/432/433/442 are still open with unchanged proposal heads. PR metadata and
synthetic merge SHAs were not used as source-correction evidence.

Actual source comparison confirms installChunkFeatHook calls only the trap side
effect before this correction. Vendored Angband 4.2.6 `reference/src/cave-square.c:1256-1268`
calls square_note_spot after removing invalid traps, only in character_dungeon.
`reference/src/cave-map.c:226-245` guards memory writes by seen/player square.
The existing TypeScript noteSpot supplies that guard. Generation has no live hook.
The proposal adds the import/call and updates its source citation. We carry those
exact executable changes, with a FORK explanation beside the call; no other
engine proposal is included.

Implementation began in isolated `task/engine-notespot` from pristine upstream,
not the documentation-bearing default. The original proposal was cherry-picked
as `9565cab2`; published original head/tags were preserved. Regression/FORK commit
is `9e946f74`. Documentation-bearing default was merged normally into the task
at `fbc7e02b95fa8b48b58b7a2e28ab3e07b7790285` for integrated validation.

## Regression proof

New `packages/core/src/game/set-feat-memory.test.ts` takes its expected results
from the C sites above. Synthetic legal fixtures use seed 1, player (5,5),
adjacent door (6,5), explicit SEEN flags, no monsters/lock and no post-command
updateView/noteSpots pass. The processPlayer command registry executes normal
close/open energy and refusal rules; assertions inspect memory before the next
command. These are engine fixtures, not a natural generated dungeon/FOV test.

All assertions and controls are unchanged between runs:

- Pristine engine: **4 fail / 3 pass / zero skips**, exit 1. Seen floor-to-open,
  seen floor-to-granite, unseen player-square update and legal close/open fail
  because remembered terrain remains old. Both remote-unseen controls (known
  and unknown) and the no-live-hook generation/flag-clear control pass.
- Exact proposal plus FORK comment: **148 pass / zero skips**, exit 0 across the
  new seven tests plus trap, known, known-pile and cave-cmd suites. Legal door
  sequence closes/opens twice for full energy and refuses a redundant open at
  zero energy; memory stays current at every boundary.

Commands (working directory is the isolated engine task):

```sh
ELECTRON_SKIP_BINARY_DOWNLOAD=1 pnpm install --frozen-lockfile --offline
CI=1 flock /tmp/angband-validation.lock pnpm exec vitest run \
  packages/core/src/game/set-feat-memory.test.ts --reporter=verbose
# Apply exact proposal, then repeat with unchanged tests plus existing suites:
CI=1 flock /tmp/angband-validation.lock pnpm exec vitest run \
  packages/core/src/game/set-feat-memory.test.ts \
  packages/core/src/game/trap.test.ts packages/core/src/game/known.test.ts \
  packages/core/src/game/known-pile.test.ts packages/core/src/game/cave-cmd.test.ts \
  --reporter=verbose
```

Node is `v26.13.0`, pnpm `12.3.4`, TypeScript `5.9.3`, Vitest `4.1.11`.
Source core, SDK and content versions are all `1.23.0-beta.2`; no npm publication
is implied. SDK tree `dc7f491841ffa412cbde81269375ea66a3b0735f` and content pack
tree `d72413a575ea48cb94da046ce5e47370e2932a97` are unchanged. Corrected core
source tree including regressions is `b70fb735a44e75aa4fb8dc2a6fdf8930c574f9ce`.
The source lockfile remains unchanged from pristine engine.

## Consumer boundary and prior evidence

Port upstream is `74a017d2f695939eb2f046b7c9ff286ef1e82568`.
[Port draft 67](https://github.com/neostryder/neo-angband-mod-borg/pull/67) remains
at `1d1793258b887efae5c862a15a92683b8258c33c`, core/SDK/content published beta.1.
[Port source 25](https://github.com/diegoami/neo-angband-borg-mod-tracker/issues/25)
owns supported dependency selection and the required full suite with relevant
content cases executing. A source checkout pin is available after integration;
it is not a newly published npm version or a verified consumer installation.

The [prior controlled port record](https://github.com/diegoami/neo-angband-borg-mod-tracker/blob/master/docs/runs/2026-11-05-port-reconciliation/README.md)
uses Node24.21.0/pnpm12.16.1, pristine port74a017d, published core/SDK beta.1
and byte-identical content pack d72413a5. Baseline is 6 pass/4 repeated-open
failures; a disposable core with ONLY429 passes all10 play/rest cases with
unchanged seeds1/7/42/99, 1500 decisions and120s timeouts. The initial9/10
run's timeout and its unchanged focused passing retry remain part of that
record. This session does not rerun or relabel that evidence. The
[hosted reconciliation](https://github.com/diegoami/neo-angband-borg-mod-tracker/blob/master/docs/runs/2026-11-05-hosted-reconciliation/README.md)
records default830pass/36skips versus draft836pass/4fail/26skips on beta.1;
those results remain valid at their exact pins.

No #67 graduation follows solely from this engine integration. No full port
suite, desktop API2 stall/countdown behavior, every seed or within-grade confusion
resolution is claimed. [Mandos126](https://github.com/diegoami/mandos/issues/126)
and [Mandos150](https://github.com/diegoami/mandos/issues/150) independently own
adoption, rebuilding and consumer verification. Their states and patches are
unchanged. Lab engine9d6ae21ec / installed mod48405e9 stay on their existing build;
no lab deployment, release, save/profile/data writes or C submission occurs.

## Completed local integration gates

All stages in [gates.json](gates.json) completed with exit0 on integration
commit `fbc7e02b95fa8b48b58b7a2e28ab3e07b7790285`; subsequent record commits
change only this evidence/ledger, not tested product source. Each stage ran
sequentially under `flock /tmp/angband-validation.lock`, with logs watched.
The wrapper set Node26.13.0 and CI=1, recorded each exit/timing, and stopped on
any failed stage. Commands are the arrays in gates.json; logs are adjacent.

- `pnpm build` (also the repository typecheck), `pnpm lint` and production web
  bundle pass.
- `pnpm test`: **14150 pass / 12 existing skips**, 717 files pass / 3 skip,
  158.66s reported test duration (159.01s wrapper). No relevant new regression
  skips, changed assertions/seeds/timeouts or waived failures. Existing optional
  C statistical/generation probe and Windows Electron canary controls are not
  certified by this Linux unit run; this is not a new 1000-level C parity run.
- CLI golden scenarios: **3/3**; MCP transport smoke passes.
- `pnpm check:npm`: all three publishable packages pass the packaging checker,
  npm11.20.0. Its existing isolated-import check skips core/SDK because they
  declare dependencies; this is not an installed consumer/full-port proof.
  No package was published. Hosted CI separately installs npm@latest.
- Desktop build and both smoke-filter self-checks pass. `xvfb-run -a node
  tools/play-smoke.mjs --no-sandbox` plays **11 steps**, no exceptions/error
  logs. It uses the disposable task build/profile, not the owner's running lab.

Raw diagnostic logs are preserved with task paths replaced by `<ENGINE_TASK>`
and terminal color codes/trailing whitespace removed; results/assertions are unchanged. Original
local logs and screenshots remain in the task worktree. The prior port timeout
is preserved in its original append-only record, not reclassified by these gates.

Workflow audit: `node tools/check-fork-workflows.mjs` passes all11 inherited
product job guards; the integration has no workflow changes relative to owned
master. Live CI and session validation are active; edge and native C release
remain disabled_manually. Upstream-only guards also contain registered Neo
publish/release/Pages/canary jobs. Version-tag ruleset26648434 remains active
with no bypass; no product tag is created/updated/deleted. Session checker
passes outside the sandbox after its initial Git subprocess received EPERM.
