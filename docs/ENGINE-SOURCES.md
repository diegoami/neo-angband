# Neo engine source corrections

| Defect | Upstream proposal | Fork source | Consumer boundary |
| --- | --- | --- | --- |
| [setFeat memory, source4](https://github.com/diegoami/neo-angband-tracker/issues/4) | [429](https://github.com/neostryder/neo-angband/pull/429), `2553cb496da46692fd64eade5f089acecdc5088e`, still open | Exact import/call carried as `9565cab2` on pristine `0cc75b4a`, regressions/FORK marker `9e946f74`; [verification and integration receipt](runs/2026-11-05-engine-notespot/README.md) | No npm release; port [25](https://github.com/diegoami/neo-angband-borg-mod-tracker/issues/25) / [draft67](https://github.com/neostryder/neo-angband-mod-borg/pull/67) need supported pins and full unskipped verification. Mandos [126](https://github.com/diegoami/mandos/issues/126) / [150](https://github.com/diegoami/mandos/issues/150) independently adopt/rebuild/verify. |
| [disturb input-flush, source5](https://github.com/diegoami/neo-angband-tracker/issues/5) | [432](https://github.com/neostryder/neo-angband/pull/432), `e8729a285474325ca25a03534db14f8c10808794`, still open | Exact existing correction/tests/docs imported as `b02adb89`; FORK provenance `94f2d1f6`; [verification and integration receipt](runs/2026-11-06-engine-input-flush/README.md) | No npm release; [Mandos128](https://github.com/diegoami/mandos/issues/128) independently owns its existing carry/adoption/rebuild/consumer verification. No patch removal or lab deployment follows from this import. |

The preserved [exact proposal patch](../patches/upstream-neo/429-square-note-spot.patch)
contains only the original source diff; tests and the FORK marker live in the
integration commits. The [exact input-flush proposal patch](../patches/upstream-neo/432-disturb-input-flush.patch)
retains its source/tests/docs diff; original author/date are preserved by its
import commit. Other proposals (#433/#442) remain unapplied here.

The pristine source reference remains `upstream/master`; `session.json` records
that source base, not a claim that upstream has accepted our carried correction.
The owned default keeps its bootstrap docs and FORK workflow guards. Published
proposal branches and tags are preserved. Source issues4/5 remain open while
their upstream428/429 and414/432 threads are unresolved; integration here does not change consumer state.
