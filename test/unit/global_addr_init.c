// Globais inicializadas com endereços de outras globais (.word sym + off):
// ponteiros para elementos, para membros, para funções, cadeias de
// ponteiros, arrays de ponteiros e structs com ponteiros.
#include "minirt.h"
struct N { int v; struct N *next; };

int arr[5] = {10, 20, 30, 40, 50};
int *p_mid = &arr[2];
int *p_end = arr + 5;
int **pp = &p_mid;
char str[] = "hello";
char *p_str = str + 1;
char *lit = "literal" + 3;

struct N n3 = {3, 0};
struct N n2 = {2, &n3};
struct N n1 = {1, &n2};
int *p_member = &n2.v;

int add1(int x) { return x + 1; }
int (*fptr)(int) = add1;
int *table[3] = {&arr[0], &arr[4], 0};
struct { int *a; char *s; } mix = {&arr[1], "xyz"};

int main() {
  EQ(30, *p_mid);
  EQ(5, p_end - arr);
  EQ(30, **pp);
  EQ('e', *p_str);
  EQ('e', *lit);
  EQ(2, *p_member);
  EQ(6, n1.v + n1.next->v + n1.next->next->v);
  EQ(0, (int)n1.next->next->next);
  EQ(8, fptr(7));
  EQ(60, *table[0] + *table[1]);
  EQ(0, (int)table[2]);
  EQ(20, *mix.a);
  EQ('z', mix.s[2]);
  p_mid++;
  EQ(40, **pp);
  return 0;
}
