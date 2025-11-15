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
#define DENOM_RANGE (MAX_DENOMINATOR - MIN_DENOMINATOR + 1)

// Compact Rational structure (same as main implementation)
typedef struct {
    int16_t whole;           // Bit 15: tuple flag, Bits 14-0: signed 15-bit integer
    uint16_t tuples[MAX_TUPLES];
} CompactRational;

typedef struct {
    int64_t numerator;
    int64_t denominator;
} Rational;

// ============================================================================
// UTILITY FUNCTIONS (copied from compact_rational.c for standalone compilation)
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

Rational cr_to_rational(const CompactRational* cr) {
    Rational result;
    int16_t whole_value = cr->whole & 0x7FFF;
    if (whole_value & 0x4000) {
        whole_value |= 0x8000;
    }
    result.numerator = whole_value;
    result.denominator = 1;

    if (!(cr->whole & 0x8000)) {
        return result;
    }

    for (int i = 0; i < MAX_TUPLES; i++) {
        uint8_t numerator = (cr->tuples[i] >> 8) & 0xFF;
        uint8_t denom_byte = cr->tuples[i] & 0xFF;
        uint8_t offset = denom_byte & 0x7F;
        uint8_t denominator = MIN_DENOMINATOR + offset;
        bool is_last = (denom_byte & 0x80) != 0;

        result.numerator = result.numerator * denominator + numerator * result.denominator;
        result.denominator *= denominator;
        reduce_rational(&result);

        if (is_last) break;
    }

    return result;
}

double cr_to_double(const CompactRational* cr) {
    Rational r = cr_to_rational(cr);
    if (r.denominator == 0) return 0.0;
    return (double)r.numerator / (double)r.denominator;
}

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

void cr_print_encoding(const CompactRational* cr) {
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
            if (end_flag) break;
            printf(", ");
        }
        printf("]");
    }
    printf("\n");
}

size_t cr_size(const CompactRational* cr) {
    size_t size = 2;
    if (!(cr->whole & 0x8000)) return size;

    for (int i = 0; i < MAX_TUPLES; i++) {
        size += 2;
        uint8_t denom_byte = cr->tuples[i] & 0xFF;
        bool is_last = (denom_byte & 0x80) != 0;
        if (is_last) break;
    }
    return size;
}

// ============================================================================
// CANONICALIZATION - APPROACH 1: GREEDY TUPLE MERGING
// ============================================================================

/**
 * Canonicalize a CompactRational to its minimal form.
 *
 * Properties of canonical form:
 * 1. No duplicate denominators (each antichain denominator appears at most once)
 * 2. All numerators satisfy: 0 < numerator < denominator
 * 3. Whole part is maximized (all complete integers extracted from tuples)
 * 4. Tuples are sorted by denominator (ascending order)
 * 5. Zero numerators are omitted
 *
 * Algorithm:
 * 1. Decode CompactRational to extract whole and fractional parts
 * 2. Accumulate all numerators by denominator (using array indexed by denom)
 * 3. Extract whole parts from numerators (when numerator >= denominator)
 * 4. Build new tuple list with only non-zero remainders
 * 5. Re-encode with proper end flags
 */
