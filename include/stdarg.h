// kscc: va_list da KSVM (doc 02 §3.2). Substitui o __va_elem do x86-64.
//
// Funções variádicas recebem TODOS os argumentos na pilha, cada um em
// slots de 4 bytes, o mais à esquerda no endereço mais baixo. O último
// parâmetro nomeado está, portanto, imediatamente abaixo do primeiro
// argumento anônimo, e va_list pode ser um simples char*.
#ifndef __STDARG_H
#define __STDARG_H

typedef char *va_list;

#define __ks_va_slot(ty) ((sizeof(ty) + 3) & ~3)

#define va_start(ap, last) \
  ((ap) = (char *)&(last) + __ks_va_slot(last))

#define va_arg(ap, ty) \
  (*(ty *)(((ap) += __ks_va_slot(ty)) - __ks_va_slot(ty)))

#define va_end(ap) ((void)0)

#define va_copy(dest, src) ((dest) = (src))

#define __GNUC_VA_LIST 1
typedef va_list __gnuc_va_list;

#endif
