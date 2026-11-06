/*
 * The final consequence: which grids does the read-back loop actually
 * enqueue, and are they the glowing grids it meant to enqueue?
 */
#include <stdio.h>
#include <stdint.h>

#define AUTO_LIGHT_MAX 1536
#define W 79
#define H 23

int16_t borg_glow_n = 0;
uint8_t borg_glow_x[AUTO_LIGHT_MAX];
uint8_t borg_glow_y[AUTO_LIGHT_MAX];
uint8_t borg_light_x[AUTO_LIGHT_MAX];
uint8_t borg_light_y[AUTO_LIGHT_MAX];

int main(void)
{
	int cells = 1817;
	int i;
	int wrong = 0, total = 0;
	int wrong_coords = 0;

	for (i = 0; i < cells; i++) {
		borg_glow_y[borg_glow_n] = (uint8_t)(i / W);
		borg_glow_x[borg_glow_n] = (uint8_t)(i % W);
		borg_glow_n++;
	}

	/* the read-back loop: enqueue (borg_glow_y[i], borg_glow_x[i]) */
	printf("the read-back loop enqueues %d grids.\n", borg_glow_n);
	printf("checking each against the grid it was supposed to hold:\n\n");

	for (i = 0; i < borg_glow_n; i++) {
		int true_y = i / W, true_x = i % W;
		int gy = borg_glow_y[i], gx = borg_glow_x[i];
		total++;
		if (gy != true_y || gx != true_x) {
			wrong++;
			/* is the coordinate even on the map? */
			if (gy >= H || gx >= W) wrong_coords++;
		}
	}

	printf("  total enqueued      : %d\n", total);
	printf("  holding the WRONG grid: %d\n", wrong);
	printf("  of those, off the map entirely (y>=%d or x>=%d): %d\n\n",
		H, W, wrong_coords);

	printf("the first few wrong ones:\n");
	int shown = 0;
	for (i = AUTO_LIGHT_MAX; i < borg_glow_n && shown < 6; i++, shown++) {
		int true_y = i / W, true_x = i % W;
		printf("  entry %4d: meant (%2d,%2d)   enqueued (%2d,%2d)%s\n",
			i, true_y, true_x, borg_glow_y[i], borg_glow_x[i],
			(borg_glow_y[i] >= H || borg_glow_x[i] >= W) ? "   <- OFF THE MAP" : "");
	}

	printf("\nso the tail of the flow's target list is garbage, and some of\n");
	printf("those coordinates are outside the map panel entirely.\n");
	return 0;
}
