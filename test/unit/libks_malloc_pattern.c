// malloc/free com um padrão de alocações e liberações intercaladas:
// ponteiros alinhados a 4, blocos vivos nunca se sobrepõem, o conteúdo
// sobrevive às operações nos vizinhos, e o heap fica consistente
// (ks_heap_check) depois de cada passo.
#include <stdlib.h>
#include <string.h>
#include "minirt.h"

#define N 40

static unsigned char *ptr[N];
static int size[N];

static int fill_byte(int i) { return (i * 37 + 11) & 0xFF; }

static void check_all(void) {
  EQ(0, ks_heap_check());
  for (int i = 0; i < N; i++) {
    if (!ptr[i])
      continue;
    EQ(0, (int)ptr[i] & 3);
    for (int k = 0; k < size[i]; k++)
      EQ(fill_byte(i), ptr[i][k]);
    for (int j = i + 1; j < N; j++) {
      if (!ptr[j])
        continue;
      int disjoint = ptr[i] + size[i] <= ptr[j] || ptr[j] + size[j] <= ptr[i];
      EQ(1, disjoint);
    }
  }
}

static void alloc(int i, int n) {
  ptr[i] = malloc(n);
  EQ(1, ptr[i] != 0);
  size[i] = n;
  memset(ptr[i], fill_byte(i), n);
}

int main() {
  EQ(0, ks_heap_check());  // antes da primeira chamada

  // Tamanhos variados, inclusive 0, 1 e não múltiplos de 4
  for (int i = 0; i < N; i++)
    alloc(i, (i * 13) % 50);
  check_all();

  // Libera os pares, realoca maiores, libera os múltiplos de 3...
  for (int i = 0; i < N; i += 2) {
    free(ptr[i]);
    ptr[i] = 0;
  }
  check_all();
  for (int i = 0; i < N; i += 2)
    alloc(i, 60 + i);
  check_all();
  for (int i = 0; i < N; i += 3) {
    free(ptr[i]);
    ptr[i] = 0;
  }
  check_all();
  for (int i = 0; i < N; i += 3)
    alloc(i, 3);
  check_all();

  // Libera tudo em ordem inversa e confere que o heap volta a um bloco só:
  // um bloco do tamanho de todos os anteriores somados cabe de novo.
  int total = 0;
  for (int i = N - 1; i >= 0; i--) {
    total += size[i] + 8;
    free(ptr[i]);
    ptr[i] = 0;
  }
  check_all();
  unsigned char *big = malloc(total);
  EQ(1, big != 0);
  free(big);

  free(0);  // free(NULL) não faz nada
  EQ(0, ks_heap_check());
  return 0;
}
