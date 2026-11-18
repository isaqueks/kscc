// float: as seis comparações, as negações e o valor de verdade sobre
// pares de valores especiais (NaN: só != é verdadeiro), em valor e em
// desvio (if, ?:, &&, ||, laço).
#include <float.h>
#include "fltout.h"

int main(void) {
  float inf = FLT_MAX * 2.0f;
  float v[] = {0.0f, -0.0f, 1.0f, -1.0f, 1e-45f, -1e-45f, FLT_MAX, inf, -inf, inf - inf, 0.5f, 0.50000006f};
  int n = (int)(sizeof(v) / sizeof(v[0]));

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      float a = v[i], b = v[j];
      int r = (a == b) | (a != b) << 1 | (a < b) << 2 | (a <= b) << 3 | (a > b) << 4 | (a >= b) << 5 |
              !(a < b) << 6 | !(a >= b) << 7 | (!a) << 8 | (a && b) << 9 | (a || b) << 10;
      int br = 0;
      if (a < b) br |= 1;
      if (!(a > b)) br |= 2;
      if (a == b || a < b) br |= 4;
      if (a != b && b <= a) br |= 8;
      br |= (a >= b ? 16 : 32);
      int k = 0;
      while (a <= b && k < 3) k++;
      printf("%2d %2d %03x %02x %d\n", i, j, r, br, k);
    }
  }
  return 0;
}
