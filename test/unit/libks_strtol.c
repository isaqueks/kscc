// strtol: bases 0, 2, 8, 10, 16 e 36, prefixos, espaços, sinal, endptr,
// estouro (ERANGE com LONG_MAX/LONG_MIN), sem dígitos e base inválida.
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include "minirt.h"

static const char *end;

static long parse(const char *s, int base) {
  return strtol(s, (char **)&end, base);
}

int main() {
  const char *s;

  s = "123";
  EQ(123, parse(s, 10));
  EQ(3, end - s);
  s = "  \t\n-42xyz";
  EQ(-42, parse(s, 10));
  EQ(7, end - s);
  s = "+17";
  EQ(17, parse(s, 0));

  // Bases e prefixos
  EQ(255, parse("ff", 16));
  EQ(255, parse("0xFF", 16));
  EQ(255, parse("0Xff", 0));
  EQ(8, parse("010", 0));
  EQ(10, parse("010", 10));
  EQ(5, parse("101", 2));
  EQ(511, parse("777", 8));
  EQ(35, parse("z", 36));
  EQ(36 * 36 - 1, parse("ZZ", 36));
  s = "129";
  EQ(1, parse(s, 2));  // para no primeiro dígito inválido
  EQ(1, end - s);

  // "0x" sem dígito hexadecimal: o número é 0 e o resto começa no 'x'
  s = "0xg";
  EQ(0, parse(s, 16));
  EQ(1, end - s);
  s = "0x";
  EQ(0, parse(s, 0));
  EQ(1, end - s);

  // Sem dígitos: 0 e endptr = início
  s = "  -abc";
  EQ(0, parse(s, 10));
  EQ(0, end - s);
  s = "";
  EQ(0, parse(s, 10));
  EQ(0, end - s);

  // Limites e estouro
  errno = 0;
  EQ(LONG_MAX, parse("2147483647", 10));
  EQ(LONG_MIN, parse("-2147483648", 10));
  EQ(0, errno);
  s = "2147483648";
  EQ(LONG_MAX, parse(s, 10));
  EQ(ERANGE, errno);
  EQ(10, end - s);  // consome todos os dígitos mesmo estourando
  errno = 0;
  EQ(LONG_MIN, parse("-2147483649", 10));
  EQ(ERANGE, errno);
  errno = 0;
  EQ(LONG_MAX, parse("0x7fffffff", 0));
  EQ(0, errno);
  EQ(LONG_MAX, parse("0x80000000", 0));
  EQ(ERANGE, errno);
  errno = 0;
  EQ(LONG_MAX, parse("99999999999999999999", 10));
  EQ(ERANGE, errno);

  // Base inválida
  errno = 0;
  s = "123";
  EQ(0, parse(s, 1));
  EQ(EINVAL, errno);
  EQ(0, end - s);
  errno = 0;
  EQ(0, parse(s, 37));
  EQ(EINVAL, errno);

  // endptr NULL é permitido
  EQ(-9, strtol("-9", 0, 10));
  return 0;
}
