// goto para frente e para trás; labels como valores (extensão GNU).
#include "minirt.h"
int main() {
  int i = 0, s = 0;
again:
  s += i;
  if (++i < 5) goto again;
  EQ(10, s);
  goto skip;
  s = 999;
skip:
  EQ(10, s);
  void *tgt = &&there;
  goto *tgt;
  s = 0;
there:
  EQ(10, s);
  return 0;
}
