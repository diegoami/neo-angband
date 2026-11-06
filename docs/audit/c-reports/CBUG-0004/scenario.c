/* Synthetic native C resistance/escape diagnostic, excluded from the source fix. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "cmd-core.h"
#include "cmds.h"
#include "game-event.h"
#include "game-world.h"
#include "init.h"
#include "mon-attack.h"
#include "mon-make.h"
#include "mon-timed.h"
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
#include "ui-menu.h"
#include "ui-term.h"
#include "borg/borg.h"
#include "borg/borg-cave.h"
#include "borg/borg-danger.h"
#include "borg/borg-escape.h"
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
static struct monster *enemy;
static const char *mode;
static int direction, kill_index;

static int phase_scrolls(void)
{
    int count = 0;
    for (struct object *obj = player->gear; obj; obj = obj->next)
        if (obj->tval == TV_SCROLL && obj->sval == lookup_sval(TV_SCROLL, "Phase Door")) count += obj->number;
    return count;
}

static void frame(const char *stage)
{
    printf("FRAME {\"stage\":\"%s\",\"hp\":%d,\"max_hp\":%d,\"mana\":%d,\"max_mana\":%d,\"scrolls\":%d,\"x\":%d,\"y\":%d,\"enemy_x\":%d,\"enemy_y\":%d,\"distance\":%d}\n",
        stage, player->chp, player->mhp, player->csp, player->msp,
        phase_scrolls(),
        player->grid.x, player->grid.y, enemy->grid.x, enemy->grid.y,
        distance(enemy->grid, player->grid));
}

static void message(game_event_type type, game_event_data *data, void *user)
{
    if (data->message.msg) printf("ENGINE %s\n", data->message.msg);
}

static struct object *item(int tval, const char *name, int number)
{
    struct object *obj = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) quit("fixture item kind missing");
    object_prep(obj, kind, 50, MAXIMISE);
    obj->number = number;
    obj->known = object_new(); object_set_base_known(player, obj);
    object_flavor_aware(player, obj); inven_carry(player, obj, false, false);
    object_touch(player, obj); player_know_object(player, obj);
    return obj;
}

int setup_tests(void **state)
{
    const char *profile = getenv("CBUG0004_PROFILE");
    if (!profile) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) return 1;
    create_needed_dirs(); Rand_quick = false; Rand_state_init(1000);
    if (!player_make_simple("Human", "Rogue", "NativeColdEscape")) return 1;
    term_init(&screen, 160, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player_place(cave, player, loc(20, 20));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 28;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    player_exp_gain(player, player_exp[18] * player->expfact / 100);
    while (player_stat_inc(player, STAT_INT)) {}
    struct object *ring = item(TV_RING, "Intelligence", 1);
    inven_wield(ring, wield_slot(ring));
    mode = getenv("CBUG0004_MODE"); if (!mode) mode = "acid-only";
    item(TV_SCROLL, "Phase Door", 1);
    player_learn_all_runes(player); calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA | PU_SPELLS;
    update_stuff(player); player->chp = player->mhp; player->csp = player->msp;
    for (int i = 0; i < player->class->magic.total_spells; ++i)
        if (streq(spell_by_index(player, i)->name, "Phase Door")) spell_learn(i);
    if (streq(mode, "acid-only") || streq(mode, "both") || streq(mode, "physical-acid-control"))
        player_inc_timed(player, TMD_OPP_ACID, 50, true, true, true);
    if (streq(mode, "cold-only") || streq(mode, "both") || streq(mode, "physical-cold-control"))
        player_inc_timed(player, TMD_OPP_COLD, 50, true, true, true);
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) return 1;
    borg_equipment(); borg_inventory(); borg_do_spell = true; borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg_cheat_spells();
    /* Construct a preparation age past the existing anti-summon cooldown. */
    borg.time.now = 100;
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x)
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
    direction = 0;
    const char *race = strstr(mode, "physical-") ? "Giant red scorpion" : "Snow troll";
    enemy = t_add_monster(cave, loc(20, 21), race);
    update_view(cave, player); update_mon(enemy, cave, true);
    if (!mflag_has(enemy->mflag, MFLAG_VISIBLE)) return 1;
    if (!observe_kill_diff(enemy->grid.y, enemy->grid.x, 0, 0)) return 1;
    kill_index = borg_grids[enemy->grid.y][enemy->grid.x].kill;
    if (kill_index <= 0) return 1;
    event_add_handler(EVENT_MESSAGE, message, NULL);
    printf("SETUP seed=1000 class=%s level=%d depth=%d hp=%d/%d sp=%d/%d ammo=%d phase=%d fail=%d\n",
        player->class->name, player->lev, player->depth, player->chp, player->mhp, player->csp, player->msp,
        borg.trait[BI_AMISSILES], borg.trait[BI_APHASE], borg_spell_fail_rate(PHASE_DOOR));
    printf("THREAT name=%s direction=%d kill_index=%d power=%d awake=%d ranged=%d friends=%d\n",
        enemy->race->name, direction, kill_index, borg_kills[kill_index].power, borg_kills[kill_index].awake,
        borg_kills[kill_index].ranged_attack, !!(enemy->race->friends || enemy->race->friends_base));
    printf("RESIST native_acid=%d native_cold=%d borg_acid=%d borg_cold=%d age=%d\n",
        player->timed[TMD_OPP_ACID], player->timed[TMD_OPP_COLD],
        borg.temp.res_acid, borg.temp.res_cold, borg.time.now);
    printf("SPEED native=%d borg=%d monster=%d individual_danger=%d fear=%d/%d\n",
        player->state.speed, borg.trait[BI_SPEED], borg_kills[kill_index].speed,
        borg_danger_one_kill(borg.c.y, borg.c.x, 1, kill_index, true, true),
        borg_fear_region[borg.c.y/11][borg.c.x/11], borg_fear_monsters[borg.c.y][borg.c.x]);
    return 0;
}

