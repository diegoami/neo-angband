# How the C Borg findings were established

This is the provenance for the seven C Borg findings filed as
[#310](https://github.com/diegoami/neo-angband-tracker/issues/310) through
[#316](https://github.com/diegoami/neo-angband-tracker/issues/316).

It lives here, next to the check programs, because it describes how the
*findings* were produced. The behavioural specification of the Borg itself
is [`docs/c-borg/BORG_SPEC.md`](../../../../c-borg/BORG_SPEC.md) and does
not carry this.

## Source

`angband/angband` at `e473720ed88a0f0c7ca66815cef3000eb4a38bc3` (upstream's
`master` head when checked on 2026-11-06), `src/borg/`, 59 `.c` files and
64,147 lines. Read file by file. The local `c-borg-reference` branch carries the same commit
and is unmodified (`git diff e473720ed origin/c-borg-reference -- src/borg`
is empty).

## The native C game and the headless harness

**Re-check note (2026-11-06 UTC).** `SUPPORT_BORG_HARNESS` is not an option of
the upstream tree at the pin (`git grep SUPPORT_BORG_HARNESS
origin/c-borg-reference -- CMakeLists.txt` finds nothing), and no branch of
this repository contains it (`git grep` over every local and remote branch
finds nothing). The harness source was therefore not available to the
re-check, and the runtime statements in this section were not re-verified.
They are kept as the original author's record. The source-level checks in
this directory do not depend on them.

The C game was built from that upstream commit with `SUPPORT_BORG=ON`, and
driven by the **headless deterministic Borg harness**
(`SUPPORT_BORG_HARNESS=ON`): a front end that supplies a terminal which
draws nothing and never sleeps, scripts the birth screens, sends `^Z z` to
start the Borg, logs every key the Borg returns, and stops on a game-turn
cap. It uses a fixed seed and a per-run `borg.txt`.

The harness is an *observer*: it does not change the game's rules, the RNG,
the player, the map or the Borg's decisions. The only additions this session
made to it were a **message observer** (every message the game adds is
written to a log — the Borg's decision notes go through `message_add()`) and
a terminal-geometry option.

Runs used: seed 1000, Human Warrior, 2,000 / 20,000 / 60,000 game turns.
Each run produced a keypress log, a message log and a JSON summary.

The message log is the useful artefact. It is the Borg's own account of what
it did, in order:

```
1	Initializing the Borg... (borg.txt)
1	# Installing keypress hook
1	# Standing on dn-stairs
1	# Flowing toward Home at cost 10
51	# Creating a monster 'singing, happy drunk' at (6,12), HP: 4, Index: 11
91	# Entering a '7' shop
91	# Selling 'Scroll of Word of Recall' to the home
201	# Performing attack type 1 with value 8.
201	# Facing village idiot at (9,26) who has 10 Hit Points.
201	# Attacking with weapon 'Dagger (1d4) (+0,+0)'
201	You hit the village idiot.
201	You have slain the village idiot.
```

## Standing the source checks apart from the runs

Source-level claims about a branch, a counter or a formula are checked with
a small standalone C program that transcribes the relevant code **verbatim**
from the upstream tree and runs it over synthetic inputs. The transcription
is checked against the source line by line, and the program's tables and
constants are taken from the pinned tree, not from memory. This is a
**source-level** check, not a game run, and the findings say so.

Those programs are in this directory; see the [README](README.md) for what
each one shows and how to run them, and [RECHECK.md](RECHECK.md) for the
re-check of the specification and of these programs.

## What is claimed, and what is not

* **Claimed:** what the Borg does, verified against the source; the main
  loop, the message trace and the recall path, verified in a running game.
* **Not claimed:** the *frequency* of any behaviour, long-run outcomes, the
  Borg's win rate, or that any specific branch is reachable in ordinary
  play. When these findings were filed, each one that had not been
  reproduced in a game said so and was labelled `evidence:source-checked`.
  Labels change on the board; this file does not track them (for example,
  #311 has since been reproduced natively — see the board).

## Running the harness without disturbing anything else

The harness writes to five directories: `user`, `save`, `archive`, `panic`
and `scores`. The game's default user directory is `~/.angband/Angband`,
which is **shared** with any other session on the machine. Every run
therefore overrides all five with `-d`:

```sh
BIN=/path/to/angband
OUT=/tmp/some/isolated/dir
mkdir -p "$OUT"/{user,save,archive,panic,scores}
cd "$(dirname "$BIN")"          # the game finds ./lib relative to the cwd
TZ=UTC LANG=C.UTF-8 LC_ALL=C.UTF-8 "$BIN" -mborgh -n -uBorg \
    -duser="$OUT/user" -dsave="$OUT/save" -darchive="$OUT/archive" \
    -dpanic="$OUT/panic" -dscores="$OUT/scores" -- \
    -seed 1000 -race 0 -class 0 -turns 60000 \
    -keylog "$OUT/keys.log" -msglog "$OUT/msgs.log" \
    -summary "$OUT/summary.json" -screen
```

The game must be run from the directory that holds `lib/` (the build's
`game/` directory), or a copy of it.
