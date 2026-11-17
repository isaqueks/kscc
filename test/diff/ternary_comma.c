// ?: encadeado (inclusive como lvalue via ponteiro), operador vírgula com
// efeitos colaterais, curto-circuito de && e || e ordem de avaliação com
// pontos de sequência bem definidos.
#include "kstest.h"

static int calls;
static int trace[64];
static int tick(int v) {
  trace[calls++ % 64] = v;
  return v;
}

static const char *grade(int score) {
  return score >= 90 ? "A" : score >= 80 ? "B" : score >= 70 ? "C" : score >= 60 ? "D" : "F";
}

static int sign3(int x) { return x > 0 ? 1 : x < 0 ? -1 : 0; }

static int clamp(int x, int lo, int hi) { return x < lo ? lo : x > hi ? hi : x; }

int main(void) {
  int scores[] = {100, 95, 89, 80, 79, 70, 65, 60, 59, 0, -5};
  for (int i = 0; i < 11; i++)
    ks_put_str(grade(scores[i]));
  ks_put_chr('\n');
  for (int x = -3; x <= 3; x++) {
    ks_put_int(sign3(x * 1000));
    ks_put_chr(' ');
    ks_put_int(clamp(x * 7, -10, 12));
    ks_put_chr(' ');
  }
  ks_put_chr('\n');
  // vírgula: valor é o último; efeitos em ordem
  int a = 1, b = 2, c;
  c = (a += 10, b *= a, a + b);
  ks_put_kv("a", a);
  ks_put_kv("b", b);
  ks_put_kv("c", c);
  int i, j;
  for (i = 0, j = 10; i < j; i++, j--)
    ;
  ks_put_kv("i", i);
  ks_put_kv("j", j);
  // curto-circuito
  calls = 0;
  int r1 = tick(0) && tick(1);
  int r2 = tick(2) || tick(3);
  int r3 = (tick(0) || tick(4)) && (tick(5), tick(0));
  int r4 = tick(6) ? tick(7) : tick(8);
  int r5 = !tick(0) ? (tick(9), tick(10)) : tick(11);
  ks_put_kv("r", r1 * 10000 + r2 * 1000 + r3 * 100 + (r4 == 7) * 10 + (r5 == 10));
  ks_put_kv("calls", calls);
  for (int k = 0; k < calls; k++) {
    ks_put_int(trace[k]);
    ks_put_chr(' ');
  }
  ks_put_chr('\n');
  // ?: escolhendo lvalue via ponteiro e struct
  int x = 1, y = 2;
  *(x < y ? &x : &y) = 100;
  ks_put_kv("x", x);
  ks_put_kv("y", y);
  struct P { int v; } p1 = {5}, p2 = {6};
  struct P chosen = (x > y ? p1 : p2);
  ks_put_kv("chosen", chosen.v);
  // ?: com tipos: ponteiro nulo e void*
  const char *s = x ? "yes" : 0;
  ks_put_str(s);
  ks_put_chr('\n');
  // aninhado no meio de expressões
  int m = 0;
  for (int k = 0; k < 20; k++)
    m += k % 3 == 0 ? k : k % 3 == 1 ? -k : (k & 1 ? 1000 : 0);
  ks_put_kv("m", m);
  // vírgula em argumentos e índices
  int arr[5] = {10, 20, 30, 40, 50};
  int idx = 0;
  int got = arr[(idx++, idx++, idx)];
  ks_put_kv("got", got);
  ks_put_kv("idx", idx);
  // condição com atribuição
  int n = 0, steps = 0;
  while ((n = n * 2 + 1) < 1000)
    steps++;
  ks_put_kv("n", n);
  ks_put_kv("steps", steps);
  return 0;
}
