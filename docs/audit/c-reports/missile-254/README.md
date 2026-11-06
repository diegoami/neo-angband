# The Borg can try to fire with no target chosen

A Warrior with a bow and arrows knows a kobold it cannot shoot from
where it stands. The Borg's shot-lining movement flow asks whether a
distance attack exists; that probe reaches the missile launch routine
with the target list empty (the Teleport Other defense resets it, and
the launch routine itself does not refill it) and with the simulation
flag off (any acted attack or defense leaves it off; it also starts
off).

**No missile finds a target, so the chosen-missile index stays -1 — but
the routine's "nothing to use" guard tests the value, which never goes
negative once initialized to zero. The real call falls through to the
firing path: it reads the description of missile slot -1 (outside the
heap-allocated inventory array) and queues a fire key, a quiver
character computed from index -1, and the target key.** Under
AddressSanitizer the original aborts with a heap-buffer-overflow READ
at the note line (borg-fight-attack.c:2148); the corrected guard
returns "nothing to fire" and queues nothing.

Measured transition: original queues 3 keys (return 0); corrected
queues 0 keys (return 0). Two controls stay byte-identical before and
after: with a real target both versions queue the genuine firing keys
and value the shot at 4, and in simulation both value it and queue
nothing.

Consequence: three junk keys enter the Borg's input stream (the game
would see a fire command with an invalid quiver slot), and every probe
of this kind reads outside the inventory array. No death or survival
claim is made.

Canonical report: [issue254](https://github.com/diegoami/neo-angband-tracker/issues/254).
Lead supplied by borg-rs (fuzz cross-check); independently validated
and reproduced natively here, including the ordinary-play reachability
through the shot-lining flow (borg_think_dungeon → borg_flow_kill_aim →
borg_has_distance_attack → borg_attack_aux_launch).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/missile-254-evidence
git show EVSHA:docs/audit/c-reports/missile-254/reproduce.py > /tmp/reproduce-missile-254.py
python3 /tmp/reproduce-missile-254.py --fixtures-ref EVSHA --full-tests
```

The runner builds two normal variants (original, corrected) and two
AddressSanitizer variants from pristine C `e473720`, verifies the
source diff is exactly the chosen-index guard, runs three recorded
situations on each, checks the fail-before/pass-after matrix and
byte-identical controls, records both full gates and writes an offline
`replay.html`. Your checkout and character/profile are not used or
edited.

Open [replay.html](replay.html) (download raw and open locally). Choose
**Shot-lining flow asks about firing with no target**, then **The
probe** and **Keys queued**. The original queues three keys computed
from missile index -1; the correction queues nothing. The other two
situations are controls whose logs match byte for byte.

Actual recorded output, original (`before-no-target.log`):

```text
DECISION no-target return=0 keys=3 sequence=f5
OUTCOME return=0 keys=3
```

The middle queued key is the non-printing quiver character computed
from index -1. Corrected (`after-no-target.log`):

```text
DECISION no-target return=0 keys=0 sequence=
OUTCOME return=0 keys=0
```

ASAN original (`asan-before-no-target.log`): `heap-buffer-overflow READ`,
`borg_attack_aux_launch` at `borg-fight-attack.c:2148`, through
`my_strcpy` while formatting the note for missile slot -1. Corrected
asan: 1/1 passed, no sanitizer report. Scenario runs disable the strcmp
interceptor and leak check because the pristine base otherwise reports
an unrelated strcmp overread during init name sorting; the tracked
defect is a compiler-instrumented read and needs no interceptors.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`405e4fd4880e7bef27d6a6328765a904638c8257`. One guard in
`src/borg/borg-fight-attack.c`. The quiver scan keeps `b_k` at -1
whenever every candidate's `borg_launch_bolt()` returns -1, and the
existing guard `if (b_n < 0)` can never be true because `b_n` starts at
zero and candidates only raise it. The correction guards on the chosen
index: `if (b_k < 0) return 0;`.

Reachability: `borg_think_dungeon`'s movement flow calls
`borg_flow_kill_aim(true)`, whose probe `borg_has_distance_attack()`
calls `borg_attack_aux_launch()` directly. The target list is not
refilled there (the attack chooser normally fills it), and the
simulation flag is whatever the previous evaluation left — false after
any acted attack or defense, and false by initial value. borg-rs found
the read by fuzzing; this reproduction shows the ordinary-play path.

### Conditions and limits

Native Human Warrior level 10, depth 10, seed 1000, lit constructed
40x40 arena, short bow, twenty arrows, torch, one awake kobold five
grids away and known. The production launch routine is called
unchanged; the empty target list and the off simulation flag reproduce
the state earlier evaluations leave behind. This isolates the launch
guard, not full autonomous play. The junk keys are shown queued, not
executed against the game. No death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs including the asan
variants. [Full receipt](receipt.json).
