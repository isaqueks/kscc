// Inicializador constante de ponto flutuante num bitfield global: a
// conversão é a do tipo do campo, como em tempo de execução (C11 6.7.9p11).
// write_bitfield convertia todo float por FTOI (int): `_Bool b : 1` com
// 0.5f ficava 0 (0.5 != 0 é verdadeiro) e `unsigned u : 32` com 3e9f
// saturava em 2147483647.
#include "float_test.h"

struct S { _Bool b : 1; unsigned u : 32; int i : 5; unsigned v : 4; _Bool n : 1; };
struct S g = {0.5f, 3e9f, -7.9f, 9.5f, 0.0f / 0.0f};
union { _Bool b : 1; int x; } gu = {0.25f};

int main() {
  EQ(1, g.b);
  EQ((int)3000000000u, (int)g.u);
  EQ(-7, g.i);
  EQ(9, g.v);
  EQ(1, g.n);                      // NaN != 0
  EQ(1, gu.b);
  // os mesmos valores em tempo de execução
  struct S l = {rt(0.5f), rt(3e9f), rt(-7.9f), rt(9.5f), rt(0.0f) / rt(0.0f)};
  EQ(l.b, g.b);
  EQ((int)l.u, (int)g.u);
  EQ(l.i, g.i);
  EQ(l.v, g.v);
  EQ(l.n, g.n);
  return 0;
}
