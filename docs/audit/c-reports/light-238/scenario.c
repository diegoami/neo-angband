/* Synthetic native C light-trait diagnostic, excluded from the source fix. */
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

int setup_tests(void **state)
{
    const char *profile = getenv("T238_PROFILE");
    const char *mode = getenv("T238_MODE");
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
    if (!player_make_simple("Human", "Warrior", "LightTwice")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    screen.offset_x = screen.offset_y = 0;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    player_place(cave, player, loc(11, 11));
    player->chp = player->mhp; player->csp = player->msp;
    if (!mode) mode = "phial";
    if (streq(mode, "phial")) {
        /* An artifact light: the Phial of Galadriel carries an actual
         * LIGHT[4] modifier (stock lights use the LIGHT_2/3 flags). */
        struct object *phial = known_item(TV_LIGHT, "Phial", 1);
        struct artifact *art = NULL;
        for (int i = 0; i < z_info->a_max; i++) {
            if (a_info[i].name && strstr(a_info[i].name, "Galadriel")) {
                art = &a_info[i];
                break;
            }
        }
        if (!art) { printf("SETUPFAIL artifact\n"); return 1; }
        phial->artifact = art;
        copy_artifact_data(phial, art);
        object_touch(player, phial);
        player_know_object(player, phial);
        inven_wield(phial, wield_slot(phial));
    } else {
        /* Control: a stock Wooden Torch - its radius comes from the
         * LIGHT_2 flag, not a modifier, so both variants agree. */
        struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
        torch->timeout = 2000;
        inven_wield(torch, wield_slot(torch));
    }
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
    printf("SETUP seed=1000 class=%s mode=%s wielded=%s\n", player->class->name,
        mode, player->body.slots[slot_by_name(player, "light")].obj ? "yes" : "no");
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    const char *mode = getenv("T238_MODE");
    if (!mode) mode = "torch";
    printf("MODE %s\n", mode);

    /* The traits were recomputed from the equipment by the setup's
     * notice/update pass. */
    int light = borg.trait[BI_LIGHT];
    struct object *worn = player->body.slots[slot_by_name(player, "light")].obj;
    int mod = worn ? worn->modifiers[OBJ_MOD_LIGHT] : 0;
    printf("GEOMETRY light_mod=%d bi_light=%d\n", mod, light);
    printf("RATING bi_light=%d\n", light);
    printf("OUTCOME bi_light=%d\n", light);
    if (streq(mode, "phial")) {
        /* Correct contract: the Phial's LIGHT[4] modifier counts once. */
        eq(light, 4);
    } else {
        /* Control: the torch's flag-based light is identical either way. */
        eq(light, 2);
    }
    ok;
}
const char *suite_name = "native-c/light-238";
struct test tests[] = {{"light_modifier_counts_once", scenario}, {NULL, NULL}};
