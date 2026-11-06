# Native C workspace of the combined engine/C session

This is the C worktree of diegoami/neo-angband-tracker, inside the same
repository with an owner-authorized dedicated native C worker lane. Read RULES.md, PLAN.md, session.json and the latest
checkpoint on diegoami/neo-angband-tracker. The engine workspace is ../..; keep the engine worker separate and do not use the Neo engine build here.

This tree contains original C src/borg and the C game. Use CMake/Make and
native C scenarios. It is NOT Neo TypeScript source. C fixes are isolated
from angband/master at the recorded pin, excluding bootstrap documents.
workbench/c-borg carries our session entry docs; c-borg-reference remains
an unchanged reference. Never post to angband/angband under the owner pause.

The canonical C catalogue and watch scripts are on this fork's master in
the engine workspace. Check shared issue state first and preserve the
running lab and old reference checkouts. See docs/SESSION.md for routing.

Native C checkpoint: https://github.com/diegoami/neo-angband-tracker/issues/207
The owner authorizes this dedicated C lane alongside the Neo engine worker;
older combined-session/no-fourth-session wording above is superseded for this
purpose. Engine checkpoint #25 retains its independent next actions.

borg-rs now posts C leads directly on this fork's issue board. Do not run
routine Rust repository/file/issue-feed watches. Preserve attribution and
CBUG IDs; independently validate direct reports before calling them defects.
The canonical C issue board and historical evidence remain in this repository.

Every bug fix must include self-contained verification in its canonical
issue and PR: copyable pinned commands, actual results, the reasoning they
support, before/after controls and limits. Follow the verification rule
in RULES.md; linked logs or passing automation alone are insufficient.

For a fresh cloud checkout, follow the native C setup and pinned reproduction
in docs/SESSION.md. Read checkpoint207 for current work before starting a task;
no workstation /tmp directory or engine checkout is required.

Owner-accepted report format: follow PR226 and issue82. Lead with what the
player sees and the measured consequence, provide three copyable commands
that build the comparison and generate an offline browser replay, explain
what to click and what changes, and keep source pins/raw logs below the
readable walkthrough. Do not substitute technical receipts for that format.
Apply RULES17 before posting every report or PR: situation, wrong action,
measured consequence and corrected action come before technical terms.
CBUG-0066 is the worked reasoning example, not only a replay recipe. Read
the opening alone and rewrite it until a maintainer new to the code can
explain the problem and change. Then provide "Verification" with pinned
commands, relevant actual output, controls and limits; never label the
ordinary explanation "Human" or imply unmeasured combat harm.
