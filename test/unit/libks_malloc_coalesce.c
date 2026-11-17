// Coalescência e first-fit: liberar vizinhos junta os blocos (o espaço é
// reaproveitado pelo primeiro bloco grande o bastante), malloc(0) devolve
// ponteiros distintos, e pedir mais que o heap dá NULL com ENOMEM sem
// estragar o que já estava alocado.
#include <errno.h>
#include <stdlib.h>
#include <ksvm/syscall.h>
#include "minirt.h"

int main() {
  // Três blocos vizinhos de 100 bytes (+ 8 de cabeçalho cada)
  char *a = malloc(100), *b = malloc(100), *c = malloc(100);
  EQ(108, b - a);
  EQ(108, c - b);
  char *guard = malloc(4);  // impede a junção com o resto do heap

  // Liberar b e depois a: um bloco livre de 216 bytes no lugar de a
  free(b);
  free(a);
  EQ(0, ks_heap_check());
  char *ab = malloc(200);
  EQ((int)a, (int)ab);

  // Liberar a do meio de três livres junta os três (antes e depois)
  free(ab);
  free(c);
  EQ(0, ks_heap_check());
  char *abc = malloc(300);
  EQ((int)a, (int)abc);
  free(abc);

  // First-fit: o primeiro buraco que serve, mesmo havendo outro melhor
  char *p1 = malloc(40), *g1 = malloc(4), *p2 = malloc(16), *g2 = malloc(4);
  free(p1);
  free(p2);
  char *q = malloc(16);
  EQ(1, q == p1 || q == p2);  // a ordem da lista de livres decide
  free(q);
  free(g1);
  free(g2);

  // malloc(0): ponteiros válidos e distintos
  char *z1 = malloc(0), *z2 = malloc(0);
  EQ(1, z1 && z2 && z1 != z2);
  free(z1);
  free(z2);

  // Sem memória
  struct ks_mem_info mi;
  ks_sys_mem_info(&mi);
  errno = 0;
  EQ(0, (int)malloc(mi.ram_size));
  EQ(ENOMEM, errno);
  errno = 0;
  EQ(0, (int)malloc((size_t)-1));
  EQ(ENOMEM, errno);
  EQ(0, (int)calloc(0x10000, 0x10000));  // n * size estoura 32 bits
  EQ(0, ks_heap_check());

  // Esgota o heap em blocos de 16 KB e libera tudo: volta a caber o mesmo
  int n = 0;
  char *blocks[512];  // 1 MB de RAM no runner: ~60 blocos de 16 KB
  while (n < 512 && (blocks[n] = malloc(16384)))
    n++;
  EQ(1, n > 4);
  EQ(1, n < 512);
  for (int i = 0; i < n; i++)
    free(blocks[i]);
  EQ(0, ks_heap_check());
  for (int i = 0; i < n; i++)
    EQ(1, (blocks[i] = malloc(16384)) != 0);
  for (int i = n - 1; i >= 0; i--)
    free(blocks[i]);

  free(guard);
  EQ(0, ks_heap_check());

  // O heap fica entre o fim do .bss e a pilha
  char *h = malloc(4);
  EQ(1, (unsigned)h >= mi.heap_start && (unsigned)h < mi.stack_limit);
  free(h);
  return 0;
}
