// Modelo de dados ILP32 (doc 02 §1).
#include "minirt.h"
#include <stddef.h>
int main() {
  EQ(1, sizeof(char));
  EQ(2, sizeof(short));
  EQ(4, sizeof(int));
  EQ(4, sizeof(long));
  EQ(4, sizeof(void *));
  EQ(4, sizeof(int (*)(void)));
  EQ(4, sizeof(size_t));
  EQ(4, sizeof(ptrdiff_t));
  EQ(1, _Generic(sizeof(int), unsigned int: 1, default: 0));
  int a[10];
  EQ(1, _Generic(&a[5] - &a[0], int: 1, default: 0));
  EQ(5, &a[5] - &a[0]);
  EQ(4, _Alignof(long));
  EQ(4, _Alignof(char *));
  EQ(1, __SIZEOF_LONG__ == 4 && __SIZEOF_POINTER__ == 4);
  EQ(1, __ksvm__ + __ILP32__ - 1);
  return 0;
}
