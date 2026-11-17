// Muitos argumentos na pilha e chamadas indiretas com eles.
#include "minirt.h"
int sum12(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
          int a9, int a10, int a11, int a12) {
  return a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9 + a10 + a11 + a12 * 1000;
}
int main() {
  int (*f)(int, int, int, int, int, int, int, int, int, int, int, int) = sum12;
  EQ(12066, sum12(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12));
  EQ(12066, f(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12));
  return 0;
}
