// <math.h> da libks (doc 05 §2).
//
// float, double e long double são todos IEEE 754 binary32 (doc 02 §1), e
// as três versões de cada função (sqrt, sqrtf, sqrtl) fazem a mesma
// coisa. As que correspondem a uma instrução da extensão F da VM (sqrt,
// fabs, floor, ceil, trunc, rint, nearbyint, fmin, fmax) são macros para
// os builtins do kscc, sem chamada; as funções de verdade também existem
// (para `&sqrtf` ou `(sqrtf)(x)`). round e fmod são C sobre elas.
//
// Sem funções transcendentais (sin, cos, exp, log, pow...) nesta versão.
// Os resultados não dependem de modo de arredondamento (só existe RNE) e
// não há exceções de ponto flutuante nem errno (math_errhandling 0).
#ifndef __MATH_H
#define __MATH_H

typedef float float_t;
typedef double double_t;

#define HUGE_VAL __builtin_huge_val()
#define HUGE_VALF __builtin_huge_valf()
#define HUGE_VALL __builtin_huge_vall()
#define INFINITY __builtin_inff()
#define NAN __builtin_nanf("")

#define FP_NAN 0
#define FP_INFINITE 1
#define FP_ZERO 2
#define FP_SUBNORMAL 3
#define FP_NORMAL 4

#define MATH_ERRNO 1
#define MATH_ERREXCEPT 2
#define math_errhandling 0

// Constantes do POSIX/XSI (arredondadas para binary32 ao serem usadas).
#define M_E 2.7182818284590452354
#define M_LOG2E 1.4426950408889634074
#define M_LOG10E 0.43429448190325182765
#define M_LN2 0.69314718055994530942
#define M_LN10 2.30258509299404568402
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_PI_4 0.78539816339744830962
#define M_1_PI 0.31830988618379067154
#define M_2_PI 0.63661977236758134308
#define M_2_SQRTPI 1.12837916709551257390
#define M_SQRT2 1.41421356237309504880
#define M_SQRT1_2 0.70710678118654752440

int __ks_fpclassify(float x);

double sqrt(double x);
float sqrtf(float x);
long double sqrtl(long double x);
double fabs(double x);
float fabsf(float x);
long double fabsl(long double x);
double floor(double x);
float floorf(float x);
long double floorl(long double x);
double ceil(double x);
float ceilf(float x);
long double ceill(long double x);
double trunc(double x);
float truncf(float x);
long double truncl(long double x);
double rint(double x);
float rintf(float x);
long double rintl(long double x);
double nearbyint(double x);
float nearbyintf(float x);
long double nearbyintl(long double x);
double round(double x);
float roundf(float x);
long double roundl(long double x);
double fmin(double x, double y);
float fminf(float x, float y);
long double fminl(long double x, long double y);
double fmax(double x, double y);
float fmaxf(float x, float y);
long double fmaxl(long double x, long double y);
double fmod(double x, double y);
float fmodf(float x, float y);
long double fmodl(long double x, long double y);
double copysign(double x, double y);
float copysignf(float x, float y);
long double copysignl(long double x, long double y);

// Depois dos protótipos: as macros não podem expandir neles.
#define sqrt(x) __builtin_sqrt(x)
#define sqrtf(x) __builtin_sqrtf(x)
#define sqrtl(x) __builtin_sqrtl(x)
#define fabs(x) __builtin_fabs(x)
#define fabsf(x) __builtin_fabsf(x)
#define fabsl(x) __builtin_fabsl(x)
#define floor(x) __builtin_floor(x)
#define floorf(x) __builtin_floorf(x)
#define floorl(x) __builtin_floorl(x)
#define ceil(x) __builtin_ceil(x)
#define ceilf(x) __builtin_ceilf(x)
#define ceill(x) __builtin_ceill(x)
#define trunc(x) __builtin_trunc(x)
#define truncf(x) __builtin_truncf(x)
#define truncl(x) __builtin_truncl(x)
#define rint(x) __builtin_rint(x)
#define rintf(x) __builtin_rintf(x)
#define rintl(x) __builtin_rintl(x)
#define nearbyint(x) __builtin_nearbyint(x)
#define nearbyintf(x) __builtin_nearbyintf(x)
#define nearbyintl(x) __builtin_nearbyintl(x)
#define fmin(x, y) __builtin_fmin(x, y)
#define fminf(x, y) __builtin_fminf(x, y)
#define fminl(x, y) __builtin_fminl(x, y)
#define fmax(x, y) __builtin_fmax(x, y)
#define fmaxf(x, y) __builtin_fmaxf(x, y)
#define fmaxl(x, y) __builtin_fmaxl(x, y)

// Classificação (C11 7.12.3): avaliam o argumento uma vez, convertido
// para float.
#define isnan(x) __builtin_isnan(x)
#define isinf(x) __builtin_isinf(x)
#define isfinite(x) __builtin_isfinite(x)
#define signbit(x) __builtin_signbit(x)
#define fpclassify(x) __ks_fpclassify(x)
#define isnormal(x) (__ks_fpclassify(x) == FP_NORMAL)

// Comparações (C11 7.12.14). Na KSVM nenhuma comparação gera exceção,
// então as relacionais comuns já são as "quietas".
#define isgreater(x, y) ((x) > (y))
#define isgreaterequal(x, y) ((x) >= (y))
#define isless(x, y) ((x) < (y))
#define islessequal(x, y) ((x) <= (y))
#define islessgreater(x, y) \
  ({ float __ks_a = (x), __ks_b = (y); __ks_a < __ks_b || __ks_a > __ks_b; })
#define isunordered(x, y) (__builtin_isnan(x) | __builtin_isnan(y))

#endif
