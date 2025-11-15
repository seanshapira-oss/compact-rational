#include "compact_rational.h"
#include <stdio.h>
#include <math.h>

#define E 2.718281828459045235360287471352662497757

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
