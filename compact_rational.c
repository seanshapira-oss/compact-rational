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

// Compact Rational structure
// NOTE: Bit 15 of 'whole' is the tuple presence flag (0 = integer only, 1 = tuples follow).
// Bits 14-0 of 'whole' represent a signed 15-bit integer (-16383 to +16383).
// The last tuple has bit 7 set in its denominator byte to indicate end of tuple sequence.
typedef struct {
    int16_t whole;           // Bit 15: tuple flag, Bits 14-0: signed 15-bit integer
    uint16_t tuples[MAX_TUPLES];  // Each: high byte = numerator, low byte = denom info
} CompactRational;

// Standard rational structure (for intermediate calculations)
typedef struct {
    int64_t numerator;
    int64_t denominator;
} Rational;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// GCD using Euclidean algorithm
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

// Reduce a rational to lowest terms
void reduce_rational(Rational* r) {
    if (r->denominator == 0) return;
    
    int64_t g = gcd(r->numerator, r->denominator);
    r->numerator /= g;
    r->denominator /= g;
    
    // Keep denominator positive
    if (r->denominator < 0) {
        r->numerator = -r->numerator;
        r->denominator = -r->denominator;
    }
}

// Find best antichain denominator for a given denominator
uint8_t find_antichain_denominator(int64_t denom) {
    // For denominators that divide into an antichain denominator nicely,
    // find the smallest one
    for (int d = MIN_DENOMINATOR; d <= MAX_DENOMINATOR; d++) {
        if (d % denom == 0) {
            return (uint8_t)d;
        }
    }
    // Otherwise, just use the denominator if it's in range
    if (denom >= MIN_DENOMINATOR && denom <= MAX_DENOMINATOR) {
        return (uint8_t)denom;
    }
    // Default to MIN_DENOMINATOR and scale
    return MIN_DENOMINATOR;
}

// ============================================================================
// COMPACT RATIONAL OPERATIONS
// ============================================================================

// Initialize a compact rational to zero
void cr_init(CompactRational* cr) {
    cr->whole = 0;  // Bit 15 = 0 (no tuples), value = 0
    memset(cr->tuples, 0, sizeof(cr->tuples));
}

// Create compact rational from integer
CompactRational cr_from_int(int32_t value) {
    CompactRational cr;
    cr_init(&cr);

    // Clamp to 15-bit signed range with warning
    if (value > MAX_WHOLE_VALUE) {
        fprintf(stderr, "Warning: value %d exceeds MAX_WHOLE_VALUE (%d), clamping to %d\n",
                value, MAX_WHOLE_VALUE, MAX_WHOLE_VALUE);
        value = MAX_WHOLE_VALUE;
    }
    if (value < MIN_WHOLE_VALUE) {
        fprintf(stderr, "Warning: value %d below MIN_WHOLE_VALUE (%d), clamping to %d\n",
                value, MIN_WHOLE_VALUE, MIN_WHOLE_VALUE);
        value = MIN_WHOLE_VALUE;
    }

    // Store as 15-bit signed value in bits 14-0, bit 15 = 0 (no tuples)
    cr.whole = (int16_t)(value & 0x7FFF);
    return cr;
}

// Create compact rational from numerator and denominator
CompactRational cr_from_fraction(int32_t num, int32_t denom) {
    CompactRational cr;
    cr_init(&cr);
    
    if (denom == 0) {
        fprintf(stderr, "Error: division by zero\n");
        return cr;
    }
    
    // Reduce the fraction
    Rational r = {num, denom};
    reduce_rational(&r);
    
    // Extract whole part
    int32_t whole = (int32_t)(r.numerator / r.denominator);
    int64_t remainder_num = r.numerator % r.denominator;
    
    // Handle negative remainders
    if (remainder_num < 0) {
        remainder_num += r.denominator;
        whole -= 1;
    }
    
    // Clamp whole part with warning
    if (whole > MAX_WHOLE_VALUE) {
        fprintf(stderr, "Warning: whole part %d exceeds MAX_WHOLE_VALUE (%d), clamping to %d\n",
                whole, MAX_WHOLE_VALUE, MAX_WHOLE_VALUE);
        whole = MAX_WHOLE_VALUE;
    }
    if (whole < MIN_WHOLE_VALUE) {
        fprintf(stderr, "Warning: whole part %d below MIN_WHOLE_VALUE (%d), clamping to %d\n",
                whole, MIN_WHOLE_VALUE, MIN_WHOLE_VALUE);
        whole = MIN_WHOLE_VALUE;
    }

    // If there's a fractional part, encode it
    if (remainder_num != 0) {
        // Find appropriate antichain denominator
        uint8_t antichain_denom = find_antichain_denominator(r.denominator);

        // Scale numerator to match antichain denominator
        uint64_t scaled_num = (remainder_num * antichain_denom) / r.denominator;

        if (scaled_num > 0 && scaled_num <= MAX_NUMERATOR) {
            // Encode whole part in bits 14-0, set bit 15 = 1 (tuples present)
            cr.whole = (int16_t)((whole & 0x7FFF) | 0x8000);

            uint8_t offset = antichain_denom - MIN_DENOMINATOR;
            uint8_t denom_byte = 0x80 | offset;  // Set end flag (bit 7 = 1)

            cr.tuples[0] = ((uint16_t)scaled_num << 8) | denom_byte;
        } else {
            // No valid fractional part, store as integer only
            cr.whole = (int16_t)(whole & 0x7FFF);  // Bit 15 = 0 (no tuples)
        }
    } else {
        // No fractional part, store as integer only
        cr.whole = (int16_t)(whole & 0x7FFF);  // Bit 15 = 0 (no tuples)
    }
    
    return cr;
}

