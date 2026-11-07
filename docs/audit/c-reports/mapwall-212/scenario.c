/* Synthetic native C map wall-change flow diagnostic, excluded from the source fix. */
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
#include "borg/borg-flow.h"
#include "borg/borg-update.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-trait.h"
#include <stdio.h>
#include <stdlib.h>

static term screen;
#define GX 13
#define GY 11

int setup_tests(void **state)
{
    const char *profile = getenv("MW212_PROFILE");
    const char *mode = getenv("MW212_MODE");
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
    if (!player_make_simple("Human", "Warrior", "MapWallChange")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    if (mode && streq(mode, "door-open")) {
        square_set_feat(cave, loc(GX, GY), FEAT_CLOSED);
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
    update_view(cave, player);
    borg.status.active = true;
    borg_update_entrypoint(true);
    /* First map pass: the borg records the grid as it is now. */
    borg_update();
    /* Fixture intervention: state a flow routine and a stone-to-mud
     * detection would have left behind on this grid. */
    borg_data_flow->data[GY][GX] = 50;
    borg_data_know->data[GY][GX] = true;
    borg_data_icky->data[GY][GX] = true;
    borg.time.wizard_light = 1;
    borg.time.detect_walls = 1;
    printf("SETUP pass1 feat=%d flow=%d know=%d icky=%d\n",
        borg_grids[GY][GX].feat, borg_data_flow->data[GY][GX],
        borg_data_know->data[GY][GX] ? 1 : 0, borg_data_icky->data[GY][GX] ? 1 : 0);
    if (!mode || streq(mode, "door-close")) {
        /* The door on the grid closes between the passes; the second
         * map pass sees it through map_info and overwrites the grid. */
        square_set_feat(cave, loc(GX, GY), FEAT_CLOSED);
    } else if (streq(mode, "door-open")) {
        /* The closed door is opened between the passes. */
        square_set_feat(cave, loc(GX, GY), FEAT_FLOOR);
    }
    printf("SETUP seed=1000 class=%s level=%d depth=%d changed=%d\n",
        player->class->name, player->lev, player->depth,
        (mode && !streq(mode, "no-change")) ? 1 : 1);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    const char *mode = getenv("MW212_MODE");
    if (!mode) mode = "door-close";
    printf("MODE %s\n", mode);

    /* Second map pass: the borg sees the changed grid. */
    borg_update();
    int flow = borg_data_flow->data[GY][GX];
    int know = borg_data_know->data[GY][GX] ? 1 : 0;
    int icky = borg_data_icky->data[GY][GX] ? 1 : 0;
    printf("STATE flow=%d know=%d icky=%d wizard_light=%d detect_walls=%d\n",
        flow, know, icky, (int)borg.time.wizard_light, (int)borg.time.detect_walls);
    printf("OUTCOME invalidated=%d flow255=%d\n",
        (know == 0 && icky == 0) ? 1 : 0, flow == 255 ? 1 : 0);
    if (streq(mode, "door-close")) {
        /* The misgate case: a floor grid turns into a closed door. The
         * original takes old_wall after overwriting the feature, sees no
         * change and keeps the stale flow; the correction invalidates the
         * flow and the stamps. */
        require(flow == 255);
        require(know == 0);
        require(icky == 0);
        require(borg.time.wizard_light == 0);
        require(borg.time.detect_walls == 0);
    } else if (streq(mode, "door-open")) {
        /* The reverse change: a closed door opens. The flow value stays
         * (only new walls get 255) but knowledge and the stamps are
         * cleared. The original keeps everything stale. */
        require(flow != 255);
        require(know == 0);
        require(icky == 0);
        require(borg.time.wizard_light == 0);
        require(borg.time.detect_walls == 0);
    } else {
        /* Control: the grid is unchanged; nothing is invalidated in
         * either variant. Complete logs are byte-identical. */
        require(flow == 50);
        require(know == 1);
        require(icky == 1);
    }
    ok;
}
const char *suite_name = "native-c/mapwall-212";
struct test tests[] = {{"wall_change_invalidates_flow", scenario}, {NULL, NULL}};
