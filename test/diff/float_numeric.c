// float: algoritmos numéricos clássicos (Newton para raiz, séries de pi e
// e, soma de Kahan, Horner, bisseção) com a mesma ordem de operações nos
// dois compiladores: os bits de cada passo têm de ser idênticos.
#include <math.h>
#include "fltout.h"

static float newton_sqrt(float a) {
  float x = a > 1 ? a / 2 : 1;
  for (int i = 0; i < 30; i++) {
    float nx = 0.5f * (x + a / x);
    if (nx == x)
      break;
    x = nx;
  }
  return x;
}

static float horner(const float *c, int n, float x) {
  float r = 0;
  for (int i = n - 1; i >= 0; i--)
    r = r * x + c[i];
  return r;
}

static float poly(float x) { return x * x * x - 2 * x - 5; }

int main(void) {
  float as[] = {2, 3, 10, 0.25f, 1e-6f, 12345.678f, 1e30f};
  for (int i = 0; i < 7; i++) {
    float s = newton_sqrt(as[i]);
    printf("sqrt %g: ", as[i]);
    fl_hex(s);
    fl_hex(sqrtf(as[i]));
    printf("%.4g\n", s);
  }

  // Leibniz e Nilakantha para pi; série de e
  float pi1 = 0, pi2 = 3, e = 1, term = 1;
  for (int k = 0; k < 2000; k++)
    pi1 += (k % 2 ? -4.0f : 4.0f) / (2 * k + 1);
  for (int k = 1; k < 200; k++) {
    float d = (2.0f * k) * (2.0f * k + 1) * (2.0f * k + 2);
    pi2 += (k % 2 ? 4.0f : -4.0f) / d;
  }
  for (int k = 1; k < 15; k++) {
    term /= k;
    e += term;
  }
  fl_put("pi leibniz", pi1);
  fl_put("pi nilakantha", pi2);
  fl_put("e", e);
  printf("%.5f %.6f %.6f\n", pi1, pi2, e);

  // soma ingênua x Kahan de 0.1 dez mil vezes
  float naive = 0, sum = 0, c = 0;
  for (int i = 0; i < 10000; i++) {
    naive += 0.1f;
    float y = 0.1f - c;
    float t = sum + y;
    c = (t - sum) - y;
    sum = t;
  }
  fl_put("naive", naive);
  fl_put("kahan", sum);
  printf("%.3f %.3f\n", naive, sum);

  float coef[] = {1, -3, 0.5f, 2, -0.25f};
  for (float x = -2; x <= 2; x += 0.375f) {
    printf("horner %g ", x);
    fl_hex(horner(coef, 5, x));
    printf("\n");
  }

  // bisseção de x^3 - 2x - 5
  float lo = 2, hi = 3;
  for (int i = 0; i < 40; i++) {
    float mid = (lo + hi) / 2;
    if (poly(mid) > 0)
      hi = mid;
    else
      lo = mid;
  }
  fl_put("raiz", lo);
  fl_put("raiz hi", hi);
  printf("%.6f\n", lo);
  return 0;
}
