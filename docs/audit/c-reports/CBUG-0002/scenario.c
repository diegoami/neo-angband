/* Diagnostic native-game observer; excluded from the proposal diff. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "cmd-core.h"
#include "game-event.h"
#include "game-world.h"
#include "generate.h"
#include "init.h"
#include "mon-make.h"
#include "mon-util.h"
#include "monster.h"
#include "player.h"
#include "option.h"
#include "player-birth.h"
#include "player-timed.h"
#include "ui-input.h"
#include "ui-term.h"
#include "borg/borg.h"
#include "borg/borg-init.h"
#include "borg/borg-io.h"
#include "borg/borg-messages.h"
#include "borg/borg-messages-react.h"
#include <stdio.h>
#include <stdlib.h>

static term screen;
static bool observing;
static int deaths;
static int knockout_messages;
static int death_reactions;

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
    int before = borg_msg_num;
    if (!observing || !text) return;
    printf("ENGINE turn=%ld stun=%d paralysis=%d: %s\n", (long)turn,
        player->timed[TMD_STUN], player->timed[TMD_PARALYZED], text);
    if (streq(text, "You have been knocked out.")) ++knockout_messages;
    /* Headless message renderer. The real Borg scrapes/parses this line. */
    Term_erase(0, 0, 255);
    Term_putstr(0, 0, -1, COLOUR_WHITE, text);
    inkey_flag = true;
    if (inkey_hack(0).code != ' ') quit("observer message was not read");
    boundary();
    if (prefix(text, "You have slain ") || prefix(text, "You have killed "))
    {
        ++deaths;
        death_reactions += borg_msg_num - before;
        printf("KILL_REACTION delta=%d\n", borg_msg_num - before);
    }
}

int setup_tests(void **state)
{
    set_file_paths();
    /* Override every writable user path before initializing/birthing. */
    string_free(ANGBAND_DIR_USER);
    string_free(ANGBAND_DIR_SAVE);
    string_free(ANGBAND_DIR_PANIC);
    string_free(ANGBAND_DIR_SCORES);
    string_free(ANGBAND_DIR_ARCHIVE);
    ANGBAND_DIR_USER = string_make("/tmp/native-c-14-evidence/profile");
    ANGBAND_DIR_SAVE = string_make("/tmp/native-c-14-evidence/profile/save");
    ANGBAND_DIR_PANIC = string_make("/tmp/native-c-14-evidence/profile/panic");
    ANGBAND_DIR_SCORES = string_make("/tmp/native-c-14-evidence/profile/scores");
    ANGBAND_DIR_ARCHIVE = string_make("/tmp/native-c-14-evidence/profile/archive");
    if (!init_angband()) return 1;
    create_needed_dirs();
    Rand_quick = false;
    Rand_state_init(1000);
    if (!player_make_simple("Human", "Warrior", "CBUG0002")) return 1;
    prepare_next_level(player);
    on_new_level();
    term_init(&screen, 255, 60, 256);
    Term_activate(&screen);
    angband_term[0] = &screen;
    borg_rand_local = 1000;
    borg_init();
    if (borg_init_failure) return 1;
    if (borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) return 1;
    printf("PROFILE %s cheat_death=%d cheat_live=%d\n", ANGBAND_DIR_USER,
        borg_cfg[BORG_CHEAT_DEATH], OPT(player, cheat_live));
    borg.status.active = true;
    borg_update_entrypoint(true);
    event_add_handler(EVENT_MESSAGE, message, NULL);
    observing = true;
    return 0;
}

int teardown_tests(void *state)
{
    observing = false;
    borg_update_entrypoint(false);
    cleanup_angband();
    term_nuke(&screen);
    angband_term[0] = NULL;
    return 0;
}

static int scenario(void *state)
{
    const char *mode = getenv("CBUG0002_MODE");
    int recovery = 0, before, after;
    int attacks = 0;
    struct monster *mon;
    struct loc grid;
    int dir;
    if (!mode) mode = "knockout";
    printf("MODE %s seed=1000 race=Human class=Warrior hp=%d depth=%d\n",
        mode, player->chp, player->depth);
    if (streq(mode, "knockout")) {
        require(player_inc_timed(player, TMD_STUN, 151, true, true, true));
        require(player_timed_grade_eq(player, TMD_STUN, "Knocked Out"));
        /* The engine forces sleep and runs time until input is possible. */
        run_game_loop();
        require(!player_timed_grade_eq(player, TMD_STUN, "Knocked Out"));
    } else if (streq(mode, "heavy-stun")) {
        require(player_inc_timed(player, TMD_STUN, 100, true, true, true));
    } else if (streq(mode, "paralysis")) {
        require(player_inc_timed(player, TMD_PARALYZED, 5, true, true, true));
        run_game_loop();
        eq(player->timed[TMD_PARALYZED], 0);
    } else {
        require(streq(mode, "none"));
    }
    /* Let remaining heavy stun expire via ordinary world processing. */
    while (player->timed[TMD_STUN] && recovery++ < 200) {
        cmdq_push(CMD_HOLD);
        run_game_loop();
    }
    eq(player->timed[TMD_STUN], 0);
    eq(player->is_dead, false);
    boundary();
    printf("RECOVERED turn=%ld hp=%d global_suppression=%d knockout_messages=%d\n",
        (long)turn, player->chp, borg_dont_react, knockout_messages);
    /* Place a legal nearby weak monster and fight using ordinary commands. */
    for (dir = 0; dir < 8; ++dir) {
        grid = loc_sum(player->grid, ddgrid_ddd[dir]);
        if (square_isempty(cave, grid)) break;
    }
    require(dir < 8);
    mon = t_add_monster(cave, grid, "Kobold");
    notnull(mon);
    update_mon(mon, cave, true);
    require(mflag_has(mon->mflag, MFLAG_VISIBLE));
    printf("COMBAT monster=%s hp=%d visible=1\n", mon->race->name, mon->hp);
    before = borg_msg_num;
    while (!deaths && !player->is_dead && attacks++ < 40) {
        cmdq_push(CMD_WALK);
        require(mflag_has(mon->mflag, MFLAG_VISIBLE));
        cmd_set_arg_direction(cmdq_peek(), "direction",
            motion_dir(player->grid, mon->grid));
        run_game_loop();
    }
    after = borg_msg_num;
    printf("OUTCOME deaths=%d reactions_before=%d reactions_after=%d delta=%d\n",
        deaths, before, after, after - before);
    require(deaths > 0);
    /* Expected corrected behavior; unfixed knockout must fail this. */
    eq(player->is_dead, false);
    eq(death_reactions, 1);
    ok;
}
const char *suite_name = "native-c/CBUG-0002";
struct test tests[] = { { "recovery_then_kill", scenario }, { NULL, NULL } };
