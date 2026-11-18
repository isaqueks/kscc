// float por manipulação de bits: frexp/ldexp, nextafter, ulp e
// decomposição feitos à mão sobre o binary32 (union) e conferidos com a
// aritmética; percorre também todos os expoentes.
#include "fltout.h"

static float my_ldexp(float x, int e) {
  while (e > 0) { x *= 2; e--; }
  while (e < 0) { x /= 2; e++; }
  return x;
}

static float my_frexp(float x, int *e) {
  unsigned u = fl_bits(x);
  int ex = (u >> 23) & 0xFF;
  if (ex == 0) {                 // subnormal ou zero: normaliza multiplicando
    if ((u & 0x7FFFFFFF) == 0) { *e = 0; return x; }
    float m = my_frexp(x * 16777216.0f, e);
    *e -= 24;
    return m;
  }
  *e = ex - 126;
  return fl_from((u & 0x807FFFFF) | (126u << 23));
}

static float next_up(float x) {
  unsigned u = fl_bits(x);
  if (x != x || u == 0x7F800000) return x;
  if (u == 0x80000000) return fl_from(1);
  return fl_from((int)u >= 0 ? u + 1 : u - 1);
}

int main(void) {
  float xs[] = {1.0f, 0.1f, -3.75f, 1e-40f, 6e37f, 1e-45f, -0.0f};
  for (int i = 0; i < 7; i++) {
    int e;
    float m = my_frexp(xs[i], &e);
    float back = my_ldexp(m, e);
    float up = next_up(xs[i]);
    printf("x=");
    fl_hex(xs[i]);
    fl_hex(m);
    printf("e=%d ", e);
    fl_hex(back);
    fl_hex(up);
    fl_hex(up - xs[i]);           // ulp
    printf("%d\n", back == xs[i]);
  }
  unsigned h = 0;
  for (int e = -149; e <= 127; e++) {
    float p = my_ldexp(1.0f, e);
    float q = p * 1.5f + p / 3;
    h = h * 33u + fl_bits(q) + fl_bits(p - q);
  }
  printf("expoentes %08x\n", h);
  // dobra até inf e divide até 0
  float x = 1;
  int n = 0;
  while (x * 2 != x) { x *= 2; n++; }
  printf("ate inf: %d ", n);
  fl_hex(x);
  x = 1;
  n = 0;
  while (x / 2 != 0) { x /= 2; n++; }
  printf("ate 0: %d ", n);
  fl_hex(x);
  printf("\n");
  return 0;
}
