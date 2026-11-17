// snprintf/vsnprintf truncando: sempre termina com '\0' dentro do limite
// e devolve o comprimento que o texto inteiro teria.
#include <string.h>
#include "libks_test.h"

static int wrap(char *buf, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsnprintf(buf, n, fmt, ap);
  va_end(ap);
  return r;
}

int main() {
  char buf[16];

  memset(buf, 'X', sizeof(buf));
  EQ(11, snprintf(buf, 6, "hello %s", "world"));
  STR_EQ("hello", buf);
  EQ('X', buf[6]);  // nada escrito além do limite

  memset(buf, 'X', sizeof(buf));
  EQ(5, snprintf(buf, 6, "%d", 12345));  // cabe exatamente
  STR_EQ("12345", buf);

  EQ(6, snprintf(buf, 6, "%d", 123456));  // um a mais
  STR_EQ("12345", buf);

  memset(buf, 'X', sizeof(buf));
  EQ(3, snprintf(buf, 1, "abc"));  // só o '\0'
  EQ(0, buf[0]);
  EQ('X', buf[1]);

  buf[0] = 'Q';
  EQ(3, snprintf(buf, 0, "abc"));  // n = 0: nada escrito
  EQ('Q', buf[0]);
  EQ(7, snprintf((char *)0, 0, "%d", -123456));  // só mede

  // Truncar no meio de uma conversão com largura
  EQ(10, snprintf(buf, 4, "%10d", 42));
  STR_EQ("   ", buf);

  EQ(13, wrap(buf, 8, "%s-%s", "abcdef", "ghijkl"));
  STR_EQ("abcdef-", buf);

  // sprintf não tem limite
  char big[64];
  EQ(26, sprintf(big, "%s%s", "abcdefghijklm", "nopqrstuvwxyz"));
  STR_EQ("abcdefghijklmnopqrstuvwxyz", big);
  EQ(0, sprintf(big, ""));
  EQ(0, big[0]);
  return 0;
}
