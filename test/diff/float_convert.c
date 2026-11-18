// float <-> inteiro dentro do alcance (fora dele é UB e o diferencial não
// testa: o kscc satura, o x86 dá INT_MIN): int, unsigned, char, short,
// _Bool para float com arredondamento RNE, e float para inteiro com
// truncamento para zero.
#include "fltout.h"

static unsigned seed = 2463534242u;
static unsigned rnd(void) {
  seed ^= seed << 13;
  seed ^= seed >> 17;
  seed ^= seed << 5;
  return seed;
}

int main(void) {
  int ints[] = {0, 1, -1, 7, -128, 255, 32767, -32768, 65535, 16777215, 16777216, 16777217,
                16777219, 33554435, 2147483647, -2147483647 - 1, 1000000007, -999999999};
  for (int i = 0; i < (int)(sizeof(ints) / sizeof(ints[0])); i++) {
    float f = (float)ints[i];
    printf("i2f %d ", ints[i]);
    fl_hex(f);
    fl_hex((float)(unsigned)ints[i]);
    fl_hex((float)(short)ints[i]);
    fl_hex((float)(unsigned char)ints[i]);
    printf("\n");
  }
  for (int i = 0; i < 200; i++) {
    unsigned u = rnd();
    unsigned sa = rnd(), sb = rnd();  // inicializadores separados: a ordem dos rnd() é fixa
    int s = (int)sa >> (sb & 31);
    printf("r %08x ", u);
    fl_hex((float)u);
    fl_hex((float)s);
    fl_hex((float)(u >> (u & 31)));
    printf("\n");
  }

  float fs[] = {0.0f, -0.0f, 0.4f, 0.5f, 0.999f, 1.0f, -1.5f, 2.5f, -2.5f, 99.99f, 255.9f, -128.7f,
                32767.5f, 65535.9f, 16777216.0f, 2147483520.0f, -2147483648.0f, 4294967040.0f, 1e-45f};
  for (int i = 0; i < (int)(sizeof(fs) / sizeof(fs[0])); i++) {
    float f = fs[i];
    printf("f2i ");
    fl_hex(f);
    printf("%d", f < 2147483648.0f ? (int)f : -1);
    printf(" %u", f > -1.0f ? (unsigned)f : 0u);
    printf(" %d %d", f > -129.0f && f < 128.0f ? (signed char)f : 0, f > -1.0f && f < 256.0f ? (unsigned char)f : 0);
    printf(" %d %d", f > -32769.0f && f < 32768.0f ? (short)f : 0, f > -1.0f && f < 65536.0f ? (unsigned short)f : 0);
    printf(" %d\n", (_Bool)f);
  }
  for (int i = 0; i < 200; i++) {
    float f = fl_from((rnd() & 0x8FFFFFFF) | 0x30000000);   // |f| entre ~4.6e-10 e ~9.2e18
    printf("rf ");
    fl_hex(f);
    if (f > -2147483648.0f && f < 2147483648.0f)
      printf("%d", (int)f);
    if (f > -1.0f && f < 4294967296.0f)
      printf(" %u", (unsigned)f);
    printf("\n");
  }
  return 0;
}
