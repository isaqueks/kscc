// Benchmark: crivo de Eratóstenes até 10000 (laços e acesso a bytes).
#include "kstest.h"

#define N 10000
static char composite[N + 1];

int main(void) {
  int count = 0;
  for (int i = 2; i <= N; i++) {
    if (composite[i])
      continue;
    count++;
    for (int j = i * i; j <= N; j += i)
      composite[j] = 1;
  }
  ks_put_kv("primes <= 10000", count);
  return 0;
}
