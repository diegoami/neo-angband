/* Synthetic native C acid-spray arc edge diagnostic, excluded from the source fix. */
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
#include "borg/borg-flow-kill.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-magic.h"
#include "borg/borg-think.h"
#include "borg/borg-trait.h"
#include <stdio.h>
#include <stdlib.h>

static term screen;
static struct monster *giant;

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
    if (borg_kills[k].power <= 0) { printf("SETUPFAIL %s power\n", tag); return 1; }
    printf("SETUP %s kill=%d power=%d\n", tag, k, borg_kills[k].power);
    return 0;
}

int setup_tests(void **state)
{
    const char *profile = getenv("AC252_PROFILE");
    const char *mode = getenv("AC252_MODE");
    const char *seedenv = getenv("AC252_SEED");
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
    Rand_quick = false; Rand_state_init(seedenv ? (uint32_t)atoi(seedenv) : 1000);
    if (!player_make_simple("Human", "Mage", "AcidSprayArcEdge")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    known_item(TV_MAGIC_BOOK, "[Attacks and Knowledge]", 1);
    player_place(cave, player, loc(11, 11));
    player_exp_gain(player, player_exp[49] * player->expfact / 100);
    while (player_stat_inc(player, STAT_INT)) {}
    player_learn_all_runes(player); calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA | PU_SPELLS;
    update_stuff(player);
    for (int i = 0; i < player->class->magic.total_spells; ++i) {
        const char *name = spell_by_index(player, i)->name;
        if (streq(name, "Acid Spray")) spell_learn(i);
    }
    player->chp = player->mhp; player->csp = player->msp;
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
    /* Acid Spray costs 5: 20 mana lets it through and blocks Frost Bolt
     * (40), keeping the spray the only live attack. */
    borg.trait[BI_MAXSP] = 20;
    borg.trait[BI_CURSP] = 20;
    int dist = 10;
    if (mode && streq(mode, "near")) dist = 8;
    printf("SETUP maxsp=%d csp=%d failacid=%d poweracid=%d dist=%d\n",
        borg.trait[BI_MAXSP], borg.trait[BI_CURSP],
        borg_spell_fail_rate(ACID_SPRAY), borg_get_spell_power(ACID_SPRAY), dist);
    if (borg_spell_fail_rate(ACID_SPRAY) > 25) { printf("SETUPFAIL acid fail rate too high\n"); return 1; }
    if (!mode || streq(mode, "edge") || streq(mode, "near")) {
        giant = t_add_monster(cave, loc(11, 11 + dist), "hill giant");
        if (!giant) { printf("SETUPFAIL giant\n"); return 1; }
        if (observe(giant, "giant")) return 1;
    }
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=%s class=%s level=%d depth=%d hp=%d/%d sp=%d/%d\n",
        seedenv ? seedenv : "1000", player->class->name, player->lev, player->depth,
        player->chp, player->mhp, player->csp, player->msp);
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
    const char *mode = getenv("AC252_MODE");
    keycode_t keys[64];
    int len;
    if (!mode) mode = "edge";
    printf("MODE %s\n", mode);

    /* The production attack chooser, acting mode: Acid Spray is the only
     * live attack, aimed at the giant south of the Borg. */
    bool acted = borg_attack(false);
    len = drain_keys(keys, 64);
    printf("DECISION acted=%d keys=%d sequence=", acted, len);
    print_keys(keys, len);
    int cast = 0;
    for (int i = 0; i + 2 < len; ++i)
        if (keys[i] == 'm' && keys[i + 1] == 'b' && keys[i + 2] == 'f')
            cast = 1;
    printf("OUTCOME acted=%d keys=%d cast=%d\n", acted, len, cast);
    if (streq(mode, "edge")) {
        /* The edge case: the giant stands exactly max_range grids south,
         * on the blast loop's excluded south edge. The original never
         * counts it and finds nothing worth casting; the correction
         * counts it and casts. */
        eq(acted ? 1 : 0, 1);
        eq(cast, 1);
    } else if (streq(mode, "near")) {
        /* Control: the giant eight grids south is inside the loop in both
         * variants. Complete logs are byte-identical. */
        eq(acted ? 1 : 0, 1);
        eq(cast, 1);
    } else {
        /* Control: no monster at all; both variants find nothing to do.
         * Complete logs are byte-identical. */
        eq(acted ? 1 : 0, 0);
        require(len == 0);
    }
    ok;
}
const char *suite_name = "native-c/acidarc-252";
struct test tests[] = {{"arc_counts_radius_edge", scenario}, {NULL, NULL}};
