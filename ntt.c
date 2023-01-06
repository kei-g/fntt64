#include "ntt.h"

#ifdef _DEBUG
  #include <assert.h>
  #define ASSERT assert
#else
  #define ASSERT(expr)
#endif

#include <stdio.h>
#include <stdlib.h>

typedef struct {
  uint64_t (*proc)(uint64_t, int);
  int remaining_bits;
} ntt_elem_t;

static uint64_t add_mod_p(uint64_t a, uint64_t b);
static void init(ntt_elem_t *forwardNTT, ntt_elem_t *inverseNTT, uint64_t *t);
static uint64_t mul_mod_p(uint64_t a, uint64_t b);
static uint64_t mul_high_mod_p(uint64_t a, int b);
static uint64_t mul_high_neg_mod_p(uint64_t a, int b);
static uint64_t mul_low_mod_p(uint64_t a, int b);
static uint64_t mul_low_neg_mod_p(uint64_t a, int b);
static uint64_t mul_mid_mod_p(uint64_t a, int b);
static uint64_t mul_mid_neg_mod_p(uint64_t a, int b);
static uint64_t negate_mod_p(uint64_t a, int b);
static void ntt(uint64_t *__restrict dst, const uint64_t *__restrict src,
                const ntt_elem_t elem[], const uint64_t t[]);
static void pack(uint64_t *__restrict dst, const uint64_t *__restrict src);
static uint64_t return_1st_arg(uint64_t a, int b);
static uint64_t reverse_bits(uint64_t a);
static uint64_t sub_mod_p(uint128_u *a, uint64_t b);
static void unpack(uint64_t *__restrict dst, const uint64_t *__restrict src);

int main(int argc, const char *argv[]) {
  fputs("[src]\n", stdout);
  uint64_t src[126] = {0};
  for (int i = 1; i < argc; i++) {
    char *ep;
    src[i - 1] = strtoul(argv[i], &ep, 10);
    printf("%16lx%c", src[i - 1], ((i - 1) & 3) == 3 ? '\n' : ' ');
  }
  putchar('\n');
  putchar('\n');

  ntt_elem_t forwardNTT[64], inverseNTT[64];
  uint64_t t[64];
  init(forwardNTT, inverseNTT, t);

#ifdef _TEST
  uint64_t f[64] = {3, 6, 9, 5};
#else
  uint64_t f[64];
  pack(f, src);
#endif
  fputs("[f]\n", stdout);
  for (int i = 0; i < 64; i++)
    printf("%16lx%c", f[i], (i & 3) == 3 ? '\n' : ' ');
  putchar('\n');

#ifdef _TEST
  uint64_t g[64] = {5, 4, 6, 2};
#else
  uint64_t g[64];
  pack(g, src + 63);
#endif
  fputs("[g]\n", stdout);
  for (int i = 0; i < 64; i++)
    printf("%16lx%c", g[i], (i & 3) == 3 ? '\n' : ' ');
  putchar('\n');

  uint64_t F[64];
  ntt(F, f, forwardNTT, t);
  fputs("[F]\n", stdout);
  for (int i = 0; i < 64; i++)
    printf("%16lx%c", F[i], (i & 3) == 3 ? '\n' : ' ');
  putchar('\n');

  uint64_t G[64];
  ntt(G, g, forwardNTT, t);
  fputs("[G]\n", stdout);
  for (int i = 0; i < 64; i++)
    printf("%16lx%c", G[i], (i & 3) == 3 ? '\n' : ' ');
  putchar('\n');

  uint64_t FG[64];
  fputs("[FG]\n", stdout);
  for (int i = 0; i < 64; i++) {
    FG[i] = mul_mod_p(F[i], G[i]);
    printf("%16lx%c", FG[i], (i & 3) == 3 ? '\n' : ' ');
  }
  putchar('\n');

  uint64_t fg[64];
  ntt(fg, FG, inverseNTT, t);
  fputs("[fg]\n", stdout);
#ifdef _TEST
  uint64_t r = 0;
#endif
  for (int i = 0; i < 64; i++) {
    // 64x = 1 mod p => x = 18158513693329981441 = 0xfbffffff04000001
#ifdef _TEST
    fg[i] = mul_mod_p(fg[i], 0xfbffffff04000001ul);
    const uint64_t x = fg[i] + r;
    r = x / 10;
    printf("%16lu%c", x % 10, (i & 3) == 3 ? '\n' : ' ');
#else
    fg[i] = mul_mod_p(fg[i], 0xfbffffff04000001ul);
    printf("%16lx%c", fg[i], (i & 3) == 3 ? '\n' : ' ');
#endif
  }
  putchar('\n');

#ifdef _TEST
#else
  uint64_t result[63];
  fputs("[result]\n", stdout);
  unpack(result, fg);
  for (int i = 0; i < 63; i++)
    printf("%16lx%c", result[i], (i & 3) == 3 ? '\n' : ' ');
  putchar('\n');
#endif

  return 0;
}

