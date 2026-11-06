/*
 * Standalone check of the blows-per-turn calculation used by the C Borg.
 *
 * angband e473720ed:
 *   src/borg/borg-trait.c:borg_calc_blows() (1035-1071)  -- the Borg's copy
 *   src/player-calcs.c:calc_blows()        (1703-1735)  -- the engine's copy
 *
 * The two differ in the weight they use:
 *     Borg   (borg-trait.c:1042):   int weight = item->weight * item->iqty;
 *     engine (player-calcs.c:1711): int weight = (obj == NULL) ? 0 : object_weight_one(obj);
 * item->weight is already one item's weight (borg_item_analyze() stores
 * object_weight_one()), so the Borg's copy weighs the whole stack.
 *
 * Both end with (engine returns hundredths, the Borg divides by 100):
 *     blows = MIN(10000 / blow_energy, 100 * class->max_attacks);
 *     MAX(blows + 100 * extra, OPT(birth_percent_damage) ? 200 : 100) [/ 100]
 * This program uses extra = 0 (no OBJ_MOD_BLOWS, no BI_EXTRA_BLOWS) and
 * birth_percent_damage off.
 *
 * Tables, taken from the pinned tree:
 *   adj_str_blow   src/player-calcs.c:548  -- first 12 of its STAT_RANGE
 *                  entries (stat indexes 0..11, STR 3..14); only those are used
 *   blows_table    src/player-calcs.c:889  -- identical to the Borg's
 *                  borg_blows_table (borg-trait.c:303)
 * The dexterity input below is the blows_table COLUMN, i.e. the value of
 * adj_dex_blow[] for the character's DEX: column 0 is DEX 3..9, column 1 is
 * DEX 10..16, column 5 is DEX 18/60-18/69 (borg-trait.c:262, same as
 * player-calcs.c adj_dex_blow).
 *
 * Warrior, lib/gamedata/class.txt: max-attacks:6, min-weight:30,
 * strength-multiplier:5.  Weights are in tenths of a pound.
 */
#include <stdio.h>

static const int adj_str_blow[12] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };

static const int blows_table[12][12] =
{
	{  100, 100, 95,  85,  75,  60,  50,  42,  35,  30,  25,  23 },
	{  100, 95,  85,  75,  60,  50,  42,  35,  30,  25,  23,  21 },
	{   95, 85,  75,  60,  50,  42,  35,  30,  26,  23,  21,  20 },
	{   85, 75,  60,  50,  42,  36,  32,  28,  25,  22,  20,  19 },
	{   75, 60,  50,  42,  36,  33,  28,  25,  23,  21,  19,  18 },
	{   60, 50,  42,  36,  33,  30,  27,  24,  22,  21,  19,  17 },
	{   50, 42,  36,  33,  30,  27,  25,  23,  21,  20,  18,  17 },
	{   42, 36,  33,  30,  28,  26,  24,  22,  20,  19,  18,  17 },
	{   36, 33,  30,  28,  26,  24,  22,  21,  20,  19,  17,  16 },
	{   35, 32,  29,  26,  24,  22,  21,  20,  19,  18,  17,  16 },
	{   34, 30,  27,  25,  23,  22,  21,  20,  19,  18,  17,  16 },
	{   33, 29,  26,  24,  22,  21,  20,  19,  18,  17,  16,  15 }
};

#define WARRIOR_ATT_MULTIPLY 5
#define WARRIOR_MIN_WEIGHT   30
#define WARRIOR_MAX_ATTACKS  6

struct item { int weight; int iqty; };

/* The shared body; the two copies differ only in weight_used. */
static void calc(int weight_used, int str_stat_index, int dex_col,
                 int *out_str_index, int *out_blows)
{
	int div, str_index, blow_energy, blows, extra = 0;

	div = (weight_used < WARRIOR_MIN_WEIGHT) ? WARRIOR_MIN_WEIGHT : weight_used;

	str_index = adj_str_blow[str_stat_index] * WARRIOR_ATT_MULTIPLY / div;
	if (str_index > 11) str_index = 11;

	blow_energy = blows_table[str_index][dex_col];

	blows = 10000 / blow_energy;
	if (blows > 100 * WARRIOR_MAX_ATTACKS) blows = 100 * WARRIOR_MAX_ATTACKS;

	blows = blows + 100 * extra;
	if (blows < 100) blows = 100;          /* birth_percent_damage off */

	*out_blows = blows / 100;
	*out_str_index = str_index;
}

static void row(const char *label, struct item it, int str_stat_index, int dex_col)
{
	int borg_si, borg_b, eng_si, eng_b;

	calc(it.weight * it.iqty, str_stat_index, dex_col, &borg_si, &borg_b);
	calc(it.weight, str_stat_index, dex_col, &eng_si, &eng_b);

	printf("%-30s weight borg %4d / engine %4d | str_index borg %2d / engine %2d"
	       " | blows borg %d / engine %d%s\n",
		label, it.weight * it.iqty, it.weight, borg_si, eng_si,
		borg_b, eng_b, (borg_b != eng_b) ? "   <-- DIVERGES" : "");
}

int main(void)
{
	printf("Warrior (lib/gamedata/class.txt): strength-multiplier=%d "
	       "min-weight=%d max-attacks=%d\n", WARRIOR_ATT_MULTIPLY,
	       WARRIOR_MIN_WEIGHT, WARRIOR_MAX_ATTACKS);
	printf("extra blows 0, birth_percent_damage off.\n\n");

	printf("--- weight 30 (3 lb), STR index 9 (STR 12), dex column 0 ---\n");
	row("iqty 1 (wielded)", (struct item){ 30, 1 }, 9, 0);
	row("iqty 5 (a home stack)", (struct item){ 30, 5 }, 9, 0);

	printf("\n--- weight 30 (3 lb), STR index 3 (STR 6), dex column 5 (DEX 18/60) ---\n");
	row("iqty 1 (wielded)", (struct item){ 30, 1 }, 3, 5);
	row("iqty 2 (a home stack)", (struct item){ 30, 2 }, 3, 5);
	row("iqty 5 (a home stack)", (struct item){ 30, 5 }, 3, 5);

	printf("\n--- weight 30 (3 lb), STR index 11 (STR 14), dex column 11 ---\n");
	row("iqty 1 (wielded)", (struct item){ 30, 1 }, 11, 11);
	row("iqty 3 (a home stack)", (struct item){ 30, 3 }, 11, 11);

	printf("\nThe engine weighs one item; the Borg's copy weighs the stack.\n");
	printf("For the wielded weapon (iqty 1) they agree.  borg_calc_blows()\n");
	printf("is also called on home items (borg-home-notice.c), where weapon\n");
	printf("stacks occur, and there the Borg can count fewer blows.\n");
	return 0;
}
