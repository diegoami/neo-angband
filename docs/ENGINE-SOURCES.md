# Neo engine source corrections

| Defect | Upstream proposal | Fork source | Consumer boundary |
| --- | --- | --- | --- |
| [setFeat memory, source4](https://github.com/diegoami/neo-angband-tracker/issues/4) | Original [429](https://github.com/neostryder/neo-angband/pull/429), `2553cb496da46692fd64eade5f089acecdc5088e`; upstream explicitly accepted the correction and closed [428](https://github.com/neostryder/neo-angband/issues/428) completed | Existing exact executable hook imported as `9565cab2`, regressions/FORK marker `9e946f74`, normal merge `19ff25ea`, retained in verified `5928146b`; [original integration evidence](runs/2026-11-05-engine-notespot/README.md). Source issue closed after accepted resolution and already-complete fork import/checks. | [Upstream mod67 acceptance](https://github.com/neostryder/neo-angband-mod-borg/pull/67#issuecomment-6676556092) reports core1.23.0-beta.3 and port70/`440d8d9` with915 tests. Port [25](https://github.com/diegoami/neo-angband-borg-mod-tracker/issues/25), Mandos [126](https://github.com/diegoami/mandos/issues/126) / [150](https://github.com/diegoami/mandos/issues/150) retain independent adoption authority; our source resolution changes no consumer dependency, backport or lab. |
| [disturb input-flush, source5](https://github.com/diegoami/neo-angband-tracker/issues/5) | [432](https://github.com/neostryder/neo-angband/pull/432), `e8729a285474325ca25a03534db14f8c10808794`, still open | Exact existing correction/tests/docs imported as `b02adb89`; FORK provenance `94f2d1f6`; [verification and integration receipt](runs/2026-11-06-engine-input-flush/README.md) | No npm release; [Mandos128](https://github.com/diegoami/mandos/issues/128) independently owns its existing carry/adoption/rebuild/consumer verification. No patch removal or lab deployment follows from this import. |

The preserved [exact proposal patch](../patches/upstream-neo/429-square-note-spot.patch)
contains only the original source diff; tests and the FORK marker live in the
integration commits. The [exact input-flush proposal patch](../patches/upstream-neo/432-disturb-input-flush.patch)
retains its source/tests/docs diff; original author/date are preserved by its
import commit. Other proposals (#433/#442) remain unapplied here.

The pristine source reference remains `upstream/master`; `session.json` records
that source base, not a claim that upstream has accepted our carried correction.
The owned default keeps its bootstrap docs and FORK workflow guards. Published
proposal branches and tags are preserved. Source4 is resolved as recorded below;
source5 and its consumer remain independent. Canonical issues own current live
state; this ledger is a source/provenance index, not another status database.


## noteSpot resolution reconciliation — 2026-11-06

[Upstream maintainer acceptance](https://github.com/neostryder/neo-angband/pull/429#issuecomment-6676136585)
confirms trap cleanup followed by noteSpot and closes the proposal because the
public repository replays private history. Its older sentence leaving428open
is superseded by issue428's later CLOSED/completed resolution at02:56:39Z.
Trust that original-scope resolution under RULES9, rather than holding the source
issue for speculative close-door/blind/refusal concerns. An actual recurrence
after adoption requires a new linked report.

The accessible pinned public source is
[0e24e79c:trap.ts214–218](https://github.com/neostryder/neo-angband/blob/0e24e79c47adb9fd14eacdf87ba1f9be269759fa/packages/core/src/game/trap.ts#L214-L218).
It executes the same trap-side-effect then noteSpot hook as our already-integrated
source, apart from our provenance comment. The maintainer cited short commit
394825525, but the public commit endpoint cannot currently resolve it; use the
accessible immutable source above for the code comparison, not an invented full
hash. Its package manifest declares core1.23.0-beta.3; actual npm delivery is
attributed to the linked maintainer/port receipt, not inferred from that manifest.

Copyable checks from this fork:

```sh
git merge-base --is-ancestor 19ff25eac8997e8feb6b5e62eefa7ffe4d134631 5928146b85cc498ca596da16571b6ee28396ef4c
git show 5928146b85cc498ca596da16571b6ee28396ef4c:packages/core/src/game/trap.ts | sed -n '211,225p'
```

The ancestry check exited0, and the hook matches the pinned upstream executable
sequence. The existing integration retains unchanged4fail/3pass-before and
148pass-after focused controls plus normal full14150/12. Actual later
[current-fork post-merge42150196523](https://github.com/diegoami/neo-angband-tracker/actions/runs/42150196523)
passed14154/12 and package/parity/MCP/live checks. No new runtime validation is
claimed or required for this unchanged-code bookkeeping reconciliation.

Mandos's existing beta.2 package/backports and the separately installed lab do
not become beta.3 through this source closure. Preserve each patch-active
removal unit until that consumer proves its replacement. No source update,
package publication, patch removal, consumer state change or lab deployment is
part of this ledger correction. Historical run receipts remain append-only.

## Exact beta.3 integration — 2026-11-06

Owner task [299](https://github.com/diegoami/neo-angband-tracker/issues/299)
imports the exact released97ceec2925e6715ba725624a61866ab93a196a12 by normal
merge while preserving existing fork corrections/docs/workflow containment.
[Source/package audit and consumer handoff](runs/2026-11-06-engine-beta3/README.md)
records registry gitHead/integrities, actual archive hashes and independent
replacement-unit checks. Published beta.3 includes noteSpot429 and class counts0cc75;
it still lacks input-flush432 and seed/birth433. The fork keeps its existing
input-flush correction, but433 remains unapplied in this source profile.

Source4/15 were already closed after accepted source correction/import; source5/6
remain open because432/433 are still unmerged upstream and absent from the released
package. No consumer patch-active or lab state changes automatically. The earlier
resolution receipts remain historical; the new package audit establishes actual
npm delivery instead of relying only on a version string or maintainer statement.
