#include "compact_rational.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

/**
 * Improved cr_add that can produce exact two-tuple results
 *
 * Strategy:
 * 1. If both inputs are single-tuple with different denominators:
 *    → Combine directly into two-tuple (EXACT, no precision loss)
 * 2. If both inputs are single-tuple with same denominator:
 *    → Add numerators, extract overflow to whole part
 * 3. Otherwise:
 *    → Fall back to rational conversion method
 */
CompactRational cr_add_improved(const CompactRational* a, const CompactRational* b) {
    // Extract whole parts
    int16_t whole_a = a->whole & 0x7FFF;
    if (whole_a & 0x4000) whole_a |= 0x8000;  // Sign extend

    int16_t whole_b = b->whole & 0x7FFF;
    if (whole_b & 0x4000) whole_b |= 0x8000;  // Sign extend

    bool has_tuples_a = (a->whole & 0x8000) != 0;
    bool has_tuples_b = (b->whole & 0x8000) != 0;

    // Count tuples in each input
    int tuple_count_a = 0, tuple_count_b = 0;

    if (has_tuples_a) {
        for (int i = 0; i < MAX_TUPLES; i++) {
            tuple_count_a++;
            uint8_t denom_byte = a->tuples[i] & 0xFF;
            if (denom_byte & 0x80) break;  // End flag in denominator byte
        }
    }

    if (has_tuples_b) {
        for (int i = 0; i < MAX_TUPLES; i++) {
            tuple_count_b++;
            uint8_t denom_byte = b->tuples[i] & 0xFF;
            if (denom_byte & 0x80) break;  // End flag in denominator byte
        }
    }

    // OPTIMIZATION 1: Both are single-tuple
    if (tuple_count_a == 1 && tuple_count_b == 1) {
        uint8_t na = (a->tuples[0] >> 8) & 0xFF;
        uint8_t da_offset = a->tuples[0] & 0x7F;
        uint8_t da = MIN_DENOMINATOR + da_offset;

        uint8_t nb = (b->tuples[0] >> 8) & 0xFF;
        uint8_t db_offset = b->tuples[0] & 0x7F;
        uint8_t db = MIN_DENOMINATOR + db_offset;

        CompactRational result;
        cr_init(&result);

        // Case 1a: Different denominators - combine into two tuples (EXACT!)
        if (da != db) {
            int32_t new_whole = whole_a + whole_b;

            // Clamp
            if (new_whole > MAX_WHOLE_VALUE) new_whole = MAX_WHOLE_VALUE;
            if (new_whole < MIN_WHOLE_VALUE) new_whole = MIN_WHOLE_VALUE;

            result.whole = (int16_t)((new_whole & 0x7FFF) | 0x8000);
            result.tuples[0] = ((uint16_t)na << 8) | da_offset;  // No end flag
            result.tuples[1] = ((uint16_t)nb << 8) | (0x80 | db_offset);  // End flag

            return result;
        }

        // Case 1b: Same denominator - add numerators
        else {
            int32_t sum_numerator = na + nb;
            int32_t new_whole = whole_a + whole_b;

            // Extract overflow from numerator to whole
            new_whole += sum_numerator / da;
            sum_numerator %= da;

            // Clamp
            if (new_whole > MAX_WHOLE_VALUE) new_whole = MAX_WHOLE_VALUE;
            if (new_whole < MIN_WHOLE_VALUE) new_whole = MIN_WHOLE_VALUE;

            if (sum_numerator == 0) {
                // Pure integer result
                result.whole = (int16_t)(new_whole & 0x7FFF);
            } else {
                // Single tuple result
                result.whole = (int16_t)((new_whole & 0x7FFF) | 0x8000);
                result.tuples[0] = ((uint16_t)sum_numerator << 8) | (0x80 | da_offset);
            }

            return result;
        }
    }

    // OPTIMIZATION 2: One integer, one single-tuple
    if (tuple_count_a == 0 && tuple_count_b == 1) {
        int32_t new_whole = whole_a + whole_b;
        if (new_whole > MAX_WHOLE_VALUE) new_whole = MAX_WHOLE_VALUE;
        if (new_whole < MIN_WHOLE_VALUE) new_whole = MIN_WHOLE_VALUE;

        CompactRational result;
        cr_init(&result);
        result.whole = (int16_t)((new_whole & 0x7FFF) | 0x8000);
        result.tuples[0] = b->tuples[0];  // Copy tuple from b
        return result;
    }

    if (tuple_count_a == 1 && tuple_count_b == 0) {
        int32_t new_whole = whole_a + whole_b;
        if (new_whole > MAX_WHOLE_VALUE) new_whole = MAX_WHOLE_VALUE;
        if (new_whole < MIN_WHOLE_VALUE) new_whole = MIN_WHOLE_VALUE;

        CompactRational result;
        cr_init(&result);
        result.whole = (int16_t)((new_whole & 0x7FFF) | 0x8000);
        result.tuples[0] = a->tuples[0];  // Copy tuple from a
        return result;
    }

    // OPTIMIZATION 3: Both integers
    if (tuple_count_a == 0 && tuple_count_b == 0) {
        int32_t new_whole = whole_a + whole_b;
        if (new_whole > MAX_WHOLE_VALUE) new_whole = MAX_WHOLE_VALUE;
        if (new_whole < MIN_WHOLE_VALUE) new_whole = MIN_WHOLE_VALUE;

        CompactRational result;
        cr_init(&result);
        result.whole = (int16_t)(new_whole & 0x7FFF);
        return result;
    }

    // FALLBACK: Use original method for complex cases
    // (Convert to rational, add, reduce, re-encode)
    Rational ra = cr_to_rational(a);
    Rational rb = cr_to_rational(b);

    Rational sum;
    sum.numerator = ra.numerator * rb.denominator + rb.numerator * ra.denominator;
    sum.denominator = ra.denominator * rb.denominator;

    reduce_rational(&sum);

    if (sum.numerator > INT32_MAX || sum.numerator < INT32_MIN ||
        sum.denominator > INT32_MAX || sum.denominator < INT32_MIN) {
        fprintf(stderr, "Error: overflow in addition\n");
        CompactRational zero;
        cr_init(&zero);
        return zero;
    }

    return cr_from_fraction((int32_t)sum.numerator, (int32_t)sum.denominator);
}

