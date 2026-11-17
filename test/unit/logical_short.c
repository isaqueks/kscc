// && e || com curto-circuito e efeitos colaterais.
#include "minirt.h"
int calls;
int t(void) { calls++; return 1; }
int f(void) { calls++; return 0; }
int main() {
  calls = 0; EQ(0, f() && t()); EQ(1, calls);
  calls = 0; EQ(1, t() && t()); EQ(2, calls);
  calls = 0; EQ(1, t() || f()); EQ(1, calls);
  calls = 0; EQ(0, f() || f()); EQ(2, calls);
  calls = 0; EQ(1, f() || (t() && t())); EQ(3, calls);
  int i = 0;
  EQ(0, i && (i = 5));
  EQ(0, i);
  EQ(1, i || (i = 7));
  EQ(7, i);
  EQ(1, i || (i = 9));
  EQ(7, i);
  EQ(1, 5 && 6);
  EQ(1, 0 || -1);
  return 0;
}
