// __builtin_* suportados pelo kscc: __builtin_alloca, __builtin_expect,
// __builtin_types_compatible_p (resolvido no parser) e offsetof
// (macro de <stddef.h>).
#include "minirt.h"
#include <stddef.h>
struct S { char c; int i; short s[3]; };

int classify(int x) {
  if (__builtin_expect(x < 0, 0))
    return -1;
  return __builtin_expect(x, 5) == 5;
}

int main() {
  char *p = __builtin_alloca(12);
  p[11] = 3;
  EQ(3, p[11]);
  EQ(-1, classify(-4));
  EQ(1, classify(5));
  EQ(0, classify(6));

  EQ(1, __builtin_types_compatible_p(int, signed));
  EQ(0, __builtin_types_compatible_p(int, unsigned));
  EQ(0, __builtin_types_compatible_p(int, long)); // tipos distintos, mesmo tamanho
  EQ(1, __builtin_types_compatible_p(char *, char *));
  EQ(0, __builtin_types_compatible_p(char *, unsigned char *));
  EQ(1, __builtin_types_compatible_p(struct S, struct S));

  EQ(4, offsetof(struct S, i));
  EQ(10, offsetof(struct S, s[1]));
  return 0;
}
