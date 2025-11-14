# Compact Rational Number Representation

## Overview

A space-efficient encoding scheme for rational numbers optimized for values that are "mostly integers with occasional simple fractions." The representation uses a signed whole part plus a variable-length sequence of fractional tuples.

## Representation Format

### Structure

1. **Whole part** (16 bits):
   - Bit 15: Tuple presence flag (0 = integer only, 1 = tuples follow)
   - Bits 14-0: Signed integer (-16,384 to +16,383)

2. **Tuple sequence** (variable length, only if bit 15 = 1):
   - Each tuple is 16 bits:
     - 8 bits: Numerator (0-255)
     - 8 bits: Denominator byte
       - Bit 7: End flag (1 = last tuple, 0 = more follow)
       - Bits 6-0: Denominator offset (0-127)
       - Actual denominator = 128 + offset

3. **Value calculation**:
   - Sign = sign of whole part
   - Value = Sign × (|whole| + sum of all fractional tuples)

### Antichain Denominator Set

Valid denominators form an **antichain** (no element divides another): **{128, 129, 130, ..., 255}**

This ensures no redundancy in representation - you never need both denominator 2 and denominator 4, for example, since they can be combined.

## Expressiveness

### Size Efficiency

| Content | Size | Example |
|---------|------|---------|
| Pure integer | 2 bytes | 42 |
| Integer + 1 fraction | 4 bytes | 7⅓ |
| Integer + 2 fractions | 6 bytes | 10 + ½ + ⅕ |
| Integer + k fractions | 2 + 2k bytes | (complex sum) |

**Comparison to baseline** (32-bit numerator + 32-bit denominator = 8 bytes):
- Integers: 75% smaller
- Simple fractions: 50% smaller
- Complex rationals: potentially larger

### Value Range

With 15-bit signed whole part and k tuples:
- **Whole part range**: -16,384 to +16,383
- **Maximum value** (with k tuples): ≈ ±16,383 + (fractional extensions)
- k=5 adds approximately ±9.8 to the range

### Common Fractions

Simple unit fractions map efficiently to antichain denominators:

| Fraction | Representation | Antichain Denominator |
|----------|----------------|----------------------|
| 1/2 | 64/128 | 128 |
| 1/3 | 43/129 | 129 |
| 1/4 | 32/128 | 128 |
| 1/5 | 26/130 | 130 |
| 1/6 | 22/132 | 132 |
| 1/7 | 19/133 | 133 |
| 1/8 | 16/128 | 128 |

**Key insight**: Fractions with denominators 2, 4, and 8 all use the same antichain denominator (128), so they combine in a single tuple when summed.

## Recommended Configuration for Score/Grade Use Case

### Parameter: k = 5 (maximum 5 tuples)

**Rationale**: 
- Scores use simple fractions with denominators {2, 3, 4, 5, 6, 7, 8}
- These map to exactly 5 distinct antichain denominators: {128, 129, 130, 132, 133}
- Guarantees **exact arithmetic** when summing any number of scores

**Properties**:
- Individual scores: typically 2 bytes (integers) or 4 bytes (integer + simple fraction)
- Grades (sums of scores): always exactly representable, up to 12 bytes maximum
- No rounding errors or approximations in grade calculations
- Significant space savings over 8-byte baseline for individual scores

### Example Encodings

**Score: 7** (integer)
- Bit 15 = 0 (no tuples)
- Bits 14-0 = 7
- **Total: 2 bytes**

**Score: 7⅓**
- Bits 15-0 = 0x8007 (flag=1, whole=7)
- Tuple: numerator=43, denominator byte=0x81 (offset=1, end=1)
- Decodes to: 7 + 43/129 = 7⅓
- **Total: 4 bytes**

**Grade: 10½ + 15⅓ + 20⅕ = 45 + 31/30**
- Bits 15-0 = flag=1, whole=46 (since 31/30 > 1, we adjust)
- Actually: 45 + 15/30 + 10/30 + 6/30 = 45 + 31/30 = 46 + 1/30
- Tuple 1: 8/128 (for 1/16 component after reduction)
- ...
- (More complex encoding, uses multiple tuples)
- **Total: varies, up to 12 bytes**

## Limitations

### 1. Restricted Value Range
- Whole part limited to ±16,383 (15 bits)
- If larger integers are common, this representation is unsuitable
- Fractional extensions add only ~±10 to this range

### 2. Denominator Constraints
- Only denominators in range 128-255 are directly representable
- Small denominators (like 3, 5, 7) must be scaled up
- This is transparent for exact representation but affects the numerator range

### 3. Numerator Overflow Risk
- Numerators limited to 0-255
- When summing many fractions with the same denominator, the numerator might overflow
- Example: Summing 300 scores of "1 + 1/3" each:
  - Whole part: 300 (✓ within range)
  - Fractional part: 300 × (43/129) = 12,900/129 ≈ 100
  - Reduced form would need numerator > 255 for some intermediate calculations
- **Mitigation**: During accumulation, convert to whole numbers when numerator exceeds 255
  - 300/129 = 2 remainder 42, so add 2 to whole part, keep 42/129

### 4. Fixed k Limits Expressiveness
- With k=5, cannot represent rationals requiring more than 5 distinct denominators
- If scoring system expands to include 1/9, 1/10, etc., might need larger k
- Each increase in k increases worst-case size by 2 bytes

### 5. Not a General Replacement
- Only beneficial when values are actually "mostly integers"
- If data is uniformly distributed rationals with arbitrary denominators, baseline is better
- Average size approaches 2+2k bytes when all k tuples are frequently used

## When to Use This Representation

**Ideal for**:
- Scoring systems with integer and simple fractional scores
- Financial calculations with limited decimal places
- Measurement systems using common fractions (halves, thirds, quarters, etc.)
- Data where >50% of values are integers or single simple fractions

**Not suitable for**:
- General rational arithmetic with arbitrary denominators
- Very large integers (beyond ±16K)
- Data requiring many simultaneous fractional components
- Systems where 8 bytes is already considered compact

## Conclusion

For the score/grade use case with simple fractions {1/2, 1/3, 1/4, 1/5, 1/6, 1/7, 1/8}, using **k=5** provides:
- ✓ Exact arithmetic (no rounding errors)
- ✓ 50-75% space savings for individual scores
- ✓ Guaranteed representability of all grades
- ✓ Reasonable worst-case size (12 bytes vs 8 byte baseline)

The representation elegantly balances compactness with expressiveness for the target use case.
