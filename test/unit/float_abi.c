// ABI (doc 02 §3): float passa e volta como um int de 32 bits (r1-r4, pilha
// do 5º em diante, retorno em r1); varargs: 4 bytes na pilha (float é
// promovido a double, que também é binary32); struct com floats é igual a
// struct com ints; ponteiros, arrays e globais.
#include <stdarg.h>
#include "float_test.h"

typedef struct { float x, y; } V2;
typedef struct { float v; } F1;
typedef struct { char c; float f; short s; } Mix;

static float add6(float a, int b, float c, double d, float e, long double f) {
  return a + b + c + d + e + f;
}
static float pick(int n, float a, float b, float c, float d, float e, float f) {
  float v[6] = {a, b, c, d, e, f};
  return v[n];
}
static V2 v2add(V2 a, V2 b) { return (V2){a.x + b.x, a.y + b.y}; }
static F1 f1neg(F1 a) { a.v = -a.v; return a; }
static float mixsum(Mix m) { return m.c + m.f + m.s; }
static double vsum(int n, ...) {
  va_list ap;
  va_start(ap, n);
  double s = 0;
  for (int i = 0; i < n; i++)
    s += va_arg(ap, double);
  va_end(ap);
  return s;
}
static float vmix(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  float s = 0;
  for (; *fmt; fmt++)
    s += *fmt == 'i' ? va_arg(ap, int) : va_arg(ap, double);
  va_end(ap);
  return s;
}
static float fact(float n) { return n <= 1 ? 1 : n * fact(n - 1); }
static float apply(float (*fn)(float), float x) { return fn(x); }
static float half(float x) { return x / 2; }
static void scale(float *p, int n, float k) { for (int i = 0; i < n; i++) p[i] *= k; }

float garr[4] = {1.5f, -2.0f, 0.25f, 1e-3f};
V2 gv = {3, 4};
float *gp = &garr[2];

int main() {
  EQB(0x41500000, add6(1, 2, 3, 4, 2.5f, 0.5L));   // 13
  EQB(0x40A00000, pick(4, 1, 2, 3, 4, 5, 6));       // 5º argumento: pilha
  EQB(0x40C00000, pick(5, 1, 2, 3, 4, 5, 6));
  EQB(0x3F800000, pick(0, 1, 2, 3, 4, 5, 6));
  V2 r = v2add((V2){1.5f, 2}, gv);
  EQB(0x40900000, r.x);
  EQB(0x40C00000, r.y);
  F1 f = f1neg((F1){2});                            // struct de 4 bytes: volta em r1
  EQB(0xC0000000, f.v);
  EQB(0x40E00000, mixsum((Mix){1, 2.5f, 3}) + 0.5f);
  EQB(0x40C00000, vsum(3, 1.0, 2.0f, 3.0));
  EQB(0x41200000, vmix("ifif", 1, 2.0f, 3, 4.0));
  EQB(0x48B13000, fact(9));                         // 362880
  EQB(0x3FC00000, apply(half, 3));
  float (*fp)(float) = half;
  EQB(0xBF000000, fp(-1));
  scale(garr, 4, 2);
  EQB(0x40400000, garr[0]);
  EQB(0xC0800000, garr[1]);
  EQB(0x3F000000, *gp);
  EQB(0x3B03126F, garr[3]);                         // 2e-3
  EQ(16, (int)sizeof(garr));
  EQ(8, (int)sizeof(V2));
  EQ(12, (int)sizeof(Mix));
  return 0;
}
