/* Synthetic native C queued-detection diagnostic, excluded from the source fix. */
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
#include "borg/borg-cave-view.h"
#include "borg/borg-cave-util.h"
#include "borg/borg-danger.h"
#include "borg/borg-fight-attack.h"
#include "borg/borg-fight-defend.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-flow.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-messages-react.h"
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

static term screen;

int setup_tests(void **state)
{
    const char *profile = getenv("D290_PROFILE");
    if (!profile) return 1;
    printf("SETUPSTEP profile\n");
    setvbuf(stdout, NULL, _IONBF, 0);
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(1000);
    if (!player_make_simple("Human", "Warrior", "QueuedDetect")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 80, 30, 256); Term_activate(&screen); angband_term[0] = &screen;
    /* Size the caves from the panel geometry the term gives: the big
     * cave spans several panel rows, the small one exactly one. */
    int ph = borg_panel_hgt();
    int pw = borg_panel_wid();
    printf("PANELS hgt=%d wid=%d\n", ph, pw);
    if (ph < 15 || pw < 15) { printf("SETUPFAIL panel geometry %dx%d\n", pw, ph); return 1; }
    int big = 2 * ph + 6;
    if (big < pw * 2 + 6) big = pw * 2 + 6;
    /* A big cave with the view pushed down to the second panel row. */
    cave = t_build_arena(big, big); player->cave = cave_new(big, big);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    character_dungeon = true;
    for (int y = 1; y < big - 1; ++y) for (int x = 1; x < big - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    for (int pass = 0; pass < 4; pass++) {
        for (struct object *obj = player->gear; obj; obj = obj->next) {
            if (obj->tval != TV_LIGHT) continue;
            if (player->body.slots[wield_slot(obj)].obj == obj) continue;
            inven_wield(obj, wield_slot(obj));
            break;
        }
    }
    player_place(cave, player, loc(11, 25));
    player->chp = player->mhp; player->csp = player->msp;
    borg_init();
    borg_update();
    borg_alloc_detection();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg.power = borg_power();
    for (int y = 0; y < cave->height && y < AUTO_MAX_Y; ++y)
        for (int x = 0; x < cave->width && x < AUTO_MAX_X; ++x) {
            borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
            borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
        }
    update_view(cave, player);
    /* First update with the view on the second panel row: the borg saves
     * these offsets as the old panel offsets. */
    screen.offset_x = 0; screen.offset_y = ph;
    borg_update();
    printf("SETUP seed=1000 class=%s level=%d depth=%d panels=%d,%d\n",
        player->class->name, player->lev, player->depth, borg.panels.x, borg.panels.y);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static void sync_grids(void)
{
    for (int y = 0; y < cave->height && y < AUTO_MAX_Y; ++y)
        for (int x = 0; x < cave->width && x < AUTO_MAX_X; ++x) {
            borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
            borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
        }
    update_view(cave, player);
}

static int scenario(void *state)
{
    int ph = borg_panel_hgt();
    const char *mode = getenv("D290_MODE");
    if (!mode) mode = "stale";
    printf("MODE %s\n", mode);

    /* Queue the self-reaction of a trap detection, as casting the spell
     * does, before the level change. */
    if (streq(mode, "stale"))
        borg_react("SELF:trap", "SELF:trap");
    printf("QUEUED panels=%d,%d\n", borg.panels.x, borg.panels.y);

    /* Change to a smaller cave, as a level change does, and update on
     * the top-left panel.  The reaction is still queued and keeps the
     * old offsets, one panel row past the smaller cave's arrays. */
    /* Exactly one panel high: the old-offset reset compares strictly,
     * so an offset equal to the cave height survives it, while the
     * panel count still yields a single row pair. */
    cave = t_build_arena(ph, ph);
    player->cave = cave_new(ph, ph);
    cave->depth = player->cave->depth = player->depth;
    for (int y = 1; y < ph - 1; ++y) for (int x = 1; x < ph - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    player_place(cave, player, loc(11, 11));
    borg.c = player->grid;
    sync_grids();
    screen.offset_x = 0; screen.offset_y = 0;
    borg_update();

    /* Control: queue the reaction for the new cave, when the old
     * offsets are the current ones; it is processed on the next update
     * and marks the panel legitimately. */
    if (streq(mode, "fresh")) {
        borg_react("SELF:trap", "SELF:trap");
        borg_update();
    }

    /* After the correction, the stale reaction is dropped and the small
     * cave's fresh detection stamps stay clear. */
    int stale_mark = borg_detect_trap && borg_detect_trap[1] ? borg_detect_trap[1][0] : -1;
    printf("GEOMETRY new_cave=%dx%d panels=%d,%d last_row_mark=%d\n",
        cave->width, cave->height, borg.panels.x, borg.panels.y, stale_mark);
    printf("RATING last_row_mark=%d rows=%d\n", stale_mark, borg.panels.y + 1);
    printf("OUTCOME last_row_mark=%d\n", stale_mark);
    if (streq(mode, "stale")) {
        /* Correct contract: a reaction queued for a panel of the
         * previous, larger cave marks nothing in the new one. */
        eq(stale_mark, 0);
    } else {
        /* Control: a reaction queued for the current cave marks it. */
        eq(stale_mark, 1);
    }
    ok;
}
const char *suite_name = "native-c/detect-290";
struct test tests[] = {{"queued_detection_respects_resized_arrays", scenario}, {NULL, NULL}};
