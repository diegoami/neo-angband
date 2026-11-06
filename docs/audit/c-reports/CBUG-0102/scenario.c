/* Synthetic native C glyph-tracker diagnostic, excluded from the source fix. */
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
#include "player-timed.h"
#include "player-util.h"
#include "ui-input.h"
#include "ui-menu.h"
#include "ui-term.h"
#include "borg/borg.h"
#include "borg/borg-cave.h"
#include "borg/borg-caution.h"
#include "borg/borg-danger.h"
#include "borg/borg-escape.h"
#include "borg/borg-fight-defend.h"
#include "borg/borg-flow-glyph.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item-use.h"
#include "borg/borg-item.h"
#include "borg/borg-magic.h"
#include "borg/borg-think.h"
#include "borg/borg-trait.h"
#include <stdio.h>
#include <stdlib.h>

static term screen;
static struct monster *enemy;
static const char *mode;
static struct object *unknown_scroll;
static int failed_floor_messages;
static int engine_messages;

static void boundary(void)
{
    struct keypress key;
    Term_erase(0, 0, 255);
    borg_keypress(ESCAPE);
    inkey_flag = true;
    key = inkey_hack(0);
    if (key.code != ESCAPE) quit("observer boundary did not drain queued key");
}

static void message(game_event_type type, game_event_data *data, void *user)
{
    const char *text = data->message.msg;
    if (!text) return;
    ++engine_messages;
    printf("ENGINE turn=%ld hp=%d: %s\n", (long)turn, player->chp, text);
    if (prefix(text, "There is no clear floor")) ++failed_floor_messages;
    /* Headless message renderer. The real Borg scrapes/parses this line. */
    Term_erase(0, 0, 255);
    Term_putstr(0, 0, -1, COLOUR_WHITE, text);
    inkey_flag = true;
    if (inkey_hack(0).code != ' ') quit("observer message was not read");
    boundary();
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

static struct object *unknown_item(int tval, const char *name, int number)
{
    struct object *obj = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) quit("fixture item kind missing");
    object_prep(obj, kind, 50, MAXIMISE);
    obj->number = number;
    obj->known = object_new();
    object_set_base_known(player, obj);
    /* No object_flavor_aware(): the Borg has not learned this flavor. */
    inven_carry(player, obj, false, false);
    return obj;
}

