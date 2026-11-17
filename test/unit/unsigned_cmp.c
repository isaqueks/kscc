// Comparações com e sem sinal (blo/bls/bhi vs blt/ble/bgt).
#include "minirt.h"
int main() {
  int m1 = -1;
  unsigned one = 1;
  EQ(0, m1 < one);             // -1 vira 0xFFFFFFFF
  EQ(1, m1 > one);
  EQ(1, m1 < 1);
  EQ(1, (unsigned)m1 >= 0xFFFFFFFFu);
  unsigned big = 0x80000000u;
  EQ(1, big > 0x7FFFFFFFu);
  int sbig = 0x80000000;       // hex que não cabe em int é unsigned int; aqui convertido
  EQ(1, sbig < 0);
  EQ(1, 2 <= 2);
  EQ(0, 3 <= 2);
  EQ(1, one >= 1u);
  unsigned short us = 65535;
  EQ(1, us > -1);              // unsigned short promove a int
  long l = -1;
  unsigned int ui = 1;
  EQ(1, l > ui);               // ILP32: long + unsigned int -> unsigned long
  return 0;
}
