// printf: %d %i %u com flags (- 0 + espaço), largura, precisão e bordas.
#include <limits.h>
#include "libks_test.h"
// expect-out: [42] [-7] [+5] [ 5]
// expect-out: [   42] [42   ] [00042] [-0042] [+0042]
int main() {
  FMT("0", "%d", 0);
  FMT("-1", "%d", -1);
  FMT("2147483647", "%d", INT_MAX);
  FMT("-2147483648", "%d", INT_MIN);
  FMT("123", "%i", 123);
  FMT("4294967295", "%u", 4294967295u);
  FMT("4294967295", "%u", -1);
  FMT("0", "%u", 0);

  // Flags
  FMT("+5", "%+d", 5);
  FMT("-5", "%+d", -5);
  FMT("+0", "%+d", 0);
  FMT(" 5", "% d", 5);
  FMT("-5", "% d", -5);
  FMT("+5", "%+ d", 5);   // '+' vence ' '
  FMT("+5", "% +d", 5);
  FMT("5", "%+u", 5);     // '+' não vale para sem sinal

  // Largura e alinhamento
  FMT("   42", "%5d", 42);
  FMT("42   ", "%-5d", 42);
  FMT("00042", "%05d", 42);
  FMT("-0042", "%05d", -42);
  FMT("+0042", "%+05d", 42);
  FMT(" 0042", "% 05d", 42);
  FMT("42   ", "%-05d", 42);  // '-' vence '0'
  FMT("123456", "%3d", 123456);
  FMT("    7", "%*d", 5, 7);
  FMT("7    ", "%*d", -5, 7);  // largura negativa = '-'
  FMT("7", "%0d", 7);

  // Precisão: mínimo de dígitos; com precisão o '0' é ignorado
  FMT("00042", "%.5d", 42);
  FMT("-00042", "%.5d", -42);
  FMT("  00042", "%7.5d", 42);
  FMT("  00042", "%07.5d", 42);
  FMT("", "%.0d", 0);       // precisão 0 com valor 0: nenhum dígito
  FMT("   ", "%3.0d", 0);
  FMT("+", "%+.0d", 0);
  FMT("42", "%.0d", 42);
  FMT("42", "%.d", 42);     // "." sozinho = precisão 0
  FMT("0042", "%.*d", 4, 42);
  FMT("42", "%.*d", -1, 42);  // precisão negativa = ausente
  FMT("00042", "%0*.*d", 5, -1, 42);  // e o 0 volta a valer

  // printf de verdade (stdout com buffer)
  printf("[%d] [%d] [%+d] [% d]\n", 42, -7, 5, 5);
  int n = printf("[%5d] [%-5d] [%05d] [%05d] [%+05d]\n", 42, 42, 42, -42, 42);
  EQ(40, n);
  return 0;
}
