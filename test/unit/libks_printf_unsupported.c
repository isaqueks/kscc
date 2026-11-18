// printf: %lld e afins imprimem "<?>" sem consumir argumento (o kscc
// rejeita long long, então ele nunca chega aqui); conversão desconhecida
// idem; %n grava o total (%hhn e %hn num char/short); '%' no fim é
// ignorado.
//
// %f %e %g %a sem a conversão de ponto flutuante ligada (esta unidade
// nunca passa float/double a uma função variádica, então o link não a
// inclui: referência fraca __ks_weak_printf_float, doc 04 §3) consomem os
// 4 bytes do argumento e imprimem "<?>". Ver libks_printf_float*.c.
#include "libks_test.h"
// expect-out: a=<?> b=7
int main() {
  FMT("<?> 5", "%lld %d", 5);
  FMT("<?> 5", "%llu %d", 5);
  FMT("<?> 5", "%llx %d", 5);
  FMT("<?> 6", "%f %d", 0, 6);
  FMT("<?> 6", "%.2f %d", 0, 6);
  FMT("<?> 6", "%e %d", 0, 6);
  FMT("<?> 6", "%g %d", 0, 6);
  FMT("<?> 6", "%a %d", 0, 6);
  FMT("<?> 6", "%Lf %d", 0, 6);
  FMT("<?> 6", "%qd %d", 6);
  FMT("<?> 7", "%k %d", 7);
  FMT("<?><?>", "%f%lld", 0);
  FMT("abc", "abc%");  // '%' incompleto no fim

  int n = -1;
  FMT("abc12", "abc%n%d", &n, 12);
  EQ(3, n);
  FMT("", "%n", &n);
  EQ(0, n);

  // %hhn e %hn gravam só um char/short: os vizinhos ficam intactos
  struct { signed char c, after[3]; } b = {9, {0x55, 0x55, 0x55}};
  FMT("abc", "abc%hhn", &b.c);
  EQ(3, b.c);
  EQ(0x55, b.after[0]);
  EQ(0x55, b.after[2]);
  struct { short h, after; } w = {9, 0x2222};
  FMT("abcde", "abcde%hn", &w.h);
  EQ(5, w.h);
  EQ(0x2222, w.after);
  FMT("ab<?>7", "ab%lln%d", 7);  // %lln não consome o ponteiro

  printf("a=%f b=%d\n", 0, 7);
  return 0;
}
