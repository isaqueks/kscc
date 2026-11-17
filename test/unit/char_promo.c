// char é signed; promoções de char/short; loads com extensão de sinal/zero.
#include "minirt.h"
int main() {
  char c = 255;
  unsigned char uc = 255;
  signed char sc = -128;
  short s = 65535;
  unsigned short us = 65535;
  EQ(-1, c);
  EQ(255, uc);
  EQ(-128, sc);
  EQ(-1, s);
  EQ(65535, us);
  EQ(254, uc + uc - 256);
  EQ(-2, c + c);
  EQ(510, uc + uc);
  char a[4];
  a[0] = 200; a[1] = 100; a[2] = -1; a[3] = 0;
  EQ(-56, a[0]);
  EQ(100, a[1]);
  EQ(299, (unsigned char)a[2] + 44);
  EQ(1, sizeof(char));
  EQ(2, sizeof(short));
  EQ(4, sizeof(+c));
  return 0;
}
