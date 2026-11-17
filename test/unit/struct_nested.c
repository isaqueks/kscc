// Structs aninhadas: offsets acumulados, cópia de sub-struct, ponteiros
// para membros internos e struct aninhada por valor.
#include "minirt.h"
struct Inner { char c; short s; int i; };
struct Mid { int tag; struct Inner in[2]; char last; };
struct Outer { char head; struct Mid m; struct Inner tail; };

int inner_sum(struct Inner x) { return x.c + x.s + x.i; }

struct Mid make_mid(int t) {
  struct Mid m;
  m.tag = t;
  m.in[0].c = 1; m.in[0].s = 2; m.in[0].i = 3;
  m.in[1].c = 4; m.in[1].s = 5; m.in[1].i = 6;
  m.last = 'z';
  return m;
}

int main() {
  EQ(8, sizeof(struct Inner));
  EQ(24, sizeof(struct Mid));
  EQ(36, sizeof(struct Outer));
  EQ(4, (int)&((struct Outer *)0)->m);
  EQ(16, (int)&((struct Outer *)0)->m.in[1]);
  EQ(28, (int)&((struct Outer *)0)->tail);

  struct Outer o;
  o.head = 'h';
  o.m = make_mid(9);
  o.tail = o.m.in[1];
  EQ(9, o.m.tag);
  EQ(5, o.m.in[1].s);
  EQ('z', o.m.last);
  EQ(15, inner_sum(o.tail));
  EQ(6, inner_sum(o.m.in[0]));

  struct Inner *p = &o.m.in[0];
  p[1].i = 100;
  EQ(100, o.m.in[1].i);
  EQ(6, o.tail.i); // a cópia não é afetada

  short *ps = &o.m.in[1].s;
  *ps = -7;
  EQ(-7, o.m.in[1].s);
  EQ(15, inner_sum(make_mid(0).in[1]));
  EQ('h', o.head);
  return 0;
}
