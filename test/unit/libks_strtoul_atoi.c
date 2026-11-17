// strtoul (faixa inteira de 32 bits, "-1", estouro), atoi/atol, abs/labs,
// div/ldiv e rand/srand.
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include "minirt.h"

int main() {
  char *end;

  errno = 0;
  EQ((int)4294967295u, (int)strtoul("4294967295", &end, 10));
  EQ(0, errno);
  EQ((int)ULONG_MAX, (int)strtoul("4294967296", &end, 10));
  EQ(ERANGE, errno);
  errno = 0;
  EQ((int)0xdeadbeef, (int)strtoul("0xDEADBEEF", &end, 0));
  EQ(0, *end);
  EQ((int)ULONG_MAX, (int)strtoul("-1", &end, 10));  // negação sem sinal
  EQ(-5, (int)strtoul("-5", &end, 10));
  EQ(0, errno);
  EQ(0, (int)strtoul("x", &end, 10));

  EQ(42, atoi("42"));
  EQ(-42, atoi("  -42abc"));
  EQ(0, atoi("abc"));
  EQ(7, atol("7"));

  EQ(5, abs(-5));
  EQ(5, abs(5));
  EQ(0, abs(0));
  EQ(INT_MIN, abs(INT_MIN));  // não representável: fica INT_MIN
  EQ(9, labs(-9L));

  div_t d = div(-7, 2);
  EQ(-3, d.quot);  // C trunca para zero
  EQ(-1, d.rem);
  ldiv_t ld = ldiv(17L, 5L);
  EQ(3, ld.quot);
  EQ(2, ld.rem);

  // rand: determinístico a partir da semente, dentro de [0, RAND_MAX]
  srand(1);
  int first[5];
  for (int i = 0; i < 5; i++) {
    first[i] = rand();
    EQ(1, first[i] >= 0 && first[i] <= RAND_MAX);
  }
  srand(1);
  for (int i = 0; i < 5; i++)
    EQ(first[i], rand());
  srand(2);
  EQ(1, rand() != first[0]);
  EQ(16838, first[0]);  // o valor do exemplo do C11 com semente 1
  return 0;
}
