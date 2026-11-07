/*
 * Standalone check of the identify-priority scoring in
 * angband e473720ed src/borg/borg-item-wear.c:borg_test_stuff() (84-245).
 *
 * Transcribed from the pinned source (scoring only; the identify keys are
 * not modelled):
 *
 *   loop 1  borg-item-wear.c:101-134   i = INVEN_WIELD .. QUIVER_END-1
 *     skip !iqty; skip !needs_ident
 *     if (art_idx)                            v = value + 150000
 *     if (borg_ego_has_random_power(...))     v = value + 100000
 *     if (borg_item_note_needs_id(item))      v = value + 20000
 *     if (!v) continue;  if (v <= b_v) continue;  b_i = i; b_v = v;
 *
 *   loop 2  borg-item-wear.c:137-205   i = 0 .. QUIVER_END-1
 *     skip !iqty; skip !needs_ident
 *     if (art_idx)                            v = value + 150000
 *     if (borg_item_note_needs_id(item))      v = value + 20000
 *     else if (free_id || borg_item_worth_id(item))  v = value
 *     if (!aware) v += a per-tval bonus * BI_MAXDEPTH (TV_FOOD included)
 *     if (!v) continue;  if (v <= b_v) continue;  b_i = i; b_v = v;
 *
 * The decisive helper is transcribed too:
 *
 *   borg-item-id.c:borg_item_note_needs_id:145-155
 *     if (item->ident) return false;
 *     if (item->needs_ident) return true;
 *     return strstr(borg_get_note(item), "{??}");
 *
 * and borg-inventory.c:borg_item_worth_id:274-287
 *     if (!borg_item_note_needs_id(item)) return false;
 *     return borg_wield_slot(item) >= 0;
 *
 * Where the fields come from (borg-item-analyze.c:450-460):
 *     ident       = object_fully_known(obj)
 *     needs_ident = !object_runes_known(obj)
 * and src/obj-knowledge.c:object_fully_known() returns false unless
 * object_runes_known() is true.  So for an item fresh from
 * borg_item_analyze(), needs_ident implies !ident, and therefore
 * borg_item_note_needs_id() is TRUE for every item that passes the
 * `!needs_ident` filter of both loops.
 *
 * The one way ident can be true together with needs_ident at scoring time
 * is borg-trait.c:borg_notice_equipment:1868-1876 / 1947-1955: for the bow
 * and weapon slots, when the item is a cursed non-artifact (and not
 * uncursable), the Borg sets ident = true in borg_items[].
 *
 * Two input facts from the pinned tree that limit which items exist:
 *   - src/obj-make.c:apply_magic() returns as soon as make_artifact()
 *     succeeds, before make_ego_item(), so ordinary generation never makes
 *     an item that is both an artifact and an ego.  Such inputs below are
 *     marked SYNTHETIC.
 *   - borg-item-analyze.c:borg_item_analyze:620-622 sets the value of a
 *     cursed item (other than the One Ring) to 0, so cursed inputs below
 *     use value 0.
 *
 * No game state is involved.  The items are synthetic.
 */
#include <stdio.h>
#include <string.h>

#define ART_BONUS  150000L
#define EGO_BONUS  100000L
#define NOTE_BONUS  20000L

/* Item kinds, only to select the "unaware" bonus; values are arbitrary. */
enum { K_WEAPON, K_RING, K_AMULET, K_ROD, K_WAND, K_STAFF, K_POTION,
       K_SCROLL, K_FOOD };

struct item {
	const char *name;
	long value;
	int art_idx;
	int random_ego;      /* borg_ego_has_random_power(&e_info[ego_idx]) */
	int ident;           /* item->ident */
	int needs_ident;     /* item->needs_ident */
	int inscribed_qq;    /* the note contains "{??}" */
	int wieldable;       /* borg_wield_slot(item) >= 0 */
	int aware;
	int kind;
};

/* borg-item-id.c:borg_item_note_needs_id:145-155 */
static int note_needs_id(const struct item *it)
{
	if (it->ident)
		return 0;
	if (it->needs_ident)
		return 1;
	return it->inscribed_qq;
}

/* borg-inventory.c:borg_item_worth_id:274-287 */
static int worth_id(const struct item *it)
{
	if (!note_needs_id(it))
		return 0;
	return it->wieldable;
}

