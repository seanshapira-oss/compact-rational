#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define MAX_TUPLES 5
#define MIN_DENOMINATOR 128
#define MAX_DENOMINATOR 255
#define MAX_NUMERATOR 255
#define MAX_WHOLE_VALUE 16383
#define MIN_WHOLE_VALUE -16383

typedef struct {
    int16_t whole;
    uint16_t tuples[MAX_TUPLES];
} CompactRational;

typedef struct {
    int64_t numerator;
    int64_t denominator;
} Rational;

// ============================================================================
// OPTIMAL ENCODING: Find best antichain representation
// ============================================================================

int64_t gcd(int64_t a, int64_t b) {
    a = llabs(a);
    b = llabs(b);
    while (b != 0) {
        int64_t temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

void reduce_rational(Rational* r) {
    if (r->denominator == 0) return;
    int64_t g = gcd(r->numerator, r->denominator);
    r->numerator /= g;
    r->denominator /= g;
    if (r->denominator < 0) {
        r->numerator = -r->numerator;
        r->denominator = -r->denominator;
    }
}

void cr_init(CompactRational* cr) {
    cr->whole = 0;
    memset(cr->tuples, 0, sizeof(cr->tuples));
}

/**
 * APPROACH 2: Optimal Antichain Denominator Selection
 *
 * Given a rational n/d (already reduced), find the best way to represent
 * the fractional part using antichain denominators 128-255.
 *
 * Goal: Minimize number of tuples while staying within numerator limits.
 *
 * Strategy:
 * 1. Try single-denominator exact representation first
 * 2. If that fails, try two-denominator combinations
 * 3. Fall back to greedy approximation if needed
 */

typedef struct {
    uint8_t numerator;
    uint8_t denominator;  // Actual denominator (128-255)
} Tuple;

typedef struct {
    int tuple_count;
    Tuple tuples[MAX_TUPLES];
    double error;  // For approximations
} EncodingResult;

/**
 * Try to represent fraction (num/denom) using a single antichain denominator
 */
bool try_single_denominator(int64_t num, int64_t denom, Tuple* result) {
    // Try each antichain denominator
    for (int d = MIN_DENOMINATOR; d <= MAX_DENOMINATOR; d++) {
        // Check if we can represent exactly: num/denom = n/d
        // This requires: num * d = n * denom, and n <= 255

        if ((num * d) % denom == 0) {
            int64_t n = (num * d) / denom;
            if (n > 0 && n <= MAX_NUMERATOR) {
                result->numerator = (uint8_t)n;
                result->denominator = (uint8_t)d;
                return true;
            }
        }
    }
    return false;
}

/**
 * Find optimal two-denominator representation
 * Solve: num/denom = n1/d1 + n2/d2
 */
bool try_two_denominators(int64_t num, int64_t denom, Tuple result[2]) {
    // Try all pairs of antichain denominators
    for (int d1 = MIN_DENOMINATOR; d1 <= MAX_DENOMINATOR; d1++) {
        for (int d2 = d1; d2 <= MAX_DENOMINATOR; d2++) {
            // Solve for n1, n2:
            // num/denom = n1/d1 + n2/d2
            // num * d1 * d2 = n1 * denom * d2 + n2 * denom * d1

            // Use greedy: maximize n1, then solve for n2
            int64_t max_n1 = (num * d1) / denom;
            if (max_n1 > MAX_NUMERATOR) max_n1 = MAX_NUMERATOR;

            for (int64_t n1 = max_n1; n1 >= 0; n1--) {
                // Remaining: num/denom - n1/d1 = (num*d1 - n1*denom) / (denom*d1)
                int64_t rem_num = num * d1 - n1 * denom;
                int64_t rem_denom = denom * d1;

                // Reduce
                int64_t g = gcd(rem_num, rem_denom);
                rem_num /= g;
                rem_denom /= g;

                if (rem_num == 0) {
                    // Exact single-denominator solution
                    result[0].numerator = (uint8_t)n1;
                    result[0].denominator = (uint8_t)d1;
                    return true;
                }

                // Try to represent remainder with d2
                if ((rem_num * d2) % rem_denom == 0) {
                    int64_t n2 = (rem_num * d2) / rem_denom;
                    if (n2 > 0 && n2 <= MAX_NUMERATOR) {
                        result[0].numerator = (uint8_t)n1;
                        result[0].denominator = (uint8_t)d1;
                        result[1].numerator = (uint8_t)n2;
                        result[1].denominator = (uint8_t)d2;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

/**
 * Greedy approximation: find best single antichain denominator
 * that minimizes error
 */
Tuple approximate_with_single(int64_t num, int64_t denom) {
    Tuple best;
    best.numerator = 0;
    best.denominator = MIN_DENOMINATOR;
    double min_error = 1e9;

    double target = (double)num / (double)denom;

    for (int d = MIN_DENOMINATOR; d <= MAX_DENOMINATOR; d++) {
        // Find closest numerator
        int64_t n = (int64_t)round(target * d);
        if (n < 0) n = 0;
        if (n > MAX_NUMERATOR) n = MAX_NUMERATOR;

        double approx = (double)n / (double)d;
        double error = fabs(approx - target);

        if (error < min_error) {
            min_error = error;
            best.numerator = (uint8_t)n;
            best.denominator = (uint8_t)d;
        }
    }

    return best;
}

/**
 * Find optimal encoding for a rational number
 */
EncodingResult find_optimal_encoding(int64_t numerator, int64_t denominator) {
    EncodingResult result;
    result.tuple_count = 0;
    result.error = 0.0;

    // Reduce the fraction
    Rational r = {numerator, denominator};
    reduce_rational(&r);

    // Extract whole part
    int64_t whole = r.numerator / r.denominator;
    int64_t rem_num = r.numerator % r.denominator;

    // Handle negative remainders
    if (rem_num < 0) {
        rem_num += r.denominator;
        whole -= 1;
    }

    if (rem_num == 0) {
        // Pure integer, no tuples needed
        return result;
    }

    printf("    Optimizing: %lld/%lld (after extracting whole=%lld)\n",
           (long long)rem_num, (long long)r.denominator, (long long)whole);

    // Try single denominator exact match
    Tuple single;
    if (try_single_denominator(rem_num, r.denominator, &single)) {
        printf("    ✓ Single-denominator exact: %d/%d\n",
               single.numerator, single.denominator);
        result.tuples[0] = single;
        result.tuple_count = 1;
        result.error = 0.0;
        return result;
    }

    // Try two denominators
    Tuple pair[2];
    if (try_two_denominators(rem_num, r.denominator, pair)) {
        printf("    ✓ Two-denominator exact: %d/%d + %d/%d\n",
               pair[0].numerator, pair[0].denominator,
               pair[1].numerator, pair[1].denominator);
        result.tuples[0] = pair[0];
        result.tuples[1] = pair[1];
        result.tuple_count = 2;
        result.error = 0.0;
        return result;
    }

    // Fall back to greedy approximation
    printf("    ⚠ No exact representation found, using approximation\n");
    Tuple approx = approximate_with_single(rem_num, r.denominator);
    result.tuples[0] = approx;
    result.tuple_count = 1;

    double target = (double)rem_num / (double)r.denominator;
    double encoded = (double)approx.numerator / (double)approx.denominator;
    result.error = fabs(encoded - target);

    printf("    ≈ Approximation: %d/%d (error: %.9f)\n",
           approx.numerator, approx.denominator, result.error);

    return result;
}

// ============================================================================
// TEST CASES
// ============================================================================

void test_optimal_encoding() {
    printf("=== Optimal Antichain Encoding Tests ===\n\n");

    struct {
        int64_t num;
        int64_t denom;
        const char* name;
    } test_cases[] = {
        {1, 2, "1/2 (half)"},
        {1, 3, "1/3 (third)"},
        {1, 4, "1/4 (quarter)"},
        {1, 5, "1/5 (fifth)"},
        {1, 6, "1/6 (sixth)"},
        {1, 7, "1/7 (seventh)"},
        {1, 8, "1/8 (eighth)"},
        {2, 3, "2/3 (two thirds)"},
        {3, 4, "3/4 (three quarters)"},
        {5, 6, "5/6"},
        {7, 10, "7/10 (0.7)"},
        {22, 7, "22/7 (pi approximation)"},
        {355, 113, "355/113 (better pi)"},
        {1, 9, "1/9 (repeating decimal)"},
        {1, 11, "1/11"},
        {1, 100, "1/100 (one percent)"},
    };

    int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);

    for (int i = 0; i < num_tests; i++) {
        printf("Test %d: %s\n", i + 1, test_cases[i].name);
        EncodingResult result = find_optimal_encoding(
            test_cases[i].num, test_cases[i].denom);

        if (result.tuple_count == 0) {
            printf("    → Integer encoding (no tuples)\n");
        } else {
            printf("    → Uses %d tuple%s", result.tuple_count,
                   result.tuple_count > 1 ? "s" : "");
            if (result.error > 0) {
                printf(" (approximate, error: %.9f)", result.error);
            } else {
                printf(" (exact)");
            }
            printf("\n");
        }
        printf("\n");
    }

    printf("=== Key Observations ===\n");
    printf("1. Common fractions (1/2, 1/3, 1/4, ..., 1/8) have exact single-tuple encodings\n");
    printf("2. Fractions with denominators <= 127 can all be exactly represented (e.g., 1/9=15/135, 1/11=12/132)\n");
    printf("3. Fractions with large denominators (> 255, e.g., 1/256, 1/257) cannot be exactly represented with single tuples\n");
    printf("4. Two-tuple combinations can exactly represent many more fractions\n");
    printf("5. The antichain property (no denominator divides another in [128,255]) prevents simple redundancy\n");
}

int main() {
    test_optimal_encoding();
    return 0;
}
