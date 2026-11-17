// Struct packed com um membro comum logo depois de um bitfield que não
// termina num limite de byte: o membro ficava no byte que ainda tem bits
// do campo, e escrever um corrompia o outro.
// (O layout de bitfields em structs packed não é o do gcc: o kscc não
// deixa um campo atravessar a unidade do seu tipo, então PB tem 9 bytes,
// não 6. Os valores, porém, têm de ser independentes.)
#include "minirt.h"
struct __attribute__((packed)) PB { char a; int b : 30; char c; };
struct __attribute__((packed)) PC { unsigned x : 3; char y; short z : 5; char w; };
struct PB g = { 1, -536870912, 3 };
int main() {
  struct PB l = { 1, -536870912, 3 };
  EQ(-536870912, l.b);
  EQ(3, l.c);
  l.c = -1;
  EQ(-536870912, l.b);
  l.b = 536870911;
  EQ(-1, l.c);
  EQ(-536870912, g.b);
  EQ(3, g.c);
  struct PC pc = { 7, -2, -16, 9 };
  EQ(7, pc.x);
  EQ(-2, pc.y);
  EQ(-16, pc.z);
  EQ(9, pc.w);
  pc.y = 0x55;
  pc.w = 0x66;
  EQ(7, pc.x);
  EQ(-16, pc.z);
  return 0;
}
