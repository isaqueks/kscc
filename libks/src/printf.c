// printf e família da libks (doc 05 §2).
//
// Um único formatador (format) escreve num destino Out, que é stdout (com
// buffer), stderr ou uma string com limite (sprintf/snprintf). Suporta
//   conversões  d i u x X o c s p n %  e  f F e E g G a A
//   flags       - 0 + espaço #
//   largura e precisão, inclusive com *
//   tamanhos    hh h l z t j  (l, z, t e j são 32 bits, como int); L nas
//               conversões de ponto flutuante (long double = binary32)
// Uma conversão desconhecida e qualquer conversão inteira com ll/L/q
// imprime "<?>" e NÃO consome argumento: o kscc rejeita long long (até a
// fase 5), então um argumento desses nunca chega a um printf compilado por
// ele. Consumir 8 bytes "por segurança" desalinharia os argumentos
// seguintes num programa que passou um int por engano.
//
// Ponto flutuante: ver convert_float (conversão exata, igual à do glibc).
// Ela só é ligada quando alguma unidade passa float/double a uma função
// variádica: o printf chama pelo ponteiro __ks_weak_printf_float, uma
// referência fraca (src/link.c, passo 5) que fica nula sem esse uso; aí a
// conversão consome o argumento e imprime "<?>". Sem isso, todo programa
// com printf levaria ~20 KB a mais (o hello não caberia no ATmega328P).
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
    else if (c == ' ')
      sp->sign = sp->sign ? sp->sign : ' ';  // '+' vence ' '
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

// ---------------------------------------------------------------------------
// Ponto flutuante: %f %F %e %E %g %G %a %A
//
// float, double e long double são binary32 (doc 02 §1): o argumento
// promovido a double ocupa 4 bytes e é lido como os seus bits (unsigned).
// Toda a conversão usa só aritmética inteira de 32 bits: o printf não
// executa instrução F nenhuma e funciona também numa VM sem a extensão
// (porta AVR).
//
// A conversão é EXATA: um binary32 finito é m * 2^e (m < 2^24), cuja
// expansão decimal é finita (até 39 dígitos inteiros e 149 de fração, no
// máximo 112 significativos). Os dígitos exatos saem de um inteiro grande
// com limbs de 16 bits, e o arredondamento para a precisão pedida é para
// o mais próximo com empate para o par, sobre o valor exato, como no
// glibc: printf("%.2f", x) dá os mesmos caracteres que o gcc/glibc dão para
// o mesmo float (promovido a double, que o representa exatamente), para
// qualquer precisão. %a imprime a mantissa normalizada (1.xxx, também para
// subnormais, que são normais em double) sem zeros finais.
//
// inf e nan (INF/NAN em F E G A) levam o sinal do bit 31, inclusive "-nan";
// o flag '0' não se aplica a eles.
// ---------------------------------------------------------------------------

#define FD_MAX 120  // dígitos significativos de um binary32 (no máximo 112)

typedef struct {
  char d[FD_MAX];  // dígitos significativos, sem zeros à esquerda nem à direita
  int n;           // quantos (0: o valor é zero)
  int dp;          // posição do ponto: valor = 0.d[0]d[1]... * 10^dp
} Dec;

// Inteiro grande para os dígitos: limbs de 16 bits, little-endian. 12
// limbs (192 bits) cobrem a parte inteira (< 2^128) e a fração (149 bits
// + 4 de folga para o *10).
#define FB_LIMBS 12

// b = b * k + c (k <= 32768, c < 2^16), nos `n` limbs; devolve o vai-um
// final.
static unsigned fb_mul(unsigned short *b, int n, unsigned k, unsigned c) {
  for (int i = 0; i < n; i++) {
    unsigned t = b[i] * k + c;
    b[i] = t;
    c = t >> 16;
  }
  return c;
}

static void dec_push(Dec *x, int c) {
  if (x->n < FD_MAX)
    x->d[x->n++] = c;
}

