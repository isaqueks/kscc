// assert falhando: descarrega stdout, imprime "arquivo:linha: assert(x)
// falhou" e sai com 1 (sem rodar atexit). Expressão verdadeira não faz
// nada; a expressão é avaliada uma vez.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
// expect-out: antes
// expect-out: [pendente]test/unit/libks_assert_fail.c:24: assert(x == 2 && "dois") falhou
// expect-exit: 1

static void handler(void) { printf("atexit nao devia rodar\n"); }

int main() {
  int x = 1, n = 0;
  atexit(handler);

  assert(x == 1);
  assert(++n == 1);
  if (n != 1)
    return 2;

  printf("antes\n");
  printf("[pendente]");  // sai antes da mensagem
  assert(x == 2 && "dois");
  return 3;
}
