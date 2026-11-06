/* Synthetic native C weapon-slay diagnostic, excluded from the source fix. */
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
#include "obj-slays.h"
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
static struct monster *animal;

static struct object *known_weapon(void)
{
    struct object *obj = object_new();
    struct object_kind *kind = lookup_kind(TV_SWORD, lookup_sval(TV_SWORD, "Broad Sword"));
    if (!kind) quit("fixture weapon kind missing");
    object_prep(obj, kind, 50, MAXIMISE);
    /* Give the weapon the animals slay (race flag ANIMAL, the base
     * multiplier of two), as a Slay Animal ego would. */
    if (!obj->slays) obj->slays = mem_zalloc(z_info->slay_max * sizeof(bool));
    int found = 0;
    for (int i = 0; i < z_info->slay_max; i++)
        if (slays[i].race_flag == RF_ANIMAL && slays[i].multiplier == 2) {
            obj->slays[i] = true;
            found++;
        }
    if (!found) quit("fixture slay missing");
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
    const char *profile = getenv("S239_PROFILE");
    const char *mode = getenv("S239_MODE");
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
    if (!player_make_simple("Human", "Warrior", "SlayTraits")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    screen.offset_x = screen.offset_y = 0;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* A Slay Animal broadsword in the weapon slot, and the birth light
     * wielded in the later light slot, which is the slot whose empty
     * slays erase the weapon's in the original. */
    struct object *sword = known_weapon();
    inven_wield(sword, wield_slot(sword));
    for (int pass = 0; pass < 4; pass++) {
        for (struct object *obj = player->gear; obj; obj = obj->next) {
            if (obj->tval != TV_LIGHT) continue;
            if (player->body.slots[wield_slot(obj)].obj == obj) continue;
            inven_wield(obj, wield_slot(obj));
            break;
        }
    }
    player_place(cave, player, loc(11, 11));
    player_exp_gain(player, player_exp[29] * player->expfact / 100);
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
    /* An animal (or, in the control, a monster with no animal flag),
     * adjacent and awake. */
    animal = t_add_monster(cave, loc(12, 11),
        (mode && streq(mode, "nonanimal")) ? "Small kobold" : "White harpy");
    if (!animal) { printf("SETUPFAIL animal\n"); return 1; }
    update_mon(animal, cave, true);
    if (!mflag_has(animal->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL visible\n"); return 1; }
    if (!observe_kill_diff(animal->grid.y, animal->grid.x, 0, 0)) { printf("SETUPFAIL observe\n"); return 1; }
    int kill_index = borg_grids[animal->grid.y][animal->grid.x].kill;
    if (kill_index <= 0) { printf("SETUPFAIL kill_index\n"); return 1; }
    borg_kills[kill_index].awake = 1;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=1000 class=%s level=%d depth=%d weapon=%s light_tval=%d\n",
        player->class->name, player->lev, player->depth, borg_items[INVEN_WIELD].desc,
        borg_items[INVEN_LIGHT].tval);
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

static int thrust_value(void)
{
    fill_temp_like_attack();
    borg_simulate = true;
    int v = borg_calculate_attack_effectiveness(BF_THRUST);
    borg_simulate = false;
    return v;
}

static int scenario(void *state)
{
    const char *mode = getenv("S239_MODE");
    printf("MODE %s\n", mode ? mode : "animal");
    int kill_index = borg_grids[animal->grid.y][animal->grid.x].kill;
    struct monster_race *r_ptr = &r_info[borg_kills[kill_index].r_idx];
    printf("GEOMETRY player=(%d,%d) animal=(%d,%d) rf_animal=%d temp_n=%d\n",
        borg.c.x, borg.c.y, animal->grid.x, animal->grid.y,
        (int)rf_has(r_ptr->flags, RF_ANIMAL), borg_temp_n);

    int trait_as_computed = borg.trait[BI_WS_ANIMAL];

    /* The thrust value with the trait as the notice computed it, and the
     * same measurement with the trait forced off and on: what the attack
     * is worth without and with the weapon's slay. */
    int auto_value = thrust_value();
    borg.trait[BI_WS_ANIMAL] = 0;
    int without_slay = thrust_value();
    borg.trait[BI_WS_ANIMAL] = 1;
    int with_slay = thrust_value();
    borg.trait[BI_WS_ANIMAL] = trait_as_computed;

    if (!mode || streq(mode, "animal"))
        printf("TRAIT ws_animal=%d\n", trait_as_computed);
    printf("RATING auto=%d without=%d with=%d\n", auto_value, without_slay, with_slay);
    printf("OUTCOME auto=%d without=%d with=%d\n",
        auto_value, without_slay, with_slay);
    if (!mode || streq(mode, "animal")) {
        /* Correct contract: the wielded Slay Animal weapon raises the
         * thrust value against an animal above the same attack without
         * the slay. */
        require(auto_value > without_slay);
    } else {
        /* Control: against a monster with no animal flag the slay changes
         * nothing; the three readings coincide. */
        eq(auto_value, without_slay);
    }
    ok;
}
const char *suite_name = "native-c/slay-239";
struct test tests[] = {{"slay_weapon_counts", scenario}, {NULL, NULL}};
