/* Synthetic native C glow-array overflow diagnostic, excluded from the source fix. */
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
#include "borg/borg-cave-light.h"
#include "borg/borg-danger.h"
#include "borg/borg-fight-attack.h"
#include "borg/borg-fight-defend.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-flow-misc.h"
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

int setup_tests(void **state)
{
    const char *profile = getenv("T311_PROFILE");
    const char *mode = getenv("T311_MODE");
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
    if (!player_make_simple("Human", "Warrior", "GlowOverflow")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 55, 42, 256); Term_activate(&screen); angband_term[0] = &screen;
    screen.offset_x = screen.offset_y = 0;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    player_place(cave, player, loc(11, 11));
    player->chp = player->mhp; player->csp = player->msp;
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    /* The whole arena is perma-lit floor in the Borg's map: the glow
     * scan collects every one of them with no bound.  The panel covers
     * the full 40x40 arena. */
    int glow_limit = (mode && streq(mode, "control")) ? 1000 : 40 * 40;
    int marked = 0;
    w_x = w_y = 0;
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
        if (marked < glow_limit) {
            borg_grids[y][x].info |= BORG_GLOW;
            marked++;
        }
    }
    update_view(cave, player);
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=1000 glowing=%d limit=%d panel=%dx%d\n", marked,
        (int)AUTO_LIGHT_MAX, (int)SCREEN_WID, (int)SCREEN_HGT);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    const char *mode = getenv("T311_MODE");
    if (!mode) mode = "glow";
    printf("MODE %s\n", mode);

    /* The production out-of-light flow: it rebuilds the glow arrays
     * from the panel, then flows. */
    (void)borg_flow_light(GOAL_FLEE);

    int over = borg_glow_n - (int)AUTO_LIGHT_MAX;
    printf("GEOMETRY glow_n=%d limit=%d\n", borg_glow_n, (int)AUTO_LIGHT_MAX);
    printf("RATING glow_n=%d overflow=%d\n", borg_glow_n, over > 0 ? over : 0);
    printf("OUTCOME glow_n=%d overflow=%d\n", borg_glow_n, over > 0 ? over : 0);
    if (streq(mode, "glow")) {
        /* Correct contract: the glow arrays never hold more entries
         * than they were sized for. */
        require(borg_glow_n <= (int)AUTO_LIGHT_MAX);
    } else {
        /* Control: a thousand glowing grids fit in both variants. */
        eq(borg_glow_n, 1000);
    }
    ok;
}
const char *suite_name = "native-c/glowoverflow-311";
struct test tests[] = {{"glow_arrays_are_bounded", scenario}, {NULL, NULL}};
