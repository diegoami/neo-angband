# Source tracker automation containment

This is a repair/proposal tracker, not a product publisher. Normal CI remains active.

- All 11 jobs in release.yml, publish-npm.yml, pages.yml, mod-canary.yml and edge.yml have FORK-marked repository guards requiring `neostryder/neo-angband`. This includes jobs using always(), so failed/skipped dependencies cannot release from our fork. Tag, release, schedule and manual events on our default branch skip those jobs.
- `node tools/check-fork-workflows.mjs` checks every job in those files and runs in fork-session-docs CI. Audit any new inherited workflow before pushing a source refresh.
- [Active version-tag ruleset 26648434](https://github.com/diegoami/neo-angband-tracker/rules/26648434) blocks creation, update and deletion of refs/tags/v*, with no bypass actors. This also prevents importing an old unguarded version tag. Existing tags are preserved. No tag push is used as a safety test.
- Settings also disable edge and the native-C create-github-release workflow (release.yaml). The latter is distinct from Neo release.yml; settings alone did not contain unregistered files before review R1.
- Pages is workflow-only, retaining the gh-pages branch/site. A same-content deployment occurred at 2026-11-05T17:59Z; the earlier statement that no deployment occurred was incorrect.
- The unrequested v1.23.0-edge.1 release remains draft, its original target and four assets preserved. The additional tag v1.23.0-edge.209 points at 836b8da2a553d9a5e567d5c56476623d501a7bec. Its origin is unknown; it is retained without movement/deletion.
- Mandos intentionally retains its separate active GitHub-release and Nexus product workflows. The fidelity port tracker keeps its release/Nexus/issue-redirect workflows disabled. These are separate repositories and settings.

For the independent review and correction record, see [Mandos migration records](https://github.com/diegoami/mandos/tree/main/docs/migrations/reorganization-review). Review R3 tracks the post-merge Electron smoke failure in [issue 171](https://github.com/diegoami/neo-angband-tracker/issues/171); a passing identical PR tree did not make the failed master run green.
