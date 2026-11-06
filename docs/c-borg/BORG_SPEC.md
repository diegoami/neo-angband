# The C Borg: purpose and architecture

**Status:** specification, not a proposal. It explains what the original
Angband C Borg *is for* and how it is put together, so that a rebuild — or a
reader trying to understand a defect — does not have to rediscover it.

**Source of truth:** the C Borg in
[`angband/angband`](https://github.com/angband/angband) at

```
e473720ed88a0f0c7ca66815cef3000eb4a38bc3
```

file `src/borg/` (59 `.c` files, 64,147 lines of C plus headers). That
commit is upstream's `master` head; see §0.

**This document does not list the Borg's bugs.** They are on this fork's
issue board, and their state changes there. What this document does is
explain **what the Borg is for and how it is built**, so that the bugs make
sense: a defect is only legible against the behaviour it disturbs. §11
describes the *shapes* of defect the architecture produces, with one
example each, so a rebuild can recognise them; the individual findings stay
on the board.

**Scope:** the C Borg only. This document says nothing about the Neo
TypeScript port (`neostryder/neo-angband-mod-borg`) or the Mandos derivative,
except where a behaviour is *shared* and therefore part of the contract a
rebuild must satisfy.

**Reading rule.** Where this document says "must", it means: the current C
Borg does this, and a rebuild that does not do this will play differently.
Where it says "known defect", it means the current C Borg does *not* do the
sane thing; the sane thing is stated alongside, and the choice is left open.

---

## 0. The upstream commit this describes

Every statement below is a statement about one specific commit of the
upstream C Borg:

| Field | Value |
| --- | --- |
| Repository | `angband/angband` |
| Branch | `master` |
| Commit | `e473720ed88a0f0c7ca66815cef3000eb4a38bc3` |
| Commit date | 2026-10-27 |
| Path read | `src/borg/` |
| Size read | 59 `.c` files, 64,147 lines (plus headers) |

That commit **is** upstream's `master` head, so `src/borg/` here is
byte-for-byte what upstream ships. It is also this session's recorded C
baseline (`session.json` → `baselines.c`) and the catalogue's C source pin
(`docs/c-bugs/catalogue.json` → `sources.c.commit`), and the local
`c-borg-reference` branch carries it unmodified:

```sh
gh api repos/angband/angband/commits/master --jq '.sha'
#   -> e473720ed88a0f0c7ca66815cef3000eb4a38bc3
git diff e473720ed origin/c-borg-reference -- src/borg   # empty
```

Line numbers cited throughout are at that commit. When upstream `master`
moves, the pin moves with it and the citations are re-read at the new head.

---

## 1. What the Borg is

### 1.0 What it is for

The Borg exists to **play Angband well without a human**. It is the
original autoplayer: the thing you leave running overnight to push a
character deeper than anyone would sit and watch, and the thing that
produced the game records of deep, successful, unattended runs.

That purpose shapes everything about it. An autoplayer that has to be
watched is not an autoplayer, so it must be able to play for hours without
supervision: it decides, it acts, it recovers from its own mistakes, and it
keeps going. It plays to win rather than to survive — it will spend a
potion on a fight it could walk away from if the potion is worth it, and
it will push toward Morgoth rather than settle for a comfortable depth.
And it does so through the only channel the game offers a player, which is
the keyboard.

The consequence is that the Borg is not a strategy and not a set of rules.
It is a **decision loop**: perceive, evaluate, act, and repeat. Its
"intelligence" is entirely in the middle step — a scoring function over
the possibilities it can perceive.

### 1.1 The mechanism

The Borg is an **autoplayer**. It reads the game's screen and stats, decides
what to do, and presses keys. It is not a rules engine: it never changes a
game rule, never reads hidden information the game has not shown it, and
never edits game state outside the ordinary keypress channel.

It is therefore a *screen-scraping* agent with three inputs:

1. the terminal screen (glyphs, colours, positions),
2. the game's own status line and character sheet,
3. the message log.

and one output: a queue of keystrokes.

Everything below follows from that design. Most of the Borg's bugs come from
the fact that its "memory" of the world is a **screen-derived map**, not the
game's own truth, and it must therefore re-derive everything it thinks it
knows every turn.

### 1.2 What it is not

* It is not a cheat engine. It has a *separate* RNG (`borg_rand_local`) so its
  decisions do not consume the game's RNG stream and change the game's rolls.
* It does not read the game's internal `cave[]`, `player->`, or `inv[]`
  structures for its decisions. It has `borg_cheat_*`-style helpers, but they
  are used for the *Borg's own* bookkeeping of what it saw, not as an oracle
  for hidden state.
* It does not run on a timer. It runs when the game asks for a key.

---

## 2. The control loop

### 2.1 Taking over the keyboard

The game installs an `inkey_hack` function pointer (`src/ui-input.c`). When
set, `inkey_ex()` calls it instead of waiting for a real keypress.

```
inkey_ex()
  └─ inkey_hack(flush_first)   →  borg_entry_point()
       └─ borg_save_keypress(generate_keypress(flush_first))
```

`borg_update_entrypoint(true)` installs the hook; `false` removes it.

### 2.2 One Borg turn

`generate_keypress()` (`src/borg/borg.c`) is the heart. In order:

1. `Term_locate()` — remember the cursor position.
2. `Term_fresh()` — force a screen refresh (the Borg normally never sees a
   stale screen).
3. `borg_notice_player()` — read the character sheet / status bar.
4. `check_for_deactivate_or_death()` — bail out if the Borg was stopped, the
   player died, the king was killed, a stop level was reached, or the money
   scum quota was met.
5. Flush the keypress buffer if asked.
6. `borg_get_messages()` — read the message log off the screen. **Some
   messages must be answered immediately** (a `-more-` prompt, a "Die? [y/n]"
   prompt); those return a keypress straight away.
7. `borg_parse(NULL)` — parse the *rest* of the messages into the Borg's own
   message structures. `borg.dont_react = false` afterwards.
8. `borg_inkey(true)` — if the Borg already has a queued key, use it.
9. `user_abort()` — if a real human pressed a key, stop the Borg.
10. If resting or repeating a command, return `EVT_NONE` (let the game run).
11. If respawning, finish the respawn.
12. `borg.targeting = false`.
13. Swap in the Borg's own RNG (`Rand_quick = true; Rand_value =
    borg_rand_local;`), call `borg_think()` in a loop until it returns true,
    then save `borg_rand_local` and restore the game's RNG state.
14. `borg_display_status()` — redraw the status line.
15. `borg_inkey(true)` — the key the Borg queued, if any.
16. If there is still no key, `borg_oops("normal abort")` and return ESCAPE.

**Invariant for a rebuild:** the Borg must produce **at least one keypress**
per turn, or the game stalls. A turn that ends with no key is an abort.

### 2.3 The decision function

`borg_think()` (`src/borg/borg-think.c`):

```
borg_think()
  ├─ borg_find_all_items()        -- re-read equipment + pack + stores
  ├─ borg_check_save()             -- autosave if configured
  ├─ borg_cheat_spells()           -- re-read spellbooks (if needed)
  ├─ if shapechanged: press 'v','r' (throw to revert form)
  ├─ borg_notice(true)             -- derive traits from items
  ├─ borg_update()                 -- re-read the map from the screen
  ├─ borg.power = borg_power()     -- score itself
  ├─ if in a store/home: return borg_think_store()
  ├─ track max clevel / max depth
  ├─ borg.time.now++               -- the Borg's clock
  ├─ borg.antibounce_count++       -- anti-loop counter
  └─ return borg_think_dungeon()   -- the actual decision
```

`borg_think()` is called in a loop (`while (!borg_think());`) because it may
return "nothing to do" (e.g. it needs to wait for a message to arrive).

### 2.4 The Borg's clock

`borg.time.now` is a `borg_time` = `int32_t`. It overflows at
`BORG_TIME_MAX - 100`; the Borg resets it to 1 and logs
`"*****WARNING***** Borg Clock Overflow"`.

**Known defect (CBUG-0246):** the reset is unreachable in practice — the
Borg aborts on the overflow first. See `borg-think.c` and the reset in
`borg_update()`.

### 2.5 The anti-bounce counter

`borg.antibounce_count` counts turns. It is incremented every
`borg_think()`, and it is bumped by **+10** whenever a note contains
`"Best Combo"` or `"Taking off "` (the Borg's own loop detection, in
`borg_note_internal`).

Uses:
* `borg.antibounce_count > 300` → refuse an emergency equipment swap
  ("Forbid if been sitting on level forever").
* `borg.antibounce_count <= 200` → regional fear from monsters is included
  in danger estimates. Above 200 the Borg stops adding that fear, to stop
  "bouncy" behaviour (running away from danger and back again).

**Invariant for a rebuild:** this is a *hidden state machine parameter* that
changes the Borg's decisions. Any rebuild must reproduce its increments
exactly or the Borg's behaviour will diverge.

---

## 3. The Borg's memory

### 3.1 Two kinds of memory

| Memory | Holds | Refreshed by |
| --- | --- | --- |
| **Screen-derived** | `borg_grids[]` — what the Borg *believes* about every visible grid | `borg_update()` every turn |
| **Cheat-derived** | `borg_items[]`, `borg.trait[]`, `borg.kills[]`, `borg_race_death[]` | `borg_notice()`, `borg_find_all_items()`, `borg_update()` |

The screen-derived map is the source of nearly all of the Borg's
misbehaviour: it is what the Borg *saw*, and it is stale, partial and
occasionally wrong.

### 3.2 The grid record

`borg_grid` (`src/borg/borg-cave.h`) holds, per dungeon grid:

| Field | Meaning |
| --- | --- |
| `feat` | the terrain feature the Borg believes is there |
| `info` | flags: `BORG_DARK`, `BORG_GLOW`, `BORG_LIGHT`, `BORG_ICKY`, `BORG_HURT`, `BORG_VIEW` … |
| `kill` | index into `borg.kills[]` if a monster is believed to be here |
| `trap` | the Borg believes there is a trap here |
| `glyph` | the glyph index of the remembered object |
| `xtra` | per-goal scratch space |

`borg_grids` is `borg_grid *borg_grids[AUTO_MAX_Y]`, one allocated row per
dungeon row, `AUTO_MAX_X` entries per row.

### 3.3 Item memory

`borg_items[]` is indexed by a **packed** index space:

| Range | Meaning | Constants |
| --- | --- | --- |
| `0 .. PACK_SLOTS-1` | the pack | `PACK_SLOTS = pack_size - quiver_slots` |
| `INVEN_WIELD .. INVEN_WIELD+11` | the equipment slots | `INVEN_WIELD = z_info->pack_size` |
| `QUIVER_START .. QUIVER_END-1` | the quiver | `QUIVER_START = INVEN_TOTAL = pack_size + 12` |

`safe_items[]` is a snapshot of `borg_items[]` taken by
`borg_find_all_items()`, used as the restore point for the Borg's
"try this item on" simulations.

**Known defect:** `borg_notice_equipment()` zeroes the wielded weapon's and
the bow's `ds/dd/to_d/to_h/weight/ident/sval` **in place**, in
`borg_items[]`, to model "attacking with bare hands". `safe_items[]` keeps
the real values, but `borg_items[]` stays wrong for the rest of the turn.
See §11.6.

### 3.4 The trait vector

`borg.trait[]` is a flat `int` array indexed by `BI_*` enumerators
(`src/borg/borg-trait.h`). It is the Borg's model of itself: current and max
HP, depth, level, resistances, flags (`BI_ISCUT`, `BI_ISPOISONED`,
`BI_ISWEAK`, `BI_ISHUNGRY`, `BI_ISPARALYZED`, `BI_ISCONFUSED`, `BI_ISBLIND`,
`BI_ISHALLU`, `BI_ISAFRAID`, …), `BI_CURHP`, `BI_MAXHP`, `BI_CDEPTH`,
`BI_MAXDEPTH`, `BI_CLEVEL`, `BI_MAXCLEVEL`, `BI_GOLD`, `BI_ACSW` (Cure Serious
Wounds count), `BI_ARESPOIS` (Resist Poison potion count), and so on.

The trait vector is **recomputed from scratch** by `borg_notice()` on every
turn, from the item list. It is not incremental.

### 3.5 The kill list

`borg.kills[]` is a list of monsters the Borg has seen, each with its race
index, grid position, and a "known" flag. `borg.kills_nxt` is the count.
`borg_race_death[]` records which races are known dead.

**Known defect (CBUG-0078):** `borg_update_kill_old()` writes
`borg_race_death[i]` with the *kill* index rather than the race index.

---

## 4. Perception: `borg_update()`

`borg_update()` (`src/borg/borg-update.c`, 3216 lines) re-derives the
Borg's picture of the level from the screen. It is the single most
important function for a rebuild to get right, and the single most
bug-prone.

### 4.1 What it reads

* the **map panel** — one glyph per grid, plus the colour/attribute
* the **message area**
* the **status line** (HP, depth, level, etc.)

### 4.2 What it writes

* `borg_grids[][]` — features, flags, kill indices, traps, glyphs
* `borg.kills[]` — the monsters seen
* `borg_race_death[]`
* `borg.time.*` — the "when did I last detect X" timers
* `borg.avoidance` — the danger level the Borg tries to avoid

### 4.3 The panel problem

The map panel is a *window* on the dungeon. `w_x`/`w_y` are the panel's top-left
dungeon coordinate. The Borg must map screen cells back to dungeon grids, and
must handle the panel scrolling under it.

`borg_need_shift_panel()` decides when to scroll the panel so the Borg can see
further. `borg.c` (`borg.c.x`, `borg.c.y`) is the Borg's *dungeon* position,
not its screen position.

**Invariant for a rebuild:** the Borg's map is anchored on **dungeon
coordinates**, and the panel is a viewport onto it. Any rebuild that works in
screen coordinates will break the moment the panel scrolls.

### 4.4 The glyph/monster bookkeeping

Monsters are tracked by *tracking* them: the Borg remembers a monster's race
and moves its remembered position forward when it sees the monster move.
`borg_locate_kill()` finds the slot for a monster description. When a monster
disappears, `borg_update_kill_old()` retires the slot.

**Known defect (CBUG-0086):** `borg_locate_kill()` uses
`my_strnicmp(who, "It", 2)`, so *any* monster whose name starts with "it"
(case-insensitively) is treated as invisible.

### 4.5 Detection timers

`borg.time.detect_traps`, `detect_doors`, `detect_walls`, `detect_evil`,
`detect_obj` record when each detection was last cast. The Borg re-casts
detections on a schedule rather than every turn, to save turns.

**Known defect (issue #219):** the object-detection state is not cleared on a
new level and carries over from the previous one.

---

## 5. Scoring itself: `borg_power()`

`borg_power()` (`src/borg/borg-power.c`, 1913 lines) computes a single large
integer describing how good the Borg's current kit is. It is the Borg's
utility function: **almost every decision is "which of these choices has the
highest power?"**

### 5.1 Shape

```
value = 0
value += <armour class>       * borg_worships_ac       bonus
value += <damage>             * borg_worships_damage   bonus
value += <speed>              * borg_worships_speed    bonus
value += <max HP>             * borg_worships_hp       bonus
value += <max mana>           * borg_worships_mana     bonus
value += <gold>               * borg_worships_gold     bonus
value += <ranged power>       * borg_worships_damage   bonus
value += <adjacent monsters>   * 1000
value += <status immunities>  * 10000
value += <useful potions>     * 25000 / 300 / 500 / …
value += <skill levels>        * 1000
value += <unique kills>       * 50000
value += <flags>              * …
```

### 5.2 The worship weights

The six `borg_worships_*` options in `borg.txt` multiply the corresponding
component. The defaults are all FALSE, so the base weights apply. The
worship weights are what make the Borg's *style* change; a rebuild must
reproduce them or the Borg will value different kit.

### 5.3 The formula section

With `borg_uses_dynamic_calcs` enabled, `borg.txt` may contain a
`[BEGIN FORMULA SECTION]` with a small expression language
(`src/borg/borg-formulas.c`, `borg-formulas-calc.c`) that adds to the power.
`borg_formula_error()` reports parse errors.

**Known defect (CBUG-0028):** three callers pass `format(...)` as the *error
argument* of `borg_formula_error()`.

**Known defect (CBUG-0029):** `parse_range()` misbehaves when
`borg_uses_dynamic_calcs` is on.

### 5.4 Power is computed from a *simulation* of wearing items

To value a candidate item, the Borg copies the item into `borg_items[]`,
re-runs `borg_notice()`, recomputes the power, and restores. That is why
`borg_simulate` exists (see §7.4) and why several bugs are of the form "the
simulation was not undone".

---

## 6. Flow: how the Borg decides *where* to go

The "flow" subsystem is the Borg's pathfinding. It is **not** A*: it is a
per-goal Dijkstra-like flood over the visible/known grid, with a per-goal
cost function.

### 6.1 The data structures

```c
struct borg_data { int16_t cost; uint8_t xtra; ... };
extern struct borg_data *borg_data_flow;  /* current flow's costs   */
extern struct borg_data *borg_data_cost;  /* current cost function  */
extern struct borg_data *borg_data_hard;  /* constant hard data     */
extern struct borg_data *borg_data_know;  /* "know" flags           */
extern struct borg_data *borg_data_icky;  /* "icky" flags           */
```

`borg_data_flow` holds the accumulated cost of the current flood. The flood
starts at the Borg's position and spreads outward; `borg_flow_commit()`
walks the flow downhill from the goal back to the Borg and produces the
next keypress.

### 6.2 The API

```c
void borg_flow_clear(void);
void borg_flow_enqueue_grid(int y, int x);
void borg_flow_spread(int depth, bool optimize, bool avoid,
                      bool tunneling, int danger, bool cave);
bool borg_flow_commit(const char *who, int why);
bool borg_flow_old(int why);
```

`borg_flow_spread()` is called with a `depth` budget: how far the flood may
travel. A small budget means "look for something nearby".

### 6.3 The goal types

`borg.goal.type` is one of a small set of `GOAL_*` defines
(`src/borg/borg-trait.h:32-41`):

| Goal | Value | Purpose | Implemented in |
| --- | ---: | --- | --- |
| `GOAL_KILL` | 1 | reach and fight a monster | `borg-flow-kill.c` |
| `GOAL_TAKE` | 2 | fetch a remembered object | `borg-flow-take.c` |
| `GOAL_MISC` | 3 | stores and other miscellany | `borg-flow-misc.c` |
| `GOAL_DARK` | 4 | explore unexplored territory | `borg-flow-dark.c` |
| `GOAL_XTRA` | 5 | searching for something specific | `borg-flow-misc.c` |
| `GOAL_BORE` | 6 | leaving the level | `borg-flow-misc.c` |
| `GOAL_FLEE` | 7 | fleeing the level | `borg-flow-misc.c` |
| `GOAL_VAULT` | 8 | excavating a vault | `borg-flow-misc.c` |
| `GOAL_RECOVER` | 9 | resting safely | `borg-flow-misc.c` |
| `GOAL_DIGGING` | 10 | digging an anti-summon corridor | `borg-flow-misc.c` |

The *flow builders* are a wider set than the goal types. `borg-flow-misc.c`
additionally provides `borg_flow_vein()`, `borg_flow_spastic()`,
`borg_flow_twitchy()`, `borg_flow_spread()`, `borg_flow_light()`,
`borg_flow_guide()`, `borg_flow_destroy()`, `borg_flow_ground()` and
`borg_flow_recover()`; `borg-flow-stairs.c` provides `borg_flow_stair_less()`
and `borg_flow_stair_more()`; `borg-flow-glyph.c` provides
`borg_flow_glyph()`.

### 6.4 The invariants that matter

* **The flow must be cleared before each goal.** Several known defects are
  "the flow was not cleared" (CBUG-0132, CBUG-0135, CBUG-0113).
* **The flow is relative to the current goal.** `borg_data_flow` is a single
  shared buffer; two goals cannot be interleaved.
* **`borg_flow_commit()` must end with exactly one keypress.** A commit that
  finds no path must return false without queueing anything.

### 6.5 Danger-aware flowing

Some goals ask "can I stand here?", which is `borg_danger()`
(§8.1) rather than a plain cost. `borg_flow_enqueue_grid()` computes the
danger of every grid it enqueues — even when the goal will ignore it, which
is wasted work (CBUG-0114).

**Known defect (issue #251):** "Bolt targeting takes a pass-wall monster's
danger at the wrong grid."

**Known defect (issue #252):** "Ball and arc blast scans skip the south and
east edges of the radius."

**Known defect (issue #253):** "Arc targeting's angle test uses the path end
for every grid (all-or-nothing)."

---

## 7. Fighting

`borg-fight-attack.c` (5322 lines) and `borg-fight-defend.c` (3605 lines)
are the two largest files in the Borg.

### 7.1 The fight cycle

```
borg_think_dungeon()
  └─ if a monster is adjacent or in sight:
       ├─ borg_defend()       -- decide to flee, fight, or use a defence
       └─ borg_attack()       -- choose and execute an attack
```

### 7.2 Attack selection

`borg_attack()` evaluates every attack type the character could use, scores
each, and picks the best. The attack types are enumerated `BF_*`
(`src/borg/borg-fight-attack.h`):

* melee (`BF_ATTACK`, `BF_WHIRLWIND`, …)
* thrown objects (`BF_THROW`)
* wands, staffs, rods (as bolts, or as beams/radii)
* bows and crossbows
* spells (`BF_*` per spell class: bolt, ball, arc, beam, …)
* activations (artifacts, rings, …)
* mimics, quivering (`BF_QUIVER`)
* *munchkin* mode (`borg-attack-munchkin.c`) — a special aggressive mode

The scoring function is `borg_attack_aux_*()`, one per attack kind. Each
returns a score; the highest score wins. Many of them work by *simulating*
the attack's damage against the current monster list.

### 7.3 The defence side

`borg_defend()` decides whether to keep fighting, flee, teleport, or use a
defensive item. It consults `borg_danger()`, the HP fraction, and whether
the monster is a unique.

**Known defect (issue #173):** `borg_defend()` leaves `borg_simulate` set when
it does nothing.

### 7.4 The simulation flag

`borg_simulate` is a global flag meaning "we are only scoring, do not act".
Most `borg_*_aux()` functions check it. Getting this wrong produces real
side effects during a scoring pass.

**Known defects:** CBUG-0103 (`borg_munchkin_mage()` sets `borg_attacking`
before its monster scan but returns false inside it); CBUG-0104 (two
functions set `borg_simulate = true` and never reset it when no attack scores
above 0).

---

## 8. Danger estimation

`borg_danger()` (`src/borg/borg-danger.c`) answers "how dangerous is grid
(y,x) right now?" on a 0..2000 scale.

```
p = 0
p += borg_fear_region[y/11][x/11] * c        /* regional fear    */
if (depth == 100 && p >= 300) p = 300         /* depth-100 cap    */
p += borg_fear_monsters[y][x] * c             /* local monsters   */
for each known monster i:
    p += borg_danger_one_kill(y, x, c, i, average, full_damage)
return min(p, 2000)
```

`borg_danger_one_kill()` walks the monster's blow table and sums the damage
each blow would do, applying resistances, and applies the `MONBLOW_*` fear
weights for non-damage effects (poison, confusion, paralysis, terrain
destruction, …).

**Known defect (CBUG-0053):** `borg_danger()` **overwrites** its
`full_damage` argument with `true`, so the partial-damage paths are only
reachable through `borg_danger_one_kill()` directly.

**Known defect (CBUG-0047):** the hit-chance formula
`150 - ((ac * 300) / 4 / 1 + (level + power) * 3)` is below 5 for any armour
class of 2 or more, so `full_damage == false` never yields partial damage.

**Known defect (CBUG-0046):** the HURT stun test `z > d_dice * d_side`
immediately follows `z = d_dice * d_side`, so it is never true.

---

## 9. Caution and escape

### 9.1 `borg_caution.c` — the reactive layer

`borg_caution_phase()` is a chain of "if the situation is X, do Y" checks
that run before the normal decision. It covers:

* fighting vs. fleeing decisions
* emergency equipment swaps (`borg_backup_swap()`)
* emergency teleportation
* stair usage under threat
* "back away" from a monster
* using a *scroll of teleportation* when cornered

### 9.2 `borg-escape.c` — the escape machinery

| Function | Purpose |
| --- | --- |
| `borg_escape()` | pick an escape route (stair, teleport, spell) |
| `borg_escape_stair()` | head for a staircase |
| `borg_recall()` | use a recall rod/activation/spell/scroll |
| `borg_surrounded()` | decide whether the Borg is about to be surrounded |
| `borg_escape_hall()` | escape along a corridor |
| `borg_caution_...` | the caution-phase helpers |

**Known defect (CBUG-0015):** `borg_escape()` fakes
`borg.trait[BI_CURSP] = borg.trait[BI_MAXSP]` and skips restoring it on some
early returns.

**Known defect (CBUG-0016):** a stair predicate queues a climb before its
phase guards finish.

### 9.3 Answering the game's prompts

Several game prompts want a yes/no answer, and the Borg has to answer them
without a human. The mechanism is the same everywhere: the Borg queues the
keypress and then queues `ESCAPE` to dismiss the prompt.

The prompt *"Set recall depth to current depth? [y/n]"* is the clearest
example, because the Borg's answer depends on a chain of five conditions
(§11.2 covers the failure mode this produces). The point for a rebuild is
the shape of the decision:

* the prompt only appears in the dungeon, when the current depth is below
  the maximum;
* the "right" answer depends on endgame state — how deep the Borg has
  been, how many uniques are still alive, whether it is in a special mode;
* the Borg's own state is split across `borg.trait[]` (current and maximum
  depth) and `borg.goal.*` (the mode flags), so the decision has to consult
  both.

A rebuild should treat "answer a prompt" as a first-class decision with its
own inputs, rather than folding it into whichever routine happened to
trigger the prompt. That keeps the state it consults visible.

---

## 10. Items

### 10.1 The pipeline

```
borg_find_all_items()      read equipment + pack + stores into borg_items[]
borg_notice(true)          derive borg.trait[] from borg_items[]
borg_item_analyze()        describe each item (borg_item_note)
borg_item_val()            score each item
borg_power()               score the Borg as a whole
```

### 10.2 `borg_item_val()` — the item score

`src/borg/borg-item-val.c`. Scores a candidate item for the "best stuff"
decision: base value, plus bonuses for armour class, damage, speed, flags,
and so on. The `sv_*` enumerators are resolved at startup by
`borg_init_item_val()`, which looks items up **by name** in the gamedata.

**Known defect (CBUG-0022):** two of those name lookups are wrong:
`sv_wand_confuse_monster` is looked up as `"Scare Monster"` (the same as
`sv_wand_fear_monster`), and `sv_wand_elec_ball` as `"Lightning Bolts"` (the
same as `sv_wand_elec_bolt`, where the intent was `"Lightning Balls"`). Both
succeed silently because the wrong name is a real item.

**Consequence of the second:** `BF_WAND_ELEC_BALL` looks the *bolt* wand up
in the pack and then launches it with a radius-2 / 64-damage **ball**
profile. The Borg never recognises a real Wand of Lightning Balls (it is
treated as unknown junk), and when it does fire, it scores the shot as a
64-damage blast while the game delivers a beam.

### 10.3 `borg_item_analyze()` — pseudo-identification

The Borg identifies items by *using* them and reading the message
("You feel a strange sensation..."). `borg_item_analyze()` holds the
per-item state machine.

### 10.4 Wearing, swapping, and the simulation

`borg-item-wear.c`. `borg_best_stuff()` picks the best item for each slot,
simulating each candidate. `borg_backup_swap()` swaps in a *backup* item
when the current gear is inadequate.

**Known defect (CBUG-0066):** the emergency-swap selector keeps the
*higher* danger score. (Upstream PR angband/angband#6809.)

**Known defect (CBUG-0067):** with an armour swap and no weapon swap, the
weapon danger stays 0, so an armour danger of 0 selects
`swap = weapon_swap - 1 = -1` and the key becomes
`all_letters_nohjkl[-1]`.

**Known defect (CBUG-0068):** the `i == -1` guards in the same function are
dead (the index is never negative there).

**Known defect (CBUG-0069):** `borg_one_step_wearing_best()` resets its
`skipping` counter on every pass of the loop.

**Known defect (CBUG-0070):** the "don't test the same item in both ring
slots" test reads `test[n + 1]` for `INVEN_LEFT`.

**Known defect (CBUG-0071):** `track_worn_time` is an `int16_t`, so
`borg.time.now` is truncated; past 32767 the "worn more than 10 turns ago"
test misbehaves.

### 10.5 `borg_calc_blows()` — blows per turn

`src/borg/borg-trait.c`. A copy of the engine's `calc_blows()`
(`src/player-calcs.c`):

```c
/* the Borg */
int weight = item->weight * item->iqty;

/* the engine */
int weight = (obj == NULL) ? 0 : object_weight_one(obj);
```

The difference is what "weight" means. The engine's `object_weight_one()`
is the **single item's** weight, because the question the calculation asks
is "how does this weapon feel in the hand" — the encumbrance of swinging
it. The Borg's version multiplies by the stack size, which is the weight
of *carrying* it.

Both meanings are correct for their own question, and the Borg uses the
carrying sense elsewhere (`borg_item_weight()`); the two must not be mixed
inside one calculation. This matters because the same function is called on
the wielded weapon (where `iqty == 1` and the two agree) and on **home**
items (where weapons are stored in stacks and they do not).

**What a rebuild should take from this:** when it reimplements an engine
calculation, it must copy the engine's *semantics*, not just its shape. The
Borg's copy of `calc_blows()` is otherwise line-for-line the engine's, which
makes a one-term difference easy to miss and easy to introduce.

---

## 11. Known defects that a rebuild should not inherit

The bugs live on this fork's issue board, and that is where they belong.
This section does not list them — a list here would go stale the moment a
finding is closed or a new one arrives.

What this section does instead is group the *shapes* of the defects that
the C Borg's architecture actually produces, and show one representative
example of each. The shapes are stable; the individual findings are not.
A rebuild that recognises the shapes will avoid the bugs whether or not it
has seen the specific findings.

Each shape below names the findings that exhibit it, so the board can be
consulted for the detail and for the evidence state.

### 11.1 Stale state between calls

The single most common defect class, and the one a rebuild is most likely
to reproduce: the Borg keeps a great deal of state in globals and expects
every function to leave it clean.

The pattern is always the same. A global flag is set to mean "we are only
scoring, do not act", a function returns early without clearing it, and the
next caller inherits a world in which scoring and acting are confused.
`borg_simulate` and `borg_attacking` are the two flags that do this most
damage, because so much of the Borg's decision logic branches on them.

The same shape appears with counters: a list's count is used to decide
whether an entry is live, but the count is not always consulted, or is
decremented without a check, and a stale entry is read as if it were
current.

**What to do differently:** a function that sets a global owes the caller a
restore, and the restore belongs on every exit path. A count that indexes
an array must be checked against the array's bounds at the point of use,
not assumed.

### 11.2 The wrong variable

The Borg has several parallel arrays that mean nearly the same thing:
`borg.trait[]` holds its model of itself, `borg.has[]` holds counts of
objects by kind, and `borg.amt_book[]` / `borg.book_idx[]` hold spellbook
state. Mixing them up is easy and usually compiles.

A representative instance: a health check written as
`borg.has[BI_CURHP] > 100` instead of `borg.trait[BI_CCURHP] > 100`. `has[]`
is indexed by *object kind*, so this reads a count of some item type
against a hit-point threshold — and it type-checks perfectly, because both
are `int`.

The other common form is a copy-paste between two near-identical blocks:
the second `borg_crush_spell` block clears `v1` again instead of `v2`, and
all five `OF_SUST_*` tests accumulate into `num_sustain_str`.

**What to do differently:** when two arrays hold different kinds of thing,
name them so the compiler helps, and check the index against the array's
documented meaning rather than its type.

### 11.3 Dead code that matters

A branch that can never be taken is usually harmless. It becomes harmful
when it is the branch that was *supposed* to fire — when the condition was
written one way and the code another, so a real case silently falls through
to a default.

`borg_spell_stat()` tests `&books[0].spells[0] != NULL`, which is always
true, so the check it was meant to make never happens. A "too afraid to
attack" test appears twice, the second unreachable. A random draw of
`randint1(100) < 1` is never true, so the 1% case never occurs.

**What to do differently:** a condition that looks unreachable is either a
typo or a missing case. Both are worth a test that would fail if the
condition were fixed.

### 11.4 The wrong comparison

The Borg's arithmetic is full of thresholds, and a comparison that can
never be true or never false makes the code above it dead.

The hit-chance formula
`150 - ((ac * 300) / 4 / 1 + (level + power) * 3)` is below its own clamp
of 5 for any armour class of 2 or more, so the "partial damage" path it
guards is unreachable in practice. The HURT stun test `z > d_dice * d_side`
immediately follows `z = d_dice * d_side`, so it is never true.

**What to do differently:** a threshold is worth checking at its extremes
— at the best and worst values the inputs can take — because that is where
the comparison quietly stops discriminating.

### 11.5 The wrong element of a parallel set

Several Borg structures are parallel arrays or switch statements over
`TV_*` kinds, and the wrong member of the set is easy to select by eye.

`borg_init_item_val()` resolves the wand kinds by *name*, and two of those
names are the wrong one — a confusion wand looked up as "Scare Monster",
and a lightning *ball* wand looked up as "Lightning Bolts". Both lookups
succeed, because both wrong names are real items; the bug is silent and
permanent.

`borg_light_beam()` tests the west neighbour from step 1 while north,
south and east start at step 0, so the west beam is a step short. A
"skip every non-floor/glyph" test is written `ag->feat != FEAT_FLOOR &&
ag->glyph`, which skips only *glyphed* non-floor grids rather than all of
them.

**What to do differently:** where a set of kinds is enumerated in more than
one place, derive the second from the first, and check that the lookup
names are the ones you meant.

### 11.6 Mutation during a read-only pass

`borg_notice()` is a *read* pass: it looks at the equipment and derives the
trait vector from it. But to model "attacking with bare hands" it zeroes
the wielded weapon's and the bow's `ds`, `dd`, `to_d`, `to_h`, `weight`,
`ident` and `sval` **in place**, in the live `borg_items[]` array.

`safe_items[]` holds the pre-corruption copy, so the simulation machinery
that restores it works. But everything else that reads `borg_items[]`
within the same turn now sees a falsified weapon: zero weight, no bonuses,
and `sval == 0` so it no longer matches the sling or the long bow.

**What to do differently:** a read pass must not write. If a temporary
model is needed, use the scratch copy that already exists.

### 11.7 Off-by-one and out-of-bounds

The Borg indexes several fixed-size arrays with values that come from the
game rather than from its own code: a slot number from `borg_slot()`, a
monster index from a list, a grid from a flow.

`borg_refuel_lantern()` starts its scan at `borg_slot(...)`, which is `-1`
when the lantern is not found. The Teleport Other skip list is read with
`ii <= count`, one entry past the end. A rod-recharge rest indexes
`borg_items[borg_slot(TV_ROD, sval)]` without checking for `-1`.

**What to do differently:** every index that came from a lookup that can
fail must be checked before it is used as an index.

### 11.8 Array capacity vs. screen size

`borg_flow_light()` (`src/borg/borg-flow-misc.c`) builds an array of every
glowing grid on the visible panel:

```c
for (y = w_y; y < w_y + SCREEN_HGT; y++)
    for (x = w_x; x < w_x + SCREEN_WID; x++)
        if (ag->info & BORG_GLOW) {
            borg_glow_y[borg_glow_n] = y;
            borg_glow_x[borg_glow_n] = x;
            borg_glow_n++;
        }
```

`borg_glow_x[]`/`borg_glow_y[]` have `AUTO_LIGHT_MAX` (1536) entries and there
is **no bound check**. The map panel size is
`SCREEN_HGT x SCREEN_WID`, derived from the terminal size and the sidebar
mode:

| Terminal | Sidebar mode | Map cells | vs. 1536 |
| --- | --- | ---: | --- |
| 80x24 | 0 (sidebar) | 22 x 66 = 1452 | fits |
| 80x24 | 1 | 20 x 79 = 1580 | **overflow** |
| 80x24 | 2 | 23 x 79 = 1817 | **overflow** |
| 80x28 | any | 1716 .. 2133 | **overflow** |
| 120x40 | any | 4028 .. 4641 | **overflow** |

So the write past the array is reachable in an ordinary 80x24 terminal with
the sidebar off, and in any larger terminal.

**What the overflow actually corrupts.** The two arrays are laid out
**contiguously** in `.bss` — from the built binary's symbol table,
`borg_glow_y` at `0x27fbc0..0x2801bf` and `borg_glow_x` at `0x2801c0..`
— and the loop writes them **interleaved**, one `y` and one `x` per
glowing grid. So:

* `borg_glow_y[n]` for `n >= 1536` lands in `borg_glow_x[n - 1536]`
* `borg_glow_x[n]` for `n >= 1536` lands in `borg_light_y[n - 1536]`

The tail of each array ends up holding the *other* array's coordinates. The
read-back loop then reads them back as `(y, x)` pairs, so from index 1536
onward it enqueues **swapped** coordinates — 278 of the 1817 enqueued grids
hold the wrong coordinates, and **212 of those are off the map panel
entirely** (`y >= 23` or `x >= 79` for an 80x24 panel with the sidebar off).
Those are fed into the flow's cost field and spread, so the Borg would be
pathing towards coordinates that name no grid.

**What to do differently:** size this array from the *terminal*, or bound
it explicitly.

### 11.9 Double counting

`borg_surrounded()` (`src/borg/borg-escape.c`) counts the non-safe adjacent
grids:

```c
if (!borg_cave_floor_grid(ag))   non_safe_grids++;
else if (ag->feat == FEAT_NONE)  non_safe_grids++;
else if (ag->kill)               non_safe_grids++;
else if (feat_is_shop(ag->feat)) non_safe_grids++;
/* Mega-Hack -- skip traps XXX XXX XXX */
if (ag->trap && !ag->glyph)      non_safe_grids++;   /* NOT an else if */
```

The trap test is a separate `if`, not the fifth `else if` of the chain (its
comment style matches the store branch exactly). A grid that is both
occupied and trapped, or both a wall and trapped, is counted **twice**. With
eight neighbours the count can reach 16 against a starting `safe_grids` of 8,
so `safe_grids` goes negative and `monsters > safe_grids` is true with **no
monsters nearby**. The Borg then believes it is surrounded and flees.

**What to do differently:** a chain of mutually exclusive tests must be
written as one chain; a bare `if` next to an `else if` chain is a separate
test, and whether that is intended is not obvious from the code.

### 11.10 Wrong mode flag

`borg_think_stair_scum()` logs *"Leaving Scumming Mode. (Town or Weak)"* and
then clears **`borg.lunal_mode`** — the *lunal* mode flag — not the scumming
mode. There is no scumming-mode flag; the state that scumming sets
(`borg.goal.fleeing_lunal`) is left dangling, so `borg.lunal_mode` and
`borg.goal.fleeing_lunal` can disagree about which mode the Borg is in. The
stair-scum call site (`borg-think-dungeon.c:1343`) runs *before* the lunal
engagement block (`borg-think-dungeon.c:1438-1456`), so the two can collide.

**What to do differently:** the state that *enters* a mode and the state
that *leaves* it should be one structure, set and cleared together.

### 11.11 Two scans sharing one best-score

`borg_test_stuff()` scores the Borg's identify priority with three
**assignments** to the same variable, so each overwrites the last:

```c
/* Preferentially ID egos and artifacts */
if (item->art_idx)
    v = item->value + 150000L;

if (borg_ego_has_random_power(&e_info[item->ego_idx])) {
    v = item->value + 100000L;      /* smaller: overwrites the artifact bonus */
}

/* Prioritize the ID */
if (borg_item_note_needs_id(item))
    v = item->value + 20000L;        /* smaller still */
```

An artifact that is also a random-power ego therefore scores as a plain
ego. The function scans the items **twice** and the second scan re-scores
artifacts at the artifact bonus, which masks the under-scoring for that
item but leaves the first scan's score in the shared `b_v`, where
`if (v <= b_v) continue;` can block a legitimate second-scan candidate.

**What to do differently:** a score built from independent contributions
should accumulate, not overwrite; and a scan that keeps the best candidate
should not share its running best with another scan.

### 11.12 A scan that keeps the last match, with no preference

`borg_wear_recharge()` scans the pack and the equipment for something to
recharge:

```c
    for (i = 0; i < INVEN_TOTAL; i++) {
        ...
        /* Where can it be worn? */
        slot = borg_wield_slot(item);
        ...
        /* note this one */
        b_i    = i;
        b_slot = slot;
    }
```

There is no early exit and no preference, so the **last** qualifying item
wins. And nothing rejects an item whose wield slot is `-1` — `borg_wield_slot()`
has no case for `TV_WAND` or `TV_STAFF`, so a pack wand or staff qualifies.
If such an item is last, `b_slot == -1`, the function falls through to
`return false`, and the Borg neither rests nor recharges the worn item it
was looking for.

**What to do differently:** a search should stop at the first acceptable
candidate, or prefer the best one explicitly; and a candidate that cannot
be used should be rejected when it is found, not discovered afterwards.

---

## 12. The Borg in town

`borg-store.c`, `borg-store-buy.c`, `borg-store-sell.c`, `borg-think-store.c`,
`borg-home-notice.c`, `borg-home-power.c`.

The Borg shops by **simulating**: it copies a candidate ware into its pack,
re-runs `borg_notice()`, recomputes the power, and compares. The "home" is
the Borg's own storage: it buys items there to keep for later and sells
things it no longer wants.

**Known defects:** CBUG-0007 (home trial early exits leave reduced
quantities/charges); CBUG-0008 (ring-side scores persist across home
wares); CBUG-0009 (home ring trials use asymmetric quantities); CBUG-0010
(the home grab skip flag leaks to later wares); CBUG-0013 (the latest
purchase/sale is outside the history reader range); CBUG-0014 (ten-entry
transaction arrays never use the tenth slot); CBUG-0016 (the urgent shop
goal is overwritten or paired with a stale ware); CBUG-0115, CBUG-0116, and
CBUG-0117, CBUG-0118 (home sell bookkeeping).

---

## 13. Configuration (`borg.txt`)

Parsed by `borg_init_txt_file()` (`src/borg/borg-init.c`). 36 settings, listed
in `enum { BORG_VERBOSE, ... BORG_MAX_SETTINGS }` in `src/borg/borg.h`, with
the same order as the `borg_settings[]` table in `borg-init.c`.

| Setting | Type | Default | Effect |
| --- | --- | --- | --- |
| `borg_verbose` | bool | FALSE | verbose logging |
| `borg_munchkin_start` | bool | FALSE | start in munchkin (speedrun) mode |
| `borg_munchkin_level` | int | 12 | the level munchkin mode starts at |
| `borg_munchkin_depth` | int | 16 | the depth munchkin mode starts at |
| `borg_worships_damage` | bool | FALSE | weight damage higher in the power score |
| `borg_worships_speed` | bool | FALSE | weight speed higher |
| `borg_worships_hp` | bool | FALSE | weight max HP higher |
| `borg_worships_mana` | bool | FALSE | weight max mana higher |
| `borg_worships_ac` | bool | FALSE | weight armour class higher |
| `borg_worships_gold` | bool | FALSE | weight gold higher |
| `borg_plays_risky` | bool | FALSE | dive without the usual unique/level gates |
| `borg_kills_uniques` | bool | FALSE | wait for uniques to die before diving |
| `borg_uses_swaps` | bool | **TRUE** | use the backup-equipment swap machinery |
| `borg_uses_dynamic_calcs` | bool | FALSE | enable the formula section |
| `borg_stop_dlevel` | int | 128 | stop at this depth |
| `borg_stop_clevel` | int | 51 | stop at this character level |
| `borg_no_deeper` | int | 127 | do not dive past this depth |
| `borg_stop_king` | bool | TRUE | stop after killing the king |
| `borg_cheat_death` | bool | FALSE | cheat death on |
| `borg_respawn_winners` | bool | FALSE | respawn after winning |
| `borg_respawn_class` | int | -1 | respawn as this class (-1 = keep) |
| `borg_respawn_race` | int | -1 | respawn as this race (-1 = keep) |
| `borg_chest_fail_tolerance` | int | 7 | chest-opening failures to tolerate |
| `borg_delay_factor` | int | 0 | extra delay per Borg action |
| `borg_money_scum_amount` | int | 0 | the money-scum quota (0 = off) |
| `borg_self_scum` | bool | TRUE | use the self-scum (lunal) mode |
| `borg_lunal_mode` | bool | FALSE | use lunal mode |
| `borg_self_lunal` | bool | FALSE | enter lunal mode by itself |
| `borg_enchant_limit` | int | 12 | how much to enchant |
| `borg_dump_level` | int | 1 | dump the map at this character level on death |
| `borg_save_death` | int | 1 | save on death |
| `borg_stop_on_bell` | bool | FALSE | stop when the bell rings |
| `borg_allow_strange_opts` | bool | FALSE | allow unusual game options |
| `borg_autosave` | bool | FALSE | autosave periodically |
| `borg_restore_ignore_settings` | bool | FALSE | restore the saved ignore settings |

**Note:** `borg_uses_swaps` defaults to **TRUE**, and `borg_stop_king`,
`borg_self_scum` also default to TRUE. A rebuild that changes a default
changes the Borg's behaviour from the first turn.

**Known defects:** CBUG-0125, CBUG-0126, CBUG-0127, CBUG-0128, CBUG-0129
(the configuration parser and the ignore-reset path).

---

## 14. Rebooting the Borg

`borg-reincarnate.c`. The Borg can respawn after death (cheat death,
`borg_respawn_winners`, `borg_respawn_class`/`_race`). The restart must
reset every piece of accumulated state.

**Known defect (CBUG-0036):** `borg_reincarnate_start()` does not fully reset
the state on respawn.

---

## 15. Logging and diagnostics

* `borg_note()` — a decision note, added to the game's message log.
* `borg_warning()` — louder.
* `borg_log_death()`, `borg_log_death_data()` — write to
  `ANGBAND_DIR_ARCHIVE/borg.dat`.
* `borg_write_map()` — dump the map (the `^z` command family).

**Known defect (CBUG-0001):** cancelling the map-dump filename prompt
dereferences a NULL file and crashes the game.

**Known defect (issue #287):** multibyte screen messages can acquire stale
text and phantom monsters.

**Known defect (CBUG-0146, #164):** the debug power output passes an `int32_t`
to a `long` format.

**Known defect (CBUG-0149, #165):** the debug flag/info text is passed as a
format string although it contains no percent signs.

**Known defect (CBUG-0150, #166):** the Borg's debug menus are allocated
without being freed.

---

## 16. What a rebuild must preserve

If a rebuild is to "be" the C Borg, these are the observable behaviours that
must be preserved. They are listed as invariants because a rebuild that
changes them will play differently, and "differently" is the thing to avoid.

1. **One keypress per turn.** A turn that produces no key is an abort and
   stalls the game.
2. **The Borg's own RNG.** The Borg's decisions must not consume the game's
   RNG stream, or the game's rolls change.
3. **Screen-only perception.** The rebuild may read the game's status line and
   messages, but must not use the game's internal state as an oracle for
   hidden information. (Exception: the `borg_cheat_*` helpers, which read the
   Borg's *own* recorded view.)
4. **Dungeon-coordinate map.** The map is anchored in dungeon coordinates and
   the panel is a viewport.
5. **Everything is re-derived each turn.** The trait vector, the map and the
   kill list are recomputed, not patched.
6. **Power is the utility.** Almost every choice is "maximise the power
   score", with the danger estimate as the other main term.
7. **Simulations must be undone.** Every "try this item on" / "try this spell"
   simulation must restore every piece of state it touched.
8. **The anti-bounce counter's increments are part of the behaviour.**
9. **The Borg's clock overflows at `BORG_TIME_MAX - 100`.**
10. **The six worship weights are the style dial.** A rebuild without them
    plays a different Borg.

---

## 17. What a rebuild should *not* preserve

The Borg's defects are on this fork's issue board, not here, so this is not
a list of them. It is the set of **principles** a rebuild should apply when
it finds itself reproducing one.

1. **Do not preserve a defect that contradicts the code's own stated
   intent.** Where a comment says one thing and the code does another, the
   comment is the specification. Fix it, and say so.
2. **Do not preserve an unbounded write.** If a buffer is filled from a
   quantity the environment controls — the terminal size, the map size, the
   stack size — then the bound must be derived from the same source.
3. **Do not preserve a count that can go negative or be double-counted.** A
   counter whose failure mode is a wrong sign is a wrong decision, not a
   cosmetic slip.
4. **Do not preserve a mode flag that is set by one routine and cleared by
   another.** Mode state must be owned in one place, and the exit path must
   clear the whole of it.
5. **Do not preserve a copy of a game function that has drifted from it.**
   When the Borg reimplements an engine calculation, the two must agree;
   where they cannot, the reason must be written down.
6. **Do not preserve a simulation that mutates the state it is simulating.**
   A "try this on" pass must leave the world exactly as it found it.
7. **Do not preserve a counter that leaks between the branches that use
   it.** Local state is local; shared state needs an explicit owner.

Each of these corresponds to a family of defects the current Borg has, and
the issue board has the specifics with their evidence.

---

## 18. Companion documents

| Document | What it holds |
| --- | --- |
| the issue board (`upstream-c` + `finding` labels) | the canonical, live state of every C Borg finding |
| `docs/c-bugs/catalogue.json` | the machine-readable finding catalogue |
| `docs/c-bugs/findings/CBUG-NNNN.json` | one JSON record per finding |
| `docs/c-bugs/watch-index.json` | the data-only watch index |
| `patches/upstream-c/` | stored proposal patches |

The board is where the bugs are, and it is the only place their state
changes. This document is the *behavioural* reference: what the Borg is,
how it is built, and what a rebuild has to preserve. Neither is a substitute
for the other, and neither is duplicated here.

---
