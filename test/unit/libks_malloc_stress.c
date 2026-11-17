// malloc/calloc/realloc/free numa sequência pseudoaleatória de 1500
// operações sobre 64 ponteiros, com tamanhos de 0 a ~600 bytes: o
// conteúdo de cada bloco vivo sobrevive a tudo (inclusive a realloc que
// move ou encolhe), calloc devolve zeros, blocos vivos nunca se
// sobrepõem e o heap fica consistente (ks_heap_check).
#include <stdlib.h>
#include <string.h>
#include "minirt.h"

#define N 64

static unsigned char *ptr[N];
static unsigned size[N], tag[N];
static unsigned seed = 12345;

static unsigned rnd(void) {
  seed = seed * 1103515245u + 12345u;
  return seed >> 8;
}

static void fill(int i) {
  for (unsigned k = 0; k < size[i]; k++)
    ptr[i][k] = tag[i] + k;
}

static void check(int i, unsigned n) {
  for (unsigned k = 0; k < n; k++)
    EQ((unsigned char)(tag[i] + k), ptr[i][k]);
}

static void check_disjoint(void) {
  for (int a = 0; a < N; a++)
    for (int b = a + 1; b < N; b++)
      if (ptr[a] && ptr[b] && size[a] && size[b])
        EQ(1, ptr[a] + size[a] <= ptr[b] || ptr[b] + size[b] <= ptr[a]);
}

int main() {
  for (int it = 0; it < 1500; it++) {
    int i = rnd() % N;
    unsigned n = (rnd() & 3) == 0 ? rnd() % 600 : rnd() % 64;

    if (ptr[i])
      check(i, size[i]);
    switch (rnd() % 4) {
    case 0:
      free(ptr[i]);
      ptr[i] = 0;
      size[i] = 0;
      break;
    case 1:
      free(ptr[i]);
      ptr[i] = malloc(n);
      EQ(1, ptr[i] != 0);
      EQ(0, (int)ptr[i] & 3);
      size[i] = n;
      tag[i] = rnd();
      fill(i);
      break;
    case 2: {
      unsigned char *q = realloc(ptr[i], n);
      if (n == 0 && ptr[i]) {  // realloc(p, 0) libera e devolve NULL
        EQ(0, (int)q);
        ptr[i] = 0;
        size[i] = 0;
        break;
      }
      EQ(1, q != 0);
      ptr[i] = q;
      check(i, size[i] < n ? size[i] : n);  // o prefixo comum sobrevive
      size[i] = n;
      fill(i);
      break;
    }
    case 3:
      free(ptr[i]);
      ptr[i] = calloc(n, 1);
      EQ(1, ptr[i] != 0);
      for (unsigned k = 0; k < n; k++)
        EQ(0, ptr[i][k]);
      size[i] = n;
      tag[i] = rnd();
      fill(i);
      break;
    }
    if (it % 100 == 0) {
      EQ(0, ks_heap_check());
      check_disjoint();
    }
  }

  for (int i = 0; i < N; i++) {
    if (ptr[i])
      check(i, size[i]);
    free(ptr[i]);
  }
  EQ(0, ks_heap_check());

  // Tudo livre de novo: o heap voltou a um bloco só, do tamanho original
  struct ks_mem_info mi;
  ks_sys_mem_info(&mi);
  void *all = malloc(mi.stack_limit - mi.heap_start - 64 - 8 - 4);
  EQ(1, all != 0);
  free(all);
  return 0;
}
