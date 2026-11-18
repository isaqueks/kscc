// Builtins de ponto flutuante (uma instrução F cada, sem chamada) e as
// funções de <math.h> da libks (as mesmas pelo nome entre parênteses,
// round, fmod, copysign, fpclassify).
#include <math.h>
#include "float_test.h"

float g_inf = INFINITY, g_nan = NAN, g_huge = HUGE_VAL;  // constantes

int main() {
  float n = bitsf(F_NAN), inf = bitsf(F_INF), nz = rt(-0.0f);
  EQB(0x3FB504F3, sqrtf(rt(2)));
  EQB(0x40400000, sqrt(rt(9)));
  EQB(F_NZERO, sqrtf(nz));            // sqrt(-0) = -0
  EQB(F_NAN, sqrtf(rt(-1)));
  EQB(F_INF, sqrtl(inf));
  EQB(0x3F800000, fabsf(rt(-1)));
  EQB(0x00000000, fabs(nz));
  EQB(F_NAN, fabsf(bitsf(0xFFC00000)));  // só o sinal
  EQB(0x40000000, floorf(rt(2.7f)));
  EQB(0xC0400000, floorf(rt(-2.1f)));
  EQB(0xBF800000, floor(rt(-0.5f)));
  EQB(0x40400000, ceilf(rt(2.1f)));
  EQB(F_NZERO, ceilf(rt(-0.5f)));     // ceil(-0.5) = -0
  EQB(0xC0000000, truncf(rt(-2.9f)));
  EQB(F_NZERO, truncf(rt(-0.7f)));
  EQB(0x40000000, rintf(rt(2.5f)));   // empate: para o par
  EQB(0x40800000, rintf(rt(3.5f)));
  EQB(F_NZERO, rintf(rt(-0.5f)));
  EQB(0x40000000, nearbyintf(rt(1.5f)));
  EQB(0x4B000001, rintf(rt(8388609.0f))); // >= 2^23 já é inteiro
  EQB(0x3F800000, fminf(rt(1), rt(2)));
  EQB(0x40000000, fmaxf(rt(1), rt(2)));
  EQB(0x3F800000, fminf(n, rt(1)));    // um NaN: o outro operando
  EQB(0x3F800000, fmax(rt(1), n));
  EQB(F_NAN, fminf(n, n));
  EQB(F_NZERO, fminf(rt(0), nz));      // fmin(+0, -0) = -0
  EQB(0x00000000, fmaxf(nz, rt(0)));
  EQB(0xBF800000, fminl(rt(-1), inf));

  EQ(1, isnan(n)); EQ(0, isnan(inf)); EQ(0, isnan(rt(1))); EQ(1, isnan(rt(0) / 0));
  EQ(1, isinf(inf)); EQ(1, isinf(-inf)); EQ(0, isinf(n)); EQ(0, isinf(rt(3e38f)));
  EQ(1, isfinite(rt(3e38f))); EQ(0, isfinite(inf)); EQ(0, isfinite(n)); EQ(1, isfinite(bitsf(1)));
  EQ(1, signbit(nz)); EQ(0, signbit(rt(0))); EQ(1, signbit(bitsf(0xFFC00000))); EQ(1, signbit(rt(-2)));
  EQ(0, signbit(n));
  EQ(0, isnan(5));                    // argumento inteiro vira float
  EQ(FP_NAN, fpclassify(n)); EQ(FP_INFINITE, fpclassify(inf)); EQ(FP_ZERO, fpclassify(nz));
  EQ(FP_SUBNORMAL, fpclassify(bitsf(5))); EQ(FP_NORMAL, fpclassify(rt(1e-30f)));
  EQ(1, isnormal(rt(1))); EQ(0, isnormal(bitsf(5)));
  EQ(1, isunordered(n, rt(1))); EQ(0, isunordered(rt(1), rt(2)));
  EQ(1, islessgreater(rt(1), rt(2))); EQ(0, islessgreater(n, rt(1)));
  EQ(1, isgreater(rt(2), rt(1))); EQ(0, isless(n, rt(1)));
  EQB(F_INF, g_inf); EQB(F_NAN, g_nan); EQB(F_INF, g_huge);
  EQB(F_INF, HUGE_VALF); EQB(F_NAN, __builtin_nan("")); EQB(F_INF, __builtin_infl());

  // as funções de verdade (nome entre parênteses ou por ponteiro)
  float (*fs[])(float) = {sqrtf, fabsf, floorf, ceilf, truncf, rintf, roundf};
  EQB(0x40000000, fs[0](4));
  EQB(0x40000000, (fabsf)(-2));
  EQB(0x40000000, (floorf)(2.5f));
  EQB(0x40400000, (ceilf)(2.5f));
  EQB(0x40000000, (truncf)(2.5f));
  EQB(0x40000000, (rintf)(2.5f));
  EQB(0x40400000, fs[6](2.5f));       // round: empate para longe de zero
  EQB(0x3F800000, (fminf)(1, 2));
  EQB(0x40000000, (fmaxf)(n, 2));
  EQB(0x40400000, (sqrt)(9));

  // round
  EQB(0x40400000, roundf(rt(2.5f)));
  EQB(0xC0400000, roundf(rt(-2.5f)));
  EQB(0x00000000, roundf(rt(0.49999997f)));  // floor(x + 0.5) daria 1
  EQB(F_NZERO, roundf(rt(-0.3f)));
  EQB(0xBF800000, round(rt(-0.5f)));
  EQB(0x4B000001, roundf(rt(8388609.0f)));
  EQB(F_NAN, roundf(n));
  EQB(F_NINF, roundl(-inf));

  // fmod: exato, com o sinal de x
  EQB(0x3F800000, fmodf(rt(7), rt(2)));
  EQB(0xBF800000, fmodf(rt(-7), rt(2)));
  EQB(0x3F800000, fmodf(rt(7), rt(-2)));
  EQB(0x3F000000, fmodf(rt(5.5f), rt(1)));
  EQB(fbits(rt(3.1f) - 3), fmodf(rt(3.1f), rt(1.0f)));  // exato
  EQB(0x00000000, fmodf(rt(6), rt(3)));
  EQB(F_NZERO, fmodf(rt(-6), rt(3)));
  EQB(0x40000000, fmodf(rt(2), inf));
  EQB(0x3F000000, fmodf(rt(0.5f), rt(3)));  // |x| < |y|: o próprio x
  EQB(F_NAN, fmodf(rt(1), rt(0)));
  EQB(F_NAN, fmodf(inf, rt(1)));
  EQB(F_NAN, fmod(n, rt(1)));
  EQB(F_NAN, fmodf(rt(1), n));
  EQB(0x00000001, fmodf(bitsf(0x00800001), bitsf(0x00000002)));  // subnormais
  EQB(0x00000000, fmodf(rt(1e20f), rt(0.5f)));
  EQB(0x3F800000, fmodf(rt(16777216.0f), rt(3)));   // 2^24 mod 3 = 1
  EQB(0x3F000000, fmodf(rt(1e20f), rt(1.5f)) - fmodf(rt(1e20f), rt(1.5f)) + 0.5f);

  // copysign
  EQB(0xC0000000, copysignf(rt(2), rt(-0.0f)));
  EQB(0x40000000, copysign(rt(-2), rt(1)));
  EQB(0xFFC00000, copysignf(n, rt(-1)));
  return 0;
}