CompactRational cr_canonicalize(const CompactRational* cr) {
    CompactRational result;
    cr_init(&result);

    // Step 1: Extract whole part from input
    int16_t whole_value = cr->whole & 0x7FFF;  // Bits 14-0
    if (whole_value & 0x4000) {  // Sign-extend if negative
        whole_value |= 0x8000;
    }
    int32_t whole = whole_value;

    // Step 2: Accumulate numerators by denominator
    // Use array indexed by (denominator - MIN_DENOMINATOR)
    uint32_t numerators[DENOM_RANGE] = {0};

    // Check if tuples exist
    if (cr->whole & 0x8000) {
        // Process each tuple
        for (int i = 0; i < MAX_TUPLES; i++) {
            uint8_t num = (cr->tuples[i] >> 8) & 0xFF;
            uint8_t denom_byte = cr->tuples[i] & 0xFF;
            uint8_t offset = denom_byte & 0x7F;
            bool is_last = (denom_byte & 0x80) != 0;

            // Accumulate numerator for this denominator
            numerators[offset] += num;

            if (is_last) break;
        }
    }

    // Step 3: Extract whole parts from numerators and normalize
    for (int i = 0; i < DENOM_RANGE; i++) {
        if (numerators[i] > 0) {
            uint8_t denom = MIN_DENOMINATOR + i;
            // Extract complete integers
            whole += numerators[i] / denom;
            numerators[i] %= denom;
        }
    }

    // Step 4: Clamp whole to valid range
    if (whole > MAX_WHOLE_VALUE) {
        fprintf(stderr, "Warning: canonical whole part %d exceeds MAX_WHOLE_VALUE, clamping\n", whole);
        whole = MAX_WHOLE_VALUE;
    }
    if (whole < MIN_WHOLE_VALUE) {
        fprintf(stderr, "Warning: canonical whole part %d below MIN_WHOLE_VALUE, clamping\n", whole);
        whole = MIN_WHOLE_VALUE;
    }

    // Step 5: Build canonical tuple list
    // Only include non-zero numerators, sorted by denominator (implicit in array traversal)
    int tuple_count = 0;
    for (int i = 0; i < DENOM_RANGE && tuple_count < MAX_TUPLES; i++) {
        if (numerators[i] > 0) {
            tuple_count++;
        }
    }

    if (tuple_count == 0) {
        // Pure integer, no tuples needed
        result.whole = (int16_t)(whole & 0x7FFF);  // Bit 15 = 0
        return result;
    }

    // Set bit 15 = 1 (tuples present)
    result.whole = (int16_t)((whole & 0x7FFF) | 0x8000);

    // Encode tuples
    int tuple_idx = 0;
    for (int i = 0; i < DENOM_RANGE && tuple_idx < MAX_TUPLES; i++) {
        if (numerators[i] > 0) {
            uint8_t offset = i;
            uint8_t num = (uint8_t)numerators[i];

            // Check if this is the last tuple
            bool is_last = (tuple_idx == tuple_count - 1);
            uint8_t denom_byte = offset | (is_last ? 0x80 : 0x00);

            result.tuples[tuple_idx] = ((uint16_t)num << 8) | denom_byte;
            tuple_idx++;
        }
    }

    return result;
}

// ============================================================================
// HELPER: Create non-canonical CompactRationals for testing
// ============================================================================

/**
 * Create a CompactRational with duplicate denominators (non-canonical)
 */
CompactRational cr_create_with_duplicates() {
    CompactRational cr;
    cr_init(&cr);

    // Create: 0 + 64/128 + 32/128 (should canonicalize to 96/128)
    cr.whole = 0x8000;  // Bit 15 = 1, whole = 0

    // Tuple 0: 64/128 (offset=0, end=0)
    cr.tuples[0] = ((uint16_t)64 << 8) | 0x00;

    // Tuple 1: 32/128 (offset=0, end=1)
    cr.tuples[1] = ((uint16_t)32 << 8) | 0x80;

    return cr;
}

/**
 * Create a CompactRational with numerator overflow (non-canonical)
 */
CompactRational cr_create_with_overflow() {
    CompactRational cr;
    cr_init(&cr);

    // Create: 0 + 200/128 + 100/128 (should canonicalize to 2 + 44/128)
    cr.whole = 0x8000;  // Bit 15 = 1, whole = 0

    // Tuple 0: 200/128 (offset=0, end=0)
    cr.tuples[0] = ((uint16_t)200 << 8) | 0x00;

    // Tuple 1: 100/128 (offset=0, end=1)
    cr.tuples[1] = ((uint16_t)100 << 8) | 0x80;

    return cr;
}

/**
 * Create a CompactRational that should absorb into whole (non-canonical)
 */
CompactRational cr_create_absorbable() {
    CompactRational cr;
    cr_init(&cr);

    // Create: 5 + 128/128 (should canonicalize to 6)
    cr.whole = 0x8005;  // Bit 15 = 1, whole = 5

    // Tuple 0: 128/128 (offset=0, end=1)
    cr.tuples[0] = ((uint16_t)128 << 8) | 0x80;

    return cr;
}

/**
 * Create a complex sum: 1/2 + 1/3 + 1/6 = 1 (should canonicalize to pure integer)
 */
