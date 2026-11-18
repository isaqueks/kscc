// ++/-- e atribuições compostas em float. x++ devolve o valor antigo
// exato (o (x += 1) - 1 do chibicc arredondava: 0.1f + 1 - 1 != 0.1f), e
// o 1 vira 1.0f. Também em membro de struct, elemento de array e *p.
#include "float_test.h"
struct S { int pad; float f; };
int main() {
  float x = rt(0.1f);
  float old = x++;
  EQB(0x3DCCCCCD, old);
  EQB(0x3F8CCCCD, x);                // 1.1f (arredondado)
  old = x--;
  EQB(0x3F8CCCCD, old);
  EQB(0x3DCCCCD0, x);                // 1.1f - 1: não volta a 0.1f
  x = rt(16777216.0f);
  old = x++;
  EQB(0x4B800000, old);
  EQB(0x4B800000, x);                // 2^24 + 1 não é representável
  EQB(0x4B800000, ++x + 1);          // ++x continua 2^24 (empate), + 1 também
  x = rt(-0.0f);
  EQB(F_NZERO, x++);
  EQB(0x3F800000, x);
  EQB(0x00000000, --x);              // 1 - 1 = +0

  struct S s = {7, 2.5f};
  EQB(0x40200000, s.f++);
  EQB(0x40600000, s.f);
  EQ(7, s.pad);
  float a[3] = {1, 2, 3};
  float *p = a + 1;
  EQB(0x40000000, (*p)--);
  EQB(0x3F800000, a[1]);
  EQB(0x40400000, a[2]++);
  EQB(0x40800000, a[2]);
  int i = 0;
  a[i++] += 0.5f;                    // lvalue avaliado uma vez
  EQ(1, i);
  EQB(0x3FC00000, a[0]);

  // compostas
  float y = rt(3);
  y += 1.5f; EQB(0x40900000, y);
  y -= 0.5;  EQB(0x40800000, y);
  y *= 2;    EQB(0x41000000, y);
  y /= 3;    EQB(0x402AAAAB, y);
  y = rt(1);
  y /= 0;    EQB(F_INF, y);
  int k = 7;
  k /= rt(2.0f);                     // (int)(7 / 2.0f) = 3
  EQ(3, k);
  unsigned char c = 250;
  c += rt(10.5f);                    // (unsigned char)(int)260.5 = 4
  EQ(4, c);
  double d = 1;
  for (int j = 0; j < 10; j++)
    d += 0.1;
  EQB(0x40000001, d);                // 2.0000002 em binary32
  return 0;
}
