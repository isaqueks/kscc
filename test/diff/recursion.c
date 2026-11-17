// Recursão: Hanói, Ackermann pequeno, permutações (com troca), N rainhas,
// fibonacci com memo, mutual recursion (par/ímpar), flood fill, subsets.
#include "kstest.h"

static int moves;
static unsigned hanoi_hash;

static void hanoi(int n, int from, int to, int via) {
  if (n == 0)
    return;
  hanoi(n - 1, from, via, to);
  moves++;
  hanoi_hash = hanoi_hash * 7u + (unsigned)(from * 3 + to);
  hanoi(n - 1, via, to, from);
}

static int ack(int m, int n) {
  if (m == 0) return n + 1;
  if (n == 0) return ack(m - 1, 1);
  return ack(m - 1, ack(m, n - 1));
}

static int perm_count;
static unsigned perm_hash;
static void permute(char *a, int k, int n) {
  if (k == n) {
    perm_count++;
    for (int i = 0; i < n; i++)
      perm_hash = perm_hash * 31u + (unsigned)a[i];
    if (perm_count <= 6) {
      ks_put_str(a);
      ks_put_chr(' ');
    }
    return;
  }
  for (int i = k; i < n; i++) {
    char t = a[k]; a[k] = a[i]; a[i] = t;
    permute(a, k + 1, n);
    t = a[k]; a[k] = a[i]; a[i] = t;
  }
}

static int queens(int row, int n, int cols, int d1, int d2) {
  if (row == n) return 1;
  int count = 0;
  for (int c = 0; c < n; c++) {
    int b1 = 1 << c, b2 = 1 << (row + c), b3 = 1 << (row - c + n - 1);
    if ((cols & b1) || (d1 & b2) || (d2 & b3)) continue;
    count += queens(row + 1, n, cols | b1, d1 | b2, d2 | b3);
  }
  return count;
}

static int memo[50];
static int fib(int n) {
  if (n < 2) return n;
  if (memo[n]) return memo[n];
  return memo[n] = fib(n - 1) + fib(n - 2);
}

static int is_odd(unsigned n);
static int is_even(unsigned n) { return n == 0 ? 1 : is_odd(n - 1); }
static int is_odd(unsigned n) { return n == 0 ? 0 : is_even(n - 1); }

static char grid[8][12] = {
    "##########",
    "#   #    #",
    "# # # ## #",
    "# #   #  #",
    "# ##### ##",
    "#     #  #",
    "##### ## #",
    "##########",
};

static int flood(int r, int c) {
  if (r < 0 || r >= 8 || c < 0 || c >= 10 || grid[r][c] != ' ') return 0;
  grid[r][c] = '.';
  return 1 + flood(r + 1, c) + flood(r - 1, c) + flood(r, c + 1) + flood(r, c - 1);
}

static int subset_sums(const int *a, int n, int target) {
  if (target == 0) return 1;
  if (n == 0 || target < 0) return 0;
  return subset_sums(a + 1, n - 1, target - a[0]) + subset_sums(a + 1, n - 1, target);
}

static int gcd(int a, int b) { return b ? gcd(b, a % b) : a; }

static int digits_sum(unsigned n) { return n < 10 ? (int)n : (int)(n % 10) + digits_sum(n / 10); }

int main(void) {
  for (int n = 1; n <= 10; n += 3) {
    moves = 0;
    hanoi_hash = 0;
    hanoi(n, 1, 3, 2);
    ks_put_str("hanoi ");
    ks_put_int(n);
    ks_put_str(": ");
    ks_put_int(moves);
    ks_put_chr(' ');
    ks_put_hex(hanoi_hash);
    ks_put_chr('\n');
  }
  for (int m = 0; m <= 2; m++)
    for (int n = 0; n <= 3; n++) {
      ks_put_int(ack(m, n));
      ks_put_chr(' ');
    }
  ks_put_kv("ack(3,3)", ack(3, 3));
  char s[] = "abcde";
  permute(s, 0, 5);
  ks_put_chr('\n');
  ks_put_kv("perms", perm_count);
  ks_put_kx("perm_hash", perm_hash);
  for (int n = 1; n <= 8; n++) {
    ks_put_int(queens(0, n, 0, 0, 0));
    ks_put_chr(' ');
  }
  ks_put_chr('\n');
  ks_put_kv("fib(40)", fib(40));
  ks_put_kv("fib(46)", fib(46));
  ks_put_kv("is_even(1001)", is_even(1001));
  ks_put_kv("is_odd(777)", is_odd(777));
  ks_put_kv("flood", flood(1, 1));
  for (int r = 0; r < 8; r++) {
    ks_put_str(grid[r]);
    ks_put_chr('\n');
  }
  int set[] = {3, 34, 4, 12, 5, 2, 7, 1};
  ks_put_kv("subsets(9)", subset_sums(set, 8, 9));
  ks_put_kv("subsets(20)", subset_sums(set, 8, 20));
  ks_put_kv("gcd", gcd(1071 * 13, 462 * 13));
  ks_put_kv("digits", digits_sum(4294967295u));
  return 0;
}
