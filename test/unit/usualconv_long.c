// Conversões usuais com int e long do mesmo tamanho (posto decide).
#include "minirt.h"
int main() {
  long l = 1; unsigned u = 1; int i = 1; unsigned long ul = 1;
  EQ(1, _Generic(l + u, unsigned long: 1, default: 0));
  EQ(1, _Generic(u + l, unsigned long: 1, default: 0));
  EQ(1, _Generic(l + i, long: 1, default: 0));
  EQ(1, _Generic(i + u, unsigned int: 1, default: 0));
  EQ(1, _Generic(ul + i, unsigned long: 1, default: 0));
  EQ(1, _Generic((char)1 + (char)1, int: 1, default: 0));
  EQ(1, _Generic(1L, long: 1, default: 0));
  EQ(1, _Generic(0xFFFFFFFF, unsigned int: 1, default: 0));
  EQ(1, _Generic(0x7FFFFFFF, int: 1, default: 0));
  EQ(1, _Generic(4294967295u, unsigned int: 1, default: 0));
  EQ(1, _Generic(0xFFFFFFFFL, unsigned long: 1, default: 0));
  l = -1;
  EQ(1, l < i);
  EQ(0, l < u);
  return 0;
}
