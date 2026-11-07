/* Synthetic native C armour-swap glove eligibility diagnostic, excluded from the source fix. */
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
#include "borg/borg.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-trait-swap.h"
#include "borg/borg-trait.h"
#include <stdio.h>
#include <stdlib.h>

static term screen;

static struct object *make_item(int tval, const char *name, int number, bool free_act)
{
    struct object *obj = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) quit("fixture item kind missing");
    object_prep(obj, kind, 50, MAXIMISE);
    obj->number = number;
    if (free_act) of_on(obj->flags, OF_FREE_ACT);
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
    if (free_act) of_on(obj->known->flags, OF_FREE_ACT);
    return obj;
}

int setup_tests(void **state)
{
    const char *profile = getenv("GL205_PROFILE");
    const char *mode = getenv("GL205_MODE");
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
    if (!player_make_simple("Human", "Warrior", "GloveArmourSwap")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    if (!mode || streq(mode, "glove-fa")) {
        /* Free-action gauntlets carried while the Borg has none worn. */
        make_item(TV_GLOVES, "Set of Gauntlets", 1, true);
    } else if (streq(mode, "plain")) {
        /* Plain gauntlets: still nothing worth swapping for. */
        make_item(TV_GLOVES, "Set of Gauntlets", 1, false);
    } else if (streq(mode, "have-fa")) {
        /* Free action already worn; the carried pair must stay a non-swap. */
        struct object *worn = make_item(TV_GLOVES, "Set of Gauntlets", 1, true);
        inven_wield(worn, wield_slot(worn));
        calc_inventory(player);
        make_item(TV_GLOVES, "Set of Gauntlets", 1, true);
    }
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
    /* Swaps are only considered at depth 50+. */
    borg.trait[BI_MAXDEPTH] = 60;
    {
        int gs = borg_slot(TV_GLOVES, lookup_sval(TV_GLOVES, "Set of Gauntlets"));
        struct object *probe = NULL;
        for (int i = 0; i < z_info->pack_size; ++i) {
            struct object *o = player->upkeep->inven[i];
            if (o && o->kind && o->tval == TV_GLOVES) { probe = o; break; }
        }
        printf("SETUP gloveslot=%d borg_fa=%d game_fa=%d game_known_fa=%d\n",
            gs, gs >= 0 && of_has(borg_items[gs].flags, OF_FREE_ACT) ? 1 : 0,
            probe && of_has(probe->flags, OF_FREE_ACT) ? 1 : 0,
            probe && probe->known && of_has(probe->known->flags, OF_FREE_ACT) ? 1 : 0);
    }
    borg.status.active = true;
    borg_update_entrypoint(true);
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
    const char *mode = getenv("GL205_MODE");
    if (!mode) mode = "glove-fa";
    printf("MODE %s\n", mode);

    /* The production armour-swap scan. */
    borg_notice_armour_swap();
    printf("SWAPS armour_swap=%d value=%d free_act=%d\n",
        armour_swap, (int)armour_swap_value, armour_swap_free_act ? 1 : 0);
    printf("OUTCOME armour_swap=%d free_act=%d\n",
        armour_swap ? 1 : 0, armour_swap_free_act ? 1 : 0);
    if (streq(mode, "glove-fa")) {
        /* The omission case: free-action gauntlets in the pack must be
         * chosen as the armour swap when the Borg has no free action. The
         * original never considers gloves and fails this assertion. */
        require(armour_swap != 0);
        eq(armour_swap_free_act ? 1 : 0, 1);
    } else {
        /* Controls: a plain glove is not worth a swap, and a carried
         * free-action glove is not a swap when free action is already
         * worn. Both variants agree. */
        require(armour_swap == 0);
        require(armour_swap_value < 1000);
    }
    ok;
}
const char *suite_name = "native-c/gloves-205";
struct test tests[] = {{"gloves_eligible_for_armour_swap", scenario}, {NULL, NULL}};
