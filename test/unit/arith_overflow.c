// Bordas de 32 bits em tempo de execução: overflow com sinal enrola (a VM
// é módulo 2^32), unsigned enrola por definição.
#include "minirt.h"
int main() {
  int max = 0x7FFFFFFF;
  unsigned umax = 0xFFFFFFFFu;
  int one = 1;
  EQ(-2147483647 - 1, max + one);
  EQ(0, (int)(umax + one));
  EQ(1, umax + one == 0);
  EQ(-1, (int)umax);
  unsigned m1 = -1u;
  EQ(1, m1 == 0xFFFFFFFF);
  EQ(1, m1 > 0);
  EQ(0, 1073741824 * 100 / 100);
  int big = 65536;
  EQ(0, big * big);
  unsigned ubig = 65537;
  EQ(131073, (int)(ubig * ubig));
  return 0;
}
