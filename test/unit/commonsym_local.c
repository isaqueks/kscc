// Definições provisórias (símbolos comuns) numa unidade só: repetidas,
// seguidas de definição com valor, array e alinhamento. O link cria o
// .bss das que ficam sem definição (src/link.c, resolve_commons).
#include "minirt.h"
int a;
int a;
int b;
int b = 7;
int arr[10];
_Alignas(16) int al;
static int st;

int main() {
  EQ(0, a);
  EQ(7, b);
  a = 3;
  arr[9] = 4;
  st = 5;
  al = 6;
  EQ(18, a + arr[9] + st + al);
  EQ(0, (int)&al % 16);
  EQ(40, sizeof(arr));
  return 0;
}
