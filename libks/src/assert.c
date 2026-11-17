// Falha de assert() (<assert.h>, doc 05 §2).
//
// O doc 05 pede mensagem + abort(). Aqui a saída é com código 1, sem
// rodar atexit: abort() na KSVM é `trap 1`, que o runner reporta como
// fault (código 70 + n e "fault: TRAP at pc=..."), misturando uma falha
// do programa com uma falha da VM. Código 1 é o que os testes e o
// diferencial com gcc esperam de um programa que se recusa a continuar.
#include <assert.h>
#include <ksvm/syscall.h>
#include <stdio.h>

_Noreturn void __ks_assert_fail(const char *expr, const char *file, int line) {
  fprintf(stderr, "%s:%d: assert(%s) falhou\n", file, line, expr);
  ks_sys_exit(1);
}
