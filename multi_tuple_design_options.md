# Multi-Tuple cr_add() Implementation Options

## The Problem
Given a fraction like `1119/1739` that doesn't fit in a single antichain denominator [128, 255], how do we represent it as a sum of multiple tuples?

Target: `a₁/d₁ + a₂/d₂ + a₃/d₃ + ...` where:
- Each `dᵢ ∈ [128, 255]` (antichain denominator)
- Each `aᵢ ≤ 255` (fits in uint8_t numerator)
- Use at most 5 tuples

---

## Option 1: Greedy Antichain Decomposition

**Algorithm:**
1. Start with target fraction `n/d`
2. Find the largest fraction `a/dc` where:
   - `dc` is an antichain denominator in [128, 255]
   - `a ≤ 255`
   - `a/dc ≤ n/d`
3. Subtract: `remainder = n/d - a/dc`
4. Repeat with remainder until:
   - Remainder is zero (exact), OR
   - Remainder is small enough to ignore (approximation), OR
   - Used 5 tuples (MAX_TUPLES limit)

**Example: 1119/1739 ≈ 0.643473**
```
Step 1: Find largest fraction ≤ 0.643473
  Try 164/255 = 0.643137 ✓ (close!)
  Create tuple: 164/255

Step 2: Remainder = 1119/1739 - 164/255 = 0.000336
  = (1119×255 - 164×1739) / (1739×255) = 589/443445

Step 3: Find fraction for 589/443445 ≈ 0.001328
  Try 1/128 = 0.007813 (too big)
  Result is very small, might approximate as 0

Final: 164/255 (1 tuple, small error)
```

**Pros:**
- Simple to implement
- Guaranteed termination
- First tuple captures most precision

**Cons:**
- May not find optimal decomposition
- Can accumulate rounding errors
- No guarantee of minimal tuples

---

## Option 2: LCM-Based Multi-Denominator Sum

**Algorithm:**
1. Start with `n/d` (already in lowest terms)
2. Find a set of antichain denominators whose LCM divides `d` or has small remainder
3. Express `n/d` as sum of fractions with those denominators

**Example: If d=1739 could be factored nicely**
```
Problem: 1739 is prime!
Can't factor into antichain denominators

Fallback: Find denominators d₁, d₂ where LCM(d₁,d₂) ≈ 1739
```

**Pros:**
- Could give exact representations when LCM aligns
- Mathematically elegant

**Cons:**
- Many denominators won't factor nicely
- 1739 is prime - no factorization possible
- Complex to implement
- Won't work for most cases

---

## Option 3: Continued Fraction Approximation

**Algorithm:**
1. Compute continued fraction expansion: `[a₀; a₁, a₂, a₃, ...]`
2. Use convergents to guide antichain denominator selection
3. Build multi-tuple representation from convergents

**Example: 1119/1739**
```
Continued fraction: [0; 1, 1, 1, 4, 1, 2, 2, ...]
Convergents:
  0/1
  1/2      (can't use: 2 not in antichain)
  1/3      (can't use: 3 not in antichain)
  2/5      (can't use: 5 not in antichain)
  9/14     (can't use: 14 not in antichain)
  ...

Need to adapt convergents to antichain range [128, 255]
```

**Pros:**
- Mathematically optimal approximations
- Well-studied theory

**Cons:**
- Complex to implement
- Convergents may not align with antichain denominators
- Requires significant adaptation

---

## Option 4: Best-Fit Search with Backtracking

**Algorithm:**
1. Use search/optimization to find best combination of antichain denominators
2. Minimize error: `|target - (a₁/d₁ + a₂/d₂ + ... + aₙ/dₙ)|`
3. Constraints:
   - Each `dᵢ ∈ [128, 255]`
   - Each `aᵢ ∈ [1, 255]`
   - `n ≤ 5` (MAX_TUPLES)

**Example: Search for 1119/1739**
```
Search space: 128 choices for each denominator
Try all combinations up to depth 5
Pick combination with minimum error

This could find: 82/128 + 1/200 or similar
```

**Pros:**
- Can find optimal decomposition
- Minimizes representation error

**Cons:**
- Computationally expensive (128^5 possibilities)
- Overkill for simple fractions
- Need heuristics to prune search space

---

## Option 5: Egyptian Fraction Greedy (Unit Fractions)

**Algorithm:**
1. Decompose into unit fractions: `1/d₁ + 1/d₂ + ...`
2. Use greedy algorithm (largest unit fraction first)
3. Scale numerators if needed

