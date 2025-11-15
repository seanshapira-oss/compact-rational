#include "compact_rational.h"
#include <stdio.h>
#include <math.h>
#include <inttypes.h>

#define E 2.718281828459045235360287471352662497757

int main() {
    printf("=== Testing Superior e Convergents ===\n\n");

    // Convergent 19: Best that fits in int32_t
    Rational conv19 = {
        .numerator = 28245729,
        .denominator = 10391023
    };

    printf("Convergent 19: %" PRId64 "/%" PRId64 "\n", conv19.numerator, conv19.denominator);
    double val19 = (double)conv19.numerator / (double)conv19.denominator;
    double err19 = fabs(val19 - E);
    printf("  Value: %.20f\n", val19);
    printf("  Error: %.20e\n", err19);
    printf("  Size check: num=%lld, denom=%lld\n",
           (long long)conv19.numerator, (long long)conv19.denominator);
    printf("  Fits in int32_t: %s\n\n",
           (conv19.numerator <= INT32_MAX && conv19.denominator <= INT32_MAX) ? "YES" : "NO");

    // Convert to CompactRational
    printf("Converting to CompactRational...\n");
    CompactRational cr19 = cr_from_fraction((int32_t)conv19.numerator,
                                            (int32_t)conv19.denominator, NULL);

    printf("  Mathematical form: ");
    cr_print(&cr19);
    printf("\n");
    printf("  Size: %zu bytes\n", cr_size(&cr19));
    printf("  Encoding: ");
    cr_print_encoding(&cr19);

    double cr_val = cr_to_double(&cr19, NULL);
    double cr_err = fabs(cr_val - E);
    printf("\n  CR Value: %.20f\n", cr_val);
    printf("  CR Error: %.20e\n", cr_err);
    printf("  CR Relative error: %.20e%%\n\n", (cr_err / E) * 100);

    // Compare with our previous best
    printf("=== Comparison ===\n\n");

    CompactRational two_tuple;
    cr_init(&two_tuple);
    two_tuple.whole = 0x8002;
    two_tuple.tuples[0] = ((uint16_t)55 << 8) | 38;
    two_tuple.tuples[1] = ((uint16_t)89 << 8) | 0xE6;

    printf("Previous best (2 + 55/166 + 89/230):\n");
    printf("  Value: %.20f\n", cr_to_double(&two_tuple, NULL));
    printf("  Error: %.20e\n", fabs(cr_to_double(&two_tuple, NULL) - E));
    printf("  Size:  6 bytes\n");
    printf("  This equals convergent 12: 25946/9545\n\n");

    printf("Convergent 19 via cr_from_fraction:\n");
    printf("  Value: %.20f\n", cr_val);
    printf("  Error: %.20e\n", cr_err);
    printf("  Size:  %zu bytes\n\n", cr_size(&cr19));

    if (cr_err < fabs(cr_to_double(&two_tuple, NULL) - E)) {
        printf("✓ Convergent 19 is %.1fx MORE accurate!\n",
               fabs(cr_to_double(&two_tuple, NULL) - E) / cr_err);
    } else {
        printf("✗ Convergent 19 loses precision in CompactRational encoding.\n");
        printf("  (This happens when the denominator doesn't fit the antichain format well)\n");
    }

    // Also test convergent 20 (even better, but larger)
    printf("\n=== Convergent 20 (for comparison) ===\n\n");
    Rational conv20 = {
        .numerator = 410105312,
        .denominator = 150869313
    };

    printf("Convergent 20: %" PRId64 "/%" PRId64 "\n", conv20.numerator, conv20.denominator);
    printf("  Fits in int32_t: %s\n",
           (conv20.numerator <= INT32_MAX && conv20.denominator <= INT32_MAX) ? "YES" : "NO");

    if (conv20.numerator <= INT32_MAX && conv20.denominator <= INT32_MAX) {
        CompactRational cr20 = cr_from_fraction((int32_t)conv20.numerator,
                                                (int32_t)conv20.denominator, NULL);
        printf("  CR Value: %.20f\n", cr_to_double(&cr20, NULL));
        printf("  CR Error: %.20e\n", fabs(cr_to_double(&cr20, NULL) - E));
        printf("  CR Size:  %zu bytes\n", cr_size(&cr20));
    }

    return 0;
}
