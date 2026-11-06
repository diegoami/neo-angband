/*
 * When does loop 1's score actually block a loop-2 candidate?
 *
 * Loop 1 leaves b_v = (some item's loop-1 score).  Loop 2 skips any
 * candidate whose score is <= b_v.  So loop 1's ego bonus (value+100000)
 * blocks a loop-2 candidate with a score in (value, value+100000].
 */
#include <stdio.h>
#define ART_BONUS 150000L
#define EGO_BONUS 100000L
#define NOTE_BONUS  20000L

struct item { const char *name; long value; int art, ego, note, aware; int maxdepth, tval; };

static long l1(const struct item *i)
{
	long v = 0;
	if (i->art)  v = i->value + ART_BONUS;
	if (i->ego)  v = i->value + EGO_BONUS;
	if (i->note) v = i->value + NOTE_BONUS;
	return v;
}

static long l2(const struct item *i)
{
	long v = 0;
	if (i->art)  v = i->value + ART_BONUS;
	if (i->note) v = i->value + NOTE_BONUS;
	else if (!i->art) v = i->value;
	if (!i->aware) {
		switch (i->tval) {
		case 90: case 91: v += i->maxdepth * 5000L; break;
		case 95: v += i->maxdepth * 3000L; break;
		case 96: case 97: v += i->maxdepth * 2000L; break;
		case 100: case 101: if (i->maxdepth >= 5) v += i->maxdepth * 500L; break;
		}
	}
	return v;
}

int main(void)
{
	/* An unaware ring at depth 50 scores value + 5000*50 = value+250000
	 * in loop 2.  For a random-power ego whose value is high enough that
	 * value+EGO_BONUS exceeds the ring's loop-2 score, the ring is
	 * blocked. */
	int depth = 50;
	long ring_value = 100;
	long ring_l2 = ring_value + depth * 5000L;

	printf("an unaware ring at depth %d, value %ld:\n", depth, ring_value);
	printf("  loop 2 score = %ld\n", ring_l2);

	printf("\nloop 1 scores a random-power ego at value+100000.\n");
	printf("For that to exceed the ring's loop-2 score of %ld we need\n", ring_l2);
	printf("value > %ld.  Take value = %ld:\n", ring_l2 - EGO_BONUS, ring_l2 - EGO_BONUS + 1);

	{
		long ego_value = ring_l2 - EGO_BONUS + 1;
		struct item ego = { "random-power ego", ego_value, 0, 1, 0, 1, depth, 0 };
		struct item ring = { "unaware ring", ring_value, 0, 0, 0, 0, depth, 90 };

		printf("\n  ego:   loop1 %ld   loop2 %ld\n", l1(&ego), l2(&ego));
		printf("  ring:  loop1 %ld   loop2 %ld\n", l1(&ring), l2(&ring));

		printf("\n  Running the two loops in order, sharing b_v:\n");
		long b_v = -1; const char *who = "(none)";
		long v;
		v = l1(&ego); if (v > b_v) { b_v = v; who = ego.name; }
		v = l1(&ring); if (v > b_v) { b_v = v; who = ring.name; }
		printf("    after loop 1: b_v = %-8ld candidate = %s\n", b_v, who);
		v = l2(&ego); if (v > b_v) { b_v = v; who = ego.name; }
		v = l2(&ring); if (v > b_v) { b_v = v; who = ring.name; }
		printf("    after loop 2: b_v = %-8ld candidate = %s\n", b_v, who);

		printf("\n  Without loop 1's ego bonus, the ring would have won:\n");
		b_v = -1; who = "(none)";
		v = l2(&ego); if (v > b_v) { b_v = v; who = ego.name; }
		v = l2(&ring); if (v > b_v) { b_v = v; who = ring.name; }
		printf("    loop 2 alone: b_v = %-8ld candidate = %s\n", b_v, who);
	}
	return 0;
}
