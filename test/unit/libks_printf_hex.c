// printf: %x %X %o %p, '#', e os modificadores hh h l z t j.
#include <stddef.h>
#include "libks_test.h"
// expect-out: ff FF 377 0xff 0377
int main() {
  FMT("0", "%x", 0);
  FMT("deadbeef", "%x", 0xdeadbeef);
  FMT("DEADBEEF", "%X", 0xdeadbeef);
  FMT("ffffffff", "%x", -1);
  FMT("17", "%o", 15);
  FMT("37777777777", "%o", -1);

  // '#': 0x/0X só para valor diferente de zero; octal começa com 0
  FMT("0x1f", "%#x", 31);
  FMT("0X1F", "%#X", 31);
  FMT("0", "%#x", 0);
  FMT("017", "%#o", 15);
  FMT("0", "%#o", 0);
  FMT("0017", "%#.4o", 15);  // a precisão já dá o 0 inicial
  FMT("0x001f", "%#.4x", 31);
  FMT("0x0001f", "%#07x", 31);  // zeros depois do prefixo
  FMT("   0x1f", "%#7x", 31);
  FMT("0x1f   ", "%-#7x", 31);
  FMT("000ff", "%05x", 255);

  // %p: 0x + hexadecimal mínimo
  FMT("0x0", "%p", (void *)0);
  FMT("0x1234", "%p", (void *)0x1234);
  FMT("  0xab", "%6p", (void *)0xab);

  // Modificadores de tamanho
  FMT("-1", "%hhd", 255);
  FMT("255", "%hhu", -1);
  FMT("ff", "%hhx", 0x12ff);
  FMT("-1", "%hd", 65535);
  FMT("65535", "%hu", -1);
  FMT("1234", "%hx", 0xabcd1234);
  FMT("-5", "%ld", -5L);
  FMT("4294967295", "%lu", 4294967295UL);
  FMT("abc", "%lx", 0xabcUL);
  FMT("12", "%zu", sizeof(int[3]));
  FMT("-3", "%td", (ptrdiff_t)-3);
  FMT("9", "%jd", 9);
  FMT("-128 127", "%hhd %hhd", 128, 127);

  int n = printf("%x %X %o %#x %#o\n", 255, 255, 255, 255, 255);
  EQ(20, n);
  return 0;
}
