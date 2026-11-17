// snprintf da libks x glibc: combinações de flags, largura, precisão e
// tamanho para %d %i %u %x %X %o %c %s %%, com os valores de borda, e
// truncamento em buffers de 0 a 6 bytes. Sob o kscc o <stdio.h> é o da
// libks; sob o gcc, o da glibc. Só formatos com comportamento definido
// pelo C11 (nada de '#' em %d, '0' em %s, precisão em %c, %p ou %n).
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "kstest.h"

static unsigned hash = 2166136261u;

static void mix(const char *s, int n) {
  for (int i = 0; i < n; i++)
    hash = (hash ^ (unsigned char)s[i]) * 16777619u;
}

// Monta "%<flags><largura><.precisão><tamanho><conv>" sem usar printf.
static void build(char *f, const char *flags, int width, int prec,
                  const char *size, char conv) {
  *f++ = '%';
  while (*flags)
    *f++ = *flags++;
  if (width >= 10)
    *f++ = '0' + width / 10;
  if (width > 0)
    *f++ = '0' + width % 10;
  if (prec >= 0) {
    *f++ = '.';
    if (prec >= 10)
      *f++ = '0' + prec / 10;
    *f++ = '0' + prec % 10;
  }
  while (*size)
    *f++ = *size++;
  *f++ = conv;
  *f = 0;
}

static void show(const char *fmt, const char *out, int ret) {
  ks_put_str(fmt);
  ks_put_str(" [");
  ks_put_str(out);
  ks_put_str("] ");
  ks_put_int(ret);
  ks_put_chr('\n');
}

static const char *int_flags[] = {"", "-", "0", "+", " ", "-0", "+0", "-+", " 0", "+ ", "0-"};
static const char *hex_flags[] = {"", "-", "0", "#", "#0", "-#"};
static const int widths[] = {0, 1, 12};
static const int precs[] = {-1, 0, 5, 11};
static const char *sizes[] = {"", "hh", "h", "l"};
static const int ivals[] = {0, 1, -1, 7, 42, -42, 255, 256, 32767, -32768, 65535,
                            INT_MAX, INT_MIN, 123456789};

int main(void) {
  char fmt[32], out[64];
  int nflags = sizeof(int_flags) / sizeof(int_flags[0]);
  int nhex = sizeof(hex_flags) / sizeof(hex_flags[0]);
  int nvals = sizeof(ivals) / sizeof(ivals[0]);

  // %d e %i: todas as combinações vão para o hash; uma amostra é impressa
  for (int fl = 0; fl < nflags; fl++)
    for (int w = 0; w < 3; w++)
      for (int p = 0; p < 4; p++)
        for (int sz = 0; sz < 4; sz++)
          for (int v = 0; v < nvals; v++) {
            build(fmt, int_flags[fl], widths[w], precs[p], sizes[sz], v & 1 ? 'i' : 'd');
            int r = snprintf(out, sizeof(out), fmt, ivals[v]);
            mix(out, r);
            mix(fmt, (int)strlen(fmt));
            if ((fl + w + p + sz + v) % 23 == 0)
              show(fmt, out, r);
          }
  ks_put_kx("hash d/i", hash);

  // %u %x %X %o (com '#' nos três últimos)
  const char convs[] = "uxXo";
  for (int c = 0; c < 4; c++) {
    for (int fl = 0; fl < nhex; fl++) {
      if (convs[c] == 'u' && hex_flags[fl][0] == '#')
        continue;
      if (convs[c] == 'u' && hex_flags[fl][0] && hex_flags[fl][1] == '#')
        continue;
      for (int w = 0; w < 3; w++)
        for (int p = 0; p < 4; p++)
          for (int sz = 0; sz < 4; sz++)
            for (int v = 0; v < nvals; v++) {
              build(fmt, hex_flags[fl], widths[w], precs[p], sizes[sz], convs[c]);
              int r = snprintf(out, sizeof(out), fmt, (unsigned)ivals[v]);
              mix(out, r);
              if ((fl + w + p + sz + v + c) % 29 == 0)
                show(fmt, out, r);
            }
    }
    ks_put_kx("hash u/x/X/o", hash);
  }

  // %c e %s com largura (e precisão em %s)
  const char *strs[] = {"", "a", "abc", "abcdefghijklmnop"};
  for (int fl = 0; fl < 2; fl++)
    for (int w = 0; w < 3; w++) {
      build(fmt, fl ? "-" : "", widths[w], -1, "", 'c');
      int r = snprintf(out, sizeof(out), fmt, 'Q');
      show(fmt, out, r);
      for (int p = 0; p < 4; p++)
        for (int s = 0; s < 4; s++) {
          build(fmt, fl ? "-" : "", widths[w], precs[p], "", 's');
          r = snprintf(out, sizeof(out), fmt, strs[s]);
          show(fmt, out, r);
        }
    }

  // Largura e precisão por '*', inclusive negativas
  const int stars[] = {-9, -1, 0, 3, 8};
  for (int a = 0; a < 5; a++)
    for (int b = 0; b < 5; b++) {
      int r = snprintf(out, sizeof(out), "[%*.*d|%-*.*s|%0*x]", stars[a], stars[b], -77,
                       stars[b], stars[a], "texto", stars[a], 0xbeef);
      show("*", out, r);
    }

  // Truncamento: o valor devolvido é o comprimento completo e o buffer
  // sempre termina em '\0' dentro do limite
  static char tfmt[] = "%s=%d%%";  // não literal: o gcc não avisa do truncamento
  for (unsigned n = 0; n <= 6; n++) {
    memset(out, '#', 16);
    int r = snprintf(out, n, tfmt, "ab", -123);
    out[15] = 0;
    show("trunc", out, r);
  }
  return 0;
}
