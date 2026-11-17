// Quicksort (Lomuto e Hoare), mergesort top-down com buffer estático e
// heapsort; compara os resultados entre si.
#include "kstest.h"

#define N 500

static int tmp[N];

static void quick_lomuto(int *a, int lo, int hi) {
  if (lo >= hi)
    return;
  int p = a[hi], i = lo;
  for (int j = lo; j < hi; j++)
    if (a[j] < p) {
      int t = a[i]; a[i] = a[j]; a[j] = t;
      i++;
    }
  int t = a[i]; a[i] = a[hi]; a[hi] = t;
  quick_lomuto(a, lo, i - 1);
  quick_lomuto(a, i + 1, hi);
}

static void quick_hoare(int *a, int lo, int hi) {
  while (lo < hi) {
    int p = a[lo + (hi - lo) / 2];
    int i = lo - 1, j = hi + 1;
    for (;;) {
      do i++; while (a[i] < p);
      do j--; while (a[j] > p);
      if (i >= j)
        break;
      int t = a[i]; a[i] = a[j]; a[j] = t;
    }
    // recursão no menor lado, laço no maior
    if (j - lo < hi - j) {
      quick_hoare(a, lo, j);
      lo = j + 1;
    } else {
      quick_hoare(a, j + 1, hi);
      hi = j;
    }
  }
}

static void merge_sort(int *a, int lo, int hi) {
  if (hi - lo < 2)
    return;
  int mid = lo + (hi - lo) / 2;
  merge_sort(a, lo, mid);
  merge_sort(a, mid, hi);
  int i = lo, j = mid, k = lo;
  while (i < mid && j < hi)
    tmp[k++] = a[i] <= a[j] ? a[i++] : a[j++];
  while (i < mid) tmp[k++] = a[i++];
  while (j < hi) tmp[k++] = a[j++];
  for (k = lo; k < hi; k++)
    a[k] = tmp[k];
}

static void sift_down(int *a, int start, int end) {
  int root = start;
  while (2 * root + 1 <= end) {
    int child = 2 * root + 1, sw = root;
    if (a[sw] < a[child]) sw = child;
    if (child + 1 <= end && a[sw] < a[child + 1]) sw = child + 1;
    if (sw == root) return;
    int t = a[root]; a[root] = a[sw]; a[sw] = t;
    root = sw;
  }
}

static void heap_sort(int *a, int n) {
  for (int s = (n - 2) / 2; s >= 0; s--)
    sift_down(a, s, n - 1);
  for (int end = n - 1; end > 0; end--) {
    int t = a[end]; a[end] = a[0]; a[0] = t;
    sift_down(a, 0, end - 1);
  }
}

static void gen(int *a, int n, unsigned s) {
  for (int i = 0; i < n; i++) {
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    a[i] = (int)(s % 100000u) - 50000;
  }
}

static unsigned sum_weighted(const int *a, int n) {
  unsigned h = 0;
  for (int i = 0; i < n; i++)
    h += (unsigned)a[i] * (unsigned)(i + 1);
  return h;
}

int main(void) {
  static int a[N], b[N], c[N], d[N];
  for (int round = 0; round < 3; round++) {
    gen(a, N, 2463534242u + (unsigned)round * 7u);
    for (int i = 0; i < N; i++)
      b[i] = c[i] = d[i] = a[i];
    quick_lomuto(a, 0, N - 1);
    quick_hoare(b, 0, N - 1);
    merge_sort(c, 0, N);
    heap_sort(d, N);
    int same = 1;
    for (int i = 0; i < N; i++)
      if (a[i] != b[i] || b[i] != c[i] || c[i] != d[i])
        same = 0;
    ks_put_kv("round", round);
    ks_put_kv("  same", same);
    ks_put_kv("  first", a[0]);
    ks_put_kv("  last", a[N - 1]);
    ks_put_kx("  wsum", sum_weighted(a, N));
  }
  return 0;
}
