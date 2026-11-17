// Buffer de stdout: descarrega em '\n', ao encher (128 bytes), em fflush,
// antes de escrever em stderr e no fim de main (exit). A ordem relativa a
// ks_sys_puts, que escreve direto no console, mostra quando a descarga
// aconteceu.
#include <string.h>
#include "libks_test.h"
// expect-out: B
// expect-out: A
// expect-out: CD
// expect-out: EF
// expect-out: 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567|*89
// expect-out: G!H
// expect-out: fim sem quebra de linha
// expect-noeol
int main() {
  // "A" fica no buffer até o '\n'; "B\n" passa na frente.
  printf("A");
  ks_sys_puts("B\n", 2);
  printf("\n");

  // fflush explícito
  printf("C");
  fflush(stdout);
  ks_sys_puts("D\n", 2);

  // stderr descarrega stdout antes
  fputs("E", stdout);
  fputs("F\n", stderr);

  // 130 bytes sem '\n': os primeiros 128 saem quando o buffer enche
  for (int i = 0; i < 128; i++)
    putchar('0' + i % 10);
  ks_sys_puts("|*", 2);  // aparece logo depois dos 128
  printf("89\n");

  fputc('G', stdout);
  fputc('!', stderr);
  putc('H', stdout);
  putchar('\n');

  // Sem '\n' nem fflush: exit (via retorno de main) descarrega
  printf("fim sem quebra de linha");
  return 0;
}
