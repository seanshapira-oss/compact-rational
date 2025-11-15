#include "compact_rational.h"
#include <stdio.h>
#include <math.h>
#include <inttypes.h>

#define E 2.718281828459045235360287471352662497757

int main() {
    // High-precision rational approximation of e
    Rational e_approx = {
        .numerator = 1484783350961841221LL,
        .denominator = 546197992715055416LL
    };

    printf("=== Analyzing High-Precision e Approximation ===\n\n");

    // Calculate the actual value
    double value = (double)e_approx.numerator / (double)e_approx.denominator;
    double error = fabs(value - E);

    printf("Input rational:\n");
    printf("  Numerator:    %" PRId64 "\n", e_approx.numerator);
    printf("  Denominator:  %" PRId64 "\n", e_approx.denominator);
    printf("  Value:        %.20f\n", value);
    printf("  True e:       %.20f\n", E);
    printf("  Error:        %.20e\n", error);
    printf("  Relative err: %.20e (%.15f%%)\n", error / E, (error / E) * 100);
    printf("\n");

    // Verify it's a valid approximation
    if (error > 1e-10) {
        printf("WARNING: This approximation has error > 1e-10\n");
        printf("This may not be an accurate representation of e.\n\n");
    } else {
        printf("✓ This is an excellent approximation of e!\n\n");
    }

    // Try to convert to CompactRational
    printf("=== Converting to CompactRational ===\n\n");

    // Check if it fits in int32_t range
    if (e_approx.numerator > INT32_MAX || e_approx.numerator < INT32_MIN ||
        e_approx.denominator > INT32_MAX || e_approx.denominator < INT32_MIN) {
        printf("The rational is too large for direct CompactRational conversion.\n");
        printf("(Requires int32_t, but values are int64_t)\n\n");

        // Reduce to find a simpler approximation
        printf("Reducing to simpler form...\n");
        Rational reduced = e_approx;
        reduce_rational(&reduced);

        printf("  Reduced numerator:   %" PRId64 "\n", reduced.numerator);
        printf("  Reduced denominator: %" PRId64 "\n", reduced.denominator);

        if (reduced.numerator <= INT32_MAX && reduced.denominator <= INT32_MAX) {
            printf("\n✓ Reduced form fits in int32_t!\n\n");

            CompactRational cr = cr_from_fraction((int32_t)reduced.numerator,
                                                  (int32_t)reduced.denominator);

            printf("CompactRational representation:\n");
            printf("  Value: ");
            cr_print(&cr);
            printf("\n");
            printf("  Size: %zu bytes\n", cr_size(&cr));
            printf("  Encoding: ");
            cr_print_encoding(&cr);

            double cr_value = cr_to_double(&cr);
            double cr_error = fabs(cr_value - E);

            printf("\n  CR value:     %.20f\n", cr_value);
            printf("  CR error:     %.20e\n", cr_error);
            printf("  CR rel err:   %.20e (%.15f%%)\n",
                   cr_error / E, (cr_error / E) * 100);
        } else {
            printf("\n✗ Even reduced form is too large for CompactRational.\n");
            printf("  Max allowed: ±%d\n", INT32_MAX);
        }
    } else {
        // Direct conversion possible
        CompactRational cr = cr_from_fraction((int32_t)e_approx.numerator,
                                              (int32_t)e_approx.denominator);

        printf("Direct CompactRational conversion:\n");
        printf("  Value: ");
        cr_print(&cr);
        printf("\n");
        printf("  Size: %zu bytes\n", cr_size(&cr));
        printf("  Encoding: ");
        cr_print_encoding(&cr);
    }

    printf("\n=== Comparison with Simple Approximations ===\n\n");

    // Compare with our previously found best single-tuple
    CompactRational simple;
    cr_init(&simple);
    simple.whole = 0x8002;  // 2 with tuples
    simple.tuples[0] = ((uint16_t)181 << 8) | 0xFC;  // 181/252, end

    printf("Best single-tuple (2 + 181/252):\n");
    printf("  Value: %.20f\n", cr_to_double(&simple));
    printf("  Error: %.20e\n", fabs(cr_to_double(&simple) - E));
    printf("  Size:  4 bytes\n\n");

    // Compare with two-tuple
    CompactRational two_tuple;
    cr_init(&two_tuple);
    two_tuple.whole = 0x8002;  // 2 with tuples
    two_tuple.tuples[0] = ((uint16_t)55 << 8) | 38;  // 55/166
    two_tuple.tuples[1] = ((uint16_t)89 << 8) | 0xE6;  // 89/230, end

    printf("Best two-tuple (2 + 55/166 + 89/230):\n");
    printf("  Value: %.20f\n", cr_to_double(&two_tuple));
    printf("  Error: %.20e\n", fabs(cr_to_double(&two_tuple) - E));
    printf("  Size:  6 bytes\n");

    return 0;
}
