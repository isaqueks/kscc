// test/embedded/selftest.c — 50 verificações de aritmética, structs,
// ponteiros e varargs; imprime "selftest ok" se todas passarem. Pensado
// para a RAM do guest no ATmega328P (1 KB para .data + .bss + pilha): sem
// arrays grandes, frames pequenos.
#include "kstest.h"
#include <stdarg.h>
#include <stddef.h>

static int checks, fails;

static void check(int ok, int line) {
  checks++;
  if (!ok) {
    fails++;
    ks_put_str("FALHOU linha ");
    ks_put_int(line);
    ks_put_chr('\n');
  }
}
#define CHECK(c) check((c) ? 1 : 0, __LINE__)

// Valores passados por função para que nenhum compilador dobre as contas.
static int id(int x) { return x; }
static unsigned uid(unsigned x) { return x; }

struct Pt { short x, y; };
struct Box { struct Pt a, b; char tag; };
union Word { unsigned u; unsigned char b[4]; };
struct Bits { unsigned lo : 4, mid : 7; int s : 5; };

static struct Pt pt_add(struct Pt p, struct Pt q) {
  struct Pt r = {(short)(p.x + q.x), (short)(p.y + q.y)};
  return r;
}

static int box_area(struct Box b) { return (b.b.x - b.a.x) * (b.b.y - b.a.y); }

static int sum_va(int n, ...) {
  va_list ap;
  va_start(ap, n);
  int s = 0;
  for (int i = 0; i < n; i++)
    s += va_arg(ap, int);
  va_end(ap);
  return s;
}

static int max_va(int n, ...) {
  va_list ap;
  va_start(ap, n);
  int m = va_arg(ap, int);
  while (--n > 0) {
    int v = va_arg(ap, int);
    if (v > m)
      m = v;
  }
  va_end(ap);
  return m;
}

static int fact(int n) { return n <= 1 ? 1 : n * fact(n - 1); }
static int twice(int (*f)(int), int x) { return f(f(x)); }
static int inc(int x) { return x + 1; }
static void swap(int *a, int *b) {
  int t = *a;
  *a = *b;
  *b = t;
}

static const char msg[] = "KSVM";
static int table[5] = {3, 1, 4, 1, 5};

int main(void) {
  // aritmética com sinal e sem sinal (12)
  CHECK(id(7) + id(-10) == -3);
  CHECK(id(-7) / id(2) == -3);
  CHECK(id(-7) % id(2) == -1);
  CHECK(id(1000) * id(-1000) == -1000000);
  CHECK(uid(0) - 1u == 0xFFFFFFFFu);
  CHECK(uid(0xFFFFFFFFu) / 3u == 0x55555555u);
  CHECK((id(-1) >> 4) == -1);
  CHECK((uid(0x80000000u) >> 31) == 1u);
  CHECK((id(1) << 30) == 0x40000000);
  CHECK(uid(65535u) * uid(65537u) == 0xFFFFFFFFu);
  CHECK(id(-1) < id(0) && uid(0xFFFFFFFFu) > uid(0));
  CHECK((unsigned)id(-1) > 0u);
  // conversões e promoções (6)
  CHECK((signed char)id(200) == -56);
  CHECK((unsigned char)id(-1) == 255);
  CHECK((short)id(70000) == 4464);
  CHECK((unsigned short)id(-2) == 65534);
  CHECK(sizeof(long) == 4 && sizeof(void *) == 4 && sizeof(short) == 2);
  CHECK((char)id(0x80) < 0);
  // structs e unions (10)
  struct Pt p = {3, 4}, q = {10, -20};
  struct Pt r = pt_add(p, q);
  CHECK(r.x == 13 && r.y == -16);
  struct Box b = {{1, 2}, {5, 8}, 'b'};
  CHECK(box_area(b) == 24);
  struct Box c = b;
  c.a.x = 0;
  CHECK(b.a.x == 1 && box_area(c) == 30);
  CHECK(sizeof(struct Pt) == 4 && sizeof(struct Box) == 10);
  CHECK(offsetof(struct Box, b) == 4 && offsetof(struct Box, tag) == 8);
  union Word w;
  w.u = uid(0x11223344u);
  CHECK(w.b[0] == 0x44 && w.b[3] == 0x11);
  w.b[1] = 0xAA;
  CHECK(w.u == 0x1122AA44u);
  struct Bits bits = {15, 100, -3};
  CHECK(bits.lo == 15 && bits.mid == 100 && bits.s == -3);
  bits.lo++;
  CHECK(bits.lo == 0);
  CHECK(sizeof(struct Bits) == 4);
  // ponteiros e arrays (10)
  int x = id(1), y = id(2);
  swap(&x, &y);
  CHECK(x == 2 && y == 1);
  int *pt = table;
  CHECK(*(pt + 2) == 4 && pt[4] == 5);
  CHECK(&table[4] - &table[1] == 3);
  pt += 3;
  *pt = 9;
  CHECK(table[3] == 9);
  const char *s = msg;
  int len = 0;
  while (*s++)
    len++;
  CHECK(len == 4 && sizeof msg == 5);
  CHECK(msg[1] == 'S');
  int *pp[2] = {&x, &y};
  **pp += 40;
  CHECK(x == 42);
  int (*fp)(int) = inc;
  CHECK(twice(fp, 5) == 7);
  CHECK(fact(id(7)) == 5040);
  char buf[8];
  for (int i = 0; i < 8; i++)
    buf[i] = (char)('a' + i);
  CHECK(buf[7] == 'h' && *(buf + 3) == 'd');
  // varargs (4)
  CHECK(sum_va(0) == 0);
  CHECK(sum_va(4, 1, 2, 3, 4) == 10);
  CHECK(sum_va(6, -1, -2, -3, 100, 200, 300) == 594);
  CHECK(max_va(5, 3, 99, -4, 98, 0) == 99);
  // controle de fluxo (8)
  int acc = 0;
  for (int i = 0; i < 10; i++) {
    if (i == 3)
      continue;
    if (i == 8)
      break;
    acc += i;
  }
  CHECK(acc == 25);
  int k = 0;
  do
    k += 3;
  while (k < 10);
  CHECK(k == 12);
  switch (id(5)) {
  case 4: acc = -1; break;
  case 5: acc = 55; // fallthrough
  case 6: acc++; break;
  default: acc = 0;
  }
  CHECK(acc == 56);
  CHECK((id(0) ? 1 : id(2) ? 2 : 3) == 2);
  CHECK((id(1), id(2), id(3)) == 3);
  int calls = 0;
  if (id(0) && (calls = 1))
    calls = 2;
  CHECK(calls == 0);
  CHECK((id(0) || id(4)) == 1);
  unsigned n = 0;
  for (unsigned v = uid(0xF0F0u); v; v &= v - 1)
    n++;
  CHECK(n == 8);

  if (checks == 50 && fails == 0) {
    ks_put_str("selftest ok\n");
    return 0;
  }
  ks_put_str("selftest FALHOU: ");
  ks_put_int(fails);
  ks_put_str(" de ");
  ks_put_int(checks);
  ks_put_chr('\n');
  return 1;
}
