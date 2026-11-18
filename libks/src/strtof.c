// strtof, strtod, strtold e atof da libks (doc 05 §2).
//
// float, double e long double são binary32 (doc 02 §1): as quatro funções
// são a mesma conversão. Aceita (C11 7.22.1.3) espaços iniciais, sinal, e
//   decimal      dígitos com '.' opcional e expoente e/E opcional
//   hexadecimal  0x/0X, dígitos hex com '.' opcional e expoente p/P opcional
//   INF, INFINITY, NAN, NAN(n-char-seq)   sem distinção de maiúsculas
// Sem conversão possível, devolve 0 e *end = s.
//
// O resultado é CORRETAMENTE ARREDONDADO (para o mais próximo, empate
// para o par) para qualquer entrada: o valor decimal é levado a uma fração
// num/den de inteiros grandes exatos e dividida até 26 bits de quociente
// mais o resto (sticky). Os primeiros 120 dígitos significativos são
// guardados; os seguintes só contam como "há algo não nulo" (um dígito 1
// extra), o que não muda o arredondamento: o ponto médio entre dois
// binary32 tem no máximo 113 dígitos significativos. Só aritmética inteira
// (nenhuma instrução F: funciona sem a extensão).
//
// errno = ERANGE quando o resultado é ±inf a partir de um número finito
// (overflow) ou zero a partir de um valor não nulo (underflow). Subnormais
// são devolvidos sem ERANGE. "nan(...)" devolve o NaN quieto 0x7FC00000
// (com o sinal pedido); o payload é ignorado.
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#define SD 120  // dígitos significativos guardados (>= 113)
#define NL 40   // limbs de 16 bits: 640 bits (o pior caso usa ~580)

typedef struct {
  unsigned short l[NL];  // little-endian
  int n;                 // limbs usados; l[n - 1] != 0 (n == 0: zero)
} Big;

static void big_norm(Big *b) {
  while (b->n > 0 && b->l[b->n - 1] == 0)
    b->n--;
}

static void big_set(Big *b, unsigned v) {
  b->n = 0;
  while (v) {
    b->l[b->n++] = v & 0xFFFF;
    v >>= 16;
  }
}

// b = b * k + a (k <= 16, a < k)
static void big_mul_add(Big *b, unsigned k, unsigned a) {
  unsigned c = a;
  for (int i = 0; i < b->n; i++) {
    unsigned t = b->l[i] * k + c;
    b->l[i] = t;
    c = t >> 16;
  }
  if (c && b->n < NL)
    b->l[b->n++] = c;
}

static void big_shl(Big *b, int sh) {
  if (b->n == 0 || sh == 0)
    return;
  int w = sh >> 4, bit = sh & 15;
  int n = b->n + w + 1;
  if (n > NL)
    n = NL;  // não acontece com as entradas limitadas (ver strtof)
  for (int i = n - 1; i >= 0; i--) {
    int j = i - w;
    unsigned hi = (j >= 0 && j < b->n) ? b->l[j] : 0;
    unsigned lo = (j >= 1 && j - 1 < b->n) ? b->l[j - 1] : 0;
    b->l[i] = ((hi << bit) | (lo >> (16 - bit))) & 0xFFFF;
  }
  b->n = n;
  big_norm(b);
}

static void big_shr1(Big *b) {
  for (int i = 0; i < b->n; i++) {
    unsigned next = (i + 1 < b->n) ? b->l[i + 1] : 0;
    b->l[i] = (b->l[i] >> 1) | ((next & 1) << 15);
  }
  big_norm(b);
}

static int big_cmp(Big *a, Big *b) {
  if (a->n != b->n)
    return a->n < b->n ? -1 : 1;
  for (int i = a->n - 1; i >= 0; i--)
    if (a->l[i] != b->l[i])
      return a->l[i] < b->l[i] ? -1 : 1;
  return 0;
}

