// Struct de 64 byte(s) por valor (doc 02 §3.1 e §3.3): argumento,
// retorno (ponteiro oculto em r1), chamadas aninhadas f(g(x)), escalares antes e depois
// da struct (r1-r4 e pilha) e chamada por ponteiro de função.
#include "minirt.h"
typedef struct { unsigned char b[64]; } S;

S make(int seed) {
  S s;
  for (int i = 0; i < 64; i++)
    s.b[i] = seed + i;
  return s;
}

int sum(S s) {
  int t = 0;
  for (int i = 0; i < 64; i++)
    t += s.b[i];
  return t;
}

// Altera a cópia recebida e a devolve.
S inc(S s, int d) {
  for (int i = 0; i < 64; i++)
    s.b[i] += d;
  return s;
}

// a-d em r1-r4; s e e na pilha, nessa ordem.
int mixed(int a, int b, S s, int c, int d, int e) {
  return a + b * 10 + c * 100 + d * 1000 + e * 10000 + sum(s) * 100000;
}

int main() {
  S x = make(1);
  int base = 64 * (64 - 1) / 2;
  EQ(base + 64, sum(x));

  S y = inc(x, 2);
  EQ(1, x.b[0]);
  EQ(64, x.b[64 - 1]);
  EQ(3, y.b[0]);
  EQ(64 + 2, y.b[64 - 1]);

  EQ(base + 6 * 64, sum(inc(make(5), 1)));
  EQ(base + 3 * 64, sum(inc(inc(make(0), 1), 2)));
  EQ(7 + 64 - 1, make(7).b[64 - 1]);

  y = make(3);
  EQ(base + 3 * 64, sum(y));

  EQ(54321 + (base + 64) * 100000, mixed(1, 2, x, 3, 4, 5));
  EQ(54321 + (base + 2 * 64) * 100000, mixed(1, 2, make(2), 3, 4, 5));

  S (*fp)(int) = make;
  int (*fs)(S) = sum;
  EQ(base + 4 * 64, fs(fp(4)));
  return 0;
}
