# Contributing to this repair fork

Start with AGENTS.md, RULES.md and PLAN.md. File work on this fork’s owner
issue, reproduce/attribute first, then use an isolated topic branch from the
correct pristine upstream base. Session docs stay on the owned workbench.
No amend/force-push or official C submission. Link separate Mandos adoption
work instead of duplicating defect status. Preserve upstream credit/licences.

## Neo engine toolchain

Use Node26 as pinned by .nvmrc and pnpm `12.3.4`, the exact packageManager
version in package.json. After selecting the Node line, install/use that
pnpm version and run pnpm install --frozen-lockfile. Source build/typecheck,
relevant regressions and the repository CI remain required for engine fixes.
Native C work uses its separate compiler/CMake toolchain instead.
