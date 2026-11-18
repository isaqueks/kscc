// Valor de verdade de um float (fcmp com zr): ±0 é falso, NaN, inf,
// subnormais e qualquer outro valor são verdadeiros. Vale para if, !, &&,
// ||, ?:, laços e conversão para _Bool.
#include "float_test.h"
int main() {
  float z = rt(0), nz = rt(-0.0f), n = bitsf(F_NAN), tiny = bitsf(1), h = rt(0.5f);
  EQ(0, (_Bool)z); EQ(0, (_Bool)nz); EQ(1, (_Bool)n); EQ(1, (_Bool)tiny); EQ(1, (_Bool)h);
  EQ(1, (_Bool)bitsf(F_NINF));
  EQ(1, !z); EQ(1, !nz); EQ(0, !n); EQ(0, !tiny); EQ(0, !h);
  EQ(1, n && h); EQ(0, nz && h); EQ(1, nz || n); EQ(0, z || nz);
  EQ(7, nz ? 3 : 7);
  EQ(3, n ? 3 : 7);
  int c = 0;
  if (nz) c |= 1;
  if (tiny) c |= 2;
  if (n) c |= 4;
  EQ(6, c);
  _Bool b = h;             // conversão na atribuição
  EQ(1, b);
  b = nz;
  EQ(0, b);
  float d = 3;
  int k = 0;
  while (d) { d -= 1; k++; }
  EQ(3, k);
  do { k++; } while (nz);
  EQ(4, k);
  return 0;
}
