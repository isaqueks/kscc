// Variáveis locais static (globais anônimas) e funções static.
#include "minirt.h"
static int counter(void) { static int n; return ++n; }
static int twice(int x) { static int calls = 10; calls++; return x * 2 + calls - calls; }
int main() {
  EQ(1, counter());
  EQ(2, counter());
  EQ(3, counter());
  EQ(8, twice(4));
  return 0;
}
