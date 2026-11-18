// float_test.h — ajudantes dos testes unitários de ponto flutuante (fase 6).
//
// Os resultados são conferidos pelos bits do binary32 (iguais em qualquer
// host: doc 04 §5.7 da ksvm2), não por comparação de floats, que esconde
// -0 e o padrão de NaN.
#ifndef KSCC_FLOAT_TEST_H
#define KSCC_FLOAT_TEST_H

#include "minirt.h"

typedef union {
  float f;
  unsigned u;
} ks_fu;

static inline unsigned fbits(float f) {
  ks_fu x;
  x.f = f;
  return x.u;
}

static inline float bitsf(unsigned u) {
  ks_fu x;
  x.u = u;
  return x.f;
}

// Bits esperados (hexadecimal) de uma expressão float.
#define EQB(bits, expr) ks_assert_eq((int)(bits), (int)fbits(expr), __LINE__)

// Valores especiais calculados em tempo de execução (o parser não dobra
// chamadas), para comparar com as mesmas contas dobradas em tempo de
// compilação.
static inline float rt(float x) { return x; }

#define F_INF 0x7F800000
#define F_NINF 0xFF800000
#define F_NAN 0x7FC00000
#define F_NZERO 0x80000000

#endif
