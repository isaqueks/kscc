// calc.c — calculadora de expressões inteiras lidas da entrada padrão,
// uma por linha, com um parser descendente recursivo:
//
//   expr   = term (("+" | "-") term)*
//   term   = unary (("*" | "/" | "%") unary)*
//   unary  = "-" unary | primary
//   primary = número | "(" expr ")"
//
//   build/kscc examples/calc.c -o calc.ksx
//   echo '2 * (3 + 4)' | ../ksvm2/build/tools/ksvm calc.ksx
//
// stdin: 1 + 2
// stdin: 2 * (3 + 4) - 5
// stdin: -(10 - 4) * 3
// stdin: 100 / 7 % 4
// stdin:
// stdin: 7 / (2 - 2)
// stdin: 3 +
// stdin: 0x1F + 010
// expect-out: > 3
// expect-out: > 9
// expect-out: > -18
// expect-out: > 2
// expect-out: > > erro: divisao por zero na coluna 3
// expect-out: > erro: esperava um numero na coluna 5
// expect-out: > 39
// expect-out: > fim
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

static char line[128];
static const char *pos;
static const char *error;
static int error_col;

static void fail(const char *msg) {
  if (!error) {
    error = msg;
    error_col = pos - line + 1;
  }
}

static void skip_spaces(void) {
  while (isspace(*pos))
    pos++;
}

static int consume(char c) {
  skip_spaces();
  if (*pos != c)
    return 0;
  pos++;
  return 1;
}

static int expr(void);

static int primary(void) {
  if (consume('(')) {
    int v = expr();
    if (!consume(')'))
      fail("esperava ')'");
    return v;
  }
  skip_spaces();
  char *end;
  long v = strtol(pos, &end, 0);  // decimal, 0x hexadecimal, 0 octal
  if (end == pos) {
    fail("esperava um numero");
    return 0;
  }
  pos = end;
  return v;
}

static int unary(void) {
  if (consume('-'))
    return -unary();
  return primary();
}

static int term(void) {
  int v = unary();
  for (;;) {
    if (consume('*')) {
      v *= unary();
    } else if (consume('/') || consume('%')) {
      const char *op = pos - 1;
      int rhs = unary();
      if (rhs == 0) {
        pos = op;  // o erro aponta para o operador
        fail("divisao por zero");
        return 0;
      }
      v = *op == '/' ? v / rhs : v % rhs;
    } else {
      return v;
    }
  }
}

static int expr(void) {
  int v = term();
  for (;;) {
    if (consume('+'))
      v += term();
    else if (consume('-'))
      v -= term();
    else
      return v;
  }
}

int main(void) {
  for (;;) {
    printf("> ");
    if (!fgets(line, sizeof(line), stdin))
      break;

    pos = line;
    error = NULL;
    skip_spaces();
    if (!*pos)
      continue;  // linha vazia

    int v = expr();
    skip_spaces();
    if (*pos && !error)
      fail("lixo depois da expressao");

    if (error)
      printf("erro: %s na coluna %d\n", error, error_col);
    else
      printf("%d\n", v);
  }
  printf("fim\n");
  return 0;
}
