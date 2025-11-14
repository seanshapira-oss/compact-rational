#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define MAX_TUPLES 5
#define MIN_DENOMINATOR 128
#define MAX_DENOMINATOR 255
#define MIN_WHOLE_VALUE -16384
#define MAX_WHOLE_VALUE 16383

// Error codes
typedef enum {
    CR_OK = 0,
    CR_OUT_OF_RANGE,
    CR_DIVISION_BY_ZERO,
    CR_OVERFLOW,
    CR_TOO_MANY_TUPLES
} CRError;

// Compact Rational structure
typedef struct {
    int16_t whole;           // Bit 15: tuple flag, bits 14-0: signed 15-bit integer
    uint16_t tuples[MAX_TUPLES];  // Each: high byte = numerator, low byte = denom info (bit 7 = end flag)
} CompactRational;

// Result type for operations that can fail
typedef struct {
    CompactRational value;
    CRError error;
} CRResult;

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

// Error message helper
const char* cr_error_string(CRError error) {
    switch (error) {
        case CR_OK: return "Success";
        case CR_OUT_OF_RANGE: return "Value out of representable range";
        case CR_DIVISION_BY_ZERO: return "Division by zero";
        case CR_OVERFLOW: return "Integer overflow";
        case CR_TOO_MANY_TUPLES: return "Too many tuple components";
        default: return "Unknown error";
    }
}

// Initialize a compact rational to zero
void cr_init(CompactRational* cr) {
    cr->whole = 0;
    memset(cr->tuples, 0, sizeof(cr->tuples));
}

// Create compact rational from integer
CRResult cr_from_int(int32_t value) {
    CRResult result;
    cr_init(&result.value);

    // Validate range for 15-bit signed integer
    if (value > MAX_WHOLE_VALUE || value < MIN_WHOLE_VALUE) {
        result.error = CR_OUT_OF_RANGE;
        fprintf(stderr, "Error: value %d out of range [%d, %d]\n",
                value, MIN_WHOLE_VALUE, MAX_WHOLE_VALUE);
        return result;
    }

    // Store as 15-bit signed value with bit 15 clear (no tuples)
    // For negative values, this automatically uses 15-bit two's complement
    result.value.whole = (int16_t)(value & 0x7FFF);
    if (value < 0 && value >= MIN_WHOLE_VALUE) {
        // For negative values, we need to preserve the 15-bit representation
        // The cast to int16_t and masking handles this correctly
        result.value.whole = (int16_t)value & 0x7FFF;
    }
    result.error = CR_OK;
    return result;
}

