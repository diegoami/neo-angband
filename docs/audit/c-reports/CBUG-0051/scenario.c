/* Synthetic native C Teleport-Other skip-list diagnostic, excluded from the source fix. */
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
static struct monster *weak, *strong;
static const char *mode;

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
    int k = borg_grids[mon->grid.y][ mon->grid.x].kill;
    if (k <= 0) { printf("SETUPFAIL %s kill_index\n", tag); return 1; }
    borg_kills[k].awake = 1;
    return 0;
}

int setup_tests(void **state)
{
    const char *profile = getenv("CBUG0051_PROFILE");
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
    if (!player_make_simple("Human", "Mage", "TeleOtherSkipList")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 100;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    mode = getenv("CBUG0051_MODE"); if (!mode) mode = "two-phase";
    player_place(cave, player, loc(11, 11));
    player_exp_gain(player, player_exp[29] * player->expfact / 100);
    while (player_stat_inc(player, STAT_INT)) {}
    player_learn_all_runes(player); calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA | PU_SPELLS;
    update_stuff(player);
    for (int i = 0; i < player->class->magic.total_spells; ++i)
        if (streq(spell_by_index(player, i)->name, "Teleport Other")) spell_learn(i);
    player->chp = player->mhp; player->csp = player->msp;
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_do_spell = true; borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg_cheat_spells();
    /* The borg assumes Morgoth is present on depth 100 (borg-update.c sets
     * near.morgoth there); the fixture sets the same assumption directly. */
    borg.near.morgoth = true;
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    weak = t_add_monster(cave, loc(11, 12), "Kobold");
    if (!weak) { printf("SETUPFAIL weak\n"); return 1; }
    if (strcmp(mode, "single")) {
        strong = t_add_monster(cave, loc(12, 11), "Hill giant");
        if (!strong) { printf("SETUPFAIL strong\n"); return 1; }
    }
    /* Observation order fixes the kill index order and with it the order of
     * target candidates inside each launch. */
    if (observe(weak, "weak")) return 1;
    if (strong && observe(strong, "strong")) return 1;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=1000 class=%s level=%d depth=%d hp=%d/%d sp=%d/%d mode=%s near_morgoth=%d\n",
        player->class->name, player->lev, player->depth, player->chp, player->mhp,
        player->csp, player->msp, mode, borg.near.morgoth);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    printf("TRACK teardown tp_other_n=%d\n", borg_tp_other_n);
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static void fill_temp(void)
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

static int scenario(void *state)
{
    printf("MODE %s\n", mode);
    int danger_strong = 0, danger_weak = 0;
    int p1 = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    for (int i = 1; i < borg_kills_nxt; ++i) {
        if (!borg_kills[i].r_idx) continue;
        int d = borg_danger_one_kill(borg.c.y, borg.c.x, 1, i, true, true);
        printf("KILL index=%d pos=(%d,%d) awake=%d danger=%d\n", i,
            borg_kills[i].pos.x, borg_kills[i].pos.y, borg_kills[i].awake, d);
        if (weak && i == borg_grids[weak->grid.y][weak->grid.x].kill) danger_weak = d;
        if (strong && i == borg_grids[strong->grid.y][strong->grid.x].kill) danger_strong = d;
    }
    printf("DANGER p1=%d strong=%d weak=%d\n", p1, danger_strong, danger_weak);
    require(danger_weak > 0);

    /* Phase 1: the Morgoth-depth evaluation of Teleport Other.  With
     * near.morgoth set and little mana, each listed monster is stored by
     * two different branches, so the second slot is written.  This is the
     * production call from borg_defend_aux_tele_away_morgoth. */
    if (strcmp(mode, "no-morgoth-phase")) {
        fill_temp();
        printf("TEMP phase1 n=%d\n", borg_temp_n);
        player->csp = 30; borg_notice(true);
        int m2 = borg_launch_bolt(0, 50, BORG_ATTACK_AWAY_ALL_MORGOTH, z_info->max_range, 0);
        player->csp = player->msp; borg_notice(true);
        printf("MORGOTH p2=%d slots_after slot0=%d slot1=%d n=%d\n", m2,
            borg_tp_other_index[0], borg_tp_other_index[1], borg_tp_other_n);
        require(m2 > 0);
        eq(borg_tp_other_n, 0);
    }

    /* Phase 2: the regular Teleport Other defense evaluation, the
     * production call from borg_defend_aux_tele_away. */
    fill_temp();
    printf("TEMP phase2 n=%d\n", borg_temp_n);
    int p2 = borg_launch_bolt(0, p1, BORG_ATTACK_AWAY_ALL, z_info->max_range, 0);
    printf("LAUNCH p2=%d\n", p2);
    int expected = strong ? danger_strong : danger_weak;
    printf("OUTCOME p2=%d expected_best=%d strong=%d weak=%d tp_other_n=%d\n",
        p2, expected, danger_strong, danger_weak, borg_tp_other_n);
    eq(borg_tp_other_n, 0);
    /* Corrected contract: only monsters the winning candidate actually
     * lists are skipped, so the value is one candidate's list, never the
     * list plus a slot left over by the Morgoth-depth evaluation. */
    eq(p2, expected);
    require(p2 > 0);
    ok;
}
const char *suite_name = "native-c/CBUG-0051";
struct test tests[] = {{"teleport_other_skips_only_listed", scenario}, {NULL, NULL}};
