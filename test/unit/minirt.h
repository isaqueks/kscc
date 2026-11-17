// minirt.h — ajudantes dos testes unitários das fases 0–3.
//
// Até a fase 3 estas funções eram lib/minirt.ksasm. Com a libks, viraram
// C sobre os wrappers de syscall (<ksvm/syscall.h>), sem passar pelo
// buffer de stdout: assim os testes antigos, que misturam ks_put_* com
// ks_sys_putc, continuam com a saída na ordem em que foi produzida, e não
// dependem de printf (que tem testes próprios). `static inline` para que
// só as usadas sejam emitidas.
#ifndef KSCC_MINIRT_H
#define KSCC_MINIRT_H

#include <ksvm/syscall.h>

static inline void ks_put_str(const char *s) {
  unsigned n = 0;
  while (s[n])
    n++;
  ks_sys_puts(s, n);
}

// Decimal sem sinal, escrito de trás para frente.
static inline void ks_put_uint(unsigned v) {
  char buf[10];
  int i = 10;
  do {
    buf[--i] = '0' + v % 10;
    v /= 10;
  } while (v);
  ks_sys_puts(buf + i, 10 - i);
}

// INT_MIN vira 2147483648 sem sinal, que ks_put_uint imprime certo.
static inline void ks_put_int(int v) {
  if (v < 0) {
    ks_sys_putc('-');
    ks_put_uint(0u - (unsigned)v);
  } else {
    ks_put_uint(v);
  }
}

// 0x + 8 dígitos minúsculos.
static inline void ks_put_hex(unsigned v) {
  ks_sys_puts("0x", 2);
  for (int shift = 28; shift >= 0; shift -= 4)
    ks_sys_putc("0123456789abcdef"[(v >> shift) & 15]);
}

// Se diferentes: "assert_eq falhou (linha L): esperado E, obtido A" e
// saída com 1.
static inline void ks_assert_eq(int expected, int actual, int line) {
  if (expected == actual)
    return;
  ks_put_str("assert_eq falhou (linha ");
  ks_put_int(line);
  ks_put_str("): esperado ");
  ks_put_int(expected);
  ks_put_str(", obtido ");
  ks_put_int(actual);
  ks_sys_putc('\n');
  ks_sys_exit(1);
}

#define EQ(expected, actual) ks_assert_eq((expected), (actual), __LINE__)
#define NL() ks_sys_putc('\n')

#endif
