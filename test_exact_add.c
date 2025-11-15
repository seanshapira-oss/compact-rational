#include "compact_rational.h"
#include <stdio.h>
#include <math.h>

// Demonstration of precision loss in current cr_add()
void demonstrate_precision_loss() {
    printf("=== Current cr_add() Precision Loss ===\n\n");

    // Create two simple fractions: 1/3 and 1/5
    CompactRational a = cr_from_fraction(1, 3);
    CompactRational b = cr_from_fraction(1, 5);

    printf("Input a: ");
    cr_print(&a);
    printf("\n  Encoding: ");
    cr_print_encoding(&a);
    printf("  Decimal: %.20f\n\n", cr_to_double(&a));

    printf("Input b: ");
    cr_print(&b);
    printf("\n  Encoding: ");
    cr_print_encoding(&b);
    printf("  Decimal: %.20f\n\n", cr_to_double(&b));

    // Add them using current implementation
    CompactRational sum = cr_add(&a, &b);

    printf("Result (current cr_add):\n");
    printf("  ");
    cr_print(&sum);
    printf("\n  Encoding: ");
    cr_print_encoding(&sum);
    printf("  Size: %zu bytes\n", cr_size(&sum));
    printf("  Decimal: %.20f\n\n", cr_to_double(&sum));

    // What should it be?
    double expected = 1.0/3.0 + 1.0/5.0;  // 8/15
    double actual = cr_to_double(&sum);
    double error = fabs(actual - expected);

    printf("Expected (1/3 + 1/5 = 8/15): %.20f\n", expected);
    printf("Error: %.20e\n\n", error);

    // What if we could keep both tuples?
    printf("=== Ideal Two-Tuple Representation ===\n\n");
    CompactRational ideal;
    cr_init(&ideal);

    // Manually construct: 0 + 43/129 + 26/130 (which is 1/3 + 1/5 in antichain form)
    ideal.whole = 0x8000;  // 0 with tuples
    ideal.tuples[0] = ((uint16_t)43 << 8) | 0x01;  // 43/129 (1/3), no end flag
    ideal.tuples[1] = ((uint16_t)26 << 8) | 0x82;  // 26/130 (1/5), end flag

    printf("Ideal two-tuple: ");
    cr_print(&ideal);
    printf("\n  Encoding: ");
    cr_print_encoding(&ideal);
    printf("  Size: %zu bytes\n", cr_size(&ideal));
    printf("  Decimal: %.20f\n", cr_to_double(&ideal));
    printf("  Error: %.20e\n\n", fabs(cr_to_double(&ideal) - expected));
}

// Test if we can represent sum exactly with two tuples
void test_exact_two_tuple_sum() {
    printf("=== Testing Exact Two-Tuple Sum ===\n\n");

    // Test case 1: 1/2 + 1/3
    printf("Test 1: 1/2 + 1/3 = 5/6\n");
    CompactRational half = cr_from_fraction(1, 2);
    CompactRational third = cr_from_fraction(1, 3);

    printf("  1/2 as CompactRational: ");
    cr_print_encoding(&half);
    printf("  1/3 as CompactRational: ");
    cr_print_encoding(&third);

    // Extract the tuple info
    uint8_t n1 = (half.tuples[0] >> 8) & 0xFF;
    uint8_t d1_offset = half.tuples[0] & 0x7F;
    uint8_t d1 = MIN_DENOMINATOR + d1_offset;

    uint8_t n2 = (third.tuples[0] >> 8) & 0xFF;
    uint8_t d2_offset = third.tuples[0] & 0x7F;
    uint8_t d2 = MIN_DENOMINATOR + d2_offset;

    printf("  Tuple 1: %d/%d\n", n1, d1);
    printf("  Tuple 2: %d/%d\n", n2, d2);

    if (d1 != d2) {
        printf("  ✓ Different denominators - can combine exactly!\n\n");

        // Create exact two-tuple sum
        CompactRational exact_sum;
        cr_init(&exact_sum);
        exact_sum.whole = 0x8000;  // 0 with tuples
        exact_sum.tuples[0] = ((uint16_t)n1 << 8) | d1_offset;  // First tuple, no end
        exact_sum.tuples[1] = ((uint16_t)n2 << 8) | (0x80 | d2_offset);  // Second tuple, end

        printf("  Exact two-tuple sum: ");
        cr_print(&exact_sum);
        printf("\n    ");
        cr_print_encoding(&exact_sum);

        double exact_val = cr_to_double(&exact_sum);
        double expected = 1.0/2.0 + 1.0/3.0;
        printf("    Value: %.20f\n", exact_val);
        printf("    Expected: %.20f\n", expected);
        printf("    Error: %.20e\n\n", fabs(exact_val - expected));
    } else {
        printf("  ✗ Same denominator - need to combine first\n\n");
    }

    // Test case 2: Compare with current implementation
    CompactRational current_sum = cr_add(&half, &third);
    printf("  Current cr_add result: ");
    cr_print(&current_sum);
    printf("\n    ");
    cr_print_encoding(&current_sum);
    printf("    Value: %.20f\n", cr_to_double(&current_sum));
    printf("    Error: %.20e\n\n", fabs(cr_to_double(&current_sum) - (1.0/2.0 + 1.0/3.0)));
}

int main() {
    demonstrate_precision_loss();
    printf("\n");
    test_exact_two_tuple_sum();

    printf("=== Conclusion ===\n\n");
    printf("When adding two single-tuple CompactRationals:\n");
    printf("1. If they have DIFFERENT antichain denominators:\n");
    printf("   → We CAN represent the sum exactly with two tuples\n");
    printf("   → No precision loss!\n");
    printf("   → Just combine the tuples directly\n\n");
    printf("2. If they have the SAME denominator:\n");
    printf("   → Combine numerators first: (n1+n2)/d\n");
    printf("   → May need to extract whole part if n1+n2 >= d\n");
    printf("   → Result is still single-tuple\n\n");
    printf("3. Current implementation:\n");
    printf("   → Converts to Rational, adds, reduces, re-encodes\n");
    printf("   → Can lose precision in re-encoding step\n");
    printf("   → Always produces single-tuple result\n");

    return 0;
}
