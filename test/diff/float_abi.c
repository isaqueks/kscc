// float na convenção de chamada: muitos argumentos (registradores e
// pilha), misturados com inteiros, structs com floats por valor e
// devolvidas, ponteiros de função, recursão e varargs (o float promovido
// a double é lido com va_arg(ap, double) e voltado a float logo em seguida:
// no gcc o double é binary64, mas o valor é o mesmo).
#include <stdarg.h>
#include "fltout.h"

typedef struct { float re, im; } Cpx;
typedef struct { char tag; float v[3]; short n; } Rec;

static Cpx cmul(Cpx a, Cpx b) { return (Cpx){a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re}; }
static Cpx cadd(Cpx a, Cpx b) { Cpx r = {a.re + b.re, a.im + b.im}; return r; }

static float many(float a, int b, float c, unsigned char d, float e, short f, float g, float h, int i, float j) {
  return ((a - b) * c + d) / e - f * g + h * i - j;
}

static Rec mkrec(char t, float x) {
  Rec r = {t, {x, x * 2, x / 3}, (short)(t * 2)};
  return r;
}

static float recsum(Rec r) { return r.v[0] + r.v[1] + r.v[2] + r.n + r.tag; }

static float vsum(const char *types, ...) {
  va_list ap;
  va_start(ap, types);
  float s = 0;
  for (const char *p = types; *p; p++) {
    if (*p == 'f') {
      float v = (float)va_arg(ap, double);
      s = s * 0.5f + v;
    } else {
      s += va_arg(ap, int);
    }
  }
  va_end(ap);
  return s;
}

static float power(float x, int n) {
  if (n == 0)
    return 1;
  float h = power(x, n / 2);
  return n % 2 ? h * h * x : h * h;
}

static float twice(float x) { return x * 2; }
static float neg(float x) { return -x; }
static float addq(float x) { return x + 0.25f; }

int main(void) {
  fl_put("many", many(1.5f, 2, 3.25f, 200, 0.75f, -3, 1e-3f, 7.5f, 11, 0.1f));
  Cpx z = {0, 0}, c = {-0.4f, 0.6f};
  for (int i = 0; i < 20; i++)
    z = cadd(cmul(z, z), c);
  fl_put("z.re", z.re);
  fl_put("z.im", z.im);
  Rec r = mkrec(7, 1.1f);
  fl_put("recsum", recsum(r));
  fl_put("recv2", r.v[2]);
  fl_put("vsum", vsum("fifiiff", 1.5f, 3, 0.1f, -2, 5, 1e-7f, -2.5f));
  fl_put("vsum2", vsum("ffffffffff", 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f));
  for (int n = 0; n < 40; n += 7) {
    printf("pow %d ", n);
    fl_hex(power(1.1f, n));
    fl_hex(power(-0.9f, n));
    printf("\n");
  }
  float (*fs[])(float) = {twice, neg, addq, twice, addq};
  float v = 0.3f;
  for (int i = 0; i < 20; i++)
    v = fs[i % 5](v);
  fl_put("fptr", v);
  printf("%.3f %.3f %.3f\n", z.re, z.im, v);
  return 0;
}
