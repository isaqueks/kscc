// Ponteiros de função: tabelas de despacho, callbacks com contexto, sort
// genérico com comparador, máquina de estados por ponteiros e funções que
// devolvem ponteiros de função.
#include "kstest.h"

typedef int (*binop)(int, int);

static int op_add(int a, int b) { return a + b; }
static int op_sub(int a, int b) { return a - b; }
static int op_mul(int a, int b) { return a * b; }
static int op_div(int a, int b) { return b ? a / b : 0; }
static int op_mod(int a, int b) { return b ? a % b : 0; }
static int op_max(int a, int b) { return a > b ? a : b; }

static const struct {
  char sym;
  binop fn;
} ops[] = {{'+', op_add}, {'-', op_sub}, {'*', op_mul}, {'/', op_div}, {'%', op_mod}, {'M', op_max}};

static binop find_op(char sym) {
  for (unsigned i = 0; i < sizeof ops / sizeof ops[0]; i++)
    if (ops[i].sym == sym)
      return ops[i].fn;
  return 0;
}

// Função que devolve ponteiro de função, sem typedef.
static int (*pick(int i))(int, int) { return ops[i % 6].fn; }

static int fold(const int *a, int n, int init, binop f) {
  for (int i = 0; i < n; i++)
    init = f(init, a[i]);
  return init;
}

// sort genérico (insertion) sobre bytes
static void gsort(void *base, int n, int size, int (*cmp)(const void *, const void *)) {
  char *b = base;
  char tmp[16];
  for (int i = 1; i < n; i++) {
    for (int k = 0; k < size; k++) tmp[k] = b[i * size + k];
    int j = i - 1;
    while (j >= 0 && cmp(b + j * size, tmp) > 0) {
      for (int k = 0; k < size; k++) b[(j + 1) * size + k] = b[j * size + k];
      j--;
    }
    for (int k = 0; k < size; k++) b[(j + 1) * size + k] = tmp[k];
  }
}

static int cmp_int(const void *a, const void *b) {
  int x = *(const int *)a, y = *(const int *)b;
  return (x > y) - (x < y);
}
static int cmp_int_desc(const void *a, const void *b) { return cmp_int(b, a); }

typedef struct { short key; char tag; } Rec;
static int cmp_rec(const void *a, const void *b) {
  const Rec *x = a, *y = b;
  if (x->key != y->key) return x->key - y->key;
  return x->tag - y->tag;
}

// callbacks com contexto
typedef void (*visit_fn)(void *ctx, int v);
static void each(const int *a, int n, visit_fn f, void *ctx) {
  for (int i = 0; i < n; i++) f(ctx, a[i]);
}
static void count_even(void *ctx, int v) { if (v % 2 == 0) ++*(int *)ctx; }
struct minmax { int lo, hi; };
static void track(void *ctx, int v) {
  struct minmax *m = ctx;
  if (v < m->lo) m->lo = v;
  if (v > m->hi) m->hi = v;
}

// máquina de estados: cada estado é uma função que devolve o próximo
struct sm;
typedef struct state { struct state (*fn)(struct sm *, char); } state;
struct sm { int nums, words, cur; };
static state st_space(struct sm *m, char c);
static state st_num(struct sm *m, char c);
static state st_word(struct sm *m, char c);
static state classify(struct sm *m, char c) {
  (void)m;
  if (c >= '0' && c <= '9') return (state){st_num};
  if (c >= 'a' && c <= 'z') return (state){st_word};
  return (state){st_space};
}
static state st_space(struct sm *m, char c) {
  state s = classify(m, c);
  if (s.fn == st_num) { m->nums++; m->cur = c - '0'; }
  if (s.fn == st_word) m->words++;
  return s;
}
static state st_num(struct sm *m, char c) {
  if (c >= '0' && c <= '9') { m->cur = m->cur * 10 + (c - '0'); return (state){st_num}; }
  return st_space(m, c);
}
static state st_word(struct sm *m, char c) {
  if (c >= 'a' && c <= 'z') return (state){st_word};
  return st_space(m, c);
}

int main(void) {
  int vals[] = {12, -5, 7, 30, 3, 8, -1, 0, 15};
  int n = sizeof vals / sizeof vals[0];
  const char *syms = "+-*/%M";
  for (const char *s = syms; *s; s++) {
    ks_put_chr(*s);
    ks_put_str(" fold = ");
    ks_put_int(fold(vals, n, *s == '*' ? 1 : 0, find_op(*s)));
    ks_put_str(", 100 op 7 = ");
    ks_put_int(find_op(*s)(100, 7));
    ks_put_chr('\n');
  }
  ks_put_kv("find_op(?) null", find_op('?') == 0);
  int acc = 0;
  for (int i = 0; i < 12; i++)
    acc = pick(i)(acc, i + 1);
  ks_put_kv("pick chain", acc);
  binop (*pp)(char) = find_op;
  ks_put_kv("(*pp)('*')(6,7)", (*pp)('*')(6, 7));
  gsort(vals, n, sizeof(int), cmp_int);
  for (int i = 0; i < n; i++) { ks_put_int(vals[i]); ks_put_chr(' '); }
  ks_put_chr('\n');
  gsort(vals, n, sizeof(int), cmp_int_desc);
  for (int i = 0; i < n; i++) { ks_put_int(vals[i]); ks_put_chr(' '); }
  ks_put_chr('\n');
  Rec recs[] = {{3, 'b'}, {1, 'z'}, {3, 'a'}, {-2, 'q'}, {1, 'a'}};
  gsort(recs, 5, sizeof(Rec), cmp_rec);
  for (int i = 0; i < 5; i++) { ks_put_int(recs[i].key); ks_put_chr(recs[i].tag); ks_put_chr(' '); }
  ks_put_chr('\n');
  int evens = 0;
  each(vals, n, count_even, &evens);
  struct minmax mm = {1000, -1000};
  each(vals, n, track, &mm);
  ks_put_kv("evens", evens);
  ks_put_kv("lo", mm.lo);
  ks_put_kv("hi", mm.hi);
  struct sm m = {0, 0, 0};
  state s = {st_space};
  const char *text = "abc 123 de4f 56 g 7890 ";
  for (const char *p = text; *p; p++)
    s = s.fn(&m, *p);
  ks_put_kv("nums", m.nums);
  ks_put_kv("words", m.words);
  ks_put_kv("last num", m.cur);
  return 0;
}
