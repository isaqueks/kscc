// test/embedded/fselftest.c — 32 verificações de ponto flutuante (extensão
// F da VM): as quatro operações, comparações com NaN, conversões com
// saturação, builtins, float em struct e por varargs. Imprime os bits de
// alguns resultados e "fselftest ok" se todas passarem.
//
// A extensão F é opcional (doc 04 §5.7 da ksvm2): a porta AVR não a tem, e
// lá uma instrução F daria FAULT_BAD_OPCODE. O programa pergunta antes, por
// CAPS(0x4000) bit 2, e sem a extensão só avisa ("fselftest: sem extensao
// F") e sai com 0, sem executar instrução F nenhuma (as constantes float
// globais estão em .data, que não são instruções). O mesmo fonte compila
// no gcc -m32 -msse -mfpmath=sse, para comparar a saída (test/embedded/run.sh).
#include "kstest.h"
#include <stdarg.h>

#ifdef __kscc__
int ks_sys_caps(int n);
#else
static int ks_sys_caps(int n) { return n == 0x4000 ? 4 : 0; }
#endif

static int checks, fails;

static void check(int ok, int line) {
  checks++;
  if (!ok) {
    fails++;
    ks_put_str("FALHOU linha ");
    ks_put_int(line);
    ks_put_chr('\n');
  }
}
#define CHECK(c) check((c) ? 1 : 0, __LINE__)

static unsigned bits(float f) {
  union { float f; unsigned u; } b;
  b.f = f;
  return b.u;
}

static float fl(unsigned u) {
  union { float f; unsigned u; } b;
  b.u = u;
  return b.f;
}

// Valores passados por função para que nenhum compilador dobre as contas.
static float id(float x) { return x; }

typedef struct { float x, y; } V2;
static V2 v2_scale(V2 v, float k) { V2 r = {v.x * k, v.y * k}; return r; }

static float sum_va(int n, ...) {
  va_list ap;
  va_start(ap, n);
  float s = 0;
  for (int i = 0; i < n; i++)
    s += (float)va_arg(ap, double);
  va_end(ap);
  return s;
}

static const float table[4] = {0.5f, 0.25f, 0.125f, 0.0625f};

int main(void) {
  if (!(ks_sys_caps(0x4000) & 4)) {
    ks_put_str("fselftest: sem extensao F (CAPS bit 2)\n");
    return 0;
  }

  float one = id(1), three = id(3), zero = id(0);
  float nan = zero / zero, inf = one / zero;
  // aritmética (8)
  CHECK(bits(one / three) == 0x3EAAAAABu);
  CHECK(bits(id(0.1f) + id(0.2f)) == 0x3E99999Au);
  CHECK(bits(three * three - 9) == 0);
  CHECK(bits(-zero) == 0x80000000u);
  CHECK(bits(inf) == 0x7F800000u && bits(-inf) == 0xFF800000u);
  CHECK(nan != nan);
  CHECK(bits(fl(1) + fl(1)) == 2);                    // subnormais
  CHECK(bits(id(3e38f) * 2) == 0x7F800000u);
  // comparações (6)
  CHECK(!(nan < one) && !(nan >= one) && nan != one);
  CHECK(one < three && three >= three && !(three < one));
  CHECK(zero == -zero);
  CHECK(-inf < one && one < inf);
  CHECK((nan ? 1 : 0) == 1 && (-zero ? 1 : 0) == 0);
  CHECK(!(one > nan) && !(one <= nan));
  // conversões (8)
  CHECK((int)id(2.9f) == 2 && (int)id(-2.9f) == -2);
  CHECK((float)16777217 == 16777216.0f);
  CHECK(bits((float)0xFFFFFFFFu) == 0x4F800000u);
  CHECK((unsigned)id(3e9f) == 3000000000u);
  CHECK((unsigned char)id(200.5f) == 200);
  CHECK((short)id(-1234.75f) == -1234);
  CHECK((_Bool)id(0.25f) == 1 && (_Bool)id(-0.0f) == 0);
  CHECK((float)(signed char)-128 == -128.0f);
  // builtins (6)
  CHECK(bits(__builtin_sqrtf(id(2))) == 0x3FB504F3u);
  CHECK(__builtin_fabsf(id(-1.5f)) == 1.5f);
  CHECK(__builtin_floorf(id(-2.5f)) == -3 && __builtin_ceilf(id(-2.5f)) == -2);
  CHECK(__builtin_truncf(id(-2.5f)) == -2 && __builtin_rintf(id(-2.5f)) == -2);
  CHECK(__builtin_fminf(nan, one) == one && __builtin_fmaxf(one, three) == three);
  CHECK(__builtin_isnan(nan) && !__builtin_isnan(inf) && __builtin_isinf(inf));
  // dados e chamadas (4)
  V2 v = v2_scale((V2){1.5f, -2}, id(4));
  CHECK(v.x == 6 && v.y == -8);
  CHECK(sum_va(3, 1.0f, 2.5f, table[1]) == 3.75f);
  float acc = 0;
  for (int i = 0; i < 4; i++)
    acc += table[i];
  CHECK(bits(acc) == 0x3F700000u);                    // 0.9375
  float x = id(0.1f), old = x++;
  CHECK(bits(old) == 0x3DCCCCCDu && bits(x) == 0x3F8CCCCDu);

  ks_put_kx("1/3", bits(one / three));
  ks_put_kx("sqrt2", bits(__builtin_sqrtf(id(2))));
  ks_put_kx("pi*r^2", bits(id(3.14159265f) * id(2.5f) * id(2.5f)));
  ks_put_kv("checks", checks);
  if (fails == 0)
    ks_put_str("fselftest ok\n");
  return fails != 0;
}
