// printf da libks x glibc para floats: os mesmos caracteres em %f %e %g %a
// (várias precisões, flags e larguras), para valores especiais e para 400
// padrões de bits pseudoaleatórios (inclusive subnormais, inf e NaN, que
// aqui vêm de bits, não de contas, e por isso têm o mesmo sinal nos dois).
// A conversão da libks é exata e arredonda como o glibc (doc 05 §2).
#include <float.h>
#include "fltout.h"

static unsigned seed = 12345;
static unsigned rnd(void) {
  seed = seed * 1103515245u + 12345u;
  unsigned a = seed >> 16;
  seed = seed * 1103515245u + 12345u;
  return (a << 16) | (seed >> 16);
}

static void show(float f) {
  printf("[%f] [%.0f] [%.3f] [%.12f] [%e] [%.0e] [%.3E] [%g] [%.1g] [%.12g] [%G] [%#g]",
         f, f, f, f, f, f, f, f, f, f, f, f);
  printf(" [%a] [%.0a] [%.2A] [%12.3f] [%-12.2e] [%+.3g] [% 012.4f] [%#.0f] [%#.0e] [%08.1f]\n",
         f, f, f, f, f, f, f, f, f, f);
}

int main(void) {
  float sp[] = {0.0f, -0.0f, 1.0f, -1.5f, 0.5f, 2.5f, 0.125f, 1e-45f, FLT_MIN, FLT_MAX, 1e38f,
                123456789.0f, 0.1f, 9.999999f, 99999.95f, 1e-5f, 0.0001f, 1e6f, 1e7f,
                16777216.0f, 0.3f, 0.05f, 0.95f, 1.5e-39f, 2.0e-38f, 999999.25f, 0.00009999f};
  for (int i = 0; i < (int)(sizeof(sp) / sizeof(sp[0])); i++)
    show(sp[i]);
  show(fl_from(0x7F800000));
  show(fl_from(0xFF800000));
  show(fl_from(0x7FC00000));
  show(fl_from(0xFFC00000));
  for (int i = 0; i < 400; i++)
    show(fl_from(rnd()));
  // %.Nf de valores com expansão longa
  printf("%.40f\n%.60e\n%.30f\n", 1e-30f, FLT_MIN, 3.14159265f);
  return 0;
}
