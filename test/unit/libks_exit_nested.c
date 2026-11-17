// exit chamado de dentro de um handler de atexit: os handlers que faltam
// rodam uma vez só (o que chamou exit não se repete), o stdout é
// descarregado no fim e o código de saída é o do exit de dentro.
#include <stdio.h>
#include <stdlib.h>
// expect-out: main
// expect-out: [h3][h2][h1 1]
// expect-noeol
// expect-exit: 5

static int runs;

static void h1(void) { printf("[h1 %d]", ++runs); }
static void h2(void) {
  printf("[h2]");
  exit(5);
}
static void h3(void) { printf("[h3]"); }

int main() {
  atexit(h1);
  atexit(h2);
  atexit(h3);
  printf("main\n");
  exit(9);
}
