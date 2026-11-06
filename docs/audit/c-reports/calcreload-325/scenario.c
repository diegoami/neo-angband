/* Synthetic native C calculation-reload diagnostic, excluded from the source fix. */
#include "unit-test.h"
#include "test-utils.h"
#include "z-virt.h"
#include "borg/borg-formulas.h"
#include "borg/borg-formulas-calc.h"
#include <stdio.h>
#include <string.h>

extern struct borg_array calculations;

int setup_tests(void **state)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("SETUPSTEP minimal\n");
    return 0;
}

int teardown_tests(void *state)
{
    return 0;
}

static int scenario(void *state)
{
    const char *mode = getenv("T325_MODE");
    if (!mode) mode = "reload";
    printf("MODE %s\n", mode);

    if (streq(mode, "reload")) {
        /* Parse, free, re-parse: the borg reinitialize path. */
        int first = parse_calculation_line("1 + 2", "1 + 2");
        require(first >= 0);
        eq(calculations.count, 1);
        eq((int)borg_calculate_dynamic(first, 0), 3);

        calculations_free();
        /* The defect: the backing pointer array is retained while the
         * count says empty, so the next add allocates over it. */
        int retained = calculations.items != NULL;
        printf("GEOMETRY freed retained=%d count=%d\n", retained,
            calculations.count);

        int second = parse_calculation_line("3 + 4", "3 + 4");
        require(second >= 0);
        int32_t value = borg_calculate_dynamic(second, 0);
        printf("RATING retained=%d reload_value=%d reload_count=%d\n",
            retained, (int)value, calculations.count);
        printf("OUTCOME retained=%d reload_value=%d reload_count=%d\n",
            retained, (int)value, calculations.count);
        /* Correct contract: freeing releases the array, and reloading
         * still works. */
        require(!retained);
        eq((int)value, 7);
        eq(calculations.count, 1);
    } else {
        /* Control: parse once, no free - identical in both variants. */
        int only = parse_calculation_line("1 + 2", "1 + 2");
        require(only >= 0);
        int32_t value = borg_calculate_dynamic(only, 0);
        printf("GEOMETRY control count=%d\n", calculations.count);
        printf("RATING control_value=%d control_count=%d\n", (int)value,
            calculations.count);
        printf("OUTCOME control_value=%d control_count=%d\n", (int)value,
            calculations.count);
        eq((int)value, 3);
        eq(calculations.count, 1);
    }
    ok;
}
const char *suite_name = "native-c/calcreload-325";
struct test tests[] = {{"calculation_reload_releases_the_array", scenario}, {NULL, NULL}};
