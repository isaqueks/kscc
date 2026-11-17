// #if é avaliado em intmax_t (64 bits), não em 32.
#include "minirt.h"
int main() {
#if 0x7FFFFFFF + 1 > 0
  int a = 1;
#else
  int a = 0;
#endif
#if 4294967296 == 0x100000000
  int b = 1;
#else
  int b = 0;
#endif
  EQ(1, a);
  EQ(1, b);
  return 0;
}
