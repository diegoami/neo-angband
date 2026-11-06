# Worker lanes across three repositories

| Session | Repository | Root under the Angband umbrella | Owns |
| --- | --- | --- | --- |
| Neo engine fixes | [diegoami/neo-angband-tracker](https://github.com/diegoami/neo-angband-tracker) | neo-angband-tracker/ | Neo engine; checkpoint #25 |
| Native C fixes | same repository | neo-angband-tracker/.worktrees/angband-c/ | C Borg/C game; dedicated checkpoint #207 |
| Port fixes | [diegoami/neo-angband-borg-mod-tracker](https://github.com/diegoami/neo-angband-borg-mod-tracker) | neo-angband-borg-mod-tracker/ | Upstream TypeScript Borg port defects and isolated Neo proposals |
| Mandos development | [diegoami/mandos](https://github.com/diegoami/mandos) | better-borg/ | Mandos-only behavior, harness/lab, integration and verified adoption |

Each session reads its own AGENTS/RULES/PLAN/checkpoint. It does not inherit
another repository's queue or start fixing a different component silently.
The old neo-angband lab checkout and pristine neo-angband-upstream / original
neo-angband-mod-borg checkouts are retained references, not these workspaces.
Do not change the live lab or its mandos-lab-data while bootstrapping.

One GitHub Angband fork network is available: our neo-angband fork therefore
hosts Neo engine master and the separately based C workbench/reference topic
branches. A C proposal must never be based on Neo TypeScript master. The
native C worktree is .worktrees/angband-c inside the engine session root;
it is an independently owned worker lane in the same repository. Its checkpoint
is #207; Neo engine checkpoint #25 remains separate.

Upstream reports stay on angband/angband or the appropriate neostryder repo
only under the standing posting rules. Our fork issues are our canonical
trackers; transferred Mandos URLs remain historical redirects. See the
migration ledger and each checkpoint for exact current issue numbers.

## C correction flow and independent Rust peer

The engine/C tracker proves/proposes C fixes and retrieves C leads from
borg-rs through direct source-attributed issues on this tracker. Routine
fetching/watching of its repository, bug file and issue feed is retired. Mandos independently
translates applicable C corrections into TS and carries/adopts them. The
fidelity-only Neo Borg mod tracker does not perform that translation service.
borg-rs independently translates/adopts C corrections in Rust, without a Neo
responsibility; its read-only reference is not a fourth managed Neo session.

## Native C in a fresh cloud session

Use a dedicated clone of `workbench/c-borg`. The fork's default `master` is
Neo TypeScript, so starting a C investigation there selects the wrong engine.
Read AGENTS.md, RULES.md, PLAN.md, session.json and the latest comments on
[checkpoint207](https://github.com/diegoami/neo-angband-tracker/issues/207)
before choosing work. That checkpoint owns live task/review status; the recipe
below owns cloud setup. Local workstation paths and previous `/tmp` directories
are not prerequisites. If the cloud environment provides a runtime skill,
follow it for network/proxy and credential setup first.

```sh
git clone --single-branch --branch workbench/c-borg https://github.com/diegoami/neo-angband-tracker.git angband-native-c
cd angband-native-c
```

For Debian/Ubuntu, the native reproduction needs Git, Python3, CMake, a C
compiler/Make, util-linux (`flock`) and X11 development files. The CMake defaults
select X11 on Linux, even though the observer runs headlessly. If absent, install
these through the cloud environment's package setup:

```sh
sudo apt-get update
sudo apt-get install -y git python3 cmake build-essential util-linux libx11-dev
```

Do not use pnpm, the Neo build, a workstation save, or the running lab for this
C task. The fixture constructs its own character, inventory, monsters and arena.

To repeat the accepted CBUG-0066 evidence at its immutable publication pin:

```sh
git fetch --no-tags origin task/cbug-0066-evidence
git show 1b67370d0dc56d9b7ef34e5356e63d21a7095545:docs/audit/c-reports/CBUG-0066/expanded/reproduce.py > /tmp/reproduce-cbug-0066.py
python3 /tmp/reproduce-cbug-0066.py --fixtures-ref 1b67370d0dc56d9b7ef34e5356e63d21a7095545 --full-tests --compare-simple
```

The runner obtains C base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`
if missing, creates a disposable detached C worktree, builds both variants and
writes logs, result.json and evidence/replay.html under a fresh printed `/tmp`
directory. Heavy commands use `/tmp/angband-validation.lock`. Expect five
complete baseline scenario failures, nine corrected passes, four byte-identical
controls, and 965/965 unit checks plus 5/5 frontend tests on both full gates.
The simple reversal passes seven cases and fails exactly armour-only and
weapon-only. Its helper restores the guarded source/binary afterwards. Inspect
assertion summaries: the native unit runner can return zero after a failed
assertion. The Python runner checks the expected outcomes.

The runner deliberately reproduces tested source revision `017ba47e`; source
proposal head `ebcd255ba9064fc44a35d693f42f9df3dc00bd88` differs only in
this owner-requested comment: `/* prefer the lower danger among available swaps. */`.
Compare before changing either the evidence pin or source claim:

```sh
git fetch --no-tags origin task/c-borg-safer-swap
git diff 017ba47ebc8731fa0a61361a6f6777759a748d2a ebcd255ba9064fc44a35d693f42f9df3dc00bd88 -- src/borg/borg-item-wear.c
```

Download the generated replay.html from the cloud artifact/file viewer and open
it locally. Select **Two threats; both backups help**, then **Show decision**
and **Show damage**. Both start at450HP; the original selects the frost dagger
and ends337HP, the guarded fix selects the shield and ends391HP. This is captured
native output, not a live autonomous run. The fixture controls setup, attack
scheduling and application of the queued wield; production Borg code chooses
and native game code computes damage. No encounter-frequency or death claim.

The source proposal is on `task/c-borg-safer-swap`, evidence on
`task/cbug-0066-evidence`, and shared verification policy on
`task/human-verifiable-bug-evidence`. Fetch a named branch before inspecting it
in a single-branch clone. Keep pristine `c-borg-reference` and published history;
never merge C source into Neo master. Upstream review is at
[angband/angband6809](https://github.com/angband/angband/pull/6809).
Agent posting there remains paused. Check live status rather than assuming
acceptance. Shared policy/evidence review and source adoption are distinct;
checkpoint207 records outstanding decisions. Do not restart knockout experiments
or promote deferred source100 from a source-reading lead into a verified bug.

## Reporting the next native C bug

Follow RULES17 before posting. A maintainer new to this work must first learn
what situation the Borg encounters, what it does wrong, what consequence was
measured and what changes after the fix. Use CBUG-0066 (PR226/issue82) as the
worked example of reasoning: show the bad choice and its consequence under
unchanged conditions, explain the correction, and test plausible simpler
alternatives when relevant. The replay supports that explanation; it does
not replace it. Put pinned commands, relevant output, controls and limits
under "Verification" after the plain explanation. Read the opening alone
before posting and rewrite it yourself if it still needs a translation.
