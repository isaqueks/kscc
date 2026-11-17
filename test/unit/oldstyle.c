// f() sem protótipo usa a convenção normal (registradores), não a
// variádica, para casar com a definição com parâmetros.
#include "minirt.h"
int add3();
int main() {
  EQ(6, add3(1, 2, 3));
  return 0;
}
int add3(int a, int b, int c) { return a + b + c; }
