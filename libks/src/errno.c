// errno da libks: a global que os wrappers de syscall (asm/syscall.ksasm)
// e as funções da libks escrevem (ver <errno.h>).
//
// Definição provisória de propósito: vira um símbolo comum, e o marcador
// "; kscc-common:" que o codegen emite precisa atravessar o libks.kso e a
// seleção por uso do link (src/link.c). Um programa que também declare
// `int ks_errno;` acaba com uma única variável, como no Unix.
int ks_errno;
