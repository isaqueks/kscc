// Arrays de structs: indexação com elementos de tamanho não potência de
// 2, arrays globais inicializados, cópia de elementos e laços.
#include "minirt.h"
struct P { short x; char name[3]; }; // 6 bytes, alinhamento 2
struct Q { int id; struct P pts[3]; };

struct P gp[4] = {{1, "ab"}, {2, "cd"}, {3, "ef"}, {4, "gh"}};
struct Q gq[2] = {{10, {{1, "a"}, {2, "b"}, {3, "c"}}}, {20}};

int sum_x(struct P *a, int n) {
  int s = 0;
  for (int i = 0; i < n; i++)
    s += a[i].x;
  return s;
}

int main() {
  EQ(6, sizeof(struct P));
  EQ(24, sizeof(gp));
  EQ(24, sizeof(struct Q));
  EQ(10, sum_x(gp, 4));
  EQ('f', gp[2].name[1]);
  EQ(6, sum_x(gq[0].pts, 3));
  EQ(0, sum_x(gq[1].pts, 3));
  EQ(20, gq[1].id);

  struct P loc[5];
  for (int i = 0; i < 5; i++) {
    loc[i].x = i * i;
    loc[i].name[0] = 'A' + i;
  }
  EQ(30, sum_x(loc, 5));
  EQ('D', loc[3].name[0]);

  loc[0] = gp[3];
  EQ(4, loc[0].x);
  EQ('h', loc[0].name[1]);

  struct P *p = &loc[4];
  EQ(4, p - loc);
  EQ(24, (char *)p - (char *)loc);
  EQ(9, (p - 1)->x);
  return 0;
}
