# CompactRational Canonicalization Strategies

## Problem Statement

Given an arbitrary CompactRational value (possibly non-canonical), find the "best" canonical representation. A canonical form should be:

1. **Unique**: Two mathematically equal values have the same canonical form
2. **Compact**: Uses minimal space (fewest tuples, smallest numerators)
3. **Normalized**: Follows consistent rules for representation

## Current Implementation Issues

### Issue 1: Duplicate Denominators

When adding `1/2 + 1/4`:
```
Current: whole=0, tuples=[64/128, 32/128]  // 4 bytes (2 tuples)
Canonical: whole=0, tuples=[96/128]         // 2 bytes (1 tuple)
```

### Issue 2: Numerator Overflow

When adding many fractions:
```
200/128 + 100/128 = 300/128
Current: Cannot represent (numerator > 255)
Canonical: whole=2, tuples=[44/128]  // 300 = 2*128 + 44
```

### Issue 3: Unreduced Tuples

```
Current: whole=5, tuples=[128/128]
Canonical: whole=6  // Absorb the 128/128 into whole part
```

### Issue 4: Negative Fractional Parts

```
Current: whole=-3, tuples=[64/128]  // Represents -3.5
Better: whole=-4, tuples=[64/128]   // Represents -3.5 consistently
```
The sign convention needs clarification - should tuples always be positive and sign determined by whole part?

## Canonicalization Approaches

### Approach 1: Greedy Tuple Merging

**Algorithm**:
1. Decode CompactRational to list of (numerator, denominator) pairs
2. Group by denominator
3. Sum numerators for each denominator
4. Extract whole parts (numerator / denominator)
5. Keep only non-zero remainders
6. Re-encode with end flag on last tuple

**Complexity**: O(k²) where k = MAX_TUPLES

**Advantages**:
- Simple to implement
- Guarantees no duplicate denominators
- Handles numerator overflow naturally

**Disadvantages**:
- Doesn't optimize denominator choice
- May not produce smallest representation

**Example**:
```c
Input:  whole=7, tuples=[64/128, 32/128, 43/129]
Decode: 7 + 64/128 + 32/128 + 43/129
Group:  7 + 96/128 + 43/129
Reduce: 7 + (extract 0) + 43/129 = 7 + 96/128 + 43/129
Output: whole=7, tuples=[96/128(end=0), 43/129(end=1)]
```

### Approach 2: Convert to Standard Rational and Decompose

**Algorithm**:
1. Convert CompactRational to single Rational (numerator/denominator)
2. Reduce to lowest terms
3. Extract whole part
4. Decompose fractional remainder into optimal tuple sequence
5. Use greedy algorithm to pick best antichain denominators

**Complexity**: O(k × d) where d = denominator range (128-255)

**Advantages**:
- Can find optimal denominator choices
- Produces truly minimal representation
- Mathematically clean approach

**Disadvantages**:
- More complex implementation
- Computationally expensive
- May not be deterministic (multiple optimal solutions)

**Example**:
```c
Input:  whole=7, tuples=[96/128, 86/129]
Convert: (7×128×129 + 96×129 + 86×128) / (128×129) = 128,630 / 16,512
Reduce:  Simplify to lowest terms
Decompose: Find optimal antichain representation
```

### Approach 3: Lazy Canonicalization

**Algorithm**:
1. Only canonicalize when necessary (e.g., before encoding)
2. Keep internal representation as standard Rational
3. Convert to CompactRational only for storage/transmission

**Complexity**: O(1) for operations, O(k×d) for final encoding

**Advantages**:
- Fast for intermediate calculations
- Can use optimal algorithm for final encoding
- No accumulation of representation errors

**Disadvantages**:
- Requires two representation types
- More complex API
- May use more memory temporarily

### Approach 4: Egyptian Fraction Decomposition (Greedy Algorithm)

**Algorithm**:
1. Convert to single Rational: n/d
2. Extract whole part: w = n/d, remainder r = n mod d
3. For remainder r/d:
   - Find largest antichain denominator D where D ≥ d
   - Add tuple with numerator = ⌊(r × D) / d⌋
   - Update remainder
   - Repeat until remainder is 0 or MAX_TUPLES reached

**Complexity**: O(k)

**Advantages**:
- Deterministic (always same output for same input)
- Relatively simple
- Produces reasonable results

**Disadvantages**:
- May not be optimal
- Greedy choice can lead to suboptimal overall representation
- Sensitive to antichain denominator ordering

### Approach 5: Dynamic Programming for Optimal Decomposition

**Algorithm**:
1. For fractional part f, find all possible tuple representations
2. Use DP to minimize:
   - Number of tuples (primary)
   - Total size in bytes (secondary)
   - Sum of numerators (tertiary, for tie-breaking)
3. Memoize subproblems

**Complexity**: O(k × 255 × numerator_range)

