// Expressão profunda: muitos push/pop aninhados.
#include "minirt.h"
int main() {
  int a = 1, b = 2, c = 3, d = 4;
  int r = ((((a + b) * (c + d)) - ((a - b) * (c - d))) * (((a * b) + (c * d)) - ((a + d) * (b + c))))
        + (a + (b + (c + (d + (a + (b + (c + (d + (a + (b + (c + d)))))))))));
  EQ((21 - 1) * (14 - 25) + 30, r);
  return 0;
}
