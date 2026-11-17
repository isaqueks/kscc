#include "kstest.h"
// Nomes C externos que o ksas leria como registrador (zr, fp, sp, r0-r31,
// a0-a3, t0-t10, s0-s13, sem diferenciar maiúsculas) precisam ser
// escapados; antes davam erro de montagem ("e um registrador").
int fp = 1, sp = 2, zr = 3, r0 = 4, r31 = 5, R7 = 6, a0 = 7, A3 = 8;
int t10 = 9, s13 = 10, Fp = 11, s1[2] = {12, 13};
int *pa = &a0, *ps = &s1[1];
int r1(int x) { return x + 100; }
int t0(int x) { return r1(x) * 2; }
int (*fptr)(int) = t0;
// Parecidos, mas não são registradores no ksas: ficam como estão.
int r32 = 14, t11 = 15, s14 = 16, a4 = 17, r01 = 18;
int main(void) {
  ks_put_kv("regs", fp + sp + zr + r0 + r31 + R7 + a0 + A3);
  ks_put_kv("aliases", t10 + s13 + Fp + s1[0] + s1[1]);
  ks_put_kv("ptrs", *pa + *ps);
  ks_put_kv("calls", r1(1) + t0(2) + fptr(3));
  ks_put_kv("near", r32 + t11 + s14 + a4 + r01);
  return 0;
}
