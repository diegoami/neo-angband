/* Synthetic native C Tap Unlife rating diagnostic, excluded from the source fix. */
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
static struct monster *lich, *giant, *bat;
static int lich_kill, giant_kill, bat_kill;
/* Pinned from the recorded seed-1000 runs; see README. The chosen cast keys
 * depend on the pack slot of each book; the target digits on the ray. The
 * corrected run targets the injured bat's ray: "* p 6 6 6 6 5 m c a". */
#define TAP249_AFTER_SEQ "* p 6 6 6 6 5 m c a"

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
    printf("SETUP %s kill=%d power=%d level=%d injury=%d undead=%d\n", tag, k,
        borg_kills[k].power, borg_kills[k].level, borg_kills[k].injury,
        rf_has(r_info[borg_kills[k].r_idx].flags, RF_UNDEAD) ? 1 : 0);
    return 0;
}

int setup_tests(void **state)
{
    const char *profile = getenv("TAP249_PROFILE");
    const char *mode = getenv("TAP249_MODE");
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
    if (!player_make_simple("Human", "Necromancer", "TapUnlifeRating")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* Both shadow books: Nether Bolt (Into the Shadows) and Tap Unlife
     * (Dark Rituals). No light source: a necromancer's fail rate rises by
     * 25 on a lit square and Tap Unlife must stay castable (fail <= 25). */
    known_item(TV_SHADOW_BOOK, "[Into the Shadows]", 1);
    known_item(TV_SHADOW_BOOK, "[Dark Rituals]", 1);
    player_place(cave, player, loc(11, 11));
    player_exp_gain(player, player_exp[12] * player->expfact / 100);
    while (player_stat_inc(player, STAT_INT)) {}
    player_learn_all_runes(player); calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA | PU_SPELLS;
    update_stuff(player);
    for (int i = 0; i < player->class->magic.total_spells; ++i) {
        const char *name = spell_by_index(player, i)->name;
        if (streq(name, "Nether Bolt") || streq(name, "Tap Unlife")) spell_learn(i);
    }
    player->chp = player->mhp; player->csp = player->msp;
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_do_spell = true; borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg_cheat_spells();
    /* The tap ray model (rad-10 dispel walk) only returns a value when the
     * ray reaches a wall: borg_inc_motion stalls one grid past the target,
     * and a non-floor grid there ends the walk with the collected damage.
     * Put a wall one grid beyond each undead used by the undead cases,
     * BEFORE the borg grid copy below. */
    square_set_feat(cave, loc(11, 17), FEAT_MAGMA);
    square_set_feat(cave, loc(15, 11), FEAT_MAGMA);
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    printf("SETUP level=%d maxsp=%d failtap=%d failbolt=%d maxrange=%d\n",
        borg.trait[BI_CLEVEL], borg.trait[BI_MAXSP],
        borg_spell_fail_rate(TAP_UNLIFE), borg_spell_fail_rate(NETHER_BOLT),
        z_info->max_range);
    if (borg_spell_fail_rate(TAP_UNLIFE) > 25) { printf("SETUPFAIL tap fail rate too high\n"); return 1; }
    lich_kill = giant_kill = bat_kill = 0;
    if (!mode || streq(mode, "undead")) {
        /* Two harmless undead (no ranged attacks, so no danger term):
         * a healthy bat far away and an injured bat nearby. The original
         * rates the tap at each target's full hp and prefers the healthy
         * one; the correction rates both at real damage, where the
         * nearly-dead one rates higher (kill bonus). */
        lich = t_add_monster(cave, loc(11, 16), "vampire bat");   /* distance 5 */
        if (!lich) { printf("SETUPFAIL healthy bat\n"); return 1; }
        if (observe(lich, "healthy")) return 1;
        lich_kill = borg_grids[lich->grid.y][lich->grid.x].kill;
        bat = t_add_monster(cave, loc(14, 11), "vampire bat");    /* distance 3 */
        if (!bat) { printf("SETUPFAIL injured bat\n"); return 1; }
        bat->hp = bat->maxhp / 5; if (bat->hp < 1) bat->hp = 1;
        if (observe(bat, "injured")) return 1;
        bat_kill = borg_grids[bat->grid.y][bat->grid.x].kill;
    }
    if (mode && streq(mode, "non-undead")) {
        giant = t_add_monster(cave, loc(11, 16), "hill giant");   /* distance 5 */
        if (!giant) { printf("SETUPFAIL giant\n"); return 1; }
        if (observe(giant, "giant")) return 1;
        giant_kill = borg_grids[giant->grid.y][giant->grid.x].kill;
    }
    if (mode && streq(mode, "bat")) {
        bat = t_add_monster(cave, loc(14, 11), "vampire bat");    /* distance 3 */
        if (!bat) { printf("SETUPFAIL bat\n"); return 1; }
        if (observe(bat, "bat")) return 1;
        bat_kill = borg_grids[bat->grid.y][bat->grid.x].kill;
    }
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=1000 class=%s level=%d depth=%d hp=%d/%d sp=%d/%d\n",
        player->class->name, player->lev, player->depth, player->chp, player->mhp,
        player->csp, player->msp);
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
    const char *mode = getenv("TAP249_MODE");
    keycode_t keys[64];
    char seq[8 * 64 + 1];
    int len, pos = 0;
    if (!mode) mode = "undead";
    printf("MODE %s\n", mode);

    /* The production attack chooser, acting mode: it fills its own target
     * list and either casts the best-rated attack or finds nothing to do. */
    bool acted = borg_attack(false);
    len = drain_keys(keys, 64);
    printf("DECISION acted=%d keys=%d sequence=", acted, len);
    print_keys(keys, len);
    for (int i = 0; i < len && pos < (int)sizeof(seq) - 2; ++i) {
        if (i) seq[pos++] = ' ';
        seq[pos++] = (char)keys[i];
    }
    seq[pos] = 0;
    printf("SEQSTRING %s\n", seq);
    int tap_cast = 0;
    for (int i = 0; i + 2 < len; ++i)
        if (keys[i] == 'm' && keys[i + 1] == 'c' && keys[i + 2] == 'a')
            tap_cast = 1;
    printf("OUTCOME acted=%d keys=%d tap_cast=%d\n", acted, len, tap_cast);
    if (streq(mode, "undead")) {
        /* The misrating case: the tap must not be rated at the target's
         * full estimated hp. The original targets the healthy bat's ray;
         * the correction rates both taps at real damage, where the
         * nearly-dead bat rates higher and is targeted. */
        eq(acted ? 1 : 0, 1);
        eq(tap_cast, 1);
        require(streq(seq, TAP249_AFTER_SEQ));
    } else if (streq(mode, "non-undead")) {
        /* Control: no undead present, the tap rates zero in both variants
         * and the bolt wins. Complete logs are byte-identical. */
        eq(acted ? 1 : 0, 1);
        require(!tap_cast);
    } else {
        /* Control: only a small undead bat, where the tap legitimately wins
         * in both variants. Complete logs are byte-identical. */
        eq(acted ? 1 : 0, 1);
        eq(tap_cast, 1);
    }
    ok;
}
const char *suite_name = "native-c/tapunlife-249";
struct test tests[] = {{"tap_unlife_rated_at_real_damage", scenario}, {NULL, NULL}};
