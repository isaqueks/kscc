#include "kstest.h"
// Inicializadores de globais: (1) um bitfield sem inicializador fazia o
// chibicc parar a struct (`break`), zerando os campos seguintes; (2) a
// conversão para _Bool (!= 0) não era aplicada, nem em campo de 1 bit;
// (3) bitfield de union não era mascarado (teste em
// test/unit/global_init_conv.c, porque
// o gcc avisa do truncamento).
struct B { int a : 3; unsigned b : 7; int c : 5; char d; int e : 20; unsigned f : 12; };
struct B gb2 = { .c = 15, .a = 3, .f = 1 };
struct B gba[2] = { [1] = { 1, 2, 3, 4, 5, 6 }, [0].e = -2 };
struct B gb3 = { .e = -524288 };
_Bool gbool[] = { 0, 5, -1, 256 };
struct { _Bool x : 1; unsigned y : 3; _Bool z; } gbf = { 2, 5, 512 };
int main(void) {
  ks_put_kv("gb2", gb2.a * 10000 + gb2.b * 1000 + gb2.c * 10 + gb2.f);
  ks_put_kv("gb2.de", gb2.d + gb2.e);
  ks_put_kv("gba1", gba[1].a + gba[1].b + gba[1].c + gba[1].d + gba[1].e + gba[1].f);
  ks_put_kv("gba0.e", gba[0].e);
  ks_put_kv("gb3.e", gb3.e);
  ks_put_kv("gbool", gbool[0] * 1000 + gbool[1] * 100 + gbool[2] * 10 + gbool[3]);
  ks_put_kv("gbf", gbf.x * 100 + gbf.y * 10 + gbf.z);
  return 0;
}
