// A expressão do assert vai à mensagem como argumento de %s, nunca como
// formato: '%' e "%d" na expressão saem literais e não consomem
// argumentos inexistentes.
#include <assert.h>
// expect-out: test/unit/libks_assert_percent.c:10: assert(x % 3 == 2 && "100%d %s%n") falhou
// expect-exit: 1

int main() {
  int x = 4;
  assert(x % 3 == 2 && "100%d %s%n");
  return 0;
}
