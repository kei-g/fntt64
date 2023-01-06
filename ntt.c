#include "ntt.h"

#ifdef _DEBUG
  #include <assert.h>
  #define ASSERT assert
#else
  #define ASSERT(expr)
#endif

#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  uint64_t (*proc)(uint64_t, int);
  int remaining_bits;
} ntt_elem_t;

static uint64_t add_mod_p(uint64_t a, uint64_t b);
static void carry_forward(uint64_t *restrict dst, const uint64_t *restrict src);
static uint64_t divide(uint64_t *restrict dst, const uint64_t *restrict src);
static void init(ntt_elem_t *forwardNTT, ntt_elem_t *inverseNTT, uint64_t *t);
static void join(uint64_t *restrict dst, const uint64_t *restrict src);
static uint64_t mul_mod_p(uint64_t a, uint64_t b);
static uint64_t mul_high_mod_p(uint64_t a, int b);
static uint64_t mul_high_neg_mod_p(uint64_t a, int b);
static uint64_t mul_low_mod_p(uint64_t a, int b);
static uint64_t mul_low_neg_mod_p(uint64_t a, int b);
static uint64_t mul_mid_mod_p(uint64_t a, int b);
static uint64_t mul_mid_neg_mod_p(uint64_t a, int b);
static uint64_t negate_mod_p(uint64_t a, int b);
static void ntt(uint64_t *restrict dst, const uint64_t *restrict src,
                const ntt_elem_t elem[], const uint64_t t[]);
static uint64_t return_1st_arg(uint64_t a, int b);
static uint64_t reverse_bits(uint64_t a);
static uint64_t sub_mod_p(uint128_u *a, uint64_t b);

#ifndef countof
  #define countof(arr) (sizeof(arr) / sizeof(arr[0]))
#endif /* countof */

int main(int argc, const char *argv[]) {
  fputs("[src]\n", stdout);
  uint64_t src[30] = {0};
  for (int i = 0; i + 1 < argc && i < (int)countof(src); i++) {
    char *ep;
    src[i] = strtoul(argv[i + 1], &ep, 10);
    src[14] &= ~(14ul << 60);
    src[29] &= 0xffffffff;
    printf("%16lx%c", src[i], (i & 3) == 3 ? '\n' : ' ');
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
  f[32] = divide(f, src);
#endif
  fputs("[f]\n", stdout);
  for (int i = 0; i < 64; i++)
    printf("%16lx%c", f[i], (i & 3) == 3 ? '\n' : ' ');
  putchar('\n');

#ifdef _TEST
  uint64_t g[64] = {5, 4, 6, 2};
#else
  uint64_t g[64];
  divide(g, src + 15);
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

  mpz_t f2, g2, fg2;
  mpz_roinit_n(f2, src, 15);
  mpz_roinit_n(g2, src + 15, 15);
  mpz_init(fg2);
  mpz_mul(fg2, f2, g2);
  const mp_limb_t *fgp = mpz_limbs_read(fg2);
  size_t len = mpz_size(fg2);
  fputs("[GMP]\n", stdout);
  for (size_t i = 0; i < len; i++)
    printf("%16zx%c", fgp[i], (i & 3) == 3 ? '\n' : ' ');
  if (len & 3)
    putchar('\n');
  putchar('\n');

#ifdef _TEST
#else
  uint64_t result[30];
  fputs("[result]\n", stdout);
  join(result, fg);
  for (size_t i = 0; i < countof(result); i++) {
    if (result[i] != fgp[i])
      fputs("\x1b[31m", stdout);
    printf("%16lx\x1b[m%c", result[i], (i & 3) == 3 ? '\n' : ' ');
  }
  putchar('\n');
#endif

  return 0;
}

static uint64_t add_mod_p(uint64_t a, uint64_t b) {
  uint128_u c = {.lo = a};
  c.value += b;
  return mod_p(&c);
}

static void carry_forward(uint64_t *restrict dst,
                          const uint64_t *restrict src) {
  uint64_t carry = 0;
  for (int i = 0; i < 64; i++) {
    uint128_u x = {.lo = src[i]};
    x.value += carry;
    carry = (uint64_t)(x.value >> 29);
    dst[i] = x.lo & 0x1fffffff;
  }
  dst[64] = carry;
}

static uint64_t divide(uint64_t *restrict dst, const uint64_t *restrict src) {
  for (int i = 0, j = 0, k = 0; i < 32; i++) {
    const int a = k + 29;
    if (a < 64) {
      dst[i] = (src[j] >> k) & 0x1fffffff;
      k = a;
    }
    else {
      const int b = 64 - k;
      dst[i] = src[j++] >> k;
      dst[i] |= (src[j] << b) & 0x1fffffff;
      k = 29 - b;
    }
  }
  for (int i = 32; i < 64; i++)
    dst[i] = 0;
  return (src[14] >> 32) & 0x1fffffff;
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

static void join(uint64_t *restrict dst, const uint64_t *restrict src) {
  uint64_t tmp[65];
  carry_forward(tmp, src);
  for (uint8_t i = 0, j = 0, k = 0; i < 29; i++, j--, k = 93 - k) {
    dst[i] = tmp[j++] >> k;
    for (k = 29 - k; k < 64; j++, k += 29)
      dst[i] |= tmp[j] << k;
  }
  dst[29] = tmp[64];
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

static void ntt(uint64_t *restrict dst, const uint64_t *restrict src,
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
