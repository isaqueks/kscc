// <stdlib.h> da libks, parte aritmética (doc 05 §2): abs, div, conversão
// de strings e rand. exit/atexit estão em exit.c, qsort/bsearch em
// qsort.c e malloc em malloc.c.
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

// abs(INT_MIN) não é representável (comportamento indefinido em C);
// aqui devolve INT_MIN, o que a negação em complemento de 2 dá.
int abs(int n) { return n < 0 ? -n : n; }
long labs(long n) { return n < 0 ? -n : n; }

div_t div(int num, int den) {
  div_t r = {num / den, num % den};
  return r;
}

ldiv_t ldiv(long num, long den) {
  ldiv_t r = {num / den, num % den};
  return r;
}

// Valor de um dígito em qualquer base até 36, ou 99 se não é dígito.
static int digit_value(int c) {
  if (isdigit(c))
    return c - '0';
  if (isalpha(c))
    return tolower(c) - 'a' + 10;
  return 99;
}

// Parte comum de strtol e strtoul: espaços, sinal, prefixo da base e
// dígitos, acumulando em unsigned com saturação em `limit`. Devolve a
// magnitude; *neg diz se havia '-', *overflow se passou de limit, e *end
// aponta para o primeiro caractere não consumido (s, se não havia
// dígitos).
static unsigned long scan(const char *s, char **end, int base, int *neg,
                          int *overflow, unsigned long limit_pos,
                          unsigned long limit_neg) {
  const char *p = s;
  unsigned long acc = 0;
  int any = 0;

  *neg = 0;
  *overflow = 0;
  if (base < 0 || base == 1 || base > 36) {
    errno = EINVAL;
    if (end)
      *end = (char *)s;
    return 0;
  }

  while (isspace(*p))
    p++;
  if (*p == '+' || *p == '-')
    *neg = *p++ == '-';

  // "0x" só é prefixo se vier um dígito hexadecimal depois: em "0xg" o
  // número é o 0 e o resto começa no 'x'.
  if ((base == 0 || base == 16) && p[0] == '0' && tolower(p[1]) == 'x' &&
      isxdigit(p[2])) {
    p += 2;
    base = 16;
  } else if (base == 0) {
    base = p[0] == '0' ? 8 : 10;
  }

  unsigned long limit = *neg ? limit_neg : limit_pos;
  for (;; p++) {
    int d = digit_value(*p);
    if (d >= base)
      break;
    any = 1;
    // acc * base + d > limit, sem estourar 32 bits
    if (acc > (limit - d) / base)
      *overflow = 1;
    else
      acc = acc * base + d;
  }

  if (end)
    *end = (char *)(any ? p : s);
  return acc;
}

long strtol(const char *restrict s, char **restrict end, int base) {
  int neg, overflow;
  unsigned long v = scan(s, end, base, &neg, &overflow, LONG_MAX,
                         (unsigned long)LONG_MAX + 1);

  if (overflow) {
    errno = ERANGE;
    return neg ? LONG_MIN : LONG_MAX;
  }
  return neg ? (long)(0 - v) : (long)v;
}

// Como no C, "-1" é aceito e dá ULONG_MAX (a negação em unsigned).
unsigned long strtoul(const char *restrict s, char **restrict end, int base) {
  int neg, overflow;
  unsigned long v = scan(s, end, base, &neg, &overflow, ULONG_MAX, ULONG_MAX);

  if (overflow) {
    errno = ERANGE;
    return ULONG_MAX;
  }
  return neg ? 0 - v : v;
}

int atoi(const char *s) { return strtol(s, NULL, 10); }
long atol(const char *s) { return strtol(s, NULL, 10); }

// Gerador congruencial do exemplo do C11 (7.22.2.2): determinístico,
// barato e sem tabela. Para entropia de verdade há ks_sys_random().
static unsigned long rand_state = 1;

int rand(void) {
  rand_state = rand_state * 1103515245 + 12345;
  return (rand_state >> 16) & RAND_MAX;
}

void srand(unsigned seed) { rand_state = seed; }
