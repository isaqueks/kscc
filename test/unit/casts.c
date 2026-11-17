// Tabela de casts entre inteiros (sxb/zxb/sxh/zxh) e para _Bool.
#include "minirt.h"
int main() {
  int i = 300, m = -1, big = 70000;
  EQ(44, (char)i);
  EQ(255, (unsigned char)m);
  EQ(-1, (signed char)m);
  EQ(4464, (short)big);
  EQ(65535, (unsigned short)m);
  EQ(-1, (short)(unsigned short)m);
  char c = -56;
  EQ(200, (unsigned char)c);
  EQ(65480, (unsigned short)c);
  unsigned char uc = 200;
  EQ(-56, (char)uc);
  short s = -2;
  EQ(254, (unsigned char)s);
  EQ(1, (_Bool)256);
  EQ(0, (_Bool)0);
  EQ(1, (_Bool)-1);
  int *p = &i;
  EQ(1, (_Bool)p);
  EQ(-1, (int)(unsigned)m);
  EQ(1, (unsigned)m == 0xFFFFFFFFu);
  long l = (long)i;
  EQ(300, l);
  return 0;
}
