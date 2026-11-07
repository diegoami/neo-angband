/* Synthetic native C dying-light diagnostic, excluded from the source fix. */
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
#include "borg/borg-item-val.h"
#include "borg/borg-think-dungeon-util.h"
#include "borg/borg-trait.h"
#include <stdio.h>
#include <stdlib.h>

static term screen;

static struct object *make_item(int tval, const char *name, int number, int timeout, bool no_fuel)
{
    struct object *obj = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) quit("fixture item kind missing");
    object_prep(obj, kind, 50, MAXIMISE);
    obj->number = number;
    obj->timeout = timeout;
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    if (no_fuel) of_on(obj->flags, OF_NO_FUEL);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
    if (no_fuel) of_on(obj->known->flags, OF_NO_FUEL);
    return obj;
}

int setup_tests(void **state)
{
    const char *profile = getenv("OL229_PROFILE");
    const char *mode = getenv("OL229_MODE");
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
    if (!player_make_simple("Human", "Warrior", "DyingLight")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *lantern;
    if (!mode || streq(mode, "dying")) {
        lantern = make_item(TV_LIGHT, "Lantern", 1, 3, false);
    } else if (streq(mode, "lit")) {
        lantern = make_item(TV_LIGHT, "Lantern", 1, 1500, false);
    } else if (streq(mode, "everburn")) {
        lantern = make_item(TV_LIGHT, "Lantern", 1, 0, true);
    } else {
        lantern = make_item(TV_LIGHT, "Lantern", 1, 0, false);
    }
    inven_wield(lantern, wield_slot(lantern));
    calc_inventory(player);
    make_item(TV_FLASK, "Flask of Oil", 1, 0, false);
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
    printf("SETUP light=%d timeout=%d nofuel=%d flasks=%d\n",
        borg.trait[BI_LIGHT], borg_items[INVEN_LIGHT].timeout,
        of_has(borg_items[INVEN_LIGHT].flags, OF_NO_FUEL) ? 1 : 0,
        borg_slot(TV_FLASK, sv_flask_oil));
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

static int drain_keys(keycode_t *keys, int max)
{
    int len = 0; keycode_t key;
    while ((key = borg_inkey(true)) != 0 && len < max) keys[len++] = key;
    return len;
}

static void print_keys(const keycode_t *keys, int len)
{
    for (int i = 0; i < len; ++i) {
        if (i) putchar(' ');
        if (keys[i] == ESCAPE) printf("<Escape>"); else printf("%c", (char)keys[i]);
    }
    putchar('\n');
}

static int scenario(void *state)
{
    const char *mode = getenv("OL229_MODE");
    keycode_t keys[64];
    int len;
    if (!mode) mode = "dying";
    printf("MODE %s\n", mode);

    /* The production out-of-light maneuver routine. */
    bool acted = borg_think_dungeon_light();
    len = drain_keys(keys, 64);
    printf("DECISION acted=%d keys=%d sequence=", acted, len);
    print_keys(keys, len);
    printf("OUTCOME acted=%d keys=%d\n", acted ? 1 : 0, len);
    if (streq(mode, "dying")) {
        /* The misgate case: the lantern is lit (radius 2) but about to
         * burn out (timeout 3) and a flask of oil is carried. The
         * original's entry test is already true in its first clause, so
         * the about-to-burn-out clause never fires and the routine does
         * nothing. The corrected test refuels. */
        eq(acted ? 1 : 0, 1);
        require(len > 0);
    } else if (streq(mode, "lit")) {
        /* Control: plenty of fuel; both variants skip. Complete logs are
         * byte-identical. */
        eq(acted ? 1 : 0, 0);
        require(len == 0);
    } else if (streq(mode, "everburn")) {
        /* Control: a fuel-less (everburning-style) light is never
         * "about to burn out"; both variants skip it. Complete logs are
         * byte-identical. */
        eq(acted ? 1 : 0, 0);
        require(len == 0);
    } else {
        /* Control: the lantern is fully burned out; both variants run
         * the same maneuvers. Complete logs are byte-identical. */
        eq(acted ? 1 : 0, 1);
        require(len > 0);
    }
    ok;
}
const char *suite_name = "native-c/outoflight-229";
struct test tests[] = {{"dying_light_starts_maneuvers", scenario}, {NULL, NULL}};
