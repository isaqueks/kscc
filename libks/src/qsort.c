// qsort e bsearch da libks (doc 05 §2).
//
// qsort é um Shell sort (lacunas de Knuth 1, 4, 13, 40, ...): sem
// recursão e sem memória extra, o que importa num MCU com 1-2 KB de RAM,
// onde a pilha do quicksort no pior caso não cabe. O(n^1.5) no pior caso
// é suficiente para as listas de um programa embarcado. Não é estável,
// como o qsort do C não precisa ser.
#include <stdlib.h>

static void swap(char *a, char *b, size_t size) {
  while (size--) {
    char t = *a;
    *a++ = *b;
    *b++ = t;
  }
}

void qsort(void *base, size_t n, size_t size,
           int (*cmp)(const void *, const void *)) {
  char *b = base;
  size_t gap = 1;

  while (gap < n / 3)
    gap = gap * 3 + 1;

  // Ordenação por inserção com passo `gap`, do maior para 1.
  for (; gap > 0; gap /= 3)
    for (size_t i = gap; i < n; i++)
      for (size_t j = i; j >= gap; j -= gap) {
        char *x = b + (j - gap) * size, *y = b + j * size;
        if (cmp(x, y) <= 0)
          break;
        swap(x, y, size);
      }
}

void *bsearch(const void *key, const void *base, size_t n, size_t size,
              int (*cmp)(const void *, const void *)) {
  const char *lo = base;

  while (n > 0) {
    const char *mid = lo + (n / 2) * size;
    int c = cmp(key, mid);
    if (c == 0)
      return (void *)mid;
    if (c > 0) {
      lo = mid + size;
      n -= n / 2 + 1;
    } else {
      n /= 2;
    }
  }
  return NULL;
}