// Dígitos decimais exatos de m * 2^e (m != 0), parando quando já há
// `maxfrac` dígitos depois do ponto ou `maxsig` significativos: o que
// sobra só importa para o arredondamento, e vira um dígito 1 extra se não
// for zero (o arredondamento olha o dígito seguinte ao último guardado e
// se há algo não nulo depois dele). A parte inteira sai sempre inteira.
static void float_digits(unsigned m, int e, Dec *x, int maxfrac, int maxsig) {
  unsigned short b[FB_LIMBS];
  char ib[44];
  int ni = 0;

  if (maxsig > FD_MAX - 8)
    maxsig = FD_MAX - 8;
  x->n = 0;
  x->dp = 0;
  for (int i = 0; i < FB_LIMBS; i++)
    b[i] = 0;

  // Parte inteira, dígitos do menos significativo para o mais.
  if (e >= 0) {
    // m << e (e <= 104): até 128 bits, 8 limbs; multiplica por 2^15 de
    // cada vez.
    int n = 2;
    b[0] = m & 0xFFFF;
    b[1] = m >> 16;
    for (int left = e; left > 0; left -= 15) {
      int sh = left < 15 ? left : 15;
      unsigned c = fb_mul(b, n, 1u << sh, 0);
      if (c)
        b[n++] = c;
    }
    // Divisões sucessivas por 10^4: quatro dígitos por passada.
    for (;;) {
      while (n > 0 && b[n - 1] == 0)
        n--;
      if (n == 0)
        break;
      unsigned r = 0;
      for (int i = n - 1; i >= 0; i--) {
        unsigned t = (r << 16) | b[i];
        b[i] = t / 10000;
        r = t % 10000;
      }
      for (int k = 0; k < 4; k++, r /= 10)
        ib[ni++] = '0' + r % 10;
    }
    while (ni > 0 && ib[ni - 1] == '0')
      ni--;  // zeros à esquerda do último grupo
  } else if (-e < 24) {
    for (unsigned v = m >> -e; v; v /= 10)
      ib[ni++] = '0' + v % 10;
  }

  for (int i = ni - 1; i >= 0; i--)
    dec_push(x, ib[i]);
  x->dp = ni;

  if (e >= 0)
    return;

  // Fração: F / 2^k com k = -e, F = m mod 2^k. Cada passo F *= 10^4; os
  // bits a partir de k são os quatro próximos dígitos. Termina quando
  // F == 0 (a expansão de 2^-k tem k dígitos) ou no limite pedido.
  int k = -e;
  unsigned f = (k < 24) ? (m & ((1u << k) - 1)) : m;
  int L = k >> 4, bit = k & 15;
  int pos = 0;  // dígitos já gerados depois do ponto
  b[0] = f & 0xFFFF;
  b[1] = f >> 16;
  for (;;) {
    unsigned any = 0;
    for (int i = 0; i <= L; i++)
      any |= b[i];
    if (!any)
      break;
    if (pos >= maxfrac || x->n >= maxsig) {
      dec_push(x, '1');  // resto não nulo (só conta para o arredondamento)
      return;
    }
    fb_mul(b, L + 2, 10000, 0);
    unsigned g = (((unsigned)b[L] | ((unsigned)b[L + 1] << 16)) >> bit) & 0x3FFF;
    b[L] &= (1u << bit) - 1;
    b[L + 1] = 0;
    for (unsigned p10 = 1000; p10; p10 /= 10, pos++) {
      int dg = g / p10 % 10;
      if (x->n == 0 && dg == 0)
        x->dp--;  // zero à esquerda: só desloca o ponto
      else
        dec_push(x, '0' + dg);
    }
  }

  while (x->n > 0 && x->d[x->n - 1] == '0')
    x->n--;
}

// Arredonda x para os r primeiros dígitos (r <= 0: nenhum), para o mais
// próximo, empate para o par, sobre o valor exato.
static void dec_round(Dec *x, int r) {
  if (r >= x->n)
    return;
  if (r < 0) {
    // valor < 10^dp <= 10^(r - prec) ... menor que meia unidade: zero
    x->n = 0;
    return;
  }

  int c = x->d[r], up;
  if (c != '5')
    up = c > '5';
  else if (r + 1 < x->n)
    up = 1;  // há dígito não nulo depois (os zeros finais já saíram)
  else
    up = r > 0 && ((x->d[r - 1] - '0') & 1);  // empate: para o par

  x->n = r;
  if (up) {
    int i = r - 1;
    while (i >= 0 && x->d[i] == '9')
      i--;
    if (i < 0) {
      // 99.9 -> 100: um dígito a mais à esquerda
      x->d[0] = '1';
      x->n = 1;
      x->dp++;
    } else {
      x->d[i]++;
      x->n = i + 1;
    }
  }
  while (x->n > 0 && x->d[x->n - 1] == '0')
    x->n--;
}

static int dec_dig(Dec *x, int i) {
  return (i >= 0 && i < x->n) ? x->d[i] : '0';
}

// Corpo de uma conversão de ponto flutuante (sem sinal, prefixo e
// preenchimento). Com emit = 0 só mede.
typedef struct {
  int style;  // 'f', 'e', 'a' ou 's' (texto: inf/nan)
  int upper;
  Dec *x;     // 'f' e 'e'
  int nf;     // dígitos depois do ponto
  int point;  // imprime o ponto
  int exp;    // 'e': expoente decimal; 'a': expoente binário
  const char *text;
  unsigned lead, frac;  // 'a': dígito antes do ponto e nhex dígitos depois
  int nhex;
} FBody;

