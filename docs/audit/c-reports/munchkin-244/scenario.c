/* Synthetic native C munchkin stair-grid diagnostic, excluded from the source fix. */
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
#include "borg/borg-danger.h"
#include "borg/borg-fight-attack.h"
#include "borg/borg-fight-defend.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-flow-stairs.h"
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
#include <stdio.h>
#include <stdlib.h>

static term screen;

int setup_tests(void **state)
{
    const char *profile = getenv("M244_PROFILE");
    const char *mode = getenv("M244_MODE");
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
    /* Munchkin mode is dispatched by borg_think_dungeon() only for a young
     * borg (max character level under 12) with the config enabled, so the
     * fixture keeps the level it was born with.  A warrior carries no
     * spellbook, which keeps the spell practice stages quiet. */
    if (!player_make_simple("Human", "Warrior", "MunchkinStair")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* Wield the birth lights so the Borg has light, plenty of fuel and
     * nothing left in the pack that it would want to wear instead. */
    int wielded = 0;
    for (int pass = 0; pass < 4; pass++) {
        for (struct object *obj = player->gear; obj; obj = obj->next) {
            if (obj->tval != TV_LIGHT) continue;
            if (player->body.slots[wield_slot(obj)].obj == obj) continue;
            inven_wield(obj, wield_slot(obj));
            wielded++;
            break;
        }
    }
    if (!wielded) { printf("SETUPFAIL light\n"); return 1; }
    player_place(cave, player, loc(11, 11));
    player_learn_all_runes(player); calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA | PU_SPELLS;
    update_stuff(player);
    player->chp = player->mhp; player->csp = player->msp;
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    if (!borg_cfg[BORG_MUNCHKIN_START]) { printf("SETUPFAIL munchkin_start config\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    /* The real turn loop publishes the Borg's power each turn; the direct
     * call into the dungeon brain below skips that, so do it here. */
    borg.power = borg_power();
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    /* A down stair the Borg has seen, sealed inside a permanent-wall pocket
     * so no flow can reach it: it satisfies the "stairs have been seen" gate
     * while every stair flow must fail, which is what brings the code to the
     * take-the-stair tests after the adjacent-monster scan. */
    for (int y = 29; y <= 31; ++y) for (int x = 29; x <= 31; ++x) {
        if (x == 30 && y == 30) continue;
        square_set_feat(cave, loc(x, y), FEAT_PERM);
        borg_grids[y][x].feat = FEAT_PERM;
    }
    square_set_feat(cave, loc(30, 30), FEAT_MORE);
    borg_grids[30][30].feat = FEAT_MORE;
    track_more.x[0] = 30; track_more.y[0] = 30; track_more.num = 1;
    /* Control: the Borg standing on a real, seen down staircase; the
     * power-diving section takes it correctly before the scan runs. */
    if (mode && streq(mode, "on-stair")) {
        square_set_feat(cave, loc(11, 11), FEAT_MORE);
        borg_grids[11][11].feat = FEAT_MORE;
        track_more.x[1] = 11; track_more.y[1] = 11; track_more.num = 2;
    }
    /* A down stair the Borg remembers one diagonal step north-west of
     * itself, on the grid the adjacent-monster scan examines last, but
     * which is not in the tracked list.  The Borg stands on plain floor. */
    borg_grids[10][10].feat = FEAT_MORE;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=1000 class=%s level=%d depth=%d hp=%d/%d track_more=%d\n",
        player->class->name, player->lev, player->depth, player->chp, player->mhp,
        track_more.num);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    const char *mode = getenv("M244_MODE");
    if (!mode) mode = "phantom";
    printf("MODE %s\n", mode);
    printf("GEOMETRY player=(%d,%d) own_feat=%d nw_feat=%d far_stair=(30,30) track_more=%d\n",
        borg.c.x, borg.c.y, borg_grids[borg.c.y][borg.c.x].feat,
        borg_grids[borg.c.y - 1][borg.c.x - 1].feat, track_more.num);

    /* The public dungeon brain; its munchkin dispatch runs for a young
     * borg with the config enabled. */
    borg_flush();
    int acted = borg_think_dungeon();

    /* Collect every keypress the decision queued. */
    char keys[64];
    int n = 0;
    keycode_t k;
    while ((k = borg_inkey(true)) != 0 && n < 60)
        keys[n++] = (char)k;
    keys[n] = 0;
    int stair_key = 0;
    for (int i = 0; i < n; i++)
        if (keys[i] == '<' || keys[i] == '>') stair_key = keys[i];
    int own_more = borg_grids[borg.c.y][borg.c.x].feat == FEAT_MORE;
    int own_less = borg_grids[borg.c.y][borg.c.x].feat == FEAT_LESS;
    printf("RATING acted=%d keys=\"%s\" stair_key=%d\n", acted, keys, stair_key);
    printf("OUTCOME acted=%d stair_key=%d own_is_stair=%d\n",
        acted, stair_key, own_more || own_less);
    /* Correct contract: a stair keypress is only queued when the Borg
     * stands on a staircase. */
    require(!stair_key || own_more || own_less);
    ok;
}
const char *suite_name = "native-c/munchkin-244";
struct test tests[] = {{"stair_key_only_on_own_stair", scenario}, {NULL, NULL}};
