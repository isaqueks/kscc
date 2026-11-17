// Bitfields: leitura com shli/shri/sari e escrita com máscara (32 bits).
#include "minirt.h"
struct S { unsigned a : 3; int b : 5; unsigned c : 24; };
int main() {
  struct S s;
  s.a = 5; s.b = -3; s.c = 0xABCDEF;
  EQ(5, s.a);
  EQ(-3, s.b);
  EQ(0xABCDEF, s.c);
  s.a = 9;
  EQ(1, s.a);
  EQ(-3, s.b);
  EQ(4, sizeof(struct S));
  return 0;
}
