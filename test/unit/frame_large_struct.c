// Frame acima de 8191 bytes com struct por valor e retorno de struct: o
// buffer de retorno, as cópias de argumentos, os parâmetros em registrador
// e as locais ficam em offsets que não cabem em 14 bits (addl/ldw).
// cflags: -Wl,--stack=65536
#include "minirt.h"
typedef struct { int v[4]; } Q;

Q make(int a) {
  Q q;
  for (int i = 0; i < 4; i++)
    q.v[i] = a + i;
  return q;
}

int qsum(Q q) { return q.v[0] + q.v[1] + q.v[2] + q.v[3]; }

// Muitas locais depois dos parâmetros: os slots dos parâmetros recebidos
// em registrador ficam no fundo de um frame de ~12 KB.
int deep(int a, int b, Q q, int c) {
  int pad[3000];
  pad[0] = a;
  pad[2999] = b;
  Q r = make(c);
  return pad[0] + pad[2999] + qsum(q) + qsum(r);
}

Q deep_ret(int a) {
  char pad[10000];
  pad[9999] = 1;
  Q r = make(a + pad[9999]);
  return r;
}

int main() {
  int big[2100];
  big[0] = 1;
  big[2099] = 2;
  Q q = make(10);
  EQ(46, qsum(q));
  EQ(1 + 2 + 46 + 26, deep(1, 2, q, 5));
  EQ(34, qsum(deep_ret(6)));
  EQ(3, big[0] + big[2099]);
  return 0;
}
