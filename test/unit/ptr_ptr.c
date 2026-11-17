// Ponteiros para ponteiros: troca via **, arrays de ponteiros, ***,
// ponteiro para ponteiro de função e ponteiro para array.
#include "minirt.h"
void swap(int **a, int **b) {
  int *t = *a;
  *a = *b;
  *b = t;
}

void set3(int ***p, int v) { ***p = v; }

int twice(int x) { return 2 * x; }

int main() {
  int x = 1, y = 2;
  int *px = &x, *py = &y;
  swap(&px, &py);
  EQ(2, *px);
  EQ(1, *py);

  int **pp = &px;
  int ***ppp = &pp;
  set3(ppp, 42);
  EQ(42, y);
  EQ(42, **pp);

  char *words[] = {"um", "dois", "tres"};
  char **w = words;
  EQ('d', **(w + 1));
  EQ('s', *(*(w + 2) + 3));
  EQ('o', w[1][1]);

  int (*f)(int) = twice;
  int (**pf)(int) = &f;
  EQ(14, (*pf)(7));

  int arr[3][4];
  int (*row)[4] = arr;
  row[2][3] = 9;
  EQ(9, arr[2][3]);
  EQ(16, (char *)(row + 1) - (char *)row);
  return 0;
}
