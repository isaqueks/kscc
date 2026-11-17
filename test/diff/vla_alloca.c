// VLA (1D, 2D, em laço, sizeof em tempo de execução, como parâmetro) e
// __builtin_alloca (alloca em laço, junto com chamadas).
#include "kstest.h"

// kscc: parâmetro VLA (`int a[n][m]`) não é suportado (limitação herdada do
// chibicc: os parâmetros anteriores não estão em escopo no declarador);
// usa ponteiro para VLA local.
static int sum_vla(int n, int m, void *p) {
  int (*a)[m] = p;
  int s = 0;
  for (int i = 0; i < n; i++)
    for (int j = 0; j < m; j++)
      s += a[i][j] * (i - j);
  return s;
}

static unsigned fill_and_hash(int n) {
  int v[n];
  for (int i = 0; i < n; i++)
    v[i] = i * i - n;
  unsigned h = (unsigned)sizeof v;
  for (int i = 0; i < n; i++)
    h = h * 131u + (unsigned)v[i];
  return h;
}

static int *pick(int *a, int k) { return a + k; }

static int alloca_sum(int n) {
  int total = 0;
  for (int round = 1; round <= 3; round++) {
    int *p = __builtin_alloca((unsigned)(n * round) * sizeof(int));
    for (int i = 0; i < n * round; i++)
      p[i] = i ^ round;
    total += *pick(p, n * round - 1) + p[0];
  }
  return total;
}

static int matrix_trace(int n) {
  int m[n][n];
  for (int i = 0; i < n; i++)
    for (int j = 0; j < n; j++)
      m[i][j] = (i + 1) * (j + 2);
  int t = 0;
  for (int i = 0; i < n; i++)
    t += m[i][i];
  return t + (int)sizeof m + (int)sizeof m[0];
}

int main(void) {
  for (int n = 1; n <= 40; n += 13)
    ks_put_kx("fill", fill_and_hash(n));
  int rows = 3, cols = 5;
  int grid[rows][cols];
  for (int i = 0; i < rows; i++)
    for (int j = 0; j < cols; j++)
      grid[i][j] = i * cols + j;
  ks_put_kv("sum_vla", sum_vla(rows, cols, grid));
  int (*gp)[cols] = grid;
  ks_put_kv("gp[2][3]", gp[2][3]);
  ks_put_kv("sizeof *gp", (int)sizeof *gp);
  gp++;
  ks_put_kv("gp[0][0] after ++", gp[0][0]);
  ks_put_kv("sizeof grid", (int)sizeof grid);
  ks_put_kv("sizeof grid[0]", (int)sizeof grid[0]);
  for (int n = 1; n < 8; n += 3)
    ks_put_kv("trace", matrix_trace(n));
  ks_put_kv("alloca", alloca_sum(17));
  // VLA dentro de laço: cada iteração cria e descarta
  unsigned acc = 0;
  for (int k = 1; k <= 50; k++) {
    char buf[k];
    for (int i = 0; i < k; i++)
      buf[i] = (char)('a' + (i + k) % 26);
    acc = acc * 31u + (unsigned char)buf[k - 1] + (unsigned)sizeof buf;
  }
  ks_put_kx("loop vla", acc);
  // alloca de char usado como string
  char *s = __builtin_alloca(16);
  for (int i = 0; i < 15; i++)
    s[i] = (char)('A' + i);
  s[15] = 0;
  ks_put_str(s);
  ks_put_chr('\n');
  return 0;
}
