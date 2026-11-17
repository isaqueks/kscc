// Operador vírgula e statement expressions.
#include "minirt.h"
int main() {
  int a, b;
  EQ(3, (a = 1, b = 2, a + b));
  EQ(5, ({ int x = 2; x += 3; x; }));
  int i, s = 0;
  for (i = 0, a = 10; i < 3; i++, a--)
    s += a;
  EQ(27, s);
  return 0;
}
