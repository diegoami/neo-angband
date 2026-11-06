/* Synthetic native C screen-text diagnostic, excluded from the source fix. */
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
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;

int setup_tests(void **state)
{
    const char *profile = getenv("T287_PROFILE");
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
    if (!player_make_simple("Human", "Warrior", "ScreenText")) { printf("SETUPFAIL birth\n"); return 1; }
    /* The multibyte conversion only widens in a multibyte locale. */
    if (!setlocale(LC_CTYPE, "C.UTF-8")) { printf("SETUPFAIL locale\n"); return 1; }
    printf("SETUP mb_cur_max=%d\n", (int)MB_CUR_MAX);
    if (MB_CUR_MAX < 2) { printf("SETUPFAIL mb encoding\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    screen.offset_x = screen.offset_y = 0;
    borg_init();
    if (borg_init_failure) { printf("SETUPFAIL borg\n"); return 1; }
    return 0;
}

int teardown_tests(void *state)
{
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static void paint_row(const wchar_t *text, int n)
{
    for (int i = 0; i < n; i++) {
        Term->scr->c[0][i] = text[i];
        Term->scr->a[0][i] = 1;
    }
}

static int scenario(void *state)
{
    const char *mode = getenv("T287_MODE");
    if (!mode) mode = "multibyte";
    printf("MODE %s\n", mode);

    /* A row of eighty characters.  In the multibyte case exactly one of
     * them occupies two bytes, so the row needs eighty-one bytes plus
     * its terminator: one byte past the wide-count limit the
     * conversion used. */
    wchar_t row[80];
    for (int i = 0; i < 80; i++)
        row[i] = (streq(mode, "multibyte") && i == 5) ? 0x00FB : L'A';
    paint_row(row, 80);

    /* The caller's buffer, here deliberately junked with no zero
     * anywhere: whatever the conversion fails to terminate will read
     * as stale text. */
    char buffer[1024];
    memset(buffer, 'X', sizeof(buffer));
    uint8_t attr = 0;
    errr rc = borg_what_text(0, 0, 80, &attr, buffer);

    size_t len = strnlen(buffer, sizeof(buffer));
    int terminated = len < sizeof(buffer);
    printf("GEOMETRY rc=%d strnlen=%lu terminated=%d\n", rc,
        (unsigned long)len, terminated);
    printf("RATING terminated=%d strnlen=%lu\n", terminated, (unsigned long)len);
    printf("OUTCOME terminated=%d strnlen=%lu\n", terminated, (unsigned long)len);
    if (streq(mode, "multibyte")) {
        /* Correct contract: the converted text is terminated at or
         * before its own length, never left to read stale bytes. */
        require(terminated);
        require(len <= 81);
    } else {
        /* Control: the pure-ASCII row fits the old limit too; both
         * variants terminate identically. */
        require(terminated);
        eq((int)len, 80);
    }
    ok;
}
const char *suite_name = "native-c/text-287";
struct test tests[] = {{"converted_screen_text_is_terminated", scenario}, {NULL, NULL}};
