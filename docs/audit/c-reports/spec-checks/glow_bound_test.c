/*
 * Standalone check of the array bound in
 * angband e473720ed src/borg/borg-flow-misc.c:borg_flow_light() (824-874).
 *
 * The Borg appends one entry per glowing grid on the visible map panel to
 * borg_glow_y[]/borg_glow_x[], which have AUTO_LIGHT_MAX entries.  There is
 * no bound check.  This test reproduces the loop's shape exactly and shows
 * the write index that the loop reaches for a given terminal geometry.
 *
 * No game state is involved; the constants are taken from the pinned tree:
 *   src/borg/borg-cave-light.h : AUTO_LIGHT_MAX 1536
 *   src/ui-term.h              : SIDEBAR_LEFT 0, SIDEBAR_TOP 1, SIDEBAR_NONE 2;
 *                                SCREEN_ROWS = Term->hgt - ROW_MAP - ROW_BOTTOM_MAP
 *                                SCREEN_HGT  = SCREEN_ROWS / tile_height
 *                                SCREEN_WID  = (Term->wid - COL_MAP - 1) / tile_width
 *   src/ui-term.c              : row_top_map{1,4,1}
 *                                row_bottom_map{1,0,0}
 *                                col_map{13,0,0}
 * Tiles are taken as 1x1 (text mode: tile_height = tile_width = 1).
 */
#include <stdio.h>

#define AUTO_LIGHT_MAX 1536

/* src/ui-term.c */
static const int row_top_map[3]    = {1, 4, 1};
static const int row_bottom_map[3] = {1, 0, 0};
static const int col_map[3]        = {13, 0, 0};

static void report(const char *label, int hgt, int wid, int sidebar)
{
	int rows  = hgt - row_top_map[sidebar] - row_bottom_map[sidebar];
	int cols  = wid - col_map[sidebar] - 1;
	int cells = rows * cols;

	/*
	 * The loop is:
	 *   for (y = w_y; y < w_y + SCREEN_HGT; y++)
	 *     for (x = w_x; x < w_x + SCREEN_WID; x++)
	 *       if (grid is glowing) { glow_y[n] = y; glow_x[n] = x; n++; }
	 * The worst case is every visible grid glowing, so the last index
	 * written is cells - 1.
	 */
	int last_index = cells - 1;

	printf("%-22s %-4s %3dx%-3d = %5d cells   last write index %5d   %s\n",
		label, "", rows, cols, cells, last_index,
		(last_index >= AUTO_LIGHT_MAX)
			? "can overflow (if every grid is BORG_GLOW)"
			: "within the arrays");
}

int main(void)
{
	printf("AUTO_LIGHT_MAX = %d\n\n", AUTO_LIGHT_MAX);

	printf("--- 80x24 (the classic terminal) ---\n");
	report("SIDEBAR_LEFT (0)", 24, 80, 0);
	report("SIDEBAR_TOP  (1)", 24, 80, 1);
	report("SIDEBAR_NONE (2)", 24, 80, 2);

	printf("\n--- 80x28 ---\n");
	report("SIDEBAR_LEFT (0)", 28, 80, 0);
	report("SIDEBAR_TOP  (1)", 28, 80, 1);
	report("SIDEBAR_NONE (2)", 28, 80, 2);

	printf("\n--- 120x40 ---\n");
	report("SIDEBAR_LEFT (0)", 40, 120, 0);
	report("SIDEBAR_TOP  (1)", 40, 120, 1);
	report("SIDEBAR_NONE (2)", 40, 120, 2);

	printf("\nThe write passes the end of the arrays once more than 1536 panel\n");
	printf("grids carry BORG_GLOW (grids the Borg believes are perma-lit).\n");
	printf("This program shows only the capacity; it does not show that a\n");
	printf("game reaches that many glowing grids.\n");
	return 0;
}
