// printf: %s %c %% com largura e precisão (inclusive *), NULL e texto
// sem conversões.
#include "libks_test.h"
// expect-out: |  abc|abc  |ab|x|%|
// expect-out: (null)
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
  FMT("%", "%%");
  FMT("100%", "%d%%", 100);
  FMT("a%b", "a%%b");
  FMT("[ab]", "[%s%s]", "a", "b");

  printf("|%5s|%-5s|%.2s|%c|%%|\n", "abc", "abc", "abc", 'x');
  printf("%s\n", (char *)0);
  return 0;
}
