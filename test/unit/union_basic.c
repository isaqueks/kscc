// Union: membros sobrepostos (little-endian), tamanho/alinhamento, union
// por valor (até 4 bytes em r1; maior por ponteiro oculto), union dentro
// de struct.
#include "minirt.h"
union U4 { int i; short s[2]; unsigned char b[4]; };
union U6 { char c[6]; short s; };
struct Tagged { int kind; union { int i; char c[8]; } v; };

union U4 swap_halves(union U4 u) {
  short t = u.s[0];
  u.s[0] = u.s[1];
  u.s[1] = t;
  return u;
}

union U6 fill(int c) {
  union U6 u;
  for (int i = 0; i < 6; i++)
    u.c[i] = c + i;
  return u;
}

int main() {
  EQ(4, sizeof(union U4));
  EQ(6, sizeof(union U6));
  EQ(2, _Alignof(union U6));
  EQ(12, sizeof(struct Tagged));

  union U4 u;
  u.i = 0x11223344;
  EQ(0x44, u.b[0]);
  EQ(0x11, u.b[3]);
  EQ(0x3344, u.s[0]);

  union U4 w = swap_halves(u);
  EQ(0x33441122, w.i);
  EQ(0x11223344, u.i);

  union U6 x = fill('a');
  EQ('f', x.c[5]);
  EQ('a' + ('b' << 8), x.s);

  struct Tagged t;
  t.kind = 1;
  t.v.i = 0;
  t.v.c[0] = 5;
  t.v.c[1] = 1;
  EQ(261, t.v.i);
  return 0;
}
