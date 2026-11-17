// alloca e VLA são liberados no epílogo (`mov sp, fp`): 2000 chamadas
// que alocam 1 KB cada cabem numa pilha de 8 KB. Recursão com alloca em
// cada nível mantém os blocos dos níveis de cima intactos.
// cflags: -Wl,--stack=8192
#include "minirt.h"

int use_alloca(int k) {
  char *p = alloca(1000);
  p[999] = k;
  return p[999];
}

int use_vla(int n) {
  int v[n];
  v[n - 1] = n;
  return v[n - 1];
}

int rec(int depth, char *parent) {
  char *mine = alloca(16);
  mine[0] = depth;
  if (depth == 0)
    return parent ? parent[0] : 0;
  int below = rec(depth - 1, mine);
  return below + mine[0] * 10;
}

int main() {
  int s = 0;
  for (int i = 0; i < 2000; i++)
    s += use_alloca(1) + use_vla(250) - 250;
  EQ(2000, s);
  // depth 3..1 contribuem 30+20+10; o nível 0 lê parent[0] = 1
  EQ(61, rec(3, 0));
  return 0;
}
