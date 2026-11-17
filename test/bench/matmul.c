// Benchmark: multiplicação de matrizes 32x32 de int (índices 2D).
#include "kstest.h"

#define N 32
static int A[N][N], B[N][N], C[N][N];

int main(void) {
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++) {
      A[i][j] = (i * 3 + j) % 17 - 8;
      B[i][j] = (i + j * 5) % 13 - 6;
    }
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++) {
      int s = 0;
      for (int k = 0; k < N; k++)
        s += A[i][k] * B[k][j];
      C[i][j] = s;
    }
  unsigned h = 0;
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
      h = h * 31u + (unsigned)C[i][j];
  ks_put_kx("hash", h);
  ks_put_kv("C[5][7]", C[5][7]);
  return 0;
}