// a -= b (a >= b)
static void big_sub(Big *a, Big *b) {
  int borrow = 0;
  for (int i = 0; i < a->n; i++) {
    int t = (int)a->l[i] - (i < b->n ? (int)b->l[i] : 0) - borrow;
    borrow = t < 0;
    a->l[i] = t & 0xFFFF;
  }
  big_norm(a);
}

static int bitlen32(unsigned v) {
  int n = 0;
  while (v) {
    n++;
    v >>= 1;
  }
  return n;
}

static int big_bitlen(Big *b) {
  if (b->n == 0)
    return 0;
  return (b->n - 1) * 16 + bitlen32(b->l[b->n - 1]);
}

// Bits do binary32 mais próximo de num / den * 2^p2 (num > 0). Destrói
// num e den.
static unsigned big_to_float(Big *num, Big *den, int p2) {
  // valor em (2^(t-1), 2^(t+1))
  int t = big_bitlen(num) - big_bitlen(den) + p2;
  if (t > 129)
    return 0x7F800000;  // > 2^129: inf
  if (t < -150)
    return 0;           // < 2^-150 (metade do menor subnormal): 0

  // q = floor(valor * 2^s), com 25 ou 26 bits; o bit 0 de q vale 2^-s.
  int s = 25 - t;
  int sh = s + p2;
  if (sh >= 0)
    big_shl(num, sh);
  else
    big_shl(den, -sh);

  // Divisão longa binária: q < 2^26.
  big_shl(den, 26);
  unsigned q = 0;
  for (int i = 26; i >= 0; i--) {
    if (big_cmp(num, den) >= 0) {
      big_sub(num, den);
      q |= 1u << i;
    }
    big_shr1(den);
  }
  int sticky = num->n != 0;

  // Descarta os bits além de 24 significativos, ou além do bit de 2^-149
  // (subnormal), arredondando para o mais próximo, empate para o par.
  int lsb = -s;
  int drop = bitlen32(q) - 24;
  if (lsb + drop < -149)
    drop = -149 - lsb;
  if (drop > 26)
    return 0;  // q < 2^26 <= 2^(drop - 1): menos que meio subnormal
  unsigned r = q >> drop;
  unsigned rem = q & ((1u << drop) - 1);
  unsigned half = 1u << (drop - 1);
  if (rem > half || (rem == half && (sticky || (r & 1))))
    r++;
  int e2 = lsb + drop;  // expoente do bit 0 de r
  if (r == 0x1000000) {
    r >>= 1;
    e2++;
  }
  if (e2 + 150 > 254)
    return 0x7F800000;
  // r >= 2^23: normal, expoente com viés e2 + 150; r < 2^23 só com
  // e2 == -149 (subnormal, expoente 0). As duas formas dão a mesma soma.
  return ((unsigned)(e2 + 149) << 23) + r;
}

static int is_digit(int c) { return c >= '0' && c <= '9'; }

