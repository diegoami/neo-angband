/* Synthetic native C recall-depth answer diagnostic, excluded from the source fix. */
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
#include "borg/borg-escape.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-item-use.h"
#include "borg/borg-item-val.h"
#include "borg/borg-prepared.h"
#include "borg/borg-trait.h"
#include <stdio.h>
#include <stdlib.h>

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
    const char *profile = getenv("RD310_PROFILE");
    const char *mode = getenv("RD310_MODE");
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
    if (!player_make_simple("Human", "Warrior", "RecallDepthAnswer")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    known_item(TV_SCROLL, "Word of Recall", 1);
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
    /* The endgame shape: 99 is the deepest level reached, fewer than five
     * uniques still live, and the borg recalls from a depth near the one
     * it is hunting. */
    borg.trait[BI_MAXDEPTH] = 99;
    borg.trait[BI_CDEPTH] = 65;
    borg_depth_hunted_unique = 65;
    borg_numb_live_unique = 2;
    if (mode && streq(mode, "generic")) {
        /* Same geometry, but the deepest level reached is 60, not 99: the
         * generic reset branch, untouched by the correction. */
        borg.trait[BI_MAXDEPTH] = 60;
        borg.trait[BI_CDEPTH] = 30;
        borg_depth_hunted_unique = 30;
    }
    if (mode && streq(mode, "mismatch")) {
        /* The hunted unique is far from the current depth: the branch that
         * declines to touch recall depth at all, untouched by the
         * correction. */
        borg_depth_hunted_unique = 30;
    }
    printf("SETUP maxdepth=%d curdepth=%d hunted=%d live_uniques=%d\n",
        borg.trait[BI_MAXDEPTH], borg.trait[BI_CDEPTH],
        borg_depth_hunted_unique, borg_numb_live_unique);
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=1000 class=%s level=%d depth=%d hp=%d/%d scroll_slot=%d\n",
        player->class->name, player->lev, player->depth, player->chp, player->mhp,
        borg_slot(TV_SCROLL, sv_scroll_word_of_recall));
    if (borg.goal.recalling) { printf("SETUPFAIL recalling already set\n"); return 1; }
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
    const char *mode = getenv("RD310_MODE");
    keycode_t keys[64];
    int len;
    if (!mode) mode = "endgame";
    printf("MODE %s\n", mode);

    /* The production recall attempt: it reads the scroll, then answers
     * the game's "Set recall depth to current depth?" prompt. */
    bool acted = borg_recall();
    len = drain_keys(keys, 64);
    printf("DECISION acted=%d keys=%d sequence=", acted, len);
    print_keys(keys, len);
    int reset_y = len >= 6 && keys[2] == 'r' && keys[4] == 'y' && keys[5] == ESCAPE;
    int keep_n = len >= 6 && keys[2] == 'r' && keys[4] == 'n' && keys[5] == ESCAPE;
    printf("OUTCOME acted=%d keys=%d reset_y=%d keep_n=%d\n",
        acted, len, reset_y ? 1 : 0, keep_n ? 1 : 0);
    eq(acted ? 1 : 0, 1);
    require(len == 6);
    if (streq(mode, "endgame")) {
        /* The misanswer case: the branch's comment says to reset depth
         * (cleaning up the last uniques for the Morgoth push); the
         * corrected code answers 'y'. The original answers 'n' and fails
         * this assertion. */
        eq(reset_y ? 1 : 0, 1);
        require(!keep_n);
    } else if (streq(mode, "generic")) {
        /* Control: the generic reset branch answers 'y' in both variants.
         * Complete logs are byte-identical. */
        eq(reset_y ? 1 : 0, 1);
    } else {
        /* Control: the hunted-depth mismatch branch answers 'n' in both
         * variants. Complete logs are byte-identical. */
        eq(keep_n ? 1 : 0, 1);
        require(!reset_y);
    }
    ok;
}
const char *suite_name = "native-c/recalldepth-310";
struct test tests[] = {{"endgame_branch_resets_recall_depth", scenario}, {NULL, NULL}};
