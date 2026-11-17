// Parser de expressões recursivo-descendente com avaliação: precedência de
// || && | ^ & == != < <= > >= << >> + - * / % e unários - ! ~, parênteses,
// variáveis de uma letra e atribuição. Erros são reportados, não abortam.
#include "kstest.h"

typedef struct {
  const char *p;
  int err;
  int vars[26];
} Parser;

static int expr(Parser *ps);

static void skip_ws(Parser *ps) {
  while (*ps->p == ' ' || *ps->p == '\t')
    ps->p++;
}

static int accept(Parser *ps, const char *op) {
  skip_ws(ps);
  int n = 0;
  while (op[n]) {
    if (ps->p[n] != op[n])
      return 0;
    n++;
  }
  // não confundir "<" com "<<" nem "&" com "&&"
  if (n == 1 && (op[0] == '<' || op[0] == '>' || op[0] == '&' || op[0] == '|') &&
      (ps->p[1] == op[0] || ps->p[1] == '='))
    return 0;
  if (n == 1 && (op[0] == '=' || op[0] == '!') && ps->p[1] == '=')
    return 0;
  ps->p += n;
  return 1;
}

static int primary(Parser *ps) {
  skip_ws(ps);
  char c = *ps->p;
  if (c >= '0' && c <= '9') {
    unsigned v = 0;
    while (*ps->p >= '0' && *ps->p <= '9')
      v = v * 10u + (unsigned)(*ps->p++ - '0');
    return (int)v;
  }
  if (c >= 'a' && c <= 'z') {
    ps->p++;
    int idx = c - 'a';
    skip_ws(ps);
    if (ps->p[0] == '=' && ps->p[1] != '=') {
      ps->p++;
      int v = expr(ps);
      ps->vars[idx] = v;
      return v;
    }
    return ps->vars[idx];
  }
  if (accept(ps, "(")) {
    int v = expr(ps);
    if (!accept(ps, ")"))
      ps->err = 1;
    return v;
  }
  ps->err = 2;
  return 0;
}

static int unary(Parser *ps) {
  if (accept(ps, "-")) return (int)(0u - (unsigned)unary(ps));
  if (accept(ps, "!")) return !unary(ps);
  if (accept(ps, "~")) return ~unary(ps);
  if (accept(ps, "+")) return unary(ps);
  return primary(ps);
}

// Aritmética em unsigned para não ter overflow com sinal (wrap definido).
static int mul(Parser *ps) {
  int v = unary(ps);
  for (;;) {
    if (accept(ps, "*")) v = (int)((unsigned)v * (unsigned)unary(ps));
    else if (accept(ps, "/")) {
      int r = unary(ps);
      if (r == 0 || (r == -1 && v == (int)0x80000000u)) { ps->err = 3; v = 0; }
      else v /= r;
    } else if (accept(ps, "%")) {
      int r = unary(ps);
      if (r == 0 || r == -1) { ps->err = 3; v = 0; }
      else v %= r;
    } else return v;
  }
}

static int add(Parser *ps) {
  int v = mul(ps);
  for (;;) {
    if (accept(ps, "+")) v = (int)((unsigned)v + (unsigned)mul(ps));
    else if (accept(ps, "-")) v = (int)((unsigned)v - (unsigned)mul(ps));
    else return v;
  }
}

static int shift(Parser *ps) {
  int v = add(ps);
  for (;;) {
    if (accept(ps, "<<")) v = (int)((unsigned)v << (add(ps) & 31));
    else if (accept(ps, ">>")) v = v >> (add(ps) & 31);
    else return v;
  }
}

static int rel(Parser *ps) {
  int v = shift(ps);
  for (;;) {
    if (accept(ps, "<=")) v = v <= shift(ps);
    else if (accept(ps, ">=")) v = v >= shift(ps);
    else if (accept(ps, "<")) v = v < shift(ps);
    else if (accept(ps, ">")) v = v > shift(ps);
    else return v;
  }
}

static int eq(Parser *ps) {
  int v = rel(ps);
  for (;;) {
    if (accept(ps, "==")) v = v == rel(ps);
    else if (accept(ps, "!=")) v = v != rel(ps);
    else return v;
  }
}

static int band(Parser *ps) {
  int v = eq(ps);
  while (accept(ps, "&")) v &= eq(ps);
  return v;
}

static int bxor(Parser *ps) {
  int v = band(ps);
  while (accept(ps, "^")) v ^= band(ps);
  return v;
}

static int bor(Parser *ps) {
  int v = bxor(ps);
  while (accept(ps, "|")) v |= bxor(ps);
  return v;
}

static int land(Parser *ps) {
  int v = bor(ps);
  while (accept(ps, "&&")) {
    int r = bor(ps); // avalia sempre (efeitos de atribuição), como uma calculadora
    v = v && r;
  }
  return v;
}

static int expr(Parser *ps) {
  int v = land(ps);
  while (accept(ps, "||")) {
    int r = land(ps);
    v = v || r;
  }
  return v;
}

static const char *tests[] = {
    "1 + 2 * 3",
    "(1 + 2) * 3",
    "100 / 7 % 4",
    "-(-5) - -5",
    "2147483647 + 1",
    "1 << 31 >> 31",
    "~0 & 255 | 1 << 8",
    "3 < 4 == 1",
    "!0 + !5 + ~~7",
    "a = 6 * 7",
    "b = a - 2, a",
    "a * b - (a + b)",
    "c = a ^ b ^ a",
    "10 / 0",
    "(1 + 2",
    "12 >= 12 && 3 <= 2 || 9 != 8",
    "x = 1, x = x << 4 | x",
    "1000000 * 1000000",
    "-7 / 2 + -7 % 2 * 100",
    "z = 0 - 2147483647 - 1, z / -1",
    "((((((((7))))))))",
    "5 > 3 > 1",
    "~(~0 << 4) ^ 5",
};

int main(void) {
  Parser ps = {0};
  for (unsigned i = 0; i < sizeof tests / sizeof tests[0]; i++) {
    ps.p = tests[i];
    ps.err = 0;
    int v = expr(&ps);
    while (accept(&ps, ",")) // operador vírgula de topo
      v = expr(&ps);
    skip_ws(&ps);
    if (*ps.p && !ps.err)
      ps.err = 4;
    ks_put_str(tests[i]);
    ks_put_str(" => ");
    if (ps.err) {
      ks_put_str("erro ");
      ks_put_int(ps.err);
    } else {
      ks_put_int(v);
    }
    ks_put_chr('\n');
  }
  ks_put_kv("a", ps.vars[0]);
  ks_put_kv("b", ps.vars[1]);
  ks_put_kv("c", ps.vars[2]);
  ks_put_kv("x", ps.vars['x' - 'a']);
  return 0;
}
