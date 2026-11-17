// while, for, do-while; break e continue em laços aninhados.
#include "minirt.h"
int main() {
  int i = 0, s = 0;
  while (i < 10) { s += i; i++; }
  EQ(45, s);
  s = 0;
  for (int j = 0; j < 100; j++) {
    if (j % 2) continue;
    if (j > 10) break;
    s += j;
  }
  EQ(30, s);
  i = 0;
  do { i += 3; } while (i < 10);
  EQ(12, i);
  i = 100;
  do i++; while (0);
  EQ(101, i);
  s = 0;
  for (int a = 0; a < 5; a++)
    for (int b = 0; b < 5; b++) {
      if (b == a) break;
      if (b == 1) continue;
      s += 10 * a + b;
    }
  EQ(10 + 20 + 30 + 32 + 40 + 42 + 43, s);
  for (;;) { i = 7; break; }
  EQ(7, i);
  int k = 0;
  while (1) { if (++k == 1000) break; }
  EQ(1000, k);
  return 0;
}
