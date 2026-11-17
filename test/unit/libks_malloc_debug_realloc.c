// KS_MALLOC_DEBUG: realloc de um bloco já liberado é acusado (sem a
// verificação, o bloco, que continua na lista de livres, voltava ao
// programa e podia ser entregue de novo pelo próximo malloc).
// cflags: -DKS_MALLOC_DEBUG
// expect-out: malloc: realloc de bloco livre
// expect-exit: 78
#include "../../libks/src/malloc.c"
#include "minirt.h"

int main() {
  char *a = malloc(32), *guard = malloc(4);
  (void)guard;
  free(a);
  realloc(a, 16);  // não retorna
  ks_put_str("nao devia chegar aqui\n");
  return 0;
}
