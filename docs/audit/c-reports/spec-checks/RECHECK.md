# Re-check of the C Borg specification and its check programs

This records how [`docs/c-borg/BORG_SPEC.md`](../../../c-borg/BORG_SPEC.md)
and the programs in this directory were re-checked after the review of
[PR #317](https://github.com/diegoami/neo-angband-tracker/pull/317#issuecomment-6697427239),
so that the next review can repeat the checks instead of re-deriving them.

* **Pin:** `angband/angband` `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`.
  `gh api repos/angband/angband/commits/master --jq .sha` returned that
  commit on 2026-11-06 (UTC); `git rev-parse origin/c-borg-reference`
  returns it too. All source reading used an export of
  `origin/c-borg-reference` (`git archive origin/c-borg-reference src lib`),
  **not** `reference/` (the 4.2.6 tree).
* **Nothing about any finding was changed.** No issue was opened, closed,
  relabelled or edited. Where the re-check bears on a finding, it is listed
  under [Observations for the owner](#observations-for-the-owner-findings-not-changed)
  and left there.

## 1. Mechanical checks on the specification

Both scripts are in [`recheck/`](recheck/). `$PIN` is a directory holding
the pinned `src/` and `lib/`.

```sh
git archive origin/c-borg-reference src lib | tar -x -C "$PIN"
SPEC_REPO=. python3 -I docs/audit/c-reports/spec-checks/recheck/check_spec.py "$PIN" docs/c-borg/BORG_SPEC.md
python3 -I docs/audit/c-reports/spec-checks/recheck/check_transcriptions.py "$PIN" docs/c-borg/BORG_SPEC.md
```

| Check | What it does | Result |
| --- | --- | --- |
| identifiers (`check_spec.py`) | every identifier in backticks and in code blocks must occur as a whole word in the pinned **code** (`$PIN/src` with comments removed; string literals kept, because option names live there) or in `$PIN/lib`. Allow-listed English/C words are skipped, **except** when used as a member, call or subscript. In `pseudocode` blocks: tokens with `_`, members, attached calls/subscripts (`name(`), and assignment targets | 1870 tokens, **0 misses** |
| `borg.` members (`check_spec.py`) | in every `borg.a…`, `a` must be declared in `struct borg_struct` (`borg.h`) | **0 misses** |
| file names (`check_spec.py`) | a bare `x.c`/`x.h`/`x.txt` must exist in the pinned tree; a repository file must be written with its path (`docs/…/x.c`) | **0 misses** |
| citations (`check_spec.py`) | every `file:function:N-M` must name a function defined in that file with `N..M` inside its body (for headers: the named identifier lies in `N..M`) | 122 citations, **0 misses** |
| transcriptions (`check_transcriptions.py`) | **every** ```` ```c ```` block must have a "Transcribed from `file:function:a-b`" line (any indentation) within the three lines above it; its lines must occur in order in the cited range after removing comments and whitespace; source lines skipped **between** two transcribed lines must be marked with `...` | 12 blocks, **0 problems** |

**What these checks do not do.** They establish that names exist, that a
citation points inside the function it names, and that a transcription is
faithful. They do **not** establish that the prose next to a citation
describes that code correctly: "`borg_flow_commit()` queues a movement key
(`borg-flow.c:borg_flow_commit:585-608`)" passes, and is false. Semantic
accuracy rests on reading and review (§2, §6). They also do not check
leading or trailing lines a transcription omits from its cited range.

`recheck/adversarial_fixture.txt` holds the second review's six false
fixtures. Results with the current scripts:

| Fixture | Caught? |
| --- | --- |
| W1 plain pseudocode identifier (`fabricatedstrategy = 123`) | yes (assignment target) |
| W2 allow-listed word as a member (`borg.master`) | yes (`BORG-FIELD`) |
| W3 repository file presented as pinned source (`teststuff_test.c`) | yes (`FILE`) |
| W4 real citation next to a false sentence | **no** — semantic, not mechanical |
| W5 indented "Transcribed from" header | yes (header found; the faked body is an unmarked gap) |
| W5b ```` ```c ```` block with no header | yes (`NO-HEADER`) |
| W6 omission without a `...` marker | yes (`UNMARKED-GAP`) |

```sh
SPEC_REPO=. python3 -I docs/audit/c-reports/spec-checks/recheck/check_spec.py "$PIN" docs/audit/c-reports/spec-checks/recheck/adversarial_fixture.txt
python3 -I docs/audit/c-reports/spec-checks/recheck/check_transcriptions.py "$PIN" docs/audit/c-reports/spec-checks/recheck/adversarial_fixture.txt
# both exit 1, listing the caught fixtures
```

The hardened transcription check found one real unmarked omission in the
specification (a field skipped in the `borg_kill` excerpt), now marked.

Run against the **original** draft (`35cd8b929`), `check_spec.py` reports
among others (and 6 `BORG-FIELD` misses such as `borg.kills`): `borg_item_val`,
`borg_escape_hall`, `borg_need_shift_panel`, `borg_flow_twitchy`,
`borg_flow_guide`, `borg_flow_destroy`, `borg_flow_ground`, `BF_ATTACK`,
`BF_THROW`, `BF_WHIRLWIND`, `BF_QUIVER`, `BI_ISHALLU`, `BI_CCURHP`,
`BORG_ICKY`, `BORG_HURT`, `kills_nxt` (from `borg.kills_nxt`), and two the
review did not list: `borg_item_note` (old §10.1) and the file name
`borg_caution.c` (old §9.1 heading).

Pin citations are written `file:function:lines`. `tools/citation-check.mjs`
resolves `file.c:N` citations against `reference/` (the 4.2.6 tree, which
has same-named `borg-*.c` files), so a plain `borg-flow.c:585` would be
checked against the wrong source; the `file:function:lines` form is not
parsed by that tool and is checked by `check_spec.py` instead.

The call graphs (§2.8, §6.3, §7.1) were derived with greps of this shape:

```sh
grep -n 'borg_attack(\|borg_defend(\|borg_caution(\|borg_flow_kill(' "$PIN"/src/borg/*.c
grep -n 'borg_flow_commit(' "$PIN"/src/borg/*.c        # goal -> committer table
grep -n 'borg_flow_spread(' "$PIN"/src/borg/*.c        # parameter usage
```

## 2. Claims changed, with their source and check

"Old" is the draft at `35cd8b929`. Line ranges are at the pin; all are
inside the named function.

| § | Old claim | Now | Source at the pin | Checked by |
| --- | --- | --- | --- | --- |
| 1.1, 1.2, 4.1, 16 | screen-only agent; "does not read `cave[]`, `player->`, or `inv[]`" | two channels: screen (message line, store header) **and** direct reads of gear, status, spells, `map_info()` on the panel, cave predicates, monster HP/timers, `r_info[]` | `borg-item.c:borg_find_all_items:117-131`, `borg-inventory.c:borg_equipment:172-209`, `borg-inventory.c:borg_inventory:214-250`, `borg-update.c:borg_update_map:279-353`, `borg-flow-kill.c:borg_update_kill_old:243-330`; `src/cave-map.c` `map_info()` reads `player->cave` | `grep -c 'player->' src/borg/*.c` (186), `grep -n 'square_\w*(cave\|map_info(' src/borg/*.c` |
| 1.1 | "never edits game state outside the keypress channel" | sets options, delay, `cheat_live`, `NOSCORE_BORG`; `do_cmd_wiz_cure_all()` on "Die?" with `borg_cheat_death` | `borg-init.c:borg_reinit_options:403-444`, `borg-messages-react.c:borg_react_prompted:105-139`, `borg-reincarnate.c` | `grep -n 'option_set(\|do_cmd_wiz\|noscore' src/borg/*.c` |
| 1.2 | separate RNG, unqualified | covers `borg_think()` only; prompt handling before the swap uses the game RNG | `borg.c:generate_keypress:355-374`; `borg_reincarnate_start()` `randint0` called from `borg_react_prompted()` | read |
| 2.2 | 16 steps, `in_shop` and `borg_step` missing, status one step late | 15 steps in source order, incl. `in_shop` reset, `EVT_NONE`, `borg_step` | `borg.c:generate_keypress:268-394` | read |
| 2.2, 16 | "at least one keypress per turn" | thinks only when the queue is empty; empty think = abort; `EVT_NONE` pass-through | `borg.c:generate_keypress:334-337` | read |
| 2.4 | "the reset in `borg_update()`", "CBUG-0246" | reset in `borg_think()`; unreachable because of the abort at `BORG_TIME_MAX - 3000`; no finding id | `borg-think.c:borg_think:242-245`, `borg-think-dungeon.c:borg_think_dungeon:1282-1288` | read |
| 2.5 | counter = turns, +10 on two notes; `<= 200` gates regional fear | many increments, +100 on messages, resets on progress; `<= 200` gates `borg_fear_monsters` (per grid); "Best Combo" never emitted | `borg-io.c:borg_note_internal:165-170`, `borg-danger.c:borg_danger:2846-2847` | `grep -n antibounce_count src/borg/*.c`; `grep -n 'Best Combo' src/borg/*.c` (only the test) |
| 2.6, 2.7, 2.8 | absent | key queue/ESCAPE discipline; rest pass-through; the full decision ladder | `borg-io.c:borg_keypress:311-336`, `borg-think-dungeon.c:borg_think_dungeon:1256-2302` | read |
| 3.1 | `borg.kills[]`; kill list "recomputed" | global `borg_kills[]`; patched and retired; cleared on level change | `borg-flow-kill.h:borg_kill:38-68`, `borg-update.c:borg_update:2232-2233` | `grep -c 'borg\.kills' src/borg/*.c` (0) |
| 3.2 | `glyph` = glyph index; no `take`/`web`/`store`; `BORG_ICKY`, `BORG_HURT` | struct transcribed; `glyph` is a bool (and never set for a glyph of warding); real flags | `borg-cave.h:borg_grid:74-86`, `borg-cave-view.h:BORG_MARK:67-75`, `borg-update.c:borg_update_map:330-353`, `lib/gamedata/trap.txt` | transcription check; `grep -n 'glyph *= ' src/borg/*.c` |
| 3.4 | `BI_ISHALLU` | `BI_ISIMAGE`; `borg.has[]` is by kind | `borg-trait.h` | identifier check |
| 4.3 | `borg_need_shift_panel()` | `borg_defend_panel_shift()` + `borg.need_shift_panel` | `borg-fight-defend.c:borg_defend_panel_shift:2819-2952` | identifier check |
| 4.6 | absent | prompt answers, parse, reaction queue, `borg_dont_react` | `borg-messages-react.c:borg_react_prompted:95-324`, `borg-messages-react.c:borg_react:41-90` | read |
| 5.1 | additive weights incl. "gold × worships_gold" | transcribed `borg_power()`: dynamic calcs **replace** equipment/inventory; +40,000 per prepared depth | `borg-power.c:borg_power:1881-1912` | transcription check |
| 5.2 | worship flags "multiply the component"; six affect power | five select weight branches; `borg_worships_gold` not read by `borg-power.c` | `borg-power.c:borg_power_equipment:87-110`, `:586-600` | `grep -n WORSHIPS src/borg/*.c` |
| 5.4 | "that is why `borg_simulate` exists" | simulation = write candidate, re-notice, restore from `safe_items[]`; `borg_simulate` is the scoring flag | `borg-item-wear.c` (`borg_best_stuff_aux()`) | read |
| 5.5 | absent | `borg_prepared()` depth gating | `borg-prepared.c:borg_prepared:538-686` | read |
| 6.1 | `struct borg_data { int16_t cost; uint8_t xtra; ... }` | `uint8_t data[AUTO_MAX_Y][AUTO_MAX_X]` | `borg-flow.h:borg_data:58-60` | transcription check |
| 6.1, 6.2 | flood starts at the Borg; commit walks back and presses a key | goals enqueued at cost 0; flood radiates from them; commit copies cost→flow, no key; `borg_flow_old()` steps downhill | `borg-flow.c:borg_flow_enqueue_grid:506-580`, `borg-flow.c:borg_flow_commit:585-608`, `borg-flow.c:borg_flow_old:1122-1203` | transcription check |
| 6.2 | `int danger, bool cave` | `int stair_idx, bool sneak` | `borg-flow.h:borg_flow_clear:131-152` | transcription check |
| 6.3 | `GOAL_BORE`/`GOAL_FLEE`/`GOAL_DIGGING` in `borg-flow-misc.c`; four extra builders | committers derived from `borg_flow_commit()` call sites; stairs and kill files; the four builders deleted | `borg-trait.h:GOAL_KILL:32-41` | `grep -n 'borg_flow_commit(' src/borg/*.c` |
| 7.1 | `borg_think_dungeon()` → `borg_defend()` then `borg_attack()` | `borg_caution()` → defend; ladder → `borg_attack(false)`; brave → defend at 100, `borg_attack(true)`; munchkin → `borg_attack(false)` | `borg-caution.c:borg_caution:1001-1022`, `borg-think-dungeon.c:borg_think_dungeon:1256-2302` | call-site grep (§1) |
| 7.2 | `BF_ATTACK`, `BF_WHIRLWIND`, `BF_THROW`, `BF_QUIVER` | `BF_THRUST`, `BF_OBJECT`, `BF_LAUNCH`, `BF_SPELL_WHIRLWIND_ATTACK`, … | `borg-fight-attack.h` | identifier check |
| 8 | pseudocode without guards | transcribed incl. depth ≤ 80, vault, antibounce guards and `full_damage = true` | `borg-danger.c:borg_danger:2824-2865` | transcription check |
| 9.1 | `borg_caution_phase()` is the reactive chain | `borg_caution()` in `borg-caution.c`; `borg_caution_phase()` = phase-door landing simulation | `borg-caution.c:borg_caution:799-2034`, `borg-escape.c:borg_caution_phase:324-412` | read |
| 9.2 | `borg_escape_hall()`; `borg_escape_stair()` heads for a stair | deleted; `borg_escape_stair()` (static) takes the up stair underfoot | `borg-escape.c` | identifier check |
| 10.1, 10.2 | `borg_item_val()` scores items; `borg_item_note` | no such functions; `borg-item-val.c` = name lookups; scoring is `borg_power()` | `borg-item-val.c:borg_init_item_val:261-516` | identifier check |
| 10.4 | — | `borg_best_stuff()` runs only in the home | `borg-item-wear.c:borg_best_stuff:1331-1431` | read |
| 10.6–10.9 | absent | spells/books, light/fuel, junk, recovery | `borg-magic.c`, `borg-light.c`, `borg-junk.c`, `borg-recover.c` | read |
| 11 | 43 `CBUG-*` ids and issue numbers | no ids; examples by code; board linked once in §18 | — | `grep -c 'CBUG-' docs/c-borg/BORG_SPEC.md` (0) |
| 11.2 | sane form `borg.trait[BI_CCURHP]` | `borg.trait[BI_CURHP]`; the `borg.has[BI_CURHP]` test exists in `borg-fight-defend.c` | `grep -n 'has\[BI_' src/borg/*.c` | identifier check |
| 11.3, 11.11 | artifact+ego scores as ego; second scan rescues the artifact | the `+20000` note assignment fires for every ordinary candidate, so neither bonus survives; exception: cursed non-artifact in bow/weapon slot | `borg-item-wear.c:borg_test_stuff:101-205`, `borg-item-id.c:borg_item_note_needs_id:145-155`, `src/obj-knowledge.c` `object_fully_known()` | `teststuff_test.c`, `teststuff_test3.c` |
| 11.6 | zeroing of weapon and bow incl. `sval` | only for empty or cursed non-artifact items; `sval` only on the bow | `borg-trait.c:borg_notice_equipment:1868-1876`, `:1947-1955` | read |
| 11.7 | `borg_refuel_lantern()` indexes with −1 | guarded; the defect is the scan start from `borg_slot()`'s last-match tie-break | `borg-light.c:borg_refuel_lantern:552-608`, `borg-inventory.c:borg_slot:88-127` | read |
| 11.8 | `.bss` addresses from one binary, counts from a probe with the opposite layout; "212 off the map"; "sidebar off" only | overflow stated; consequence build-dependent; one named build's layout and simulated result in the probe; `SIDEBAR_TOP` also overflows | `borg-flow-misc.c:borg_flow_light:834-847`, `borg-cave-light.h:AUTO_LIGHT_MAX:30` | §3 below; `glow_bound_test.c`, `glow_overflow_probe.c` |
| 11.9 | wall+trap example | the ordinary case is a monster on a known trap; `monsters` counts awake only | `borg-escape.c:borg_surrounded:219-236` | `surrounded_test.c` |
| 11.10 | scumming sets `borg.goal.fleeing_lunal` | scumming sets `borg.goal.fleeing`; `fleeing_lunal` is set only by lunal mode; the "Town" guard is unreachable at the only call site | `borg-think-dungeon-util.c:borg_think_stair_scum:363-368`, `borg-think-dungeon.c:borg_think_dungeon:1340-1344` | `grep -n 'fleeing_lunal\s*=' src/borg/*.c` |
| 11.12 | a pack wand/staff qualifies | only rods carry a timeout among unwieldable items; the last-match preference stands | `borg-item-wear.c:borg_wear_recharge:1451-1538`, `src/obj-tval.c` `tval_can_have_timeout()` | `recharge_test.c` |
| 13 | 36 settings; `borg_lunal_mode` "use lunal mode"; `borg_self_scum` "(lunal) mode"; respawn −1 = keep; `borg_save_death` "save on death" | 35 settings (+ `BORG_MAX_SETTINGS`); `borg_lunal_mode` only seeds `borg.lunal_mode`; self-scum = scum for wanted items; −1 = random; save_death gates the death logs; "Read by" column | `borg.h`, `borg-init.c` | `grep -n "borg_cfg\[BORG_X\]" src/borg/*.c` per setting |
| 13.1 | absent | mode table | `borg-think-dungeon.c:borg_think_dungeon:1438-1456`, `:1460-1471` | read |
| 15 | `borg_log_death()` writes `borg.dat` | `borg-log.txt`; `borg_log_death_data()` writes `borg.dat` | `borg-log.c:borg_log_death:48-89`, `borg-log.c:borg_log_death_data:94-121` | read |
| 16 | ten invariants incl. "everything re-derived", clock overflow, worship multipliers | re-sorted into behaviour / decision constants / configuration; kill list patched; unreachable reset and dead "Best Combo" trigger demoted | — | — |
| 17 | "the comment is the specification. Fix it" | conflict stated; §16 wins by default; UB fixes are not departures; other fixes are declared departures; comment is evidence, code is the default reading | `borg-escape.c:borg_recall:64-117` | `recall_branch_test.c` |
| 18 | no pointer to the check programs | added | — | — |

### Where this re-check disagrees with the review

* **Routine combat.** The review says routine combat goes through
  `borg_flow_kill()`. At the pin `borg_think_dungeon()` calls
  `borg_attack(false)` directly (the rung after `borg_flow_kill_corridor()`);
  `borg_flow_kill()` only walks towards a monster (`GOAL_KILL`). §7.1 says so.
* **`borg_flow_light()`** exists at the pin (`borg-flow-misc.c`, 824-874),
  and the old §11.8 transcription of its scan loop matches it; what was
  wrong was the layout analysis, not the function.
* **`borg_refuel_lantern()`** exists (static, `borg-light.c`, 552-608); the
  review's description (guarded; scan starts at the last lantern) is right.
* **`borg.has[BI_CURHP]`** is real code (`borg-fight-defend.c`), so the old
  §11.2 example was right about the bug and wrong only about the fix
  (`BI_CCURHP`).
* **Six worship options** in the review's E9: five are read by
  `borg-power.c`; `borg_worships_gold` is not.

### Further errors found beyond the review

* `borg_item_note` (no such identifier) and `borg_caution.c` (no such file)
  in the old draft.
* §2.5: the `<= 200` guard is on `borg_fear_monsters[][]`, not regional fear.
* §5.1: dynamic calcs replace, not add; the prepared-depth bonus was missing.
* §13: three wrong effect descriptions (`borg_lunal_mode`, `borg_self_scum`,
  respawn −1) and the setting count (35, not 36).
* §11.10: the state attributed to scumming (`fleeing_lunal`) is lunal mode's.
* The check programs (§4).

## 3. The glow-array layout (criterion 4)

The specification names no address. The probe names one build:

```sh
git archive origin/c-borg-reference | tar -x -C "$TREE"
cd "$TREE"
flock /tmp/angband-validation.lock sh -c \
  'cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DSUPPORT_BORG=ON && cmake --build build -j8'
nm -S -n build/game/angband | grep -E ' (borg_glow|borg_light)_[xyn]$'
objdump -d --no-show-raw-insn build/game/angband | awk '/<borg_flow_light>:/,/<borg_flow_vault>:/'
```

gcc 15.2.0; `CMAKE_C_FLAGS_RELWITHDEBINFO` is `-O2 -g -DNDEBUG`. Output:

```text
000000000022e940 0000000000000600 B borg_glow_y
000000000022ef40 0000000000000600 B borg_glow_x
000000000022f540 0000000000000002 B borg_glow_n
000000000022f560 0000000000000600 B borg_light_y
000000000022fb60 0000000000000600 B borg_light_x
0000000000230160 0000000000000002 B borg_light_n
```

and in `borg_flow_light` (abridged `objdump` lines):

```text
  157af9:	xor    %r9d,%r9d                       counter in %r9
  157b0b:	lea    0xd742e(%rip),%r14  # borg_glow_x
  157b6f:	add    $0x1,%r9d
  157b73:	lea    0xd6dc6(%rip),%rdx  # borg_glow_y
  157b7a:	mov    %cl,(%r14,%rax,1)               x stored first
  157b7e:	mov    %bpl,(%rdx,%rax,1)              then y
  157be4:	mov    %r9w,0xd7954(%rip)  # borg_glow_n   stored once, after the loop
  157c33:	movswl 0xd7906(%rip),%eax  # borg_glow_n   re-read in the read-back loop
```

So in this build `borg_glow_y[1536..]` overwrites the head of
`borg_glow_x[]`, `borg_glow_x[1536..1537]` land on `borg_glow_n` (then
overwritten by the final store), `borg_glow_x[1538..1567]` on padding and
`borg_glow_x[1568..]` on `borg_light_y[]`. `glow_overflow_probe.c`
simulates exactly this; its result (below) is for this build only. The
previous probe declared the arrays in the opposite order and relied on its
own compiler's layout, which is why it reported a different consequence.

## 4. The check programs (criterion 5)

What was wrong in each, and what changed:

| Program | Problem in the previous version | Now |
| --- | --- | --- |
| `teststuff_test.c` | case 2 printed that 250100 is "BELOW" 105000; the needs-ID note was an independent input, but it follows from `ident`/`needs_ident` and is true for every ordinary candidate; loop 2's `else if (free_id \|\| borg_item_worth_id())` was modelled as `else if (!art_idx)`; `TV_FOOD` missing; the `needs_ident` filter missing | transcribes `borg_item_note_needs_id()`/`borg_item_worth_id()` and both loop bodies; shows that the artifact/ego bonuses never survive for ordinary candidates, and the cursed-slot exception; after the second review: artifact+ego marked `SYNTHETIC`, cursed values 0 |
| `teststuff_test3.c` | built on the same independent-note model | shows loop 1 changes loop 2's choice only through a tie (case C, added after the second review) or the cursed-slot exception |
| `blows_test.c` | `WARRIOR_MAX_ATTACKS 5` (class.txt has 6); final step `(blows + 100) / 100` instead of `MAX(blows + 100 * extra, 100) / 100`, so every count was one too high; weights labelled in pounds (they are tenths); dex input unlabelled | fixed; the divergent example is now 1 blow (Borg) vs 2 (engine) |
| `glow_bound_test.c` | sidebar mode 1 labelled "RIGHT" (it is `SIDEBAR_TOP`); "OVERFLOW" stated as a fact | labels fixed; "can overflow (if every grid is BORG_GLOW)"; tile size stated |
| `glow_overflow_probe.c` | relied on its own compiler's layout (opposite of the cited build); counts reported as the game's; the "first wrong ones" loop started at 1536 and printed correct entries | simulates the named build's layout and code in a byte arena; prints actual wrong entries; result labelled as that build's |
| `recall_branch_test.c` | controls D, F and G never reached the branch their labels named (they failed the depth-window test first) | inputs chosen so each control reaches its branch; prints the Borg's note as well as the comment and key |
| `recharge_test.c` | the "worn ring" was in the pack (index 2 < `INVEN_WIELD` 4); all `TV_LIGHT` skipped (source skips torches and lanterns only); "pack wand with a timeout" presented as ordinary; a confusing remark that the source condition is "inverted" | real worn/pack indexes; source light rule; the wand case marked `SYNTHETIC` with the reason |
| `surrounded_test.c` | hallway exception used `monsters` where the source uses `adjacent_monsters`; the headline case (walls with traps) is the least plausible state; "clearly meant to be" about intent | `adjacent` input; ordinary case (monsters, asleep or awake, on known traps) added; intent claim softened |
| `README.md` | said §18 of the specification holds the harness commands (they are in `PROVENANCE.md`); claimed declaration order fixes the `.bss` layout | corrected |

Output of all eight, compiled and run as in the README
(`gcc -O0 -Wall -Wextra`, gcc 15.2.0); "(compiled: 0 warnings)" means gcc
printed nothing:

```text
gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0
##### blows_test.c
(compiled: 0 warnings)
Warrior (lib/gamedata/class.txt): strength-multiplier=5 min-weight=30 max-attacks=6
extra blows 0, birth_percent_damage off.

--- weight 30 (3 lb), STR index 9 (STR 12), dex column 0 ---
iqty 1 (wielded)               weight borg   30 / engine   30 | str_index borg  2 / engine  2 | blows borg 1 / engine 1
iqty 5 (a home stack)          weight borg  150 / engine   30 | str_index borg  0 / engine  2 | blows borg 1 / engine 1

--- weight 30 (3 lb), STR index 3 (STR 6), dex column 5 (DEX 18/60) ---
iqty 1 (wielded)               weight borg   30 / engine   30 | str_index borg  1 / engine  1 | blows borg 2 / engine 2
iqty 2 (a home stack)          weight borg   60 / engine   30 | str_index borg  0 / engine  1 | blows borg 1 / engine 2   <-- DIVERGES
iqty 5 (a home stack)          weight borg  150 / engine   30 | str_index borg  0 / engine  1 | blows borg 1 / engine 2   <-- DIVERGES

--- weight 30 (3 lb), STR index 11 (STR 14), dex column 11 ---
iqty 1 (wielded)               weight borg   30 / engine   30 | str_index borg  2 / engine  2 | blows borg 5 / engine 5
iqty 3 (a home stack)          weight borg   90 / engine   30 | str_index borg  0 / engine  2 | blows borg 4 / engine 5   <-- DIVERGES

The engine weighs one item; the Borg's copy weighs the stack.
For the wielded weapon (iqty 1) they agree.  borg_calc_blows()
is also called on home items (borg-home-notice.c), where weapon
stacks occur, and there the Borg can count fewer blows.
exit=0
##### glow_bound_test.c
(compiled: 0 warnings)
AUTO_LIGHT_MAX = 1536

--- 80x24 (the classic terminal) ---
SIDEBAR_LEFT (0)             22x66  =  1452 cells   last write index  1451   within the arrays
SIDEBAR_TOP  (1)             20x79  =  1580 cells   last write index  1579   can overflow (if every grid is BORG_GLOW)
SIDEBAR_NONE (2)             23x79  =  1817 cells   last write index  1816   can overflow (if every grid is BORG_GLOW)

--- 80x28 ---
SIDEBAR_LEFT (0)             26x66  =  1716 cells   last write index  1715   can overflow (if every grid is BORG_GLOW)
SIDEBAR_TOP  (1)             24x79  =  1896 cells   last write index  1895   can overflow (if every grid is BORG_GLOW)
SIDEBAR_NONE (2)             27x79  =  2133 cells   last write index  2132   can overflow (if every grid is BORG_GLOW)

--- 120x40 ---
SIDEBAR_LEFT (0)             38x106 =  4028 cells   last write index  4027   can overflow (if every grid is BORG_GLOW)
SIDEBAR_TOP  (1)             36x119 =  4284 cells   last write index  4283   can overflow (if every grid is BORG_GLOW)
SIDEBAR_NONE (2)             39x119 =  4641 cells   last write index  4640   can overflow (if every grid is BORG_GLOW)

The write passes the end of the arrays once more than 1536 panel
grids carry BORG_GLOW (grids the Borg believes are perma-lit).
This program shows only the capacity; it does not show that a
game reaches that many glowing grids.
exit=0
##### glow_overflow_probe.c
(compiled: 0 warnings)
build: e473720ed RelWithDebInfo (-O2), layout from nm -S (see source)
glowing panel grids written: 1817 (array capacity 1536)

entries the read-back loop enqueues: 1817
first wrong entries:
  entry    0: meant ( 0, 0)  enqueued ( 0,19)
  entry    1: meant ( 0, 1)  enqueued ( 0,19)
  entry    2: meant ( 0, 2)  enqueued ( 0,19)
  entry    3: meant ( 0, 3)  enqueued ( 0,19)
  entry 1536: meant (19,35)  enqueued (19,25)
  entry 1537: meant (19,36)  enqueued (19, 7)

wrong entries: 279 of 1817; off the panel: 0
borg_light_y[] entries overwritten: 249 (from borg_light_y[0])

In this build the wrong entries have the right y and a wrong x
(another grid's y, or a byte of borg_glow_n), all on the panel.
This is a property of this build, not of the source.
exit=0
##### recall_branch_test.c
(compiled: 0 warnings)
case                                                                     key  note written by the code
A: TARGET maxdepth 99, depth 95, hunted 92, 3 uniques alive              n    # Not Resetting recall depth.
                                                                              comment: Do reset Depth, cleaning up last uniques to hit Morgoth.  Want to keep using up the stock of potions
                                                                              *** comment says reset; key and note say not ***

B: TARGET maxdepth 99, depth 88, hunted 85, 4 uniques alive              n    # Not Resetting recall depth.
                                                                              comment: Do reset Depth, cleaning up last uniques to hit Morgoth.  Want to keep using up the stock of potions
                                                                              *** comment says reset; key and note say not ***

C: control, depth 95, Sauron dead (Sauron branch)                        y    # Resetting recall depth.
                                                                              comment: Do reset Depth

D: control, maxdepth 99, 5 uniques alive (final else)                    y    # Resetting recall depth.
                                                                              comment: Do reset Depth

E: control, depth far from the hunted depth (window fails)               n    # Not resetting recall depth.
                                                                              comment: else branch: Do not reset Depth

F: control, fleeing munchkin (munchkin branch)                           y    # Resetting recall depth during munchkin mode.
                                                                              comment: Do reset Depth

G: control, depth 100 below maxdepth 110, not king (depth>=100 branch)   n    # Not Resetting recall depth.
                                                                              comment: Do not reset Depth

H: control, depth not below maxdepth (no prompt)                         -    (none)
                                                                              comment: (no prompt: depth 50 is not below maxdepth 50)

exit=0
##### recharge_test.c
(compiled: 0 warnings)
depth 5 (in the dungeon) for every case

control: nothing recharging                          chosen: (none)                     nothing found -> return false
control: worn ring recharging                        chosen:                            rest R 7 5 Enter (worn item or rod)
                                                             worn ring (index 7)
control: pack amulet recharging                      chosen:                            wear it, then rest (pack wearable)
                                                             pack amulet (index 1)
LAST MATCH: pack amulet, then pack rod               chosen:                            rest R 7 5 Enter (worn item or rod)
                                                             pack rod (index 2)
LAST MATCH: pack rod, then worn ring                 chosen:                            rest R 7 5 Enter (worn item or rod)
                                                             worn ring (index 7)
SYNTHETIC: pack amulet, then pack wand w/ timeout    chosen:                            GIVE UP -> return false
                                                             pack wand (index 3)

The scan keeps the LAST item with a timeout; there is no
preference between resting for a rod, resting for a worn item,
and swapping in a pack item.  The give-up path needs an
unwieldable non-rod item with a timeout (SYNTHETIC above).
exit=0
##### surrounded_test.c
(compiled: 0 warnings)
safe_grids starts at 8; the verdict is `monsters > safe_grids`
(monsters = awake, visible, within 3 grids).

control: 8 plain floors                        as-written safe=  8 not surrounded | trap test as else-if: safe=  8 not surrounded
control: 1 wall + 7 floors                     as-written safe=  7 not surrounded | trap test as else-if: safe=  7 not surrounded
control: 1 known trap on a floor + 7 floors    as-written safe=  7 not surrounded | trap test as else-if: safe=  7 not surrounded
1 awake monster standing on a trap             as-written safe=  6 not surrounded | trap test as else-if: safe=  7 not surrounded
4 SLEEPING monsters on traps + 1 trap, 0 awake as-written safe= -1 SURROUNDED     | trap test as else-if: safe=  3 not surrounded  <-- THE VERDICT DIFFERS
3 monsters on traps (2 awake) + 3 walls        as-written safe= -1 SURROUNDED     | trap test as else-if: safe=  2 not surrounded  <-- THE VERDICT DIFFERS
4 wall+trap (unusual) + 4 trap floors, 0 awake as-written safe= -4 SURROUNDED     | trap test as else-if: safe=  0 not surrounded  <-- THE VERDICT DIFFERS

The trap test is a separate `if`, so a grid that also satisfies
the chain (a wall, an unknown grid, a monster -- asleep or awake --
or a shop) is counted twice.  The ordinary case is a monster
standing on a known trap.  A borg_grid with trap set and a wall
feature needs the map to change under a remembered trap; this
program does not show that it happens.
exit=0
##### teststuff_test.c
(compiled: 0 warnings)
=== 1. Which (ident, needs_ident) pairs can reach the scoring? ===
object_fully_known() requires object_runes_known(), so straight
from borg_item_analyze() needs_ident=1 implies ident=0.

  ident=0 needs_ident=0  note_needs_id=0   skipped by both loops (!needs_ident)
  ident=0 needs_ident=1  note_needs_id=1   every ordinary candidate
  ident=1 needs_ident=0  note_needs_id=0   skipped by both loops (!needs_ident)
  ident=1 needs_ident=1  note_needs_id=0   only via the cursed bow/weapon zeroing

=== 2. Ordinary candidates (ident=0, needs_ident=1) ===
The +20000 assignment always fires and overwrites the artifact
and ego assignments above it, in both loops.  (free_id=0, depth 30)

  SYNTHETIC: artifact + random-power ego         loop1   25000   loop2   25000   (note_needs_id=1)
  plain artifact                                 loop1   25000   loop2   25000   (note_needs_id=1)
  random-power ego, not an artifact              loop1   25000   loop2   25000   (note_needs_id=1)
  plain wieldable item needing ID                loop1   25000   loop2   25000   (note_needs_id=1)
  -> all four score value+20000: the artifact and ego bonuses
     never survive for an ordinary candidate.

=== 3. The exception: a cursed non-artifact in the bow/weapon slot ===
borg_notice_equipment() sets ident=1 on it, so note_needs_id()=0.

  cursed random-power ego weapon (wielded)       loop1  100000   loop2       0   (note_needs_id=0)
  cursed plain weapon (wielded)                  loop1       0   loop2       0   (note_needs_id=0)
  with free_id=1 (Identify Rune castable):
  cursed random-power ego weapon (wielded)       loop1  100000   loop2       0   (note_needs_id=0)
  cursed plain weapon (wielded)                  loop1       0   loop2       0   (note_needs_id=0)
  -> here the ego bonus does survive loop 1 (0+100000); in loop 2
     the item scores its value, 0, so it is not a loop-2 candidate.
exit=0
##### teststuff_test3.c
(compiled: 0 warnings)
=== A. ordinary candidates (BI_MAXDEPTH 10, free_id 0) ===
    pack: unaware ring                           loop1       0  loop2   70100
    worn: artifact needing ID                    loop1   25000  loop2   25000
  after loop 1 : b_v   25000  worn: artifact needing ID
  after loop 2 : b_v   70100  pack: unaware ring
  loop 2 alone : b_v   70100  pack: unaware ring

=== B. the exception: cursed non-artifact ego in the weapon slot ===
    pack: unaware ring                           loop1       0  loop2   70100
    worn: CURSED random-power ego weapon         loop1  100000  loop2       0
  after loop 1 : b_v  100000  worn: CURSED random-power ego weapon
  after loop 2 : b_v  100000  worn: CURSED random-power ego weapon
  loop 2 alone : b_v   70100  pack: unaware ring

=== C. a tie between a pack item and a worn item ===
    pack: plain wieldable item needing ID        loop1       0  loop2   20100
    worn: plain item needing ID, same value      loop1   20100  loop2   20100
  after loop 1 : b_v   20100  worn: plain item needing ID, same value
  after loop 2 : b_v   20100  worn: plain item needing ID, same value
  loop 2 alone : b_v   20100  pack: plain wieldable item needing ID

In A loop 1 makes no difference.  In B the cursed weapon's loop-1
score (0+100000) blocks the unaware ring (100+20000+50000) that
loop 2 alone would have chosen.  In C the scores tie at 20100 and
loop 1 keeps the worn item, where loop 2 alone keeps the pack item.
exit=0
```

## 5. Housekeeping (criterion 6)

Run from the repository root of the branch:

```text
$ node tools/check-session.mjs
diegoami/neo-angband-tracker: engine-and-c-fixes; bootstrap files and origin verified
$ node tools/check-fork-workflows.mjs
11 inherited jobs restricted to neostryder/neo-angband; fork tag/manual/release/schedule events skip every job, including always() jobs
$ node tools/c-bugs.mjs --check
c-bugs: 152 individually tracked import records; source coverage valid; issue list owns live state
$ node tools/private-scan.mjs
private-scan: clean (2169 tracked files)
$ node tools/citation-check.mjs
citation-check: 6595 citation(s) extracted
  OUT OF RANGE     : 2
  ANCHOR MISMATCH  : 108
  anchor OK        : 1641  (checked and passed)
  UNCHECKED        : 4844  (in range, nothing checkable found)
  -> mechanically checked (not UNCHECKED): 1751/6595 (26.6%)
```

`citation-check` was run with `--json` on the previous draft and on this one
and the non-OK rows compared by (verdict, file, citation, symbol): **no row
added, none removed**; the 108 `ANCHOR_MISMATCH` and 2 `OUT_OF_RANGE` rows
are the pre-existing ones in the Neo port (the review counted the same 108
on `master`). The `UNCHECKED` count fell from 4850 to 4844: the previous
draft's plain `file.c:N` citations are gone; the remaining new ones are
this file's own (its example of the plain form in §1, and this sentence),
which the tool resolves against `reference/` and leaves unchecked.

`node tools/citation-check.mjs --check` exits 1 on this branch **and on
`master`**, because of the same two `OUT_OF_RANGE` rows
(`packages/core/src/game/effect-exp-seam.test.ts:3`,
`packages/web/src/main.ts:11593`); this branch adds none.

The diff against `master` touches only `docs/c-borg/BORG_SPEC.md` and
`docs/audit/c-reports/spec-checks/`; nothing under `.github/`, no C or
TypeScript source.

## 6. Second review (at `9a6dcaa41`): what changed

[Review](https://github.com/diegoami/neo-angband-tracker/pull/317#issuecomment-6698919506).
Every item was re-verified at the pin before changing anything; all were
confirmed. Spec sections are in `docs/c-borg/BORG_SPEC.md`.

| Item | Verified at the pin | Change |
| --- | --- | --- |
| E1 perception: real race/kind reads; `map_info()` objects | `borg-flow-kill.c:borg_guess_race:892-907`, `borg-flow-take.c:borg_guess_kind:72-84`, `borg-flow-take.c:borg_get_top_object:50-64`; chest `pval` in `borg_play_step()`; `src/cave-map.c` loops over `square_object(player->cave, …)` | §1.1 adds an "identities" bullet and remembered objects; §1.1 "does not do" restated (a monster starts from a visible sighting or a message name); §4.4 rewritten; §16.1.3 lists the identity reads |
| E2 cure-all is a game action | `borg-messages-react.c:borg_react_prompted:105-139`; `do_cmd_wiz_cure_all()` in `src/cmd-wizard.c` | §1.1, §1.2, §16.1.1: keystrokes are the action channel with one explicit cheat-death exception |
| E3 late unique gate | `borg-prepared.c:borg_prepared:626-685` | §5.5 step 5 rewritten as the two branches, incl. `BI_MAXDEPTH <= 98`, the `>= 98` depth test not gated by `borg_kills_uniques`, `ready_morgoth`, and the Teleport Level exception |
| E4 caution order; mage guard | `borg-caution.c:borg_caution:799-2034` (restock/danger flags after defend/heal, swap, recall) | §9.1 rewritten as 15 steps in source order with the mage guard |
| E5 flow builders, `GOAL_BORE`, cache lifetime | `borg_flow_dark_1()`/`_2()` call `borg_flow_direct()`; `borg_flow_stair_less(GOAL_BORE, …)` in `borg_leave_level()`; know/icky wiped only with `borg_danger_wipe` | §6.2 names the direct-path variant and the cache lifetime; §6.3 `GOAL_BORE` row; §6.5 evaluation timing |
| E6 config/mode dispatch | lunal routine only in `borg-think-dungeon.c:borg_think_dungeon:1438-1456`; self-scum sets `borg_cfg[BORG_MONEY_SCUM_AMOUNT]` (`borg-store-buy.c`); stop needs gold `>` target and `!borg_self_scum` (`borg.c:check_for_deactivate_or_death:190-199`); risky in `borg_heal()` and `borg_escape()`; desperate `< 20` case | §12 money scumming; §13 rows; §13.1 lunal, self-scum, money-scum, desperate rows |
| E7 practice gate; light prep; junk | `borg-magic-play.c:borg_play_magic:165-170`; `borg_prepared_aux()` light/fuel checks; `borg-junk.c` `p > b_p`, inscribe, drop, later `k` via `borg_destroy_floor()` | §10.6, §10.7, §10.8 |
| E8 teststuff inputs | `src/obj-make.c` `apply_magic()` returns after `make_artifact()`; `borg-item-analyze.c:borg_item_analyze:620-622` cursed value 0; tie retention | programs relabelled/normalised, tie case C added; §11.3, §11.11; README; owner note below |
| O twitch state machine | `borg-think-dungeon.c:borg_think_dungeon:2124-2263`; `> 20` at 1501 | §2.8 ladder lines and a paragraph on the counter |
| O fear overrides | `borg-flow.c:borg_flow_spread:442-474` | §6.5 lists all nine assignments in order |
| O store dispatch | `borg-think-store.c:borg_choose_shop:86-280` | §12 lists the dispatch before steps 1–7B |
| O "Read by" completeness | `grep -l 'borg_cfg\[BORG_X\]' src/borg/*.c` for all 35 | §13 column now lists every reader file |
| J §16 simulation rule; §17.2 grouping | — | §16.2 item 6 states the discipline and its violations; §17.2 splits reachable decision changes from latent/masked ones |
| PROVENANCE present-tense label statement | — | rephrased as the state when filed |
| Checker weaknesses | §1 above | scripts hardened; W4 documented as out of reach |

## Observations for the owner (findings not changed)

The re-check bears on four of the findings that cite these programs. None
was edited; each needs the owner's decision.

* **#315 (`borg_test_stuff()`).** The issue says an artifact that is also a
  random-power ego "is scored as a plain ego" and that the second scan
  rescues the artifact. At the pin, for every ordinary candidate the
  `+20000` assignment overwrites both bonuses in both loops
  (`teststuff_test.c` §2), and an item that is both an artifact and an ego
  does not arise in ordinary generation (`apply_magic()` returns after
  `make_artifact()`). The overwrite is real and larger than described; the
  stated scenario and the "masked" reasoning do not hold. Loop 1 changes
  loop 2's choice only through a tie or a cursed non-artifact in the
  weapon/bow slot (value 0) (`teststuff_test3.c`).
* **#313 (blows).** The issue's headline example ("2 where the engine gives
  3") came from the program's off-by-one; the corrected program gives 1
  where the engine gives 2 for the same inputs. The divergence itself
  stands.
* **#316 (`borg_wear_recharge()`).** The give-up path needs an unwieldable
  non-rod item with a timeout. Wands and staffs never have one at the pin
  (`tval_can_have_timeout()` is `TV_ROD` only; activation timeouts are on
  wearable items). The last-match selection is real; the headline scenario
  ("an unwearable pack wand") is not one ordinary play produces.
* **#311 (glow overflow).** The write past the end is real. "212 off-map
  flow targets" is a property of the old probe's layout; in the one build
  measured here no enqueued grid is off the panel, x coordinates are wrong
  near the start of the list, and `borg_light_y[]` is overwritten
  (`glow_overflow_probe.c`). Other builds can differ.
* **#312 (surrounded).** The double count is real; the realistic trigger is
  monsters (asleep or awake) on known traps rather than walls with traps,
  and a sleeping monster adds to `non_safe_grids` without adding to
  `monsters`.
* **#310 (recall depth).** Confirmed; the Borg's own note agrees with the
  key, not with the comment, which matters for §17 of the specification.

## Not verified here

* Anything that needs a game run: how often any branch is reached, whether
  more than 1536 `BORG_GLOW` grids occur in practice, the harness runs
  described in `PROVENANCE.md` (its source is not on any branch here).
* The exact numeric weights inside `borg_power_equipment()` /
  `borg_power_inventory()` beyond the branches cited, the per-band contents
  of `borg_prepared_aux()` beyond the outline, and the store-step details of
  `borg_choose_shop()` beyond its step list.
* The `^Z` debug-command surface.
* The glow-array consequence for any build other than the one named.
