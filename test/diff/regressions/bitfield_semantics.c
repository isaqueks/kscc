// Regressão: semântica de bitfields que o kscc divergia do gcc.
//  1. promoção: unsigned:12 promove a int (C11 6.3.1.1p2), então
//     -f.u < 0 é verdadeiro;
//  2. valor de uma atribuição a bitfield é o valor armazenado (truncado),
//     não o valor do lado direito;
//  3. pós-incremento/decremento devolve o valor antigo mesmo no wrap;
//  4. _Bool:1 lido como 0/1 (o kscc estendia o sinal: -1).
#include "kstest.h"

struct S {
  unsigned u : 12;
  int s : 5;
  _Bool b : 1;
  unsigned char c : 3;
  unsigned w : 32;
};

static int id(int x) { return x; }

int main(void) {
  struct S f = {4095, 15, 1, 7, 0xFFFFFFFFu};
  ks_put_kv("-f.u < 0", -f.u < 0);
  ks_put_kv("f.u - 4096 < 0", f.u - 4096 < 0);
  ks_put_kv("~f.c", ~f.c);
  ks_put_kv("f.c - 8 < 0", f.c - 8 < 0);
  ks_put_kv("f.w > 0", f.w > 0);
  ks_put_kv("(f.u = 5000)", (f.u = id(5000)));
  ks_put_kv("(f.s = 20)", (f.s = id(20)));
  ks_put_kv("(f.c = 9)", (f.c = id(9)));
  ks_put_kv("(f.b = 2)", (f.b = id(2)));
  ks_put_kv("f.b", f.b);
  f.u = 4095;
  ks_put_kv("f.u++", f.u++);
  ks_put_kv("f.u", f.u);
  ks_put_kv("f.u--", f.u--);
  ks_put_kv("f.u", f.u);
  f.s = 15;
  ks_put_kv("f.s++", f.s++);
  ks_put_kv("f.s", f.s);
  ks_put_kv("f.s--", f.s--);
  ks_put_kv("++f.s", ++f.s);
  f.c = 7;
  ks_put_kv("++f.c", ++f.c);
  ks_put_kv("f.c += 10", f.c += id(10));
  ks_put_kv("f.c", f.c);
  int x = f.u = id(4097);
  ks_put_kv("x", x);
  unsigned y = f.w = 0x80000000u;
  ks_put_kx("y", y);
  ks_put_kv("f.b ? 10 : 20", f.b ? 10 : 20);
  f.b = 0;
  ks_put_kv("f.b after 0", f.b);
  f.u = 4095;
  ks_put_kv("++f.u - 5000 < 0", ++f.u - 5000 < 0);
  ks_put_kv("(f.u = id(1)) - 2 < 0", (f.u = id(1)) - 2 < 0);
  ks_put_kv("(f.u += id(1)) - 5000 < 0", (f.u += id(1)) - 5000 < 0);
  ks_put_kv("f.u++ - 5000 < 0", f.u++ - 5000 < 0);
  ks_put_kv("f.u-- - 5000 < 0", f.u-- - 5000 < 0);
  ks_put_kv("f.w++ > 0", f.w++ > 0);
  return 0;
}