static uint64_t add_mod_p(uint64_t a, uint64_t b) {
  uint128_u c = {.lo = a};
  c.value += b;
  return mod_p(&c);
}

static void init(ntt_elem_t *forwardNTT, ntt_elem_t *inverseNTT, uint64_t *t) {
  uint64_t (*const mul[])(uint64_t, int) = {
    mul_low_mod_p,     mul_mid_mod_p,     mul_high_mod_p,
    mul_low_neg_mod_p, mul_mid_neg_mod_p, mul_high_neg_mod_p,
  };
  for (int i = 0, j = 39; i < 64; i++, j += 39) {
    forwardNTT[i].proc = mul[(j >> 5) % 6];
    forwardNTT[i].remaining_bits = j & 31;
    t[i] = reverse_bits(i);
  }
  forwardNTT[31].proc = negate_mod_p;
  forwardNTT[63].proc = return_1st_arg;
  for (int i = 0; i < 63; i++)
    inverseNTT[i] = forwardNTT[62 - i];
  inverseNTT[63] = forwardNTT[63];
}

static uint64_t mul_mod_p(uint64_t a, uint64_t b) {
  uint128_u x = {.lo = a};
  x.value *= b;
  return mod_p(&x);
}

static uint64_t mul_high_mod_p(uint64_t a, int b) {
  ASSERT(b > 0);
  // (a << b) * 2^{64} => 2^{32}-1
  // (a >> (32 - b)) * 2^{96} => -1
  // (a >> (64 - b)) * 2^{128} => -2^{32}
  const uint64_t c0 = (a << b) & 0xffffffff;
  const uint64_t c1 = (a >> (32 - b)) & 0xffffffff;
  const uint64_t c2 = a >> (64 - b);
  if (c0 < c2) {
    uint128_u c = {.lo = c2 - c0};
    c.value <<= 32;
    c.value += c0 + c1;
    return 0xffffffff00000001 - mod_p(&c);
  }
  else {
    uint128_u c = {.lo = c0 - c2};
    c.value <<= 32;
    return sub_mod_p(&c, c0 + c1);
  }
}

static uint64_t mul_high_neg_mod_p(uint64_t a, int b) {
  return 0xffffffff00000001 - mul_high_mod_p(a, b);
}

static uint64_t mul_low_mod_p(uint64_t a, int b) {
  ASSERT(b > 0);
  // (a << b)
  // (a >> (32 - b)) * 2^{32}
  // (a >> (64 - b)) * 2^{64} => 2^{32}-1
  const uint64_t c0 = (a << b) & 0xffffffff;
  const uint64_t c1 = (a >> (32 - b)) & 0xffffffff;
  const uint64_t c2 = a >> (64 - b);
  uint128_u c = {.lo = c1 + c2};
  c.value <<= 32;
  c.value += c0;
  return sub_mod_p(&c, c2);
}

