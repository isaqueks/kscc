// Quais globais vão para .rodata: const T, array de const, T *const e
// struct const. `const char *p` (ponteiro para const) continua gravável,
// assim como struct com membro const. O teste termina escrevendo em uma
// struct const, o que dá FAULT_MEM_PERM.
// expect-exit: 75
#include "minirt.h"
struct S { int a, b; };
struct M { const int k; int v; };

int x = 1;
const char *msg = "abc";          // o ponteiro é gravável
int *const px = &x;               // o ponteiro é const
const struct S cs = {3, 4};
struct M m = {7, 8};              // membro const, objeto gravável
static const short sh[2] = {-1, -2};

int main() {
  msg = "de";
  EQ('d', msg[0]);
  *px = 5;
  EQ(5, x);
  m.v = 9;
  EQ(9, m.v);
  EQ(7, m.k);
  EQ(-3, sh[0] + sh[1]);
  EQ(7, cs.a + cs.b);
  struct S *p = (struct S *)&cs;
  p->b = 0;
  return 0;
}
