#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <inttypes.h>

#define E 2.718281828459045235360287471352662497757

// GCD using Euclidean algorithm
int64_t gcd64(int64_t a, int64_t b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b != 0) {
        int64_t temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

// Find continued fraction convergents of e
void find_e_convergents() {
    printf("=== Continued Fraction Convergents of e ===\n\n");

    // The continued fraction of e is [2; 1, 2, 1, 1, 4, 1, 1, 6, 1, 1, 8, ...]
    // Pattern: [2; 1, 2k, 1] for k = 1, 2, 3, ...
    int cf[] = {2, 1, 2, 1, 1, 4, 1, 1, 6, 1, 1, 8, 1, 1, 10, 1, 1, 12, 1, 1, 14, 1, 1, 16};
    int num_terms = sizeof(cf) / sizeof(cf[0]);

    int64_t h_prev = 1, h_curr = cf[0];
    int64_t k_prev = 0, k_curr = 1;

    printf("Convergent %2d: %lld/%lld = %.20f (error: %.2e)\n",
           0, h_curr, k_curr, (double)h_curr / k_curr,
           fabs((double)h_curr / k_curr - E));

    for (int i = 1; i < num_terms; i++) {
        int64_t h_next = cf[i] * h_curr + h_prev;
        int64_t k_next = cf[i] * k_curr + k_prev;

        double value = (double)h_next / k_next;
        double error = fabs(value - E);

        printf("Convergent %2d: %lld/%lld\n", i, h_next, k_next);
        printf("              = %.20f\n", value);
        printf("              error: %.2e (%.10f%%)\n", error, (error/E)*100);

        // Check if denominator fits in int32_t
        if (k_next <= INT32_MAX) {
            printf("              ✓ Fits in CompactRational (denom ≤ %d)\n", INT32_MAX);
        } else {
            printf("              ✗ Too large for CompactRational (denom > %d)\n", INT32_MAX);
        }
        printf("\n");

        h_prev = h_curr;
        h_curr = h_next;
        k_prev = k_curr;
        k_curr = k_next;
    }
}

// Check the provided rational
void check_provided_rational() {
    printf("=== Analyzing Provided Rational ===\n\n");

    int64_t num = 1484783350961841221LL;
    int64_t den = 546197992715055416LL;

    printf("Numerator:   %" PRId64 "\n", num);
    printf("Denominator: %" PRId64 "\n", den);

    // Compute GCD
    int64_t g = gcd64(num, den);
    printf("\nGCD: %" PRId64 "\n", g);

    if (g > 1) {
        printf("Reduced numerator:   %" PRId64 "\n", num / g);
        printf("Reduced denominator: %" PRId64 "\n", den / g);
    } else {
        printf("Already in lowest terms.\n");
    }

    double value = (double)num / (double)den;
    double error = fabs(value - E);

    printf("\nValue: %.20f\n", value);
    printf("Error: %.2e\n", error);
    printf("Relative error: %.10f%%\n", (error/E)*100);

    // Check if it could be related to a convergent
    printf("\nChecking if this is a known convergent...\n");

    // This might be close to convergent p_19/q_19 or similar
    // Let me calculate a few more convergents to see
}

int main() {
    find_e_convergents();
    printf("\n");
    check_provided_rational();

    return 0;
}
