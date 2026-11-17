// ++/-- pré e pós em int, char (com enrolamento), unsigned e ponteiros.
#include "minirt.h"
int main() {
  int i = 5;
  EQ(5, i++);
  EQ(7, ++i);
  EQ(7, i--);
  EQ(5, --i);
  char c = 127;
  c++;
  EQ(-128, c);
  unsigned char uc = 0;
  uc--;
  EQ(255, uc);
  int a[3] = {1, 2, 3}, *p = a;
  EQ(1, *p++);
  EQ(3, *++p);
  EQ(2, *--p);
  int x = 10;
  x += 5; EQ(15, x);
  x -= 3; EQ(12, x);
  x *= 2; EQ(24, x);
  x /= 5; EQ(4, x);
  x %= 3; EQ(1, x);
  x |= 6; EQ(7, x);
  x &= 5; EQ(5, x);
  x ^= 1; EQ(4, x);
  return 0;
}
