# borg-rs intake and independent C-fix consumption

borg-rs is an independent Rust Borg port. It supplies C defect leads and may
carry/translate C fixes into Rust. It has no Neo Angband responsibility.
This engine/C session fetches it read-only; it does not alter or push Rust
code. The dedicated reference clone has push disabled.

Run node tools/borg-rs-watch.mjs at session start/end and after source changes.
It inspects default-branch head, docs/c-bugs.md when present and the GitHub
issue feed when enabled. Missing file + available issues is a supported
transition, not evidence that old reports vanished. API failures retain the
last successful baseline; a feed window limit is reported as incomplete.

New/edited leads require C-vs-Rust-only classification, source/version/line
or issue evidence, human consequence and duplicate checks against existing
CBUG IDs/source URLs. Rust-reported native tests are source-reported evidence
until independently verified here; never silently promote candidates.

If borg-rs posts directly here, use source:borg-rs plus upstream-c/finding and
an appropriate evidence/candidate label, include source repository, issue/commit,
C pin, repro/controls and existing CBUG ID where known. Reuse/comment on the
canonical issue instead of creating another source defect. No automatic issue
creation, closure, translation, fix application or official C posting.

The canonical C source issue links proposals/native receipts and, when useful,
separate consumer records: Mandos for applicable TS translations/adoption;
borg-rs for Rust translation/adoption. Consumers do not duplicate source state.
Only Mandos issues have its carried/fixed/patch-active tags. Port-fidelity
work remains in neo-angband-borg-mod-tracker, not a C-correction translator.
