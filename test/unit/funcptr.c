// Ponteiros de função: chamada indireta (callr r5), com 6 argumentos,
// passados como argumento e devolvidos.
#include "minirt.h"
int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
int six(int a, int b, int c, int d, int e, int f) { return a + b + c + d + e + f * 100; }
int apply(int (*fn)(int, int), int x, int y) { return fn(x, y); }
int (*choose(int k))(int, int) { return k ? add : sub; }
int main() {
  int (*p)(int, int) = add;
  EQ(5, p(2, 3));
  p = sub;
  EQ(-1, p(2, 3));
  EQ(5, (*p)(8, 3));
  EQ(30, apply(add, 10, 20));
  EQ(7, choose(1)(3, 4));
  EQ(-1, choose(0)(3, 4));
  int (*q)(int, int, int, int, int, int) = six;
  EQ(615, q(1, 2, 3, 4, 5, 6));
  int (*tbl[2])(int, int);
  tbl[0] = add; tbl[1] = sub;
  EQ(9, tbl[0](4, 5) + tbl[1](5, 5));
  return 0;
}
