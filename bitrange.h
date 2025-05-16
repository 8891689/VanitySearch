/* bitrange.h */
#ifndef BITRANGE_H
#define BITRANGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIMBS 8  // 8 × 32 = 256 bits

// BigUInt: a 256-bit unsigned integer stored as an array of eight 32-bit limbs
typedef struct {
    uint32_t limbs[LIMBS];
} BigUInt;

/**
 * Computes [min_out, max_out] based on bit count:
 *   bits ∈ [1, 256]
 *   min_out = 2^(bits-1)
 *   max_out = 2^bits - 1
 * @param bits     number of bits
 * @param min_out  output pointer for minimum
 * @param max_out  output pointer for maximum
 * @return 0 on success, -1 on invalid bits
 */
int set_bitrange(int bits, BigUInt *min_out, BigUInt *max_out);

/**
 * Parses hex range A:B → [min_out, max_out]:
 *   param format "A:B" (hex strings, no "0x")
 * @param param    input string "A:B"
 * @param min_out  output pointer for A
 * @param max_out  output pointer for B
 * @return 0 on success, -1 on format or parse error
 */
int set_range(const char *param, BigUInt *min_out, BigUInt *max_out);

#ifdef __cplusplus
}
#endif

#endif // BITRANGE_H

