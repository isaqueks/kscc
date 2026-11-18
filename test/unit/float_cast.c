// Conversões int <-> float (itof/utof/ftoi/ftou, cast_table F32): RNE na
// ida, truncamento na volta, saturação fora do alcance e NaN -> 0 (fora
// do alcance é UB em C; o kscc dá o resultado definido da VM). Para
// char/short: ftoi e depois a redução, como int -> char.
#include "float_test.h"
static int to_i(float f) { return (int)f; }
static unsigned to_u(float f) { return (unsigned)f; }
static float from_i(int i) { return i; }
static float from_u(unsigned u) { return u; }
int main() {
  // inteiro -> float
  EQB(0x00000000, from_i(0));
  EQB(0xBF800000, from_i(-1));
  EQB(0x4B800000, from_i(16777216));
  EQB(0x4B800000, from_i(16777217));   // empate: para o par
  EQB(0x4B800002, from_i(16777219));   // empate: para o par (sobe)
  EQB(0x4B800002, from_i(16777218 + 1));
  EQB(0xCF000000, from_i(-2147483647 - 1));
  EQB(0x4F000000, from_i(2147483647)); // arredonda para 2^31
  EQB(0x4F800000, from_u(0xFFFFFFFFu)); // 2^32
  EQB(0x4F000000, from_u(0x80000000u));
  EQB(0x4F000001, from_u(0x80000080u + 0x01u)); // acima do empate: sobe
  EQB(0x437F0000, (float)(unsigned char)rt(255));
  EQB(0xC3000000, (float)(signed char)-128);
  EQB(0x477FFF00, (float)(unsigned short)65535);
  EQB(0xC7000000, (float)(short)-32768);
  EQB(0x3F800000, (float)(_Bool)7);

  // float -> inteiro: trunca para zero
  EQ(2, to_i(2.9f)); EQ(-2, to_i(-2.9f)); EQ(0, to_i(-0.5f)); EQ(0, to_i(rt(-0.0f)));
  EQ(16777216, to_i(16777216.0f));
  EQ(-2147483647 - 1, to_i(-2147483648.0f));
  // saturação e NaN
  EQ(2147483647, to_i(2147483648.0f));
  EQ(2147483647, to_i(1e10f));
  EQ(2147483647, to_i(bitsf(F_INF)));
  EQ(-2147483647 - 1, to_i(-1e10f));
  EQ(-2147483647 - 1, to_i(bitsf(F_NINF)));
  EQ(0, to_i(bitsf(F_NAN)));
  EQ(0, to_i(bitsf(0xFFC00000)));
  // unsigned: negativos dão 0, a partir de 2^32 satura
  EQ(3, (int)to_u(3.99f));
  EQ(0, (int)to_u(-0.5f));
  EQ(0, (int)to_u(-1.0f));
  EQ(0, (int)to_u(-1e10f));
  EQ((int)0x80000000u, (int)to_u(2147483648.0f));
  EQ((int)0xFFFFFF00u, (int)to_u(4294967040.0f));
  EQ((int)0xFFFFFFFFu, (int)to_u(4294967296.0f));
  EQ((int)0xFFFFFFFFu, (int)to_u(bitsf(F_INF)));
  EQ(0, (int)to_u(bitsf(F_NAN)));
  // char/short: ftoi e redução (300 -> 44, como o gcc com cvttss2si)
  float f300 = rt(300.7f);
  EQ(44, (signed char)f300);
  EQ(44, (unsigned char)f300);
  EQ(-56, (signed char)rt(200.0f));
  EQ(200, (unsigned char)rt(200.0f));
  EQ(255, (unsigned char)rt(-1.0f));
  EQ(-1, (short)rt(65535.0f));
  EQ(65535, (unsigned short)rt(65535.5f));
  EQ(1000, (short)rt(1000.3f));
  // _Bool: != 0
  EQ(1, (_Bool)rt(0.25f)); EQ(0, (_Bool)rt(-0.0f)); EQ(1, (_Bool)bitsf(F_NAN));

  // conversão implícita na atribuição, no retorno e nos argumentos
  int i = rt(7.75f);
  EQ(7, i);
  float g = 5;
  EQB(0x40A00000, g);
  unsigned char uc = rt(65.9f);
  EQ(65, uc);
  i = 3;
  i *= rt(2.5f);           // i = (int)(3 * 2.5f)
  EQ(7, i);
  i += 0.75;
  EQ(7, i);
  return 0;
}
