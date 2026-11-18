// <math.h>: sqrtf fabsf floorf ceilf truncf rintf nearbyintf roundf fmodf
// copysignf fminf fmaxf e as classificações sobre um conjunto de valores
// (no kscc as primeiras são instruções F; no gcc, libm/SSE). fminf/fmaxf
// só com pares que não sejam ±0 entre si: a VM define fmax(-0, +0) = +0,
// e o glibc devolve qualquer um dos dois (C11 permite).
#include <float.h>
#include <math.h>
#include "fltout.h"

int main(void) {
  float v[] = {0.0f, -0.0f, 0.3f, -0.5f, 0.5f, 1.5f, 2.5f, -2.5f, 3.49999976f, -7.7f, 8388607.5f,
               8388609.0f, 1e-45f, FLT_MIN, 123456.789f, 1e20f, -3e38f, INFINITY, -INFINITY, NAN};
  int n = (int)(sizeof(v) / sizeof(v[0]));
  for (int i = 0; i < n; i++) {
    float x = v[i];
    printf("%2d ", i);
    fl_hex(sqrtf(x < 0 ? -x : x));
    fl_hex(fabsf(x));
    fl_hex(floorf(x));
    fl_hex(ceilf(x));
    fl_hex(truncf(x));
    fl_hex(rintf(x));
    fl_hex(nearbyintf(x));
    fl_hex(roundf(x));
    fl_hex(copysignf(1.5f, x));
    printf("%d%d%d%d %d\n", isnan(x) != 0, isinf(x) != 0, isfinite(x) != 0, signbit(x) != 0,
           fpclassify(x));
  }
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      float x = v[i], y = v[j];
      printf("%2d %2d ", i, j);
      fl_hex(fmodf(x, y));
      if (!(x == 0 && y == 0)) {
        fl_hex(fminf(x, y));
        fl_hex(fmaxf(x, y));
      }
      printf("%d\n", isunordered(x, y));
    }
  }
  // fmod com quocientes grandes e subnormais
  for (float a = 1e30f; a > 1; a /= 7.3f) {
    printf("fmod %g ", a);
    fl_hex(fmodf(a, 0.1f));
    fl_hex(fmodf(-a, 3.0f));
    fl_hex(fmodf(a, 1e-40f));
    printf("\n");
  }
  return 0;
}
