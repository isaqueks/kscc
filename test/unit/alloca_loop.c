// alloca em laço: cada iteração ganha um bloco novo (liberados juntos no
// epílogo), blocos não se sobrepõem, tamanhos arredondados a 4, e o
// conteúdo dos anteriores sobrevive.
// cflags: -Wl,--stack=65536
#include "minirt.h"
#include <stddef.h>

int fill_and_sum(int n) {
  int *blocks[20];
  for (int i = 0; i < n; i++) {
    blocks[i] = alloca((i + 1) * sizeof(int));
    for (int j = 0; j <= i; j++)
      blocks[i][j] = i;
  }
  int s = 0;
  for (int i = 0; i < n; i++)
    for (int j = 0; j <= i; j++)
      s += blocks[i][j];
  return s;
}

int main() {
  // sum_{i<n} i*(i+1)
  EQ(0, fill_and_sum(0));
  EQ(2, fill_and_sum(2));
  EQ(2660, fill_and_sum(20));

  char *prev = alloca(1);
  for (int i = 1; i <= 8; i++) {
    char *p = alloca(i);
    EQ(0, (int)p % 4);
    EQ(i <= 4 ? 4 : 8, prev - p);
    prev = p;
  }

  // Muitas chamadas em laço: a pilha cresce, mas a função volta limpa.
  for (int k = 0; k < 200; k++)
    EQ(2660, fill_and_sum(20));
  return 0;
}
