// Ordenações simples (bubble, insertion, selection, shell) sobre arrays
// pseudoaleatórios gerados por um LCG unsigned.
#include "kstest.h"

#define N 200

static unsigned seed = 12345u;
static int next_rand(void) {
  seed = seed * 1103515245u + 12345u;
  return (int)((seed >> 8) % 2001u) - 1000;
}

static void fill(int *a, int n) {
  for (int i = 0; i < n; i++)
    a[i] = next_rand();
}

static void bubble(int *a, int n) {
  for (int i = 0; i < n - 1; i++) {
    int swapped = 0;
    for (int j = 0; j < n - 1 - i; j++)
      if (a[j] > a[j + 1]) {
        int t = a[j];
        a[j] = a[j + 1];
        a[j + 1] = t;
        swapped = 1;
      }
    if (!swapped)
      break;
  }
}

static void insertion(int *a, int n) {
  for (int i = 1; i < n; i++) {
    int v = a[i], j = i - 1;
    while (j >= 0 && a[j] > v) {
      a[j + 1] = a[j];
      j--;
    }
    a[j + 1] = v;
  }
}

static void selection(int *a, int n) {
  for (int i = 0; i < n - 1; i++) {
    int m = i;
    for (int j = i + 1; j < n; j++)
      if (a[j] < a[m])
        m = j;
    int t = a[i];
    a[i] = a[m];
    a[m] = t;
  }
}

static void shell(int *a, int n) {
  for (int gap = n / 2; gap > 0; gap /= 2)
    for (int i = gap; i < n; i++) {
      int v = a[i], j;
      for (j = i; j >= gap && a[j - gap] > v; j -= gap)
        a[j] = a[j - gap];
      a[j] = v;
    }
}

static int is_sorted(const int *a, int n) {
  for (int i = 1; i < n; i++)
    if (a[i - 1] > a[i])
      return 0;
  return 1;
}

static unsigned checksum(const int *a, int n) {
  unsigned h = 2166136261u;
  for (int i = 0; i < n; i++)
    h = (h ^ (unsigned)a[i]) * 16777619u;
  return h;
}

static void report(const char *name, int *a, int n) {
  ks_put_str(name);
  ks_put_str(": sorted=");
  ks_put_int(is_sorted(a, n));
  ks_put_str(" min=");
  ks_put_int(a[0]);
  ks_put_str(" max=");
  ks_put_int(a[n - 1]);
  ks_put_str(" med=");
  ks_put_int(a[n / 2]);
  ks_put_str(" h=");
  ks_put_hex(checksum(a, n));
  ks_put_chr('\n');
}

int main(void) {
  int a[N];
  void (*sorts[])(int *, int) = {bubble, insertion, selection, shell};
  const char *names[] = {"bubble", "insertion", "selection", "shell"};
  for (int k = 0; k < 4; k++) {
    seed = 777u + (unsigned)k;
    fill(a, N);
    ks_put_kx("before", checksum(a, N));
    sorts[k](a, N);
    report(names[k], a, N);
  }
  // Casos de borda: repetidos, ao contrário, n = 1 e n = 0.
  int b[8] = {5, 5, 5, 5, 5, 5, 5, 5};
  insertion(b, 8);
  report("repeated", b, 8);
  for (int i = 0; i < 8; i++)
    b[i] = 8 - i;
  bubble(b, 8);
  report("reversed", b, 8);
  bubble(b, 1);
  shell(b, 0);
  report("noop", b, 8);
  return 0;
}