static uint64_t mul_low_neg_mod_p(uint64_t a, int b) {
  return 0xffffffff00000001 - mul_low_mod_p(a, b);
}

static uint64_t mul_mid_mod_p(uint64_t a, int b) {
  ASSERT(b > 0);
  // (a << b) * 2^{32}
  // (a >> (32 - b)) * 2^{64} => 2^{32}-1
  // (a >> (64 - b)) * 2^{96} => -1
  const uint64_t c0 = (a << b) & 0xffffffff;
  const uint64_t c1 = (a >> (32 - b)) & 0xffffffff;
  const uint64_t c2 = a >> (64 - b);
  uint128_u c = {.lo = c0 + c1};
  c.value <<= 32;
  return sub_mod_p(&c, c1 + c2);
}

static uint64_t mul_mid_neg_mod_p(uint64_t a, int b) {
  return 0xffffffff00000001 - mul_mid_mod_p(a, b);
}

static uint64_t negate_mod_p(uint64_t a, int) {
  return 0xffffffff00000001 - a;
}

static void ntt(uint64_t *__restrict dst, const uint64_t *__restrict src,
                const ntt_elem_t elem[], const uint64_t t[]) {
  uint64_t tmp[64], *to[] = {dst, tmp, dst, tmp, dst, tmp};
  const uint64_t *from[] = {src, dst, tmp, dst, tmp, dst};
  for (int c = 0, d = 5, e = 32; c < 6; c++, d--, e >>= 1)
    for (int i = 0; i < 64; i++) {
      const uint64_t j = (t[i >> d] + 63) & 63;
      const ntt_elem_t *u = &elem[j];
      const int k = i & (63 ^ e);
      const uint64_t v = (*u->proc)(from[c][k + e], u->remaining_bits);
      to[c][i] = add_mod_p(from[c][k], v);
    }
  for (int i = 0; i < 64; i++)
    dst[i] = tmp[t[i]];
}

static void pack(uint64_t *__restrict dst, const uint64_t *__restrict src) {
  // pack from 63 64bits-integers to 64 63bits-integers
  dst[0] = src[0] >> 1;
  for (uint64_t i = 1, j = 63; i < 62; i++, j--)
    dst[i] = (src[i - 1] >> j) | ((src[i] << (i + 1)) & 0x7ffffffffffffffful);
  dst[62] = src[61] >> 1;
  dst[63] = src[62] & 0x7ffffffffffffffful;
}

static uint64_t return_1st_arg(uint64_t a, int) {
  return a;
}

static uint64_t reverse_bits(uint64_t a) {
  return ((a >> 5) & 1) | ((a >> 3) & 2) | ((a >> 1) & 4) | ((a << 1) & 8) |
         ((a << 3) & 16) | ((a << 5) & 32);
}

static uint64_t sub_mod_p(uint128_u *a, uint64_t b) {
  while (a->value < b)
    a->value += 0xffffffff00000001;
  a->value -= b;
  if (0xffffffff00000001 <= a->value)
    a->value -= 0xffffffff00000001;
  return a->lo;
}

/**
 * XXX: This implement may not work properly due to it not being tested.
 */
static void unpack(uint64_t *__restrict dst, const uint64_t *__restrict src) {
  // carry forward
  uint64_t carry = 0, tmp[65];
  for (int i = 0; i < 65; i++) {
    uint128_u x = {.lo = i >> 6 ? 0 : src[i]};
    x.value += carry;
    carry = (uint64_t)(x.value >> 63);
    tmp[i] = x.lo & 0x7fffffffffffffff;
  }

  // unpack from 64 63bits-integers to 63 64bits-integers
  for (long i = 0, j = 63, k = 0x7fffffffffffffff; i < 63; i++, j--, k >>= 1)
    dst[i] = (tmp[i] & k) | (tmp[i + 1] << j);
}
