# Direct borg-rs report intake

borg-rs is an independent Rust Borg port with no Neo Angband responsibility.
It posts reports directly to diegoami/neo-angband-tracker. Those posted
reports are this workspace's intake entry point; no Rust worker or Rust
adoption task is managed here.

## Posted-report triage

Read the report on this tracker and deduplicate against canonical source
issues, existing CBUG IDs and source URLs/anchors. Reuse/comment on the
canonical issue rather than creating another source defect. Include
source:borg-rs plus upstream-c/finding and an appropriate evidence/candidate
label where applicable. Record supplied source repository/issue/commit,
C pin, repro, controls and existing CBUG ID where known; missing evidence
remains a limitation, not an invitation to restart external polling.

Classify C-versus-Rust-only behavior and check source/version/line evidence,
human consequence and reproduction controls. Rust-reported native tests
remain source-reported evidence until independently verified here; never
silently promote a candidate. Ordinary native C source inspection and
verification remain in scope. Rust-only findings do not authorize Rust work.
No automatic issue creation, closure, translation, fix application or official
C posting follows from a directly posted report.

## Retained historical/manual evidence

Do not proactively poll/fetch the borg-rs repository, docs/c-bugs.md or its
external issue board. Do not run tools/borg-rs-watch.mjs at startup/end or
after source changes as a standing routine. Its implementation, tests,
last-successful baseline, existing read-only reference and historical
snapshots remain preserved as legacy/manual evidence. They do not define
a second live queue or authorize another managed clone. A separately
owner-requested historical/manual investigation must have explicit scope;
this document does not authorize background monitoring or Rust adoption.

## Source and consumer boundaries

The canonical C source issue links proposals/native receipts and applicable
Mandos TS translation/adoption records. Independently supplied external
Rust receipts may be linked as evidence; this workspace neither dispatches
nor manages their work. Consumers do not duplicate source state. Only Mandos
issues have its carried/fixed/patch-active tags. Port-fidelity work remains
in neo-angband-borg-mod-tracker, not a C-correction translator. The standing
pause on angband/angband submissions and prohibition on Rust source changes
remain in force.
