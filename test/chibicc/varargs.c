#include "test.h"
#include <stdarg.h>

int sum1(int x, ...) {
  va_list ap;
  va_start(ap, x);

  for (;;) {
    int y = va_arg(ap, int);
    if (y == 0)
      return x;
    x += y;
  }
}

#ifdef KSCC_HAS_FLOAT // kscc: ponto flutuante só na fase 6
int sum2(int x, ...) {
  va_list ap;
  va_start(ap, x);

  for (;;) {
    double y = va_arg(ap, double);
    x += y;

    int z = va_arg(ap, int);
    if (z == 0)
      return x;
    x += z;
  }
}
#endif

// kscc: va_copy sem depender de vsprintf: soma os argumentos duas vezes,
// a segunda pela cópia (a versão original, fmt, fica para a fase 4).
int sum_twice(int n, ...) {
  va_list ap;
  va_start(ap, n);

  va_list ap2;
  va_copy(ap2, ap);

  int x = 0;
  for (int i = 0; i < n; i++)
    x += va_arg(ap, int);
  for (int i = 0; i < n; i++)
    x += va_arg(ap2, int) * 100;
  va_end(ap2);
  va_end(ap);
  return x;
}

#ifdef KSCC_PHASE4 // kscc: vsprintf só com a libks (fase 4)
void fmt(char *buf, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  va_list ap2;
  va_copy(ap2, ap);
  vsprintf(buf, fmt, ap2);
  va_end(buf);
}
#endif

int main() {
  ASSERT(6, sum1(1, 2, 3, 0));
  ASSERT(55, sum1(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0));
#ifdef KSCC_HAS_FLOAT // kscc: ponto flutuante só na fase 6
  ASSERT(21, sum2(1, 2.0, 3, 4.0, 5, 6.0, 0));
  ASSERT(21, sum2(1, 2.0, 3, 4.0, 5, 6.0, 0));
  ASSERT(210, sum2(1, 2.0, 3, 4.0, 5, 6.0, 7, 8.0, 9, 10.0, 11, 12.0, 13, 14.0, 15, 16.0, 17, 18.0, 19, 20.0, 0));
#endif
  ASSERT(606, sum_twice(3, 1, 2, 3)); // kscc: substitui fmt (va_copy) até a fase 4
#ifdef KSCC_PHASE4 // kscc: vsprintf só com a libks (fase 4)
  ASSERT(0, ({ char buf[100]; fmt(buf, "%d %d", 2, 3); strcmp(buf, "2 3"); }));
#endif

  kscc_ok(); // kscc: printf só na fase 4 (libks)
  return 0;
}
