// KS_MALLOC_DEBUG: este teste inclui libks/src/malloc.c compilado com a
// verificação ligada. As definições do programa vencem as da libks no
// link (seleção por uso), então malloc/free aqui são as de depuração.
// Blocos liberados ficam com 0xDD; free duplo é acusado e chama abort
// (trap 1: o ksvm sai com 70 + 8, FAULT_TRAP).
// cflags: -DKS_MALLOC_DEBUG
// expect-out: malloc: free duplo
// expect-exit: 78
#include "../../libks/src/malloc.c"
#include "minirt.h"

int main() {
  unsigned char *a = malloc(40), *b = malloc(40);
  memset(a, 0x11, 40);
  memset(b, 0x22, 40);

  // Depois do free, a área útil (menos os 8 bytes dos elos da lista de
  // livres) é 0xDD
  free(b);
  for (int i = 8; i < 40; i++)
    EQ(0xDD, b[i]);
  for (int i = 0; i < 40; i++)
    EQ(0x11, a[i]);

  // O bloco liberado é reaproveitado sem queixa (o 0xDD está intacto)
  unsigned char *c = malloc(40);
  EQ((int)b, (int)c);
  free(c);
  EQ(0, ks_heap_check());

  free(a);
  free(a);  // não retorna
  ks_put_str("nao devia chegar aqui\n");
  return 0;
}
