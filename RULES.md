# Working rules

1. Work only in this session's repository and scope. Read AGENTS.md, this
   file, PLAN.md, session.json and the latest checkpoint issue before acting.
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
   TS derivative; borg-rs independently translates/adopts them in Rust. Neither
   translation is a port-fidelity defect. Mark Mandos departures explicitly.
5. No angband/angband issues, PRs or comments from us under the standing pause.
   Keep C proposals in diegoami repositories with reproducible native legal
   scenarios, controls, player consequence and patch links. Confirmed Neo
   engine/port reports may use their correct neostryder repository after
   reproduction, provenance and duplicate checks. Local proposals may be
   closed/unmerged with c-proposal/postponed while retaining branches/patches.
6. Owned default branches contain session docs. Preserve those commits and
   integrate upstream by normal merge when needed; do not reset them to a
   pristine mirror. Remote upstream references are refreshed fast-forward.
   Start actual fix/proposal branches from the correct pristine source base,
   excluding our session docs. C and Neo have different source trees/bases.
   No amend or force-push; preserve every published topic head and all tags.
7. Keep proposal source and comments suitable for upstream review. Do not add
   FORK markers, internal catalogue IDs or tracking labels to source comments.
   Explain the functional change plainly in commits; keep internal provenance
   on the issue and in supporting documentation. Preserve upstream credit,
   licensing, AI_USAGE_POLICY and original technical docs. No AI trailers.
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

14. Owner intake policy (2026-11-06): borg-rs posts C leads directly as issues
    on diegoami/neo-angband-tracker. Review and deduplicate those board reports;
    do not routinely fetch/watch the borg-rs repository, bug file or issue feed.
    Retain historical source attribution and CBUG IDs. A direct report remains
    a lead until independently validated against native C. Revisit the Rust
    repository only for an explicit owner request or a specific investigation
    that requires its historical evidence.

15. For every defect investigated or fixed by this session, include a self-contained
    "Verification" section in BOTH the canonical issue and fix PR. Write
    it for a skeptical maintainer who has not read the conversation. State the
    observable problem, intended behavior and exact source/build pins; give
    copyable commands the reviewer can run, the actual relevant output, and explain
    what each result proves and does not prove. For claims such as an unused
    field or wrong variable, show a scoped exhaustive symbol search, classify
    declarations/reads/writes and inspect the decisive call-site context. Do not
    substitute an assertion, test name, green CI summary or evidence link for
    this explanation. Provide a reproducible before/failure and after/success
    sequence with unchanged conditions, negative controls and their outcomes;
    expose synthetic interventions, reachability assumptions and practical
    limits. Link immutable full logs/fixtures/patches as supporting detail and
    explain how to inspect or rerun them. Verbosity is welcome where
    it makes the reasoning checkable. Update this account when evidence or the
    patch changes. Passing automation is not owner acceptance: keep readiness
    for the owner's external filing explicitly subject to their assessment.

16. Make every source issue actionable from its opening screen: explain in everyday
    language what the Borg/player does, what goes wrong, and what remains working.
    State the observed harmful outcome and why its priority is justified; separate
    possible consequences from measured ones. Prioritize reproducible crashes,
    stuck behavior, wasted resources and bad survival/combat decisions over code
    anomalies or internal-state differences with no demonstrated practical harm.
    A clear coding mistake alone does not justify high priority. Reassess imported
    severity after investigation and park further low-impact work when appropriate.
    For every proposal prepared for owner filing, put the direct upstream compare/
    Create pull request link, a copyable plain-language title and description, and
    one direct immutable link to the evidence explanation near the TOP of the
    canonical issue. Keep detailed commands/history below the concise explanation
    (collapsible where useful). Do not make the owner navigate internal PRs,
    checkpoints or raw logs to find the filing material. This handoff is for the
    owner; it does not authorize this agent to post upstream under a standing pause.

    Owner-accepted presentation examples are
    https://github.com/diegoami/neo-angband-tracker/pull/226 and
    https://github.com/diegoami/neo-angband-tracker/issues/82.
    Match their practical explanation, three-command reproduction, generated
    offline replay and explicit click-through instructions. Put the observable
    before/after result and unchanged controls before code-level receipts.
    Explain the strength and limits of the measured consequence plainly; an
    extra cast/mana cost must not be presented as measured combat harm.

17. Every bug report and PR must explain the problem to a maintainer who has
    not read our conversation or investigated this code. Before function names,
    variables, test counts or build details, explain the situation the Borg
    encounters, what it actually does wrong, the consequence measured, and
    what it does after the fix. Use concrete actions and ordinary language:
    "The Borg chooses haste but casts regeneration first, spending an extra
    20 mana. After the fix, it casts only haste."

    Do not lead with "preparation scoring", "simulation side effects",
    "native fixture" or "command energy". Explain necessary technical terms
    when introducing them. Then provide a section headed "Verification":
    copyable pinned commands, relevant actual output, and why that output
    supports the claim. Describe unaffected controls, unchanged before/after
    conditions and experiment limits. Passing tests and linked logs alone
    are insufficient. Keep the full technical evidence below the explanation;
    do not label ordinary explanations "Human".

    Use CBUG-0066 (PR226/issue82) as a worked example of reasoning, not merely
    a reproduction recipe: demonstrate the bad choice, show its consequence,
    compare unchanged conditions and explain why the correction works. Test
    plausible simpler alternatives when relevant. Before posting, read the
    opening without the technical sections. Could a new maintainer explain
    what goes wrong, why it matters and what changes? If not, rewrite it
    yourself. The owner must not have to request a translation. Do not inflate
    severity or claim combat harm, extra enemy attacks or deaths that the
    experiment did not demonstrate.

18. Every posted native C fix follows the evidence-package standard in
    [docs/C-FIX-POSTING.md](docs/C-FIX-POSTING.md): one source branch from
    pristine c-borg-reference plus one evidence branch from owned master
    whose docs/audit/c-reports/<ID>/ directory carries both the generators
    (scenario.c, run-cases.py, make_replay.py, reproduce.py, borg.txt) and
    the captured artifacts of the verified run (replay.html with
    replay-data.json, README.md guide, receipt.json hashes, every phase log
    including asan variants and gates). The verification matrix is
    fail-before/pass-after, byte-identical unchanged controls, full native
    gates on both variants, an asan variant for memory-safety defects, and a
    headless render check of the exact committed replay with no console
    errors. Reproduce commands pin an immutable fixtures sha; appended
    artifacts move the head past the pin and the README states the
    relationship. Real game messages and production Borg choices only; no
    hand-fed text, manufactured counts or waived checks.
