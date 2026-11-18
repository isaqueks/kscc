// float: qsort com comparador de floats (sem NaN), busca binária,
// mediana, histograma por faixa (float -> índice) e média/variância.
#include <stdlib.h>
#include "fltout.h"

static unsigned seed = 99;
static unsigned rnd(void) {
  seed = seed * 1103515245u + 12345u;
  return (seed >> 8) & 0xFFFFFF;
}

static int cmp(const void *a, const void *b) {
  float x = *(const float *)a, y = *(const float *)b;
  return (x > y) - (x < y);
}

int main(void) {
  float a[300];
  for (int i = 0; i < 300; i++)
    a[i] = (float)rnd() / 16777216.0f * 200.0f - 100.0f;
  qsort(a, 300, sizeof(float), cmp);
  for (int i = 0; i < 300; i += 23) {
    printf("a[%3d] ", i);
    fl_hex(a[i]);
    printf("%.4f\n", a[i]);
  }
  float med = (a[149] + a[150]) / 2;
  fl_put("mediana", med);

  float key = a[200];
  int lo = 0, hi = 299, found = -1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    if (a[mid] < key)
      lo = mid + 1;
    else if (a[mid] > key)
      hi = mid - 1;
    else {
      found = mid;
      break;
    }
  }
  printf("busca %d\n", found);

  int hist[10] = {0};
  for (int i = 0; i < 300; i++) {
    int b = (int)((a[i] + 100.0f) / 20.0f);
    if (b > 9)
      b = 9;
    hist[b]++;
  }
  for (int i = 0; i < 10; i++)
    printf("%d ", hist[i]);
  printf("\n");

  float mean = 0, m2 = 0;
  for (int i = 0; i < 300; i++) {
    float d = a[i] - mean;
    mean += d / (i + 1);
    m2 += d * (a[i] - mean);
  }
  fl_put("media", mean);
  fl_put("variancia", m2 / 299);
  printf("%.5g %.5g\n", mean, m2 / 299);
  return 0;
}
