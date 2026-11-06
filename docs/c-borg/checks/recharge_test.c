/*
 * Standalone check of the candidate selection in
 * angband e473720ed src/borg/borg-item-wear.c:borg_wear_recharge().
 *
 * The scan and the two outcome branches are transcribed from the pinned
 * source.  The item array is the Borg's packed index space: the pack
 * first, then the equipment slots from INVEN_WIELD.
 */
#include <stdio.h>
#include <string.h>

#define TV_WAND   96
#define TV_STAFF  97
#define TV_ROD    95
#define TV_RING   100
#define TV_LIGHT  13

/* The Borg's packed index space: the pack occupies 0..pack_size-1 and the
 * equipment starts at INVEN_WIELD == pack_size.  Here the pack has 4
 * slots, so the equipment starts at 4. */
#define PACK_SLOTS   4
#define INVEN_WIELD  4
#define INVEN_TOTAL 10
#define sv_rod_recall 3

/* borg_wield_slot() for the tvals this test uses; -1 = no slot.
 * The returned values are the real equipment indices. */
static int wield_slot(int tval)
{
	switch (tval) {
	case 13:  return INVEN_WIELD + 8;  /* TV_LIGHT -> INVEN_LIGHT */
	case 100: return INVEN_WIELD + 6;  /* TV_RING  -> INVEN_LEFT  */
	case 95:  return -1;               /* TV_ROD   -> no slot     */
	case 96:  return -1;               /* TV_WAND  -> no slot     */
	case 97:  return -1;               /* TV_STAFF -> no slot     */
	}
	return -1;
}

struct item {
	int tval;
	int qty;
	int timeout;   /* non-zero = needs recharging */
	int sval;
};

static void scan(const struct item *items, int n, int depth,
                 int *out_i, int *out_slot, const char **outcome)
{
	int i;
	int b_i = -1;
	int b_slot = -1;

	for (i = 0; i < n; i++) {
		const struct item *item = &items[i];
		int slot;

		if (!item->qty) continue;
		if (!item->timeout) continue;

		slot = wield_slot(item->tval);

		/* if this is a rod, only count it if it is a rod of recall
		 * and we are in town.  (The source's condition is inverted
		 * in the source text; here we take the branch the source
		 * takes when NOT in town, which is the interesting case.) */
		if (item->tval == TV_ROD && item->sval != sv_rod_recall && depth == 0)
			continue;

		/* skip torches and lanterns */
		if (item->tval == TV_LIGHT)
			continue;

		b_i = i;
		b_slot = slot;
	}

	*out_i = b_i;
	*out_slot = b_slot;

	if (b_i == -1) {
		*outcome = "nothing to recharge";
		return;
	}
	if (b_i >= INVEN_WIELD || items[b_i].tval == TV_ROD) {
		*outcome = "waiting for recharge (rest 750)";
		return;
	}
	if (b_slot >= INVEN_WIELD) {
		*outcome = "swapping item for recharge (rest 750)";
		return;
	}
	*outcome = "NOTHING TO RECHARGE";
}

static void show(const char *label, struct item items[], int n)
{
	int bi, bs;
	const char *outcome;
	scan(items, n, 5, &bi, &bs, &outcome);

	printf("%-52s %-14s %6d  %s%s\n",
		label,
		(bi == -1) ? "(none)" :
			(items[bi].tval == TV_WAND) ? "pack wand" :
			(items[bi].tval == TV_STAFF) ? "pack staff" :
			(items[bi].tval == TV_ROD) ? "rod" : "worn ring",
		bs, outcome,
		(strncmp(outcome, "NOTHING", 7) == 0 && bi != -1) ? "  <-- the ring is lost" : "");
}

int main(void)
{
	struct item items[8];

	/* The Borg's index space: the pack first, then the equipment. */
	items[0] = (struct item){ TV_WAND,  1, 0, 0 };   /* pack wand, charged     */
	items[1] = (struct item){ TV_WAND,  1, 0, 0 };   /* pack wand, charged     */
	items[2] = (struct item){ TV_RING,  1, 0, 0 };   /* worn ring, charged     */

	show("control: no timed items", items, 3);

	items[2].timeout = 1;
	show("control: a worn ring only (last timed item)", items, 3);

	items[0].tval = TV_ROD; items[0].timeout = 1; items[0].sval = sv_rod_recall; items[2].timeout = 0;
	show("control: a worn rod only (last timed item)", items, 3);

	/* TEST A: worn ring first, pack wand last */
	items[0] = (struct item){ TV_WAND, 1, 0, 0 };
	items[1] = (struct item){ TV_WAND, 1, 0, 0 };
	items[2] = (struct item){ TV_RING, 1, 1, 0 };
	items[3] = (struct item){ TV_WAND, 1, 1, 0 };   /* the LAST timed item */
	show("TEST A: worn ring first, pack wand last", items, 4);

	/* TEST B: worn ring first, pack staff last */
	items[3] = (struct item){ TV_STAFF, 1, 1, 0 };
	show("TEST B: worn ring first, pack staff last", items, 4);

	/* TEST C: pack wand first, worn ring last */
	items[0] = (struct item){ TV_WAND, 1, 1, 0 };
	items[1] = (struct item){ TV_WAND, 1, 0, 0 };
	items[2] = (struct item){ TV_RING, 1, 1, 0 };
	items[3] = (struct item){ TV_WAND, 1, 0, 0 };
	show("TEST C: pack wand first, worn ring last", items, 4);

	/* TEST D: worn ring + pack wand, wand last */
	items[0] = (struct item){ TV_WAND, 1, 0, 0 };
	items[1] = (struct item){ TV_WAND, 1, 0, 0 };
	items[2] = (struct item){ TV_RING, 1, 1, 0 };
	items[3] = (struct item){ TV_WAND, 1, 1, 0 };
	show("TEST D: worn ring + pack wand, pack wand last", items, 4);

	printf("\nThe scan has no early exit and no preference: b_i is\n");
	printf("overwritten on every qualifying item, so the LAST timed item\n");
	printf("wins.  And nothing rejects an item whose wield slot is -1\n");
printf("(a wand or a staff in the pack), so that item can be selected\n");
	printf("and then make the routine give up entirely.\n");
	return 0;
}
