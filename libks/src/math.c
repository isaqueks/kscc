// <math.h> da libks (doc 05 §2): as funções de verdade por trás das
// macros (para quem pega o endereço ou escreve `(sqrtf)(x)`), round, fmod,
// copysign e fpclassify.
//
// float, double e long double são todos binary32, então as três versões
// de cada função são iguais. Os nomes vão entre parênteses para que as
// macros de <math.h> não os expandam.
#include <math.h>

typedef union {
  float f;
  unsigned u;
} Bits;

static unsigned to_bits(float x) {
  Bits b;
  b.f = x;
  return b.u;
}

static float from_bits(unsigned u) {
  Bits b;
  b.u = u;
  return b.f;
}

float (sqrtf)(float x) { return __builtin_sqrtf(x); }
double (sqrt)(double x) { return __builtin_sqrt(x); }
long double (sqrtl)(long double x) { return __builtin_sqrtl(x); }

float (fabsf)(float x) { return __builtin_fabsf(x); }
double (fabs)(double x) { return __builtin_fabs(x); }
long double (fabsl)(long double x) { return __builtin_fabsl(x); }

float (floorf)(float x) { return __builtin_floorf(x); }
double (floor)(double x) { return __builtin_floor(x); }
long double (floorl)(long double x) { return __builtin_floorl(x); }

float (ceilf)(float x) { return __builtin_ceilf(x); }
double (ceil)(double x) { return __builtin_ceil(x); }
long double (ceill)(long double x) { return __builtin_ceill(x); }

float (truncf)(float x) { return __builtin_truncf(x); }
double (trunc)(double x) { return __builtin_trunc(x); }
long double (truncl)(long double x) { return __builtin_truncl(x); }

float (rintf)(float x) { return __builtin_rintf(x); }
double (rint)(double x) { return __builtin_rint(x); }
long double (rintl)(long double x) { return __builtin_rintl(x); }

float (nearbyintf)(float x) { return __builtin_rintf(x); }
double (nearbyint)(double x) { return __builtin_rint(x); }
long double (nearbyintl)(long double x) { return __builtin_rintl(x); }

float (fminf)(float x, float y) { return __builtin_fminf(x, y); }
double (fmin)(double x, double y) { return __builtin_fmin(x, y); }
long double (fminl)(long double x, long double y) { return __builtin_fminl(x, y); }

float (fmaxf)(float x, float y) { return __builtin_fmaxf(x, y); }
double (fmax)(double x, double y) { return __builtin_fmax(x, y); }
long double (fmaxl)(long double x, long double y) { return __builtin_fmaxl(x, y); }

// round: o inteiro mais próximo, empate para longe de zero. x - trunc(x)
// é exato (a parte fracionária de um binary32 é representável), então a
// comparação com 0.5 não erra perto do empate (o clássico floor(x + 0.5)
// dá 1 para 0.49999997). NaN e ±inf passam por trunc e voltam como estão
// (NaN canônico); o sinal de zero é preservado (round(-0.3) = -0).
float (roundf)(float x) {
  float t = __builtin_truncf(x);
  if (__builtin_fabsf(x - t) >= 0.5f)
    t += (x < 0) ? -1.0f : 1.0f;
  return t;
}

double (round)(double x) { return (roundf)(x); }
long double (roundl)(long double x) { return (roundf)(x); }

float (copysignf)(float x, float y) {
  return from_bits((to_bits(x) & 0x7FFFFFFF) | (to_bits(y) & 0x80000000));
}

double (copysign)(double x, double y) { return (copysignf)(x, y); }
long double (copysignl)(long double x, long double y) { return (copysignf)(x, y); }

// fmod: x - n*y com n = trunc(x/y), calculado exatamente (C11 7.12.10.1:
// o resultado é sempre representável). Só aritmética inteira sobre as
// mantissas: alinha o expoente de x ao de y subtraindo y deslocado, como
// numa divisão longa binária (o algoritmo clássico das libms). O
// resultado tem o sinal de x; y == 0, x infinito ou NaN dão NaN.
float (fmodf)(float x, float y) {
  unsigned ux = to_bits(x), uy = to_bits(y);
  unsigned sx = ux & 0x80000000;
  unsigned ax = ux & 0x7FFFFFFF, ay = uy & 0x7FFFFFFF;

  if (ay == 0 || ay > 0x7F800000 || ax >= 0x7F800000)
    return from_bits(0x7FC00000);
  if (ax < ay)
    return x;              // |x| < |y|: o próprio x (inclusive ±0)
  if (ax == ay)
    return from_bits(sx);  // ±0 com o sinal de x

  // Mantissas inteiras com o bit implícito e expoentes do bit 0.
  int ex = ax >> 23, ey = ay >> 23;
  unsigned mx = ax & 0x7FFFFF, my = ay & 0x7FFFFF;
  if (ex)
    mx |= 0x800000;
  else
    ex = 1;
  if (ey)
    my |= 0x800000;
  else
    ey = 1;
  // Normaliza os subnormais: bit mais alto na posição 23.
  while (mx < 0x800000) {
    mx <<= 1;
    ex--;
  }
  while (my < 0x800000) {
    my <<= 1;
    ey--;
  }

  // mx < 2^24 e my < 2^24: mx - my cabe e, depois de dobrar, mx < 2^25.
  for (; ex > ey; ex--) {
    if (mx >= my)
      mx -= my;
    mx <<= 1;
  }
  if (mx >= my)
    mx -= my;
  if (mx == 0)
    return from_bits(sx);

  // Renormaliza o resto (menor que my) e reempacota.
  while (mx < 0x800000) {
    mx <<= 1;
    ey--;
  }
  if (ey >= 1)
    return from_bits(sx | ((unsigned)ey << 23) | (mx & 0x7FFFFF));
  // Subnormal: o resto é exato, então o deslocamento não perde bits.
  return from_bits(sx | (mx >> (1 - ey)));
}

double (fmod)(double x, double y) { return (fmodf)(x, y); }
long double (fmodl)(long double x, long double y) { return (fmodf)(x, y); }

int __ks_fpclassify(float x) {
  unsigned a = to_bits(x) & 0x7FFFFFFF;
  if (a == 0)
    return FP_ZERO;
  if (a < 0x800000)
    return FP_SUBNORMAL;
  if (a < 0x7F800000)
    return FP_NORMAL;
  return a == 0x7F800000 ? FP_INFINITE : FP_NAN;
}
