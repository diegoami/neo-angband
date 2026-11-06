# Worker lanes across three repositories

| Session | Repository | Root under the Angband umbrella | Owns |
| --- | --- | --- | --- |
| Neo engine fixes | [diegoami/neo-angband-tracker](https://github.com/diegoami/neo-angband-tracker) | neo-angband-tracker/ | Neo engine; checkpoint #25 |
| Native C fixes | same repository | neo-angband-tracker/.worktrees/angband-c/ | C Borg/C game; dedicated checkpoint #207 |
| Port fixes | [diegoami/neo-angband-borg-mod-tracker](https://github.com/diegoami/neo-angband-borg-mod-tracker) | neo-angband-borg-mod-tracker/ | Upstream TypeScript Borg port defects and isolated Neo proposals |
| Mandos development | [diegoami/mandos](https://github.com/diegoami/mandos) | better-borg/ | Mandos-only behavior, harness/lab, integration and verified adoption |

Each session reads its own AGENTS/RULES/PLAN/checkpoint. It does not inherit
another repository's queue or start fixing a different component silently.
The old neo-angband lab checkout and pristine neo-angband-upstream / original
neo-angband-mod-borg checkouts are retained references, not these workspaces.
Do not change the live lab or its mandos-lab-data while bootstrapping.

One GitHub Angband fork network is available: our neo-angband fork therefore
hosts Neo engine master and the separately based C workbench/reference topic
branches. A C proposal must never be based on Neo TypeScript master. The
native C worktree is .worktrees/angband-c inside the engine session root;
it is an independently owned worker lane in the same repository. Its checkpoint
is #207; Neo engine checkpoint #25 remains separate.

Upstream reports stay on angband/angband or the appropriate neostryder repo
only under the standing posting rules. Our fork issues are our canonical
trackers; transferred Mandos URLs remain historical redirects. See the
migration ledger and each checkpoint for exact current issue numbers.

## C correction flow and independent Rust peer

The engine/C tracker proves/proposes C fixes and retrieves C leads from
borg-rs through direct source-attributed issues on this tracker. Routine
fetching/watching of its repository, bug file and issue feed is retired. Mandos independently
translates applicable C corrections into TS and carries/adopts them. The
fidelity-only Neo Borg mod tracker does not perform that translation service.
borg-rs independently translates/adopts C corrections in Rust, without a Neo
responsibility; its read-only reference is not a fourth managed Neo session.
