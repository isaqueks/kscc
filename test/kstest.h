// test/kstest.h — saída mínima comum aos programas de test/diff, test/fuzz,
// test/bench e test/embedded, para que o mesmo fonte rode no kscc (ksvm,
// AVR, ARM) e no gcc nativo sem depender de printf.
//
//   ks_put_str(s)   string terminada em '\0'
//   ks_put_chr(c)   um byte
//   ks_put_int(v)   decimal com sinal
//   ks_put_uint(v)  decimal sem sinal
//   ks_put_hex(v)   0x + 8 dígitos hexadecimais minúsculos
//   ks_put_kv(k, v) "k = v\n" (v decimal com sinal)
//   ks_sys_exit(c)  termina com o código c
//
// Sob o kscc as rotinas são escritas em C aqui mesmo, sobre as syscalls
// ks_sys_puts/ks_sys_exit (existem tanto no lib/minirt.ksasm das fases 0-3
// quanto na libks, doc 05 §1). Não dependemos dos ks_put_* do minirt porque a
// libks não promete mantê-los; como as funções daqui são static, viram
// _S<id>_ks_put_* no assembly e não colidem com os símbolos globais do
// runtime. Sob o gcc, as mesmas funções usam printf/exit.
#ifndef KSTEST_H
#define KSTEST_H

#ifdef __kscc__

int ks_sys_puts(const char *p, int len);
void ks_sys_exit(int code);

static inline void ks_put_str(const char *s) {
  int n = 0;
  while (s[n])
    n++;
  ks_sys_puts(s, n);
}

static inline void ks_put_chr(int c) {
  char b = (char)c;
  ks_sys_puts(&b, 1);
}

static inline void ks_put_uint(unsigned v) {
  char buf[12];
  int i = 12;
  do {
    buf[--i] = (char)('0' + v % 10);
    v /= 10;
  } while (v);
  ks_sys_puts(buf + i, 12 - i);
}

static inline void ks_put_int(int v) {
  if (v < 0) {
    ks_put_chr('-');
    ks_put_uint(0u - (unsigned)v);
  } else {
    ks_put_uint((unsigned)v);
  }
}

static inline void ks_put_hex(unsigned v) {
  char buf[10];
  buf[0] = '0';
  buf[1] = 'x';
  for (int i = 0; i < 8; i++)
    buf[2 + i] = "0123456789abcdef"[(v >> (28 - 4 * i)) & 0xF];
  ks_sys_puts(buf, 10);
}

#else // gcc nativo

#include <stdio.h>
#include <stdlib.h>

static inline void ks_put_str(const char *s) { fputs(s, stdout); }
static inline void ks_put_chr(int c) { putchar(c); }
static inline void ks_put_uint(unsigned v) { printf("%u", v); }
static inline void ks_put_int(int v) { printf("%d", v); }
static inline void ks_put_hex(unsigned v) { printf("0x%08x", v); }
static inline void ks_sys_exit(int code) {
  fflush(stdout);
  exit(code);
}

#endif

static inline void ks_put_kv(const char *k, int v) {
  ks_put_str(k);
  ks_put_str(" = ");
  ks_put_int(v);
  ks_put_chr('\n');
}

static inline void ks_put_kx(const char *k, unsigned v) {
  ks_put_str(k);
  ks_put_str(" = ");
  ks_put_hex(v);
  ks_put_chr('\n');
}

#endif
