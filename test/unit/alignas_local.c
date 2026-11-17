// _Alignas em locais (8, 16, 64) e struct com membro alinhado: a área
// alinhada é calculada no prólogo a partir de fp (doc 02 §1). Testado em
// profundidades de pilha diferentes para variar o alinhamento de fp.
#include "minirt.h"
struct A16 { _Alignas(16) char c; int x; };

int check(int depth) {
  char c1;
  _Alignas(8) short s;
  char c2;
  _Alignas(16) int v[3];
  _Alignas(64) char buf[5];
  struct A16 a;
  c1 = 1; c2 = 2; s = 3; v[2] = 4; buf[4] = 5; a.x = 6;

  EQ(0, (int)&s % 8);
  EQ(0, (int)v % 16);
  EQ(0, (int)buf % 64);
  EQ(0, (int)&a % 16);
  EQ(21, c1 + c2 + s + v[2] + buf[4] + a.x);

  if (depth > 0) {
    char odd[depth * 4]; // VLA: desloca sp em múltiplos de 4
    odd[0] = 0;
    return check(depth - 1) + odd[0];
  }
  return 1;
}

int main() {
  EQ(16, _Alignof(struct A16));
  EQ(16, sizeof(struct A16));
  EQ(32, ({ _Alignas(32) char x, y; &y - &x; }));
  EQ(1, check(0));
  EQ(1, check(1));
  EQ(1, check(2));
  EQ(1, check(3));
  return 0;
}
