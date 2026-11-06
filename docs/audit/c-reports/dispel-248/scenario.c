/* Synthetic native C Dispel Life rating diagnostic, excluded from the source fix. */
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
static struct monster *target;
static int engine_messages;

static void message(game_event_type type, game_event_data *data, void *user)
{
    const char *text = data->message.msg;
    if (!text) return;
    ++engine_messages;
    printf("ENGINE: %s\n", text);
    /* Headless message renderer; no borg turn is driven here. */
    Term_erase(0, 0, 255);
    Term_putstr(0, 0, -1, COLOUR_WHITE, text);
}

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
    const char *profile = getenv("D248_PROFILE");
    const char *mode = getenv("D248_MODE");
    int spell_index = -1;
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
    if (!player_make_simple("Human", "Necromancer", "DispelLifeRating")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    known_item(TV_SHADOW_BOOK, "[Fear and Torment]", 1);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    player_place(cave, player, loc(11, 11));
    player_exp_gain(player, player_exp[24] * player->expfact / 100);
    while (player_stat_inc(player, STAT_INT)) {}
    player_learn_all_runes(player); calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA | PU_SPELLS;
    update_stuff(player);
    for (int i = 0; i < player->class->magic.total_spells; ++i) {
        if (streq(spell_by_index(player, i)->name, "Dispel Life")) { spell_learn(i); spell_index = i; }
        if (streq(spell_by_index(player, i)->name, "Nether Bolt")) spell_learn(i);
    }
    if (spell_index < 0) { printf("SETUPFAIL spell\n"); return 1; }
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
    /* The living target is a hill giant (no undead or nonliving flags);
     * the control is a vampire bat (explicitly undead, drain-immune). */
    target = t_add_monster(cave, loc(11, 16),
        (mode && streq(mode, "undead")) ? "Vampire bat" : "Hill giant");
    if (!target) { printf("SETUPFAIL target\n"); return 1; }
    mflag_on(target->mflag, MFLAG_AWARE); update_mon(target, cave, true);
    if (!mflag_has(target->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL visible\n"); return 1; }
    if (!observe_kill_diff(target->grid.y, target->grid.x, 0, 0)) { printf("SETUPFAIL observe\n"); return 1; }
    int kill_index = borg_grids[target->grid.y][target->grid.x].kill;
    if (kill_index <= 0) { printf("SETUPFAIL kill_index\n"); return 1; }
    borg_kills[kill_index].awake = 1;
    borg.status.active = true;
    borg_update_entrypoint(true);
    event_add_handler(EVENT_MESSAGE, message, NULL);
    printf("SETUP seed=1000 class=%s level=%d depth=%d hp=%d/%d sp=%d/%d target=%s hp=%d undead=%d\n",
        player->class->name, player->lev, player->depth, player->chp, player->mhp,
        player->csp, player->msp, target->race->name, target->hp,
        rf_has(target->race->flags, RF_UNDEAD));
    return 0;
}

int teardown_tests(void *state)
{
    event_remove_handler(EVENT_MESSAGE, message, NULL);
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
    const char *mode = getenv("D248_MODE");
    if (!mode) mode = "living";
    bool living = streq(mode, "living");
    printf("MODE %s\n", mode);
    require(borg_temp_n == 0);

    /* Control mode: only the primary bolt, whose rating the Dispel Life
     * filter never touches. */
    if (streq(mode, "bolt")) {
        fill_temp_like_attack();
        borg_simulate = true;
        int bolt_dam = ((((borg.trait[BI_CLEVEL] / 4) + 3) * (4 + 1)) / 2);
        int bolt = borg_attack_aux_spell_bolt(
            NETHER_BOLT, 0, bolt_dam, BORG_ATTACK_NETHER, z_info->max_range, false);
        borg_simulate = false;
        printf("RATING bolt_control=%d\n", bolt);
        require(bolt > 0);
        ok;
    }

    /* The exact production call the attack chooser makes for Dispel Life,
     * evaluated the way the chooser evaluates it. */
    fill_temp_like_attack();
    borg_simulate = true;
    int dam = ((borg.trait[BI_CLEVEL] * 3) + 1) / 2;
    printf("GATE spell_okay=%d legal=%d temp_n=%d\n",
        borg_spell_okay_fail(DISPEL_LIFE, 25), borg_spell_legal(DISPEL_LIFE),
        borg_temp_n);
    int launch = borg_launch_bolt(0, dam, BORG_ATTACK_DRAIN_LIFE, z_info->max_range, 0);
    printf("LAUNCH direct=%d\n", launch);
    int value = borg_attack_aux_spell_bolt(
        DISPEL_LIFE, 0, dam, BORG_ATTACK_DRAIN_LIFE, z_info->max_range, false);
    /* Control: the class's primary bolt, whose rating this filter never
     * touches, valued against the same living giant. */
    borg_simulate = false;
    printf("RATING value=%d expected_average=%d clevel=%d\n", value, dam,
        borg.trait[BI_CLEVEL]);

    /* The game's own drain: a native cast shows whom the spell really hurts. */
    int spell_index = -1;
    for (int i = 0; i < player->class->magic.total_spells; ++i)
        if (streq(spell_by_index(player, i)->name, "Dispel Life")) spell_index = i;
    require(spell_index >= 0);
    int hp_before = target->hp;
    struct command cmd = { .code = CMD_CAST, .context = CTX_GAME };
    cmd_set_arg_choice(&cmd, "spell", spell_index);
    do_cmd_cast(&cmd);
    cmd_release(&cmd);
    printf("OUTCOME hp_before=%d hp_after=%d hp_lost=%d undead=%d\n",
        hp_before, target->hp, hp_before - target->hp,
        rf_has(target->race->flags, RF_UNDEAD));

    /* Corrected contract: the rating matches the game's drain.  The living
     * monster is worth the launch value minus the spell's mana cost (the
     * chooser's standing penalty); the undead one is never worth casting.
     * The primary-bolt control is untouched by the filter. */
    int expected = dam - borg_get_spell_power(DISPEL_LIFE);
    if (living) { eq(value, expected); require(target->hp < hp_before); }
    else { require(value <= 0); eq(target->hp, hp_before); }
    ok;
}
const char *suite_name = "native-c/dispel-248";
struct test tests[] = {{"dispel_life_rates_living_monsters", scenario}, {NULL, NULL}};
