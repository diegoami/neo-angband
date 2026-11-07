/* Synthetic native C missile-launch diagnostic, excluded from the source fix. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "cmd-core.h"
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
#include "borg/borg-fight-defend.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-flow.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item-use.h"
#include "borg/borg-item.h"
#include "borg/borg-magic.h"
#include "borg/borg-think-dungeon-util.h"
#include "borg/borg-think.h"
#include "borg/borg-trait.h"
#include <stdio.h>
#include <stdlib.h>

static term screen;
static struct monster *enemy;

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
    const char *profile = getenv("M254_PROFILE");
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
    if (!player_make_simple("Human", "Warrior", "MissileNoTarget")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 10;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    player_place(cave, player, loc(11, 11));
    /* A bow and a quiver of arrows: the launch path is armed. */
    struct object *bow = known_item(TV_BOW, "Short Bow", 1);
    inven_wield(bow, wield_slot(bow));
    known_item(TV_ARROW, "Arrow", 20);
    player_exp_gain(player, player_exp[9] * player->expfact / 100);
    player_learn_all_runes(player); calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA | PU_SPELLS;
    update_stuff(player);
    player->chp = player->mhp;
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_do_spell = true; borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg_cheat_spells();
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    /* A known monster the Borg would like to shoot. */
    enemy = t_add_monster(cave, loc(11, 16), "Kobold");
    if (!enemy) { printf("SETUPFAIL enemy\n"); return 1; }
    mflag_on(enemy->mflag, MFLAG_AWARE); update_mon(enemy, cave, true);
    if (!mflag_has(enemy->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL visible\n"); return 1; }
    if (!observe_kill_diff(enemy->grid.y, enemy->grid.x, 0, 0)) { printf("SETUPFAIL observe\n"); return 1; }
    int kill_index = borg_grids[enemy->grid.y][enemy->grid.x].kill;
    if (kill_index <= 0) { printf("SETUPFAIL kill_index\n"); return 1; }
    borg_kills[kill_index].awake = 1;
    borg.status.active = true;
    borg_update_entrypoint(true);
    for (int i = QUIVER_START; i < QUIVER_END; ++i)
        if (borg_items[i].iqty)
            printf("QUIVER slot=%d letter_index=%d tval=%d sval=%d n=%d\n", i,
                i - QUIVER_START, borg_items[i].tval, borg_items[i].sval,
                borg_items[i].iqty);
    printf("SETUP seed=1000 class=%s level=%d depth=%d hp=%d/%d ammo_tval=%d bow_iqty=%d\n",
        player->class->name, player->lev, player->depth, player->chp, player->mhp,
        borg.trait[BI_AMMO_TVAL], borg_items[INVEN_BOW].iqty);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static void fill_temp_like_attack(void)
{
    borg_grid *ag;
    borg_temp_n = 0;
    for (int i = 1; i < borg_kills_nxt; i++) {
        borg_kill *kill = &borg_kills[i];
        if (!kill->r_idx) continue;
        if (borg_timer(kill->when) > 2) continue;
        ag = &borg_grids[kill->pos.y][kill->pos.x];
        if (!(ag->info & BORG_OKAY)) continue;
        if (!(ag->info & BORG_VIEW)) continue;
        if ((ag->feat >= FEAT_RUBBLE) && (ag->feat <= FEAT_PERM)) continue;
        if (distance(borg.c, kill->pos) > z_info->max_range) continue;
        borg_temp_x[borg_temp_n] = kill->pos.x;
        borg_temp_y[borg_temp_n] = kill->pos.y;
        borg_temp_n++;
    }
}

static int drain_keys(keycode_t *keys, int max)
{
    int len = 0; keycode_t key;
    while ((key = borg_inkey(true)) != 0 && len < max) keys[len++] = key;
    return len;
}

static int scenario(void *state)
{
    const char *mode = getenv("M254_MODE");
    keycode_t keys[64];
    int len;
    if (!mode) mode = "no-target";
    printf("MODE %s\n", mode);

    if (streq(mode, "no-target")) {
        /* The production state left by the shot-lining flow: the previous
         * evaluation (for example the Teleport Other defense) reset the
         * target list, and the simulation flag is off after any acted
         * attack or defense, or by its initial value. */
        borg_temp_n = 0;
        borg_simulate = false;
        int n = borg_attack_aux_launch();
        len = drain_keys(keys, 64);
        printf("DECISION no-target return=%d keys=%d sequence=", n, len);
        for (int i = 0; i < len; ++i) {
            if (keys[i] == ESCAPE) printf("<Escape>"); else printf("%c", (char)keys[i]);
        }
        putchar('\n');
        printf("OUTCOME return=%d keys=%d\n", n, len);
        eq(n, 0);
        eq(len, 0);
        ok;
    }
    if (streq(mode, "live-shot")) {
        fill_temp_like_attack();
        borg_simulate = false;
        int n = borg_attack_aux_launch();
        len = drain_keys(keys, 64);
        printf("DECISION live-shot return=%d keys=%d sequence=", n, len);
        for (int i = 0; i < len; ++i) {
            if (keys[i] == ESCAPE) printf("<Escape>"); else printf("%c", (char)keys[i]);
        }
        putchar('\n');
        printf("OUTCOME return=%d keys=%d\n", n, len);
        require(n > 0);
        require(len >= 3);
        ok;
    }
    require(streq(mode, "simulated"));
    fill_temp_like_attack();
    borg_simulate = true;
    int n = borg_attack_aux_launch();
    len = drain_keys(keys, 64);
    printf("DECISION simulated return=%d keys=%d\n", n, len);
    printf("OUTCOME return=%d keys=%d\n", n, len);
    require(n > 0);
    eq(len, 0);
    ok;
}
const char *suite_name = "native-c/missile-254";
struct test tests[] = {{"launch_without_target_does_not_fire", scenario}, {NULL, NULL}};
