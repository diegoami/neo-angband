/* Synthetic native C arc-angle diagnostic, excluded from the source fix. */
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
static struct monster *front, *behind;

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
    return 0;
}

int setup_tests(void **state)
{
    const char *profile = getenv("A253_PROFILE");
    const char *mode = getenv("A253_MODE");
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
    if (!player_make_simple("Human", "Mage", "ArcAngleGrid")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    known_item(TV_MAGIC_BOOK, "[Attacks and Knowledge]", 1);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    player_place(cave, player, loc(11, 11));
    player_exp_gain(player, player_exp[24] * player->expfact / 100);
    while (player_stat_inc(player, STAT_INT)) {}
    player_learn_all_runes(player); calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA | PU_SPELLS;
    update_stuff(player);
    for (int i = 0; i < player->class->magic.total_spells; ++i)
        if (streq(spell_by_index(player, i)->name, "Acid Spray")) spell_learn(i);
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
    /* The in-arc monster is four grids north; the bystander is four grids
     * south, inside the spell's ten-grid range but 180 degrees off the
     * spray.  The in-arc control uses two monsters straight ahead. */
    front = t_add_monster(cave, loc(11, 7), "Kobold");
    if (!front) { printf("SETUPFAIL front\n"); return 1; }
    if (observe(front, "front")) return 1;
    if (mode && streq(mode, "arc-both")) {
        behind = t_add_monster(cave, loc(11, 9), "Kobold");   /* also north */
        if (!behind) { printf("SETUPFAIL second\n"); return 1; }
        if (observe(behind, "second")) return 1;
    }
    if (!mode || streq(mode, "arc-bystander")) {
        behind = t_add_monster(cave, loc(11, 15), "Kobold");  /* south */
        if (!behind) { printf("SETUPFAIL behind\n"); return 1; }
        if (observe(behind, "behind")) return 1;
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

static int acid_spray_value(void)
{
    /* The exact production call the chooser makes for Acid Spray. */
    int dam = ((borg.trait[BI_CLEVEL] / 2) * (8 + 1)) / 2;
    borg_simulate = true;
    int v = borg_attack_aux_spell_bolt(
        ACID_SPRAY, 60, dam, BORG_ATTACK_ACID, 10, true);
    borg_simulate = false;
    return v;
}

static int scenario(void *state)
{
    const char *mode = getenv("A253_MODE");
    if (!mode) mode = "arc-bystander";
    printf("MODE %s\n", mode);
    fill_temp_like_attack();
    printf("GEOMETRY front=(%d,%d) behind=(%d,%d) player=(%d,%d) temp_n=%d\n",
        front->grid.x, front->grid.y, behind ? behind->grid.x : -1,
        behind ? behind->grid.y : -1, borg.c.x, borg.c.y, borg_temp_n);

    /* Reference: the value with the bystander unknown to the Borg (its
     * kill record is hidden for the measurement, then restored). */
    int ref = 0, value = 0, saved_ridx = 0;
    int behind_index = -1;
    if (behind) behind_index = borg_grids[behind->grid.y][behind->grid.x].kill;
    if (behind_index > 0) {
        saved_ridx = borg_kills[behind_index].r_idx;
        borg_kills[behind_index].r_idx = 0;
    }
    fill_temp_like_attack();
    ref = acid_spray_value();
    if (behind_index > 0) borg_kills[behind_index].r_idx = saved_ridx;

    /* Full world: whatever the bystander contributes now shows up in the
     * value only if the angle test wrongly counts an off-arc grid. */
    fill_temp_like_attack();
    value = acid_spray_value();
    printf("RATING reference=%d value=%d\n", ref, value);
    printf("OUTCOME reference=%d value=%d\n", ref, value);
    require(ref > 0);
    /* Corrected contract: an off-arc bystander adds nothing; an in-arc
     * second monster adds its own contribution. */
    if (streq(mode, "arc-both")) { require(value > ref); }
    else { eq(value, ref); }
    ok;
}
const char *suite_name = "native-c/arc-253";
struct test tests[] = {{"arc_counts_only_monsters_inside_the_angle", scenario}, {NULL, NULL}};
