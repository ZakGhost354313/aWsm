#include "../runtime.h"

__attribute__((noreturn)) __attribute__((always_inline)) void awsm_abi__trap_unreachable() {
    fprintf(stderr, "WebAssembly control flow unexpectedly reached unreachable instruction\n");
    exit(EXIT_FAILURE);
}

/* Instantiation */

// If we are using runtime globals, we need to populate them
WEAK void populate_globals() {}

/* End of Instantiation */


/* Memory Instructions:*/

INLINE i32 instruction_memory_size() {
    return memory_size / WASM_PAGE_SIZE;
}

INLINE i32 instruction_memory_grow(i32 count) {
    i32 prev_size = instruction_memory_size();
    for (int i = 0; i < count; i++) {
        expand_memory();
    }

    return prev_size;
}

INLINE void initialize_region(u32 offset, u32 data_count, char* data) {
    // FIXME: Hack around segmented and unsegmented access
    memcpy(get_memory_ptr_for_runtime(offset, data_count), data, data_count);
}

/* End of Memory Instructions:*/


/* Table Instructions:*/

INLINE void add_function_to_table(u32 idx, u32 type_id, char* pointer) {
    awsm_assert(idx < INDIRECT_TABLE_SIZE);
    indirect_table[idx] = (struct indirect_table_entry){ .type_id = type_id, .func_pointer = pointer };
}

INLINE void clear_table() {
    for (int i = 0; i < INDIRECT_TABLE_SIZE; i++) {
        indirect_table[i] = (struct indirect_table_entry){ 0 };
    }
}

/* End of Table Instructions:*/


/* Numeric Instructions:*/

INLINE u32 rotl_u32(u32 n, u32 c_u32) {
    // WASM requires a modulus here (usually a single bitwise op, but it means we need no assert)
    unsigned int       c    = c_u32 % (CHAR_BIT * sizeof(n));
    const unsigned int mask = (CHAR_BIT * sizeof(n) - 1); // assumes width is a power of 2.

    c &= mask;
    return (n << c) | (n >> ((-c) & mask));
}

INLINE u32 rotr_u32(u32 n, u32 c_u32) {
    // WASM requires a modulus here (usually a single bitwise op, but it means we need no assert)
    unsigned int       c    = c_u32 % (CHAR_BIT * sizeof(n));
    const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);

    c &= mask;
    return (n >> c) | (n << ((-c) & mask));
}

INLINE u64 rotl_u64(u64 n, u64 c_u64) {
    // WASM requires a modulus here (usually a single bitwise op, but it means we need no assert)
    unsigned int       c    = c_u64 % (CHAR_BIT * sizeof(n));
    const unsigned int mask = (CHAR_BIT * sizeof(n) - 1); // assumes width is a power of 2.

    c &= mask;
    return (n << c) | (n >> ((-c) & mask));
}

INLINE u64 rotr_u64(u64 n, u64 c_u64) {
    // WASM requires a modulus here (usually a single bitwise op, but it means we need no assert)
    unsigned int       c    = c_u64 % (CHAR_BIT * sizeof(n));
    const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);

    c &= mask;
    return (n >> c) | (n << ((-c) & mask));
}

// Now safe division and remainder
INLINE u32 u32_div(u32 a, u32 b) {
    awsm_assert(b);
    return a / b;
}

INLINE u32 u32_rem(u32 a, u32 b) {
    awsm_assert(b);
    return a % b;
}

INLINE i32 i32_div(i32 a, i32 b) {
    awsm_assert(b && (a != INT32_MIN || b != -1));
    return a / b;
}

INLINE i32 i32_rem(i32 a, i32 b) {
    awsm_assert(b != 0);

    /* Because MIN is one less than MAX, we can FPE here */
    if (unlikely(a == INT32_MIN)) {
        return (a + abs(b)) % b;
    }

    return a % b;
}

INLINE u64 u64_div(u64 a, u64 b) {
    awsm_assert(b);
    return a / b;
}

INLINE u64 u64_rem(u64 a, u64 b) {
    awsm_assert(b);
    return a % b;
}

INLINE i64 i64_div(i64 a, i64 b) {
    awsm_assert(b && (a != INT64_MIN || b != -1));
    return a / b;
}

