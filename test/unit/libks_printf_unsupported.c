// printf: %lld, %f e afins imprimem "<?>" sem consumir argumento (o
// kscc rejeita long long e ponto flutuante, então eles nunca chegam aqui);
// conversão desconhecida idem; %n grava o total (%hhn e %hn num
// char/short); '%' no fim é ignorado.
#include "libks_test.h"
// expect-out: a=<?> b=7
int main() {
  FMT("<?> 5", "%lld %d", 5);
  FMT("<?> 5", "%llu %d", 5);
  FMT("<?> 5", "%llx %d", 5);
  FMT("<?> 6", "%f %d", 6);
  FMT("<?> 6", "%.2f %d", 6);
  FMT("<?> 6", "%e %d", 6);
  FMT("<?> 6", "%g %d", 6);
  FMT("<?> 6", "%a %d", 6);
  FMT("<?> 6", "%Lf %d", 6);
  FMT("<?> 6", "%qd %d", 6);
  FMT("<?> 7", "%k %d", 7);
  FMT("<?><?>", "%f%lld");
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

  printf("a=%f b=%d\n", 7);
  return 0;
}
