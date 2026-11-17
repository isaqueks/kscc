// Aritmética unsigned com wrap, comparações mistas signed/unsigned,
// conversões nas bordas, divisão/resto unsigned e shifts.
#include "kstest.h"

static unsigned u_add(unsigned a, unsigned b) { return a + b; }
static unsigned u_sub(unsigned a, unsigned b) { return a - b; }
static unsigned u_mul(unsigned a, unsigned b) { return a * b; }

int main(void) {
  unsigned max = 0xFFFFFFFFu;
  ks_put_kx("max+1", u_add(max, 1));
  ks_put_kx("0-1", u_sub(0, 1));
  ks_put_kx("max*max", u_mul(max, max));
  ks_put_kx("65536*65536", u_mul(65536u, 65536u));
  ks_put_kx("0x10001*0x10001", u_mul(0x10001u, 0x10001u));
  ks_put_kx("-1u/3", max / 3u);
  ks_put_kx("-1u%7", max % 7u);
  ks_put_kx("0x80000000/0xFFFF", 0x80000000u / 0xFFFFu);
  ks_put_kx("big/big", 0xFFFFFFF0u / 0x80000001u);

  // comparações com conversões usuais
  int neg = -1;
  unsigned one = 1;
  ks_put_kv("-1 < 1u", neg < one);
  ks_put_kv("-1 > 1u", neg > one);
  ks_put_kv("-1 == max", (unsigned)neg == max);
  ks_put_kv("(int)max", (int)max);
  ks_put_kv("long vs unsigned", -1L < 1u); // ILP32: long vira unsigned long
  unsigned short us = 65535;
  ks_put_kv("us+1", us + 1);           // promoção para int
  unsigned char uc = 200;
  ks_put_kv("uc+uc", uc + uc);
  ks_put_kv("(unsigned char)(uc+uc)", (unsigned char)(uc + uc));
  ks_put_kv("-uc", -uc);
  ks_put_kx("-us as unsigned", (unsigned)-us);

  // laço com contador unsigned descendo até 0
  unsigned sum = 0, cnt = 0;
  for (unsigned i = 10; i-- > 0;)
    sum += i, cnt++;
  ks_put_kv("sum", (int)sum);
  ks_put_kv("cnt", (int)cnt);

  // shifts
  unsigned x = 0x87654321u;
  for (int s = 0; s < 32; s += 5) {
    ks_put_hex(x << s);
    ks_put_chr(' ');
    ks_put_hex(x >> s);
    ks_put_chr(' ');
    ks_put_hex((unsigned)((int)x >> s));
    ks_put_chr('\n');
  }

  // LCG e xorshift longos: wrap acumulado
  unsigned a = 1, b = 2463534242u;
  for (int i = 0; i < 10000; i++) {
    a = a * 1664525u + 1013904223u;
    b ^= b << 13;
    b ^= b >> 17;
    b ^= b << 5;
  }
  ks_put_kx("lcg", a);
  ks_put_kx("xorshift", b);

  // conversões de unsigned para tipos menores
  unsigned v = 0x12345678u;
  ks_put_kv("(signed char)", (signed char)v);
  ks_put_kv("(unsigned char)", (unsigned char)v);
  ks_put_kv("(short)", (short)v);
  ks_put_kv("(unsigned short)", (unsigned short)v);
  ks_put_kv("(short)0x8000", (short)0x8000u);
  ks_put_kv("(signed char)0xFF", (signed char)0xFFu);
  ks_put_kv("(char)0x80", (char)0x80u);

  // overflow detectado à mão
  unsigned p = 3000000000u, q = 2000000000u;
  unsigned r = p + q;
  ks_put_kv("carry", r < p);
  ks_put_kv("borrow", q - p > q);
  unsigned m = 0xFFFFu;
  ks_put_kv("mul overflow", m != 0 && (m * 0x10002u) / m != 0x10002u);
  return 0;
}
