/*
 * When can loop 1 of borg_test_stuff() change loop 2's choice?
 *
 * angband e473720ed src/borg/borg-item-wear.c:borg_test_stuff() (84-245)
 * runs two loops that share b_i / b_v and the test `if (v <= b_v) continue;`.
 * Loop 1 scans INVEN_WIELD .. QUIVER_END-1 (equipment + quiver); loop 2
 * scans 0 .. QUIVER_END-1 (pack + equipment + quiver), so every loop-1
 * candidate is scanned again by loop 2.
 *
 * The scoring bodies and borg_item_note_needs_id() are the same
 * transcription as teststuff_test.c (see there for line numbers).
 *
 * Result shown here:
 *   - for ordinary candidates (needs_ident=1, ident=0) the loop-1 score is
 *     value+20000 and the same item's loop-2 score is at least that, so
 *     loop 1 changes loop 2's choice only by winning a TIE: `v <= b_v`
 *     keeps the loop-1 (equipment/quiver) item, while loop 2 alone, which
 *     scans the pack first, would keep an equal-scoring pack item;
 *   - the exception is a cursed non-artifact random-power ego in the
 *     weapon or bow slot (ident forced true by borg_notice_equipment()):
 *     its loop-1 score is value+100000 (value is 0 for a cursed item,
 *     borg-item-analyze.c:620-622) and its loop-2 score is lower, so its
 *     loop-1 score can block a loop-2 candidate.
 */
#include <stdio.h>

#define ART_BONUS  150000L
#define EGO_BONUS  100000L
#define NOTE_BONUS  20000L

enum { K_WEAPON, K_RING };

struct item {
	const char *name;
	int in_loop1;        /* equipment or quiver */
	long value;
	int art_idx, random_ego, ident, needs_ident, wieldable, aware, kind;
};

static int note_needs_id(const struct item *it)
{
	if (it->ident) return 0;
	if (it->needs_ident) return 1;
	return 0;
}

static int worth_id(const struct item *it)
{
	return note_needs_id(it) ? it->wieldable : 0;
}

static long l1(const struct item *it)
{
	long v = 0;
	if (!it->needs_ident) return 0;
	if (it->art_idx) v = it->value + ART_BONUS;
	if (it->random_ego) v = it->value + EGO_BONUS;
	if (note_needs_id(it)) v = it->value + NOTE_BONUS;
	return v;
}

static long l2(const struct item *it, int free_id, int maxdepth)
{
	long v = 0;
	if (!it->needs_ident) return 0;
	if (it->art_idx) v = it->value + ART_BONUS;
	if (note_needs_id(it)) v = it->value + NOTE_BONUS;
	else if (free_id || worth_id(it)) v = it->value;
	if (!it->aware && it->kind == K_RING) v += maxdepth * 5000L;
	return v;
}

/* items[] in index order: pack first, then equipment */
static void run(const char *title, const struct item *items, int n,
                int free_id, int maxdepth)
{
	int i;
	long b_v = -1, v;
	const char *who = "(none)";

	printf("%s\n", title);
	for (i = 0; i < n; i++)
		printf("    %-44s loop1 %7ld  loop2 %7ld\n", items[i].name,
			items[i].in_loop1 ? l1(&items[i]) : 0,
			l2(&items[i], free_id, maxdepth));

	for (i = 0; i < n; i++) {
		if (!items[i].in_loop1) continue;
		v = l1(&items[i]);
		if (!v || v <= b_v) continue;
		b_v = v; who = items[i].name;
	}
	printf("  after loop 1 : b_v %7ld  %s\n", b_v, who);
	for (i = 0; i < n; i++) {
		v = l2(&items[i], free_id, maxdepth);
		if (!v || v <= b_v) continue;
		b_v = v; who = items[i].name;
	}
	printf("  after loop 2 : b_v %7ld  %s\n", b_v, who);

	b_v = -1; who = "(none)";
	for (i = 0; i < n; i++) {
		v = l2(&items[i], free_id, maxdepth);
		if (!v || v <= b_v) continue;
		b_v = v; who = items[i].name;
	}
	printf("  loop 2 alone : b_v %7ld  %s\n\n", b_v, who);
}

int main(void)
{
	int depth = 10;

	{
		struct item it[] = {
			{ "pack: unaware ring", 0, 100, 0, 0, 0, 1, 1, 0, K_RING },
			{ "worn: artifact needing ID", 1, 5000, 1, 0, 0, 1, 1, 1, K_WEAPON },
		};
		run("=== A. ordinary candidates (BI_MAXDEPTH 10, free_id 0) ===", it, 2, 0, depth);
	}
	{
		struct item it[] = {
			{ "pack: unaware ring", 0, 100, 0, 0, 0, 1, 1, 0, K_RING },
			{ "worn: CURSED random-power ego weapon", 1, 0, 0, 1, 1, 1, 1, 1, K_WEAPON },
		};
		run("=== B. the exception: cursed non-artifact ego in the weapon slot ===", it, 2, 0, depth);
	}
	{
		struct item it[] = {
			{ "pack: plain wieldable item needing ID", 0, 100, 0, 0, 0, 1, 1, 1, K_WEAPON },
			{ "worn: plain item needing ID, same value", 1, 100, 0, 0, 0, 1, 1, 1, K_WEAPON },
		};
		run("=== C. a tie between a pack item and a worn item ===", it, 2, 0, depth);
	}
	printf("In A loop 1 makes no difference.  In B the cursed weapon's loop-1\n");
	printf("score (0+100000) blocks the unaware ring (100+20000+50000) that\n");
	printf("loop 2 alone would have chosen.  In C the scores tie at 20100 and\n");
	printf("loop 1 keeps the worn item, where loop 2 alone keeps the pack item.\n");
	return 0;
}
