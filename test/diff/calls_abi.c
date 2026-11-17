// Convenção de chamada: muitos argumentos misturando escalares pequenos e
// structs de tamanhos 1, 3, 5, 6, 12 (não múltiplos de 4), retornos de
// struct por ponteiro de função, varargs com structs ímpares, argumentos
// que são chamadas aninhadas e recursão com argumentos na pilha.
#include "kstest.h"
#include <stdarg.h>

typedef struct { char a; } S1;
typedef struct { char a, b, c; } S3;
typedef struct { char a[5]; } S5;
typedef struct { short a, b, c; } S6;
typedef struct { int a; short b; char c; int d; } S12;

static S1 mk1(int x) { return (S1){(char)x}; }
static S3 mk3(int x) { return (S3){(char)x, (char)(x + 1), (char)(x + 2)}; }
static S5 mk5(int x) {
  S5 s;
  for (int i = 0; i < 5; i++) s.a[i] = (char)(x * (i + 1));
  return s;
}
static S6 mk6(int x) { return (S6){(short)x, (short)-x, (short)(x * 100)}; }
static S12 mk12(int x) { return (S12){x, (short)(x >> 1), (char)x, ~x}; }

static int mixed(char c, S3 s3, short h, S1 s1, int i, S5 s5, unsigned char u, S6 s6, S12 s12, int last) {
  unsigned r = (unsigned)c; // unsigned: o hash pode dar a volta
  r = r * 7 + s3.a + s3.b * 2 + s3.c * 3;
  r = r * 7 + h;
  r = r * 7 + s1.a;
  r = r * 7 + i;
  for (int k = 0; k < 5; k++) r = r * 3 + s5.a[k];
  r = r * 7 + u;
  r = r * 7 + s6.a + s6.b + s6.c;
  r = r * 7 + s12.a + s12.b + s12.c + s12.d;
  return (int)(r * 7 + (unsigned)last);
}

static int ten(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j) {
  return a - b + c * 2 - d + e * 3 - f + g * 4 - h + i * 5 - j;
}

static int deep(int n, int a, int b, int c, int d, int e, int f) {
  if (n == 0) return a + b + c + d + e + f;
  return deep(n - 1, b, c, d, e, f, a + n) + 1;
}

static int va_structs(int n, ...) {
  va_list ap;
  va_start(ap, n);
  int r = 0;
  for (int i = 0; i < n; i++) {
    switch (i % 4) {
    case 0: { S3 s = va_arg(ap, S3); r = r * 5 + s.a + s.b + s.c; break; }
    case 1: { S5 s = va_arg(ap, S5); r = r * 5 + s.a[0] + s.a[4]; break; }
    case 2: r = r * 5 + va_arg(ap, int); break;
    default: { S6 s = va_arg(ap, S6); r = r * 5 + s.a + s.c; break; }
    }
  }
  va_end(ap);
  return r;
}

typedef S12 (*maker12)(int);
typedef S3 (*maker3)(int);

static S12 apply12(maker12 f, int x) { return f(x); }

int main(void) {
  S3 s3 = mk3(10);
  S5 s5 = mk5(3);
  S6 s6 = mk6(7);
  S12 s12 = mk12(1000);
  ks_put_kv("mixed", mixed('x', s3, -300, mk1(-5), 123456, s5, 200, s6, s12, -1));
  ks_put_kv("mixed2", mixed((char)mk1(65).a, mk3(-3), (short)mk6(2).c, mk1(1), ten(1, 2, 3, 4, 5, 6, 7, 8, 9, 10),
                            mk5(-1), (unsigned char)mk3(250).c, mk6(-9), mk12(-77), mk12(5).d));
  ks_put_kv("ten", ten(ten(1, 1, 1, 1, 1, 1, 1, 1, 1, 1), 2, 3, 4, 5, 6, 7, 8, 9, ten(0, 0, 0, 0, 0, 0, 0, 0, 0, 1)));
  ks_put_kv("deep", deep(20, 1, 2, 3, 4, 5, 6));
  ks_put_kv("va", va_structs(8, mk3(1), mk5(2), 3, mk6(4), mk3(5), mk5(6), 7, mk6(8)));
  maker12 m12 = mk12;
  maker3 m3 = mk3;
  S12 r12 = apply12(m12, 31);
  ks_put_kv("r12", r12.a + r12.b + r12.c + r12.d);
  ks_put_kv("m3", m3(40).b + (*m3)(50).c);
  ks_put_kv("chain", mk12(mk3(mk1(9).a).c).d);
  S6 arr6[3] = {mk6(1), mk6(2), mk6(3)};
  arr6[1] = arr6[2];
  ks_put_kv("arr6", arr6[0].c + arr6[1].c + arr6[2].b);
  return 0;
}
