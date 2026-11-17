// atexit: handlers em ordem inversa, rodando antes da descarga de stdout
// (o "pendente" sem '\n' sai por último); exit de uma função aninhada
// com o código pedido; o 9º atexit é recusado (KS_ATEXIT_MAX = 8).
#include <stdio.h>
#include <stdlib.h>
#include "minirt.h"
// expect-out: [h3][h2][h1]
// expect-out: pendente
// expect-noeol
// expect-exit: 7

static void h1(void) { ks_put_str("[h1]\n"); }
static void h2(void) { ks_put_str("[h2]"); }
static void h3(void) { ks_put_str("[h3]"); }
static void nop(void) {}

static void deep(int n) {
  if (n == 0)
    exit(7);
  deep(n - 1);
}

int main() {
  EQ(0, atexit(h1));
  EQ(0, atexit(h2));
  EQ(0, atexit(h3));
  for (int i = 3; i < KS_ATEXIT_MAX; i++)
    EQ(0, atexit(nop));
  EQ(-1, atexit(nop));

  printf("pendente");  // sem '\n': fica no buffer até o exit
  deep(5);
  return 0;
}
