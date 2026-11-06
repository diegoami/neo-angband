/* Synthetic native C activation-lookup diagnostic, excluded from the source fix. */
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
static struct monster *target;

static struct object *known_item(int tval, const char *name)
{
    struct object *obj = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) quit("fixture item kind missing");
    object_prep(obj, kind, 50, MAXIMISE);
    obj->number = 1;
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
    return obj;
}

static struct object *activated_item(int tval, const char *name, const char *act_name)
{
    struct object *obj = known_item(tval, name);
    struct activation *act = &activations[1];
    while (act) {
        if (!my_stricmp(act->name, act_name)) break;
        act = act->next;
    }
    if (!act) quit("fixture activation missing");
    obj->activation = act;
    obj->timeout = 0;
    return obj;
}

int setup_tests(void **state)
{
    const char *profile = getenv("A260_PROFILE");
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
    if (!player_make_simple("Human", "Mage", "ActivLookups")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    screen.offset_x = screen.offset_y = 0;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
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
    /* The miswired world wears the activators whose maneuvers look up
     * the wrong ids: the greater acid bolt and the all-monsters dispel.
     * The control world wears the lesser acid bolt, whose maneuver is
     * wired correctly. */
    const char *mode0 = getenv("A260_MODE");
    if (!mode0 || streq(mode0, "miswired")) {
        struct object *ring3 = activated_item(TV_RING, "Protection", "ACID_BOLT3");
        inven_wield(ring3, wield_slot(ring3));
    } else {
        struct object *ring2 = activated_item(TV_RING, "Protection", "ACID_BOLT2");
        inven_wield(ring2, wield_slot(ring2));
    }
    player_place(cave, player, loc(11, 11));
    player_exp_gain(player, player_exp[39] * player->expfact / 100);
    while (player_stat_inc(player, STAT_INT)) {}
    player_learn_all_runes(player); calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA | PU_SPELLS;
    update_stuff(player);
    player->chp = player->mhp; player->csp = player->msp;
    borg_init();
    borg_update();
    borg_alloc_detection();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg.power = borg_power();
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    target = t_add_monster(cave, loc(11, 15), "Kobold");
    if (!target) { printf("SETUPFAIL target\n"); return 1; }
    update_mon(target, cave, true);
    if (!mflag_has(target->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL visible\n"); return 1; }
    if (!observe_kill_diff(target->grid.y, target->grid.x, 0, 0)) { printf("SETUPFAIL observe\n"); return 1; }
    int kill_index = borg_grids[target->grid.y][target->grid.x].kill;
    if (kill_index <= 0) { printf("SETUPFAIL kill_index\n"); return 1; }
    borg_kills[kill_index].awake = 1;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=1000 class=%s level=%d depth=%d\n",
        player->class->name, player->lev, player->depth);
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

static int maneuver_value(int attack_type)
{
    fill_temp_like_attack();
    borg_simulate = true;
    int v = borg_calculate_attack_effectiveness(attack_type);
    borg_simulate = false;
    return v;
}

static int scenario(void *state)
{
    const char *mode = getenv("A260_MODE");
    if (!mode) mode = "full";
    printf("MODE %s\n", mode);
    printf("GEOMETRY player=(%d,%d) target=(%d,%d) temp_n=%d\n",
        borg.c.x, borg.c.y, target->grid.x, target->grid.y, borg_temp_n);

    int v_bolt3 = maneuver_value(BF_ACT_ACID_BOLT3);
    int v_bolt2 = maneuver_value(BF_ACT_ACID_BOLT2);

    if (streq(mode, "control")) {
        (void)v_bolt3;
        /* Control: the correctly-wired lesser-bolt maneuver is valued
         * identically before and after the correction. */
        printf("RATING bolt2=%d\n", v_bolt2);
        printf("OUTCOME bolt2=%d\n", v_bolt2);
        require(v_bolt2 > 0);
        ok;
    }
    printf("RATING bolt3=%d bolt2=%d\n", v_bolt3, v_bolt2);
    printf("OUTCOME bolt3=%d bolt2=%d\n", v_bolt3, v_bolt2);
    /* Correct contract: the worn greater-bolt activator is valued by
     * the maneuver that describes it. */
    require(v_bolt3 > 0);
    ok;
}
const char *suite_name = "native-c/activ-260";
struct test tests[] = {{"maneuvers_find_their_activations", scenario}, {NULL, NULL}};
