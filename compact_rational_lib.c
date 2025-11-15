#include "compact_rational.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
    // Extract whole parts
    int16_t whole_a = a->whole & 0x7FFF;
    if (whole_a & 0x4000) whole_a |= 0x8000;  // Sign extend

    int16_t whole_b = b->whole & 0x7FFF;
    if (whole_b & 0x4000) whole_b |= 0x8000;  // Sign extend

    bool has_tuples_a = (a->whole & 0x8000) != 0;
    bool has_tuples_b = (b->whole & 0x8000) != 0;

    // Count tuples in each input
    int tuple_count_a = 0, tuple_count_b = 0;

    if (has_tuples_a) {
        for (int i = 0; i < MAX_TUPLES; i++) {
            tuple_count_a++;
            uint8_t denom_byte = a->tuples[i] & 0xFF;
            if (denom_byte & 0x80) break;  // End flag in denominator byte
        }
    }

    if (has_tuples_b) {
        for (int i = 0; i < MAX_TUPLES; i++) {
            tuple_count_b++;
            uint8_t denom_byte = b->tuples[i] & 0xFF;
            if (denom_byte & 0x80) break;  // End flag in denominator byte
        }
    }

    // OPTIMIZATION 1: Both are single-tuple
    if (tuple_count_a == 1 && tuple_count_b == 1) {
        uint8_t na = (a->tuples[0] >> 8) & 0xFF;
        uint8_t da_offset = a->tuples[0] & 0x7F;
        uint8_t da = MIN_DENOMINATOR + da_offset;

        uint8_t nb = (b->tuples[0] >> 8) & 0xFF;
        uint8_t db_offset = b->tuples[0] & 0x7F;
        uint8_t db = MIN_DENOMINATOR + db_offset;

        CompactRational result;
        cr_init(&result);

        // Case 1a: Different denominators - combine into two tuples (EXACT!)
        if (da != db) {
            int32_t new_whole = whole_a + whole_b;

            // Clamp
            if (new_whole > MAX_WHOLE_VALUE) new_whole = MAX_WHOLE_VALUE;
            if (new_whole < MIN_WHOLE_VALUE) new_whole = MIN_WHOLE_VALUE;

            result.whole = (int16_t)((new_whole & 0x7FFF) | 0x8000);
            result.tuples[0] = ((uint16_t)na << 8) | da_offset;  // No end flag
            result.tuples[1] = ((uint16_t)nb << 8) | (0x80 | db_offset);  // End flag

            return result;
        }

        // Case 1b: Same denominator - add numerators
        else {
            int32_t sum_numerator = na + nb;
            int32_t new_whole = whole_a + whole_b;

            // Extract overflow from numerator to whole
            new_whole += sum_numerator / da;
            sum_numerator %= da;

            // Clamp
            if (new_whole > MAX_WHOLE_VALUE) new_whole = MAX_WHOLE_VALUE;
            if (new_whole < MIN_WHOLE_VALUE) new_whole = MIN_WHOLE_VALUE;

            if (sum_numerator == 0) {
                // Pure integer result
                result.whole = (int16_t)(new_whole & 0x7FFF);
            } else {
                // Single tuple result
                result.whole = (int16_t)((new_whole & 0x7FFF) | 0x8000);
                result.tuples[0] = ((uint16_t)sum_numerator << 8) | (0x80 | da_offset);
            }

            return result;
        }
    }

    // OPTIMIZATION 2: One integer, one single-tuple
    if (tuple_count_a == 0 && tuple_count_b == 1) {
        int32_t new_whole = whole_a + whole_b;
        if (new_whole > MAX_WHOLE_VALUE) new_whole = MAX_WHOLE_VALUE;
        if (new_whole < MIN_WHOLE_VALUE) new_whole = MIN_WHOLE_VALUE;

        CompactRational result;
        cr_init(&result);
        result.whole = (int16_t)((new_whole & 0x7FFF) | 0x8000);
        result.tuples[0] = b->tuples[0];  // Copy tuple from b
        return result;
    }

    if (tuple_count_a == 1 && tuple_count_b == 0) {
        int32_t new_whole = whole_a + whole_b;
        if (new_whole > MAX_WHOLE_VALUE) new_whole = MAX_WHOLE_VALUE;
        if (new_whole < MIN_WHOLE_VALUE) new_whole = MIN_WHOLE_VALUE;

        CompactRational result;
        cr_init(&result);
        result.whole = (int16_t)((new_whole & 0x7FFF) | 0x8000);
        result.tuples[0] = a->tuples[0];  // Copy tuple from a
        return result;
    }

    // OPTIMIZATION 3: Both integers
    if (tuple_count_a == 0 && tuple_count_b == 0) {
        int32_t new_whole = whole_a + whole_b;
        if (new_whole > MAX_WHOLE_VALUE) new_whole = MAX_WHOLE_VALUE;
        if (new_whole < MIN_WHOLE_VALUE) new_whole = MIN_WHOLE_VALUE;

        CompactRational result;
        cr_init(&result);
        result.whole = (int16_t)(new_whole & 0x7FFF);
        return result;
    }

    // FALLBACK: Use rational conversion for complex cases (multi-tuple inputs)
    Rational ra = cr_to_rational(a);
    Rational rb = cr_to_rational(b);

    Rational sum;
    sum.numerator = ra.numerator * rb.denominator + rb.numerator * ra.denominator;
    sum.denominator = ra.denominator * rb.denominator;

    reduce_rational(&sum);

    if (sum.numerator > INT32_MAX || sum.numerator < INT32_MIN ||
        sum.denominator > INT32_MAX || sum.denominator < INT32_MIN) {
        fprintf(stderr, "Error: overflow in addition - result (%lld/%lld) exceeds int32_t range\n",
                (long long)sum.numerator, (long long)sum.denominator);
        return cr_from_int(0);
    }

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
