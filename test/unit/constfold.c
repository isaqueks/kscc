// Expressões constantes dobradas pelo eval() em 32 bits (inicializadores de
// globais, tamanhos de array, case e enum).
#include "minirt.h"
int g1 = 0x7FFFFFFF + 1;
unsigned g2 = -1u / 2;
int g3 = (char)300;
int g4 = (unsigned char)-1;
int g5 = -7 / 2;
int g6 = -7 % 2;
int g7 = 1 << 31 >> 31;
unsigned g8 = 1u << 31 >> 31;
int g9 = (-1u > 0) + (-1 < 0u);
int g10 = (_Bool)256;
int g11 = 0xFFFFFFFF == -1;
int g12 = (short)65535;
int g13 = (65536 * 65536);
long g14 = 2147483647L + 1;
unsigned g15 = 0x80000000 >> 4;
int g16 = (unsigned short)-2 + 1;
int g17 = ~(unsigned char)0;
int g18 = sizeof(long) + sizeof(void *) * 10 + sizeof(int *) * 100;
enum { E1 = 0x7FFFFFFF, E2 = -2147483647 - 1 };
int arr[(1 << 4) - 6];
int main() {
  EQ(-2147483647 - 1, g1);
  EQ(0x7FFFFFFF, g2);
  EQ(44, g3);
  EQ(255, g4);
  EQ(-3, g5);
  EQ(-1, g6);
  EQ(-1, g7);
  EQ(1, g8);
  EQ(1, g9);
  EQ(1, g10);
  EQ(1, g11);
  EQ(-1, g12);
  EQ(0, g13);
  EQ(-2147483647 - 1, g14);
  EQ(0x08000000, g15);
  EQ(65535, g16);
  EQ(-1, g17);
  EQ(444, g18);
  EQ(0x7FFFFFFF, E1);
  EQ(1, E2 < 0);
  EQ(40, sizeof(arr));
  switch (5) { case 2 + 3: break; default: EQ(0, 1); }
  return 0;
}