// Create compact rational from numerator and denominator
CRResult cr_from_fraction(int32_t num, int32_t denom) {
    CRResult result;
    cr_init(&result.value);

    if (denom == 0) {
        result.error = CR_DIVISION_BY_ZERO;
        fprintf(stderr, "Error: division by zero\n");
        return result;
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

    // Validate whole part range
    if (whole > MAX_WHOLE_VALUE || whole < MIN_WHOLE_VALUE) {
        result.error = CR_OUT_OF_RANGE;
        fprintf(stderr, "Error: whole part %d out of range [%d, %d]\n",
                whole, MIN_WHOLE_VALUE, MAX_WHOLE_VALUE);
        return result;
    }

    // Store whole part as 15-bit signed value
    result.value.whole = (int16_t)whole & 0x7FFF;

    // If there's a fractional part, encode it
    if (remainder_num != 0) {
        // Find appropriate antichain denominator
        uint8_t antichain_denom = find_antichain_denominator(r.denominator);

        // Scale numerator to match antichain denominator
        uint64_t scaled_num = (remainder_num * antichain_denom) / r.denominator;

        if (scaled_num > 0 && scaled_num <= 255) {
            result.value.whole |= 0x8000;  // Set tuple presence flag (bit 15)

            uint8_t offset = antichain_denom - MIN_DENOMINATOR;
            uint8_t denom_byte = 0x80 | offset;  // Set end flag (bit 7)

            result.value.tuples[0] = ((uint16_t)scaled_num << 8) | denom_byte;
        }
    }

    result.error = CR_OK;
    return result;
}

// Decode compact rational to standard rational
Rational cr_to_rational(const CompactRational* cr) {
    Rational result;

    // Extract whole part (bits 14-0, sign-extended to int16_t)
    int16_t whole_value = cr->whole & 0x7FFF;  // Get 15-bit value
    if (cr->whole & 0x4000) {  // If bit 14 (sign bit of 15-bit value) is set
        whole_value |= 0x8000;  // Sign extend to 16-bit by setting bit 15
    }

    result.numerator = whole_value;
    result.denominator = 1;

    // Check bit 15 for tuple presence flag
    if (!(cr->whole & 0x8000)) {
        return result;  // No tuples, return the integer
    }

    // Process tuples until we find the end flag
    for (int i = 0; i < MAX_TUPLES; i++) {
        uint8_t numerator = (cr->tuples[i] >> 8) & 0xFF;
        uint8_t denom_byte = cr->tuples[i] & 0xFF;
        uint8_t offset = denom_byte & 0x7F;  // Bits 6-0
        bool end_flag = (denom_byte & 0x80) != 0;  // Bit 7
        uint8_t denominator = MIN_DENOMINATOR + offset;

        // Add this fraction: result += numerator/denominator
        // result = (result.num * denom + numerator * result.denom) / (result.denom * denom)
        result.numerator = result.numerator * denominator + numerator * result.denominator;
        result.denominator *= denominator;

        reduce_rational(&result);

        // Stop if this is the last tuple
        if (end_flag) {
            break;
        }
    }

    return result;
}

// Convert to double (for display/comparison)
double cr_to_double(const CompactRational* cr) {
    Rational r = cr_to_rational(cr);
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
CRResult cr_add(const CompactRational* a, const CompactRational* b) {
    CRResult result;
    cr_init(&result.value);

    // Convert both to standard rationals, add them, then encode back
    Rational ra = cr_to_rational(a);
    Rational rb = cr_to_rational(b);

    // Add: ra + rb = (ra.num * rb.denom + rb.num * ra.denom) / (ra.denom * rb.denom)
    Rational sum;
    sum.numerator = ra.numerator * rb.denominator + rb.numerator * ra.denominator;
    sum.denominator = ra.denominator * rb.denominator;

    reduce_rational(&sum);

    // Check for overflow before casting to int32_t
    if (sum.numerator > INT32_MAX || sum.numerator < INT32_MIN ||
        sum.denominator > INT32_MAX || sum.denominator < INT32_MIN) {
        result.error = CR_OVERFLOW;
        fprintf(stderr, "Error: overflow in addition (intermediate result too large)\n");
        return result;
    }

    // Convert back to compact form
    return cr_from_fraction((int32_t)sum.numerator, (int32_t)sum.denominator);
}

// Count number of tuples by scanning for end flag
static int cr_count_tuples(const CompactRational* cr) {
    if (!(cr->whole & 0x8000)) {
        return 0;  // No tuples
    }

    for (int i = 0; i < MAX_TUPLES; i++) {
        uint8_t denom_byte = cr->tuples[i] & 0xFF;
        bool end_flag = (denom_byte & 0x80) != 0;
        if (end_flag) {
            return i + 1;  // Return count including this tuple
        }
    }
    return MAX_TUPLES;  // All tuples used (shouldn't happen with proper encoding)
}

// Print raw encoding (for debugging)
void cr_print_encoding(const CompactRational* cr) {
    int num_tuples = cr_count_tuples(cr);
    printf("Encoding: whole=0x%04hX (%d tuples)", (uint16_t)cr->whole, num_tuples);

    if (cr->whole & 0x8000) {
        printf(" [");
        for (int i = 0; i < MAX_TUPLES; i++) {
            uint8_t num = (cr->tuples[i] >> 8) & 0xFF;
            uint8_t denom_byte = cr->tuples[i] & 0xFF;
            uint8_t offset = denom_byte & 0x7F;
            bool end_flag = (denom_byte & 0x80) != 0;

            printf("%d/%d%s", num, MIN_DENOMINATOR + offset, end_flag ? "(end)" : "");

            if (end_flag) {
                break;  // Stop at end flag
            }
            if (i < MAX_TUPLES - 1) {
                printf(", ");
            }
        }
        printf("]");
    }
    printf("\n");
}

// Get size in bytes
size_t cr_size(const CompactRational* cr) {
    int num_tuples = cr_count_tuples(cr);
    return 2 + (num_tuples * 2);  // 2 bytes for whole, 2 bytes per tuple
}

// ============================================================================
// TEST CASES
// ============================================================================

void run_tests() {
    printf("=== Compact Rational Number Tests ===\n\n");

    // Test 1: Pure integers
    printf("Test 1: Pure integers\n");
    CRResult res1 = cr_from_int(42);
    if (res1.error == CR_OK) {
        printf("  42 -> ");
        cr_print(&res1.value);
        printf(" [%zu bytes]\n", cr_size(&res1.value));
        cr_print_encoding(&res1.value);
    }
    printf("\n");

    // Test 2: Simple fractions
    printf("Test 2: Simple fractions\n");
    CRResult res2 = cr_from_fraction(22, 3);  // 7 1/3
    if (res2.error == CR_OK) {
        printf("  22/3 -> ");
        cr_print(&res2.value);
        printf(" [%zu bytes]\n", cr_size(&res2.value));
        cr_print_encoding(&res2.value);
    }
    printf("\n");

    // Test 3: Common fractions
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
        CRResult res = cr_from_fraction(fractions[i].num, fractions[i].denom);
        if (res.error == CR_OK) {
            printf("  %s -> ", fractions[i].name);
            cr_print(&res.value);
            printf(" [%zu bytes]\n", cr_size(&res.value));
        }
    }
    printf("\n");

    // Test 4: Addition (score summing)
    printf("Test 4: Addition (summing scores)\n");
    CRResult score1_res = cr_from_fraction(15, 2);  // 7.5
    CRResult score2_res = cr_from_fraction(25, 3);  // 8.333...
    CRResult score3_res = cr_from_fraction(41, 5);  // 8.2

    if (score1_res.error == CR_OK) {
        printf("  Score 1: ");
        cr_print(&score1_res.value);
        printf("\n");
    }
    if (score2_res.error == CR_OK) {
        printf("  Score 2: ");
        cr_print(&score2_res.value);
        printf("\n");
    }
    if (score3_res.error == CR_OK) {
        printf("  Score 3: ");
        cr_print(&score3_res.value);
        printf("\n");
    }

    if (score1_res.error == CR_OK && score2_res.error == CR_OK && score3_res.error == CR_OK) {
        CRResult sum_res = cr_add(&score1_res.value, &score2_res.value);
        if (sum_res.error == CR_OK) {
            sum_res = cr_add(&sum_res.value, &score3_res.value);
            if (sum_res.error == CR_OK) {
                printf("  Total: ");
                cr_print(&sum_res.value);
                printf(" [%zu bytes]\n", cr_size(&sum_res.value));
                cr_print_encoding(&sum_res.value);
            }
        }
    }
    printf("\n");

    // Test 5: Realistic grading scenario
    printf("Test 5: Realistic grading scenario\n");
    CRResult grades_res[] = {
        cr_from_fraction(17, 2),   // 8.5
        cr_from_fraction(28, 3),   // 9.333...
        cr_from_fraction(19, 2),   // 9.5
        cr_from_int(10),           // 10
        cr_from_fraction(31, 3),   // 10.333...
    };

    CRResult total_res = cr_from_int(0);
    for (int i = 0; i < 5; i++) {
        if (grades_res[i].error == CR_OK) {
            printf("  Assignment %d: ", i + 1);
            cr_print(&grades_res[i].value);
            printf(" [%zu bytes]\n", cr_size(&grades_res[i].value));
            if (total_res.error == CR_OK) {
                total_res = cr_add(&total_res.value, &grades_res[i].value);
            }
        }
    }

    if (total_res.error == CR_OK) {
        printf("  Final Grade: ");
        cr_print(&total_res.value);
        printf(" [%zu bytes]\n", cr_size(&total_res.value));
        cr_print_encoding(&total_res.value);
    }
    printf("\n");

    // Test 6: Negative numbers
    printf("Test 6: Negative numbers\n");
    CRResult neg1_res = cr_from_fraction(-7, 2);
    if (neg1_res.error == CR_OK) {
        printf("  -7/2 -> ");
        cr_print(&neg1_res.value);
        printf(" [%zu bytes]\n", cr_size(&neg1_res.value));
        cr_print_encoding(&neg1_res.value);
    }
    printf("\n");

    // Test 7: Edge cases
    printf("Test 7: Edge cases\n");
    CRResult max_val_res = cr_from_int(16383);
    if (max_val_res.error == CR_OK) {
        printf("  Max whole (16383): ");
        cr_print(&max_val_res.value);
        printf("\n");
    }

    CRResult min_val_res = cr_from_int(-16383);
    if (min_val_res.error == CR_OK) {
        printf("  Min whole (-16383): ");
        cr_print(&min_val_res.value);
        printf("\n");
    }

    // Test 8: Out of range errors
    printf("\nTest 8: Error handling\n");
    CRResult too_large = cr_from_int(20000);
    printf("  cr_from_int(20000): %s\n", cr_error_string(too_large.error));

    CRResult too_small = cr_from_int(-20000);
    printf("  cr_from_int(-20000): %s\n", cr_error_string(too_small.error));

    CRResult div_zero = cr_from_fraction(1, 0);
    printf("  cr_from_fraction(1, 0): %s\n", cr_error_string(div_zero.error));

    printf("\n=== All Tests Complete ===\n");
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    run_tests();
    return 0;
}
