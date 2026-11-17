// Strings com escapes: simples, octal, hexadecimal, '\0' no meio,
// concatenação de literais, e saída pelo runtime.
// expect-out: tab:	fim
// expect-out: aspas:"x" barra:\ apostrofo:'
// expect-out: oct:ABC hex:abc
#include "minirt.h"
int len(char *s, int n) {
  int i = 0;
  while (i < n && s[i])
    i++;
  return i;
}

int main() {
  ks_put_str("tab:\tfim\n");
  ks_put_str("aspas:\"x\" barra:\\ apostrofo:\'\n");
  ks_put_str("oct:\101\102\103 hex:\x61\x62\x63\n");

  char s[] = "a\0b";
  EQ(4, sizeof(s));
  EQ(1, len(s, 4));
  EQ('b', s[2]);

  EQ(7, "\a\b\f\n\r\t\v"[0]);
  EQ(11, "\a\b\f\n\r\t\v"[6]);
  EQ(-1, "\377"[0]);
  EQ(255, (unsigned char)"\xff"[0]);
  EQ(27, "\e"[0]);
  EQ(9, sizeof("abc" "def" "\x41\x42"));
  EQ('B', ("abc" "def" "\x41\x42")[7]);
  EQ(0, "\0"[0]);
  EQ(2, sizeof("\0"));
  EQ('?', "\?"[0]);
  return 0;
}
