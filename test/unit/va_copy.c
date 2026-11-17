// va_copy: a cópia anda independente da original; cópia feita no meio
// da lista; va_list passada adiante por valor e por ponteiro.
#include "minirt.h"
#include <stdarg.h>

int sum_v(int n, va_list ap) {
  int s = 0;
  while (n--)
    s += va_arg(ap, int);
  return s;
}

// Consome um argumento pelo ponteiro: o chamador vê o avanço.
int take(va_list *ap) { return va_arg(*ap, int); }

int f(int n, ...) {
  va_list ap, cp;
  va_start(ap, n);
  int first = va_arg(ap, int);
  va_copy(cp, ap);              // cópia depois do primeiro
  int rest = sum_v(n - 1, ap);  // por valor: ap não anda aqui
  int again = sum_v(n - 1, cp);
  int second = take(&ap);       // ap ainda aponta para o segundo
  int third = take(&ap);
  va_end(cp);
  va_end(ap);
  return first * 1000000 + rest * 10000 + again * 100 + second * 10 + third;
}

int main() {
  // first=1, rest=2+3+4=9, again=9, second=2, third=3
  EQ(1000000 + 90000 + 900 + 20 + 3, f(4, 1, 2, 3, 4));
  return 0;
}
