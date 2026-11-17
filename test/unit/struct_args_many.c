// Vários argumentos struct misturados com mais de 4 escalares: ordem na
// pilha (o mais à esquerda no endereço mais baixo), structs não consomem
// registrador, e retorno grande desloca os escalares (r2-r4 e pilha).
#include "minirt.h"
typedef struct { char c[5]; } S5;
typedef struct { int v[3]; } S12;

S5 mk5(int k) {
  S5 s;
  for (int i = 0; i < 5; i++)
    s.c[i] = k * 10 + i;
  return s;
}

int f(S5 a, int x1, S12 b, int x2, int x3, S5 c, int x4, int x5, int x6) {
  return a.c[4] + b.v[2] * 100 + c.c[0] * 10000 +
         x1 + x2 * 2 + x3 * 3 + x4 * 4 + x5 * 5 + x6 * 6;
}

// Retorno por ponteiro oculto (r1) + 6 escalares: a-c em r2-r4, d-f na pilha.
S12 g(int a, int b, int c, int d, int e, int f) {
  S12 r = {{a + b, c + d, e + f}};
  return r;
}

int main() {
  S12 b = {{1, 2, 3}};
  EQ(4 + 300 + 300000 + 1 + 4 + 9 + 16 + 25 + 36,
     f(mk5(0), 1, b, 2, 3, mk5(3), 4, 5, 6));

  S12 r = g(1, 2, 3, 4, 5, 6);
  EQ(3, r.v[0]);
  EQ(7, r.v[1]);
  EQ(11, r.v[2]);
  EQ(4 + 1100 + 100000 + 21,
     f(mk5(0), 1, g(1, 2, 3, 4, 5, 6), 1, 1, mk5(1), 1, 1, 1));
  return 0;
}
