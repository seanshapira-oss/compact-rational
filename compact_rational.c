#include "compact_rational.h"
#include <stdio.h>

// ============================================================================
// TEST CASES
// ============================================================================

void run_tests() {
    printf("=== Compact Rational Number Tests ===\n\n");

    // Test 1: Pure integers
    printf("Test 1: Pure integers\n");
    CompactRational cr1 = cr_from_int(42);
    printf("  42 -> ");
    cr_print(&cr1);
    printf(" [%zu bytes]\n", cr_size(&cr1));
    cr_print_encoding(&cr1);
    printf("\n");

    // Test 2: Simple fractions
    printf("Test 2: Simple fractions\n");
    CompactRational cr2 = cr_from_fraction(22, 3);  // 7 1/3
    printf("  22/3 -> ");
    cr_print(&cr2);
    printf(" [%zu bytes]\n", cr_size(&cr2));
    cr_print_encoding(&cr2);
    printf("\n");

    // Test 3: Halves, quarters, eighths (same antichain denom)
    printf("Test 3: Common fractions\n");
    struct {
        int num, denom;
        const char* name;
    } fractions[] = {
        {1, 2, "1/2"},
        {1, 3, "1/3"},
        {1, 4, "1/4"},
        {1, 5, "1/5"},
        {1, 6, "1/6"},
        {1, 7, "1/7"},
        {1, 8, "1/8"},
    };

    for (int i = 0; i < 7; i++) {
        CompactRational cr = cr_from_fraction(fractions[i].num, fractions[i].denom);
        printf("  %s -> ", fractions[i].name);
        cr_print(&cr);
        printf(" [%zu bytes]\n", cr_size(&cr));
    }
    printf("\n");

    // Test 4: Addition (score summing)
    printf("Test 4: Addition (summing scores)\n");
    CompactRational score1 = cr_from_fraction(15, 2);  // 7.5
    CompactRational score2 = cr_from_fraction(25, 3);  // 8.333...
    CompactRational score3 = cr_from_fraction(41, 5);  // 8.2

    printf("  Score 1: ");
    cr_print(&score1);
    printf("\n");
    printf("  Score 2: ");
    cr_print(&score2);
    printf("\n");
    printf("  Score 3: ");
    cr_print(&score3);
    printf("\n");

    CompactRational sum = cr_add(&score1, &score2);
    sum = cr_add(&sum, &score3);

    printf("  Total: ");
    cr_print(&sum);
    printf(" [%zu bytes]\n", cr_size(&sum));
    cr_print_encoding(&sum);
    printf("\n");

    // Test 5: Realistic grading scenario
    printf("Test 5: Realistic grading scenario\n");
    CompactRational grades[] = {
        cr_from_fraction(17, 2),   // 8.5
        cr_from_fraction(28, 3),   // 9.333...
        cr_from_fraction(19, 2),   // 9.5
        cr_from_int(10),           // 10
        cr_from_fraction(31, 3),   // 10.333...
    };

    CompactRational total = cr_from_int(0);
    for (int i = 0; i < 5; i++) {
        printf("  Assignment %d: ", i + 1);
        cr_print(&grades[i]);
        printf(" [%zu bytes]\n", cr_size(&grades[i]));
        total = cr_add(&total, &grades[i]);
    }

    printf("  Final Grade: ");
    cr_print(&total);
    printf(" [%zu bytes]\n", cr_size(&total));
    cr_print_encoding(&total);
    printf("\n");

    // Test 6: Negative numbers
    printf("Test 6: Negative numbers\n");
    CompactRational neg1 = cr_from_fraction(-7, 2);
    printf("  -7/2 -> ");
    cr_print(&neg1);
    printf(" [%zu bytes]\n", cr_size(&neg1));
    cr_print_encoding(&neg1);
    printf("\n");

    // Test 7: Edge cases
    printf("Test 7: Edge cases\n");
    CompactRational max_val = cr_from_int(16383);
    printf("  Max whole (16383): ");
    cr_print(&max_val);
    printf("\n");

    CompactRational min_val = cr_from_int(-16383);
    printf("  Min whole (-16383): ");
    cr_print(&min_val);
    printf("\n");

    // Test 8: Negative integers (NEW - addresses code review)
    printf("\nTest 8: Negative integers without fractions\n");
    CompactRational neg_int1 = cr_from_int(-1);
    printf("  -1 -> ");
    cr_print(&neg_int1);
    printf(" [%zu bytes]\n", cr_size(&neg_int1));
    cr_print_encoding(&neg_int1);

    CompactRational neg_int2 = cr_from_int(-100);
    printf("  -100 -> ");
    cr_print(&neg_int2);
    printf(" [%zu bytes]\n", cr_size(&neg_int2));
    cr_print_encoding(&neg_int2);

    CompactRational neg_int3 = cr_from_int(-16383);
    printf("  -16383 -> ");
    cr_print(&neg_int3);
    printf(" [%zu bytes]\n", cr_size(&neg_int3));
    cr_print_encoding(&neg_int3);

    // Test 9: Boundary value tests (NEW - addresses code review)
    printf("\nTest 9: Boundary values (clamping tests)\n");
    printf("  Testing value above max (16384):\n  ");
    CompactRational above_max = cr_from_int(16384);
    cr_print(&above_max);
    printf("\n");

    printf("  Testing value below min (-16384):\n  ");
    CompactRational below_min = cr_from_int(-16384);
    cr_print(&below_min);
    printf("\n");

    printf("  Testing very large value (1000000):\n  ");
    CompactRational very_large = cr_from_int(1000000);
    cr_print(&very_large);
    printf("\n");

    // Test 10: Error case - division by zero (NEW - addresses code review)
    printf("\nTest 10: Error cases\n");
    printf("  Testing division by zero:\n  ");
    CompactRational zero_denom = cr_from_fraction(1, 0);
    cr_print(&zero_denom);
    printf("\n");

    // Test 11: Negative fractions (NEW - addresses code review)
    printf("\nTest 11: Negative fractions\n");
    CompactRational neg_frac1 = cr_from_fraction(-5, 2);
    printf("  -5/2 -> ");
    cr_print(&neg_frac1);
    printf(" [%zu bytes]\n", cr_size(&neg_frac1));
    cr_print_encoding(&neg_frac1);

    CompactRational neg_frac2 = cr_from_fraction(5, -3);
    printf("  5/-3 -> ");
    cr_print(&neg_frac2);
    printf(" [%zu bytes]\n", cr_size(&neg_frac2));
    cr_print_encoding(&neg_frac2);

    CompactRational neg_frac3 = cr_from_fraction(-22, -3);
    printf("  -22/-3 -> ");
    cr_print(&neg_frac3);
    printf(" [%zu bytes]\n", cr_size(&neg_frac3));
    cr_print_encoding(&neg_frac3);

    // Test 12: Addition with negative numbers (NEW - addresses code review)
    printf("\nTest 12: Addition with negative numbers\n");
    CompactRational pos_num = cr_from_int(10);
    CompactRational neg_num = cr_from_int(-7);
    CompactRational sum_mixed = cr_add(&pos_num, &neg_num);
    printf("  10 + (-7) = ");
    cr_print(&sum_mixed);
    printf("\n");

    CompactRational neg_a = cr_from_int(-5);
    CompactRational neg_b = cr_from_int(-3);
    CompactRational sum_neg = cr_add(&neg_a, &neg_b);
    printf("  (-5) + (-3) = ");
    cr_print(&sum_neg);
    printf("\n");

    printf("\n=== All Tests Complete ===\n");
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    run_tests();
    return 0;
}
