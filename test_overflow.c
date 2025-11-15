#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_TUPLES 5
#define MIN_DENOMINATOR 128
#define MAX_DENOMINATOR 255
#define MAX_NUMERATOR 255
#define MAX_WHOLE_VALUE 16383
#define MIN_WHOLE_VALUE -16383

typedef struct {
    int16_t whole;
    uint16_t tuples[MAX_TUPLES];
} CompactRational;

typedef struct {
    int64_t numerator;
    int64_t denominator;
} Rational;

// Minimal implementations needed for testing
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

uint8_t find_antichain_denominator(int64_t denom) {
    for (int d = MIN_DENOMINATOR; d <= MAX_DENOMINATOR; d++) {
        if (d % denom == 0) {
            return (uint8_t)d;
        }
    }
    if (denom >= MIN_DENOMINATOR && denom <= MAX_DENOMINATOR) {
        return (uint8_t)denom;
    }
    return MIN_DENOMINATOR;
}

void cr_init(CompactRational* cr) {
    cr->whole = 0;
    memset(cr->tuples, 0, sizeof(cr->tuples));
}

CompactRational cr_from_int(int32_t value) {
    CompactRational cr;
    cr_init(&cr);
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
    cr.whole = (int16_t)(value & 0x7FFF);
    return cr;
}

CompactRational cr_from_fraction(int32_t num, int32_t denom) {
    CompactRational cr;
    cr_init(&cr);
    
    if (denom == 0) {
        fprintf(stderr, "Error: division by zero\n");
        return cr;
    }
    
    Rational r = {num, denom};
    reduce_rational(&r);
    
    int32_t whole = (int32_t)(r.numerator / r.denominator);
    int64_t remainder_num = r.numerator % r.denominator;
    
    if (remainder_num < 0) {
        remainder_num += r.denominator;
        whole -= 1;
    }
    
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

    if (remainder_num != 0) {
        uint8_t antichain_denom = find_antichain_denominator(r.denominator);
        uint64_t scaled_num = (remainder_num * antichain_denom) / r.denominator;

        printf("  [DEBUG] Encoding fraction: %lld/%lld\n", remainder_num, r.denominator);
        printf("  [DEBUG] Antichain denom: %d, scaled numerator: %llu\n", antichain_denom, scaled_num);

        if (scaled_num > 0 && scaled_num <= MAX_NUMERATOR) {
            cr.whole = (int16_t)((whole & 0x7FFF) | 0x8000);
            uint8_t offset = antichain_denom - MIN_DENOMINATOR;
            uint8_t denom_byte = 0x80 | offset;
            cr.tuples[0] = ((uint16_t)scaled_num << 8) | denom_byte;
            printf("  [DEBUG] Created 1 tuple: %llu/%d\n", scaled_num, antichain_denom);
        } else {
            printf("  [DEBUG] Scaled numerator (%llu) exceeds MAX_NUMERATOR (%d), storing as integer only\n", 
                   scaled_num, MAX_NUMERATOR);
            cr.whole = (int16_t)(whole & 0x7FFF);
        }
    } else {
        cr.whole = (int16_t)(whole & 0x7FFF);
    }
    
    return cr;
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

        if (is_last) {
            break;
        }
    }

    return result;
}

double cr_to_double(const CompactRational* cr) {
    Rational r = cr_to_rational(cr);
    if (r.denominator == 0) {
        fprintf(stderr, "Error: division by zero in cr_to_double\n");
        return 0.0;
    }
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

CompactRational cr_add(const CompactRational* a, const CompactRational* b) {
    Rational ra = cr_to_rational(a);
    Rational rb = cr_to_rational(b);

    Rational sum;
    sum.numerator = ra.numerator * rb.denominator + rb.numerator * ra.denominator;
    sum.denominator = ra.denominator * rb.denominator;

    printf("  [DEBUG] Before reduction: %lld/%lld\n", sum.numerator, sum.denominator);
    reduce_rational(&sum);
    printf("  [DEBUG] After reduction: %lld/%lld\n", sum.numerator, sum.denominator);

    if (sum.numerator > INT32_MAX || sum.numerator < INT32_MIN ||
        sum.denominator > INT32_MAX || sum.denominator < INT32_MIN) {
        fprintf(stderr, "Error: overflow in addition - result (%lld/%lld) exceeds int32_t range\n",
                (long long)sum.numerator, (long long)sum.denominator);
        return cr_from_int(0);
    }

    return cr_from_fraction((int32_t)sum.numerator, (int32_t)sum.denominator);
}

int main() {
    printf("=== CompactRational Tuple Overflow Investigation ===\n\n");
    
    printf("1. Maximum number of tuples: MAX_TUPLES = %d\n\n", MAX_TUPLES);
    
    printf("2. Testing addition that might require multiple tuples:\n");
    printf("   Adding fractions with different antichain denominators\n\n");
    
    // Create fractions with different antichain denominators
    // 1/3 uses denominator 129
    // 1/5 uses denominator 130
    printf("Test A: Adding 1/3 + 1/5 (should need 2 different tuples)\n");
    CompactRational a1 = cr_from_fraction(1, 3);
    printf("  1/3 = ");
    cr_print(&a1);
    printf("\n");
    
    CompactRational a2 = cr_from_fraction(1, 5);
    printf("  1/5 = ");
    cr_print(&a2);
    printf("\n");
    
    printf("  Adding...\n");
    CompactRational sum_a = cr_add(&a1, &a2);
    printf("  Result: ");
    cr_print(&sum_a);
    printf("\n");
    printf("  Expected: 8/15 (0.533333)\n\n");
    
    printf("Test B: Complex addition requiring many denominators\n");
    printf("  1/2 + 1/3 + 1/5 + 1/7\n");
    CompactRational b1 = cr_from_fraction(1, 2);
    CompactRational b2 = cr_from_fraction(1, 3);
    CompactRational b3 = cr_from_fraction(1, 5);
    CompactRational b4 = cr_from_fraction(1, 7);
    
    printf("  Adding step by step:\n");
    CompactRational sum_b = cr_add(&b1, &b2);
    printf("    1/2 + 1/3 = ");
    cr_print(&sum_b);
    printf("\n");
    
    sum_b = cr_add(&sum_b, &b3);
    printf("    + 1/5 = ");
    cr_print(&sum_b);
    printf("\n");
    
    sum_b = cr_add(&sum_b, &b4);
    printf("    + 1/7 = ");
    cr_print(&sum_b);
    printf("\n");
    printf("  Expected: 247/210 = 1 37/210 (1.176190)\n\n");
    
    printf("Test C: Numerator overflow test\n");
    printf("  Adding many 1/3's to force numerator > 255\n");
    CompactRational sum_c = cr_from_int(0);
    for (int i = 0; i < 100; i++) {
        CompactRational third = cr_from_fraction(1, 3);
        sum_c = cr_add(&sum_c, &third);
    }
    printf("  100 × (1/3) = ");
    cr_print(&sum_c);
    printf("\n");
    printf("  Expected: 100/3 = 33 1/3 (33.333333)\n\n");
    
    printf("3. Key findings:\n");
    printf("   - cr_from_fraction() only creates AT MOST 1 tuple\n");
    printf("   - Even if result needs multiple denominators, only one is encoded\n");
    printf("   - Addition converts to rational, adds, then converts back\n");
    printf("   - The conversion back loses information if multiple tuples needed\n");
    printf("   - If numerator > 255, fractional part is dropped entirely\n\n");
    
    return 0;
}
