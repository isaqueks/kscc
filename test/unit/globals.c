// Globais: .data, .bss, static, ponteiros inicializados (.word sym + off).
#include "minirt.h"
int gi = 42;
int gz;
static int gs = -7;
char gc = -1;
short gsh = 1000;
int garr[4] = {1, 2, 3, 4};
int *gp = &garr[2];
int *gp2 = garr + 1;
int big[1000];
unsigned gu = 0xFFFFFFFF;
int main() {
  EQ(42, gi);
  EQ(0, gz);
  EQ(-7, gs);
  EQ(-1, gc);
  EQ(1000, gsh);
  EQ(3, *gp);
  EQ(2, *gp2);
  EQ(10, garr[0] + garr[1] + garr[2] + garr[3]);
  gz = 5; gs = gz * 2; gi += gs;
  EQ(52, gi);
  big[999] = 9; big[0] = 1;
  EQ(10, big[0] + big[999] + big[500]);
  EQ(1, gu == -1u);
  EQ(4000, sizeof(big));
  return 0;
}
