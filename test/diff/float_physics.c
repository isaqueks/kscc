// float: simulações com integração de Euler e Verlet (projétil com arrasto,
// sistema massa-mola, órbita), estado em structs passadas por valor e
// devolvidas por valor.
#include "fltout.h"

typedef struct {
  float x, y, vx, vy;
} Body;

static Body step_drag(Body b, float dt, float k) {
  float ax = -k * b.vx;
  float ay = -9.81f - k * b.vy;
  b.x += b.vx * dt;
  b.y += b.vy * dt;
  b.vx += ax * dt;
  b.vy += ay * dt;
  return b;
}

static void spring(float *x, float *v, float dt, int n) {
  // Verlet de velocidade: x'' = -4x
  float a = -4 * *x;
  for (int i = 0; i < n; i++) {
    *x += *v * dt + 0.5f * a * dt * dt;
    float na = -4 * *x;
    *v += 0.5f * (a + na) * dt;
    a = na;
  }
}

int main(void) {
  Body b = {0, 0, 30, 40};
  int t = 0;
  while (b.y >= 0 && t < 1000) {
    b = step_drag(b, 0.01f, 0.05f);
    if (t % 50 == 0) {
      printf("t=%4d ", t);
      fl_hex(b.x);
      fl_hex(b.y);
      fl_hex(b.vx);
      fl_hex(b.vy);
      printf("%.2f %.2f\n", b.x, b.y);
    }
    t++;
  }
  printf("impacto em t=%d x=%.3f\n", t, b.x);

  float x = 1, v = 0;
  for (int i = 0; i < 10; i++) {
    spring(&x, &v, 0.001f, 157);
    printf("mola ");
    fl_hex(x);
    fl_hex(v);
    printf("E=%.5f\n", 2 * x * x + 0.5f * v * v);
  }

  // órbita circular (GM = 1), passo simplético
  Body o = {1, 0, 0, 1};
  for (int i = 0; i < 2000; i++) {
    float r2 = o.x * o.x + o.y * o.y;
    float r3 = r2 * __builtin_sqrtf(r2);
    o.vx -= o.x / r3 * 0.005f;
    o.vy -= o.y / r3 * 0.005f;
    o.x += o.vx * 0.005f;
    o.y += o.vy * 0.005f;
  }
  fl_put("orbita x", o.x);
  fl_put("orbita y", o.y);
  printf("%.4f %.4f\n", o.x, o.y);
  return 0;
}
