// realloc crescendo (no lugar, se o vizinho está livre, ou movendo) e
// encolhendo, com o conteúdo preservado; realloc(NULL)/realloc(p, 0);
// calloc devolve memória zerada mesmo reaproveitando um bloco sujo.
#include <stdlib.h>
#include <string.h>
#include "minirt.h"

static void check_seq(unsigned char *p, int n) {
  for (int i = 0; i < n; i++)
    EQ(i & 0xFF, p[i]);
}

static void fill_seq(unsigned char *p, int n) {
  for (int i = 0; i < n; i++)
    p[i] = i;
}

int main() {
  // realloc(NULL, n) = malloc(n)
  unsigned char *p = realloc(0, 10);
  EQ(1, p != 0);
  fill_seq(p, 10);

  // Cresce no lugar: nada depois de p ainda
  unsigned char *q = realloc(p, 100);
  EQ((int)p, (int)q);
  check_seq(q, 10);
  fill_seq(q, 100);

  // Encolhe no lugar, e a sobra volta a ser usável
  q = realloc(q, 20);
  EQ((int)p, (int)q);
  check_seq(q, 20);
  unsigned char *after = malloc(40);
  EQ(1, after > q && after < q + 108);
  EQ(0, ks_heap_check());

  // Agora não dá para crescer no lugar: move e copia
  unsigned char *moved = realloc(q, 200);
  EQ(1, moved != q);
  check_seq(moved, 20);
  EQ(0, ks_heap_check());

  // Crescer no lugar absorvendo o vizinho livre
  // (x maior que o buraco deixado por q, para ficar junto de y)
  unsigned char *x = malloc(40), *y = malloc(64), *guard = malloc(4);
  fill_seq(x, 16);
  free(y);
  unsigned char *x2 = realloc(x, 60);
  EQ((int)x, (int)x2);
  check_seq(x2, 16);
  EQ(0, ks_heap_check());

  // realloc(p, 0) libera e devolve NULL
  EQ(0, (int)realloc(x2, 0));
  EQ(0, ks_heap_check());

  // Muitos realloc crescendo de 1 em 1 byte
  unsigned char *s = 0;
  for (int n = 1; n <= 300; n++) {
    s = realloc(s, n);
    EQ(1, s != 0);
    s[n - 1] = (n - 1) & 0xFF;
  }
  check_seq(s, 300);
  free(s);

  // Falha de realloc mantém o bloco original
  unsigned char *keep = malloc(8);
  fill_seq(keep, 8);
  EQ(0, (int)realloc(keep, (size_t)-1 / 2));
  check_seq(keep, 8);
  free(keep);

  // calloc zera um bloco antes sujo
  unsigned char *dirty = malloc(64), *g2 = malloc(4);  // g2: sem junção
  memset(dirty, 0x5A, 64);
  free(dirty);
  int *z = calloc(16, sizeof(int));
  EQ((int)dirty, (int)z);
  for (int i = 0; i < 16; i++)
    EQ(0, z[i]);
  free(z);
  unsigned char *c0 = calloc(0, 4);
  EQ(1, c0 != 0);
  free(c0);

  free(moved);
  free(after);
  free(guard);
  free(g2);
  EQ(0, ks_heap_check());
  return 0;
}
