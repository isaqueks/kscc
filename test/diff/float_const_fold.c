// float: as mesmas expressões dobradas em tempo de compilação
// (inicializadores de globais e static) e calculadas em tempo de execução
// (operandos voláteis). Com -fsingle-precision-constant o gcc também dobra
// em binary32; os bits têm de bater entre os dois compiladores e entre as
// duas formas.
#include <float.h>
#include "fltout.h"

static float g[] = {
  1.0f / 3.0f, 2.0f / 3.0f, 0.1 + 0.2, 1e38f * 10, 1.0f / 0.0f, -1.0f / 0.0f,
  16777216.0f + 1, 16777217, 4294967295u, -2147483647 - 1, 1e-45f / 2, 3e-45f / 2,
  (float)(int)2.75f, 0.0 ? 1.0f : 2.5f, -(0.0f), 0.1f * 3, 0.7f - 0.6f,
  1.1f * 1.1f * 1.1f, 100.0f / 7 * 7, FLT_MAX + FLT_MAX / 1e8f, FLT_MIN / 3, 1e10f / 3e-30f,
};
static int gi[] = {
  2.9f, -2.9f, 1.5f < 2.5f, 0.1 + 0.2 == 0.3, 1e30f > 1e29f, !0.5f, 0.5f && 0.0f, -0.0f || 0.0f,
  (int)(0.1f * 10), (unsigned char)250.7f, (short)-12345.6f, (int)1e9f, (int)FLT_MIN,
};
static _Bool gb[] = {0.5f, -0.0f, 1e-45f};

int main(void) {
  volatile float one = 1, two = 2, three = 3, zero = 0, tenth = 0.1f, big = 1e38f, ten = 10;
  float r[] = {
    one / three, two / three, tenth + 0.2f, big * ten, one / zero, -one / zero,
    16777216.0f + one, (float)(16777216 + (int)one), (float)(4294967294u + (unsigned)one),
    (float)(-2147483647 - (int)one), 1e-45f / two, 3e-45f / two, (float)(int)(2.75f * one),
    zero ? one : 2.5f, -zero, tenth * 3, 0.7f * one - 0.6f,
    1.1f * one * 1.1f * 1.1f, 100.0f / 7 * 7 * one, FLT_MAX + FLT_MAX * one / 1e8f, FLT_MIN / three,
    1e10f * one / 3e-30f,
  };
  for (int i = 0; i < (int)(sizeof(g) / sizeof(g[0])); i++) {
    printf("%2d ", i);
    fl_hex(g[i]);
    fl_hex(r[i]);
    printf("%s\n", fl_bits(g[i]) == fl_bits(r[i]) ? "=" : "DIFERENTE");
  }
  for (int i = 0; i < (int)(sizeof(gi) / sizeof(gi[0])); i++)
    printf("gi %d\n", gi[i]);
  printf("gb %d %d %d\n", gb[0], gb[1], gb[2]);
  static float s = 2.0f / 7.0f;
  float l = 2.0f / 7.0f;
  fl_put("static", s);
  fl_put("local", l);
  fl_put("runtime", two / (7 * one));
  return 0;
}
