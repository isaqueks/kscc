// float: produto de matrizes 4x4, eliminação de Gauss-Jordan com pivô
// parcial (inversa) e determinante; matrizes em arrays 2D, em structs e
// passadas por ponteiro.
#include "fltout.h"

typedef struct {
  float m[4][4];
} Mat;

static Mat mul(const Mat *a, const Mat *b) {
  Mat r;
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++) {
      float s = 0;
      for (int k = 0; k < 4; k++)
        s += a->m[i][k] * b->m[k][j];
      r.m[i][j] = s;
    }
  return r;
}

static float fabs_(float x) { return x < 0 ? -x : x; }

// Devolve o determinante; inv recebe a inversa.
static float invert(Mat a, Mat *inv) {
  float det = 1;
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      inv->m[i][j] = i == j;
  for (int c = 0; c < 4; c++) {
    int p = c;
    for (int r = c + 1; r < 4; r++)
      if (fabs_(a.m[r][c]) > fabs_(a.m[p][c]))
        p = r;
    if (a.m[p][c] == 0)
      return 0;
    if (p != c) {
      for (int k = 0; k < 4; k++) {
        float t = a.m[c][k]; a.m[c][k] = a.m[p][k]; a.m[p][k] = t;
        t = inv->m[c][k]; inv->m[c][k] = inv->m[p][k]; inv->m[p][k] = t;
      }
      det = -det;
    }
    float d = a.m[c][c];
    det *= d;
    for (int k = 0; k < 4; k++) {
      a.m[c][k] /= d;
      inv->m[c][k] /= d;
    }
    for (int r = 0; r < 4; r++) {
      if (r == c)
        continue;
      float f = a.m[r][c];
      for (int k = 0; k < 4; k++) {
        a.m[r][k] -= f * a.m[c][k];
        inv->m[r][k] -= f * inv->m[c][k];
      }
    }
  }
  return det;
}

static void show(const char *name, const Mat *m) {
  printf("%s\n", name);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++)
      fl_hex(m->m[i][j]);
    printf("| %8.3f %8.3f %8.3f %8.3f\n", m->m[i][0], m->m[i][1], m->m[i][2], m->m[i][3]);
  }
}

int main(void) {
  Mat a = {{{2, -1, 0, 3}, {1, 0.5f, -2, 1}, {0.25f, 4, 1, -1}, {3, 1, 1, 0.1f}}};
  Mat b;
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      b.m[i][j] = (float)(i * 4 + j + 1) / 7;
  Mat c = mul(&a, &b);
  show("a*b", &c);
  Mat inv;
  float det = invert(a, &inv);
  fl_put("det", det);
  show("inv(a)", &inv);
  Mat id = mul(&a, &inv);
  show("a*inv(a)", &id);

  // Hilbert 4x4: mal condicionada, sensível a cada arredondamento
  Mat h;
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      h.m[i][j] = 1.0f / (i + j + 1);
  det = invert(h, &inv);
  fl_put("det hilbert", det);
  show("inv(hilbert)", &inv);
  return 0;
}
