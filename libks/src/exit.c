// exit, _Exit e atexit da libks (doc 05 §2).
//
// _start (asm/crt0.ksasm) chama exit com o valor de main, então voltar de
// main também roda os handlers e descarrega stdout.
#include <ksvm/syscall.h>
#include <stdlib.h>

static void (*handlers[KS_ATEXIT_MAX])(void);
static int nhandlers;

// Descarga de stdout, posta por stdio.c quando o buffer recebe o primeiro
// byte. exit chama pelo ponteiro, e não fflush direto, para que um
// programa que nunca usa stdio não arraste o buffer de 128 bytes e o
// resto de stdio.c para a imagem (importa no AVR; ver a seleção por uso
// em src/link.c).
void (*__ks_stdio_flush)(void);

int atexit(void (*fn)(void)) {
  if (nhandlers == KS_ATEXIT_MAX)
    return -1;
  handlers[nhandlers++] = fn;
  return 0;
}

// Handlers em ordem inversa de registro (C11 7.22.4.4). O contador desce
// antes da chamada: um handler que chame exit não repete os anteriores.
_Noreturn void exit(int code) {
  while (nhandlers > 0)
    handlers[--nhandlers]();
  if (__ks_stdio_flush)
    __ks_stdio_flush();
  ks_sys_exit(code);
}

_Noreturn void _Exit(int code) {
  ks_sys_exit(code);
}
