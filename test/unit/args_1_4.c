// Funções com 1 e 4 argumentos (todos em r1-r4).
#include "minirt.h"
int one(int a) { return a * 2; }
int four(int a, int b, int c, int d) { return a * 1000 + b * 100 + c * 10 + d; }
int order(int a, int b, int c, int d) { return a - b - c - d; }
int main() {
  EQ(14, one(7));
  EQ(1234, four(1, 2, 3, 4));
  EQ(4, order(10, 3, 2, 1));
  EQ(4321, four(4, 3, 2, 1));
  EQ(1234, four(one(0) + 1, 2, one(1) + 1, four(0, 0, 0, 4)));
  return 0;
}
