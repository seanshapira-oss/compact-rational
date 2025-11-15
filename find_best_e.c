#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define MIN_DENOMINATOR 128
#define MAX_DENOMINATOR 255
#define MAX_NUMERATOR 255
#define MAX_TUPLES 5

// The mathematical constant e
#define E 2.718281828459045235360287471352662497757

// Result structure to track the best approximation
typedef struct {
    int whole;
    int num_tuples;
    struct {
        uint8_t numerator;
        uint8_t denominator;
    } tuples[MAX_TUPLES];
    double value;
    double error;
} Approximation;

// Compute the value of an approximation
double compute_value(const Approximation* approx) {
    double value = approx->whole;
    for (int i = 0; i < approx->num_tuples; i++) {
        value += (double)approx->tuples[i].numerator / (double)approx->tuples[i].denominator;
    }
    return value;
}

// Find best single-tuple approximation
Approximation find_best_single_tuple() {
    Approximation best;
    best.whole = 2;
    best.num_tuples = 1;
    best.error = INFINITY;

    // Try all valid denominators and numerators
    for (int d = MIN_DENOMINATOR; d <= MAX_DENOMINATOR; d++) {
        for (int n = 1; n < d && n <= MAX_NUMERATOR; n++) {
            double value = 2.0 + (double)n / (double)d;
            double error = fabs(value - E);

            if (error < best.error) {
                best.error = error;
                best.value = value;
                best.tuples[0].numerator = n;
                best.tuples[0].denominator = d;
            }
        }
    }

    return best;
}

// Find best two-tuple approximation
Approximation find_best_two_tuple() {
    Approximation best;
    best.whole = 2;
    best.num_tuples = 2;
    best.error = INFINITY;

    // Target for the fractional part
    double target = E - 2.0;  // 0.71828...

    // Try all valid combinations of two tuples
    for (int d1 = MIN_DENOMINATOR; d1 <= MAX_DENOMINATOR; d1++) {
        for (int n1 = 1; n1 < d1 && n1 <= MAX_NUMERATOR; n1++) {
            double frac1 = (double)n1 / (double)d1;
            if (frac1 >= target) continue;  // First fraction shouldn't exceed target

            double remaining = target - frac1;

            // Try second tuple (must be different denominator for canonical form)
            for (int d2 = MIN_DENOMINATOR; d2 <= MAX_DENOMINATOR; d2++) {
                if (d2 == d1) continue;  // Skip duplicate denominators

                for (int n2 = 1; n2 < d2 && n2 <= MAX_NUMERATOR; n2++) {
                    double frac2 = (double)n2 / (double)d2;
                    double total = frac1 + frac2;
                    double error = fabs(total - target);

                    if (error < best.error) {
                        best.error = error;
                        best.value = 2.0 + total;
                        best.tuples[0].numerator = n1;
                        best.tuples[0].denominator = d1;
                        best.tuples[1].numerator = n2;
                        best.tuples[1].denominator = d2;
                    }
                }
            }
        }
    }

    return best;
}

// Greedy algorithm: find best approximation by adding one fraction at a time
Approximation find_best_greedy(int max_tuples) {
    Approximation approx;
    approx.whole = 2;
    approx.num_tuples = 0;

    double remaining = E - 2.0;
    bool used_denoms[MAX_DENOMINATOR - MIN_DENOMINATOR + 1] = {false};

    for (int tuple_idx = 0; tuple_idx < max_tuples && remaining > 1e-10; tuple_idx++) {
        double best_frac = 0;
        int best_n = 0, best_d = 0;
        double best_error = INFINITY;

        // Find the fraction that gets us closest to remaining
        for (int d = MIN_DENOMINATOR; d <= MAX_DENOMINATOR; d++) {
            int offset = d - MIN_DENOMINATOR;
            if (used_denoms[offset]) continue;  // Skip already used denominators

            for (int n = 1; n < d && n <= MAX_NUMERATOR; n++) {
                double frac = (double)n / (double)d;
                if (frac > remaining) break;  // Don't overshoot

                double error = fabs(remaining - frac);
                if (error < best_error) {
                    best_error = error;
                    best_frac = frac;
                    best_n = n;
                    best_d = d;
                }
            }
        }

        if (best_n == 0) break;  // No suitable fraction found

        approx.tuples[tuple_idx].numerator = best_n;
        approx.tuples[tuple_idx].denominator = best_d;
        approx.num_tuples++;
        used_denoms[best_d - MIN_DENOMINATOR] = true;
        remaining -= best_frac;
    }

    approx.value = compute_value(&approx);
    approx.error = fabs(approx.value - E);

    return approx;
}

void print_approximation(const char* name, const Approximation* approx) {
    printf("%s:\n", name);
    printf("  Value: %.15f\n", approx->value);
    printf("  Error: %.15e\n", approx->error);
    printf("  Representation: %d", approx->whole);

    for (int i = 0; i < approx->num_tuples; i++) {
        printf(" + %d/%d", approx->tuples[i].numerator, approx->tuples[i].denominator);
    }
    printf("\n");

    // Show as CompactRational encoding
    printf("  CompactRational encoding:\n");
    printf("    whole = 0x%04X (bit15=%d, value=%d)\n",
           approx->num_tuples > 0 ? (0x8000 | approx->whole) : approx->whole,
           approx->num_tuples > 0 ? 1 : 0,
           approx->whole);

    for (int i = 0; i < approx->num_tuples; i++) {
        uint8_t offset = approx->tuples[i].denominator - MIN_DENOMINATOR;
        bool is_last = (i == approx->num_tuples - 1);
        uint8_t denom_byte = offset | (is_last ? 0x80 : 0x00);
        uint16_t tuple = ((uint16_t)approx->tuples[i].numerator << 8) | denom_byte;

        printf("    tuple[%d] = 0x%04X (%d/%d%s)\n",
               i, tuple,
               approx->tuples[i].numerator,
               approx->tuples[i].denominator,
               is_last ? ", end" : "");
    }
    printf("\n");
}

int main() {
    printf("Finding optimal CompactRational representation of e = %.15f\n\n", E);

    // Find best single-tuple approximation
    printf("=== Searching single-tuple approximations ===\n");
    Approximation single = find_best_single_tuple();
    print_approximation("Best single-tuple", &single);

    // Find best two-tuple approximation
    printf("=== Searching two-tuple approximations ===\n");
    Approximation two = find_best_two_tuple();
    print_approximation("Best two-tuple", &two);

    // Find best greedy approximations for 3, 4, 5 tuples
    for (int n = 3; n <= 5; n++) {
        printf("=== Greedy %d-tuple approximation ===\n", n);
        Approximation greedy = find_best_greedy(n);
        print_approximation("Greedy approach", &greedy);
    }

    printf("=== Recommendation ===\n");
    printf("For the best balance of accuracy and size, the single-tuple representation\n");
    printf("is recommended: 2 + %d/%d\n", single.tuples[0].numerator, single.tuples[0].denominator);
    printf("Error: %.2e (about %.4f%%)\n", single.error, (single.error / E) * 100);

    return 0;
}