int setup_tests(void **state)
{
    const char *profile = getenv("CBUG0102_PROFILE");
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
    if (!player_make_simple("Human", "Warrior", "GlyphTrackGuard")) { printf("SETUPFAIL birth\n"); return 1; }
    printf("SETUPSTEP birth\n");
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* Light source with fuel: not nasty, no fuel branch. */
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    mode = getenv("CBUG0102_MODE"); if (!mode) mode = "stairs";
    struct loc start = streq(mode, "pairing") ? loc(10, 10) : loc(11, 11);
    player_place(cave, player, start);
    if (streq(mode, "stairs"))
        square_set_feat(cave, start, FEAT_OPEN); /* doorway: not trappable */
    /* Pack: in pairing mode a known Rune of Protection is carried from the
     * start; otherwise the only rune scroll is an unidentified one whose
     * kind the player has not learned (flavor awareness is per kind). */
    if (streq(mode, "pairing"))
        known_item(TV_SCROLL, "Rune of Protection", 1);
    else
        unknown_scroll = unknown_item(TV_SCROLL, "Rune of Protection", 1);
    player_learn_all_runes(player); calc_inventory(player);
    /* The starting Word of Recall scroll was consumed reaching the dungeon
     * (fixture narrative); drop it so the emergency cannot recall out. */
    for (int i = 0; i < z_info->pack_size; ++i) {
        struct object *o = player->upkeep->inven[i];
        if (o && o->tval == TV_SCROLL
            && o->sval == lookup_sval(TV_SCROLL, "Word of Recall")) {
            inven_drop(o, o->number);
            break;
        }
    }
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA | PU_SPELLS;
    update_stuff(player);
    player->chp = 8; /* wounded: pos_danger must exceed four times this */
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg init=%d cheat_death=%d\n", borg_init_failure, borg_cfg[BORG_CHEAT_DEATH]); return 1; }
    printf("SETUPSTEP borg\n");
    borg_equipment(); borg_inventory(); borg_do_spell = true; borg_notice(true);
    borg.c = player->grid; borg.avoidance = 180;
    borg.ready_morgoth = 0; /* fresh low-level warrior is not Morgoth-ready */
    borg_cheat_spells();
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x)
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
    enemy = t_add_monster(cave, loc(11, 10), "mage");
    if (!enemy) { printf("SETUPFAIL monster\n"); return 1; }
    printf("SETUPSTEP monster %s\n", enemy->race->name);
    update_view(cave, player);
    mflag_on(enemy->mflag, MFLAG_AWARE); update_mon(enemy, cave, true);
    if (!mflag_has(enemy->mflag, MFLAG_VISIBLE)) {
        printf("SETUPFAIL visible d=%d view=%d seen=%d glow=%d infra=%d torch=%d blind=%d max_sight=%d invisible=%d\n",
            distance(enemy->grid, player->grid),
            square_isview(cave, enemy->grid), square_isseen(cave, enemy->grid),
            sqinfo_has(square(cave, enemy->grid)->info, SQUARE_GLOW),
            player->state.see_infra, player->state.cur_light,
            player->timed[TMD_BLIND], z_info->max_sight,
            monster_is_invisible(enemy));
        return 1;
    }
    if (!observe_kill_diff(enemy->grid.y, enemy->grid.x, 0, 0)) { printf("SETUPFAIL observe\n"); return 1; }
    int kill_index = borg_grids[enemy->grid.y][enemy->grid.x].kill;
    if (kill_index <= 0) { printf("SETUPFAIL kill_index=%d\n", kill_index); return 1; }
    borg_kills[kill_index].awake = 1;
    borg.status.active = true;
    borg_update_entrypoint(true);
    event_add_handler(EVENT_MESSAGE, message, NULL);
    printf("SETUP seed=1000 class=%s level=%d depth=%d hp=%d/%d sp=%d/%d mode=%s\n",
        player->class->name, player->lev, player->depth, player->chp, player->mhp,
        player->csp, player->msp, mode);
    printf("THREAT name=%s kill_index=%d awake=%d ranged=%d\n", enemy->race->name,
        kill_index, borg_kills[kill_index].awake, borg_kills[kill_index].ranged_attack);
    printf("SPELLS ");
    for (int i = 0; i < borg_kills[kill_index].ranged_attack; i++)
        printf("%d,", borg_kills[kill_index].spell[i]);
    putchar('\n');
    printf("GRID player=(%d,%d) feat=%d trappable=%d mage=(%d,%d)\n",
        borg.c.x, borg.c.y, square(cave, player->grid)->feat,
        square_istrappable(cave, player->grid), enemy->grid.x, enemy->grid.y);
    printf("TRACK start num=%d size=%d\n", track_glyph.num, track_glyph.size);
    return 0;
}

