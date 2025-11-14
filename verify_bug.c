#include <stdio.h>
#include <stdint.h>

// Demonstrate the critical bug with negative integers and bit 15

int main() {
    printf("=== Verifying Critical Bug: Negative Integers and Bit 15 ===\n\n");

    // Test case 1: Positive integer (should have bit 15 = 0)
    int16_t pos = 100;
    printf("Positive integer 100:\n");
    printf("  int16_t value: 0x%04X\n", (uint16_t)pos);
    printf("  Bit 15 (tuple flag): %d\n", (pos & 0x8000) ? 1 : 0);
    printf("  Expected bit 15: 0 (no tuples)\n");
    printf("  Status: %s\n\n", (pos & 0x8000) ? "FAIL ❌" : "PASS ✓");

    // Test case 2: Negative integer (should have bit 15 = 0, but doesn't!)
    int16_t neg = -100;
    printf("Negative integer -100:\n");
    printf("  int16_t value: 0x%04X\n", (uint16_t)neg);
    printf("  Bit 15 (tuple flag): %d\n", (neg & 0x8000) ? 1 : 0);
    printf("  Expected bit 15: 0 (no tuples)\n");
    printf("  Status: %s\n\n", (neg & 0x8000) ? "FAIL ❌" : "PASS ✓");

    // Test case 3: Small negative
    int16_t neg_small = -1;
    printf("Negative integer -1:\n");
    printf("  int16_t value: 0x%04X\n", (uint16_t)neg_small);
    printf("  Bit 15 (tuple flag): %d\n", (neg_small & 0x8000) ? 1 : 0);
    printf("  Expected bit 15: 0 (no tuples)\n");
    printf("  Status: %s\n\n", (neg_small & 0x8000) ? "FAIL ❌" : "PASS ✓");

    printf("=== Conclusion ===\n");
    printf("The specification says bit 15 is a tuple presence flag.\n");
    printf("But the implementation uses int16_t, where bit 15 is the sign bit.\n");
    printf("For ALL negative integers, bit 15 is SET (=1).\n");
    printf("This contradicts the spec's claim that bit 15 indicates tuples.\n\n");
    printf("The code only works because it relies on the separate 'num_tuples' field,\n");
    printf("NOT on bit 15 as the specification describes.\n");

    return 0;
}
