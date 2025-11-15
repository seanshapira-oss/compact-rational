#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    int16_t whole;
    uint16_t tuples[5];
} CompactRational;

typedef struct {
    int64_t numerator;
    int64_t denominator;
} Rational;

// From compact_rational.c
extern CompactRational cr_from_fraction(int32_t num, int32_t denom);
extern Rational cr_to_rational(const CompactRational* cr);
extern void cr_print(const CompactRational* cr);
extern CompactRational cr_add(const CompactRational* a, const CompactRational* b);
extern double cr_to_double(const CompactRational* cr);

int main() {
    printf("=== Testing cr_add() Correctness ===\n\n");

    printf("Test: 12/37 + 15/47\n\n");

    // Expected result: (12*47 + 15*37)/(37*47) = (564 + 555)/1739 = 1119/1739
    printf("Expected (exact): 1119/1739 = %.10f\n", 1119.0/1739.0);

    printf("\nStep 1: Create 12/37\n");
    CompactRational a = cr_from_fraction(12, 37);
    printf("  Stored as: ");
    cr_print(&a);
    printf("\n");
    Rational ra = cr_to_rational(&a);
    printf("  Reads back as: %lld/%lld = %.10f\n",
           (long long)ra.numerator, (long long)ra.denominator,
           (double)ra.numerator / (double)ra.denominator);

    printf("\nStep 2: Create 15/47\n");
    CompactRational b = cr_from_fraction(15, 47);
    printf("  Stored as: ");
    cr_print(&b);
    printf("\n");
    Rational rb = cr_to_rational(&b);
    printf("  Reads back as: %lld/%lld = %.10f\n",
           (long long)rb.numerator, (long long)rb.denominator,
           (double)rb.numerator / (double)rb.denominator);

    printf("\nStep 3: Add them using cr_add()\n");
    CompactRational sum = cr_add(&a, &b);
    printf("  Result: ");
    cr_print(&sum);
    printf("\n");

    Rational rsum = cr_to_rational(&sum);
    printf("  As rational: %lld/%lld = %.10f\n",
           (long long)rsum.numerator, (long long)rsum.denominator,
           (double)rsum.numerator / (double)rsum.denominator);

    printf("\nComparison:\n");
    double expected = 1119.0 / 1739.0;
    double actual = cr_to_double(&sum);
    double error = fabs(expected - actual);
    double rel_error = error / expected * 100.0;

    printf("  Expected: %.10f\n", expected);
    printf("  Actual:   %.10f\n", actual);
    printf("  Error:    %.10f (%.4f%%)\n", error, rel_error);

    if (error < 0.0001) {
        printf("  ✓ CORRECT (within tolerance)\n");
    } else {
        printf("  ✗ INCORRECT (significant error)\n");
    }

    return 0;
}
