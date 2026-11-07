# Engine edge.218 source integration, 2026-11-07

Task [372](https://github.com/diegoami/neo-angband-tracker/issues/372), parent
[workspace26](https://github.com/diegoami/angband-workspace/issues/26).
Base23f0292f0ff8e65545002ccfb4bf0fac1d0ded47 preserves the owner's fourteen
C-documentation commits after the previous17d2c37 engine baseline. Exact official
source06a2321ce30a414df20d1b20d486446e7688171c is the GitHub edge.218 tag target.
No native C source or independent worktree is changed.

## Profiles and publication

Official GitHub desktop edge.218 is a stamped automatic build, not a new npm
release. Registry Core/SDK/content remain latest1.22.0/next1.23.0-beta.3; querying
Core1.23.0-edge.218 returned404. All source package versions remain beta.3.
Unpatched exact-source artifacts must therefore name source/hash/graph identity.
No product tag, release or package is published by this integration.

Official Linux archive: Neo.Angband-1.23.0-edge.218-x64.tar.gz,143757198 bytes,
SHA256564ebc53b46d77c66a0fbcb764ebf33ba38564aca6596ccc21aad8e43f69041b.
Its actual download/verification and compatibility results remain pending.

## Merge and accepted scopes

Four conflicts contain our earlier input-flush contribution only: CHANGELOG,
both MOD_SEAMS copies and the obsolete local FORK comment in player-path.ts.
Those files now use exact upstream06a contents. Input-flush runtime and tests
were already byte-identical apart from the retired carry comment; none of the
source behavior or tests are weakened. All eleven inherited product-job guards
remain; no Actions workflow changed upstream in this13-commit refresh.

Upstream explicitly accepted the proposed source fixes despite the proposals
showing CLOSED rather than MERGED, because its published history is replayed:

- [432 acceptance](https://github.com/neostryder/neo-angband/pull/432#issuecomment-6706058643): bc41839, input-flush/source5.
- [433 acceptance](https://github.com/neostryder/neo-angband/pull/433#issuecomment-6706058856):67b1d8,82656b,bd396, truthful seed/accepted birth/source6.
- [442 acceptance](https://github.com/neostryder/neo-angband/pull/442#issuecomment-6706059093):ab4263, load-resume/source7.
- [477 accepted controller resolution](https://github.com/neostryder/neo-angband/issues/477#issuecomment-6706058424):a984e04,8d6b8ef,06a2321.

Source5/6/7 closure waits actual fork import plus normal required verification.
Source8 is still a mixed418/385 watch; recording the accepted477 leaf does not
claim all first-party management or headless capabilities fixed. Consumers own
independent adoption/retirement states; no consumer labels appear on source bugs.

## Capabilities and compatibility boundaries

The new public controller route uses an isolated Worker, declared controller
permissions and content-bound player consent. Worker code cannot access raw
Core. The documented headless --unsafe-in-process and lab-only ?labprocess=<id>
set developerInProcess; with engine:access the engine domain can expose Core,
and consented in-process truth is possible. Management remains first-party
listed. The lab URL option exists only in NEO_ANGBAND_LAB_BUILD=1 builds; it is
not an ordinary release-build setting. These source traces are not yet actual
Mandos desktop or runner proof. No identity spoof or host rewrite is authorized.

## Required verification and handoff

Focused critical Core/SDK/web/runner, fork/session and CI checks are pending,
then one required hosted full build/lint/tests/package/parity/MCP/live gate.
No redundant local full is planned. An immutable unpatched official06a package
recipe and artifact tuple will follow; packing edits only a disposable archive,
never active manifests. Exact-source transitive SDK must match the direct SDK.
No private lab/profile/save/data, backup/OAuth, Rust polling/dispatch, native C
implementation or official Angband submission occurs here. Parent owns merges,
actual-main results, consumer final readiness and workspace recovery pins.
