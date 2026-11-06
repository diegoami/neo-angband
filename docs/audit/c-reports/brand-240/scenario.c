/* Synthetic native C brand-need diagnostic, excluded from the source fix. */
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
#include "borg/borg-item-activation.h"
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

static struct object *known_kind(struct object_kind *kind, int number)
{
    struct object *obj = object_new();
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
    const char *profile = getenv("N240_PROFILE");
    const char *mode = getenv("N240_MODE");
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
    /* A druid owns Brand Ammunition; a long bow gives the shooter
     * power of three the brand need requires. */
    if (!player_make_simple("Human", "Druid", "BrandNeed")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    screen.offset_x = screen.offset_y = 0;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    for (int b = 0; b < player->class->magic.num_books; ++b)
        known_kind(lookup_kind(player->class->magic.books[b].tval,
            player->class->magic.books[b].sval), 1);
    for (int pass = 0; pass < 4; pass++) {
        for (struct object *obj = player->gear; obj; obj = obj->next) {
            if (obj->tval != TV_LIGHT) continue;
            if (player->body.slots[wield_slot(obj)].obj == obj) continue;
            inven_wield(obj, wield_slot(obj));
            break;
        }
    }
    /* The long bow (shooter power three) and the brandable stack: five
     * identified plain arrows.  The control carries only two. */
    {
        struct object *bow = known_kind(lookup_kind(TV_BOW, lookup_sval(TV_BOW, "Long Bow")), 1);
        inven_wield(bow, wield_slot(bow));
        /* A ring carrying the firebrand activation satisfies the brand
         * ability the need test looks for. */
        struct object *brandring = known_kind(lookup_kind(TV_RING, lookup_sval(TV_RING, "Protection")), 1);
        struct activation *act = &activations[1];
        while (act) {
            if (!my_stricmp(act->name, "FIREBRAND")) break;
            act = act->next;
        }
        if (!act) quit("fixture activation missing");
        brandring->activation = act;
        brandring->timeout = 0;
        inven_wield(brandring, wield_slot(brandring));
        if (!mode || streq(mode, "brand"))
            known_kind(lookup_kind(TV_ARROW, lookup_sval(TV_ARROW, "Arrow")), 5);
        else
            known_kind(lookup_kind(TV_ARROW, lookup_sval(TV_ARROW, "Arrow")), 2);
    }
    player_place(cave, player, loc(11, 11));
    player_exp_gain(player, player_exp[49] * player->expfact / 100);
    while (player_stat_inc(player, STAT_WIS)) {}
    player_learn_all_runes(player); calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA | PU_SPELLS;
    update_stuff(player);
    player->chp = player->mhp; player->csp = player->msp;
    for (int i = 0; i < player->class->magic.total_spells; ++i)
        spell_learn(i);
    borg_init();
    borg_update();
    borg_alloc_detection();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_do_spell = true;
    borg_cheat_spells();
    borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg.power = borg_power();
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=1000 class=%s level=%d ammo_power=%d ammo_tval=%d brand_legal=%d\n",
        player->class->name, player->lev, borg.trait[BI_AMMO_POWER],
        borg.trait[BI_AMMO_TVAL],
        (int)(borg_equips_item(act_firebrand, false)
              || borg_spell_legal_fail(BRAND_AMMUNITION, 65)));
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    const char *mode = getenv("N240_MODE");
    if (!mode) mode = "brand";
    printf("MODE %s\n", mode);
    printf("GEOMETRY ammo_power=%d arrows=%s\n", borg.trait[BI_AMMO_POWER],
        streq(mode, "brand") ? "5" : "2");
    int need = borg.trait[BI_NEED_BRAND_WEAPON];
    printf("RATING need_brand=%d\n", need);
    printf("OUTCOME need_brand=%d\n", need);
    if (streq(mode, "brand")) {
        /* Correct contract: one qualifying stack needs the brand once,
         * ten points. */
        eq(need, 10);
    } else {
        /* Control: two arrows do not qualify (five needed). */
        eq(need, 0);
    }
    ok;
}
const char *suite_name = "native-c/brand-240";
struct test tests[] = {{"brand_need_counts_once", scenario}, {NULL, NULL}};
