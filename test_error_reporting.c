#include "compact_rational.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// ERROR REPORTING TEST CASES
// ============================================================================

void test_error_reporting() {
    printf("=== Error Reporting Tests ===\n\n");
    CRError error;

    // Test 1: Value clamping - too large
    printf("Test 1: Value clamping (too large)\n");
    CompactRational cr1 = cr_from_int(20000, &error);
    if (error.code != CR_SUCCESS) {
        printf("  ERROR: %s\n", error.message);
        printf("  Original value: %d, Limit: %d\n", error.value1, error.value2);
    }
    printf("  Result: ");
    cr_print(&cr1);
    printf("\n\n");

    // Test 2: Value clamping - too small
    printf("Test 2: Value clamping (too small)\n");
    CompactRational cr2 = cr_from_int(-20000, &error);
    if (error.code != CR_SUCCESS) {
        printf("  ERROR: %s\n", error.message);
        printf("  Original value: %d, Limit: %d\n", error.value1, error.value2);
    }
    printf("  Result: ");
    cr_print(&cr2);
    printf("\n\n");

    // Test 3: Normal value (no error)
    printf("Test 3: Normal value (no error)\n");
    CompactRational cr3 = cr_from_int(42, &error);
    if (error.code == CR_SUCCESS) {
        printf("  SUCCESS: No errors\n");
    }
    printf("  Result: ");
    cr_print(&cr3);
    printf("\n\n");

    // Test 4: Division by zero
    printf("Test 4: Division by zero\n");
    CompactRational cr4 = cr_from_fraction(5, 0, &error);
    if (error.code != CR_SUCCESS) {
        printf("  ERROR: %s\n", error.message);
        printf("  Code: %d\n", error.code);
    }
    printf("  Result: ");
    cr_print(&cr4);
    printf("\n\n");

    // Test 5: Fraction with clamping
    printf("Test 5: Fraction with clamping\n");
    CompactRational cr5 = cr_from_fraction(50000, 3, &error);
    if (error.code != CR_SUCCESS) {
        printf("  ERROR: %s\n", error.message);
        printf("  Original whole part: %d, Limit: %d\n", error.value1, error.value2);
    }
    printf("  Result: ");
    cr_print(&cr5);
    printf("\n\n");

    // Test 6: Addition without error
    printf("Test 6: Addition (no error)\n");
    CompactRational a = cr_from_int(10, NULL);
    CompactRational b = cr_from_int(20, NULL);
    CompactRational sum = cr_add(&a, &b, &error);
    if (error.code == CR_SUCCESS) {
        printf("  SUCCESS: No errors\n");
    }
    printf("  10 + 20 = ");
    cr_print(&sum);
    printf("\n\n");

    // Test 7: Test overflow detection (if we can trigger it)
    printf("Test 7: Large addition (checking for overflow handling)\n");
    CompactRational large1 = cr_from_int(16000, NULL);
    CompactRational large2 = cr_from_int(16000, NULL);
    CompactRational large_sum = cr_add(&large1, &large2, &error);
    if (error.code != CR_SUCCESS) {
        printf("  ERROR: %s\n", error.message);
    } else {
        printf("  SUCCESS: Result within range\n");
    }
    printf("  16000 + 16000 = ");
    cr_print(&large_sum);
    printf("\n\n");

    // Test 8: Using NULL for error (backward compatibility)
    printf("Test 8: Backward compatibility (NULL error parameter)\n");
    CompactRational cr8 = cr_from_int(100, NULL);  // No error reporting
    printf("  Created value without error checking: ");
    cr_print(&cr8);
    printf("\n\n");

    // Test 9: Chained operations with error checking
    printf("Test 9: Chained operations with error checking\n");
    CompactRational v1 = cr_from_fraction(15, 2, &error);
    if (error.code != CR_SUCCESS) {
        printf("  Error in first fraction: %s\n", error.message);
    }

    CompactRational v2 = cr_from_fraction(25, 3, &error);
    if (error.code != CR_SUCCESS) {
        printf("  Error in second fraction: %s\n", error.message);
    }

    CompactRational result = cr_add(&v1, &v2, &error);
    if (error.code != CR_SUCCESS) {
        printf("  Error in addition: %s\n", error.message);
    } else {
        printf("  SUCCESS: All operations completed without errors\n");
    }

    printf("  15/2 + 25/3 = ");
    cr_print(&result);
    printf("\n\n");

    // Test 10: cr_to_double with error checking
    printf("Test 10: cr_to_double with error checking\n");
    CompactRational normal = cr_from_int(42, NULL);
    double d = cr_to_double(&normal, &error);
    if (error.code == CR_SUCCESS) {
        printf("  SUCCESS: Conversion to double successful\n");
    }
    printf("  Double value: %.6f\n", d);
    printf("\n");

    printf("=== All Error Reporting Tests Complete ===\n");
}

int main() {
    test_error_reporting();
    return 0;
}
