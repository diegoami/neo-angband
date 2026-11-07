/*
 * Standalone check of the candidate selection in
 * angband e473720ed src/borg/borg-item-wear.c:borg_wear_recharge() (1451-1538).
 *
 * The scan (1463-1492) and the three outcomes (1495-1537) are transcribed
 * from the pinned source.  The item array is the Borg's packed index space:
 * the pack first, then the equipment from INVEN_WIELD.  Here the pack has 4
 * slots, so INVEN_WIELD = 4; INVEN_TOTAL = INVEN_WIELD + 12.
 *
 * borg_wield_slot() (borg-inventory.c:42-77) returns an equipment index for
 * weapons, bows, rings (INVEN_LEFT), amulets, lights, armour, and -1 for
 * everything else (rods, wands, staffs, ...).
 *
 * What the game produces at the pin: an object's `timeout` is set by
 * activation (USE_TIMEOUT, src/cmd-obj.c) and recharges only while equipped,
 * except rods, which recharge in the pack (src/game-world.c;
 * src/obj-tval.c:tval_can_have_timeout() is TV_ROD only).  Wands and staffs
 * use charges (pval), never timeout.  So "a pack wand with a timeout"
 * (cases marked SYNTHETIC) is not a state ordinary play reaches; those cases
 * show what the code does with it, not that it happens.
 *
 * No game state is involved.
 */
#include <stdio.h>

enum { K_RING, K_AMULET, K_ROD, K_WAND, K_STAFF, K_TORCH, K_LANTERN,
       K_ARTIFACT_LIGHT };

#define INVEN_WIELD  4
#define INVEN_LEFT   (INVEN_WIELD + 3)
#define INVEN_NECK   (INVEN_WIELD + 4)
#define INVEN_LIGHT  (INVEN_WIELD + 5)
#define INVEN_TOTAL  (INVEN_WIELD + 12)

struct item {
	int kind;
	int qty;
	int timeout;        /* non-zero = still recharging */
	int is_recall_rod;  /* sval == sv_rod_recall */
};

static const char *kname(int k)
{
	switch (k) {
	case K_RING: return "ring";       case K_AMULET: return "amulet";
	case K_ROD: return "rod";         case K_WAND: return "wand";
	case K_STAFF: return "staff";     case K_TORCH: return "torch";
	case K_LANTERN: return "lantern"; case K_ARTIFACT_LIGHT: return "artifact light";
	}
	return "?";
}

/* borg_wield_slot() for the kinds used here */
static int wield_slot(int k)
{
	switch (k) {
	case K_RING: return INVEN_LEFT;
	case K_AMULET: return INVEN_NECK;
	case K_TORCH: case K_LANTERN: case K_ARTIFACT_LIGHT: return INVEN_LIGHT;
	}
	return -1;
}

static const char *scan(const struct item items[INVEN_TOTAL], int depth, int *out_i)
{
	int i, b_i = -1, slot = -1, b_slot = -1;

	for (i = 0; i < INVEN_TOTAL; i++) {
		const struct item *item = &items[i];
		if (!item->qty) continue;
		if (!item->timeout) continue;
		slot = wield_slot(item->kind);
		if (item->kind == K_ROD && !item->is_recall_rod && depth == 0)
			continue;
		if (item->kind == K_TORCH || item->kind == K_LANTERN)
			continue;
		b_i = i;
		b_slot = slot;
	}
	*out_i = b_i;
	if (b_i == -1)
		return "nothing found -> return false";
	if (b_i >= INVEN_WIELD || items[b_i].kind == K_ROD)
		return "rest R 7 5 Enter (worn item or rod)";
	if (b_slot >= INVEN_WIELD)
		return "wear it, then rest (pack wearable)";
	return "GIVE UP -> return false";
}

static void show(const char *label, struct item items[INVEN_TOTAL])
{
	int bi;
	const char *outcome = scan(items, 5, &bi);
	printf("%-52s chosen: %-26s %s\n", label,
		bi < 0 ? "(none)" : "", outcome);
	if (bi >= 0)
		printf("%-52s         %s %s (index %d)\n", "",
			bi >= INVEN_WIELD ? "worn" : "pack", kname(items[bi].kind), bi);
}

static void clear(struct item items[INVEN_TOTAL])
{
	int i;
	for (i = 0; i < INVEN_TOTAL; i++)
		items[i] = (struct item){ K_RING, 0, 0, 0 };
}

int main(void)
{
	struct item it[INVEN_TOTAL];

	printf("depth 5 (in the dungeon) for every case\n\n");

	clear(it);
	show("control: nothing recharging", it);

	clear(it);
	it[INVEN_LEFT] = (struct item){ K_RING, 1, 50, 0 };
	show("control: worn ring recharging", it);

	clear(it);
	it[1] = (struct item){ K_AMULET, 1, 50, 0 };
	show("control: pack amulet recharging", it);

	clear(it);
	it[1] = (struct item){ K_AMULET, 1, 50, 0 };
	it[2] = (struct item){ K_ROD, 1, 20, 0 };
	show("LAST MATCH: pack amulet, then pack rod", it);

	clear(it);
	it[2] = (struct item){ K_ROD, 1, 20, 0 };
	it[INVEN_LEFT] = (struct item){ K_RING, 1, 50, 0 };
	show("LAST MATCH: pack rod, then worn ring", it);

	clear(it);
	it[1] = (struct item){ K_AMULET, 1, 50, 0 };
	it[3] = (struct item){ K_WAND, 1, 1, 0 };
	show("SYNTHETIC: pack amulet, then pack wand w/ timeout", it);

	printf("\nThe scan keeps the LAST item with a timeout; there is no\n");
	printf("preference between resting for a rod, resting for a worn item,\n");
	printf("and swapping in a pack item.  The give-up path needs an\n");
	printf("unwieldable non-rod item with a timeout (SYNTHETIC above).\n");
	return 0;
}
