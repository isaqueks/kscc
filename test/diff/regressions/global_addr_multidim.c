// Regressão (Csmith): endereço de elemento de array multidimensional global
// como inicializador constante (`int *p = &g[0][1][3];`) dava "not a
// compile-time constant".
#include "kstest.h"

static int g1[9] = {1, 2, 3, 4, 5};
static int *p1 = &g1[3];
static int g2[3][9] = {{1}, {2, 3, 4, 5, 6}};
static int *p2 = &g2[1][3];
static int g3[2][3][9] = {{{1}, {7, 8, 9, 10, 11}}, {{0}, {0}, {1, 2, 3}}};
static int *p3 = &g3[0][1][3];
static int *p4[] = {&g3[1][2][2], &g2[2][0], g2[1], &g3[0][0][0] + 5, *g2 + 1};
static int (*row)[9] = &g2[1];
static int (*plane)[3][9] = &g3[1];
struct S { int a[2][4]; char c; } gs[3] = {{{{1, 2, 3, 4}, {5, 6, 7, 8}}, 'x'}};
static int *p5 = &gs[0].a[1][2];
static char *p6 = &gs[2].c;

int main(void) {
  ks_put_kv("p1", (int)(p1 - g1) * 100 + *p1);
  ks_put_kv("p2", (int)(p2 - g2[0]) * 100 + *p2);
  ks_put_kv("p3", (int)(p3 - g3[0][0]) * 100 + *p3);
  for (int i = 0; i < 5; i++)
    ks_put_kv("p4", *p4[i]);
  ks_put_kv("p4 idx", (int)(p4[0] - &g3[0][0][0]) * 1000 + (int)(p4[1] - &g2[0][0]) * 100 +
                          (int)(p4[2] - &g2[0][0]) * 10 + (int)(p4[4] - &g2[0][0]));
  ks_put_kv("row", (*row)[4]);
  ks_put_kv("plane", (*plane)[2][2]);
  ks_put_kv("p5", *p5);
  ks_put_kv("p6", (int)(p6 - (char *)gs));
  return 0;
}
