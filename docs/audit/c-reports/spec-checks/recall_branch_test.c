/*
 * Standalone check of the branch structure in
 * angband e473720ed src/borg/borg-escape.c:borg_recall().
 *
 * The code below is a transcription of the branch at borg-escape.c:64-115.
 * It is used to show which key each branch queues and how that compares
 * with the branch's own comment.  No game state is involved.
 */
#include <stdio.h>
#include <string.h>

struct state {
    int maxdepth, depth, hunted, liveuniq, king;
    int fleeing_munchkin;
    int sauron_dead;
};

/* returns the queued key; fills the comment the branch claims */
static char decide(const struct state *s, char *comment, size_t n)
{
    int diff_max = s->maxdepth - s->hunted;
    int diff_cur = s->depth - s->hunted;

    if (!(s->depth < s->maxdepth && s->depth != 0)) {
        snprintf(comment, n, "(no prompt: depth %d is not below maxdepth %d)", s->depth, s->maxdepth);
        return 0;
    }
    if (!(diff_max > 5 && diff_cur <= 5 && diff_cur > -5)) {
        snprintf(comment, n, "else branch: Do not reset Depth");
        return 'n';
    }
    if (s->depth >= 80 && s->depth < 100 && s->sauron_dead) {
        snprintf(comment, n, "Do reset Depth");
        return 'y';
    }
    if (s->fleeing_munchkin) {
        snprintf(comment, n, "Do reset Depth");
        return 'y';
    }
    if (s->depth >= 100 && !s->king) {
        snprintf(comment, n, "Do not reset Depth");
        return 'n';
    }
    if (s->maxdepth == 99 && s->liveuniq < 5) {
        snprintf(comment, n, "Do reset Depth, cleaning up last uniques to hit Morgoth.  Want to keep using up the stock of potions");
        return 'n';
    }
    snprintf(comment, n, "Do reset Depth");
    return 'y';
}

int main(void)
{
    struct {
        const char *name;
        struct state s;
    } cases[] = {
        { "A: TARGET: MAXDEPTH=99, depth 75, <5 uniques alive (the maxdepth==99 branch)",
          { 99, 75, 75, 3, 0, 0, 0 } },
        { "B: TARGET, depth 78, <5 uniques alive",
          { 99, 78, 75, 3, 0, 0, 0 } },
        { "C: control, depth 95 (>=80 and <100), Sauron dead (the sauron branch wins first)",
          { 99, 95, 90, 3, 0, 0, 1 } },
        { "C2: control, depth 95, Sauron NOT dead, <5 uniques alive",
          { 99, 95, 90, 3, 0, 0, 0 } },
        { "D: control, MAXDEPTH=99, 5 uniques alive (the condition is false)",
          { 99, 98, 90, 5, 0, 0, 0 } },
        { "E: control, MAXDEPTH=60 (the condition is false)",
          { 60, 58, 30, 3, 0, 0, 0 } },
        { "F: control, munchkin fleeing (the munchkin branch wins first)",
          { 99, 98, 90, 3, 0, 1, 0 } },
        { "G: control, depth >= 100 and not king (the depth>=100 branch wins)",
          { 120, 100, 90, 3, 0, 0, 0 } },
    };
    int i;
    printf("%-72s %-8s %-8s %s\n", "case", "queued", "comment", "verdict");
    for (i = 0; i < (int)(sizeof(cases)/sizeof(cases[0])); i++) {
        char comment[256];
        char k = decide(&cases[i].s, comment, sizeof(comment));
        const char *verdict = "";
        if (k == 'y') verdict = "resets the recall depth";
        else if (k == 'n') verdict = "leaves the recall depth alone";
        printf("%-72s %-8c %-8s %s\n", cases[i].name,
            k ? k : '-', k ? "y/n" : "-", verdict);
        printf("%-72s comment: %s\n", "", comment);
        if (strstr(comment, "Do reset Depth") && k == 'n')
            printf("%-72s   *** COMMENT SAYS RESET, CODE SENDS 'n' ***\n", "");
        printf("\n");
    }
    return 0;
}