**Example: 1119/1739**
```
Extract whole part: 0
Remainder: 1119/1739

Greedy unit fractions:
  Largest 1/d ≤ 1119/1739: 1/2 (but 2 not in antichain)
  Largest 1/d in [128,255]: 1/128

  1119/1739 - 1/128 = (1119×128 - 1739)/(1739×128)
                    = (143232 - 1739)/222592
                    = 141493/222592

Continue with 141493/222592...
```

**Pros:**
- Well-known algorithm
- Guaranteed termination

**Cons:**
- Can require many terms
- May exceed 5 tuples
- Unit fractions waste numerator space (only use 1 out of 255)

---

## Option 6: Hybrid Greedy with Error Bounds

**Algorithm:**
1. Use greedy approach (Option 1)
2. Track error at each step
3. If error exceeds threshold, add another tuple
4. Stop when error < epsilon OR hit MAX_TUPLES

**Example Implementation:**
```c
CompactRational cr_from_fraction_multituple(int32_t num, int32_t denom) {
    CompactRational cr;
    cr_init(&cr);

    // Extract whole part
    int32_t whole = num / denom;
    int64_t rem_num = num % denom;

    if (rem_num < 0) {
        rem_num += denom;
        whole--;
    }

    // Store whole part
    cr.whole = whole | 0x8000;  // Set tuple flag

    int tuple_idx = 0;
    double error_threshold = 0.0001;  // Configurable

    while (rem_num > 0 && tuple_idx < MAX_TUPLES) {
        // Find best antichain denominator
        uint8_t best_denom = MIN_DENOMINATOR;
        uint8_t best_num = 0;
        double best_error = 1.0;

        for (int d = MIN_DENOMINATOR; d <= MAX_DENOMINATOR; d++) {
            uint64_t scaled = (rem_num * d) / denom;
            if (scaled > MAX_NUMERATOR) scaled = MAX_NUMERATOR;
            if (scaled == 0) continue;

            // Calculate error if we use this tuple
            double tuple_val = (double)scaled / d;
            double target_val = (double)rem_num / denom;
            double error = fabs(target_val - tuple_val);

            if (error < best_error) {
                best_error = error;
                best_num = scaled;
                best_denom = d;
            }
        }

        // Add this tuple
        bool is_last = (tuple_idx == MAX_TUPLES - 1) || (best_error < error_threshold);
        uint8_t denom_byte = (best_denom - MIN_DENOMINATOR) | (is_last ? 0x80 : 0x00);
        cr.tuples[tuple_idx] = (best_num << 8) | denom_byte;

        // Update remainder
        rem_num = rem_num * best_denom - best_num * denom;
        denom = denom * best_denom;
        reduce_rational(&(Rational){rem_num, denom});

        if (best_error < error_threshold) break;
        tuple_idx++;
    }

    return cr;
}
```

**Pros:**
- Balances accuracy vs. tuple count
- Configurable error threshold
- Practical and implementable

**Cons:**
- Still greedy (may not be optimal)
- Need to choose error threshold
- More complex than single-tuple version

---

## Recommendation: Hybrid Greedy (Option 6)

**Why:**
1. **Practical**: Can implement with reasonable complexity
2. **Flexible**: Error threshold lets users choose accuracy vs. space
3. **Bounded**: Guaranteed to use ≤ MAX_TUPLES
4. **Good enough**: Greedy gives decent (if not optimal) results

**Key design decisions:**
- Error threshold: 0.0001 (configurable)
- Search all denominators [128, 255] at each step
- Pick denominator that minimizes error for current remainder
- Stop when error is acceptable or tuples exhausted

**Alternative for exact results:**
- Could also implement Option 4 (search) for small fractions where exactness matters
- Use hybrid for larger/complex fractions where approximation is acceptable

---

## Implementation Considerations

1. **Backward compatibility**: Need to handle existing single-tuple CompactRationals
2. **Testing**: Need comprehensive tests for multi-tuple arithmetic
3. **Error propagation**: How do errors compound through multiple operations?
4. **Performance**: Multi-tuple operations will be slower
5. **Configuration**: Should error threshold be configurable?

## Next Steps

1. Implement Option 6 (Hybrid Greedy) in `cr_from_fraction()`
2. Update `cr_add()` to call the new implementation
3. Add tests for multi-tuple generation
4. Verify correctness with test cases like 12/37 + 15/47
5. Benchmark performance impact
