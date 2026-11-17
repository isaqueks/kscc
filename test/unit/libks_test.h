// libks_test.h — verificações dos testes unitários da libks (fase 4).
//
// As mensagens de falha saem por ks_sys_puts, sem passar por printf nem
// pelo buffer de stdout da libks, que são justamente o que muitos destes
// testes verificam.
#ifndef KSCC_LIBKS_TEST_H
#define KSCC_LIBKS_TEST_H

#include <stdarg.h>
#include <stdio.h>
#include "minirt.h"

static inline void ks_fail_str(int line, const char *expected, const char *actual) {
  ks_put_str("falhou (linha ");
  ks_put_int(line);
  ks_put_str("): esperado \"");
  ks_put_str(expected);
  ks_put_str("\", obtido \"");
  ks_put_str(actual);
  ks_put_str("\"\n");
  ks_sys_exit(1);
}

static inline void ks_check_str(int line, const char *expected, const char *actual) {
  int i = 0;
  while (expected[i] && expected[i] == actual[i])
    i++;
  if (expected[i] != actual[i])
    ks_fail_str(line, expected, actual);
}

// Formata com vsnprintf num buffer grande e compara o texto e o valor
// devolvido (que deve ser o comprimento do texto esperado).
static inline void ks_check_fmt(int line, const char *expected, const char *fmt, ...) {
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  ks_check_str(line, expected, buf);
  int len = 0;
  while (expected[len])
    len++;
  ks_assert_eq(len, n, line);
}

#define STR_EQ(expected, actual) ks_check_str(__LINE__, (expected), (actual))
#define FMT(expected, ...) ks_check_fmt(__LINE__, (expected), __VA_ARGS__)

#endif
