// Ponto flutuante: as quatro operações (fadd/fsub/fmul/fdiv), negação,
// zeros com sinal, inf, NaN canônico e subnormais. Operandos vêm de rt()
// para que as contas sejam feitas pela VM (as dobradas pelo parser estão
// em float_const_fold.c).
#include "float_test.h"
int main() {
  float one = rt(1), two = rt(2), three = rt(3), zero = rt(0), nzero = rt(-0.0f);
  EQB(0x40400000, one + two);
  EQB(0xBF800000, one - two);
  EQB(0x40C00000, two * three);
  EQB(0x3EAAAAAB, one / three);        // 1/3, RNE
  EQB(0x3F2AAAAB, two / three);
  EQB(0x3E99999A, rt(0.1f) + rt(0.2f)); // em binary32 dá exatamente 0.3f
  EQB(0x3E99999A, rt(0.3f));
  EQB(0x3F800000, rt(0.1f) * 10);      // int 10 vira 10.0f

  // inf e overflow
  EQB(F_INF, one / zero);
  EQB(F_NINF, one / nzero);
  EQB(F_NINF, -one / zero);
  EQB(F_INF, rt(3e38f) * two);
  EQB(F_NINF, rt(-3e38f) - rt(3e38f));
  EQB(F_INF, rt(3.4028235e38f) + rt(1e32f)); // passa de FLT_MAX + meio ulp

  // operações inválidas: NaN canônico 0x7FC00000, qualquer que seja o NaN
  EQB(F_NAN, zero / zero);
  EQB(F_NAN, (one / zero) - (one / zero));
  EQB(F_NAN, zero * (one / zero));
  EQB(F_NAN, bitsf(0xFFC00001) + one);  // NaN negativo com payload -> canônico
  EQB(F_NAN, bitsf(0x7F800001) * one);  // NaN sinalizador -> canônico

  // zeros: x - x = +0, -0 + -0 = -0, sinal de produto/quociente = xor
  EQB(0x00000000, three - three);
  EQB(0x00000000, zero + nzero);
  EQB(F_NZERO, nzero + nzero);
  EQB(F_NZERO, nzero * three);
  EQB(0x00000000, nzero * -three);
  EQB(F_NZERO, zero / -three);

  // negação unária: fneg só troca o sinal (também em 0 e NaN)
  EQB(F_NZERO, -zero);
  EQB(0x00000000, -nzero);
  EQB(0xFFC00000, -(zero / zero));
  EQB(0xC0400000, -three);
  EQB(0x40400000, +three);

  // subnormais: entram e saem sem flush-to-zero
  float tiny = bitsf(0x00000001);         // 2^-149
  EQB(0x00000002, tiny + tiny);
  EQB(0x00000000, tiny / two);            // 2^-150: empate, para o par (0)
  EQB(0x00000002, bitsf(0x00000003) / two); // 1.5 * 2^-149 -> 2 (par)
  EQB(0x00800000, bitsf(0x007FFFFF) + tiny); // vira o menor normal
  EQB(0x00400000, bitsf(0x00800000) / two);
  EQB(0x34000000, rt(1.0f) * rt(1.1920929e-7f)); // FLT_EPSILON
  EQB(0x3F800000, one + bitsf(0x33800000));      // 1 + eps/2: empate -> 1
  EQB(0x3F800001, one + bitsf(0x33800001));      // um pouco mais: sobe
  return 0;
}
