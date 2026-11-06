# Repair patches

Active native C proposal patches are under upstream-c/, linked from the
canonical finding issue/record and local PR. A stored patch is not applied.
Preserve source/base pins and before/after/control receipts; only claim a C
outcome after native legal reproduction. C submissions remain paused.

Mandos maintains its own published-package/lab backports with named consumer
receipts; those are distinct from a C-source patch here. Historical copies in
Mandos remain frozen for old report links. Do not update two live ledgers.

Neo engine corrections carried in this fork are indexed in
[the engine source ledger](../docs/ENGINE-SOURCES.md). Stored proposals and
source integrations are distinguished from npm publication and consumer adoption.

## CBUG-0094 preparation-spell simulation

Source [issue #110](https://github.com/diegoami/neo-angband-tracker/issues/110),
C source `31a042d00581ed914d30f17cca1ed0e70e99d21f` against pristine C
`e473720ed88a0f0c7ca66815cef3000eb4a38bc3`.
[Stored patch](upstream-c/CBUG-0094-regeneration-simulation.patch) and
[native verification, full logs and portable reproduction](../docs/audit/c-reports/CBUG-0094/README.md).
Returning regeneration's score during simulation prevents an extra cast:
20 less mana and 100 less native command energy in each of two cases, nine
corrected passes and seven identical controls. The fixture isolates the
preparation selector and decodes its queued actions into native cast commands;
no full-autoplay or elapsed-world-turn claim. This is a stored/local C proposal,
not source adoption or a correction applied to an engine or consumer.
