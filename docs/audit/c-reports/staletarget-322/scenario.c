/* Synthetic native C stale-target diagnostic, excluded from the source fix. */
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
static struct monster *foe;

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
    return 0;
}

int setup_tests(void **state)
{
    const char *profile = getenv("T322_PROFILE");
    const char *mode = getenv("T322_MODE");
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
    if (!player_make_simple("Human", "Warrior", "StaleTarget")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    struct object *sword = known_item(TV_SWORD, "Broad Sword", 1);
    inven_wield(sword, wield_slot(sword));
    player_place(cave, player, loc(11, 11));
    player->chp = player->mhp; player->csp = player->msp;
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    /* A kobold one grid south of the player (loc takes x, y): a
     * legitimate adjacent target, direction 2. */
    foe = t_add_monster(cave, loc(11, 12), "Kobold");
    if (!foe) { printf("SETUPFAIL foe\n"); return 1; }
    if (observe(foe, "foe")) return 1;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=1000 class=%s level=%d depth=%d hp=%d/%d mode=%s\n",
        player->class->name, player->lev, player->depth, player->chp,
        player->mhp, mode ? mode : "stale");
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

static int scenario(void *state)
{
    const char *mode = getenv("T322_MODE");
    keycode_t keys[64];
    int len;
    if (!mode) mode = "stale";
    printf("MODE %s\n", mode);

    int k = borg_grids[foe->grid.y][foe->grid.x].kill;
    require(k > 0);
    if (streq(mode, "stale")) {
        /* Teleport-and-blindness artifact: the tracked monster's entry
         * still points at the grid the player now occupies. */
        borg_grids[foe->grid.y][foe->grid.x].kill = 0;
        borg_kills[k].pos = borg.c;
        borg_grids[borg.c.y][borg.c.x].kill = k;
    }
    printf("GEOMETRY target=(%d,%d) player=(%d,%d) dist=%d\n",
        borg_kills[k].pos.y, borg_kills[k].pos.x, borg.c.y, borg.c.x,
        (int)distance(borg.c, borg_kills[k].pos));

    /* The production attack chooser, acting mode: it fills its own
     * target list and either attacks or finds nothing to do. */
    bool acted = borg_attack(false);
    len = drain_keys(keys, 64);
    char keybuf[64];
    for (int i = 0; i < len; ++i) keybuf[i] = (char)keys[i];
    keybuf[len] = 0;
    int center = strchr(keybuf, '5') != NULL;
    printf("RATING acted=%d keys=\"%s\" center_attack=%d\n", acted ? 1 : 0,
        keybuf, center);
    printf("OUTCOME acted=%d keys=\"%s\" center_attack=%d\n", acted ? 1 : 0,
        keybuf, center);
    if (streq(mode, "stale")) {
        /* Correct contract: a monster tracked on the player's own grid
         * is stale, not a melee target; nothing may be queued (the '5'
         * key is the center square, whose empty prompt trips the
         * unexpected-direction abort). */
        eq(acted ? 1 : 0, 0);
        eq(len, 0);
    } else {
        /* Control: the real adjacent monster is attacked identically
         * in both variants. */
        eq(acted ? 1 : 0, 1);
        eq(len, 2);
        eq(keybuf[0], '+');
        eq(keybuf[1], '2');
    }
    ok;
}
const char *suite_name = "native-c/staletarget-322";
struct test tests[] = {{"stale_own_grid_is_not_a_melee_target", scenario}, {NULL, NULL}};
