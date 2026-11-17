// Inteiros grandes em arrays (base 10000 em unsigned short e base 2^16 em
// unsigned): fatorial de 60, potências de 2, Fibonacci 300 e multiplicação
// escolar, impressos em decimal.
#include "kstest.h"

#define DIG 64 // dígitos base 10000 → até 256 dígitos decimais

typedef struct {
  unsigned short d[DIG]; // little-endian: d[0] menos significativo
  int n;
} Big;

static void big_set(Big *b, unsigned v) {
  b->n = 0;
  do {
    b->d[b->n++] = (unsigned short)(v % 10000);
    v /= 10000;
  } while (v);
}

static void big_mul_small(Big *b, unsigned m) {
  unsigned carry = 0;
  for (int i = 0; i < b->n; i++) {
    unsigned t = b->d[i] * m + carry;
    b->d[i] = (unsigned short)(t % 10000);
    carry = t / 10000;
  }
  while (carry) {
    b->d[b->n++] = (unsigned short)(carry % 10000);
    carry /= 10000;
  }
}

static Big big_add(Big a, Big b) {
  Big r;
  unsigned carry = 0;
  int n = a.n > b.n ? a.n : b.n;
  for (int i = 0; i < n; i++) {
    unsigned t = carry + (i < a.n ? a.d[i] : 0) + (i < b.n ? b.d[i] : 0);
    r.d[i] = (unsigned short)(t % 10000);
    carry = t / 10000;
  }
  r.n = n;
  if (carry) r.d[r.n++] = (unsigned short)carry;
  return r;
}

static Big big_mul(const Big *a, const Big *b) {
  unsigned acc[DIG * 2] = {0};
  for (int i = 0; i < a->n; i++)
    for (int j = 0; j < b->n; j++) {
      acc[i + j] += (unsigned)a->d[i] * b->d[j];
      // normaliza já para não estourar
      int k = i + j;
      while (acc[k] >= 10000) {
        acc[k + 1] += acc[k] / 10000;
        acc[k] %= 10000;
        k++;
      }
    }
  Big r;
  r.n = a->n + b->n;
  for (int i = 0; i < r.n; i++)
    r.d[i] = (unsigned short)acc[i];
  while (r.n > 1 && r.d[r.n - 1] == 0) r.n--;
  return r;
}

static void big_print(const char *label, const Big *b) {
  ks_put_str(label);
  ks_put_str(" = ");
  ks_put_uint(b->d[b->n - 1]);
  for (int i = b->n - 2; i >= 0; i--) {
    unsigned v = b->d[i];
    ks_put_chr('0' + (int)(v / 1000));
    ks_put_chr('0' + (int)(v / 100 % 10));
    ks_put_chr('0' + (int)(v / 10 % 10));
    ks_put_chr('0' + (int)(v % 10));
  }
  ks_put_chr('\n');
}

static int digit_sum(const Big *b) {
  int s = 0;
  for (int i = 0; i < b->n; i++) {
    unsigned v = b->d[i];
    s += (int)(v % 10 + v / 10 % 10 + v / 100 % 10 + v / 1000);
  }
  return s;
}

int main(void) {
  Big f;
  big_set(&f, 1);
  for (unsigned i = 2; i <= 60; i++)
    big_mul_small(&f, i);
  big_print("60!", &f);
  ks_put_kv("digit sum", digit_sum(&f));
  Big p;
  big_set(&p, 1);
  for (int i = 0; i < 200; i++)
    big_mul_small(&p, 2);
  big_print("2^200", &p);
  Big a, b;
  big_set(&a, 0);
  big_set(&b, 1);
  for (int i = 0; i < 300; i++) {
    Big c = big_add(a, b);
    a = b;
    b = c;
  }
  big_print("fib(300)", &a);
  Big x, y;
  big_set(&x, 123456789);
  big_mul_small(&x, 98765); // limbo * m + carry cabe em 32 bits
  big_set(&y, 4294967295u);
  Big z = big_mul(&x, &y);
  big_print("x", &x);
  big_print("x*y", &z);
  Big sq = big_mul(&p, &p);
  big_print("2^400", &sq);
  return 0;
}