// Decode compact rational to standard rational
Rational cr_to_rational(const CompactRational* cr) {
    Rational result;

    // Extract the 15-bit signed integer from bits 14-0
    int16_t whole_value = cr->whole & 0x7FFF;  // Mask to 15 bits

    // Sign-extend if bit 14 (sign bit of 15-bit value) is set
    if (whole_value & 0x4000) {  // If bit 14 is set (negative in 15-bit)
        whole_value |= 0x8000;   // Set bit 15 to sign-extend to 16-bit
    }

    result.numerator = whole_value;
    result.denominator = 1;

    // Check bit 15 for tuple presence flag
    if (!(cr->whole & 0x8000)) {
        // Bit 15 = 0, no tuples
        return result;
    }

    // Bit 15 = 1, tuples are present
    // Process tuples until we find one with end flag (bit 7 set in denominator byte)
    for (int i = 0; i < MAX_TUPLES; i++) {
        uint8_t numerator = (cr->tuples[i] >> 8) & 0xFF;
        uint8_t denom_byte = cr->tuples[i] & 0xFF;
        uint8_t offset = denom_byte & 0x7F;
        uint8_t denominator = MIN_DENOMINATOR + offset;
        bool is_last = (denom_byte & 0x80) != 0;

        // Add this fraction: result += numerator/denominator
        // result = (result.num * denom + numerator * result.denom) / (result.denom * denom)
        result.numerator = result.numerator * denominator + numerator * result.denominator;
        result.denominator *= denominator;

        reduce_rational(&result);

        // Stop if this is the last tuple
        if (is_last) {
            break;
        }
    }

    return result;
}

// Convert to double (for display/comparison)
double cr_to_double(const CompactRational* cr) {
    Rational r = cr_to_rational(cr);

    // Defensive check for zero denominator
    if (r.denominator == 0) {
        fprintf(stderr, "Error: division by zero in cr_to_double\n");
        return 0.0;
    }

    return (double)r.numerator / (double)r.denominator;
}

// Print compact rational in human-readable form
void cr_print(const CompactRational* cr) {
    Rational r = cr_to_rational(cr);
    
    if (r.denominator == 1) {
        printf("%lld", (long long)r.numerator);
    } else {
        int64_t whole = r.numerator / r.denominator;
        int64_t rem = llabs(r.numerator % r.denominator);
        
        if (whole != 0) {
            printf("%lld", (long long)whole);
            if (rem != 0) {
                printf(" %lld/%lld", (long long)rem, (long long)r.denominator);
            }
        } else {
            if (r.numerator < 0) printf("-");
            printf("%lld/%lld", (long long)rem, (long long)r.denominator);
        }
    }
    printf(" (%.6f)", cr_to_double(cr));
}

// Add two compact rationals
CompactRational cr_add(const CompactRational* a, const CompactRational* b) {
    // Convert both to standard rationals, add them, then encode back
    Rational ra = cr_to_rational(a);
    Rational rb = cr_to_rational(b);

    // Add: ra + rb = (ra.num * rb.denom + rb.num * ra.denom) / (ra.denom * rb.denom)
    Rational sum;
    sum.numerator = ra.numerator * rb.denominator + rb.numerator * ra.denominator;
    sum.denominator = ra.denominator * rb.denominator;

    reduce_rational(&sum);

    // Check for overflow before downcasting to int32_t
    if (sum.numerator > INT32_MAX || sum.numerator < INT32_MIN ||
        sum.denominator > INT32_MAX || sum.denominator < INT32_MIN) {
        fprintf(stderr, "Error: overflow in addition - result (%lld/%lld) exceeds int32_t range\n",
                (long long)sum.numerator, (long long)sum.denominator);
        return cr_from_int(0);  // Return zero on overflow
    }

    // Convert back to compact form
    return cr_from_fraction((int32_t)sum.numerator, (int32_t)sum.denominator);
}

// Print raw encoding (for debugging)
void cr_print_encoding(const CompactRational* cr) {
    // Cast to uint16_t to avoid sign extension artifacts in hex display
    bool has_tuples = (cr->whole & 0x8000) != 0;
    printf("Encoding: whole=0x%04X (bit15=%d)", (uint16_t)cr->whole, has_tuples ? 1 : 0);

    if (has_tuples) {
        printf(" [");
        for (int i = 0; i < MAX_TUPLES; i++) {
            uint8_t num = (cr->tuples[i] >> 8) & 0xFF;
            uint8_t denom_byte = cr->tuples[i] & 0xFF;
            uint8_t offset = denom_byte & 0x7F;
            bool end_flag = (denom_byte & 0x80) != 0;

            printf("%d/%d%s", num, MIN_DENOMINATOR + offset, end_flag ? "(end)" : "");

            if (end_flag) {
                break;  // Stop after the last tuple
            }
            printf(", ");
        }
        printf("]");
    }
    printf("\n");
}

// Get size in bytes
size_t cr_size(const CompactRational* cr) {
    // Start with 2 bytes for the whole field
    size_t size = 2;

    // Check if tuples are present (bit 15 of whole)
    if (!(cr->whole & 0x8000)) {
        // No tuples
        return size;
    }

    // Count tuples until we find the end flag
    for (int i = 0; i < MAX_TUPLES; i++) {
        size += 2;  // Each tuple is 2 bytes
        uint8_t denom_byte = cr->tuples[i] & 0xFF;
        bool is_last = (denom_byte & 0x80) != 0;
        if (is_last) {
            break;
        }
    }

    return size;
}

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
