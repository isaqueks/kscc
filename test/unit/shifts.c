// Deslocamentos de 0 a 31, aritmético vs lógico, promoção do operando.
#include "minirt.h"
int main() {
  int one = 1, n31 = 31, m1 = -1;
  unsigned u = 0xFFFFFFFFu;
  EQ(-2147483647 - 1, one << n31);
  EQ(-1, m1 >> n31);
  EQ(1, (int)(u >> n31));
  EQ(-1, m1 >> 1);
  EQ(0x7FFFFFFF, (int)(u >> 1));
  EQ(8, 1 << 3);
  EQ(2, 5 >> 1);
  EQ(1, one << 0);
  char c = 1;
  EQ(1024, c << 10);           // c é promovido a int
  unsigned char uc = 0x80;
  EQ(0x8000, uc << 8);
  EQ(-2, ~c);
  EQ(-129, ~uc);               // ~ promove para int
  EQ(4, sizeof(c << 1));
  int x = 5;
  x <<= 2; EQ(20, x);
  x >>= 3; EQ(2, x);
  return 0;
}
