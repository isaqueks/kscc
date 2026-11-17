// Divisão e resto com sinal (truncamento para zero), shifts aritméticos de
// negativos (definido pela implementação: aritmético nos dois), conversões
// entre int e unsigned nas bordas e abs/min/max sem ramos.
#include "kstest.h"

static int floor_div(int a, int b) {
  int q = a / b;
  if ((a % b != 0) && ((a < 0) != (b < 0))) q--;
  return q;
}

static int mod_pos(int a, int b) {
  int r = a % b;
  return r < 0 ? r + (b < 0 ? -b : b) : r;
}

static int abs_nobranch(int x) {
  int m = x >> 31;
  return (x ^ m) - m;
}

static int min_nobranch(int a, int b) { return b ^ ((a ^ b) & -(a < b)); }

int main(void) {
  int vals[] = {7, -7, 1, -1, 0, 100, -100, 2147483647, -2147483647 - 1, 12345, -98765};
  int divs[] = {1, -1, 2, -2, 3, -3, 7, -7, 1000, -2147483647 - 1};
  unsigned h = 0;
  for (int i = 0; i < 11; i++)
    for (int j = 0; j < 10; j++) {
      int a = vals[i], b = divs[j];
      if (a == -2147483647 - 1 && b == -1) continue;
      int q = a / b, r = a % b;
      h = h * 31u + (unsigned)q;
      h = h * 31u + (unsigned)r;
      if (q * b + r != a) ks_put_str("identidade falhou\n");
    }
  ks_put_kx("divmod hash", h);
  for (int i = -7; i <= 7; i += 2) {
    ks_put_int(i / 2);
    ks_put_chr(' ');
    ks_put_int(i % 2);
    ks_put_chr(' ');
    ks_put_int(floor_div(i, 2));
    ks_put_chr(' ');
    ks_put_int(mod_pos(i, 2));
    ks_put_chr(' ');
    ks_put_int(i >> 1);
    ks_put_chr('|');
  }
  ks_put_chr('\n');
  int neg[] = {-1, -2, -3, -1024, -2147483647 - 1, -5};
  for (int i = 0; i < 6; i++) {
    for (int s = 0; s < 32; s += 7) {
      ks_put_int(neg[i] >> s);
      ks_put_chr(' ');
    }
    ks_put_chr('\n');
  }
  for (int i = 0; i < 11; i++) {
    if (vals[i] == -2147483647 - 1) continue;
    ks_put_int(abs_nobranch(vals[i]));
    ks_put_chr(' ');
  }
  ks_put_chr('\n');
  ks_put_kv("min", min_nobranch(-5, 3) * 100 + min_nobranch(9, 2));
  // divisão por constantes (o gcc usa multiplicação mágica; aqui div real)
  unsigned acc = 0;
  for (int x = -1000; x <= 1000; x += 37)
    acc = acc * 7u + (unsigned)(x / 3 + x / 5 * 10 + x % 7 * 100 + x / -9 * 1000);
  ks_put_kx("const div", acc);
  // unsigned vs signed na mesma operação
  int sm = -17;
  unsigned um = 5;
  ks_put_kx("sm / um", (unsigned)sm / um);
  ks_put_kx("sm % um", (unsigned)sm % um);
  ks_put_kv("sm / (int)um", sm / (int)um);
  ks_put_kv("sm % (int)um", sm % (int)um);
  ks_put_kv("INT_MIN / 2", (-2147483647 - 1) / 2);
  ks_put_kv("INT_MIN % 3", (-2147483647 - 1) % 3);
  ks_put_kv("INT_MAX / -1", 2147483647 / -1);
  return 0;
}
