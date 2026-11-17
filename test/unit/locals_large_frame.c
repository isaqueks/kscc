// Frame maior que 8191 bytes: offsets de fp precisam de addl (doc 01 §5).
// cflags: -Wl,--stack=65536
#include "minirt.h"
int deep(int n) {
  int buf[3000];
  int last = 7;
  buf[0] = n;
  buf[2999] = n * 2;
  return buf[0] + buf[2999] + last;
}
int main() {
  int a = 1;
  int big[2500];
  int b = 2;
  big[0] = 3; big[2499] = 4;
  EQ(10, a + b + big[0] + big[2499]);
  EQ(37, deep(10));
  return 0;
}
