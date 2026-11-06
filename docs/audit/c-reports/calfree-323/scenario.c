/* Synthetic native C calculation-free diagnostic, excluded from the source fix. */
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
    const char *mode = getenv("T323_MODE");
    if (!mode) mode = "free";
    printf("MODE %s\n", mode);

    if (streq(mode, "free")) {
        /* Parse and free, as a borg reinitialize does for every
         * dynamic formula. */
        int idx = parse_calculation_line("1 + 2", "1 + 2");
        require(idx >= 0);
        eq((int)borg_calculate_dynamic(idx, 0), 3);
        calculations_free();
        printf("GEOMETRY freed count=%d\n", calculations.count);
        printf("RATING freed_value=3 freed_count=%d\n", calculations.count);
        printf("OUTCOME freed_value=3 freed_count=%d\n", calculations.count);
        eq(calculations.count, 0);
    } else {
        /* Control: parse and evaluate without freeing - the free path
         * is never reached. */
        int idx = parse_calculation_line("42", "42");
        require(idx >= 0);
        int32_t value = borg_calculate_dynamic(idx, 0);
        printf("GEOMETRY control count=%d\n", calculations.count);
        printf("RATING control_value=%d control_count=%d\n", (int)value,
            calculations.count);
        printf("OUTCOME control_value=%d control_count=%d\n", (int)value,
            calculations.count);
        eq((int)value, 42);
        eq(calculations.count, 1);
    }
    ok;
}
const char *suite_name = "native-c/calfree-323";
struct test tests[] = {{"calculation_free_releases_the_token_array", scenario}, {NULL, NULL}};
