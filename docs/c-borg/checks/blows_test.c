/*
 * Standalone check of the blows-per-turn calculation used by the C Borg.
 *
 * angband e473720ed:
 *   src/borg/borg-trait.c:borg_calc_blows()  -- the Borg's copy
 *   src/player-calcs.c:calc_blows()          -- the engine's copy
 *
 * The Borg's copy multiplies the item weight by the stack size:
 *     int weight = item->weight * item->iqty;
 * The engine's copy uses the single-item weight:
 *     int weight = (obj == NULL) ? 0 : object_weight_one(obj);
 *
 * All the tables and constants below are taken verbatim from the pinned
 * tree (src/player-calcs.c, lib/gamedata/class.txt).
 */
#include <stdio.h>

/* src/player-calcs.c: adj_str_blow[STAT_RANGE] */
static const int adj_str_blow[12] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };

/* src/player-calcs.c: blows_table[12][12] */
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

/* lib/gamedata/class.txt: Warrior */
#define WARRIOR_ATT_MULTIPLY 5
#define WARRIOR_MIN_WEIGHT   30
#define WARRIOR_MAX_ATTACKS  5

struct item { int weight; int iqty; };

/* The two formulas, differing only in the weight they use. */
static void calc(int weight_used, int str_stat_index, int dex_index,
                 int *out_str_index, int *out_blows)
{
	int div, str_index, blow_energy, blows;

	div = (weight_used < WARRIOR_MIN_WEIGHT)
		? WARRIOR_MIN_WEIGHT : weight_used;

	str_index = adj_str_blow[str_stat_index] * WARRIOR_ATT_MULTIPLY / div;
	if (str_index > 11) str_index = 11;

	blow_energy = blows_table[str_index][dex_index];

	blows = (10000 / blow_energy);
	if (blows > 100 * WARRIOR_MAX_ATTACKS) blows = 100 * WARRIOR_MAX_ATTACKS;

	/* the Borg's "(blows + 100) / 100" */
	*out_blows = (blows + 100) / 100;
	*out_str_index = str_index;
}

static void row(const char *label, struct item it, int str_stat_index, int dex_index)
{
	int borg_si, borg_b, eng_si, eng_b;

	/* the Borg */
	calc(it.weight * it.iqty, str_stat_index, dex_index, &borg_si, &borg_b);
	/* the engine */
	calc(it.weight, str_stat_index, dex_index, &eng_si, &eng_b);

	printf("%-34s weight borg %4d / engine %4d | str_index borg %2d / engine %2d"
	       " | blows borg %d / engine %d %s\n",
		label, it.weight * it.iqty, it.weight, borg_si, eng_si,
		borg_b, eng_b, (borg_b != eng_b) ? "  <-- DIVERGES" : "");
}

int main(void)
{
	printf("Warrior (lib/gamedata/class.txt): att_multiply=%d min_weight=%d "
	       "max_attacks=%d\n", WARRIOR_ATT_MULTIPLY, WARRIOR_MIN_WEIGHT,
	       WARRIOR_MAX_ATTACKS);
	printf("Dexterity index 0 (dex 3) unless a row says otherwise.\n\n");

	printf("--- a 30-lb sword (weight 30), strength stat index 9 (str 12) ---\n");
	row("iqty 1 (wielded or a single item)", (struct item){ 30, 1 }, 9, 0);
	row("iqty 5 (a stack, as stored in home)", (struct item){ 30, 5 }, 9, 0);

	printf("\n--- a 15-lb dagger (weight 15), strength stat index 9 ---\n");
	row("iqty 1", (struct item){ 15, 1 }, 9, 0);
	row("iqty 5 (a stack, as stored in home)", (struct item){ 15, 5 }, 9, 0);

	printf("\n--- a 30-lb sword, strength stat index 4 (str 7) ---\n");
	row("iqty 1", (struct item){ 30, 1 }, 4, 0);
	row("iqty 5 (a stack, as stored in home)", (struct item){ 30, 5 }, 4, 0);

	printf("\n--- the case where the BLOW COUNT itself diverges ---\n");
	printf("strength stat index 3 (str 6), dexterity index 5, 30-lb sword\n");
	row("iqty 1", (struct item){ 30, 1 }, 3, 5);
	row("iqty 2 (a stack, as stored in home)", (struct item){ 30, 2 }, 3, 5);
	row("iqty 5 (a stack, as stored in home)", (struct item){ 30, 5 }, 3, 5);

	printf("\nThe engine weighs the single item in both cases.\n");
	printf("The Borg multiplies by the stack size, so a stack of five\n");
	printf("30-lb swords is weighed as 150 instead of 30.  That pushes the\n");
	printf("strength-vs-weight index down, and the blow count follows.\n");
	printf("\nThe wielded weapon has iqty == 1, so the two agree there.\n");
	printf("Home items ARE stored in stacks, and borg_calc_blows() is\n");
	printf("called on them from borg-home-notice.c, so the home damage\n");
	printf("total is computed with the wrong weight.\n");
	return 0;
}
