// A++ / A-- com A _Bool (variável ou campo _Bool : 1). O chibicc gerava
// (A += 1) - 1, e como o valor novo satura em 0/1 o antigo não era
// reconstruído: b++ com b == 1 dava 0, e b-- com b == 0 dava 1. O lvalue
// deve ser avaliado uma vez só. (Em test/unit porque o gcc avisa de ++ e
// -- em _Bool com -Wall.)
#include "minirt.h"
struct S { _Bool f : 1; _Bool g; int n; };
int main() {
  _Bool a = 1, c = 0;
  EQ(1, a++);
  EQ(1, a);
  EQ(0, c++);
  EQ(1, c);
  _Bool b = 0, d = 1;
  EQ(0, b--);
  EQ(1, b);
  EQ(1, d--);
  EQ(0, d);

  struct S s[3] = { { 1, 1, 7 }, { 0, 0, 0 }, { 1, 0, 0 } };
  struct S *p = s;
  EQ(1, p++->f++);
  EQ(1, s[0].f);
  EQ(1, p - s);
  EQ(1, s[0].g++);
  EQ(1, s[0].g);
  EQ(0, p->f--);
  EQ(1, p->f);
  EQ(7, s[0].n);
  EQ(4, sizeof(s[0].f++ + 0));

  _Bool v[4] = { 1, 0, 1, 1 };
  int i = 0, t = 0;
  while (i < 4)
    t = t * 10 + v[i++]++;
  EQ(1011, t);
  EQ(4, i);
  EQ(1111, v[0] * 1000 + v[1] * 100 + v[2] * 10 + v[3]);
  return 0;
}
