#include "kstest.h"
// Tipo enumerado sem valores negativos é unsigned int, como no gcc e no
// clang (escolha da implementação, C11 6.7.2.2p4). Com o enum signed, um
// campo `enum Color c : 2` guardando W (3) era lido como -1 e `x.c == W`
// dava falso. As constantes continuam int.
enum Color { R, G, B, W };
enum Signed { NEG = -1, POS = 1 };
struct X { enum Color c : 2; unsigned other : 6; enum Signed s : 2; };
int main(void) {
  struct X x = { W, 1, POS };
  ks_put_kv("x.c == W", x.c == W);
  ks_put_kv("x.c", (int)x.c);
  ks_put_kv("x.c > G", x.c > G);
  x.c = B;
  x.s = NEG;
  ks_put_kv("x.c == B", x.c == B);
  ks_put_kv("x.s", (int)x.s);
  enum Color e = R;
  ks_put_kv("R - 1 < 0", R - 1 < 0);
  ks_put_kv("e - 1 > 0", e - 1 > 0);
  enum Signed n = NEG;
  ks_put_kv("n < 0", n < 0);
  ks_put_kv("sizeof", (int)sizeof(enum Color));
  return 0;
}
