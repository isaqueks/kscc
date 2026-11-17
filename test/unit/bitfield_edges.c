// Bitfields nas bordas: largura 1 e 31/32, com e sem sinal, valores
// máximos/mínimos, overflow ao atribuir, vizinhos preservados, bitfields
// em global inicializada e em unsigned char.
#include "minirt.h"
struct A { unsigned u1 : 1; int s1 : 1; unsigned u31 : 31; };
struct B { int s31 : 31; unsigned top : 1; };
struct C { unsigned all : 32; };
struct D { unsigned char lo : 4, hi : 4; };
struct E { int a : 7; int b : 25; };

struct E ge = {-64, -16777216};

int main() {
  struct A a = {0};
  a.u1 = 1;
  a.s1 = 1; // não cabe: vira -1
  a.u31 = 0x7FFFFFFF;
  EQ(1, a.u1);
  EQ(-1, a.s1);
  EQ(0x7FFFFFFF, a.u31);
  a.u31 = 0xFFFFFFFF; // trunca para 31 bits
  EQ(0x7FFFFFFF, a.u31);
  EQ(1, a.u1);
  EQ(-1, a.s1);
  EQ(8, sizeof(struct A));

  struct B b;
  b.s31 = -1073741824; // mínimo de 31 bits
  b.top = 1;
  EQ(-1073741824, b.s31);
  EQ(1, b.top);
  b.s31 = 1073741823; // máximo
  EQ(1073741823, b.s31);
  EQ(1, b.top);
  b.s31 = 1073741824; // estoura para o mínimo
  EQ(-1073741824, b.s31);
  EQ(4, sizeof(struct B));

  struct C c;
  c.all = 0xFFFFFFFF;
  EQ(-1, (int)c.all);
  EQ(1, c.all > 0);

  struct D d;
  d.lo = 15;
  d.hi = 9;
  EQ(15, d.lo);
  EQ(9, d.hi);
  d.lo = 16; // trunca para 0
  EQ(0, d.lo);
  EQ(9, d.hi);
  EQ(1, sizeof(struct D));

  EQ(-64, ge.a);
  EQ(-16777216, ge.b);
  ge.a = 63;
  EQ(63, ge.a);
  EQ(-16777216, ge.b);
  ge.b += 1;
  EQ(-16777215, ge.b);
  EQ(63, ge.a);
  return 0;
}
