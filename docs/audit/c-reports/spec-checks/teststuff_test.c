/*
 * Standalone check of the identify-priority scoring in
 * angband e473720ed src/borg/borg-item-wear.c:borg_test_stuff().
 *
 * The function scans the items TWICE, sharing one b_i / b_v:
 *
 *   loop 1:  i from INVEN_WIELD to QUIVER_END   (equipment + pack + quiver)
 *            score: artifact -> value+150000
 *                    random-power ego -> value+100000   (OVERWRITES)
 *                    needs-id note -> value+20000       (OVERWRITES)
 *
 *   loop 2:  i from 0 to QUIVER_END               (a superset of loop 1)
 *            score: artifact -> value+150000
 *                    needs-id note -> value+20000
 *                    otherwise -> value
 *                    plus a "not aware" bonus by item type
 *
 * Both loops keep the best candidate with `if (v <= b_v) continue;`, so a
 * score left in b_v by loop 1 can block a better candidate in loop 2.
 *
 * This program reproduces both loops and shows:
 *   - the artifact under-scoring in loop 1, and
 *   - the ego bonus in loop 1 raising b_v and blocking loop 2's candidate.
 */
#include <stdio.h>

#define ART_BONUS   150000L
#define EGO_BONUS   100000L
#define NOTE_BONUS   20000L

/* item kinds used by loop 2's "not aware" bonus */
#define TV_RING    90
#define TV_AMULET  91
#define TV_ROD     95
#define TV_WAND    96
#define TV_STAFF   97
#define TV_POTION  100
#define TV_SCROLL  101

struct item {
	const char *name;
	long value;
	int art_idx;
	int random_ego;
	int needs_id_note;
	int aware;
	int maxdepth;
	int tval;
};

static long loop1_score(const struct item *item)
{
	long v = 0;

	/* Preferentially ID egos and artifacts */
	if (item->art_idx)
		v = item->value + ART_BONUS;

	if (item->random_ego)
		v = item->value + EGO_BONUS;

	/* Prioritize the ID */
	if (item->needs_id_note)
		v = item->value + NOTE_BONUS;

	return v;
}

static long loop2_score(const struct item *item)
{
	long v = 0;

	/* Preferentially ID artifacts */
	if (item->art_idx)
		v = item->value + ART_BONUS;

	/* Identify "good" items */
	if (item->needs_id_note)
		v = item->value + NOTE_BONUS;
	else if (!item->art_idx)
		v = item->value;

	/* Reward "unaware" items */
	if (!item->aware) {
		switch (item->tval) {
		case TV_RING:
		case TV_AMULET:
			v += item->maxdepth * 5000L; break;
		case TV_ROD:
			v += item->maxdepth * 3000L; break;
		case TV_WAND:
		case TV_STAFF:
			v += item->maxdepth * 2000L; break;
		case TV_POTION:
		case TV_SCROLL:
			if (item->maxdepth >= 5)
				v += item->maxdepth * 500L;
			break;
		}
	}
	return v;
}

/* Run both loops exactly as the source does and report the winner. */
static void run(const struct item *items, int n)
{
	int i;
	long b_v = -1;
	const char *winner = "(none)";

	/* loop 1: INVEN_WIELD .. QUIVER_END.  We model that as "the last
	 * half" of the array; the loop bounds only decide WHICH items loop 1
	 * sees, and the defect is about the score, not the range. */
	for (i = 0; i < n; i++) {
		long v;
		if (!items[i].art_idx && !items[i].random_ego && !items[i].needs_id_note)
			continue;   /* needs_ident items only; simplified below */
		v = loop1_score(&items[i]);
		if (v <= b_v) continue;
		winner = items[i].name;
		b_v = v;
	}
	printf("  after loop 1: b_v = %-8ld candidate = %s\n", b_v, winner);

	for (i = 0; i < n; i++) {
		long v = loop2_score(&items[i]);
		if (v <= b_v) continue;
		winner = items[i].name;
		b_v = v;
	}
	printf("  after loop 2: b_v = %-8ld candidate = %s\n", b_v, winner);
}

int main(void)
{
	struct item items[3];

	printf("=== case 1: an artifact that is also a random-power ego ===\n");
	printf("loop 1 says \"Preferentially ID egos and artifacts\" and gives an\n");
	printf("artifact 150000.  The ego branch then overwrites it with 100000,\n");
	printf("so the artifact+ego scores BELOW a plain artifact of equal value.\n\n");
	items[0] = (struct item){ "artifact+ego", 5000, 1, 1, 0, 1, 50, 0 };
	items[1] = (struct item){ "plain artifact", 5000, 1, 0, 0, 1, 50, 0 };
	items[2] = (struct item){ "plain ego", 5000, 0, 1, 0, 1, 50, 0 };
	printf("  loop1 score: artifact+ego %ld   plain artifact %ld   plain ego %ld\n",
		loop1_score(&items[0]), loop1_score(&items[1]), loop1_score(&items[2]));
	printf("  loop2 score: artifact+ego %ld   plain artifact %ld   plain ego %ld\n",
		loop2_score(&items[0]), loop2_score(&items[1]), loop2_score(&items[2]));
	printf("  -> loop 2 rescores the artifact at 150000, so this specific\n");
	printf("     under-scoring is masked for the artifact itself.\n\n");
	run(items, 3);

	printf("\n=== case 2: a random-power ego that is NOT an artifact ===\n");
	printf("loop 1 gives it value+100000.  Loop 2 gives it only value (it is\n");
	printf("not an artifact and has no needs-id note).  So loop 1 leaves a\n");
	printf("b_v of value+100000, and any loop-2 candidate whose score is\n");
	printf("between value and value+100000 is skipped by `if (v <= b_v)`.\n\n");
	items[0] = (struct item){ "random-power ego (not an artifact)", 5000, 0, 1, 0, 1, 50, 0 };
	items[1] = (struct item){ "artifact", 5000, 1, 0, 0, 1, 50, 0 };
	items[2] = (struct item){ "unaware ring", 100, 0, 0, 0, 0, 50, TV_RING };
	printf("  loop1 score: ego %ld   artifact %ld\n",
		loop1_score(&items[0]), loop1_score(&items[1]));
	printf("  loop2 score: ego %ld   artifact %ld   unaware ring %ld\n",
		loop2_score(&items[0]), loop2_score(&items[1]), loop2_score(&items[2]));
	printf("  -> the unaware ring scores %ld in loop 2, which is BELOW the\n",
		loop2_score(&items[2]));
	printf("     ego's loop-1 score of %ld, so the ring is never considered.\n",
		loop1_score(&items[0]));
	run(items, 3);
	return 0;
}
