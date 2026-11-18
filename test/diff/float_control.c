// float em controle de fluxo e lvalues: contadores float em laços, ++/--
// (o valor antigo exato), atribuições compostas com inteiros e floats,
// ternário, &&/||, float como condição, switch sobre (int)f, membros de
// struct, elementos de array e *p.
#include "fltout.h"

struct P { int id; float w; float hist[4]; };

int main(void) {
  float x = 0;
  int n = 0;
  for (x = 0; x < 1; x += 0.1f)
    n++;
  printf("laco 0.1: %d ", n);
  fl_hex(x);
  printf("\n");
  n = 0;
  for (float y = 1e8f; y < 1e8f + 100; y++)   // y++ não sai do lugar em 1e8 (ulp 8): limitado
    if (++n > 20)
      break;
  printf("laco 1e8: %d\n", n);

  float a = 0.1f, old = a++;
  fl_put("old", old);
  fl_put("a", a);
  old = a--;
  fl_put("old2", old);
  fl_put("a2", a);
  float b = 16777216.0f;
  b++;
  fl_put("b++", b);
  ++b;
  fl_put("++b", b);
  b += 2;
  fl_put("b+=2", b);

  int k = 10;
  k *= 1.75f;     // 17
  k -= 0.5f;      // 16 (16.5 -> 16)
  k /= 3.0f;      // 5
  printf("k=%d\n", k);
  unsigned char c = 200;
  c += 55.9f;     // 255
  printf("c=%d\n", c);

  struct P p = {1, 0.5f, {0}};
  struct P *pp = &p;
  for (int i = 0; i < 12; i++) {
    pp->w *= 1.5f;
    p.hist[i % 4] += pp->w;
    float *h = &p.hist[(i + 1) % 4];
    (*h)++;
  }
  fl_put("w", p.w);
  for (int i = 0; i < 4; i++)
    fl_hex(p.hist[i]);
  printf("\n");

  float vals[] = {0.0f, -0.0f, 0.25f, -3.5f, 1e-45f, 7.9f};
  for (int i = 0; i < 6; i++) {
    float v = vals[i];
    int s = 0;
    if (v) s |= 1;
    if (!v) s |= 2;
    s |= (v ? 4 : 8);
    s |= (v && v > 0) << 4;
    s |= (v || v < 0) << 5;
    switch ((int)v) {
    case 0: s |= 64; break;
    case 7: s |= 128; break;
    case -3: s |= 256; break;
    default: s |= 512;
    }
    float t = v > 0 ? v * 2 : v < 0 ? -v : 1.0f;
    printf("v%d %03x ", i, s);
    fl_hex(t);
    printf("\n");
  }

  float acc = 1;
  int it = 0;
  do {
    acc /= 3;
    it++;
  } while (acc);
  printf("underflow em %d\n", it);
  return 0;
}
