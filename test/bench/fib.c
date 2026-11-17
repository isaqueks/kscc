// Benchmark: fib(25) recursivo (chamadas e retorno).
#include "kstest.h"

static int fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }

int main(void) {
  ks_put_kv("fib(25)", fib(25));
  return 0;
}
