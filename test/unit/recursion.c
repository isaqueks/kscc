// Recursão: fatorial, fibonacci, ackermann pequeno, recursão mútua.
#include "minirt.h"
int fact(int n) { return n <= 1 ? 1 : n * fact(n - 1); }
int fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }
int ack(int m, int n) {
  if (m == 0) return n + 1;
  if (n == 0) return ack(m - 1, 1);
  return ack(m - 1, ack(m, n - 1));
}
int is_odd(int n);
int is_even(int n) { return n == 0 ? 1 : is_odd(n - 1); }
int is_odd(int n) { return n == 0 ? 0 : is_even(n - 1); }
int main() {
  EQ(3628800, fact(10));
  EQ(6765, fib(20));
  EQ(9, ack(2, 3));
  EQ(61, ack(3, 3));
  EQ(1, is_even(100));
  EQ(1, is_odd(77));
  return 0;
}
