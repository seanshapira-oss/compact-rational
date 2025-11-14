#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define MAX_TUPLES 5
#define MIN_DENOMINATOR 128
#define MAX_DENOMINATOR 255

// Compact Rational structure
typedef struct {
    int16_t whole;           // Bit 15: tuple flag, bits 14-0: signed integer
    uint8_t num_tuples;      // Number of tuples present (0 to MAX_TUPLES)
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
    cr->whole = 0;
    cr->num_tuples = 0;
    memset(cr->tuples, 0, sizeof(cr->tuples));
}

// Create compact rational from integer
CompactRational cr_from_int(int32_t value) {
    CompactRational cr;
    cr_init(&cr);
    
    // Clamp to 15-bit signed range
    if (value > 16383) value = 16383;
    if (value < -16383) value = -16383;
    
    cr.whole = (int16_t)value;  // Bit 15 is 0 (no tuples)
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
    
    // Clamp whole part
    if (whole > 16383) whole = 16383;
    if (whole < -16383) whole = -16383;
    
    cr.whole = (int16_t)whole;
    
    // If there's a fractional part, encode it
    if (remainder_num != 0) {
        // Find appropriate antichain denominator
        uint8_t antichain_denom = find_antichain_denominator(r.denominator);
        
        // Scale numerator to match antichain denominator
        uint64_t scaled_num = (remainder_num * antichain_denom) / r.denominator;
        
        if (scaled_num > 0 && scaled_num <= 255) {
            cr.whole |= 0x8000;  // Set tuple presence flag
            cr.num_tuples = 1;
            
            uint8_t offset = antichain_denom - MIN_DENOMINATOR;
            uint8_t denom_byte = 0x80 | offset;  // Set end flag
            
            cr.tuples[0] = ((uint16_t)scaled_num << 8) | denom_byte;
        }
    }
    
    return cr;
}

// Decode compact rational to standard rational
Rational cr_to_rational(const CompactRational* cr) {
    Rational result;
    
    // Extract whole part (bits 14-0, sign-extended)
    int16_t whole_value = cr->whole & 0x7FFF;
    if (cr->whole & 0x4000) {  // Sign bit of 15-bit value
        whole_value |= 0x8000;  // Sign extend
    }
    
    result.numerator = whole_value;
    result.denominator = 1;
    
    // If no tuples, return the integer
    if (!(cr->whole & 0x8000)) {
        return result;
    }
    
    // Process tuples and accumulate fractions
    for (int i = 0; i < cr->num_tuples && i < MAX_TUPLES; i++) {
        uint8_t numerator = (cr->tuples[i] >> 8) & 0xFF;
        uint8_t denom_byte = cr->tuples[i] & 0xFF;
        uint8_t offset = denom_byte & 0x7F;
        uint8_t denominator = MIN_DENOMINATOR + offset;
        
        // Add this fraction: result += numerator/denominator
        // result = (result.num * denom + numerator * result.denom) / (result.denom * denom)
        result.numerator = result.numerator * denominator + numerator * result.denominator;
        result.denominator *= denominator;
        
        reduce_rational(&result);
    }
    
    return result;
}

// Convert to double (for display/comparison)
double cr_to_double(const CompactRational* cr) {
    Rational r = cr_to_rational(cr);
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
    
    // Convert back to compact form
    return cr_from_fraction((int32_t)sum.numerator, (int32_t)sum.denominator);
}

// Print raw encoding (for debugging)
void cr_print_encoding(const CompactRational* cr) {
    printf("Encoding: whole=0x%04X (%d tuples)", cr->whole, cr->num_tuples);
    
    if (cr->whole & 0x8000) {
        printf(" [");
        for (int i = 0; i < cr->num_tuples && i < MAX_TUPLES; i++) {
            uint8_t num = (cr->tuples[i] >> 8) & 0xFF;
            uint8_t denom_byte = cr->tuples[i] & 0xFF;
            uint8_t offset = denom_byte & 0x7F;
            bool end_flag = (denom_byte & 0x80) != 0;
            
            printf("%d/%d%s", num, MIN_DENOMINATOR + offset, end_flag ? "(end)" : "");
            if (i < cr->num_tuples - 1) printf(", ");
        }
        printf("]");
    }
    printf("\n");
}

// Get size in bytes
size_t cr_size(const CompactRational* cr) {
    return 2 + (cr->num_tuples * 2);  // 2 bytes for whole, 2 bytes per tuple
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
    
    printf("\n=== All Tests Complete ===\n");
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    run_tests();
    return 0;
}
