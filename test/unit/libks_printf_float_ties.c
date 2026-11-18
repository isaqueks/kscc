// printf %e/%g de floats inteiros grandes (expoente binário >= 0) com
// empate exato no dígito de arredondamento: o empate vai para o par, como no
// glibc. A parte inteira desses valores guardava os zeros finais, e
// dec_round tomava o empate por "há dígito não nulo depois" e arredondava
// para cima (25000000 com %.0e dava "3e+07").
#include "libks_test.h"
int main() {
  FMT("2e+07", "%.0e", 25000000.0f);          // 2.5e7 exato: empate, para o par
  FMT("2.e+07", "%#.0e", 25000000.0f);
  FMT("2e+07", "%.1g", 25000000.0f);
  FMT("2e+07", "%.0g", 25000000.0f);
  FMT("1.2e+08", "%.1e", 125000000.0f);       // 1.25e8 exato
  FMT("1.2e+08", "%.2g", 125000000.0f);
  FMT("1.23456e+07", "%g", 12345650.0f);      // 1.2345650e7 exato, %g padrão
  FMT("1.23456E+07", "%G", 12345650.0f);
  FMT("1.23456e+07", "%#g", 12345650.0f);
  FMT("4e+07", "%.0e", 35000000.0f);          // empate com ímpar antes: para cima
  FMT("3e+07", "%.0e", 25000002.0f);          // acima do empate
  FMT("25000000", "%.0f", 25000000.0f);       // %f não arredonda a parte inteira
  FMT("2.500000e+07", "%e", 25000000.0f);
  return 0;
}
