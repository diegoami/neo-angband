/* Synthetic native C lunal-exit diagnostic, excluded from the source fix. */
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
#include "borg/borg-escape.h"
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

int setup_tests(void **state)
{
    const char *profile = getenv("T314_PROFILE");
    const char *mode = getenv("T314_MODE");
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
    if (!player_make_simple("Human", "Warrior", "LunalExit")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    screen.offset_x = screen.offset_y = 0;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    if (!mode) mode = "town";
    /* The bail fires in town (or when weak); the control runs the
     * dungeon case with a full pack, which returns before any flag is
     * touched. */
    player->depth = player->max_depth = cave->depth = player->cave->depth
        = streq(mode, "town") ? 0 : 2;
    character_dungeon = true;
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
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=1000 class=%s depth=%d\n", player->class->name,
        player->depth);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    const char *mode = getenv("T314_MODE");
    if (!mode) mode = "town";
    printf("MODE %s\n", mode);

    /* The Borg is in lunal mode, both halves of the state set. */
    borg.lunal_mode = true;
    borg.goal.fleeing_lunal = true;

    if (!streq(mode, "town")) {
        /* Control: a full pack bails out before any flag is touched. */
        borg_items[PACK_SLOTS - 1].iqty = 1;
    }

    (void)borg_think_stair_scum();

    int pair_intact = borg.lunal_mode && borg.goal.fleeing_lunal;
    int mismatch = !borg.lunal_mode && borg.goal.fleeing_lunal;
    printf("GEOMETRY lunal_mode=%d fleeing_lunal=%d\n",
        borg.lunal_mode ? 1 : 0, borg.goal.fleeing_lunal ? 1 : 0);
    printf("RATING lunal_mode=%d fleeing_lunal=%d\n",
        borg.lunal_mode ? 1 : 0, borg.goal.fleeing_lunal ? 1 : 0);
    printf("OUTCOME lunal_mode=%d fleeing_lunal=%d\n",
        borg.lunal_mode ? 1 : 0, borg.goal.fleeing_lunal ? 1 : 0);
    if (streq(mode, "town")) {
        /* Correct contract: leaving the mode clears both halves. */
        require(!borg.lunal_mode);
        require(!borg.goal.fleeing_lunal);
    } else {
        /* Control: the full-pack bail touches no flags. */
        require(pair_intact);
        require(!mismatch);
    }
    ok;
}
const char *suite_name = "native-c/lunal-314";
struct test tests[] = {{"leaving_scumming_clears_the_lunal_pair", scenario}, {NULL, NULL}};
