// Variádica chamando variádica: repassa argumentos explícitos a outra
// variádica, passa a va_list a uma função "v", e chama variádica por
// ponteiro de função. Também verifica que os argumentos do chamador
// não são corrompidos pelas chamadas internas.
// expect-out: [a=1 b=22 c=-3]
#include "minirt.h"
#include <stdarg.h>

int vcat(int n, va_list ap) {
  int r = 0;
  for (int i = 0; i < n; i++)
    r = r * 10 + va_arg(ap, int);
  return r;
}

int cat(int n, ...) {
  va_list ap;
  va_start(ap, n);
  int r = vcat(n, ap);
  va_end(ap);
  return r;
}

// Chama cat com os próprios argumentos anônimos em ordem inversa.
int rev3(int tag, ...) {
  va_list ap;
  va_start(ap, tag);
  int a = va_arg(ap, int);
  int b = va_arg(ap, int);
  int c = va_arg(ap, int);
  int r = cat(3, c, b, a);
  // Os anônimos continuam acessíveis depois da chamada interna.
  va_list ap2;
  va_start(ap2, tag);
  r += 1000 * va_arg(ap2, int) * tag;
  va_end(ap2);
  va_end(ap);
  return r;
}

void print_kv(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  for (char *p = fmt; *p; p++) {
    if (*p == '%') {
      ks_put_int(va_arg(ap, int));
      continue;
    }
    ks_sys_putc(*p);
  }
  va_end(ap);
}

int main() {
  EQ(123, cat(3, 1, 2, 3));
  EQ(321 + 1000 * 1 * 2, rev3(2, 1, 2, 3));
  int (*fp)(int, ...) = cat;
  EQ(4567, fp(4, 4, 5, 6, 7));
  print_kv("[a=% b=% c=%]\n", 1, 22, -3);
  return 0;
}
