// printf e família da libks (doc 05 §2).
//
// Um único formatador (format) escreve num destino Out, que é stdout (com
// buffer), stderr ou uma string com limite (sprintf/snprintf). Suporta
//   conversões  d i u x X o c s p n %
//   flags       - 0 + espaço #
//   largura e precisão, inclusive com *
//   tamanhos    hh h l z t j  (l, z, t e j são 32 bits, como int)
// Qualquer outra conversão (%f %e %g %a, ou desconhecida) e qualquer
// conversão com ll/L/q imprime "<?>" e NÃO consome argumento: o kscc
// rejeita long long (até a fase 5) e ponto flutuante (até a fase 6), então
// um argumento desses nunca chega a um printf compilado por ele. Consumir
// 8 bytes "por segurança" desalinharia os argumentos seguintes num
// programa que passou um int por engano.
#include <stdarg.h>
#include <stdio.h>

void __ks_stdout_putc(int c);
void __ks_stderr_putc(int c);

enum { OUT_STDOUT, OUT_STDERR, OUT_BUF };

typedef struct {
  int kind;
  char *buf;   // OUT_BUF: destino
  size_t cap;  // OUT_BUF: bytes de buf, contando o '\0'
  size_t len;  // caracteres produzidos (inclusive os que não couberam)
} Out;

typedef struct {
  int left;   // '-'
  int zero;   // '0'
  int sign;   // '+' ou ' ' (0 se nenhum); '+' vence ' '
  int alt;    // '#'
  int width;
  int prec;   // -1: sem precisão
  int size;   // 'H' (hh), 'h', 0 (int/l/z/t/j) ou 'L' (ll/L/q)
} Spec;

static void put(Out *o, int c) {
  if (o->kind == OUT_BUF) {
    if (o->len + 1 < o->cap)
      o->buf[o->len] = c;
  } else if (o->kind == OUT_STDOUT) {
    __ks_stdout_putc(c);
  } else {
    __ks_stderr_putc(c);
  }
  o->len++;
}

static void put_n(Out *o, const char *s, int n) {
  while (n-- > 0)
    put(o, *s++);
}

static void pad(Out *o, int c, int n) {
  while (n-- > 0)
    put(o, c);
}

static int is_digit(int c) { return c >= '0' && c <= '9'; }

// Lê um número decimal ou '*' (que vem do próximo argumento int).
static int read_count(const char **fmt, va_list *ap) {
  int n = 0;

  if (**fmt == '*') {
    (*fmt)++;
    return va_arg(*ap, int);
  }
  while (is_digit(**fmt))
    n = n * 10 + *(*fmt)++ - '0';
  return n;
}

static void parse_flags(const char **fmt, Spec *sp) {
  for (;; (*fmt)++) {
    int c = **fmt;
    if (c == '-')
      sp->left = 1;
    else if (c == '0')
      sp->zero = 1;
    else if (c == '+')
      sp->sign = '+';
    else if (c == ' ' && sp->sign != '+')
      sp->sign = ' ';
    else if (c == '#')
      sp->alt = 1;
    else
      return;
  }
}

static void parse_size(const char **fmt, Spec *sp) {
  const char *p = *fmt;

  if (p[0] == 'h' && p[1] == 'h') {
    sp->size = 'H';
    p += 2;
  } else if (p[0] == 'l' && p[1] == 'l') {
    sp->size = 'L';
    p += 2;
  } else if (*p == 'h') {
    sp->size = 'h';
    p++;
  } else if (*p == 'L' || *p == 'q') {
    sp->size = 'L';
    p++;
  } else if (*p == 'l' || *p == 'z' || *p == 't' || *p == 'j') {
    p++;
  }
  *fmt = p;
}

// Tudo entre o '%' e a letra da conversão.
static void parse_spec(const char **fmt, va_list *ap, Spec *sp) {
  parse_flags(fmt, sp);

  // Largura negativa vinda de '*' é o flag '-' com o valor absoluto.
  sp->width = read_count(fmt, ap);
  if (sp->width < 0) {
    sp->left = 1;
    sp->width = -sp->width;
  }

  // Precisão negativa vinda de '*' vale como ausente (C11 7.21.6.1p5).
  sp->prec = -1;
  if (**fmt == '.') {
    (*fmt)++;
    sp->prec = read_count(fmt, ap);
    if (sp->prec < 0)
      sp->prec = -1;
  }

  parse_size(fmt, sp);
}

// Inteiro já convertido para magnitude sem sinal. `sign` é o caractere de
// sinal a imprimir (0 se nenhum); `prefix` é "0x"/"0X" ou "".
static void put_int(Out *o, Spec *sp, unsigned v, int base, int upper,
                    int sign, const char *prefix) {
  const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
  char buf[12];  // 11 dígitos octais bastam para 32 bits
  int nd = 0;

  // Precisão 0 com valor 0 não imprime dígito algum (C11 7.21.6.1p8).
  if (v != 0 || sp->prec != 0)
    do {
      buf[nd++] = digits[v % base];
      v /= base;
    } while (v);

  // '#' em octal: o primeiro dígito tem de ser 0.
  int prec = sp->prec;
  if (base == 8 && sp->alt && (nd == 0 || buf[nd - 1] != '0') && prec <= nd)
    prec = nd + 1;

  int plen = 0;
  while (prefix[plen])
    plen++;

  int zeros = prec > nd ? prec - nd : 0;
  int len = (sign != 0) + plen + zeros + nd;

  // '0' completa a largura com zeros depois do sinal/prefixo, mas só sem
  // precisão e sem '-'.
  if (sp->zero && !sp->left && sp->prec < 0 && sp->width > len) {
    zeros += sp->width - len;
    len = sp->width;
  }

  if (!sp->left)
    pad(o, ' ', sp->width - len);
  if (sign)
    put(o, sign);
  put_n(o, prefix, plen);
  pad(o, '0', zeros);
  while (nd > 0)
    put(o, buf[--nd]);
  if (sp->left)
    pad(o, ' ', sp->width - len);
}

