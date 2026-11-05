# Neo engine source corrections

| Defect | Upstream proposal | Fork source | Consumer boundary |
| --- | --- | --- | --- |
| [setFeat memory, source4](https://github.com/diegoami/neo-angband-tracker/issues/4) | [429](https://github.com/neostryder/neo-angband/pull/429), `2553cb496da46692fd64eade5f089acecdc5088e`, still open | Exact import/call carried as `9565cab2` on pristine `0cc75b4a`, regressions/FORK marker `9e946f74`; [verification and integration receipt](runs/2026-11-05-engine-notespot/README.md) | No npm release; port [25](https://github.com/diegoami/neo-angband-borg-mod-tracker/issues/25) / [draft67](https://github.com/neostryder/neo-angband-mod-borg/pull/67) need supported pins and full unskipped verification. Mandos [126](https://github.com/diegoami/mandos/issues/126) / [150](https://github.com/diegoami/mandos/issues/150) independently adopt/rebuild/verify. |

The preserved [exact proposal patch](../patches/upstream-neo/429-square-note-spot.patch)
contains only the original source diff; tests and the FORK marker live in the
integration commits. Other proposals (#432/#433/#442) remain unapplied here.

The pristine source reference remains `upstream/master`; `session.json` records
that source base, not a claim that upstream has accepted our carried correction.
The owned default keeps its bootstrap docs and FORK workflow guards. Published
proposal branches and tags are preserved. Source issue4 remains open while
upstream428/429 are unresolved; integration here does not change consumer state.
