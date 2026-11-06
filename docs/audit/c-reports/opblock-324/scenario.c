/* Synthetic native C operator-block diagnostic, excluded from the source fix. */
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
    const char *mode = getenv("T324_MODE");
    if (!mode) mode = "operators";
    printf("MODE %s\n", mode);

    if (streq(mode, "operators")) {
        /* A formula with operators: the precedence pass builds a
         * temporary operator list whose backing array was lost. */
        int idx = parse_calculation_line("1 + 2 * 3", "1 + 2 * 3");
        require(idx >= 0);
        int32_t value = borg_calculate_dynamic(idx, 0);
        printf("GEOMETRY parsed count=%d\n", calculations.count);
        printf("RATING formula_value=%d calc_count=%d\n", (int)value,
            calculations.count);
        printf("OUTCOME formula_value=%d calc_count=%d\n", (int)value,
            calculations.count);
        /* Precedence is applied identically in both variants. */
        eq((int)value, 7);
        eq(calculations.count, 1);
    } else {
        /* Control: a bare constant builds no operator list at all. */
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
const char *suite_name = "native-c/opblock-324";
struct test tests[] = {{"operator_block_backing_array_is_freed", scenario}, {NULL, NULL}};
