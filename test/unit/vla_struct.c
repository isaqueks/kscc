// VLA de structs e de ponteiros, sizeof avaliado em tempo de execução
// (inclusive sizeof de tipo VLA), VLA junto com locais comuns e
// chamadas com argumentos na pilha depois da alocação.
#include "minirt.h"
typedef struct { short a; char b; } T; // 4 bytes

int sum7(int a, int b, int c, int d, int e, int f, int g) {
  return a + b + c + d + e + f + g;
}

int f(int n) {
  int before = 11;
  T ts[n];
  int *ptrs[n + 1];
  int after = 22;
  EQ(n * 4, sizeof(ts));
  EQ((n + 1) * 4, sizeof(ptrs));
  EQ(n * 3 * 2, sizeof(short[n][3]));
  for (int i = 0; i < n; i++) {
    ts[i].a = i;
    ts[i].b = 'a' + i;
    ptrs[i] = &before;
  }
  ptrs[n] = &after;
  int s = 0;
  for (int i = 0; i < n; i++)
    s += ts[i].a + *ptrs[i];
  return s + *ptrs[n] + sum7(1, 2, 3, 4, 5, 6, ts[n - 1].b);
}

int main() {
  // n=3: (0+1+2) + 3*11 + 22 + (21 + 'c')
  EQ(3 + 33 + 22 + 21 + 'c', f(3));
  EQ(45 + 110 + 22 + 21 + 'j', f(10));
  return 0;
}
