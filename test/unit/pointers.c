// Ponteiros: &, *, aritmética escalada, diferença, comparações.
#include "minirt.h"
int main() {
  int a[5] = {10, 20, 30, 40, 50};
  int *p = a;
  EQ(30, *(p + 2));
  EQ(30, p[2]);
  EQ(30, 2[p]);
  p += 3;
  EQ(40, *p);
  EQ(3, p - a);
  EQ(-3, a - p);
  EQ(1, p > a);
  *p = 99;
  EQ(99, a[3]);
  int x = 5, *q = &x, **qq = &q;
  **qq = 6;
  EQ(6, x);
  char s[3]; char *c = s;
  EQ(1, &s[1] - c);
  short sh[4]; short *ps = sh + 3;
  EQ(3, ps - sh);
  EQ(6, (char *)ps - (char *)sh);
  return 0;
}
