// printf: %s %c %% com largura e precisão (inclusive *), NULL, %c com 0,
// uma conversão maior que o buffer de stdout e texto sem conversões.
#include "libks_test.h"
// expect-out: |  abc|abc  |ab|x|%|
// expect-out: (null)
// expect-out: abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmn|
int main() {
  FMT("", "");
  FMT("sem conversoes", "sem conversoes");
  FMT("abc", "%s", "abc");
  FMT("", "%s", "");
  FMT("  abc", "%5s", "abc");
  FMT("abc  ", "%-5s", "abc");
  FMT("abcdef", "%3s", "abcdef");
  FMT("ab", "%.2s", "abcdef");
  FMT("", "%.0s", "abcdef");
  FMT("   ab", "%5.2s", "abcdef");
  FMT("ab   ", "%-5.2s", "abcdef");
  FMT("abc", "%.10s", "abc");
  FMT("  abc", "%*s", 5, "abc");
  FMT("abc  ", "%*s", -5, "abc");
  FMT("abcd", "%.*s", 4, "abcdefgh");
  FMT("abcdefgh", "%.*s", -1, "abcdefgh");
  FMT("(null)", "%s", (char *)0);
  FMT("(nu", "%.3s", (char *)0);

  // A precisão não lê além dela: o array não tem '\0'
  char raw[3] = {'x', 'y', 'z'};
  FMT("xy", "%.2s", raw);

  FMT("A", "%c", 'A');
  FMT("    A", "%5c", 'A');
  FMT("A    ", "%-5c", 'A');
  FMT("\xe9", "%c", 0x1e9);  // só o byte baixo
  // %c com 0 grava o byte 0 no meio do texto (e conta no total)
  char z[8];
  EQ(3, snprintf(z, sizeof(z), "a%cb", 0));
  EQ('a', z[0]);
  EQ(0, z[1]);
  EQ('b', z[2]);
  EQ(0, z[3]);

  FMT("%", "%%");
  FMT("100%", "%d%%", 100);
  FMT("a%b", "a%%b");
  FMT("[ab]", "[%s%s]", "a", "b");

  printf("|%5s|%-5s|%.2s|%c|%%|\n", "abc", "abc", "abc", 'x');
  printf("%s\n", (char *)0);

  // Uma conversão maior que o buffer de 128 bytes de stdout
  char line[301];
  for (int i = 0; i < 300; i++)
    line[i] = 'a' + i % 26;
  line[300] = 0;
  EQ(302, printf("%s|\n", line));
  return 0;
}
