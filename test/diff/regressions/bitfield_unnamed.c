#include "kstest.h"
struct A { int a; int : 0; int b; };
struct B { char a; int : 3; char b; };
struct C { char a; int : 0; char b; };
struct D { int a; unsigned : 4; int b : 4; int c; };
struct A ga = {1, 2};
struct D gd = {5, 6, 7};
int main(void) {
  struct A la = {3, 4};
  struct D ld = {-1, -2, -3};
  ks_put_kv("sa", (int)sizeof(struct A));
  ks_put_kv("sb", (int)sizeof(struct B));
  ks_put_kv("sc", (int)sizeof(struct C));
  ks_put_kv("sd", (int)sizeof(struct D));
  ks_put_kv("ga.b", ga.b);
  ks_put_kv("la.b", la.b);
  ks_put_kv("gd", gd.a * 100 + gd.b * 10 + gd.c);
  ks_put_kv("ld", ld.a * 100 + ld.b * 10 + ld.c);
  return 0;
}
