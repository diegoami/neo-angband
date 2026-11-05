# Direct borg-rs reports and independent C-fix consumption

Owner policy updated 2026-11-06: borg-rs posts C leads directly as issues on
**diegoami/neo-angband-tracker**. Review that board as the intake source.
Do not routinely fetch/watch the Rust repository, docs/c-bugs.md or its issue
feed, and do not run tools/borg-rs-watch.mjs at session start/end or after
source changes. The script and prior snapshots remain historical tooling and
provenance; their presence does not authorize routine monitoring. Consult the
Rust repository only for an explicit owner request or historical evidence
needed by a specific investigation.

Direct reports use source:borg-rs, upstream-c/finding and an appropriate
evidence/candidate label. Preserve supplied source IDs, commits, C pins and
CBUG IDs. Search existing source issues before creating a finding; reuse the
canonical issue for a duplicate. Missing provenance should be recorded as a
limitation, not invented. A reported native test remains source-reported until
independently validated here. Distinguish C defects from Rust-only behavior.

Retain human consequence, legal native reproduction, controls and evidence
confidence on the canonical issue. No automatic closure, fix application or
official Angband posting is authorized by intake. Historical import records
and file snapshots remain unchanged and are not a second live queue.

The C source issue links local proposals/native receipts and distinct consumer
records where useful. Mandos independently translates/adopts applicable C
corrections to TS; borg-rs independently translates/adopts them to Rust. Neither
is the Neo mod's fidelity task. Consumers do not duplicate source state;
carried/fixed/patch-active labels belong only on the relevant consumer records.

Native C checkpoint: https://github.com/diegoami/neo-angband-tracker/issues/207.
