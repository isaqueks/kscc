// Convenção variádica (doc 02 §3.2): todos os argumentos na pilha;
// va_list é char*.
#include "minirt.h"
#include <stdarg.h>
int sum(int n, ...) {
  va_list ap;
  va_start(ap, n);
  int s = 0;
  for (int i = 0; i < n; i++)
    s += va_arg(ap, int);
  va_end(ap);
  return s;
}
int first_char(char c, ...) {
  va_list ap;
  va_start(ap, c);
  int x = va_arg(ap, int);
  return c * 1000 + x;
}
int main() {
  EQ(0, sum(0));
  EQ(6, sum(3, 1, 2, 3));
  EQ(55, sum(10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10));
  EQ(3007, first_char(3, 7));
  return 0;
}
