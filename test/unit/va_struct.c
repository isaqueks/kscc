// Struct como argumento anônimo (va_arg(ap, struct), slot arredondado a
// 4) e variádica que devolve struct: até 4 bytes em r1; maior com o
// ponteiro oculto no primeiro slot da pilha (variádica: nada em r1-r4).
#include "minirt.h"
#include <stdarg.h>
typedef struct { char c[3]; } S3;
typedef struct { int a, b, c; } S12;

int sum_s3(int n, ...) {
  va_list ap;
  va_start(ap, n);
  int s = 0;
  for (int i = 0; i < n; i++) {
    S3 x = va_arg(ap, S3);
    s += x.c[0] + x.c[1] + x.c[2];
  }
  int tail = va_arg(ap, int); // depois de structs de 3 bytes (slots de 4)
  va_end(ap);
  return s * 100 + tail;
}

S12 make(int n, ...) {
  va_list ap;
  va_start(ap, n);
  S12 r = {0, 0, 0};
  for (int i = 0; i < n; i++)
    r.a += va_arg(ap, int);
  r.b = n;
  r.c = va_arg(ap, int);
  va_end(ap);
  return r;
}

S3 small(int k, ...) {
  va_list ap;
  va_start(ap, k);
  S3 r = {k, va_arg(ap, int), 9};
  va_end(ap);
  return r;
}

int main() {
  S3 a = {1, 2, 3}, b = {10, 20, 30};
  EQ(6600 + 7, sum_s3(2, a, b, 7));

  S12 r = make(3, 1, 2, 3, 99);
  EQ(6, r.a);
  EQ(3, r.b);
  EQ(99, r.c);
  EQ(42, make(1, 40, 2).a + make(0, 2).c);

  S3 s = small(4, 5);
  EQ(18, s.c[0] + s.c[1] + s.c[2]);
  return 0;
}
