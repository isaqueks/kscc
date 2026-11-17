// Struct como valor em expressões: atribuição encadeada, ?:, vírgula,
// literal composto, retorno usado como argumento direto, struct vazia e
// struct de 3 bytes com alinhamento 1 em endereço ímpar.
#include "minirt.h"
typedef struct { char a, b, c; } S3;
typedef struct { int x, y; } P;

S3 mk3(char a, char b, char c) { return (S3){a, b, c}; }
int s3sum(S3 s) { return s.a + s.b + s.c; }
P mkp(int x, int y) { P p = {x, y}; return p; }
P addp(P a, P b) { return mkp(a.x + b.x, a.y + b.y); }

struct { char pad; S3 s; } odd; // s em endereço ímpar

int main() {
  P a, b, c;
  a = b = c = mkp(1, 2);
  EQ(3, a.x + a.y);
  EQ(3, c.x + c.y);

  P d = 1 ? mkp(5, 6) : mkp(7, 8);
  EQ(11, d.x + d.y);
  P e = (a.x = 9, a);
  EQ(9, e.x);

  P f = addp(addp(a, b), (P){100, 200});
  EQ(110, f.x);
  EQ(204, f.y);

  odd.s = mk3(1, 2, 3);
  EQ(1, (int)&odd.s % 2);
  EQ(6, s3sum(odd.s));
  S3 t = mk3(-1, -2, 4);
  EQ(1, s3sum(t));
  EQ(3, sizeof(S3));
  return 0;
}
