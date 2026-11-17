// ?: aninhado, só um dos ramos é avaliado, ?: GNU com operando omitido.
#include "minirt.h"
int n;
int bump(int v) { n++; return v; }
int main() {
  int a = 3, b = 0;
  EQ(1, a ? 1 : 2);
  EQ(2, b ? 1 : 2);
  EQ(30, a > 2 ? a > 5 ? 50 : 30 : 10);
  n = 0;
  EQ(7, b ? bump(5) : bump(7));
  EQ(1, n);
  EQ(3, a ?: 9);
  EQ(9, b ?: 9);
  unsigned u = 1;
  EQ(1, (a ? -1 : u) > 0);      // tipo comum unsigned
  b ? (void)bump(1) : (void)0;
  EQ(1, n);
  return 0;
}
