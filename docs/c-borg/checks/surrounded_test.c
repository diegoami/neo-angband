/*
 * Standalone check of the neighbour counter in
 * angband e473720ed src/borg/borg-escape.c:borg_surrounded().
 *
 * The four "Mega-Hack" branches form one if/else-if chain, but the trap
 * test is a separate `if`, so a grid that satisfies a chain condition AND
 * the trap condition is counted twice.
 *
 * Two loop bodies are transcribed from the pinned source:
 *   as_written()  -- the source, exactly (the trap test is a separate if)
 *   corrected()   -- the same chain with the trap test moved in as the
 *                    fifth `else if`, i.e. what the code was clearly meant
 *                    to be
 */
#include <stdio.h>

#define FEAT_NONE   0

struct grid {
	int floor_grid;  /* borg_cave_floor_grid() */
	int feat;        /* ag->feat */
	int kill;        /* ag->kill (non-zero = a monster is here) */
	int is_shop;     /* feat_is_shop(ag->feat) */
	int trap;        /* ag->trap */
	int glyph;       /* ag->glyph */
};

static void as_written(const struct grid g[8], int monsters,
                       int *out_safe, int *out_verdict)
{
	int i;
	int non_safe_grids = 0;
	int safe_grids = 8;   /* the starting value in the source */

	for (i = 0; i < 8; i++) {
		const struct grid *ag = &g[i];

		/* Skip walls/doors */
		if (!ag->floor_grid)
			non_safe_grids++;

		/* Skip unknown grids */
		else if (ag->feat == FEAT_NONE)
			non_safe_grids++;

		/* Skip monster grids */
		else if (ag->kill)
			non_safe_grids++;

		/* Mega-Hack -- skip stores XXX XXX XXX */
		else if (ag->is_shop)
			non_safe_grids++;

		/* Mega-Hack -- skip traps XXX XXX XXX */
		if (ag->trap && !ag->glyph)
			non_safe_grids++;
	}

	safe_grids = safe_grids - non_safe_grids;

	*out_safe = safe_grids;

	/* Am I in hallway? If so don't worry about it */
	if (safe_grids == 1 && monsters == 1) {
		*out_verdict = 0;
		return;
	}

	/* I am likely to get surrounded */
	*out_verdict = (monsters > safe_grids);
}

static void corrected(const struct grid g[8], int monsters,
                      int *out_safe, int *out_verdict)
{
	int i;
	int non_safe_grids = 0;
	int safe_grids = 8;

	for (i = 0; i < 8; i++) {
		const struct grid *ag = &g[i];

		if (!ag->floor_grid)
			non_safe_grids++;
		else if (ag->feat == FEAT_NONE)
			non_safe_grids++;
		else if (ag->kill)
			non_safe_grids++;
		else if (ag->is_shop)
			non_safe_grids++;
		/* the same test, but as the fifth else-if of the chain */
		else if (ag->trap && !ag->glyph)
			non_safe_grids++;
	}

	safe_grids = safe_grids - non_safe_grids;
	*out_safe = safe_grids;

	if (safe_grids == 1 && monsters == 1) {
		*out_verdict = 0;
		return;
	}
	*out_verdict = (monsters > safe_grids);
}

static void plain(struct grid g[8])
{
	int i;
	for (i = 0; i < 8; i++) {
		g[i].floor_grid = 1;
		g[i].feat = 1;      /* not FEAT_NONE */
		g[i].kill = 0;
		g[i].is_shop = 0;
		g[i].trap = 0;
		g[i].glyph = 0;
	}
}

static void show(const char *label, struct grid g[8], int monsters)
{
	int sb, vb, sc, vc;

	as_written(g, monsters, &sb, &vb);
	corrected(g, monsters, &sc, &vc);

	printf("%-46s as-written safe=%3d %-14s | trap test as else-if: safe=%3d %-14s%s\n",
		label, sb, vb ? "SURROUNDED" : "not surrounded",
		sc, vc ? "SURROUNDED" : "not surrounded",
		(vb != vc) ? "  <-- THE VERDICT DIFFERS" : "");
}

int main(void)
{
	struct grid g[8];

	printf("safe_grids starts at 8; the verdict is `monsters > safe_grids`.\n\n");

	plain(g);
	show("control: 8 plain floors", g, 0);

	plain(g);
	g[0].floor_grid = 0;
	show("control: 1 wall + 7 floors", g, 0);

	plain(g);
	g[0].trap = 1;   /* a trap on an ordinary floor: the chain does not fire */
	show("control: 1 trap-only floor + 7 floors", g, 0);

	plain(g);
	g[0].kill = 1;
	g[0].trap = 1;
	show("control: 1 monster-on-trap + 7 floors", g, 1);

	plain(g);
	for (int i = 0; i < 8; i++) g[i].trap = 1;
	show("control: 8 trap-only floors", g, 0);

	plain(g);
	for (int i = 0; i < 3; i++) { g[i].floor_grid = 0; g[i].trap = 1; }
	for (int i = 3; i < 6; i++) { g[i].trap = 1; }
	show("TEST: 3 wall+trap, 3 trap-only, 0 monsters", g, 0);

	plain(g);
	for (int i = 0; i < 4; i++) { g[i].floor_grid = 0; g[i].trap = 1; }
	for (int i = 4; i < 8; i++) { g[i].trap = 1; }
	show("TEST: 4 wall+trap, 4 trap-only, 0 monsters", g, 0);

	plain(g);
	for (int i = 0; i < 4; i++) { g[i].floor_grid = 0; g[i].trap = 1; }
	show("TEST: 4 wall+trap, 4 plain floors, 0 monsters", g, 0);

	printf("\nThe trap test is a separate `if`, not the fifth `else if` of\n");
	printf("the chain, so a grid that is both a wall (or a monster, or a\n");
printf("shop) and a trap is counted twice.  The extra count pushes\n");
printf("safe_grids below zero, and `monsters > safe_grids` is then true\n");
printf("with ZERO monsters, so the Borg flees a fight it is not in.\n");
	return 0;
}