static int hex_val(int c) {
  if (is_digit(c))
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

// Expoente opcional depois de 'e'/'p': só é consumido se tem dígitos.
static const char *parse_exp(const char *p, int *exp) {
  const char *q = p + 1;
  int neg = 0, v = 0;
  if (*q == '+' || *q == '-')
    neg = *q++ == '-';
  if (!is_digit(*q))
    return p;
  while (is_digit(*q)) {
    if (v < 100000)
      v = v * 10 + (*q - '0');
    q++;
  }
  *exp += neg ? -v : v;
  return q;
}

static unsigned range(unsigned bits) {
  if ((bits & 0x7FFFFFFF) == 0 || (bits & 0x7FFFFFFF) == 0x7F800000)
    errno = ERANGE;
  return bits;
}

// Decimal: num = os dígitos significativos (até SD), valor = num * 10^e10.
static unsigned parse_dec(const char **pp) {
  const char *p = *pp;
  Big num, den;
  int nd = 0, sticky = 0, e10 = 0, point = 0;

  big_set(&num, 0);
  for (;; p++) {
    if (*p == '.' && !point) {
      point = 1;
      continue;
    }
    if (!is_digit(*p))
      break;
    int d = *p - '0';
    if (nd == 0 && d == 0) {
      if (point)
        e10--;  // zero à esquerda depois do ponto
      continue;
    }
    if (nd < SD) {
      big_mul_add(&num, 10, d);
      nd++;
      if (point)
        e10--;
    } else {
      sticky |= d;
      if (!point)
        e10++;
    }
  }
  if (*p == 'e' || *p == 'E')
    p = parse_exp(p, &e10);
  *pp = p;

  if (nd == 0)
    return 0;
  if (sticky) {
    big_mul_add(&num, 10, 1);
    nd++;
    e10--;
  }

  // valor em [10^X, 10^(X+1)): FLT_MAX ~ 3.4e38 e meio subnormal ~ 7.0e-46
  int x = nd - 1 + e10;
  if (x > 38)
    return range(0x7F800000);
  if (x < -46)
    return range(0);

  big_set(&den, 1);
  for (int i = 0; i < e10; i++)
    big_mul_add(&num, 10, 0);
  for (int i = 0; i < -e10; i++)
    big_mul_add(&den, 10, 0);
  return range(big_to_float(&num, &den, 0));
}

// Hexadecimal: num = os dígitos (até 32: 128 bits), valor = num * 2^p2.
static unsigned parse_hex(const char **pp) {
  const char *p = *pp + 2;
  Big num, den;
  int nh = 0, sticky = 0, p2 = 0, point = 0;

  big_set(&num, 0);
  for (;; p++) {
    if (*p == '.' && !point) {
      point = 1;
      continue;
    }
    int d = hex_val(*p);
    if (d < 0)
      break;
    if (nh == 0 && d == 0) {
      if (point)
        p2 -= 4;
      continue;
    }
    if (nh < 32) {
      big_mul_add(&num, 16, d);
      nh++;
      if (point)
        p2 -= 4;
    } else {
      sticky |= d;
      if (!point)
        p2 += 4;
    }
  }
  if (*p == 'p' || *p == 'P')
    p = parse_exp(p, &p2);
  *pp = p;

  if (nh == 0)
    return 0;
  if (sticky) {
    big_mul_add(&num, 2, 1);
    p2--;
  }
  big_set(&den, 1);
  return range(big_to_float(&num, &den, p2));
}

// `p` começa com `word` (minúsculo), sem distinção de maiúsculas?
static const char *match_word(const char *p, const char *word) {
  for (; *word; p++, word++)
    if (tolower((unsigned char)*p) != *word)
      return 0;
  return p;
}

float strtof(const char *restrict s, char **restrict end) {
  const char *p = s, *q;
  unsigned bits;
  int neg = 0;

  while (isspace((unsigned char)*p))
    p++;
  if (*p == '+' || *p == '-')
    neg = *p++ == '-';

  if ((q = match_word(p, "inf"))) {
    const char *q2 = match_word(q, "inity");
    p = q2 ? q2 : q;
    bits = 0x7F800000;
  } else if ((q = match_word(p, "nan"))) {
    p = q;
    if (*p == '(') {
      const char *r = p + 1;
      while (isalnum((unsigned char)*r) || *r == '_')
        r++;
      if (*r == ')')
        p = r + 1;
    }
    bits = 0x7FC00000;
  } else if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X') &&
             (hex_val(p[2]) >= 0 || (p[2] == '.' && hex_val(p[3]) >= 0))) {
    bits = parse_hex(&p);
  } else if (is_digit(*p) || (*p == '.' && is_digit(p[1]))) {
    bits = parse_dec(&p);
  } else {
    if (end)
      *end = (char *)s;
    return 0;
  }

  if (end)
    *end = (char *)p;
  union {
    unsigned u;
    float f;
  } v;
  v.u = bits | ((unsigned)neg << 31);
  return v.f;
}

double strtod(const char *restrict s, char **restrict end) {
  return strtof(s, end);
}

long double strtold(const char *restrict s, char **restrict end) {
  return strtof(s, end);
}

double atof(const char *s) {
  return strtof(s, 0);
}
