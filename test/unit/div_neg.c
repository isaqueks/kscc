// Divisão e resto truncam para zero; resto tem o sinal do dividendo.
#include "minirt.h"
int main() {
  int a = -7, b = 2, c = -2, d = 7;
  EQ(-3, a / b);
  EQ(-1, a % b);
  EQ(-3, d / c);
  EQ(1, d % c);
  EQ(3, a / c);
  EQ(-1, a % c);
  unsigned ua = 0xFFFFFFF0u, ub = 16;
  EQ(268435455, (int)(ua / ub));
  EQ(0, (int)(ua % ub));
  EQ(1, (int)(0x80000000u / 0x7FFFFFFFu));
  int min = -2147483647 - 1, m1 = -1;
  EQ(-2147483647 - 1, min / m1);   // VM: INT_MIN / -1 = INT_MIN, sem fault
  EQ(0, min % m1);
  return 0;
}
