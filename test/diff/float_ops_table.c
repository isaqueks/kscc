// float: + - * / entre todos os pares de um conjunto de valores especiais
// (zeros com sinal, subnormais, FLT_MIN/FLT_MAX, inf, NaN, valores comuns).
// Bits de cada resultado (NaN como "nan", test/fltout.h).
#include <float.h>
#include "fltout.h"

static float vals[] = {
  0.0f, -0.0f, 1.0f, -1.0f, 0.5f, 3.0f, -7.25f, 0.1f, 1e10f, -1e-10f,
  1e-45f, 1.1754942e-38f, FLT_MIN, FLT_MAX, -FLT_MAX, 16777216.0f, 1.0000001f,
  3.4e38f, 2.5e-39f, 123456.789f,
};
#define N (int)(sizeof(vals) / sizeof(vals[0]))

int main(void) {
  float inf = FLT_MAX * 2.0f;
  float nan = inf - inf;
  float all[N + 3];
  for (int i = 0; i < N; i++)
    all[i] = vals[i];
  all[N] = inf;
  all[N + 1] = -inf;
  all[N + 2] = nan;

  for (int i = 0; i < N + 3; i++) {
    for (int j = 0; j < N + 3; j++) {
      float a = all[i], b = all[j];
      printf("%2d %2d: ", i, j);
      fl_hex(a + b);
      fl_hex(a - b);
      fl_hex(a * b);
      fl_hex(a / b);
      printf("\n");
    }
  }
  return 0;
}
