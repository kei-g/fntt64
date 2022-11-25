#ifdef CONFIRM
  #include <stdio.h>
  #include <stdlib.h>
#endif

#include "ntt.h"

uint64_t mod_p(uint128_u *x) {
#ifdef USE_UDIV3
  x->value %= 0xffffffff00000001ul;
  return x->lo;
#endif
#ifdef CONFIRM
  const uint64_t expected = x->value % 0xffffffff00000001ul;
#endif /* CONFIRM */
  uint64_t d0 = x->data[0];
  uint64_t d2 = x->data[2];
  d2 += x->data[3];
  if (d0 < d2)
    d0 += 0xffffffff00000001ul;
  d0 -= d2;
  uint64_t d1 = x->data[1];
  d1 += x->data[2];
  uint128_u d = {.lo = d1};
  d.value <<= 32;
  d.value += d0;
  if (0xffffffff00000001ul <= d.value)
    d.value -= 0xffffffff00000001ul;
  if (0xffffffff00000001ul <= d.value)
    d.value -= 0xffffffff00000001ul;
  x->lo = d.lo;
#ifdef CONFIRM
  if (x->lo != expected) {
    printf("%lx should be %lx\n", x->lo, expected);
    exit(0);
  }
#endif /* CONFIRM */
  return x->lo;
}