CompactRational cr_create_canceling_fractions() {
    CompactRational cr;
    cr_init(&cr);

    // 1/2 = 64/128, 1/3 = 43/129, 1/6 = 22/132
    // Sum = 0 + 64/128 + 43/129 + 22/132
    // In canonical form, this should reduce to 1 exactly

    cr.whole = 0x8000;  // Bit 15 = 1, whole = 0

    // Tuple 0: 64/128 (offset=0, end=0)
    cr.tuples[0] = ((uint16_t)64 << 8) | 0x00;

    // Tuple 1: 43/129 (offset=1, end=0)
    cr.tuples[1] = ((uint16_t)43 << 8) | 0x01;

    // Tuple 2: 22/132 (offset=4, end=1)
    cr.tuples[2] = ((uint16_t)22 << 8) | 0x84;

    return cr;
}

// ============================================================================
// TEST CASES
// ============================================================================

void test_canonicalization() {
    printf("=== CompactRational Canonicalization Tests ===\n\n");

    // Test 1: Duplicate denominators
    printf("Test 1: Duplicate denominators (64/128 + 32/128)\n");
    CompactRational cr1 = cr_create_with_duplicates();
    printf("  Before: ");
    cr_print(&cr1);
    printf(" [%zu bytes]\n  ", cr_size(&cr1));
    cr_print_encoding(&cr1);

    CompactRational canon1 = cr_canonicalize(&cr1);
    printf("  After:  ");
    cr_print(&canon1);
    printf(" [%zu bytes]\n  ", cr_size(&canon1));
    cr_print_encoding(&canon1);
    printf("  Expected: 96/128 = 0.75 in single tuple\n\n");

    // Test 2: Numerator overflow
    printf("Test 2: Numerator overflow (200/128 + 100/128)\n");
    CompactRational cr2 = cr_create_with_overflow();
    printf("  Before: ");
    cr_print(&cr2);
    printf(" [%zu bytes]\n  ", cr_size(&cr2));
    cr_print_encoding(&cr2);

    CompactRational canon2 = cr_canonicalize(&cr2);
    printf("  After:  ");
    cr_print(&canon2);
    printf(" [%zu bytes]\n  ", cr_size(&canon2));
    cr_print_encoding(&canon2);
    printf("  Expected: 2 + 44/128 = 2.34375\n\n");

    // Test 3: Absorbable tuple
    printf("Test 3: Absorbable tuple (5 + 128/128)\n");
    CompactRational cr3 = cr_create_absorbable();
    printf("  Before: ");
    cr_print(&cr3);
    printf(" [%zu bytes]\n  ", cr_size(&cr3));
    cr_print_encoding(&cr3);

    CompactRational canon3 = cr_canonicalize(&cr3);
    printf("  After:  ");
    cr_print(&canon3);
    printf(" [%zu bytes]\n  ", cr_size(&canon3));
    cr_print_encoding(&canon3);
    printf("  Expected: 6 (pure integer)\n\n");

    // Test 4: Canceling fractions
    printf("Test 4: Fractions that sum to integer (1/2 + 1/3 + 1/6)\n");
    CompactRational cr4 = cr_create_canceling_fractions();
    printf("  Before: ");
    cr_print(&cr4);
    printf(" [%zu bytes]\n  ", cr_size(&cr4));
    cr_print_encoding(&cr4);

    CompactRational canon4 = cr_canonicalize(&cr4);
    printf("  After:  ");
    cr_print(&canon4);
    printf(" [%zu bytes]\n  ", cr_size(&canon4));
    cr_print_encoding(&canon4);
    printf("  Expected: 1 (pure integer) - but may have small rounding\n\n");

    // Test 5: Already canonical
    printf("Test 5: Already canonical value\n");
    CompactRational cr5;
    cr_init(&cr5);
    cr5.whole = 0x8007;  // 7 with tuples
    cr5.tuples[0] = ((uint16_t)43 << 8) | 0x81;  // 43/129, end

    printf("  Before: ");
    cr_print(&cr5);
    printf(" [%zu bytes]\n  ", cr_size(&cr5));
    cr_print_encoding(&cr5);

    CompactRational canon5 = cr_canonicalize(&cr5);
    printf("  After:  ");
    cr_print(&canon5);
    printf(" [%zu bytes]\n  ", cr_size(&canon5));
    cr_print_encoding(&canon5);
    printf("  Expected: No change (7 + 43/129)\n\n");

    printf("=== All Tests Complete ===\n");
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    test_canonicalization();
    return 0;
}
