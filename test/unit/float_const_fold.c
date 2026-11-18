// Constantes dobradas pelo parser (inicializadores de globais, que exigem
// constante) dão o mesmo padrão de bits que a mesma conta feita pela VM em
// tempo de execução: cada operação é arredondada para binary32 (eval_double
// em parse.c), e a conversão float -> inteiro é a da VM.
#include "float_test.h"

float g_div = 1.0f / 3.0f;
float g_sum = 0.1 + 0.2;               // literais sem sufixo também são float
float g_chain = 1.0f / 3 * 3 - 1;
float g_big = 16777216.0f + 1.0f;      // não é 16777217
float g_mix = 16777217;                // int -> float com RNE
float g_u = 4294967295u;
float g_neg = -(0.0f);
float g_inf = 1.0f / 0.0f;
float g_nan = 0.0f / 0.0f;
float g_sub = 1e-45f * 0.5f;           // 2^-150: empate para 0
float g_ovf = 3e38f * 10;
double g_dbl = 1e-7 * 3;
long double g_ld = 2.5L / 4;
float g_tern = 0.0 ? 1.0f : 2.0f;
float g_comma = (1.0f, 3.5f);
float g_cast = (float)(int)2.75f;
float g_lit[] = {0.1f, 1e10f, -2.5e-3f, 0x1.8p1f, 1e39f, 1.17549435e-38f, 7e-46f, 0x1p-149f};
int g_i = 2.99f;                       // float -> int truncado
int g_isat = 1e10f;                    // saturação (FTOI)
int g_inan = 0.0f / 0.0f;              // NaN -> 0
unsigned g_uneg = -0.5f;               // FTOU: 0
unsigned g_usat = 1e10f;               // FTOU satura
signed char g_c = 300.5f;              // ftoi + redução: 44
_Bool g_b1 = 0.25f, g_b0 = -0.0f, g_bn = 0.0f / 0.0f;
int g_lt = 1.5f < 2.5f, g_eqn = (0.0f / 0.0f) == (0.0f / 0.0f), g_nen = (0.0f / 0.0f) != 1.0f;
int g_not = !0.5f, g_and = 0.5f && 0.25f, g_or = 0.0f || -0.0f;
int g_cnd = 0.5f ? 10 : 20;
int g_cndz = -0.0f ? 10 : 20;
char g_arr[(int)2.5f + 1];             // cast de float em expressão constante inteira
struct { float a; int b; float c[2]; } g_s = {1.25f, 2, {3.0f, -4.5f}};

int main() {
  float one = rt(1), three = rt(3), z = rt(0);
  EQB(fbits(one / three), g_div);
  EQB(fbits(rt(0.1f) + rt(0.2f)), g_sum);
  EQB(fbits(one / 3 * 3 - 1), g_chain);
  EQB(0x00000000, g_chain);
  EQB(fbits(rt(16777216.0f) + one), g_big);
  EQB(0x4B800000, g_big);
  EQB(0x4B800000, g_mix);
  EQB(0x4F800000, g_u);
  EQB(F_NZERO, g_neg);
  EQB(F_INF, g_inf);
  EQB(F_NAN, g_nan);
  EQB(fbits(z / z), g_nan);
  EQB(0, g_sub);
  EQB(fbits(rt(1e-45f) * rt(0.5f)), g_sub);
  EQB(F_INF, g_ovf);
  EQB(fbits(rt(1e-7f) * 3), g_dbl);
  EQB(0x3F200000, g_ld);
  EQB(0x40000000, g_tern);
  EQB(0x40600000, g_comma);
  EQB(0x40000000, g_cast);
  EQB(0x3DCCCCCD, g_lit[0]);
  EQB(0x501502F9, g_lit[1]);
  EQB(0xBB23D70A, g_lit[2]);
  EQB(0x40400000, g_lit[3]);
  EQB(F_INF, g_lit[4]);
  EQB(0x00800000, g_lit[5]);
  EQB(0x00000000, g_lit[6]);           // 7e-46 < 2^-150: zero
  EQB(0x00000001, g_lit[7]);
  EQ(2, g_i);
  EQ(2147483647, g_isat);
  EQ(0, g_inan);
  EQ(0, (int)g_uneg);
  EQ((int)0xFFFFFFFFu, (int)g_usat);
  EQ(44, g_c);
  EQ(1, g_b1); EQ(0, g_b0); EQ(1, g_bn);
  EQ(1, g_lt); EQ(0, g_eqn); EQ(1, g_nen);
  EQ(0, g_not); EQ(1, g_and); EQ(0, g_or);
  EQ(10, g_cnd); EQ(20, g_cndz);
  EQ(3, (int)sizeof(g_arr));
  EQB(0x3FA00000, g_s.a); EQ(2, g_s.b); EQB(0x40400000, g_s.c[0]); EQB(0xC0900000, g_s.c[1]);
  // mesmas conversões em tempo de execução
  EQ(g_isat, (int)rt(1e10f));
  EQ(g_inan, (int)(z / z));
  EQ(g_c, (signed char)rt(300.5f));
  EQ((int)g_usat, (int)(unsigned)rt(1e10f));
  // static local e locais com inicializador constante
  static float s_loc = 2.0f / 3.0f;
  float loc = 2.0f / 3.0f;
  EQB(fbits(rt(2.0f) / rt(3.0f)), s_loc);
  EQB(fbits(s_loc), loc);
  return 0;
}
