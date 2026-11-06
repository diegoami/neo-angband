# Neo CI change scope

Owner maintenance task: diegoami/neo-angband-tracker#223, coordinated through
diegoami/angband-workspace#9. This policy changes validation scheduling, not
source ownership, release permissions or consumer verification requirements.

CI runs once per pull-request update and on master pushes; manual dispatch
always runs the full gate. Session metadata checks run on PRs and master or
workbench/c-borg pushes, with no arbitrary topic-branch push duplication.
Superseded PR runs cancel; master push runs remain observable. Issue/comment
events never trigger either workflow. The separate native C proposal workflow
and its branch/source contracts are unchanged.

The scope job compares the tested PR merge tree with its verified first
parent, checking both event-supplied parent SHAs. On a master push it compares
exact before/after SHAs. It never infers a diff from HEAD~1 for a topic branch.
Missing history, initial pushes, malformed events/diffs, invalid paths or an
empty diff select full CI. A scope-job failure also cannot suppress the full
test/live-game jobs.

The small lane accepts only reviewed regular non-executable additions or
modifications: named session docs, review Markdown, named source catalogue
records, selected textual run/evidence receipts and stored proposal patches.
The exact allowlist lives in tools/ci-scope.mjs. This is not an extension-wide
Markdown exemption or a docs-directory exemption. Deletes, renames, symlinks,
executable files and unknown paths select full CI.

README.md, CONTRIBUTING.md and docs/INSTALL.md additionally run the existing
product contracts in desktop packaging.test.ts and core toolchain.test.ts.
Inspection found README/INSTALL macOS/download contracts and CONTRIBUTING's
package-manager contract there. Technical/modding/releasing docs, changelog,
package docs, fixtures and source-driven documentation remain full: their
contracts also include npm-publish, version-sync, release-notes, SDK doc sync,
modding-status, hooks, tutorial and other runtime-facing suites. New contracts
must update the scope mapping before their input files can use a small lane.
The focused Vitest configuration selects the real tests, without the root
global setup that generates unrelated tile assets.

Every PR still gets session/bootstrap, catalogue/source coverage, privacy,
classifier controls and publisher-guard checks. Full scope retains the entire
existing build/lint/bundle/test/parity/package/MCP and live-game jobs, before
merge and again on the actual master push. Workflow, classifier, test,
dependency, tool, configuration, runtime and unknown-path changes always
select full. This CI-maintenance PR therefore runs its own full hosted gate.
No publisher workflow, tag ruleset or package publication is changed.

C evidence with a runnable observer, driver or configuration is deliberately
not classified as inert. It still selects the conservative full gate; the
Neo gameplay suite is not claimed as proof of native C behavior. A future
separate, validated C-evidence gate could optimise that case. Native C source
proposals continue through their independently owned native workflow; this
change neither validates their scenarios nor replaces their human receipts.
No Rust source/feed is read by classification or its fixture-only tests.

For a local control run use node --test tools/ci-scope.node-test.mjs. The tests
cover pure records, actual doc contracts, code/lock/test/workflow/unknown and
runnable evidence paths, mixed diffs, symlinks/renames, exact push/PR ranges,
real Git merge trees and uncertain/missing-diff fallback. Hosted workflow
checks remain authoritative; a metadata-only local check does not waive the
full gate when scope is full.
