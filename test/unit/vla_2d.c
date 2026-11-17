// VLA 2D: sizeof em tempo de execução, indexação com linhas de tamanho
// variável, ponteiro para VLA e VLA dentro de laço. (Parâmetro
// `int m[][n]` não é aceito pelo front-end do chibicc: n ainda não está
// no escopo; usa-se um ponteiro para VLA dentro da função.)
#include "minirt.h"

int trace(int n, void *p) {
  int (*m)[n] = p;
  int t = 0;
  for (int i = 0; i < n; i++)
    t += m[i][i];
  return t;
}

int sum_rows(int r, int c) {
  int a[r][c];
  EQ(r * c * 4, sizeof(a));
  EQ(c * 4, sizeof(a[0]));
  for (int i = 0; i < r; i++)
    for (int j = 0; j < c; j++)
      a[i][j] = i * 100 + j;
  int (*row)[c] = a + 1;
  int s = 0;
  for (int j = 0; j < c; j++)
    s += (*row)[j];
  return s;
}

int main() {
  int n = 4;
  int m[n][n];
  for (int i = 0; i < n; i++)
    for (int j = 0; j < n; j++)
      m[i][j] = i == j ? i + 1 : 0;
  EQ(10, trace(n, m));
  EQ(64, sizeof(m));

  EQ(100 * 3 + 3, sum_rows(2, 3));
  EQ(100 * 7 + 21, sum_rows(5, 7));

  int total = 0;
  for (int k = 1; k <= 5; k++) {
    char buf[k][k + 1];
    total += sizeof(buf);
    buf[k - 1][k] = (char)k;
    total += buf[k - 1][k];
  }
  EQ(2 + 6 + 12 + 20 + 30 + 15, total);
  return 0;
}
