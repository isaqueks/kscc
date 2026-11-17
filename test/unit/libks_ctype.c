// <ctype.h>: cada classe conferida contra a definição por faixas para os
// 128 códigos ASCII, e fora deles (EOF, bytes > 127) nada pertence.
#include <ctype.h>
#include <stdio.h>
#include "minirt.h"

static int in(int c, int lo, int hi) { return c >= lo && c <= hi; }

int main() {
  for (int c = 0; c < 128; c++) {
    int digit = in(c, '0', '9');
    int upper = in(c, 'A', 'Z');
    int lower = in(c, 'a', 'z');
    int space = c == ' ' || in(c, '\t', '\r');
    int xdigit = digit || in(c, 'a', 'f') || in(c, 'A', 'F');
    int print = in(c, 32, 126);

    EQ(digit, !!isdigit(c));
    EQ(upper, !!isupper(c));
    EQ(lower, !!islower(c));
    EQ(upper || lower, !!isalpha(c));
    EQ(upper || lower || digit, !!isalnum(c));
    EQ(space, !!isspace(c));
    EQ(xdigit, !!isxdigit(c));
    EQ(print, !!isprint(c));
    EQ(lower ? c - 32 : c, toupper(c));
    EQ(upper ? c + 32 : c, tolower(c));
  }

  int outside[] = {EOF, 128, 200, 255, -128, 1000};
  for (int i = 0; i < 6; i++) {
    int c = outside[i];
    EQ(0, isdigit(c) || isalpha(c) || isalnum(c) || isspace(c) ||
              isupper(c) || islower(c) || isxdigit(c) || isprint(c));
    EQ(c, toupper(c));
    EQ(c, tolower(c));
  }

  // char com sinal: '\xe9' é negativo e não é letra
  char e = '\xe9';
  EQ(0, isalpha(e));
  return 0;
}
