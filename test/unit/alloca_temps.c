// alloca no meio de uma expressão com temporários empilhados: argumentos
// já avaliados (inclusive na pilha, 5º em diante, e struct por valor) e
// operandos pendentes descem junto com sp (builtin_alloca do chibicc).
#include "minirt.h"
typedef struct { int a, b; } P;

int f6(int a, int b, char *p, int d, int e, int f) {
  p[0] = 'x';
  return a + b * 10 + d * 100 + e * 1000 + f * 10000 + (p[0] == 'x');
}

int fs(P x, char *p, P y) {
  p[3] = 1;
  return x.a + x.b + y.a + y.b + p[3];
}

char *id(char *p) { return p; }

int main() {
  // Argumentos avaliados da direita para a esquerda: f, e, d já estão
  // na pilha quando alloca roda.
  EQ(54321 + 1, f6(1, 2, alloca(10), 3, 4, 5));

  P x = {1, 2}, y = {3, 4};
  EQ(11, fs(x, alloca(4), y));

  // Operando pendente de uma soma (push r1 antes de avaliar o lado
  // direito).
  int k = 7;
  int v = k + (__builtin_alloca(8) != 0) + k * 2;
  EQ(22, v);

  // alloca dentro de argumento de chamada aninhada.
  char *p = id(id(alloca(3)));
  p[0] = 'q';
  EQ('q', *p);
  return 0;
}
