// printf %f %F %e %E %g %G %a %A: conversão exata do binary32, arredondada
// para o mais próximo com empate para o par sobre o valor exato (igual ao
// glibc), flags, largura, precisão, L e casos de borda (zeros com sinal,
// inf, nan, subnormais, FLT_MAX, empates). Os mesmos valores são conferidos
// contra o glibc em test/diff/libks_float_printf.c.
#include <float.h>
#include "libks_test.h"
#include "float_test.h"
// expect-out: pi=3.141593 e=2.718282e+00 g=0.1 nan=nan -inf=-inf
int main() {
  float n = bitsf(F_NAN), inf = bitsf(F_INF);
  FMT("0.000000", "%f", 0.0);
  FMT("-0.000000", "%f", -0.0);
  FMT("1.000000", "%f", 1.0f);
  FMT("-1.500000", "%f", -1.5);
  FMT("0.100000", "%f", 0.1f);
  FMT("0.10000000149011611938", "%.20f", 0.1f);  // o valor exato do float
  FMT("3.14159274101257324218750", "%.23f", 3.14159265f);
  FMT("16777216.000000", "%f", 16777217.0f);
  FMT("340282346638528859811704183484516925440.000000", "%f", FLT_MAX);
  FMT("0.000000", "%f", FLT_TRUE_MIN);
  FMT("1.401298e-45", "%e", FLT_TRUE_MIN);
  FMT("0.00000000000000000000000000000000000000000000140129846432481707092372958328991613128026194187651577175706828388979108268586060148663818836212158203125",
      "%.149f", FLT_TRUE_MIN);  // a expansão exata de 2^-149
  FMT("1.17549435082228750796873653722224568e-38", "%.35e", FLT_MIN);

  // empates (valores exatos): para o par
  FMT("0", "%.0f", 0.5); FMT("2", "%.0f", 1.5); FMT("2", "%.0f", 2.5); FMT("4", "%.0f", 3.5);
  FMT("0.2", "%.1f", 0.25); FMT("0.4", "%.1f", 0.375); FMT("1.12", "%.2f", 1.125);
  FMT("0.9", "%.1f", 0.95f);       // 0.95f = 0.949999988...: não é empate
  FMT("0.3", "%.1f", 0.35f);       // 0.35f = 0.3499999940...
  FMT("1", "%.0f", 0.5000001f);
  FMT("10.00", "%.2f", 9.999f);    // vai-um até a parte inteira
  FMT("100", "%.0f", 99.5);
  FMT("-0", "%.0f", -0.4);

  // %e %E
  FMT("1.000000e+00", "%e", 1.0); FMT("0.000000e+00", "%e", 0.0); FMT("-0.000000e+00", "%e", -0.0);
  FMT("1.234568e+04", "%e", 12345.678f); FMT("1.2346E+04", "%.4E", 12345.678f);
  FMT("3.402823e+38", "%e", FLT_MAX); FMT("1e+10", "%.0e", 1e10f); FMT("2e+00", "%.0e", 2.5);
  FMT("1.e+00", "%#.0e", 1.0); FMT("1.0e-07", "%.1e", 1e-7f);
  FMT("1.000e+01", "%.3e", 9.9996f);

  // %g %G
  FMT("0", "%g", 0.0); FMT("-0", "%g", -0.0); FMT("1", "%g", 1.0); FMT("0.1", "%g", 0.1f);
  FMT("100000", "%g", 1e5f); FMT("1e+06", "%g", 1e6f); FMT("1.23457e+06", "%g", 1234567.0f);
  FMT("0.0001", "%g", 0.0001f); FMT("1e-05", "%g", 1e-5f); FMT("1E-05", "%G", 1e-5f);
  FMT("0.000123457", "%g", 0.000123456789f);
  FMT("3.14159", "%g", 3.14159265f); FMT("3.141592741", "%.10g", 3.14159265f);
  FMT("3", "%.1g", 3.14159f); FMT("3", "%.0g", 3.14159f); FMT("1e+01", "%.1g", 9.5f);
  FMT("1.00000", "%#g", 1.0); FMT("100000.", "%#g", 1e5f); FMT("1.e+06", "%#.1g", 1e6f);
  FMT("1.00000e+06", "%#g", 999999.5f);  // o glibc 2.39 dá "1.e+06" (perde os zeros que o # manda manter)
  FMT("1e+02", "%.2g", 99.5f); FMT("99", "%.2g", 99.49f);
  FMT("16777216", "%.10g", 16777216.0f);

  // %a %A: mantissa normalizada, sem zeros finais
  FMT("0x1p+0", "%a", 1.0); FMT("0x0p+0", "%a", 0.0); FMT("-0x0p+0", "%a", -0.0);
  FMT("0x1.8p+0", "%a", 1.5); FMT("-0x1.4p+3", "%a", -10.0); FMT("0x1.99999ap-4", "%a", 0.1f);
  FMT("0x1p-149", "%a", FLT_TRUE_MIN); FMT("0x1.fffffep+127", "%a", FLT_MAX);
  FMT("0X1.8P+1", "%A", 3.0); FMT("0x1.000p+0", "%.3a", 1.0);
  FMT("0x2p+0", "%.0a", 1.5); FMT("0x1p+1", "%.0a", 2.5);   // empate: para o par
  FMT("0x1.ap-4", "%.1a", 0.1f); FMT("0x1.p+0", "%#a", 1.0); FMT("0x1.0000000p+0", "%.7a", 1.0);

  // inf e nan: sinal do bit 31, sem zeros de preenchimento
  FMT("inf", "%f", inf); FMT("-inf", "%f", -inf); FMT("INF", "%F", inf); FMT("nan", "%f", n);
  FMT("-nan", "%f", -n); FMT("NAN", "%E", n); FMT("inf", "%g", inf); FMT("-INF", "%A", -inf);
  FMT("+inf", "%+f", inf); FMT("   nan", "%06f", n); FMT(" inf", "% e", inf);
  FMT("nan   ", "%-6g", n);

  // flags e largura
  FMT("  3.14", "%6.2f", 3.14159f); FMT("3.14  ", "%-6.2f", 3.14159f);
  FMT("003.14", "%06.2f", 3.14159f); FMT("-03.14", "%06.2f", -3.14159f);
  FMT("+3.14", "%+.2f", 3.14159f); FMT(" 3.14", "% .2f", 3.14159f);
  FMT("+3.14", "%+ .2f", 3.14159f); FMT("3.", "%#.0f", 3.0); FMT("3", "%.0f", 3.0);
  FMT("0x001.8p+0", "%010a", 1.5); FMT("-0001e+00", "%09.0e", -1.0);
  FMT("    1.5e+00", "%*.*e", 11, 1, 1.5); FMT("1.50", "%.*f", 2, 1.5);
  FMT("1.5", "%.*g", -1, 1.5);     // precisão negativa: ausente

  // L (long double) e l (ignorado)
  FMT("12.300000", "%Lf", 12.3L); FMT("2.5e+00", "%.1Le", 2.5L); FMT("0.5", "%lg", 0.5);

  // vários na mesma chamada, misturados com inteiros (va_arg de 4 bytes)
  FMT("1 2.0 3 4.00 5", "%d %.1f %d %.2f %d", 1, 2.0f, 3, 4.0, 5);

  printf("pi=%f e=%e g=%g nan=%f -inf=%f\n", 3.14159265, 2.718281828f, 0.1, n, -inf);
  return 0;
}
