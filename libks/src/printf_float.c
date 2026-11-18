// Liga a conversão de ponto flutuante do printf (printf.c). Este bloco só
// entra no link quando alguma unidade passa float/double a uma função
// variádica: o codegen emite `; kscc-need: __ks_weak_printf_float`, e o
// link (src/link.c, passo 5) troca o símbolo comum de printf.c, nulo, por
// esta definição.
#include <stdarg.h>

void __ks_printf_float(void *o, void *sp, int conv, va_list *ap);

void (*__ks_weak_printf_float)(void *o, void *sp, int conv, va_list *ap) = __ks_printf_float;