static void put_str(Out *o, Spec *sp, const char *s) {
  int n = 0;

  if (!s)
    s = "(null)";
  while (s[n] && (sp->prec < 0 || n < sp->prec))
    n++;
  if (!sp->left)
    pad(o, ' ', sp->width - n);
  put_n(o, s, n);
  if (sp->left)
    pad(o, ' ', sp->width - n);
}

static void put_char(Out *o, Spec *sp, int c) {
  if (!sp->left)
    pad(o, ' ', sp->width - 1);
  put(o, c);
  if (sp->left)
    pad(o, ' ', sp->width - 1);
}

// Argumento com sinal de %d/%i, já reduzido por hh/h.
static int signed_arg(Spec *sp, va_list *ap) {
  int v = va_arg(*ap, int);

  if (sp->size == 'H')
    return (signed char)v;
  if (sp->size == 'h')
    return (short)v;
  return v;
}

static unsigned unsigned_arg(Spec *sp, va_list *ap) {
  unsigned v = va_arg(*ap, unsigned);

  if (sp->size == 'H')
    return (unsigned char)v;
  if (sp->size == 'h')
    return (unsigned short)v;
  return v;
}

static void convert_signed(Out *o, Spec *sp, va_list *ap) {
  int v = signed_arg(sp, ap);
  unsigned mag = v < 0 ? 0u - (unsigned)v : (unsigned)v;

  put_int(o, sp, mag, 10, 0, v < 0 ? '-' : sp->sign, "");
}

static void convert_hex(Out *o, Spec *sp, int conv, va_list *ap) {
  unsigned v = unsigned_arg(sp, ap);
  const char *prefix = "";

  if (sp->alt && v)
    prefix = conv == 'x' ? "0x" : "0X";
  put_int(o, sp, v, 16, conv == 'X', 0, prefix);
}

// Uma conversão. Devolve 0 se `conv` não é suportada (ver o topo).
static int convert(Out *o, Spec *sp, int conv, va_list *ap) {
  if (sp->size == 'L')
    return 0;

  switch (conv) {
  case 'd':
  case 'i':
    convert_signed(o, sp, ap);
    return 1;
  case 'u':
    put_int(o, sp, unsigned_arg(sp, ap), 10, 0, 0, "");
    return 1;
  case 'o':
    put_int(o, sp, unsigned_arg(sp, ap), 8, 0, 0, "");
    return 1;
  case 'x':
  case 'X':
    convert_hex(o, sp, conv, ap);
    return 1;
  case 'p':
    // Sempre "0x" + hexadecimal mínimo (NULL é "0x0").
    put_int(o, sp, (unsigned)va_arg(*ap, void *), 16, 0, 0, "0x");
    return 1;
  case 'c':
    put_char(o, sp, (unsigned char)va_arg(*ap, int));
    return 1;
  case 's':
    put_str(o, sp, va_arg(*ap, const char *));
    return 1;
  case 'n':
    *va_arg(*ap, int *) = o->len;
    return 1;
  case '%':
    put(o, '%');
    return 1;
  }
  return 0;
}

static int format(Out *o, const char *fmt, va_list ap) {
  while (*fmt) {
    if (*fmt != '%') {
      put(o, *fmt++);
      continue;
    }
    fmt++;

    Spec sp = {0};
    parse_spec(&fmt, &ap, &sp);
    if (!*fmt)
      break;  // '%' incompleto no fim do formato
    if (!convert(o, &sp, *fmt, &ap))
      put_n(o, "<?>", 3);
    fmt++;
  }

  if (o->kind == OUT_BUF && o->cap > 0)
    o->buf[o->len < o->cap ? o->len : o->cap - 1] = '\0';
  return o->len;
}

int vfprintf(FILE *restrict f, const char *restrict fmt, va_list ap) {
  Out o = {0};

  if (f == stdout) {
    o.kind = OUT_STDOUT;
  } else if (f == stderr) {
    o.kind = OUT_STDERR;
    fflush(stdout);  // mantém a ordem no console comum
  } else {
    return -1;
  }
  return format(&o, fmt, ap);
}

int vsnprintf(char *restrict buf, size_t n, const char *restrict fmt, va_list ap) {
  Out o = {OUT_BUF, buf, n, 0};
  return format(&o, fmt, ap);
}

// Sem limite: o maior size_t faz `len + 1 < cap` valer sempre.
int vsprintf(char *restrict buf, const char *restrict fmt, va_list ap) {
  return vsnprintf(buf, (size_t)-1, fmt, ap);
}

int vprintf(const char *restrict fmt, va_list ap) {
  return vfprintf(stdout, fmt, ap);
}

int printf(const char *restrict fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vfprintf(stdout, fmt, ap);
  va_end(ap);
  return n;
}

int fprintf(FILE *restrict f, const char *restrict fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vfprintf(f, fmt, ap);
  va_end(ap);
  return n;
}

int sprintf(char *restrict buf, const char *restrict fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vsprintf(buf, fmt, ap);
  va_end(ap);
  return n;
}

int snprintf(char *restrict buf, size_t n, const char *restrict fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsnprintf(buf, n, fmt, ap);
  va_end(ap);
  return r;
}