static int body_put(Out *o, int c, int emit) {
  if (emit)
    put(o, c);
  return 1;
}

static int body_exp(Out *o, int e, int mindig, int emit) {
  char eb[12];
  int ne = 0, len = 0;
  unsigned a = e < 0 ? 0u - (unsigned)e : (unsigned)e;

  len += body_put(o, e < 0 ? '-' : '+', emit);
  do {
    eb[ne++] = '0' + a % 10;
    a /= 10;
  } while (a);
  while (ne < mindig)
    eb[ne++] = '0';
  while (ne > 0)
    len += body_put(o, eb[--ne], emit);
  return len;
}

static int float_body(Out *o, FBody *b, int emit) {
  const char *hex = b->upper ? "0123456789ABCDEF" : "0123456789abcdef";
  int len = 0;

  switch (b->style) {
  case 'f':
    if (b->x->dp <= 0)
      len += body_put(o, '0', emit);
    for (int i = 0; i < b->x->dp; i++)
      len += body_put(o, dec_dig(b->x, i), emit);
    if (b->point)
      len += body_put(o, '.', emit);
    for (int j = 0; j < b->nf; j++)
      len += body_put(o, dec_dig(b->x, b->x->dp + j), emit);
    return len;
  case 'e':
    len += body_put(o, dec_dig(b->x, 0), emit);
    if (b->point)
      len += body_put(o, '.', emit);
    for (int j = 0; j < b->nf; j++)
      len += body_put(o, dec_dig(b->x, 1 + j), emit);
    len += body_put(o, b->upper ? 'E' : 'e', emit);
    return len + body_exp(o, b->exp, 2, emit);
  case 'a':
    len += body_put(o, hex[b->lead], emit);
    if (b->point)
      len += body_put(o, '.', emit);
    for (int j = 0; j < b->nf; j++)
      len += body_put(o, j < b->nhex ? hex[(b->frac >> (4 * (b->nhex - 1 - j))) & 15] : '0', emit);
    len += body_put(o, b->upper ? 'P' : 'p', emit);
    return len + body_exp(o, b->exp, 1, emit);
  }
  for (const char *s = b->text; *s; s++)
    len += body_put(o, *s, emit);
  return len;
}

// Sinal, "0x" (%a), largura com espaços ou zeros (depois do sinal e do
// prefixo; nunca em inf/nan) e o corpo.
static void put_float(Out *o, Spec *sp, int sign, FBody *b) {
  int prefix = b->style == 'a' ? 2 : 0;
  int zeros = sp->zero && !sp->left && b->style != 's';
  int len = (sign != 0) + prefix + float_body(o, b, 0);
  int fill = sp->width > len ? sp->width - len : 0;

  if (!sp->left && !zeros)
    pad(o, ' ', fill);
  if (sign)
    put(o, sign);
  if (prefix) {
    put(o, '0');
    put(o, b->upper ? 'X' : 'x');
  }
  if (zeros)
    pad(o, '0', fill);
  float_body(o, b, 1);
  if (sp->left)
    pad(o, ' ', fill);
}

// %a: m * 2^e com m normalizado em [2^23, 2^24): 1.xxxxxx (6 dígitos
// hexadecimais = os 23 bits da fração seguidos de um 0) * 2^(e + 23).
// Sem precisão: sem zeros finais (o mínimo exato). Com precisão < 6:
// arredonda para o mais próximo, empate para o par (o dígito antes do
// ponto conta: 0x1.8p+0 com %.0a dá 0x2p+0).
static void float_hex(Spec *sp, unsigned m, int e, FBody *b) {
  b->style = 'a';
  b->lead = 0;
  b->frac = 0;
  b->nhex = 0;
  b->exp = 0;
  if (m) {
    while (m < 0x800000) {
      m <<= 1;
      e--;
    }
    unsigned f = (m & 0x7FFFFF) << 1;
    int nh = 6;
    b->lead = 1;
    if (sp->prec >= 0 && sp->prec < 6) {
      int drop = 4 * (6 - sp->prec);
      unsigned keep = f >> drop;
      unsigned rem = f & ((1u << drop) - 1);
      unsigned half = 1u << (drop - 1);
      unsigned last = sp->prec > 0 ? keep : b->lead;
      if (rem > half || (rem == half && (last & 1)))
        keep++;
      if (keep >> (4 * sp->prec)) {
        keep = 0;
        b->lead++;
      }
      f = keep;
      nh = sp->prec;
    } else if (sp->prec < 0) {
      while (nh > 0 && (f & 15) == 0) {
        f >>= 4;
        nh--;
      }
    }
    b->frac = f;
    b->nhex = nh;
    b->exp = e + 23;
  }
  b->nf = sp->prec < 0 ? b->nhex : sp->prec;
  b->point = b->nf > 0 || sp->alt;
}

