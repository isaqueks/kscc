// Inicializadores designados em locais e globais: campos fora de ordem,
// índices de array, aninhados, sobrescrita e zeragem do resto.
#include "minirt.h"
struct P { int x, y, z; };
struct L { struct P a, b; int tag[4]; };

int garr[8] = {[2] = 5, [6] = 7, 8};
struct P gp = {.z = 3, .x = 1};
struct L gl = {.b.y = 9, .tag[3] = 4, .a = {1, 2, 3}};
char gstr[6] = {[1] = 'a', 'b', [0] = 'z'};

int main() {
  EQ(0, garr[0]);
  EQ(5, garr[2]);
  EQ(7, garr[6]);
  EQ(8, garr[7]);
  EQ(1, gp.x);
  EQ(0, gp.y);
  EQ(3, gp.z);
  EQ(9, gl.b.y);
  EQ(0, gl.b.x);
  EQ(4, gl.tag[3]);
  EQ(3, gl.a.z);
  EQ('z', gstr[0]);
  EQ('b', gstr[2]);
  EQ(0, gstr[5]);

  int a[5] = {[4] = 1, [1] = 2, 3};
  EQ(0, a[0]);
  EQ(2, a[1]);
  EQ(3, a[2]);
  EQ(1, a[4]);

  struct L l = {.tag = {[2] = 6}, .b = {.z = 5}, .a.x = 7, .a.x = 8};
  EQ(8, l.a.x);
  EQ(0, l.a.y);
  EQ(5, l.b.z);
  EQ(6, l.tag[2]);
  EQ(0, l.tag[0]);

  struct P ps[3] = {[1].y = 4, [2] = {.x = 1}};
  EQ(4, ps[1].y);
  EQ(1, ps[2].x);
  EQ(0, ps[0].z);
  return 0;
}
