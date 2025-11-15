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

#define E 2.718281828459045235360287471352662497757

// Compact Rational structure
typedef struct {
    int16_t whole;
    uint16_t tuples[MAX_TUPLES];
} CompactRational;

typedef struct {
    int64_t numerator;
    int64_t denominator;
} Rational;

// Utility functions
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

// Create e representation
CompactRational create_e_single_tuple() {
    CompactRational cr;
    cr_init(&cr);

    // Best single-tuple: 2 + 181/252
    cr.whole = 0x8002;  // Bit 15 = 1, whole = 2

    // 181/252, offset = 252 - 128 = 124
    uint8_t offset = 124;
    uint8_t denom_byte = 0x80 | offset;  // End flag set
    cr.tuples[0] = ((uint16_t)181 << 8) | denom_byte;

    return cr;
}

CompactRational create_e_two_tuple() {
    CompactRational cr;
    cr_init(&cr);

    // Best two-tuple: 2 + 55/166 + 89/230
    cr.whole = 0x8002;  // Bit 15 = 1, whole = 2

    // First tuple: 55/166, offset = 166 - 128 = 38
    uint8_t offset1 = 38;
    cr.tuples[0] = ((uint16_t)55 << 8) | offset1;

    // Second tuple: 89/230, offset = 230 - 128 = 102
    uint8_t offset2 = 102;
    uint8_t denom_byte2 = 0x80 | offset2;  // End flag set
    cr.tuples[1] = ((uint16_t)89 << 8) | denom_byte2;

    return cr;
}

int main() {
    printf("=== Canonical CompactRational Representations of e ===\n\n");
    printf("True value of e: %.15f\n\n", E);

    // Single-tuple representation
    printf("--- Best Single-Tuple Representation ---\n");
    CompactRational e_single = create_e_single_tuple();
    printf("Mathematical form: ");
    cr_print(&e_single);
    printf("\n");
    printf("Decimal value: %.15f\n", cr_to_double(&e_single));
    printf("Error: %.15e (%.6f%%)\n",
           fabs(cr_to_double(&e_single) - E),
           fabs(cr_to_double(&e_single) - E) / E * 100);
    printf("Size: %zu bytes\n", cr_size(&e_single));
    cr_print_encoding(&e_single);
    printf("\n");

    // Two-tuple representation
    printf("--- Best Two-Tuple Representation ---\n");
    CompactRational e_two = create_e_two_tuple();
    printf("Mathematical form: ");
    cr_print(&e_two);
    printf("\n");
    printf("Decimal value: %.15f\n", cr_to_double(&e_two));
    printf("Error: %.15e (%.9f%%)\n",
           fabs(cr_to_double(&e_two) - E),
           fabs(cr_to_double(&e_two) - E) / E * 100);
    printf("Size: %zu bytes\n", cr_size(&e_two));
    cr_print_encoding(&e_two);
    printf("\n");

    // Comparison
    printf("--- Comparison ---\n");
    printf("Single-tuple is more compact (%zu bytes) but less accurate\n", cr_size(&e_single));
    printf("Two-tuple is more accurate (%.1fx better) but larger (%zu bytes)\n",
           fabs(cr_to_double(&e_single) - E) / fabs(cr_to_double(&e_two) - E),
           cr_size(&e_two));
    printf("\n");

    printf("=== Recommendation ===\n");
    printf("For general use: 2 + 181/252\n");
    printf("  - Compact: 4 bytes total\n");
    printf("  - Accurate: ~0.001%% error\n");
    printf("  - Simple: single tuple\n\n");

    printf("For high precision: 2 + 55/166 + 89/230\n");
    printf("  - Very accurate: ~0.0000002%% error\n");
    printf("  - Only 6 bytes total\n");
    printf("  - 5000× more accurate than single-tuple\n");

    return 0;
}
