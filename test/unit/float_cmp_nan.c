// Comparações com fcmp: == beq, != bne, < bmi, <= bls (o parser troca os
// lados de > e >=). Com NaN só != é verdadeiro; !(a < b) não é a >= b.
// Vale para o valor 0/1 e para os desvios (if, ?:, while, &&, ||).
#include "float_test.h"
int main() {
  float n = bitsf(F_NAN), one = rt(1), two = rt(2), z = rt(0), nz = rt(-0.0f);
  float inf = bitsf(F_INF), ninf = bitsf(F_NINF);

  // valores
  EQ(0, n == n); EQ(1, n != n); EQ(0, n < n); EQ(0, n <= n); EQ(0, n > n); EQ(0, n >= n);
  EQ(0, n == one); EQ(1, n != one); EQ(0, n < one); EQ(0, n <= one); EQ(0, n > one); EQ(0, n >= one);
  EQ(0, one == n); EQ(1, one != n); EQ(0, one < n); EQ(0, one <= n); EQ(0, one > n); EQ(0, one >= n);
  EQ(1, !(n < one)); EQ(1, !(n <= one)); EQ(1, !(n > one)); EQ(1, !(n >= one));
  EQ(0, !(n != one)); EQ(1, !(n == one));
  // NaN com sinal ou payload diferente continua NaN
  EQ(1, bitsf(0xFFC00000) != bitsf(0xFFC00000));
  EQ(0, bitsf(0x7F800001) == bitsf(0x7F800001));

  // ordenados
  EQ(1, one < two); EQ(1, one <= two); EQ(0, one > two); EQ(0, one >= two);
  EQ(0, one == two); EQ(1, one != two); EQ(1, two >= two); EQ(1, two <= two);
  EQ(1, ninf < one); EQ(1, inf > two); EQ(1, inf == inf); EQ(0, inf != inf); EQ(1, ninf < inf);
  // -0 == +0
  EQ(1, z == nz); EQ(0, z != nz); EQ(0, z < nz); EQ(1, z <= nz); EQ(1, nz >= z);
  // mistura com inteiro: o int vira float
  EQ(1, one == 1); EQ(1, 2 > one); EQ(1, rt(16777216.0f) == 16777217); // 16777217 arredonda

  // desvios
  int c = 0;
  if (n < one) c += 1;
  if (n >= one) c += 2;
  if (n != one) c += 4;
  if (!(n < one)) c += 8;
  if (n == n) c += 16;
  if (one < two) c += 32;
  if (!(two < one)) c += 64;
  EQ(4 + 8 + 32 + 64, c);
  EQ(5, n > one ? 3 : 5);
  EQ(3, n != one ? 3 : 5);
  EQ(0, (n < one) && 1);
  EQ(1, (n < one) || (n != n));
  int k = 0;
  while (n < one && k < 10) k++;
  EQ(0, k);
  float x = 0;
  for (k = 0; x < 2; x += 0.5f) k++;
  EQ(4, k);
  return 0;
}