INLINE i64 i64_rem(i64 a, i64 b) {
    awsm_assert(b != 0);

    /* Because MIN is one less than MAX, we can FPE here */
    if (unlikely(a == INT64_MIN)) {
        return (a + labs(b)) % b;
    }

    return a % b;
}

// float to integer conversion methods
// In C, float => int conversions always truncate
// If a int2float(int::min_value) <= float <= int2float(int::max_value), it must always be safe to truncate it
u32 u32_trunc_f32(float f) {
    float integer_part = 0;
    float decimal_part = modff(f, &integer_part);

    if (unlikely(integer_part < 0 || integer_part > (float)UINT32_MAX)) {
        fprintf(stderr, "integer overflow\n");
        awsm_assert(0);
    }

    return (u32)f;
}

i32 i32_trunc_f32(float f) {
    float integer_part = 0;
    float decimal_part = modff(f, &integer_part);

    if (unlikely(integer_part < (float)INT32_MIN || integer_part > (float)INT32_MAX)) {
        fprintf(stderr, "integer overflow\n");
        awsm_assert(0);
    }

    return (i32)f;
}

u32 u32_trunc_f64(double f) {
    double integer_part = 0;
    double decimal_part = modf(f, &integer_part);

    if (unlikely(integer_part < 0 || integer_part > (double)UINT32_MAX)) {
        fprintf(stderr, "integer overflow\n");
        awsm_assert(0);
    }
    return (u32)f;
}

i32 i32_trunc_f64(double f) {
    double integer_part = 0;
    double decimal_part = modf(f, &integer_part);

    if (unlikely(integer_part < (double)INT32_MIN || integer_part > (double)INT32_MAX)) {
        fprintf(stderr, "integer overflow\n");
        awsm_assert(0);
    }

    return (i32)f;
}

u64 u64_trunc_f32(float f) {
    float integer_part = 0;
    float decimal_part = modff(f, &integer_part);

    if (unlikely(integer_part < 0 || integer_part > (float)UINT64_MAX)) {
        fprintf(stderr, "integer overflow\n");
        awsm_assert(0);
    }

    return (u64)f;
}

i64 i64_trunc_f32(float f) {
    float integer_part = 0;
    float decimal_part = modff(f, &integer_part);

    if (unlikely(integer_part < (float)INT64_MIN || integer_part > (float)INT64_MAX)) {
        fprintf(stderr, "integer overflow\n");
        awsm_assert(0);
    }

    return (i64)f;
}

u64 u64_trunc_f64(double f) {
    double integer_part = 0;
    double decimal_part = modf(f, &integer_part);

    if (unlikely(integer_part < 0 || integer_part > (double)UINT64_MAX)) {
        fprintf(stderr, "integer overflow\n");
        awsm_assert(0);
    }

    return (u64)f;
}

i64 i64_trunc_f64(double f) {
    double integer_part = 0;
    double decimal_part = modf(f, &integer_part);

    if (unlikely(integer_part < (double)INT64_MIN || integer_part > (double)INT64_MAX)) {
        fprintf(stderr, "integer overflow\n");
        awsm_assert(0);
    }

    return (i64)f;
}

// Float => Float truncation functions
INLINE float f32_trunc_f32(float f) {
    return truncf(f);
}

INLINE float f32_min(float a, float b) {
    return a < b ? a : b;
}

INLINE float f32_max(float a, float b) {
    return a > b ? a : b;
}

INLINE float f32_floor(float a) {
    return floorf(a);
}

INLINE float f32_ceil(float a) {
    return ceilf(a);
}

INLINE float f32_nearest(float a) {
    return nearbyintf(a);
}

INLINE float f32_copysign(float a, float b) {
    return copysignf(a, b);
}

INLINE double f64_trunc_f64(double f) {
    return trunc(f);
}

INLINE double f64_min(double a, double b) {
    return a < b ? a : b;
}

INLINE double f64_max(double a, double b) {
    return a > b ? a : b;
}

INLINE double f64_floor(double a) {
    return floor(a);
}

INLINE double f64_ceil(double a) {
    return ceil(a);
}

INLINE double f64_nearest(double a) {
    return nearbyint(a);
}

INLINE double f64_copysign(double a, double b) {
    return copysign(a, b);
}

/* End of Numeric Instructions */
