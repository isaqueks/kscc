// 5 e 9 argumentos: do 5º em diante vão na pilha (fp + 8 + 4k).
#include "minirt.h"
int five(int a, int b, int c, int d, int e) { return a + 2*b + 3*c + 4*d + 5*e; }
int nine(int a, int b, int c, int d, int e, int f, int g, int h, int i) {
  return a*1 + b*2 + c*3 + d*4 + e*5 + f*6 + g*7 + h*8 + i*9;
}
int pick(int a, int b, int c, int d, int e, int f, int g, int h, int i, int k) {
  switch (k) {
  case 0: return a; case 4: return e; case 5: return f; case 8: return i;
  }
  return -1;
}
int sub9(int a, int b, int c, int d, int e, int f, int g, int h, int i) {
  return a - b - c - d - e - f - g - h - i;
}
int main() {
  EQ(1 + 4 + 9 + 16 + 25, five(1, 2, 3, 4, 5));
  EQ(285, nine(1, 2, 3, 4, 5, 6, 7, 8, 9));
  EQ(11, pick(11, 12, 13, 14, 15, 16, 17, 18, 19, 0));
  EQ(15, pick(11, 12, 13, 14, 15, 16, 17, 18, 19, 4));
  EQ(16, pick(11, 12, 13, 14, 15, 16, 17, 18, 19, 5));
  EQ(19, pick(11, 12, 13, 14, 15, 16, 17, 18, 19, 8));
  EQ(100 - 44, sub9(100, 1, 2, 3, 4, 5, 6, 7, 16));
  EQ(285, nine(five(1, 0, 0, 0, 0), 2, 3, 4, 5, 6, 7, 8, nine(9, 0, 0, 0, 0, 0, 0, 0, 0)));
  return 0;
}
