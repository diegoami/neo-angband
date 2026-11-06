/* Synthetic native C reserve-bolt diagnostic, excluded from the source fix. */
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

static term screen;
static struct monster *bully, *weakling;

int setup_tests(void **state)
{
    const char *profile = getenv("R258_PROFILE");
    const char *mode = getenv("R258_MODE");
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
    /* The reserve bolt is a young-character emergency: under level 15. */
    if (!player_make_simple("Human", "Mage", "ReserveBolt")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    screen.offset_x = screen.offset_y = 0;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    for (int pass = 0; pass < 4; pass++) {
        for (struct object *obj = player->gear; obj; obj = obj->next) {
            if (obj->tval != TV_LIGHT) continue;
            if (player->body.slots[wield_slot(obj)].obj == obj) continue;
            inven_wield(obj, wield_slot(obj));
            break;
        }
    }
    player_place(cave, player, loc(11, 11));
    player_exp_gain(player, player_exp[12] * player->expfact / 100);
    while (player_stat_inc(player, STAT_INT)) {}
    player_learn_all_runes(player); calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA | PU_SPELLS;
    update_stuff(player);
    player->chp = player->mhp;
    /* The emergency premise: not enough mana to cast the bolt the
     * normal way, while the maximum still allows the faked cast. */
    player->csp = 0;
    for (int i = 0; i < player->class->magic.total_spells; ++i)
        spell_learn(i);
    borg_init();
    borg_update();
    borg_alloc_detection();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_do_spell = true;
    borg_cheat_spells();
    borg_notice(true);
    borg.c = player->grid; borg.ready_morgoth = 0;
    borg.power = borg_power();
    /* A low avoidance so the pair of monsters clears the must-be-dangerous
     * gate, as a young mage's fear of two monsters would. */
    borg.avoidance = 4;
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    /* The bully is observed first, so it owns the lower kill index: a
     * young red dragon far across the arena.  The weakling is a kobold
     * two grids away.  In the control only the kobold stands. */
    if (!mode || streq(mode, "veto")) {
        bully = t_add_monster(cave, loc(11, 25), "Young red dragon");
        if (!bully) { printf("SETUPFAIL bully\n"); return 1; }
        update_mon(bully, cave, true);
        if (!mflag_has(bully->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL bully visible\n"); return 1; }
        if (!observe_kill_diff(bully->grid.y, bully->grid.x, 0, 0)) { printf("SETUPFAIL bully observe\n"); return 1; }
        borg_kills[borg_grids[bully->grid.y][bully->grid.x].kill].awake = 1;
    }
    weakling = t_add_monster(cave, loc(11, 13), "Kobold");
    if (!weakling) { printf("SETUPFAIL weakling\n"); return 1; }
    update_mon(weakling, cave, true);
    if (!mflag_has(weakling->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL weakling visible\n"); return 1; }
    if (!observe_kill_diff(weakling->grid.y, weakling->grid.x, 0, 0)) { printf("SETUPFAIL weakling observe\n"); return 1; }
    borg_kills[borg_grids[weakling->grid.y][weakling->grid.x].kill].awake = 1;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=1000 class=%s level=%d depth=%d avoidance=%d\n",
        player->class->name, player->lev, player->depth, borg.avoidance);
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

static int scenario(void *state)
{
    const char *mode = getenv("R258_MODE");
    if (!mode) mode = "veto";
    fill_temp_like_attack();
    int danger = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    printf("MODE %s\n", mode);
    printf("GEOMETRY player=(%d,%d) weakling=(%d,%d) bully=%s danger=%d avoidance=%d temp_n=%d\n",
        borg.c.x, borg.c.y, weakling->grid.x, weakling->grid.y,
        bully ? "(11,25)" : "none", danger, borg.avoidance, borg_temp_n);

    /* The exact production call the chooser makes for the emergency
     * magic missile. */
    fill_temp_like_attack();
    borg_simulate = true;
    int v = borg_calculate_attack_effectiveness(BF_SPELL_MAGIC_MISSILE_RESERVE);
    borg_simulate = false;
    printf("RATING value=%d danger_gate=%d\n", v, danger >= borg.avoidance * 2);
    printf("OUTCOME value=%d\n", v);
    /* Correct contract: a distant tough monster does not veto the
     * emergency bolt for the one nearby weak monster. */
    require(v > 0);
    ok;
}
const char *suite_name = "native-c/reserve-258";
struct test tests[] = {{"reserve_bolt_checks_near_monsters", scenario}, {NULL, NULL}};
