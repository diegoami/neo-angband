/* Synthetic native C ring-attack diagnostic, excluded from the source proposal. */
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
#include "obj-util.h"
#include "option.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-timed.h"
#include "player-util.h"
#include "target.h"
#include "ui-menu.h"
#include "ui-term.h"
#include "borg/borg.h"
#include "borg/borg-cave.h"
#include "borg/borg-fight-attack.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-item-use.h"
#include "borg/borg-item-val.h"
#include "borg/borg-projection.h"
#include "borg/borg-trait.h"
#include <stdio.h>
#include <stdlib.h>

static term screen;
static const char *mode;
static struct object *right_ring, *left_ring;
static int midx, start_enemy_hp;

static int enemy_hp(void) { return cave->monsters[midx].race ? cave->monsters[midx].hp : 0; }
static void frame(const char *stage)
{
    printf("FRAME {\"stage\":\"%s\",\"hp\":%d,\"mana\":%d,\"enemy_hp\":%d,\"enemy_start_hp\":%d,\"right_timeout\":%d,\"left_timeout\":%d,\"fire_protection\":%d}\n",
        stage, player->chp, player->csp, enemy_hp(), start_enemy_hp,
        right_ring ? right_ring->timeout : -1, left_ring ? left_ring->timeout : -1, player->timed[TMD_OPP_FIRE]);
}
static void message(game_event_type type, game_event_data *data, void *user)
{
    if (data->message.msg) printf("ENGINE %s\n", data->message.msg);
}
static struct object *ring(const char *name, const char *slot)
{
    struct object *obj = object_new();
    object_prep(obj, lookup_kind(TV_RING, lookup_sval(TV_RING, name)), 50, MAXIMISE);
    obj->known = object_new(); object_set_base_known(player, obj);
    object_flavor_aware(player, obj); inven_carry(player, obj, false, false);
    object_touch(player, obj); player_know_object(player, obj);
    inven_wield(obj, slot_by_name(player, slot));
    return obj;
}
int setup_tests(void **state)
{
    const char *profile = getenv("CBUG0062_PROFILE"); if (!profile) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) return 1;
    create_needed_dirs(); Rand_quick = false; Rand_state_init(1000);
    if (!player_make_simple("Human", "Warrior", "NativeRingAttack")) return 1;
    term_init(&screen, 160, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player_place(cave, player, loc(20, 20)); character_dungeon = true;
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    player_exp_gain(player, player_exp[38] * player->expfact / 100);
    while (player_stat_inc(player, STAT_INT)) {}
    mode = getenv("CBUG0062_MODE"); if (!mode) mode = "left-ready";
    bool right_fire = streq(mode, "right-control") || streq(mode, "both-ready") || streq(mode, "right-charging-left-ready") || streq(mode, "blind-control");
    bool left_fire = streq(mode, "left-ready") || streq(mode, "both-ready") || streq(mode, "right-charging-left-ready") || streq(mode, "left-charging");
    right_ring = ring(right_fire ? "Flames" : "Strength", "right hand");
    left_ring = ring(left_fire ? "Flames" : "Strength", "left hand");
    if (streq(mode, "right-charging-left-ready")) right_ring->timeout = 10;
    if (streq(mode, "left-charging")) left_ring->timeout = 10;
    player_learn_all_runes(player); calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA;
    update_stuff(player); player->chp = player->mhp; player->csp = player->msp;
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) return 1;
    borg_equipment(); borg_inventory(); borg_notice(true); borg.c = player->grid; borg.avoidance = player->chp;
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x)
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
    struct monster *enemy = t_add_monster(cave, loc(20, 14), "Ogre");
    update_view(cave, player); update_mon(enemy, cave, true);
    if (!mflag_has(enemy->mflag, MFLAG_VISIBLE)) return 1;
    if (!observe_kill_diff(enemy->grid.y, enemy->grid.x, 0, 0)) return 1;
    midx = enemy->midx; start_enemy_hp = enemy->hp;
    borg_temp_n = 1; borg_temp_x[0] = enemy->grid.x; borg_temp_y[0] = enemy->grid.y;
    if (streq(mode, "blind-control")) {
        player_inc_timed(player, TMD_BLIND, 10, true, true, true); borg_notice_player();
    }
    event_add_handler(EVENT_MESSAGE, message, NULL);
    printf("SETUP seed=1000 class=Warrior level=%d depth=%d hp=%d device_native=%d device_borg=%d target_hp=%d distance=%d\n",
        player->lev, player->depth, player->chp, player->state.skills[SKILL_DEVICE], borg.trait[BI_DEV], enemy->hp, distance(player->grid, enemy->grid));
    printf("RINGS right_fire=%d left_fire=%d right_timeout=%d left_timeout=%d right_known=%d left_known=%d\n",
        right_fire, left_fire, right_ring->timeout, left_ring->timeout,
        borg_items[INVEN_RIGHT].ident, borg_items[INVEN_LEFT].ident);
    printf("ACTIVATION native_failure_per1000=%d\n", get_use_device_chance(right_fire ? right_ring : left_ring));
    return 0;
}
int teardown_tests(void *state)
{
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}
static int scenario(void *state)
{
    bool expected = streq(mode, "left-ready") || streq(mode, "right-control") || streq(mode, "both-ready") || streq(mode, "right-charging-left-ready");
    printf("MODE %s\n", mode); frame("start");
    borg_simulate = false;
    int score = borg_calculate_attack_effectiveness(BF_RING_FIRE);
    keycode_t keys[128], key; int len = 0, activation = -1, selected_slot = -1;
    while ((key = borg_inkey(true)) != 0 && len < 128) keys[len++] = key;
    printf("DECISION score=%d keys=%d goal_x=%d goal_y=%d sequence=", score, len, borg.goal.g.x, borg.goal.g.y);
    for (int i = 0; i < len; ++i) { if (keys[i] == ESCAPE) printf("<Escape>"); else printf("%c", (char)keys[i]); if (keys[i] == 'A') activation = i; }
    putchar('\n');
    if (activation >= 0) {
        require(activation + 1 < len);
        for (int i = 0; i < player->body.count; ++i) if ((keycode_t)all_letters_nohjkl[i] == keys[activation+1]) selected_slot = i;
        require(selected_slot >= 0);
        struct object *obj = player->body.slots[selected_slot].obj;
        require(obj && obj->tval == TV_RING && obj->sval == lookup_sval(TV_RING, "Flames") && obj->timeout == 0);
        require(borg.goal.g.x == cave->monsters[midx].grid.x && borg.goal.g.y == cave->monsters[midx].grid.y);
        target_set_location(borg.goal.g.y, borg.goal.g.x);
        struct command cmd = { .code = CMD_ACTIVATE, .context = CTX_GAME };
        cmd_set_arg_item(&cmd, "item", obj); cmd_set_arg_target(&cmd, "target", DIR_TARGET);
        do_cmd_activate(&cmd); cmd_release(&cmd);
    }
    frame("activation");
    int damage = start_enemy_hp - enemy_hp();
    printf("OUTCOME activated=%d selected_slot=%d enemy_before=%d enemy_after=%d damage=%d killed=%d right_timeout=%d left_timeout=%d fire_protection=%d\n",
        activation >= 0, selected_slot, start_enemy_hp, enemy_hp(), damage, !cave->monsters[midx].race,
        right_ring->timeout, left_ring->timeout, player->timed[TMD_OPP_FIRE]);
    eq(activation >= 0, expected);
    if (expected) {
        require(score > 0); require(damage > 0); require(player->timed[TMD_OPP_FIRE] > 0);
        if (streq(mode, "left-ready") || streq(mode, "right-charging-left-ready")) {
            eq(selected_slot, slot_by_name(player, "left hand"));
        } else {
            eq(selected_slot, slot_by_name(player, "right hand"));
        }
    } else { eq(score, 0); eq(damage, 0); }
    ok;
}
const char *suite_name = "native-c/CBUG-0062";
struct test tests[] = {{"attack_uses_ready_ring_on_either_hand", scenario}, {NULL, NULL}};
