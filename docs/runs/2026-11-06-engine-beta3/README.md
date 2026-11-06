# Exact beta.3 engine integration and consumer handoff

Canonical integration task: [source299](https://github.com/diegoami/neo-angband-tracker/issues/299).
Parent: [workspace22](https://github.com/diegoami/angband-workspace/issues/22).
This is the Neo engine lane; independently owned native C207 and its worktrees,
proposal branches and validation remain untouched.

## Source and package identity

Integrate **97ceec2925e6715ba725624a61866ab93a196a12**, the actual GitHub
v1.23.0-beta.3 release and npm gitHead of Core, SDK and content beta.3.
The owned base is f186aa1fce2e590eb9bbcb91dc369f3f8b6af688; the normal release
merge is ea570d397 (its full ID is recoverable from this branch's ancestry).
Common Git base0cc75 contains the accepted class-book correction; a Git version
label alone did not prove the older beta.2 npm artifact contained it.

Only four release commits are imported: d6b760290 (API2 mod-list handling),
5437472e2 (noteSpot), cee2e08a7 (noteSpot regression) and97ceec292 (version).
No later upstream master commit is selected. The explicit fetch used
`--no-tags` and stored the commit at refs/remotes/upstream/release-beta3;
no local product tag was created or pushed.

[Machine-readable package/source receipt](pins.json) records all three exact
registry URLs, gitHead, SHA-512 integrities, downloaded archive SHA-256 and sizes.
Each actual archive's computed SHA-512 equals its registry integrity. Archives
are recoverable from those immutable version URLs; no bulky tarball is committed.
Npm pack is a local download/pack operation here, not publication.

## Merge and containment

The two conflicts were resolved narrowly:

- CHANGELOG preserves our unreleased input-flush correction and upstream's
  beta.3 release section.
- trap.ts uses upstream's single trap-cleanup then noteSpot sequence. Its old
  local FORK-carry comment is retired because the accepted upstream implementation
  is now the actual imported source. Existing independent set-feat-memory and
  input-flush regression suites remain; upstream's new live-setFeat test is added.

No Actions workflow changed between release base and release, or in the owned
integration diff. `check-fork-workflows.mjs` independently passes all11 inherited
repository guards. Product tag blocking/publishing containment remains; there
was no tag, release, npm publication, Pages deployment or lab operation.

The isolated frozen install set two tracked SDK bin files executable. Their
content was unchanged; restoring their original100644 modes removes only that
setup side effect. No generated source content was discarded. Package/lock
versions are upstream beta.3; pnpm-lock.yaml itself is unchanged.
session.json's Neo engine source baseline is now97ceec; port/C baselines remain.

## Backport and bug-state disposition

| Independent unit | Actual published beta.3 check | Source state / consumer responsibility |
| --- | --- | --- |
| noteSpot429 / source4 | Exactly one noteSpot call follows trap cleanup in dist/game/trap.js's live hook | Source4 already CLOSED after explicit accepted resolution and equivalent fork import. Port25 and Mandos126/150 retain their own verified package-adoption authority; remove only this package unit after their controls pass. |
| Class-book count0cc75 / source15 | dist/player/bind.js uses `numSpells: spells.length` | Source15 already CLOSED. Mandos147 stays independently verified; its package patch-active maintenance can retire after actual beta.3 all-class lookup checks. |
| input-flush432 / source5 | Published disturb() still has no input-flush signal | Upstream432 is OPEN/unmerged, source5 stays OPEN. Existing fork-source correction is retained; published-package consumers still need their marked carry. No release-number closure or patch removal. |
| Accepted-birth/seed433 / source6 | RunReport lacks seed/birth fields; context lacks birthRngState; save runtime lacks masterSeed | Upstream433 is OPEN/unmerged, source6 stays OPEN. It was not imported into this source fork and is not added by this release task. Mandos129 must preserve its separately audited package/runtime/declaration carry. |

These are direct published-artifact checks, not inference from open PR titles.
The source fork profile and published-plus-consumer-backports profile are
**different**: the former adds source5 flush but does not add source6 birth
provenance; the latter may carry both432/433. Do not claim one substitutes for
the other. No source carried/fixed/patch-active labels are introduced here.
Canonical source and consumer issues own future live state.

## Exact consumer recipe

Prefer matching official Core, SDK and content **1.23.0-beta.3** for the default
consumer profile, with only its independently verified remaining backports.
Inspect the final dependency graph: changing a direct SDK without correcting
an old transitive override does not prove one coherent SDK instance.

For a named fork-source profile, use this PR's exact approved source commit
(or the separately verified merged head supplied by the parent), not moving
master or an arbitrary npm version. In an isolated source tree:

```sh
ELECTRON_SKIP_BINARY_DOWNLOAD=1 pnpm install --frozen-lockfile
pnpm build
mkdir -p /tmp/neo-beta3-fork-packages
npm pack ./packages/core ./packages/mod-sdk ./packages/content --ignore-scripts --pack-destination /tmp/neo-beta3-fork-packages
```

Use those source artifacts/paths with the consumer's explicit exact-source
Core/SDK/content recipe and record their actual hashes. Their package version
also says beta.3; that does not make them the official registry artifact. Keep
the fork archive directory separate from official package downloads. All heavy
local builds/tests, including this recipe, use /tmp/angband-validation.lock.
No package publication is part of this recipe.

## Verification and limits

[Focused log](focused.log), numeric [exit0](focused.exit): isolated frozen install,
`pnpm build`,61 tests across trap.test.ts/set-feat-memory.test.ts/player-path.test.ts
(all3 files passed, zero skips), eight actual CI range/scope controls, eleven
workflow guards, session check and fourteen aligned version sites pass.
The first attempted control filename was nonexistent and is not counted as a
pass; the actual tools/ci-scope.node-test.mjs subsequently passed8/8.

Required hosted full engine/build/lint/bundle/parity/package/MCP and actual live
production game checks remain separate before parent merge approval. A focused
hook test is not a full browser deployment or proof all consumer variants work.
No duplicate local full is started when exact-head required hosted gates suffice.
Any real failure retains its result and requires attribution; no existing skips,
seeds, assertions or timeouts are waived. Parent owns merge/post-main observation
and workspace recovery pins; consumers own their own patches/plugin/gates and
lab deployment is not authorized by a source merge.

Full release-merge identity: ea570d3975803e2d7dece67915b8f9a2e4d4689c.
The focused terminal log is deliberately tracked as reviewed public test evidence
although the repository's general *.log rule ignores disposable logs. It contains
only this synthetic source build/test output, not lab/profile/credential data.

Privacy normalization follow-up: the terminal output contained one pnpm store
path naming the local user home. The initial tracked-file scan flagged that
path; the command wrapper incorrectly continued into the normal commit/push.
A new normal follow-up replaces only that filesystem prefix with `<user-home>/`;
all build/test outcomes remain unchanged. No credential or owner lab/profile
data was involved, and no history rewrite is performed. The clean final scan
is the applicable result; the failed attempt is retained in this receipt.
