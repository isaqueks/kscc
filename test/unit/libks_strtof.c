// strtof/strtod/strtold/atof: decimal, hexadecimal, inf/nan, ponteiro de
// fim, errno (ERANGE) e arredondamento correto (empates, subnormais,
// FLT_MAX, mais de 120 dígitos). Ver também test/diff/libks_strtof.c.
#include <errno.h>
#include <stdlib.h>
#include "float_test.h"

static float conv(const char *s, int want_end) {
  char *end;
  float f = strtof(s, &end);
  EQ(want_end, (int)(end - s));
  return f;
}

int main() {
  EQB(0x00000000, conv("0", 1));
  EQB(F_NZERO, conv("-0", 2));
  EQB(0x3F800000, conv("1", 1));
  EQB(0x3DCCCCCD, conv("0.1", 3));
  EQB(0x3DCCCCCD, conv(".1", 2));
  EQB(0x40490FDB, conv("3.14159265358979", 16));
  EQB(0x461C4000, conv("1e4", 3));
  EQB(0x461C4000, conv("1E+4", 4));
  EQB(0x3C23D70A, conv("1e-2", 4));
  EQB(0x3F800000, conv("  \t\n+1.", 7));
  EQB(0xC2F6E979, conv("-123.456", 8));
  EQB(0x501502F9, conv("10000000000", 11));
  EQB(0x3F800000, conv("1e", 1));       // expoente sem dígitos: não é consumido
  EQB(0x3F800000, conv("1e+", 1));
  EQB(0x40A00000, conv("5x", 1));
  EQB(0x447A0000, conv("1000.000000", 11));
  EQB(0x3F800000, conv("00000001.0000", 13));

  // empates e arredondamento correto
  EQB(0x4B800000, conv("16777217", 8));  // empate: para o par
  EQB(0x4B800002, conv("16777219", 8));
  EQB(0x4B800001, conv("16777217.000000001", 18));  // acima do empate
  EQB(0x3F800000, conv("1.000000059604644775390625", 26));  // 1 + 2^-24: empate
  EQB(0x3F800001, conv("1.0000000596046447753906250000000000001", 39));
  // 130 dígitos: o empate exato está nos 25 primeiros, e só o último (além
  // dos 120 guardados) decide para cima
  EQB(0x3F800001, conv("1.00000005960464477539062500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001", 130));
  EQB(0x3F800000, conv("1.00000005960464477539062500000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", 130));

  // limites
  EQB(0x7F7FFFFF, conv("3.40282347e38", 13));
  EQB(0x7F7FFFFF, conv("3.4028235677973366e38", 21));  // logo abaixo de FLT_MAX + meio ulp
  errno = 0;
  EQB(F_INF, conv("3.4028235677973367e38", 21));       // a partir dele: inf
  EQ(ERANGE, errno);
  errno = 0;
  EQB(0xFF800000, conv("-1e39", 5));
  EQ(ERANGE, errno);
  errno = 0;
  EQB(0x00800000, conv("1.17549435e-38", 14));
  EQB(0x00000001, conv("1.4e-45", 7));
  EQB(0x00000001, conv("7.1e-46", 7));                 // acima de meio subnormal
  EQ(0, errno);                                         // subnormal sem ERANGE
  EQB(0x00000000, conv("7.0e-46", 7));                 // abaixo: zero
  EQ(ERANGE, errno);
  errno = 0;
  EQB(0x00000000, conv("1e-100000", 9));
  EQ(ERANGE, errno);
  errno = 0;
  EQB(0x00000000, conv("0e999999", 8));                // zero exato: sem ERANGE
  EQ(0, errno);

  // hexadecimal
  EQB(0x3FC00000, conv("0x1.8p0", 7));
  EQB(0x3FC00000, conv("0X1.8P0", 7));
  EQB(0x41200000, conv("0xa", 3));
  EQB(0x00000001, conv("0x1p-149", 8));
  EQB(0x7F7FFFFF, conv("0x1.fffffep127", 14));
  EQB(F_INF, conv("0x1.ffffffp127", 14));               // arredonda para cima: inf
  EQB(0x3F800000, conv("0x1.000001p0", 12));            // empate: para o par
  EQB(0x3F800001, conv("0x1.0000011p0", 13));
  EQB(0x3F000000, conv("0x.8", 4));
  EQB(0x00000000, conv("0x", 1));                       // só o "0"
  EQB(0x00000000, conv("0xg", 1));
  EQB(0x41800000, conv("0x10p", 4));                    // 'p' sem dígitos

  // inf e nan
  EQB(F_INF, conv("inf", 3));
  EQB(F_INF, conv("INFINITY", 8));
  EQB(F_NINF, conv("-Inf", 4));
  EQB(F_INF, conv("infinit", 3));
  EQB(F_NAN, conv("nan", 3));
  EQB(0xFFC00000, conv("-NaN", 4));
  EQB(F_NAN, conv("nan(abc_12)", 11));
  EQB(F_NAN, conv("nan(", 3));                          // sem ')': só "nan"

  // sem conversão: 0 e end = s
  EQB(0, conv("", 0));
  EQB(0, conv("abc", 0));
  EQB(0, conv("-", 0));
  EQB(0, conv(".", 0));
  EQB(0, conv(" .e1", 0));
  EQB(0, conv("+-1", 0));

  // strtod, strtold, atof: a mesma conversão
  char *end;
  EQB(0x3DCCCCCD, strtod("0.1", &end));
  EQ(0, *end);
  EQB(0x3DCCCCCD, strtold("0.1z", &end));
  EQ('z', *end);
  EQB(0xC0490FDA, atof("-3.1415926"));  // mais perto de 3.14159250 que de 3.14159274
  EQB(0, atof("x"));
  EQB(0x40000000, strtof("2", 0));
  return 0;
}