int teardown_tests(void *state)
{
    event_remove_handler(EVENT_MESSAGE, message, NULL);
    borg_update_entrypoint(false);
    printf("TRACK teardown num=%d\n", track_glyph.num);
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int drain_keys(keycode_t *keys, int max)
{
    int len = 0; keycode_t key;
    while ((key = borg_inkey(true)) != 0 && len < max) keys[len++] = key;
    return len;
}

static int letter_slot(keycode_t key)
{
    for (int i = 0; i < z_info->pack_size; ++i)
        if ((keycode_t)all_letters_nohjkl[i] == key) return i;
    return -1;
}

static void native_read(struct object *obj)
{
    struct command cmd = { .code = CMD_READ_SCROLL, .context = CTX_GAME };
    cmd_set_arg_item(&cmd, "item", obj);
    do_cmd_read_scroll(&cmd);
    cmd_release(&cmd);
}

static int scenario(void *state)
{
    keycode_t keys[64];
    int len, slot, num_a = 0;
    struct object *obj;
    int start_messages = engine_messages;
    printf("MODE %s\n", mode);

    /* Phase A: the emergency caution turn reads the unknown scroll. */
    if (streq(mode, "stairs") || streq(mode, "clear-floor")) {
        int danger = borg_danger(borg.c.y, borg.c.x, 1, true, false);
        printf("DANGER phaseA current=%d emergency_gate=%d\n", danger,
            borg.trait[BI_CURHP] * 4);
        for (int i = 0; i < z_info->pack_size; ++i)
            if (borg_items[i].iqty)
                printf("BMIRROR before i=%d letter=%c tval=%d sval=%d aware=%d\n",
                    i, all_letters_nohjkl[i], borg_items[i].tval,
                    borg_items[i].sval, borg_items[i].aware);
        bool acted = borg_caution();
        printf("STATE fleeing=%d ready_morgoth=%d recalling=%d near_unique=%d leaving=%d descending=%d\n",
            borg.goal.fleeing, borg.ready_morgoth, borg.goal.recalling,
            borg.near.unique, borg.goal.leaving, borg.goal.descending);
        len = drain_keys(keys, 64);
        printf("DECISION phaseA acted=%d keys=%d sequence=", acted, len);
        for (int i = 0; i < len; ++i) {
            if (keys[i] == ESCAPE) printf("<Escape>"); else printf("%c", (char)keys[i]);
        }
        putchar('\n');
        require(acted);
        int first = 0;
        while (first < len && keys[first] == ESCAPE) first++;
        require(len - first == 3);
        require(keys[first] == 'r' && keys[first+2] == ESCAPE);
        slot = letter_slot(keys[first+1]); require(slot >= 0);
        for (int i = 0; i < z_info->pack_size; ++i) {
            struct object *o = player->upkeep->inven[i];
            if (o) printf("PACK i=%d letter=%c tval=%d sval=%d n=%d aware=%d unknown=%d borg(tval=%d sval=%d aware=%d)\n",
                i, all_letters_nohjkl[i], o->tval, o->sval, o->number,
                object_flavor_is_aware(o), o == unknown_scroll,
                borg_items[i].tval, borg_items[i].sval, borg_items[i].aware);
        }
        obj = player->upkeep->inven[slot]; require(obj == unknown_scroll);
        native_read(obj);
        printf("MESSAGE failed_floor=%d total_new=%d\n",
            failed_floor_messages, engine_messages - start_messages);
        printf("TRACK phaseA num=%d\n", track_glyph.num);
        num_a = track_glyph.num;
        /* The read taught the flavor; the Borg now carries another known
         * Rune of Protection for the later creation (fixture grant). */
        known_item(TV_SCROLL, "Rune of Protection", 1);
        /* The Borg steps off the failed grid onto open floor. */
        square_set_feat(cave, loc(10, 10), FEAT_FLOOR);
        borg_grids[10][10].feat = FEAT_FLOOR;
        player_place(cave, player, loc(10, 10));
        borg.c = player->grid;
        borg_inventory(); borg_notice(true);
        update_view(cave, player);
    }

    /* Phase B: the defend chooser creates a glyph with the known scroll. */
    int p1 = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    borg_on_glyph = true;
    int p2 = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    borg_on_glyph = false;
    printf("DANGER phaseB p1=%d p2=%d avoidance=%d\n", p1, p2, borg.avoidance);
    bool defended = borg_defend(p1);
    len = drain_keys(keys, 64);
    printf("DECISION phaseB acted=%d keys=%d sequence=", defended, len);
    for (int i = 0; i < len; ++i) {
        if (keys[i] == ESCAPE) printf("<Escape>"); else printf("%c", (char)keys[i]);
    }
    putchar('\n');
    require(defended);
    int first_b = 0;
    while (first_b < len && keys[first_b] == ESCAPE) first_b++;
    require(len - first_b == 2);
    require(keys[first_b] == 'r');
    slot = letter_slot(keys[first_b+1]); require(slot >= 0);
    obj = player->upkeep->inven[slot];
    require(obj && obj->tval == TV_SCROLL
        && obj->sval == lookup_sval(TV_SCROLL, "Rune of Protection"));
    int num_acted = track_glyph.num;
    printf("TRACK phaseB_acted num=%d\n", num_acted);
    native_read(obj);
    int num_final = track_glyph.num;
    printf("OUTCOME glyph_created=%d warded_grid=(%d,%d) hp=%d num_final=%d\n",
        square_iswarded(cave, player->grid), player->grid.x, player->grid.y,
        player->chp, num_final);
    /* Corrected contract: the failed read removed nothing, the creation is
     * tracked once, in bounds, and the native grid is warded. */
    eq(streq(mode, "stairs") ? failed_floor_messages : 0,
        streq(mode, "stairs") ? 1 : 0);
    eq(num_a, 0);
    eq(num_acted, 1);
    eq(square_iswarded(cave, player->grid) ? 1 : 0, 1);
    eq(num_final, 1);
    ok;
}
const char *suite_name = "native-c/CBUG-0102";
struct test tests[] = {{"failed_glyph_keeps_count_in_bounds", scenario}, {NULL, NULL}};
