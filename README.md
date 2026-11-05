# Native C Borg and Angband repair worktree

This is the dedicated native C lane of diegoami/neo-angband-tracker.
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
cmake --build build/session --target alltests --parallel 2
```

This project defines alltests/allunittests custom targets (not a CTest suite).
These are the original C CMake options, not Rust harness flags or Neo package
commands. Add the issue's legal game scenario and before/after controls to the
unit/build checks. Coordinate builds with the shared validation lock.

Local bootstrap installed CMake3.31.10 as a user tool via uv; GCC/Make/pkg-config
were already present. On another host install a C compiler, Make and CMake
first. No privileged system package or owner lab/data change is required.

Native C checkpoint: https://github.com/diegoami/neo-angband-tracker/issues/207.
borg-rs supplies leads directly on our issue board; see RULES.md for intake.
