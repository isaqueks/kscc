// Inicializadores locais: ND_MEMZERO (desenrolado e em laço) + stores.
#include "minirt.h"
int main() {
  int a[5] = {1, 2};
  EQ(3, a[0] + a[1] + a[2] + a[3] + a[4]);
  int b[100] = {[50] = 7};
  int s = 0;
  for (int i = 0; i < 100; i++) s += b[i];
  EQ(7, s);
  char c[7] = {1};
  EQ(1, c[0] + c[6]);
  char str[] = "abc";
  EQ(4, sizeof(str));
  EQ('b', str[1]);
  EQ(0, str[3]);
  return 0;
}
