# Error Reporting Guide

This document explains how to use the new error reporting mechanism in the compact rational library.

## Overview

The library now supports programmatic error reporting, allowing calling code to detect and handle errors instead of relying on stderr output. This is **fully backward compatible** - existing code continues to work without modification.

## Error Types

```c
typedef enum {
    CR_SUCCESS = 0,                   // Operation completed successfully
    CR_ERROR_NONE = 0,                // Alias for CR_SUCCESS
    CR_ERROR_DIVISION_BY_ZERO,        // Attempted division by zero
    CR_ERROR_VALUE_CLAMPED,           // Input value was clamped to valid range
    CR_ERROR_OVERFLOW,                // Arithmetic overflow detected
    CR_ERROR_INVALID_DENOMINATOR,     // Zero denominator in rational conversion
    CR_ERROR_TUPLE_BOUNDS             // Tuple array bounds exceeded
} CRErrorCode;
```

## Error Structure

```c
typedef struct {
    CRErrorCode code;                 // Error code
    char message[256];                // Human-readable error message
    int32_t value1;                   // Context-specific value
    int32_t value2;                   // Additional context value
} CRError;
```

## Updated Function Signatures

All functions that can produce errors now accept an optional `CRError*` parameter:

```c
CompactRational cr_from_int(int32_t value, CRError* error);
CompactRational cr_from_fraction(int32_t num, int32_t denom, CRError* error);
CompactRational cr_add(const CompactRational* a, const CompactRational* b, CRError* error);
double cr_to_double(const CompactRational* cr, CRError* error);
```

## Usage Examples

### Example 1: Ignoring Errors (Backward Compatible)

Pass `NULL` for the error parameter to maintain old behavior:

```c
CompactRational cr = cr_from_int(42, NULL);  // No error checking
CompactRational sum = cr_add(&a, &b, NULL);  // No error checking
```

### Example 2: Basic Error Checking

Check if an error occurred:

```c
CRError error;
CompactRational cr = cr_from_int(20000, &error);

if (error.code != CR_SUCCESS) {
    printf("Error: %s\n", error.message);
    // Handle error...
}
```

### Example 3: Getting Error Details

Access context values to understand what happened:

```c
CRError error;
CompactRational cr = cr_from_int(20000, &error);

if (error.code == CR_ERROR_VALUE_CLAMPED) {
    printf("Value %d was clamped to %d\n",
           error.value1,    // Original value (20000)
           error.value2);   // Limit it was clamped to (16383)
}
```

### Example 4: Handling Division by Zero

```c
CRError error;
CompactRational cr = cr_from_fraction(5, 0, &error);

if (error.code == CR_ERROR_DIVISION_BY_ZERO) {
    printf("Error: %s\n", error.message);
    printf("Attempted: %d / %d\n", error.value1, error.value2);
    // Handle the error appropriately
    return DEFAULT_VALUE;
}
```

### Example 5: Overflow Detection

```c
CRError error;
CompactRational result = cr_add(&large_a, &large_b, &error);

if (error.code == CR_ERROR_OVERFLOW) {
    printf("Overflow detected: %s\n", error.message);
    // Handle overflow...
}
```

### Example 6: Chained Operations with Error Checking

```c
CRError error;

CompactRational a = cr_from_fraction(15, 2, &error);
if (error.code != CR_SUCCESS) {
    fprintf(stderr, "Error creating first value: %s\n", error.message);
    return;
}

CompactRational b = cr_from_fraction(25, 3, &error);
if (error.code != CR_SUCCESS) {
    fprintf(stderr, "Error creating second value: %s\n", error.message);
    return;
}

CompactRational sum = cr_add(&a, &b, &error);
if (error.code != CR_SUCCESS) {
    fprintf(stderr, "Error in addition: %s\n", error.message);
    return;
}

// All operations succeeded
printf("Result: ");
cr_print(&sum);
```

### Example 7: Validation Function

Create a helper to validate operations:

```c
bool validate_cr_operation(const CRError* error, const char* operation) {
    if (error->code != CR_SUCCESS) {
        fprintf(stderr, "Error in %s: %s\n", operation, error->message);
        if (error->value1 != 0 || error->value2 != 0) {
            fprintf(stderr, "  Context: value1=%d, value2=%d\n",
                    error->value1, error->value2);
        }
        return false;
    }
    return true;
}

// Usage:
CRError error;
CompactRational cr = cr_from_int(value, &error);
if (!validate_cr_operation(&error, "cr_from_int")) {
    return ERROR_CODE;
}
```

## Error Context Values

The `value1` and `value2` fields provide context-specific information:

### CR_ERROR_VALUE_CLAMPED
- `value1`: Original value that was clamped
- `value2`: The limit it was clamped to (MAX_WHOLE_VALUE or MIN_WHOLE_VALUE)

### CR_ERROR_DIVISION_BY_ZERO
- `value1`: Numerator
- `value2`: Denominator (0)

### CR_ERROR_OVERFLOW
- `value1`: Truncated numerator value
- `value2`: Truncated denominator value

### CR_SUCCESS
- `value1`: 0
- `value2`: 0

## Testing

A comprehensive test suite is available in `test_error_reporting.c`:

```bash
gcc -Wall -Wextra -o test_error_reporting test_error_reporting.c compact_rational_lib.c -lm
./test_error_reporting
```

This demonstrates all error conditions and how to handle them.

## Migration Guide

### For Existing Code

No changes required! All existing code continues to work:

```c
// Old code still works:
CompactRational cr = cr_from_int(42);  // ❌ Compiler error: missing parameter

// Update to:
CompactRational cr = cr_from_int(42, NULL);  // ✅ Works, same behavior as before
```

### For New Code

Take advantage of error reporting:

```c
CRError error;
CompactRational cr = cr_from_int(value, &error);

if (error.code != CR_SUCCESS) {
    // Handle error programmatically
    switch (error.code) {
        case CR_ERROR_VALUE_CLAMPED:
            log_warning("Value clamped: %s", error.message);
            break;
        case CR_ERROR_DIVISION_BY_ZERO:
            log_error("Division by zero: %s", error.message);
            return NULL;
        default:
            log_error("Unexpected error: %s", error.message);
            return NULL;
    }
}
```

## Benefits

1. **Programmatic Error Handling**: No need to parse stderr output
2. **Detailed Context**: Get the actual values that caused the error
3. **Type Safety**: Specific error codes instead of string parsing
4. **Backward Compatible**: Existing code works unchanged (with NULL parameter)
5. **Better Testing**: Can verify error conditions programmatically
6. **Library-Friendly**: Suitable for use in larger applications

## See Also

- `test_error_reporting.c` - Comprehensive test examples
- `compact_rational.h` - Complete API documentation
- `ERROR_HANDLING_ANALYSIS.md` - Comparison with previous error handling approach
