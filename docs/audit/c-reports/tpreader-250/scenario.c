/* Synthetic native C Teleport Other stale-list danger diagnostic, excluded from the source fix. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "cmd-core.h"
#include "cmds.h"
#include "game-event.h"
#include "game-world.h"
#include "init.h"
#include "mon-make.h"
#include "mon-util.h"
#include "monster.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "option.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-timed.h"
#include "player-util.h"
#include "ui-input.h"
#include "ui-menu.h"
#include "ui-term.h"
#include "borg/borg.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-fight-attack.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-trait.h"
#include <stdio.h>
#include <stdlib.h>

static term screen;
static struct monster *giant;
static int giant_kill;

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) quit("fixture item kind missing");
    object_prep(obj, kind, 50, MAXIMISE);
    obj->number = number;
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
    return obj;
}

static int observe(struct monster *mon, const char *tag)
{
    mflag_on(mon->mflag, MFLAG_AWARE); update_mon(mon, cave, true);
    if (!mflag_has(mon->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL %s visible\n", tag); return 1; }
    if (!observe_kill_diff(mon->grid.y, mon->grid.x, 0, 0)) { printf("SETUPFAIL %s observe\n", tag); return 1; }
    int k = borg_grids[mon->grid.y][mon->grid.x].kill;
    if (k <= 0) { printf("SETUPFAIL %s kill_index\n", tag); return 1; }
    borg_kills[k].awake = 1;
    if (borg_kills[k].power <= 0) { printf("SETUPFAIL %s power\n", tag); return 1; }
    printf("SETUP %s kill=%d power=%d\n", tag, k, borg_kills[k].power);
    return 0;
}

int setup_tests(void **state)
{
    const char *profile = getenv("TP250_PROFILE");
    const char *mode = getenv("TP250_MODE");
    if (!profile) return 1;
    printf("SETUPSTEP profile\n");
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(1000);
    if (!player_make_simple("Human", "Warrior", "TpOtherStaleList")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    known_item(TV_LIGHT, "Wooden Torch", 1);
    player_place(cave, player, loc(11, 11));
    calc_inventory(player);
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    struct object *torch = NULL;
    for (int i = 0; i < player->upkeep->inven_cnt; ++i) {
        struct object *o = player->upkeep->inven[i];
        if (o && o->tval == TV_LIGHT) { torch = o; break; }
    }
    if (torch) { torch->timeout = 2000; }
    giant = t_add_monster(cave, loc(12, 11), "hill giant");   /* adjacent */
    if (!giant) { printf("SETUPFAIL giant\n"); return 1; }
    if (observe(giant, "giant")) return 1;
    giant_kill = borg_grids[giant->grid.y][giant->grid.x].kill;
    borg.status.active = true;
    borg_update_entrypoint(true);
    /* The Teleport Other exclusion list, in the state a longer earlier
     * evaluation leaves behind: one current entry and one stale entry one
     * past the count. */
    if (!mode || streq(mode, "stale-hit")) {
        /* One real entry that is not the giant, plus the stale leftover
         * one past the count: the giant's index, left there by a longer
         * earlier evaluation. */
        borg_tp_other_index[0] = 0;
        borg_tp_other_index[1] = giant_kill;
        borg_tp_other_n = 1;
    } else if (streq(mode, "in-list")) {
        borg_tp_other_index[0] = giant_kill;
        borg_tp_other_n = 1;
    } else {
        borg_tp_other_n = 0;
    }
    printf("SETUP n=%d idx0=%d idx1=%d\n", borg_tp_other_n,
        borg_tp_other_index[0], borg_tp_other_index[1]);
    printf("SETUP seed=1000 class=%s level=%d depth=%d hp=%d/%d\n",
        player->class->name, player->lev, player->depth, player->chp, player->mhp);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    const char *mode = getenv("TP250_MODE");
    if (!mode) mode = "stale-hit";
    printf("MODE %s\n", mode);

    /* The production per-monster danger routine, exactly as the Teleport
     * Other scoring calls it for the giant two grids away. */
    int d = borg_danger_one_kill(borg.c.y, borg.c.x, 1, giant_kill, true, true);
    printf("DANGER danger=%d\n", d);
    printf("OUTCOME danger=%d\n", d > 0 ? 1 : 0);
    if (streq(mode, "stale-hit")) {
        /* The misread case: the giant is NOT in the one-entry list, but a
         * stale entry one past the count holds its index, so the original
         * excludes its danger entirely. The correction reads only the
         * real entries. */
        eq(d > 0 ? 1 : 0, 1);
    } else if (streq(mode, "in-list")) {
        /* Control: the giant genuinely is in the list; both variants
         * exclude its danger. Complete logs are byte-identical. */
        eq(d, 0);
    } else {
        /* Control: empty list; both variants count the danger. Complete
         * logs are byte-identical. */
        eq(d > 0 ? 1 : 0, 1);
    }
    ok;
}
const char *suite_name = "native-c/tpreader-250";
struct test tests[] = {{"stale_tp_other_entry_does_not_exclude_danger", scenario}, {NULL, NULL}};
