// vprintf/vsprintf/vfprintf com va_list repassada por funções do usuário
// (uma vez direto e outra por va_copy), e printf com muitos argumentos.
#include <stdarg.h>
#include <stdio.h>
#include "libks_test.h"
// expect-out: log: 1 dois 3
// expect-out: erro: 9
// expect-out: 1 2 3 4 5 6 7 8 9 10 11 12
// expect-out: 27

static void logf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  printf("log: ");
  vprintf(fmt, ap);
  va_end(ap);
}

static int twice(char *a, char *b, const char *fmt, ...) {
  va_list ap, ap2;
  va_start(ap, fmt);
  va_copy(ap2, ap);
  int n = vsprintf(a, fmt, ap);
  vsprintf(b, fmt, ap2);
  va_end(ap2);
  va_end(ap);
  return n;
}

static void err(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}

int main() {
  logf("%d %s %d\n", 1, "dois", 3);

  char a[32], b[32];
  EQ(7, twice(a, b, "%c-%05d", 'z', 42));
  STR_EQ("z-00042", a);
  STR_EQ("z-00042", b);

  err("erro: %d\n", 9);
  EQ(-1, fprintf(stdin, "x"));

  int n = printf("%d %d %d %d %d %d %d %d %d %d %d %d\n", 1, 2, 3, 4, 5, 6,
                 7, 8, 9, 10, 11, 12);
  printf("%d\n", n);
  return 0;
}
