// Membros com offset acima de 8191 (addl em gen_addr), struct grande
// copiada por atribuição, por valor e por retorno (laço de cópia).
// cflags: -Wl,--stack=131072
#include "minirt.h"
struct Big {
  int first;
  char pad[9000];
  int after;      // offset 9004
  short tail[3];  // offset 9008
};

struct Big gb;

int get_after(struct Big b) { return b.after + b.tail[2]; }

struct Big bump(struct Big b) {
  b.after++;
  b.tail[2] = 7;
  return b;
}

int main() {
  EQ(9004, (int)&((struct Big *)0)->after);
  EQ(9016, sizeof(struct Big));

  gb.first = 1;
  gb.after = 41;
  gb.tail[2] = 1;
  EQ(42, get_after(gb));

  struct Big l = bump(gb);
  EQ(42, l.after);
  EQ(7, l.tail[2]);
  EQ(41, gb.after);
  EQ(1, l.first);

  struct Big *p = &l;
  p->tail[0] = -3;
  EQ(-3, l.tail[0]);
  gb = l;
  EQ(49, get_after(gb));
  return 0;
}
