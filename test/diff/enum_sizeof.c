// enum (valores implícitos, negativos, repetidos, em switch e como índice)
// e sizeof/offsetof/_Alignof de tipos básicos e structs com padding (ILP32
// nos dois alvos).
#include "kstest.h"
#include <stddef.h>

enum Color { RED, GREEN = 5, BLUE, ALPHA = -3, BETA, SAME = BLUE, LAST = 0x7FFFFFFF };
enum Small { S0, S1, S2, S_COUNT };

struct A { char c; int i; };
struct B { char c; short s; char d; };
struct C { short s; char c[3]; };
struct D { char c; struct A a; char e; };
struct E { int i; char c; };
struct F { struct B b[3]; char tail; };
union U { char c[5]; int i; short s; };
struct G { char c; union U u; };
struct H { unsigned char flag; void *p; unsigned short n; };
typedef struct { int x; char name[7]; } Named;

static const char *small_name[S_COUNT] = {[S2] = "dois", [S0] = "zero", [S1] = "um"};

static int color_score(enum Color c) {
  switch (c) {
  case RED: return 1;
  case GREEN: return 2;
  case BLUE: return 3;
  case ALPHA: return 4;
  case BETA: return 5;
  default: return 0;
  }
}

#define SZ(t) ks_put_kv("sizeof(" #t ")", (int)sizeof(t))
#define AL(t) ks_put_kv("_Alignof(" #t ")", (int)_Alignof(t))
#define OFF(t, m) ks_put_kv("offsetof(" #t ", " #m ")", (int)offsetof(t, m))

int main(void) {
  ks_put_kv("RED", RED);
  ks_put_kv("GREEN", GREEN);
  ks_put_kv("BLUE", BLUE);
  ks_put_kv("ALPHA", ALPHA);
  ks_put_kv("BETA", BETA);
  ks_put_kv("SAME", SAME);
  ks_put_kv("LAST", LAST);
  ks_put_kv("sizeof(enum Color)", (int)sizeof(enum Color));
  enum Color c = BETA;
  c++;
  ks_put_kv("BETA+1", c);
  ks_put_kv("score", color_score(RED) * 10000 + color_score(GREEN) * 1000 + color_score(SAME) * 100 +
                         color_score(ALPHA) * 10 + color_score((enum Color)42));
  for (enum Small s = S0; s < S_COUNT; s++) {
    ks_put_str(small_name[s]);
    ks_put_chr(' ');
  }
  ks_put_chr('\n');
  ks_put_kv("ALPHA < RED", ALPHA < RED);
  SZ(char); SZ(short); SZ(int); SZ(long); SZ(void *); SZ(size_t); SZ(ptrdiff_t);
  SZ(struct A); SZ(struct B); SZ(struct C); SZ(struct D); SZ(struct E); SZ(struct F);
  SZ(union U); SZ(struct G); SZ(struct H); SZ(Named); SZ(Named[3]);
  AL(char); AL(short); AL(int); AL(long); AL(struct B); AL(struct C); AL(union U); AL(struct H);
  OFF(struct A, i); OFF(struct B, s); OFF(struct B, d); OFF(struct C, c);
  OFF(struct D, a); OFF(struct D, e); OFF(struct D, a.i); OFF(struct F, tail);
  OFF(struct F, b[2].d); OFF(struct G, u); OFF(struct H, p); OFF(struct H, n); OFF(Named, name);
  int arr[17];
  ks_put_kv("sizeof arr", (int)sizeof arr);
  ks_put_kv("count", (int)(sizeof arr / sizeof *arr));
  ks_put_kv("sizeof \"abc\"", (int)sizeof "abc");
  ks_put_kv("sizeof 'a'", (int)sizeof 'a');
  ks_put_kv("sizeof (char)1 + 1", (int)sizeof((char)1 + 1));
  ks_put_kv("sizeof 1L", (int)sizeof 1L);
  ks_put_kv("sizeof 1u", (int)sizeof 1u);
  char ch = 0;
  ks_put_kv("sizeof ch++ (nao avalia)", (int)sizeof ch++);
  ks_put_kv("ch", ch);
  return 0;
}
