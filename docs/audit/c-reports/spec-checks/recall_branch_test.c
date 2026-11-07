/*
 * Standalone check of the branch structure in
 * angband e473720ed src/borg/borg-escape.c:borg_recall() (55-128).
 *
 * The code below transcribes the recall-depth answer at borg-escape.c:64-117:
 * the condition that the prompt appears, the depth-window test, and the
 * five-way branch.  For each branch it records the key queued, the note the
 * Borg writes (borg_note), and the comment above the keypress, so the three
 * can be compared.  No game state is involved; inputs are synthetic.
 *
 * Inputs: maxdepth = BI_MAXDEPTH, depth = BI_CDEPTH,
 *         hunted = borg_depth_hunted_unique, liveuniq = borg_numb_live_unique,
 *         king = BI_KING, fleeing_munchkin = borg.goal.fleeing_munchkin,
 *         sauron_dead = borg_race_death[borg.id.sauron] != 0.
 */
#include <stdio.h>
#include <string.h>

struct state {
    int maxdepth, depth, hunted, liveuniq, king;
    int fleeing_munchkin;
    int sauron_dead;
};

/* returns the queued key; fills the comment the branch claims */
static char decide(const struct state *s, char *comment, size_t n, const char **note)
{
    int diff_max = s->maxdepth - s->hunted;
    int diff_cur = s->depth - s->hunted;

    if (!(s->depth < s->maxdepth && s->depth != 0)) {
        snprintf(comment, n, "(no prompt: depth %d is not below maxdepth %d)", s->depth, s->maxdepth);
        *note = "(none)";
        return 0;
    }
    if (!(diff_max > 5 && diff_cur <= 5 && diff_cur > -5)) {
        snprintf(comment, n, "else branch: Do not reset Depth");
        *note = "# Not resetting recall depth.";
        return 'n';
    }
    if (s->depth >= 80 && s->depth < 100 && s->sauron_dead) {
        snprintf(comment, n, "Do reset Depth");
        *note = "# Resetting recall depth.";
        return 'y';
    }
    if (s->fleeing_munchkin) {
        snprintf(comment, n, "Do reset Depth");
        *note = "# Resetting recall depth during munchkin mode.";
        return 'y';
    }
    if (s->depth >= 100 && !s->king) {
        snprintf(comment, n, "Do not reset Depth");
        *note = "# Not Resetting recall depth.";
        return 'n';
    }
    if (s->maxdepth == 99 && s->liveuniq < 5) {
        snprintf(comment, n, "Do reset Depth, cleaning up last uniques to hit Morgoth.  Want to keep using up the stock of potions");
        *note = "# Not Resetting recall depth.";
        return 'n';
    }
    snprintf(comment, n, "Do reset Depth");
    *note = "# Resetting recall depth.";
    return 'y';
}

int main(void)
{
    struct {
        const char *name;
        struct state s;
    } cases[] = {
        /* maxdepth depth hunted liveuniq king munchkin sauron_dead */
        { "A: TARGET maxdepth 99, depth 95, hunted 92, 3 uniques alive",
          { 99, 95, 92, 3, 0, 0, 0 } },
        { "B: TARGET maxdepth 99, depth 88, hunted 85, 4 uniques alive",
          { 99, 88, 85, 4, 0, 0, 0 } },
        { "C: control, depth 95, Sauron dead (Sauron branch)",
          { 99, 95, 92, 3, 0, 0, 1 } },
        { "D: control, maxdepth 99, 5 uniques alive (final else)",
          { 99, 95, 92, 5, 0, 0, 0 } },
        { "E: control, depth far from the hunted depth (window fails)",
          { 60, 40, 30, 3, 0, 0, 0 } },
        { "F: control, fleeing munchkin (munchkin branch)",
          { 99, 92, 90, 3, 0, 1, 0 } },
        { "G: control, depth 100 below maxdepth 110, not king (depth>=100 branch)",
          { 110, 100, 98, 3, 0, 0, 0 } },
        { "H: control, depth not below maxdepth (no prompt)",
          { 50, 50, 45, 3, 0, 0, 0 } },
    };
    int i;
    printf("%-72s %s  %s\n", "case", "key", "note written by the code");
    for (i = 0; i < (int)(sizeof(cases)/sizeof(cases[0])); i++) {
        char comment[256];
        const char *note = "";
        char k = decide(&cases[i].s, comment, sizeof(comment), &note);
        printf("%-72s %c    %s\n", cases[i].name, k ? k : '-', note);
        printf("%-72s      comment: %s\n", "", comment);
        if (strstr(comment, "Do reset Depth") && k == 'n')
            printf("%-72s      *** comment says reset; key and note say not ***\n", "");
        printf("\n");
    }
    return 0;
}