**Advantages**:
- Provably optimal representation
- Handles all edge cases
- Can optimize for different criteria

**Disadvantages**:
- Complex implementation
- High computational cost
- May be overkill for simple use cases

## Recommended Approach for CompactRational

### Primary: Approach 1 (Greedy Tuple Merging)

This is the best balance of simplicity and effectiveness:

```c
CompactRational cr_canonicalize(const CompactRational* cr) {
    // Step 1: Decode to intermediate form
    int32_t whole = extract_whole(cr);

    // Step 2: Collect all tuples and group by denominator
    // Use array indexed by denominator (128-255)
    uint32_t numerators[128] = {0};  // Index 0 = denom 128, etc.

    for each tuple in cr:
        numerators[denom - 128] += tuple_numerator

    // Step 3: Extract whole parts from numerators
    for i in 0..127:
        uint8_t denom = 128 + i
        if numerators[i] >= denom:
            whole += numerators[i] / denom
            numerators[i] %= denom

    // Step 4: Build canonical tuple list
    // Only include non-zero numerators
    CompactRational result
    result.whole = whole (with tuple flag if needed)

    int tuple_idx = 0
    for i in 0..127:
        if numerators[i] > 0:
            result.tuples[tuple_idx] = encode_tuple(numerators[i], 128+i, is_last)
            tuple_idx++

    return result
}
```

### Enhancement: Optimal Denominator Selection

For truly optimal results, when creating a CompactRational from a standard rational n/d:

```c
// Find the best antichain denominator(s) to represent n/d
// This minimizes the number of tuples needed

CompactRational cr_from_fraction_optimal(int64_t num, int64_t denom) {
    reduce(&num, &denom)

    // Extract whole
    int32_t whole = num / denom
    int64_t remainder = num % denom

    if (remainder == 0)
        return cr_from_int(whole)

    // Find optimal antichain decomposition
    // Try to represent remainder/denom with minimal tuples

    // Strategy: Find antichain denominators D1, D2, ... such that
    // remainder/denom = n1/D1 + n2/D2 + ...
    // minimizing the number of terms

    // Greedy approach:
    for d in 128..255:
        if (remainder * d) % denom == 0:
            // Can represent exactly with single denominator
            numerator = (remainder * d) / denom
            if numerator <= 255:
                // Single tuple solution found!
                return create_single_tuple(whole, numerator, d)

    // If no single-tuple solution, use multi-tuple approach
    // (More complex algorithm needed)
}
```

## Canonicalization Properties

A canonical CompactRational should satisfy:

1. **No duplicate denominators**: Each antichain denominator appears at most once
2. **Numerators in range**: All numerators n satisfy 0 < n < denominator
3. **Minimal tuples**: No two tuples can be combined into one
4. **Whole maximization**: Whole part is as large as possible (fractional part < 1)
5. **Deterministic ordering**: Tuples appear in a fixed order (e.g., ascending denominator)

## Testing Canonicalization

### Test Cases

```c
// Test 1: Duplicate denominators
Input:  whole=0, tuples=[64/128, 32/128]
Output: whole=0, tuples=[96/128]

// Test 2: Numerator overflow
Input:  whole=0, tuples=[200/128, 100/128]
Output: whole=2, tuples=[44/128]

// Test 3: Absorbed tuples
Input:  whole=5, tuples=[128/128]
Output: whole=6

// Test 4: Multiple operations
cr1 = 1/2
cr2 = 1/4
cr3 = 1/4
sum = cr1 + cr2 + cr3  // Should be 1 exactly
canonical(sum) = whole=1  // Not whole=0, tuples=[128/128]

// Test 5: Complex sum
cr1 = 7 + 1/3
cr2 = 5 + 1/2
cr3 = 2 + 1/6
sum = cr1 + cr2 + cr3  // = 15
canonical(sum) = whole=15  // All fractions cancel
```

## Implementation Priority

1. **Phase 1**: Implement `cr_canonicalize()` with Approach 1 (tuple merging)
2. **Phase 2**: Update `cr_add()` to automatically canonicalize results
3. **Phase 3**: Add tests to verify canonical properties
4. **Phase 4**: (Optional) Implement optimal denominator selection for `cr_from_fraction()`

## Performance Considerations

- Canonicalization is O(k) where k=5, so very fast
- Can be done automatically after each operation
- Memory overhead: minimal (reuses existing structure)
- Trade-off: slight computation cost vs. space savings

## Conclusion

**Greedy Tuple Merging (Approach 1)** is the recommended canonicalization strategy because:
- Simple to implement and understand
- Fast (O(k) with small constant)
- Handles all critical issues (duplicates, overflow, reduction)
- Produces compact representations in practice
- Deterministic and testable

For advanced use cases requiring provably optimal representations, Approach 2 or 5 could be considered, but the added complexity is likely not justified for the target use case of scoring systems.
