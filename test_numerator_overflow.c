#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX_TUPLES 5
#define MIN_DENOMINATOR 128
#define MAX_NUMERATOR 255

typedef struct {
    int16_t whole;
    uint16_t tuples[MAX_TUPLES];
} CompactRational;

typedef struct {
    int64_t numerator;
    int64_t denominator;
} Rational;

// From compact_rational.c
extern CompactRational cr_from_fraction(int32_t num, int32_t denom);
extern Rational cr_to_rational(const CompactRational* cr);
extern void cr_print(const CompactRational* cr);
extern CompactRational cr_add(const CompactRational* a, const CompactRational* b);

int main() {
    printf("=== Numerator Overflow Test ===\n\n");
    
    printf("SCENARIO 1: What happens when scaled numerator > 255?\n");
    printf("Creating a fraction that will have a large numerator when scaled\n\n");
    
    // 200/201 scaled to 201 antichain denom: numerator stays 200 (OK)
    // But 256/257 scaled to 257 denom would need numerator 256 (OVERFLOW!)
    
    printf("Test: 256/257\n");
    printf("  This fraction, when mapped to antichain denominator 257,\n");
    printf("  would need a numerator of 256, which exceeds MAX_NUMERATOR (255)\n\n");
    
    CompactRational big = cr_from_fraction(256, 257);
    printf("  Result: ");
    cr_print(&big);
    printf("\n");
    printf("  Expected: Should drop fractional part, store as integer 0\n\n");
    
    printf("SCENARIO 2: Adding many fractions with the same denominator\n");
    printf("This simulates accumulating scores that share a denominator\n\n");
    
    // Adding 200 copies of 1/129 should give us 200/129
    // The scaled numerator would be 200, which is fine
    // But 300 copies would give 300/129, which after reduction might overflow
    
    printf("Test: Adding 200 copies of 1/129\n");
    CompactRational sum = cr_from_fraction(0, 1);
    CompactRational one_129 = cr_from_fraction(1, 129);
    
    for (int i = 0; i < 200; i++) {
        sum = cr_add(&sum, &one_129);
    }
    
    printf("  Result: ");
    cr_print(&sum);
    printf("\n");
    printf("  Expected: 200/129 = 1 71/129 (1.550388)\n\n");
    
    printf("SCENARIO 3: Fraction that reduces, causing numerator overflow\n");
    printf("Creating 32640/128 = 255 exactly (at the boundary)\n\n");
    
    CompactRational boundary = cr_from_fraction(32640, 128);
    printf("  Result: ");
    cr_print(&boundary);
    printf("\n");
    printf("  Expected: 255 (integer)\n\n");
    
    printf("SCENARIO 4: Fraction slightly above boundary\n");
    printf("Creating 32768/128 = 256 (exceeds MAX_NUMERATOR when not reduced)\n\n");
    
    CompactRational above = cr_from_fraction(32768, 128);
    printf("  Result: ");
    cr_print(&above);
    printf("\n");
    printf("  Expected: 256 (integer)\n\n");
    
    printf("SCENARIO 5: Non-reducible fraction with large numerator\n");
    printf("Creating 256/255 which doesn't reduce much\n\n");
    
    CompactRational non_reduce = cr_from_fraction(256, 255);
    printf("  Result: ");
    cr_print(&non_reduce);
    printf("\n");
    printf("  Expected: 1 1/255 (1.003922)\n");
    printf("  Actual behavior: After extracting whole part (1), remainder is 1/255\n");
    printf("  Scaled numerator: 1 (fits in uint8_t)\n\n");
    
    return 0;
}
