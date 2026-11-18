// strtof da libks x glibc: os bits do resultado e o ponteiro de fim para
// literais variados (decimal, hexadecimal, inf/nan, limites, empates,
// muitos dígitos) e para 300 valores pseudoaleatórios impressos com %.9g e
// %.6e e lidos de volta (ida e volta). O payload de nan(...) é ignorado
// pela libks, então só nan sem payload entra aqui.
#include <stdlib.h>
#include "fltout.h"

static unsigned seed = 777;
static unsigned rnd(void) {
  seed = seed * 1664525u + 1013904223u;
  unsigned a = seed >> 16;
  seed = seed * 1664525u + 1013904223u;
  return (a << 16) | (seed >> 16);
}

static void st(const char *s) {
  char *end;
  float f = strtof(s, &end);
  printf("%-40.40s ", s);
  fl_hex(f);
  printf("+%d\n", (int)(end - s));
}

int main(void) {
  static const char *ss[] = {
    "0", "1", "-1", "0.1", "3.14159", "1e10", "1e38", "3.4028235e38", "3.4028236e38", "1e39",
    "1e-45", "7e-46", "7.1e-46", "1.401298464324817e-45", "0x1p-149", "0x1.fffffep127",
    "0x1.ffffffp127", "inf", "-Infinity", "nan", "  +.5e+1x", "1e", "0x", ".e1", "-", "",
    "123.456e-2", "0.000000000000000000000000000000000000011754943508222875", "16777217",
    "16777219", "0x1.000001p0", "1.000000059604644775390625", "4.9e-324", "2.5e-46",
    "1.00000005960464477539062500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001",
    "000000000000000000000000000000000000000000000000000123.5",
    "0.0000000000000000000000000000000000000000000000000000000000000000000000001e70",
    "3.40282356779733661637539395458142568447e38", "3.40282356779733661637539395458142568448e38",
    "1.1754942807573642917e-38", "0x.0000000000000000000000000000000001p+130", "1e+4294967306",
    "1e-4294967306", "9999999999999999999999999999999999999999e-40", "0X1P", "0x1p+", "INFINITE",
    "nAn", "-0", "-0x0p0", "1.5e3f", ".5", "5.", "1e0001", "\t\n 42",
  };
  for (int i = 0; i < (int)(sizeof(ss) / sizeof(ss[0])); i++)
    st(ss[i]);
  char buf[64];
  for (int i = 0; i < 300; i++) {
    float f = fl_from(rnd());
    if (f != f)
      continue;
    snprintf(buf, sizeof buf, "%.9g", f);
    st(buf);
    snprintf(buf, sizeof buf, "%.6e", f);
    st(buf);
    snprintf(buf, sizeof buf, "%.3f", f);
    st(buf);
  }
  return 0;
}
