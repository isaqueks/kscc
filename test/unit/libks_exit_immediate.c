// _Exit termina na hora: sem handlers de atexit e sem descarregar o
// buffer de stdout (o que já saiu por '\n' fica; o resto se perde).
#include <stdio.h>
#include <stdlib.h>
// expect-out: visivel
// expect-exit: 4

static void handler(void) { printf("handler nao devia rodar\n"); }

int main() {
  atexit(handler);
  printf("visivel\n");
  printf("perdido");
  _Exit(4);
}
