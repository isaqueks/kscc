// Acesso a membros e cópia de struct por atribuição (já suportados).
#include "minirt.h"
struct P { char c; int x; short s; };
struct Big { int v[20]; };
int main() {
  struct P p, q;
  p.c = 'a'; p.x = 1000; p.s = -2;
  q = p;
  EQ('a', q.c);
  EQ(1000, q.x);
  EQ(-2, q.s);
  EQ(12, sizeof(struct P));
  struct P *pp = &q;
  pp->x = 5;
  EQ(5, q.x);
  struct Big b1, b2;
  for (int i = 0; i < 20; i++) b1.v[i] = i;
  b2 = b1;
  EQ(19, b2.v[19]);
  EQ(7, b2.v[7]);
  return 0;
}
