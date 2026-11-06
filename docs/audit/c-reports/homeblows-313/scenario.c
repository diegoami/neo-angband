/* Synthetic native C home-blows diagnostic, excluded from the source fix. */
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
#include "player-spell.h"
#include "player-timed.h"
#include "player-util.h"
#include "ui-input.h"
#include "ui-menu.h"
#include "ui-term.h"
#include "z-virt.h"
#include "borg/borg.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-util.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-fight-attack.h"
#include "borg/borg-fight-defend.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-flow.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item-analyze.h"
#include "borg/borg-item-use.h"
#include "borg/borg-item.h"
#include "borg/borg-magic.h"
#include "borg/borg-power.h"
#include "borg/borg-think-dungeon-util.h"
#include "borg/borg-think-dungeon.h"
#include "borg/borg-think.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
static struct object *rapiers;

int setup_tests(void **state)
{
    const char *profile = getenv("T313_PROFILE");
    const char *mode = getenv("T313_MODE");
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
    if (!player_make_simple("Human", "Warrior", "HomeBlows")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    screen.offset_x = screen.offset_y = 0;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    player_place(cave, player, loc(11, 11));
    player->chp = player->mhp; player->csp = player->msp;
    if (!mode) mode = "stack";

    /* A stack of ten rapiers in the pack (the home-stack case), or a
     * single rapier (control). */
    struct object_kind *kind = lookup_kind(TV_SWORD, lookup_sval(TV_SWORD, "Rapier"));
    if (!kind) { printf("SETUPFAIL kind\n"); return 1; }
    rapiers = object_new();
    object_prep(rapiers, kind, 50, MAXIMISE);
    rapiers->number = streq(mode, "stack") ? 10 : 1;
    rapiers->known = object_new();
    object_set_base_known(player, rapiers);
    object_flavor_aware(player, rapiers);
    inven_carry(player, rapiers, false, false);
    object_touch(player, rapiers);
    player_know_object(player, rapiers);
    player->upkeep->update |= PU_BONUS;
    update_stuff(player);

    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=1000 class=%s str=%d dex=%d stack=%d\n",
        player->class->name, player->stat_cur[STAT_STR],
        player->stat_cur[STAT_DEX], rapiers->number);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    const char *mode = getenv("T313_MODE");
    if (!mode) mode = "stack";
    printf("MODE %s\n", mode);

    /* The borg's copy of the item, parsed the production way. */
    borg_item item;
    char desc[80];
    borg_item_analyze(&item, rapiers, desc, false);
    require(item.iqty == rapiers->number);

    /* The borg's blows for the stack, and the engine's blows for one
     * weapon of the stack. */
    int borg_blows = borg_calc_blows(&item);
    int engine_blows = calc_blows(player, rapiers, &player->state, 0) / 100;
    printf("GEOMETRY iqty=%d weight=%d weight_one=%d\n", item.iqty,
        item.weight, object_weight_one(rapiers));
    printf("RATING borg_blows=%d engine_blows=%d\n", borg_blows, engine_blows);
    printf("OUTCOME borg_blows=%d engine_blows=%d\n", borg_blows, engine_blows);
    /* Correct contract: blows per weapon must not depend on how many
     * weapons are stacked. */
    eq(borg_blows, engine_blows);
    ok;
}
const char *suite_name = "native-c/homeblows-313";
struct test tests[] = {{"stack_weighs_like_one_weapon", scenario}, {NULL, NULL}};
