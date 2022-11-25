#ifndef include_ntt_h
#define include_ntt_h

#include <stdint.h>

typedef union {
  uint32_t data[4];
  struct {
    uint64_t lo;
    uint64_t hi;
  };
  __uint128_t value;
} uint128_u;

uint64_t mod_p(uint128_u *x);

#endif /* include_ntt_h */
