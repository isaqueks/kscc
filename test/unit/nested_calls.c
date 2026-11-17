// Chamadas aninhadas nos argumentos: os registradores r1-r4 só são
// carregados depois de todos os argumentos avaliados.
#include "minirt.h"
int id(int x) { return x; }
int f4(int a, int b, int c, int d) { return a * 1000 + b * 100 + c * 10 + d; }
int f6(int a, int b, int c, int d, int e, int f) { return a + b + c + d + e + f; }
int main() {
  EQ(1234, f4(id(1), id(2), f4(0, 0, 0, 3), id(id(4))));
  EQ(21, f6(id(1), 2, f6(1, 1, 1, 0, 0, 0), 4, id(5), f6(0, 0, 0, 0, 0, 6)));
  EQ(4321, f4(f4(0, 0, 0, 4), f4(0, 0, 0, 3), f4(0, 0, 0, 2), f4(0, 0, 0, 1)));
  return 0;
}
