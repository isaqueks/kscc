// memcpy (alinhado e não), memmove com sobreposição nos dois sentidos,
// memset, memcmp (sinal sem sinal), memchr.
#include <string.h>
#include "libks_test.h"

static unsigned char a[64], b[64];

static void fill(unsigned char *p, int n, int seed) {
  for (int i = 0; i < n; i++)
    p[i] = seed + i * 7;
}

int main() {
  // memcpy em todas as combinações de desalinhamento e tamanhos 0..19
  for (int so = 0; so < 4; so++)
    for (int dof = 0; dof < 4; dof++)
      for (int n = 0; n < 20; n++) {
        fill(a, 64, 1);
        memset(b, 0xEE, 64);
        EQ((int)(b + dof), (int)memcpy(b + dof, a + so, n));
        for (int i = 0; i < 64; i++) {
          int inside = i >= dof && i < dof + n;
          EQ(inside ? a[so + i - dof] : 0xEE, b[i]);
        }
      }

  // memmove para a frente e para trás sobre o mesmo buffer
  for (int n = 0; n < 20; n++)
    for (int d = -5; d <= 5; d++) {
      fill(a, 64, 3);
      fill(b, 64, 3);
      memmove(a + 20 + d, a + 20, n);
      for (int i = 0; i < 64; i++) {
        int inside = i >= 20 + d && i < 20 + d + n;
        EQ(inside ? b[20 + i - (20 + d)] : b[i], a[i]);
      }
    }

  // memset com valor > 255 usa só o byte baixo; desalinhado
  for (int off = 0; off < 4; off++) {
    memset(a, 0, 64);
    EQ((int)(a + off), (int)memset(a + off, 0x1AB, 13));
    for (int i = 0; i < 64; i++)
      EQ(i >= off && i < off + 13 ? 0xAB : 0, a[i]);
  }

  // memcmp compara bytes sem sinal
  EQ(0, memcmp("abc", "abd", 2));
  EQ(1, memcmp("abd", "abc", 3) > 0);
  EQ(1, memcmp("\x01", "\xff", 1) < 0);
  EQ(0, memcmp("x", "y", 0));

  // memchr
  const char *s = "hello\0world";
  EQ(4, (char *)memchr(s, 'o', 11) - s);
  EQ(7, (char *)memchr(s + 5, 'o', 6) - s);
  EQ(5, (char *)memchr(s, 0, 11) - s);
  EQ(0, (int)memchr(s, 'z', 11));
  EQ(0, (int)memchr(s, 'h', 0));
  const char *ff = "a\xff";
  EQ(1, memchr(ff, 0xff, 2) == ff + 1);  // c convertido para unsigned char
  EQ(1, memchr(ff, -1, 2) == ff + 1);
  return 0;
}
