// _Bool: armazenamento normalizado em 0/1, !, !!.
#include "minirt.h"
_Bool g;
int main() {
  _Bool b = 42;
  EQ(1, b);
  b = 0;
  EQ(0, b);
  g = 7;
  EQ(1, g);
  int x = 5;
  EQ(0, !x);
  EQ(1, !!x);
  EQ(1, !0);
  EQ(1, sizeof(_Bool));
  return 0;
}