/* loop 1 body, within borg-item-wear.c lines 101-134 */
static long loop1_score(const struct item *it)
{
	long v = 0;
	if (!it->needs_ident)
		return 0;
	if (it->art_idx)
		v = it->value + ART_BONUS;
	if (it->random_ego)
		v = it->value + EGO_BONUS;
	if (note_needs_id(it))
		v = it->value + NOTE_BONUS;
	return v;
}

/* loop 2 body, within borg-item-wear.c lines 137-205 */
static long loop2_score(const struct item *it, int free_id, int maxdepth)
{
	long v = 0;
	if (!it->needs_ident)
		return 0;
	if (it->art_idx)
		v = it->value + ART_BONUS;
	if (note_needs_id(it))
		v = it->value + NOTE_BONUS;
	else if (free_id || worth_id(it))
		v = it->value;
	if (!it->aware) {
		switch (it->kind) {
		case K_RING: case K_AMULET: v += (maxdepth * 5000L); break;
		case K_ROD:                 v += (maxdepth * 3000L); break;
		case K_WAND: case K_STAFF:  v += (maxdepth * 2000L); break;
		case K_POTION: case K_SCROLL:
			if (maxdepth < 5) break;
			v += (maxdepth * 500L); break;
		case K_FOOD:                v += (maxdepth * 10L); break;
		}
	}
	return v;
}

static void show(const struct item *it, int free_id, int maxdepth)
{
	printf("  %-46s loop1 %7ld   loop2 %7ld   (note_needs_id=%d)\n",
		it->name, loop1_score(it), loop2_score(it, free_id, maxdepth),
		note_needs_id(it));
}

int main(void)
{
	int ident, needs;

	printf("=== 1. Which (ident, needs_ident) pairs can reach the scoring? ===\n");
	printf("object_fully_known() requires object_runes_known(), so straight\n");
	printf("from borg_item_analyze() needs_ident=1 implies ident=0.\n\n");
	for (ident = 0; ident <= 1; ident++)
		for (needs = 0; needs <= 1; needs++) {
			struct item it = { "", 0, 0, 0, ident, needs, 0, 1, 1, K_WEAPON };
			const char *reach;
			if (!needs)
				reach = "skipped by both loops (!needs_ident)";
			else if (ident)
				reach = "only via the cursed bow/weapon zeroing";
			else
				reach = "every ordinary candidate";
			printf("  ident=%d needs_ident=%d  note_needs_id=%d   %s\n",
				ident, needs, note_needs_id(&it), reach);
		}

	printf("\n=== 2. Ordinary candidates (ident=0, needs_ident=1) ===\n");
	printf("The +20000 assignment always fires and overwrites the artifact\n");
	printf("and ego assignments above it, in both loops.  (free_id=0, depth 30)\n\n");
	{
		struct item a = { "SYNTHETIC: artifact + random-power ego", 5000, 1, 1, 0, 1, 0, 1, 1, K_WEAPON };
		struct item b = { "plain artifact", 5000, 1, 0, 0, 1, 0, 1, 1, K_WEAPON };
		struct item c = { "random-power ego, not an artifact", 5000, 0, 1, 0, 1, 0, 1, 1, K_WEAPON };
		struct item d = { "plain wieldable item needing ID", 5000, 0, 0, 0, 1, 0, 1, 1, K_WEAPON };
		show(&a, 0, 30); show(&b, 0, 30); show(&c, 0, 30); show(&d, 0, 30);
		printf("  -> all four score value+20000: the artifact and ego bonuses\n");
		printf("     never survive for an ordinary candidate.\n");
	}

	printf("\n=== 3. The exception: a cursed non-artifact in the bow/weapon slot ===\n");
	printf("borg_notice_equipment() sets ident=1 on it, so note_needs_id()=0.\n\n");
	{
		struct item e = { "cursed random-power ego weapon (wielded)", 0, 0, 1, 1, 1, 0, 1, 1, K_WEAPON };
		struct item f = { "cursed plain weapon (wielded)", 0, 0, 0, 1, 1, 0, 1, 1, K_WEAPON };
		show(&e, 0, 30); show(&f, 0, 30);
		printf("  with free_id=1 (Identify Rune castable):\n");
		show(&e, 1, 30); show(&f, 1, 30);
		printf("  -> here the ego bonus does survive loop 1 (0+100000); in loop 2\n");
		printf("     the item scores its value, 0, so it is not a loop-2 candidate.\n");
	}
	return 0;
}
