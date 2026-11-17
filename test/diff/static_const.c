// static locais (contadores, cache, ponteiro para static), tabelas const
// globais e locais, globais static inicializadas com endereços e strings.
#include "kstest.h"

static const unsigned short crc16_seed[] = {0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5};
static const char *const weekdays[] = {"dom", "seg", "ter", "qua", "qui", "sex", "sab"};
static int global_counter = 100;
static int *const counter_ptr = &global_counter;
static const struct {
  const char *name;
  int value;
  const int *ref;
} entries[] = {{"um", 1, &global_counter}, {"dois", 2, 0}, {"tres", 3, &global_counter}};

static int next_id(void) {
  static int id = 1000;
  return id++;
}

static int *counter_addr(void) {
  static int c;
  c += 5;
  return &c;
}

static unsigned fib_cached(unsigned n) {
  static unsigned cache[48];
  static int hits;
  if (n == 47) return (unsigned)hits;
  if (n < 2) return n;
  if (cache[n]) { hits++; return cache[n]; }
  return cache[n] = fib_cached(n - 1) + fib_cached(n - 2);
}

static const char *day_name(int d) {
  static const char unknown[] = "???";
  return d >= 0 && d < 7 ? weekdays[d] : unknown;
}

static int sum_table(void) {
  static const int primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
  const int n = sizeof primes / sizeof primes[0];
  int s = 0;
  for (int i = 0; i < n; i++)
    s += primes[i] * (i + 1);
  return s;
}

int main(void) {
  for (int i = 0; i < 3; i++)
    ks_put_kv("id", next_id());
  int *p = counter_addr();
  counter_addr();
  counter_addr();
  ks_put_kv("static via ptr", *p);
  *p = 1;
  ks_put_kv("after write", *counter_addr());
  ks_put_kv("fib(40)", (int)fib_cached(40));
  ks_put_kv("hits", (int)fib_cached(47));
  for (int d = -1; d <= 7; d += 2) {
    ks_put_str(day_name(d));
    ks_put_chr(' ');
  }
  ks_put_chr('\n');
  ks_put_kv("primes", sum_table());
  unsigned x = 0;
  for (unsigned i = 0; i < sizeof crc16_seed / sizeof crc16_seed[0]; i++)
    x = (x << 3) ^ crc16_seed[i];
  ks_put_kx("seeds", x);
  *counter_ptr += 23;
  for (int i = 0; i < 3; i++) {
    ks_put_str(entries[i].name);
    ks_put_str(": ");
    ks_put_int(entries[i].value * (entries[i].ref ? *entries[i].ref : -1));
    ks_put_chr('\n');
  }
  const int local_const = global_counter * 2;
  ks_put_kv("local_const", local_const);
  static const char msg[] = "const static local\n";
  ks_put_str(msg);
  ks_put_kv("sizeof msg", (int)sizeof msg);
  ks_put_kv("weekdays[3][1]", weekdays[3][1]);
  return 0;
}
