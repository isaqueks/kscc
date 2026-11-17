// Aritmética de ponteiros com elementos de 1, 2, 3, 4, 12 e 1000 bytes:
// soma, subtração, diferença, comparação, índices negativos.
#include "minirt.h"
struct T3 { char c[3]; };
struct T12 { int a, b, c; };
struct Big { char c[1000]; };

struct Big bigs[3];

int main() {
  char c[10];
  short s[10];
  struct T3 t3[10];
  int i[10];
  struct T12 t12[10];

  EQ(7, (char *)(c + 7) - (char *)c);
  EQ(14, (char *)(s + 7) - (char *)s);
  EQ(21, (char *)(t3 + 7) - (char *)t3);
  EQ(28, (char *)(i + 7) - (char *)i);
  EQ(84, (char *)(t12 + 7) - (char *)t12);
  EQ(2000, (char *)&bigs[2] - (char *)bigs);

  EQ(7, &s[9] - &s[2]);
  EQ(5, &t3[8] - &t3[3]);
  EQ(-3, &t12[1] - &t12[4]);
  EQ(2, &bigs[2] - &bigs[0]);

  int *p = &i[5];
  p[-2] = 33;
  EQ(33, i[3]);
  *(p - 5) = 11;
  EQ(11, i[0]);
  p += 2;
  p--;
  EQ(6, p - i);

  short *q = s + 3;
  q[1] = -5;
  EQ(-5, s[4]);

  struct T12 *r = t12 + 9;
  r->c = 77;
  EQ(77, t12[9].c);
  EQ(1, &t12[2] < &t12[3]);
  EQ(0, &t12[5] < &t12[3]);

  bigs[1].c[999] = 8;
  struct Big *b = bigs;
  EQ(8, (b + 1)->c[999]);
  return 0;
}
