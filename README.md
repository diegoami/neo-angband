# Native C Borg and Angband repair worktree

This is the native C side of diegoami/neo-angband-tracker's combined repair session.
The same fork also hosts the Neo engine source on master; C and Neo use
different bases/builds. Start with AGENTS.md, RULES.md, PLAN.md and this fork's
checkpoint. Catalogue/patches/trackers are owned by the fork's master; access
that tree in the parent engine workspace, not the Mandos product session.

Use a separate C build directory and original compilation instructions
under docs/hacking. Run native tests plus legal before/after/control scenarios.
No Neo pnpm commands here; Node is only for portable session metadata checks.
Our docs do not authorize official C submissions. Source/legal files from
Angband remain unchanged; this branch adds repair-session bootstrap docs.

## Native build entry

```sh
cmake -S . -B build/session -DSUPPORT_BORG=ON -DSUPPORT_TEST_FRONTEND=ON
cmake --build build/session --parallel 2
ctest --test-dir build/session --output-on-failure
```

These are the original C CMake options, not Rust harness flags or Neo package
commands. Add the issue's legal game scenario and before/after controls to the
unit/build checks. Coordinate builds with the shared validation lock.
