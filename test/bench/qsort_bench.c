// Benchmark: quicksort próprio (com comparador por ponteiro de função) de
// 2000 ints pseudoaleatórios.
#include "kstest.h"

#define N 2000
static int a[N];

static int cmp_int(const void *p, const void *q) {
  int x = *(const int *)p, y = *(const int *)q;
  return (x > y) - (x < y);
}

static void swap(int *x, int *y) {
  int t = *x;
  *x = *y;
  *y = t;
}

static void qs(int *v, int n, int (*cmp)(const void *, const void *)) {
  while (n > 1) {
    int p = n / 2, i = 0;
    swap(&v[p], &v[n - 1]);
    for (int j = 0; j < n - 1; j++)
      if (cmp(&v[j], &v[n - 1]) < 0)
        swap(&v[i++], &v[j]);
    swap(&v[i], &v[n - 1]);
    if (i < n - 1 - i) {
      qs(v, i, cmp);
      v += i + 1;
      n -= i + 1;
    } else {
      qs(v + i + 1, n - i - 1, cmp);
      n = i;
    }
  }
}

int main(void) {
  unsigned s = 1;
  for (int i = 0; i < N; i++) {
    s = s * 1103515245u + 12345u;
    a[i] = (int)(s >> 8) % 100000;
  }
  qs(a, N, cmp_int);
  int ok = 1;
  for (int i = 1; i < N; i++)
    if (a[i - 1] > a[i])
      ok = 0;
  ks_put_kv("sorted", ok);
  ks_put_kv("min", a[0]);
  ks_put_kv("max", a[N - 1]);
  return 0;
}
