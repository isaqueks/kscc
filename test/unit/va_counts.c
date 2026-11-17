// Variádicas com 0, 1, 5 e 10 argumentos anônimos (doc 02 §3.2): tudo na
// pilha, va_start = fp + 8 + tamanho dos nomeados, slots de 4 bytes.
// char/short anônimos chegam promovidos a int.
#include "minirt.h"
#include <stdarg.h>

// Soma ponderada: detecta ordem errada, não só valores errados.
int wsum(int n, ...) {
  va_list ap;
  va_start(ap, n);
  int s = 0;
  for (int i = 1; i <= n; i++)
    s += va_arg(ap, int) * i;
  va_end(ap);
  return s;
}

// Nenhum argumento anônimo: só o nomeado, que também está na pilha.
int only_named(int a, ...) {
  va_list ap;
  va_start(ap, a);
  va_end(ap);
  return a * 2;
}

// Vários nomeados (todos na pilha) antes dos anônimos.
int many_named(char a, short b, int c, int d, int e, int f, ...) {
  va_list ap;
  va_start(ap, f);
  int x = va_arg(ap, int);
  int y = va_arg(ap, int);
  va_end(ap);
  return a + b * 10 + c * 100 + d * 1000 + e * 10000 + f * 100000 + x * 1000000 + y * 10000000;
}

int main() {
  EQ(0, wsum(0));
  EQ(14, only_named(7));
  EQ(7, wsum(1, 7));
  EQ(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4 + 5 * 5, wsum(5, 1, 2, 3, 4, 5));
  EQ(385, wsum(10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10));

  char c = -3;
  short s = 1000;
  unsigned char uc = 200;
  EQ(-3 + 2000 + 600, wsum(3, c, s, uc));
  EQ(87654321, many_named(1, 2, 3, 4, 5, 6, 7, 8));
  EQ(-1 + 20, many_named(-1, 2, 0, 0, 0, 0, 0, 0));
  return 0;
}
