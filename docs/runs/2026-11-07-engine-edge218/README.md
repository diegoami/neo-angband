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

## Verified update: focused source and actual artifact identity

The normal exact-source merge is61139db61. Build passed. [Focused controls](focused.log)
passed598 tests/21 files/zero skips in82.90s; [exit0](focused.exit). Existing CI
routing controls8, publishing guards11, session and all14 version sites pass.
Only the extra existing set-feat-memory.test.ts differs from official06a inside
Core/SDK/content source, and package rules exclude tests. The gameplay/SDK/data
implementation is unpatched official source. Required hosted full is still a
separate merge gate, not inferred from these focused results.

The official Linux archive was actually downloaded and its143757198 bytes and
SHA256564ebc53b46d77c66a0fbcb764ebf33ba38564aca6596ccc21aad8e43f69041b
verified. It is safely unpacked in disposable storage. No GUI was launched and
no stock interactive compatibility is claimed by that download.

[pins.json](pins.json) freezes two reproducible artifact profiles, neither
published to npm:

1. **official06a-source-beta3**: pristine official source, unmodified source
   version1.23.0-beta.3. Core/SDK/content package versions remain beta.3 but the
   actual new source and hashes identify them independently of registry beta.3.
2. **official06a-stamped-edge218**: another pristine official source export,
   stamped with the exact official edge workflow's `node tools/version.mjs edge 218`
   before building. Package versions and Core.ENGINE_VERSION are edge.218,
   matching the official desktop version. All13 changed tracked files are the
   expected14 version sites (RELEASING has two sites); there are no extra gameplay
   source changes. This is a locally reproduced official-source build, not a
   downloaded official npm package or a product publication.

Both profiles build Core, SDK and content and then pack them only in disposable
exports. The artifact manifest includes every actual filename, size, SHA256 and
SHA512 integrity. The stamp's exact changed paths are recorded, not assumed.

## Reproduce the package tuple

From this repository, choose fresh empty directories and archive official06a;
never rewrite an active checkout's versions or workspace ranges:

```sh
mkdir /tmp/neo-edge-repro-source /tmp/neo-edge-repro-packages
git archive 06a2321ce30a414df20d1b20d486446e7688171c | tar -xf - -C /tmp/neo-edge-repro-source
cd /tmp/neo-edge-repro-source
ELECTRON_SKIP_BINARY_DOWNLOAD=1 pnpm install --frozen-lockfile
# Only for the release-style edge.218 profile:
# node tools/version.mjs edge 218
pnpm --filter @rpgm-tools/neo-angband-mod-sdk build
pnpm --filter @rpgm-tools/neo-angband-core build
pnpm --filter @rpgm-tools/neo-angband-content build
node tools/pin-workspace-ranges.mjs --write
pnpm --dir packages/core pack --pack-destination /tmp/neo-edge-repro-packages
pnpm --dir packages/mod-sdk pack --pack-destination /tmp/neo-edge-repro-packages
pnpm --dir packages/content pack --pack-destination /tmp/neo-edge-repro-packages
```

The same steps were executed for both frozen tuples, under the shared validation
lock. Actual Node26.13.0/pnpm12.3.4 identities are in the receipt. Packing uses
pnpm pack, not npm publication. A different packer may produce a different
archive even when source/data are equivalent; consumers record their actual
source/artifact profile rather than assuming version labels imply equality.

## Required consumer SDK graph

Pin all three direct dependencies to the matching local tarballs. In pnpm12,
the transitive SDK override belongs in **pnpm-workspace.yaml**, not the ignored
`pnpm.overrides` package.json field:

```yaml
overrides:
  '@rpgm-tools/neo-angband-mod-sdk': 'file:/tmp/neo-edge-repro-packages/rpgm-tools-neo-angband-mod-sdk-1.23.0-edge.218.tgz'
```

Use the beta.3 filename for the raw-source profile. Preserve the consumer's
existing workspace settings; this snippet is not permission to replace them.
Both a direct Core dependency and the Core's transitive SDK must resolve the
same **actual** file package. The first probe used the unsupported package.json
location: pnpm explicitly ignored it, installed the old registry SDK for Core,
and the realpath guard failed. That failed setup is preserved in local raw
artifacts and is not counted as compatibility success. No engine patch was
needed; the corrected supported override fixes the installation contract.

The corrected isolated consumer probes import actual packed Core/SDK through
bare package specifiers, resolve content, require the public startGame,
describeObject and simulateLoadout exports, and compare direct/transitive SDK
realpaths. [Raw-source probe](source-probe-coherent.log) and
[edge-stamped probe](stamped-probe.log) pass. Each reports one SDK and the correct
actual ENGINE_VERSION. These package/graph checks are producer readiness, not
Mandos plugin/full-suite or desktop-play acceptance. Consumers independently
rebuild and run their own relevant/default gates.
