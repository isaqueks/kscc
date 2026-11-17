// puts/fputs/fputc/putchar: valores devolvidos, stdout e stderr no mesmo
// console e na ordem do programa, stdin não é gravável.
#include <stdio.h>
#include "minirt.h"
// expect-out: um
// expect-out: dois tres
// expect-out: quatro
// expect-out: Z
// expect-out:
// expect-out: cinco
int main() {
  EQ(1, puts("um") >= 0);
  EQ(1, fputs("dois ", stdout) >= 0);
  EQ(1, fputs("tres\n", stderr) >= 0);
  EQ(1, fputs("", stdout) >= 0);
  fputs("quatro", stdout);
  EQ('\n', fputc('\n', stderr));
  EQ('Z', putchar('Z'));
  putchar('\n');
  puts("");
  EQ(EOF, fputc('x', stdin));
  EQ(EOF, fputs("x", stdin));
  EQ(0, fflush(stdout));
  EQ(0, fflush(0));
  EQ(0, fflush(stderr));
  printf("cinco\n");
  return 0;
}
