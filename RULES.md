# Working rules

1. Work only in this session's repository and scope. Read AGENTS.md, this
   file, PLAN.md, session.json and the latest checkpoint issue before acting.
   The umbrella is a router, not a shared repository or shared task scope.
2. Current bug state and upstream-board/source review comes first; then
   triage/evidence and list maintenance, then fixes. Do not turn a reported
   lead into a verified defect. A closed upstream thread is not proof of a fix.
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
   TS derivative; borg-rs independently translates/adopts them in Rust. Neither
   translation is a port-fidelity defect. Mark Mandos departures explicitly.
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
9. Source issues here use open/closed for source/upstream correction state.
   Do not put carried/fixed/partial/not-carried or patch-active consumer tags
   on source issues. Link a distinct Mandos adoption issue where applicable.
   There carried means fixed on our side but still open until the correction
   is fixed upstream AND adopted downstream with our changes, rebuilt and
   verified. Then fixed/closed; partial scope stays open. patch-active belongs
   only on consumer backport maintenance, independently of closure. Proposals
   or stored patches alone are not applied corrections. No release wait alone.
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

14. Owner intake policy (2026-11-06): borg-rs posts C leads directly as issues
    on diegoami/neo-angband-tracker. Review and deduplicate those board reports;
    do not routinely fetch/watch the borg-rs repository, bug file or issue feed.
    Retain historical source attribution and CBUG IDs. A direct report remains
    a lead until independently validated against native C. Revisit the Rust
    repository only for an explicit owner request or a specific investigation
    that requires its historical evidence.

15. For every defect investigated or fixed by this session, include a self-contained
    "Human verification" section in BOTH the canonical issue and fix PR. Write
    it for a skeptical maintainer who has not read the conversation. State the
    observable problem, intended behavior and exact source/build pins; give
    copyable commands a human can run, the actual relevant output, and explain
    what each result proves and does not prove. For claims such as an unused
    field or wrong variable, show a scoped exhaustive symbol search, classify
    declarations/reads/writes and inspect the decisive call-site context. Do not
    substitute an assertion, test name, green CI summary or evidence link for
    this explanation. Provide a reproducible before/failure and after/success
    sequence with unchanged conditions, negative controls and their outcomes;
    expose synthetic interventions, reachability assumptions and practical
    limits. Link immutable full logs/fixtures/patches as supporting detail and
    explain how the human can inspect or rerun them. Verbosity is welcome where
    it makes the reasoning checkable. Update this account when evidence or the
    patch changes. Passing automation is not human acceptance: keep readiness
    for the owner's external filing explicitly subject to their assessment.
