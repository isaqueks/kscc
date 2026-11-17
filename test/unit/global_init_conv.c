// Conversões em inicializadores globais que o gcc aceita com aviso.
// - Endereço convertido para _Bool vale 1. Antes virava `.word x`: 4 bytes
//   num objeto de 1, sobrescrevendo o vizinho; e (_Bool)&x num int dava o
//   endereço de x.
// - Bitfield de union é mascarado à largura (antes: valor inteiro).
#include "minirt.h"
int x;
_Bool bb = &x;
char after = 7;
int ib = (_Bool)&x;
struct { char a; _Bool b; char c; } sb = { 1, (_Bool)&x, 3 };
union { unsigned a : 3; int b; } gu = { 15 };
int main() {
  EQ(1, bb);
  EQ(7, after);
  EQ(1, ib);
  EQ(1, sb.a);
  EQ(1, sb.b);
  EQ(3, sb.c);
  EQ(7, gu.b);
  return 0;
}