int teardown_tests(void *state)
{
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    bool expected = streq(mode, "acid-only") || streq(mode, "neither") || strstr(mode, "physical-");
    printf("MODE %s\n", mode);
    int danger = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    printf("DANGER current=%d escape_threshold=%d\n", danger, borg.avoidance * 8 / 10);
    int start_hp = player->chp, start_sp = player->csp;
    int start_scrolls = phase_scrolls();
    struct loc start = player->grid;
    frame("start");
    bool decision = borg_escape(danger);
    keycode_t keys[64]; int len = 0; keycode_t key;
    while ((key = borg_inkey(true)) != 0 && len < 64) keys[len++] = key;
    printf("DECISION escape=%d keys=%d sequence=", decision, len);
    for (int i = 0; i < len; ++i) {
        if (keys[i] == ESCAPE) printf("<Escape>"); else printf("%c", (char)keys[i]);
    }
    putchar('\n');
    int first = 0;
    while (first < len && keys[first] == ESCAPE) first++;
    if (len) {
        require(first < len);
        require(keys[first] == 'm' || keys[first] == 'r');
        bool scroll = keys[first] == 'r';
        require(len - first == (scroll ? 2 : 3));
        int inv = -1, offset = -1;
        for (int i = 0; i < z_info->pack_size; ++i) if ((keycode_t)all_letters_nohjkl[i] == keys[first+1]) inv = i;
        require(inv >= 0);
        struct object *book_obj = player->upkeep->inven[inv]; require(book_obj);
        if (scroll) {
            require(book_obj->tval == TV_SCROLL && book_obj->sval == lookup_sval(TV_SCROLL, "Phase Door"));
            struct command cmd = { .code = CMD_READ_SCROLL, .context = CTX_GAME };
            cmd_set_arg_item(&cmd, "item", book_obj); do_cmd_read_scroll(&cmd); cmd_release(&cmd);
        } else {
            for (int i = 0; i < 26; ++i) if ((keycode_t)all_letters_nohjkl[i] == keys[first+2]) offset = i;
            require(offset >= 0);
            const struct class_book *book = player_object_to_book(player, book_obj);
            require(book && offset < book->num_spells);
            int sidx = book->spells[offset].sidx;
            require(streq(spell_by_index(player, sidx)->name, "Phase Door") && spell_okay_to_cast(player, sidx));
            struct command cmd = { .code = CMD_CAST, .context = CTX_GAME };
            cmd_set_arg_choice(&cmd, "spell", sidx); do_cmd_cast(&cmd); cmd_release(&cmd);
        }
    }
    frame("decision");
    int melee = 0;
    if (distance(enemy->grid, player->grid) <= 1) {
        melee++; make_attack_normal(enemy, player);
    }
    frame("attack");
    printf("OUTCOME escape=%d moved=%d distance=%d melee=%d hp_before=%d hp_after=%d hp_lost=%d mana_spent=%d scrolls_spent=%d\n",
        decision, !loc_eq(start, player->grid), distance(enemy->grid, player->grid), melee,
        start_hp, player->chp, start_hp - player->chp, start_sp - player->csp, start_scrolls - phase_scrolls());
    eq(decision, expected);
    if (expected) {
        require(!loc_eq(start, player->grid)); require(distance(enemy->grid, player->grid) > 1);
        eq(melee, 0); eq(start_hp - player->chp, 0);
    } else {
        eq(len, 0); require(loc_eq(start, player->grid)); eq(start_sp - player->csp, 0);
    }
    ok;
}
const char *suite_name = "native-c/CBUG-0004";
struct test tests[] = {{"escape_uses_actual_cold_protection", scenario}, {NULL, NULL}};
