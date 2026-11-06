/*
 * What does the borg_flow_light() overflow corrupt -- in ONE named build?
 *
 * angband e473720ed src/borg/borg-flow-misc.c:borg_flow_light() (824-874)
 * writes borg_glow_y[n] / borg_glow_x[n] for every BORG_GLOW grid of the
 * panel with no bound check (AUTO_LIGHT_MAX = 1536), then enqueues
 * (borg_glow_y[i], borg_glow_x[i]) for i < borg_glow_n.
 *
 * Writing past an array is undefined behaviour.  What it corrupts depends on
 * the linker's layout of the zero-initialised data and on the code the
 * compiler generates.  This program does NOT rely on its own compiler's
 * layout: it simulates, in one byte arena, the layout and code of the build
 * below, and reports the result for that build only.
 *
 * The build: this tree at e473720ed, configured with
 *     cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DSUPPORT_BORG=ON
 *     (gcc 15.2.0, CMAKE_C_FLAGS_RELWITHDEBINFO = -O2 -g -DNDEBUG)
 * `nm -S -n build/game/angband` for these symbols:
 *     000000000022e940 0000000000000600 B borg_glow_y
 *     000000000022ef40 0000000000000600 B borg_glow_x
 *     000000000022f540 0000000000000002 B borg_glow_n
 *     000000000022f560 0000000000000600 B borg_light_y
 *     000000000022fb60 0000000000000600 B borg_light_x
 *     0000000000230160 0000000000000002 B borg_light_n
 * `objdump -d` of borg_flow_light in that binary shows the scan loop keeping
 * the counter in a register (%r9), storing x (to borg_glow_x) and then y (to
 * borg_glow_y) for each glowing grid, and storing the counter to
 * borg_glow_n once after the loop; the read-back loop re-reads borg_glow_n
 * from memory.  The simulation follows exactly that.
 *
 * Input: an 80x24 terminal with SIDEBAR_NONE (a 23 x 79 panel), panel origin
 * at dungeon grid (0,0), and every panel grid glowing -- the worst case.
 * With another origin the wrong x values change; they are y coordinates of
 * other glowing grids.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define AUTO_LIGHT_MAX 1536
#define W 79
#define H 23

/* offsets relative to borg_glow_y, from the nm output above */
#define OFF_GLOW_Y  0x000
#define OFF_GLOW_X  0x600
#define OFF_GLOW_N  0xc00
#define OFF_LIGHT_Y 0xc20
#define OFF_LIGHT_X 0x1220
#define ARENA       0x1822

static uint8_t arena[ARENA];

int main(void)
{
	int n = 0;                 /* the counter, held in a register */
	int y, x, i;
	int16_t glow_n;
	int wrong = 0, off_panel = 0, first_shown = 0;
	int light_y_hit = 0;

	memset(arena, 0, sizeof(arena));

	/* the scan loop, as compiled: x first, then y, counter in a register */
	for (y = 0; y < H; y++)
		for (x = 0; x < W; x++) {
			arena[OFF_GLOW_X + n] = (uint8_t)x;
			arena[OFF_GLOW_Y + n] = (uint8_t)y;
			if (OFF_GLOW_X + n >= OFF_LIGHT_Y && OFF_GLOW_X + n < OFF_LIGHT_X)
				light_y_hit++;
			n++;
		}

	/* the counter is stored once, after the loop (little-endian int16_t) */
	glow_n = (int16_t)n;
	memcpy(&arena[OFF_GLOW_N], &glow_n, sizeof(glow_n));

	printf("build: e473720ed RelWithDebInfo (-O2), layout from nm -S (see source)\n");
	printf("glowing panel grids written: %d (array capacity %d)\n\n", n, AUTO_LIGHT_MAX);

	/* the read-back loop: enqueue (glow_y[i], glow_x[i]) for i < borg_glow_n */
	memcpy(&glow_n, &arena[OFF_GLOW_N], sizeof(glow_n));
	printf("entries the read-back loop enqueues: %d\n", glow_n);
	printf("first wrong entries:\n");
	for (i = 0; i < glow_n; i++) {
		int true_y = i / W, true_x = i % W;
		int gy = arena[OFF_GLOW_Y + i];
		int gx = arena[OFF_GLOW_X + i];
		if (gy == true_y && gx == true_x)
			continue;
		wrong++;
		if (gy >= H || gx >= W)
			off_panel++;
		if (first_shown < 4 || i == 1536 || i == 1537) {
			printf("  entry %4d: meant (%2d,%2d)  enqueued (%2d,%2d)\n",
				i, true_y, true_x, gy, gx);
			first_shown++;
		}
	}
	printf("\nwrong entries: %d of %d; off the panel: %d\n", wrong, glow_n, off_panel);
	printf("borg_light_y[] entries overwritten: %d (from borg_light_y[0])\n", light_y_hit);
	printf("\nIn this build the wrong entries have the right y and a wrong x\n");
	printf("(another grid's y, or a byte of borg_glow_n), all on the panel.\n");
	printf("This is a property of this build, not of the source.\n");
	return 0;
}
