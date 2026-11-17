// Varargs com <stdarg.h>: soma, máximo, lista terminada por sentinela,
// va_copy, repasse de va_list e um mini formatador (%d %u %x %s %c %%).
#include "kstest.h"
#include <stdarg.h>

static int sum(int n, ...) {
  va_list ap;
  va_start(ap, n);
  int s = 0;
  for (int i = 0; i < n; i++)
    s += va_arg(ap, int);
  va_end(ap);
  return s;
}

static int max_of(int n, ...) {
  va_list ap;
  va_start(ap, n);
  int m = va_arg(ap, int);
  for (int i = 1; i < n; i++) {
    int v = va_arg(ap, int);
    if (v > m) m = v;
  }
  va_end(ap);
  return m;
}

// strings até NULL; devolve comprimento total
static int total_len(const char *first, ...) {
  va_list ap;
  va_start(ap, first);
  int n = 0;
  for (const char *s = first; s; s = va_arg(ap, const char *))
    while (*s++) n++;
  va_end(ap);
  return n;
}

// percorre duas vezes com va_copy
static int sum_and_count_pos(int n, ...) {
  va_list ap, ap2;
  va_start(ap, n);
  va_copy(ap2, ap);
  int s = 0, pos = 0;
  for (int i = 0; i < n; i++) s += va_arg(ap, int);
  for (int i = 0; i < n; i++) pos += va_arg(ap2, int) > 0;
  va_end(ap2);
  va_end(ap);
  return s * 100 + pos;
}

static char out[256];
static int outn;

static void emit(char c) {
  if (outn < 255) out[outn++] = c;
}

static void emit_uint(unsigned v, unsigned base, int width, char pad) {
  char tmp[12];
  int n = 0;
  do {
    tmp[n++] = "0123456789abcdef"[v % base];
    v /= base;
  } while (v);
  for (int i = n; i < width; i++) emit(pad);
  while (n) emit(tmp[--n]);
}

static int vfmt(const char *f, va_list ap) {
  int start = outn;
  for (; *f; f++) {
    if (*f != '%') { emit(*f); continue; }
    f++;
    char pad = ' ';
    int width = 0;
    if (*f == '0') { pad = '0'; f++; }
    while (*f >= '0' && *f <= '9') width = width * 10 + (*f++ - '0');
    switch (*f) {
    case 'd': {
      int v = va_arg(ap, int);
      if (v < 0) { emit('-'); emit_uint(0u - (unsigned)v, 10, width - 1, pad); }
      else emit_uint((unsigned)v, 10, width, pad);
      break;
    }
    case 'u': emit_uint(va_arg(ap, unsigned), 10, width, pad); break;
    case 'x': emit_uint(va_arg(ap, unsigned), 16, width, pad); break;
    case 'c': emit((char)va_arg(ap, int)); break;
    case 's': for (const char *s = va_arg(ap, const char *); *s; s++) emit(*s); break;
    case '%': emit('%'); break;
    default: emit('?'); break;
    }
  }
  return outn - start;
}

static int fmt(const char *f, ...) {
  va_list ap;
  va_start(ap, f);
  int n = vfmt(f, ap);
  va_end(ap);
  return n;
}

// variádica com nomeados de tipos pequenos e struct no meio dos anônimos
typedef struct { int a; char b; short c; } Trio;
static int mixed(char c, short s, ...) {
  va_list ap;
  va_start(ap, s);
  int x = va_arg(ap, int);           // char promovido
  Trio t = va_arg(ap, Trio);
  unsigned u = va_arg(ap, unsigned); // unsigned short promovido
  va_end(ap);
  return c + s + x + t.a + t.b + t.c + (int)u;
}

int main(void) {
  ks_put_kv("sum0", sum(0));
  ks_put_kv("sum5", sum(5, 1, 2, 3, 4, 5));
  ks_put_kv("sum12", sum(12, 1, -2, 3, -4, 5, -6, 7, -8, 9, -10, 11, -12));
  ks_put_kv("max", max_of(7, -5, 3, 99, -100, 42, 98, 0));
  ks_put_kv("max1", max_of(1, -7));
  ks_put_kv("total_len", total_len("abc", "", "defgh", "ij", (const char *)0));
  ks_put_kv("copy", sum_and_count_pos(6, 10, -3, 5, 0, 8, -1));
  int n = fmt("[%d|%5d|%d|%05u|%x|%08x|%c%c|%s|%%|%q]", 42, -7, 0, 123u, 0xBEEFu, 255u, 'o', 'k', "str");
  n += fmt(" %d %d %u", -2147483647 - 1, 2147483647, 4294967295u);
  out[outn] = 0;
  ks_put_str(out);
  ks_put_chr('\n');
  ks_put_kv("n", n);
  Trio t = {1000, 'x', -300};
  char c = 'A';
  unsigned short us = 65000;
  ks_put_kv("mixed", mixed(5, -6, c, t, us));
  return 0;
}
