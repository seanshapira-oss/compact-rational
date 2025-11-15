#ifndef COMPACT_RATIONAL_H
#define COMPACT_RATIONAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define MAX_TUPLES 5
#define MIN_DENOMINATOR 128
#define MAX_DENOMINATOR 255
#define MAX_NUMERATOR 255
#define MAX_WHOLE_VALUE 16383
#define MIN_WHOLE_VALUE -16383

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

/**
 * Compact Rational structure
 *
 * Encoding:
 * - Bit 15 of 'whole': tuple presence flag (0 = integer only, 1 = tuples follow)
 * - Bits 14-0 of 'whole': signed 15-bit integer (-16383 to +16383)
 * - Each tuple: high byte = numerator, low byte = denominator info
 * - Last tuple: bit 7 set in denominator byte to indicate end of sequence
 */
typedef struct {
    int16_t whole;                    // Bit 15: tuple flag, Bits 14-0: signed 15-bit integer
    uint16_t tuples[MAX_TUPLES];      // Each: high byte = numerator, low byte = denom info
} CompactRational;

/**
 * Standard rational structure (for intermediate calculations)
 */
typedef struct {
    int64_t numerator;
    int64_t denominator;
} Rational;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Compute greatest common divisor using Euclidean algorithm
 */
int64_t gcd(int64_t a, int64_t b);

/**
 * Reduce a rational to lowest terms
 */
void reduce_rational(Rational* r);

/**
 * Find best antichain denominator for a given denominator
 */
uint8_t find_antichain_denominator(int64_t denom);

// ============================================================================
// CONSTRUCTION AND INITIALIZATION
// ============================================================================

/**
 * Initialize a compact rational to zero
 */
void cr_init(CompactRational* cr);

/**
 * Create compact rational from integer
 * Values outside [-16383, 16383] will be clamped with a warning
 */
CompactRational cr_from_int(int32_t value);

/**
 * Create compact rational from numerator and denominator
 * The fraction will be reduced and encoded optimally
 */
CompactRational cr_from_fraction(int32_t num, int32_t denom);

// ============================================================================
// CONVERSION FUNCTIONS
// ============================================================================

/**
 * Decode compact rational to standard rational
 */
Rational cr_to_rational(const CompactRational* cr);

/**
 * Convert to double (for display/comparison)
 */
double cr_to_double(const CompactRational* cr);

// ============================================================================
// ARITHMETIC OPERATIONS
// ============================================================================

/**
 * Add two compact rationals
 * Result is automatically reduced and re-encoded
 */
CompactRational cr_add(const CompactRational* a, const CompactRational* b);

// ============================================================================
// DISPLAY AND DEBUG FUNCTIONS
// ============================================================================

/**
 * Print compact rational in human-readable form
 * Format: "whole num/denom (decimal_value)"
 */
void cr_print(const CompactRational* cr);

/**
 * Print raw encoding (for debugging)
 * Shows the internal bit representation
 */
void cr_print_encoding(const CompactRational* cr);

/**
 * Get size in bytes
 * Returns actual storage size (2 bytes for whole + 2 bytes per tuple)
 */
size_t cr_size(const CompactRational* cr);

#endif // COMPACT_RATIONAL_H
