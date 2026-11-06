/* Synthetic native C preparation scenario; excluded from the source fix. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "cmd-core.h"
#include "cmds.h"
#include "game-event.h"
#include "game-world.h"
#include "init.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-util.h"
#include "obj-tval.h"
#include "option.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-spell.h"
#include "player-timed.h"
#include "player-util.h"
#include "ui-term.h"
#include "ui-menu.h"
#include "borg/borg.h"
#include "borg/borg-init.h"
#include "borg/borg-item.h"
#include "borg/borg-inventory.h"
#include "borg/borg-trait.h"
#include "borg/borg-magic.h"
#include "borg/borg-fight-perm.h"
#include "borg/borg-io.h"
#include "borg/borg-cave.h"
#include "borg/borg-think.h"
#include <stdio.h>
#include <stdlib.h>

static term screen;
static const char *mode;
static int regen, haste;

static void message(game_event_type type, game_event_data *data, void *user)
{
    if (data->message.msg) printf("ENGINE %s\n", data->message.msg);
}

static struct object *item(int tval, const char *name)
{
    struct object *obj = object_new();
    object_prep(obj, lookup_kind(tval, lookup_sval(tval, name)), 50, MAXIMISE);
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
    const char *profile = getenv("CBUG0094_PROFILE");
    if (!profile) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) return 1;
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(1000);
    if (!player_make_simple("Human", "Druid", "NativePreparation")) return 1;
    term_init(&screen, 160, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 50;
    character_dungeon = true;
    for (int y = 1; y < 23; ++y) for (int x = 1; x < 23; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    player_exp_gain(player, 50000000);
    while (player_stat_inc(player, STAT_WIS)) {}
    struct object *ring = item(TV_AMULET, "Wisdom");
    inven_wield(ring, wield_slot(ring));
    item(TV_NATURE_BOOK, "[Nature Craft]");
    player_learn_all_runes(player);
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA | PU_SPELLS;
    update_stuff(player);
    player->chp = player->mhp; player->csp = player->msp;
    mode = getenv("CBUG0094_MODE"); if (!mode) mode = "regen-only";
    regen = haste = -1;
    for (int i = 0; i < player->class->magic.total_spells; ++i) {
        const struct class_spell *s = spell_by_index(player, i);
        if (streq(s->name, "Rapid Regeneration")) regen = i;
        if (streq(s->name, "Haste Self")) haste = i;
    }
    if (regen < 0 || haste < 0) return 1;
    if (!streq(mode, "unlearned")) spell_learn(regen);
    if (streq(mode, "speed-wins") || streq(mode, "speed-only")) spell_learn(haste);
    if (streq(mode, "speed-only")) player->spell_flags[regen] &= ~PY_SPELL_LEARNED;
    if (streq(mode, "already-active")) player_inc_timed(player, TMD_HEAL, 7, true, true, true);
    if (streq(mode, "blind")) player_inc_timed(player, TMD_BLIND, 10, true, true, true);
    if (streq(mode, "confused")) player_inc_timed(player, TMD_CONFUSED, 10, true, true, true);
    if (streq(mode, "town")) player->depth = cave->depth = player->cave->depth = 0;
    if (streq(mode, "low-mana")) player->csp = player->msp / 2;
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) return 1;
    borg_equipment(); borg_inventory(); borg_do_spell = true; borg_notice(true);
    borg.c = player->grid;
    borg_cheat_spells();
    printf("SETUP seed=1000 class=%s level=%d depth=%d hp=%d/%d sp=%d/%d wisdom=%d amulet_wis=%d\n",
        player->class->name, player->lev, player->depth, player->chp, player->mhp,
        player->csp, player->msp, player->state.stat_use[STAT_WIS], ring->modifiers[OBJ_MOD_WIS]);
    printf("ELIGIBILITY learned=%d native_fail=%d borg_fail=%d cost=%d okay=%d\n",
        !!(player->spell_flags[regen] & PY_SPELL_LEARNED), spell_chance(regen),
        borg_spell_fail_rate(RAPID_REGENERATION), borg_get_spell_power(RAPID_REGENERATION),
        borg_spell_okay_fail(RAPID_REGENERATION, 5));
    event_add_handler(EVENT_MESSAGE, message, NULL);
    return 0;
}

int teardown_tests(void *state)
{
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    bool affected = streq(mode, "regen-only") || streq(mode, "speed-wins");
    int expected = affected || streq(mode, "speed-only") ? 1 : 0;
    int chosen = streq(mode, "speed-wins") || streq(mode, "speed-only") ? haste : regen;
    printf("MODE %s\n", mode);
    require(player->lev == 50 && player->mhp >= 100 && player->msp > 210);
    if (affected) require(borg_spell_okay_fail(RAPID_REGENERATION, 5));
    int start_sp = player->csp, count = 0, energy = 0, regen_count = 0, haste_count = 0;
    bool decision = borg_perma_spell();
    keycode_t keys[64]; int len = 0; keycode_t key;
    while ((key = borg_inkey(true)) != 0 && len < 64) keys[len++] = key;
    printf("DECISION result=%d keys=%d no_rest_prep=%d sequence=", decision, len, borg.no_rest_prep);
    for (int i = 0; i < len; ++i) printf("%c", (char)keys[i]);
    putchar('\n');
    require(len % 3 == 0);
    for (int i = 0; i < len; i += 3) {
        require(keys[i] == 'm');
        int inv = -1, offset = -1;
        for (int j = 0; j < z_info->pack_size; ++j) if ((keycode_t)all_letters_nohjkl[j] == keys[i+1]) inv = j;
        for (int j = 0; j < 26; ++j) if ((keycode_t)all_letters_nohjkl[j] == keys[i+2]) offset = j;
        require(inv >= 0 && offset >= 0);
        struct object *book_obj = player->upkeep->inven[inv];
        require(book_obj && tval_is_book_k(book_obj->kind));
        const struct class_book *book = player_object_to_book(player, book_obj);
        require(book && offset < book->num_spells);
        int sidx = book->spells[offset].sidx;
        require(spell_okay_to_cast(player, sidx));
        struct command cmd = { .code = CMD_CAST, .context = CTX_GAME };
        cmd_set_arg_choice(&cmd, "spell", sidx);
        player->upkeep->energy_use = 0;
        int before = player->csp;
        do_cmd_cast(&cmd);
        cmd_release(&cmd);
        energy += player->upkeep->energy_use; count++;
        if (sidx == regen) regen_count++;
        if (sidx == haste) haste_count++;
        printf("CAST spell=%s sp_before=%d sp_after=%d energy=%d heal=%d fast=%d\n",
            spell_by_index(player, sidx)->name, before, player->csp,
            player->upkeep->energy_use, player->timed[TMD_HEAL], player->timed[TMD_FAST]);
    }
    printf("OUTCOME commands=%d regen=%d haste=%d mana_spent=%d action_energy=%d heal=%d fast=%d\n",
        count, regen_count, haste_count, start_sp-player->csp, energy,
        player->timed[TMD_HEAL], player->timed[TMD_FAST]);
    eq(count, expected);
    eq(start_sp-player->csp, expected ? spell_by_index(player, chosen)->smana : 0);
    eq(energy, expected * z_info->move_energy);
    if (expected) {
        eq(regen_count, chosen == regen ? 1 : 0);
        eq(haste_count, chosen == haste ? 1 : 0);
        require(player->timed[chosen == regen ? TMD_HEAL : TMD_FAST] > 0);
    }
    ok;
}
const char *suite_name = "native-c/CBUG-0094";
struct test tests[] = {{"preparation_has_one_action", scenario}, {NULL, NULL}};
