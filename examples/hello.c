// hello.c — o menor programa com a libks.
//
//   build/kscc examples/hello.c -o hello.ksx && ../ksvm2/build/tools/ksvm hello.ksx
//
// expect-out: Olá, KSVM!
#include <stdio.h>

int main(void) {
  printf("Olá, %s!\n", "KSVM");
  return 0;
}
