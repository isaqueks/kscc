// Matrizes: multiplicação, transposição, potência, determinante inteiro
// (eliminação de Bareiss) e traço, com arrays 2D, ponteiros para linhas e
// matrizes dentro de structs passadas por valor.
#include "kstest.h"

#define N 6

typedef struct {
  int m[N][N];
} Mat;

static Mat mat_mul(Mat a, Mat b) {
  Mat r;
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++) {
      int s = 0;
      for (int k = 0; k < N; k++)
        s += a.m[i][k] * b.m[k][j];
      r.m[i][j] = s;
    }
  return r;
}

static void transpose(int (*m)[N]) {
  for (int i = 0; i < N; i++)
    for (int j = i + 1; j < N; j++) {
      int t = m[i][j];
      m[i][j] = m[j][i];
      m[j][i] = t;
    }
}

static Mat identity(void) {
  Mat r = {{{0}}};
  for (int i = 0; i < N; i++)
    r.m[i][i] = 1;
  return r;
}

// Potência por quadrados, mod 1000 nas entradas para não estourar.
static Mat mat_pow_mod(Mat a, unsigned e) {
  Mat r = identity();
  while (e) {
    if (e & 1u) {
      r = mat_mul(r, a);
      for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
          r.m[i][j] %= 1000;
    }
    a = mat_mul(a, a);
    for (int i = 0; i < N; i++)
      for (int j = 0; j < N; j++)
        a.m[i][j] %= 1000;
    e >>= 1;
  }
  return r;
}

// Determinante de Bareiss (divisões exatas), matriz pequena com entradas pequenas.
static int det(const int src[N][N], int n) {
  int a[N][N];
  for (int i = 0; i < n; i++)
    for (int j = 0; j < n; j++)
      a[i][j] = src[i][j];
  int sign = 1, prev = 1;
  for (int k = 0; k < n - 1; k++) {
    if (a[k][k] == 0) {
      int sw = -1;
      for (int i = k + 1; i < n; i++)
        if (a[i][k] != 0) { sw = i; break; }
      if (sw < 0) return 0;
      for (int j = 0; j < n; j++) {
        int t = a[k][j]; a[k][j] = a[sw][j]; a[sw][j] = t;
      }
      sign = -sign;
    }
    for (int i = k + 1; i < n; i++)
      for (int j = k + 1; j < n; j++)
        a[i][j] = (a[i][j] * a[k][k] - a[i][k] * a[k][j]) / prev;
    prev = a[k][k];
  }
  return sign * a[n - 1][n - 1];
}

static void print_mat(const char *name, const int (*m)[N]) {
  ks_put_str(name);
  ks_put_str(":\n");
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      ks_put_chr(' ');
      ks_put_int(m[i][j]);
    }
    ks_put_chr('\n');
  }
}

int main(void) {
  Mat a, b;
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++) {
      a.m[i][j] = (i * 7 + j * 3) % 5 - 2;
      b.m[i][j] = (i == j) * 2 + (j == i + 1) - (i == j + 2);
    }
  print_mat("a", (const int (*)[N])a.m);
  print_mat("b", (const int (*)[N])b.m);
  Mat c = mat_mul(a, b);
  print_mat("a*b", (const int (*)[N])c.m);
  transpose(c.m);
  print_mat("(a*b)^T", (const int (*)[N])c.m);
  // (a*b)^T == b^T * a^T
  transpose(a.m);
  transpose(b.m);
  Mat d = mat_mul(b, a);
  int same = 1;
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
      same &= c.m[i][j] == d.m[i][j];
  ks_put_kv("transpose identity", same);
  Mat f = {{{1, 1}, {1, 0}}};
  for (int i = 2; i < N; i++)
    f.m[i][i] = 1;
  Mat p = mat_pow_mod(f, 30);
  ks_put_kv("fib(30) mod 1000", p.m[0][1]);
  int trace = 0;
  for (int i = 0; i < N; i++)
    trace += p.m[i][i];
  ks_put_kv("trace", trace);
  int h[N][N];
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
      h[i][j] = (i + 1) * (j + 2) % 7 + (i == j) * 3;
  for (int n = 1; n <= N; n++) {
    ks_put_str("det");
    ks_put_int(n);
    ks_put_str(" = ");
    ks_put_int(det((const int (*)[N])h, n));
    ks_put_chr('\n');
  }
  ks_put_kv("sizeof(Mat)", (int)sizeof(Mat));
  ks_put_kv("sizeof(h[0])", (int)sizeof h[0]);
  return 0;
}