static void convert_float(Out *o, Spec *sp, int conv, va_list *ap) {
  // O double promovido: 4 bytes com os bits do binary32.
  unsigned bits = va_arg(*ap, unsigned);
  int lc = conv | 0x20;  // f e g a
  unsigned ex = (bits >> 23) & 0xFF;
  unsigned m = bits & 0x7FFFFF;
  int sign = (bits >> 31) ? '-' : sp->sign;
  FBody b = {0};
  Dec x;

  b.upper = conv != lc;
  if (ex == 0xFF) {
    b.style = 's';
    if (m)
      b.text = b.upper ? "NAN" : "nan";
    else
      b.text = b.upper ? "INF" : "inf";
    put_float(o, sp, sign, &b);
    return;
  }

  // valor = m * 2^e
  int e = -149;
  if (ex) {
    m |= 0x800000;
    e = (int)ex - 150;
  }

  if (lc == 'a') {
    float_hex(sp, m, e, &b);
    put_float(o, sp, sign, &b);
    return;
  }

  // Dígitos necessários: o arredondamento olha o dígito seguinte ao último
  // impresso (f: prec + 1 depois do ponto; e: prec + 2 significativos; g: P
  // + 1, com folga).
  int prec = sp->prec < 0 ? 6 : sp->prec;
  int P = sp->prec < 0 ? 6 : (sp->prec == 0 ? 1 : sp->prec);
  if (m) {
    if (lc == 'f')
      float_digits(m, e, &x, prec + 1, FD_MAX);
    else
      float_digits(m, e, &x, 1 << 30, (lc == 'e' ? prec : P) + 2);
  } else {
    x.n = 0;
    x.dp = 0;
  }
  b.x = &x;

  if (lc == 'f') {
    dec_round(&x, x.dp + prec);
    b.style = 'f';
    b.nf = prec;
  } else if (lc == 'e') {
    if (x.n)
      dec_round(&x, prec + 1);
    b.style = 'e';
    b.nf = prec;
    b.exp = x.n ? x.dp - 1 : 0;
  } else {
    // %g (C11 7.21.6.1p8): P dígitos significativos; X é o expoente que o
    // estilo e daria. Estilo f se P > X >= -4, senão e; sem '#', sem zeros
    // finais na fração (nem o ponto, se ela some).
    int X = 0;
    if (x.n) {
      Dec y = x;
      dec_round(&y, P);
      X = y.dp - 1;
    }
    if (P > X && X >= -4) {
      int fp = P - 1 - X;
      dec_round(&x, x.dp + fp);
      b.style = 'f';
      b.nf = fp;
      if (!sp->alt) {
        int have = x.n - x.dp;
        b.nf = have < 0 ? 0 : (have < fp ? have : fp);
      }
    } else {
      dec_round(&x, P);
      b.style = 'e';
      b.exp = x.dp - 1;
      b.nf = P - 1;
      if (!sp->alt) {
        int have = x.n - 1;
        b.nf = have < 0 ? 0 : (have < P - 1 ? have : P - 1);
      }
    }
  }
  b.point = b.nf > 0 || sp->alt;
  put_float(o, sp, sign, &b);
}

// Ponto de entrada da conversão de ponto flutuante: só alcançável pelo
// ponteiro inicializado em printf_float.c.
void __ks_printf_float(void *o, void *sp, int conv, va_list *ap) {
  convert_float(o, sp, conv, ap);
}

// Definição provisória (símbolo comum): sem printf_float.c no link, vale 0.
void (*__ks_weak_printf_float)(void *o, void *sp, int conv, va_list *ap);

static int is_float_conv(int c) {
  c |= 0x20;
  return c == 'f' || c == 'e' || c == 'g' || c == 'a';
}

// Uma conversão. Devolve 0 se `conv` não é suportada (ver o topo).
static int convert(Out *o, Spec *sp, int conv, va_list *ap) {
  // L (long double) só nas conversões de ponto flutuante.
  if (is_float_conv(conv)) {
    if (__ks_weak_printf_float) {
      __ks_weak_printf_float(o, sp, conv, ap);
    } else {
      (void)va_arg(*ap, unsigned);
      put_n(o, "<?>", 3);
    }
    return 1;
  }
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
  case 'n': {
    // hh e h gravam num signed char/short: gravar um int pisaria nos
    // bytes vizinhos do objeto apontado.
    void *p = va_arg(*ap, void *);
    if (sp->size == 'H')
      *(signed char *)p = o->len;
    else if (sp->size == 'h')
      *(short *)p = o->len;
    else
      *(int *)p = o->len;
    return 1;
  }
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
