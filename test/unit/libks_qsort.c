// qsort com comparadores diferentes (crescente, decrescente, structs por
// campo, strings, elementos de 3 e de 100 bytes), tamanhos 0/1/2, muitos
// repetidos e um comparador inconsistente (não pode escrever fora do
// vetor nem perder elementos); bsearch achando e não achando.
#include <stdlib.h>
#include <string.h>
#include "minirt.h"

static int asc(const void *a, const void *b) {
  int x = *(const int *)a, y = *(const int *)b;
  return (x > y) - (x < y);  // sem estourar com INT_MIN
}

static int desc(const void *a, const void *b) { return asc(b, a); }

typedef struct {
  char name[8];
  int age;
} Person;

static int by_age(const void *a, const void *b) {
  return ((const Person *)a)->age - ((const Person *)b)->age;
}

static int by_str(const void *a, const void *b) {
  return strcmp(*(char *const *)a, *(char *const *)b);
}

typedef struct {
  unsigned char k[3];
} Tri;

static int by_tri(const void *a, const void *b) {
  return memcmp(a, b, 3);
}

typedef struct {
  int k;
  char pad[96];
} Big;

static int by_big(const void *a, const void *b) {
  return ((const Big *)a)->k - ((const Big *)b)->k;
}

static unsigned chaos = 7;
static int chaotic(const void *a, const void *b) {
  (void)a;
  (void)b;
  chaos = chaos * 1103515245u + 12345u;
  return (int)((chaos >> 16) % 3) - 1;
}

static int calls;
static int counting(const void *a, const void *b) {
  calls++;
  return asc(a, b);
}

static void check_sorted(int *v, int n) {
  for (int i = 1; i < n; i++)
    EQ(1, v[i - 1] <= v[i]);
}

int main() {
  int v[100];

  // Pseudoaleatório com repetidos, e as bordas do int
  for (int i = 0; i < 100; i++)
    v[i] = (i * 7919) % 31 - 15;
  v[3] = -2147483647 - 1;
  v[50] = 2147483647;
  qsort(v, 100, sizeof(int), asc);
  check_sorted(v, 100);
  EQ(-2147483647 - 1, v[0]);
  EQ(2147483647, v[99]);

  qsort(v, 100, sizeof(int), desc);
  for (int i = 1; i < 100; i++)
    EQ(1, v[i - 1] >= v[i]);

  // Já ordenado e todos iguais
  for (int i = 0; i < 100; i++)
    v[i] = i;
  qsort(v, 100, sizeof(int), asc);
  for (int i = 0; i < 100; i++)
    EQ(i, v[i]);
  for (int i = 0; i < 100; i++)
    v[i] = 4;
  qsort(v, 100, sizeof(int), asc);
  check_sorted(v, 100);

  // n = 0 não chama o comparador; 1 e 2
  calls = 0;
  qsort(v, 0, sizeof(int), counting);
  qsort(0, 0, sizeof(int), counting);
  EQ(0, calls);
  v[0] = 9;
  qsort(v, 1, sizeof(int), counting);
  EQ(9, v[0]);
  v[0] = 2;
  v[1] = 1;
  qsort(v, 2, sizeof(int), asc);
  EQ(1, v[0]);
  EQ(2, v[1]);

  Person p[5] = {{"ana", 30}, {"bia", 25}, {"caio", 41}, {"duda", 19}, {"eva", 33}};
  qsort(p, 5, sizeof(Person), by_age);
  EQ(19, p[0].age);
  EQ('d', p[0].name[0]);
  EQ(41, p[4].age);
  EQ('c', p[4].name[0]);

  char *words[] = {"pera", "uva", "abacate", "kiwi", "banana", "abacaxi"};
  qsort(words, 6, sizeof(char *), by_str);
  EQ(0, strcmp(words[0], "abacate"));
  EQ(0, strcmp(words[1], "abacaxi"));
  EQ(0, strcmp(words[5], "uva"));

  Tri t[7];
  for (int i = 0; i < 7; i++) {
    t[i].k[0] = (7 - i) % 3;
    t[i].k[1] = i;
    t[i].k[2] = 0xAA;
  }
  qsort(t, 7, sizeof(Tri), by_tri);
  for (int i = 1; i < 7; i++)
    EQ(1, memcmp(&t[i - 1], &t[i], 3) < 0);
  for (int i = 0; i < 7; i++)
    EQ(0xAA, t[i].k[2]);

  // Elementos grandes (100 bytes): a troca leva o elemento inteiro
  static Big big[20];
  for (int i = 0; i < 20; i++) {
    big[i].k = (i * 7) % 20;
    memset(big[i].pad, big[i].k, sizeof(big[i].pad));
  }
  qsort(big, 20, sizeof(Big), by_big);
  for (int i = 0; i < 20; i++) {
    EQ(i, big[i].k);
    for (int j = 0; j < (int)sizeof(big[i].pad); j++)
      EQ(i, big[i].pad[j]);
  }

  // Comparador inconsistente (resultado aleatório): a ordem é qualquer,
  // mas nada fora do vetor é tocado e os elementos continuam os mesmos
  int g[1 + 60 + 1];
  g[0] = 0x11111111;
  g[61] = 0x22222222;
  for (int i = 1; i <= 60; i++)
    g[i] = i;
  qsort(g + 1, 60, sizeof(int), chaotic);
  EQ(0x11111111, g[0]);
  EQ(0x22222222, g[61]);
  int seen[61] = {0};
  for (int i = 1; i <= 60; i++) {
    EQ(1, g[i] >= 1 && g[i] <= 60);
    EQ(0, seen[g[i]]++);
  }

  // bsearch sobre o vetor ordenado
  for (int i = 0; i < 100; i++)
    v[i] = i * 2;
  for (int key = -1; key <= 200; key++) {
    int *r = bsearch(&key, v, 100, sizeof(int), asc);
    if (key >= 0 && key < 200 && key % 2 == 0)
      EQ(key / 2, r - v);
    else
      EQ(0, (int)r);
  }
  int key = 5;
  EQ(0, (int)bsearch(&key, v, 0, sizeof(int), asc));
  return 0;
}
