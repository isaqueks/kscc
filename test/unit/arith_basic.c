// + - * / % e unário, com operandos vindos de variáveis (não dobrados).
#include "minirt.h"
int main() {
  int a = 17, b = 5, c = -3;
  EQ(22, a + b);
  EQ(12, a - b);
  EQ(85, a * b);
  EQ(3, a / b);
  EQ(2, a % b);
  EQ(-51, a * c);
  EQ(3, -c);
  EQ(-17, -a);
  EQ(10, - - +10);
  EQ(47, 5 + 6 * 7);
  EQ(15, 5 * (9 - b - 1));
  EQ(-1, ~0);
  EQ(0, ~-1);
  EQ(1, a & 1);
  EQ(21, a | b);
  EQ(20, a ^ b);
  return 0;
}
