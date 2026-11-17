// KS_MALLOC_DEBUG: escrever num bloco depois do free é acusado quando o
// bloco é reaproveitado ("bloco livre modificado"), e um cabeçalho pisado
// é acusado no free.
// cflags: -DKS_MALLOC_DEBUG
// expect-out: malloc: bloco livre modificado depois do free
// expect-exit: 78
#include "../../libks/src/malloc.c"
#include "minirt.h"

int main() {
  char *a = malloc(32), *guard = malloc(4);
  free(a);
  a[20] = 'x';  // uso depois do free
  (void)guard;
  malloc(32);   // reaproveita a: acusa
  ks_put_str("nao devia chegar aqui\n");
  return 0;
}
