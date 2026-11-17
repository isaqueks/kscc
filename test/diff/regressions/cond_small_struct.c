#include "kstest.h"
// ?: entre structs/unions de menos de 4 bytes: a promoção inteira das
// conversões aritméticas era aplicada às structs, e o valor da expressão
// virava o endereço delas com tipo int (inicialização, passagem por
// valor e return davam lixo; `(c ? x : y).b` era rejeitado).
struct S1 { char a; };
struct S3 { char a, b, c; };
union U2 { short s; char c; };
static int take1(struct S1 s, int k) { return s.a * 10 + k; }
static int take3(int k, struct S3 s) { return s.a * 100 + s.b * 10 + s.c + k * 1000; }
static struct S3 pick(int c, struct S3 x, struct S3 y) { return c ? x : y; }
static union U2 pu(int c, union U2 x, union U2 y) { return c ? x : y; }
int main(void) {
  volatile int c = 1;
  struct S1 a = { 4 }, b = { 5 };
  struct S3 x = { 1, 2, 3 }, y = { 7, 8, 9 };
  union U2 u = { .s = 300 }, v = { .s = 400 };
  struct S1 r = c ? a : b;
  ks_put_kv("init", r.a);
  r = !c ? a : b;
  ks_put_kv("assign", r.a);
  ks_put_kv("take1", take1(c ? a : b, 1));
  ks_put_kv("take3", take3(2, !c ? x : y));
  struct S3 p = pick(1, x, y);
  ks_put_kv("pick", p.a * 100 + p.b * 10 + p.c);
  ks_put_kv("pu", pu(0, u, v).s);
  ks_put_kv("member", (c ? x : y).b);
  ks_put_kv("sizeof", (int)sizeof(c ? x : y));
  return 0;
}
