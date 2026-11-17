#include "test.h"

int main() {
#ifdef KSCC_HAS_FLOAT // kscc: ponto flutuante só na fase 6
  ASSERT(1, _Generic(100.0, double: 1, int *: 2, int: 3, float: 4));
  ASSERT(2, _Generic((int *)0, double: 1, int *: 2, int: 3, float: 4));
  ASSERT(2, _Generic((int[3]){}, double: 1, int *: 2, int: 3, float: 4));
  ASSERT(3, _Generic(100, double: 1, int *: 2, int: 3, float: 4));
  ASSERT(4, _Generic(100f, double: 1, int *: 2, int: 3, float: 4));
#endif
  // kscc: os mesmos casos sem ponto flutuante (double -> long, float -> unsigned)
  ASSERT(1, _Generic(100L, long: 1, int *: 2, int: 3, unsigned: 4));
  ASSERT(2, _Generic((int *)0, long: 1, int *: 2, int: 3, unsigned: 4));
  ASSERT(2, _Generic((int[3]){}, long: 1, int *: 2, int: 3, unsigned: 4));
  ASSERT(3, _Generic(100, long: 1, int *: 2, int: 3, unsigned: 4));
  ASSERT(4, _Generic(100u, long: 1, int *: 2, int: 3, unsigned: 4));

  printf("OK\n");
  return 0;
}
