# Compact Rational Number Representation

A space-efficient encoding scheme for rational numbers optimized for values that are "mostly integers with occasional simple fractions."

## Overview

This C implementation provides a compact representation that uses significantly less space than the standard 8-byte (32-bit numerator + 32-bit denominator) approach for common use cases like scoring and grading systems.

### Key Features

- **Space Efficient**: 2 bytes for integers, 4 bytes for simple fractions (50-75% space savings)
- **Exact Arithmetic**: No rounding errors when working with common fractions
- **Optimized for Scoring**: Perfect for educational grading, test scores, and similar applications
- **Antichain Denominators**: Uses denominators 128-255 to eliminate redundancy

## Representation Format

The encoding consists of:

1. **Whole part** (16 bits):
   - Bit 15: Tuple presence flag (0 = integer only, 1 = tuples follow)
   - Bits 14-0: Signed integer (-16,384 to +16,383)

2. **Tuple sequence** (variable length, only if bit 15 = 1):
   - Each tuple is 16 bits:
     - 8 bits: Numerator (0-255)
     - 8 bits: Denominator info
       - Bit 7: End flag
       - Bits 6-0: Denominator offset (actual = 128 + offset)

## Size Comparison

| Content | Compact Size | Baseline (8 bytes) | Savings |
|---------|--------------|-------------------|---------|
| Pure integer | 2 bytes | 8 bytes | 75% |
| Simple fraction | 4 bytes | 8 bytes | 50% |
| Complex rational | 2 + 2k bytes | 8 bytes | Varies |

## Usage

### Basic Example

```c
#include "compact_rational.h"

// Create from integer
CompactRational score1 = cr_from_int(42);

// Create from fraction (7 and 1/3)
CompactRational score2 = cr_from_fraction(22, 3);

// Add scores
CompactRational total = cr_add(&score1, &score2);

// Print result
cr_print(&total);  // Output: 49 1/3 (49.333333)

// Convert to double
double value = cr_to_double(&total);  // 49.333333
```

### Compiling

```bash
make
./compact_rational
```

Or compile manually:

```bash
gcc -o compact_rational compact_rational.c -lm -Wall
./compact_rational
```

## API Reference

### Creation Functions

- `CompactRational cr_from_int(int32_t value)` - Create from integer
- `CompactRational cr_from_fraction(int32_t num, int32_t denom)` - Create from numerator/denominator
- `void cr_init(CompactRational* cr)` - Initialize to zero

### Conversion Functions

- `Rational cr_to_rational(const CompactRational* cr)` - Convert to standard rational
- `double cr_to_double(const CompactRational* cr)` - Convert to floating point

### Arithmetic Functions

- `CompactRational cr_add(const CompactRational* a, const CompactRational* b)` - Add two rationals

### Utility Functions

- `void cr_print(const CompactRational* cr)` - Print human-readable form
- `void cr_print_encoding(const CompactRational* cr)` - Print raw encoding (debug)
- `size_t cr_size(const CompactRational* cr)` - Get size in bytes

## Common Fractions

Simple unit fractions map efficiently to antichain denominators:

| Fraction | Antichain Denominator | Encoding Size |
|----------|----------------------|---------------|
| 1/2 | 128 | 4 bytes |
| 1/3 | 129 | 4 bytes |
| 1/4 | 128 | 4 bytes |
| 1/5 | 130 | 4 bytes |
| 1/6 | 132 | 4 bytes |
| 1/7 | 133 | 4 bytes |
| 1/8 | 128 | 4 bytes |

**Note**: Fractions with denominators 2, 4, and 8 all use antichain denominator 128, so they combine efficiently.

## Limitations

1. **Value Range**: Whole part limited to ±16,383
2. **Denominator Constraints**: Only denominators 128-255 directly representable
3. **Numerator Overflow**: Individual tuple numerators limited to 0-255
4. **Fixed Complexity**: Maximum 5 tuples (configurable via `MAX_TUPLES`)

## Use Cases

**Ideal for:**
- Educational grading systems
- Test scoring with fractional points
- Financial calculations with limited decimal places
- Measurement systems using common fractions

**Not suitable for:**
- General rational arithmetic with arbitrary denominators
- Very large integers (beyond ±16K)
- High-precision scientific calculations

## Configuration

The implementation uses `MAX_TUPLES = 5`, which is optimal for scoring systems with denominators {2, 3, 4, 5, 6, 7, 8}. This can be adjusted in the source if needed.

## License

MIT License - See LICENSE file for details

## Author

Implementation based on the compact rational representation specification.

## Contributing

Contributions welcome! Please feel free to submit issues or pull requests.
