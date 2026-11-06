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

## CBUG-0061 healing-potion hesitation

A wounded, confused Warrior last drank a healing potion five Borg turns ago.
The original leaves its potions unused; the correction drinks one, heals
60 HP and clears confusion. The recent-drink inverse conserves a potion but
leaves injury/confusion untreated on that call. Emergency bypass still drinks;
this is a resource tradeoff, not a uniform safety claim.

Source [issue77](https://github.com/diegoami/neo-angband-tracker/issues/77),
C source `cedf25eb790f2ff7c7c161cd1612b18ed99fb35e` against pristine C
`e473720ed88a0f0c7ca66815cef3000eb4a38bc3`.
[Stored patch](upstream-c/CBUG-0061-healing-potion-timing.patch) and
[readable comparison, simple alternative, replay and Verification](../docs/audit/c-reports/CBUG-0061/README.md).
This stored C proposal is not source adoption or an engine/consumer correction.
