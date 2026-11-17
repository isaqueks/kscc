// printf: %lld, %f e afins imprimem "<?>" sem consumir argumento (o
// kscc rejeita long long e ponto flutuante, então eles nunca chegam aqui);
// conversão desconhecida idem; %n grava o total; '%' no fim é ignorado.
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

  printf("a=%f b=%d\n", 7);
  return 0;
}
