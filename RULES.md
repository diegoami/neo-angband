# Working rules

1. Work only in this session's repository and scope. Read AGENTS.md, this
   file, PLAN.md, session.json and the lane checkpoint before acting: Neo engine
   uses #25; the independently owned native C session uses #207 (session:c).
   The umbrella is a router, not a shared repository or shared task scope.
2. Current bug state and upstream-board/source review comes first; then
   triage/evidence and list maintenance, then fixes. Do not turn a reported
   lead into a verified defect. A closure without an upstream fix claim is not
   a correction receipt; apply rule 9 to an upstream claimed fix.
   Priority applies within this session's component; the independent C/engine
   stream does not wait for the port-repair queue to become empty.
3. One canonical defect issue in its owner repository. Link other repository
   work as a distinct integration/adoption task; do not copy live defect state.
   Preserve transferred issue history and old redirects; qualify cross-repo
   references as owner/repo#number or full URLs, never bare numbers.
4. Name the origin: C Borg, C game, Neo engine, Neo port or Mandos-only.
   Compare pinned current C and imported4.2.6 when attributing port behavior.
   A faithful copy of a C defect belongs to the engine/C repository; the Neo
   port follows C. Do not use the mod tracker as a C-correction translation
   service. Mandos independently translates applicable C fixes into its own
   TS derivative. Any Rust translation/adoption remains external to this
   workspace; we manage no Rust worker or adoption task. A consumer
   translation is not a port-fidelity defect. Mark Mandos departures explicitly.
5. No angband/angband issues, PRs or comments from us under the standing pause.
   Keep C proposals in diegoami repositories with reproducible native legal
   scenarios, controls, human consequence and patch links. Confirmed Neo
   engine/port reports may use their correct neostryder repository after
   reproduction, provenance and duplicate checks. Local proposals may be
   closed/unmerged with c-proposal/postponed while retaining branches/patches.
6. Owned default branches contain session docs. Preserve those commits and
   integrate upstream by normal merge when needed; do not reset them to a
   pristine mirror. Remote upstream references are refreshed fast-forward.
   Start actual fix/proposal branches from the correct pristine source base,
   excluding our session docs. C and Neo have different source trees/bases.
   No amend or force-push; preserve every published topic head and all tags.
7. Mark intentional code divergence beside its source reference with FORK
   and explain what/why in the commit. Preserve upstream credit, licensing,
   AI_USAGE_POLICY and original technical docs. No AI attribution trailers.
8. Relevant fail-before/pass-after and controls precede a fix claim. Use the
   correct C or TypeScript build environment; state pins/options/RNG/perception
   and practical limits. Do not change assertions/seeds or waive relevant
   skips to get green. Baseline failures require attribution and tracking.
9. Source issues use open/closed for upstream correction and our fork import.
   Trust upstream's explicit fixed resolution or accepted correction; an OPEN
   proposal's "Fixes #N" text or our own fork merge is not upstream resolution.
   Use merged-upstream only temporarily while that correction awaits import
   into the appropriate fork source tree. After import and normal checks,
   close the original source issue and remove the temporary label. Do not keep
   it open for hypothetical residual faults, extra independent reproof or a
   package release; a real recurrence found after adoption gets a NEW linked
   bug. Closed cases need no permanent merged label.
   Do not put carried/fixed/partial/not-carried or patch-active consumer tags
   on source issues. Link a distinct Mandos adoption issue where applicable.
   On Mandos adoption issues, carried means fixed on our side but still open
   until the correction is fixed upstream AND adopted downstream with our
   changes, rebuilt and verified. Then fixed/closed; partial scope stays open.
   patch-active belongs only on consumer backport maintenance, independently
   of closure. Proposals or stored patches alone are not applied corrections.
10. Preserve owner saves, browser profiles, install data and the running lab.
    A source merge is not a deployment. Read-only references stay pristine;
    work in task worktrees. Run records/evidence already committed are append-only.
11. Coordinate heavy local checks across all three sessions with the shared
    lock: flock /tmp/angband-validation.lock <command>. Log long runs, watch
    progress and keep one heavy local gate at a time. Do not launch background
    agents without owner direction. Reviews inspect; they do not mutate code.
12. Record decisions/receipts and the next exact action on the owner issue and
    checkpoint at each merge or stop. Update source monitors, patch ledgers and
    affected links in the same pass. Never silently drop a superseded hunk.
13. Current work is bugs, triage, workflow maintenance and owner-requested
    rework. Backlogged gameplay strategies/learning/unattended stage-resume
    features stay parked unless the owner explicitly changes that scope.
14. Before pushing an upstream refresh, audit every new or changed Actions
    workflow and trigger. Inherited product jobs are FORK-guarded to their
    original repository; run node tools/check-fork-workflows.mjs. The owned
    source tracker blocks v* tag creation/update/deletion by ruleset; do not
    bypass it or import product tags. CI remains active. Mandos intentionally
    retains its own product-release workflows under its separate session.
15. borg-rs reports directly to diegoami/neo-angband-tracker. Triage and
    deduplicate those posted reports normally; verify C-versus-Rust attribution
    and evidence before making a source-defect claim. Do not proactively poll
    or fetch its repository, docs/c-bugs.md or external issue board, or create
    another managed reference clone. Existing snapshots/watch implementation
    remain legacy/manual evidence only; they are not a startup/end-of-session
    obligation. Rust source work and managed Rust adoption tasks are out of scope.
    This prohibition includes indirect monitor follows. Keep Rust provenance
    in Source/discovery fields, not watched Threads status references; verify
    monitor scope before running inherited tooling (see docs/BORG-RS-INTAKE.md).