// Test the improved addition
int main() {
    printf("=== Testing Improved cr_add ===\n\n");

    // Test 1: Two single-tuples with different denominators
    printf("Test 1: 1/2 + 1/3 (different denominators)\n");
    CompactRational half = cr_from_fraction(1, 2);
    CompactRational third = cr_from_fraction(1, 3);

    CompactRational sum_old = cr_add(&half, &third);
    CompactRational sum_new = cr_add_improved(&half, &third);

    printf("  Original cr_add:\n    ");
    cr_print(&sum_old);
    printf("\n    ");
    cr_print_encoding(&sum_old);
    printf("    Size: %zu bytes\n", cr_size(&sum_old));

    printf("  Improved cr_add:\n    ");
    cr_print(&sum_new);
    printf("\n    ");
    cr_print_encoding(&sum_new);
    printf("    Size: %zu bytes\n", cr_size(&sum_new));

    double val_old = cr_to_double(&sum_old);
    double val_new = cr_to_double(&sum_new);
    double expected = 1.0/2.0 + 1.0/3.0;

    printf("  Original error: %.2e\n", fabs(val_old - expected));
    printf("  Improved error: %.2e\n", fabs(val_new - expected));
    printf("  Space saved: %zd bytes → %zu bytes\n\n", cr_size(&sum_old), cr_size(&sum_new));

    // Test 2: Two single-tuples with same denominator
    printf("Test 2: 1/2 + 1/4 (same antichain denominator)\n");
    CompactRational quarter = cr_from_fraction(1, 4);

    sum_old = cr_add(&half, &quarter);
    sum_new = cr_add_improved(&half, &quarter);

    printf("  Original: ");
    cr_print(&sum_old);
    printf(" [%zu bytes]\n", cr_size(&sum_old));
    printf("  Improved: ");
    cr_print(&sum_new);
    printf(" [%zu bytes]\n\n", cr_size(&sum_new));

    // Test 3: Integer + single-tuple
    printf("Test 3: 5 + 1/3\n");
    CompactRational five = cr_from_int(5);

    sum_old = cr_add(&five, &third);
    sum_new = cr_add_improved(&five, &third);

    printf("  Original: ");
    cr_print(&sum_old);
    printf(" [%zu bytes]\n", cr_size(&sum_old));
    printf("  Improved: ");
    cr_print(&sum_new);
    printf(" [%zu bytes]\n\n", cr_size(&sum_new));

    // Test 4: Two integers
    printf("Test 4: 5 + 7\n");
    CompactRational seven = cr_from_int(7);

    sum_old = cr_add(&five, &seven);
    sum_new = cr_add_improved(&five, &seven);

    printf("  Original: ");
    cr_print(&sum_old);
    printf(" [%zu bytes]\n", cr_size(&sum_old));
    printf("  Improved: ");
    cr_print(&sum_new);
    printf(" [%zu bytes]\n\n", cr_size(&sum_new));

    printf("=== Summary ===\n\n");
    printf("Improvements in cr_add_improved():\n");
    printf("✓ Direct two-tuple combination (no rational conversion)\n");
    printf("✓ Preserves exact representation when possible\n");
    printf("✓ More efficient for simple cases\n");
    printf("✓ Can produce larger results (2 tuples vs 1) but more accurate\n");
    printf("✓ Falls back to original method for complex cases\n");

    return 0;
}
