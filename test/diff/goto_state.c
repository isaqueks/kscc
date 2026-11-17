// goto: laços feitos à mão, saída de laços aninhados, padrão de limpeza
// (cleanup em cascata) e uma máquina de estados com rótulos.
#include "kstest.h"

static int find_pair(const int *a, int n, int target, int *pi, int *pj) {
  for (int i = 0; i < n; i++)
    for (int j = i + 1; j < n; j++)
      if (a[i] + a[j] == target) {
        *pi = i;
        *pj = j;
        goto found;
      }
  return 0;
found:
  return 1;
}

static int resources[4];
static int log_pos;
static char log_buf[64];

static void log_c(char c) { log_buf[log_pos++] = c; }

// Adquire até `fail_at` recursos e falha; libera na ordem inversa.
static int acquire_all(int fail_at) {
  int rc = -1;
  if (fail_at == 0) goto out;
  resources[0] = 1; log_c('A');
  if (fail_at == 1) goto free0;
  resources[1] = 1; log_c('B');
  if (fail_at == 2) goto free1;
  resources[2] = 1; log_c('C');
  if (fail_at == 3) goto free2;
  rc = 0;
  log_c('!');
free2:
  resources[2] = 0; log_c('c');
free1:
  resources[1] = 0; log_c('b');
free0:
  resources[0] = 0; log_c('a');
out:
  log_c('|');
  return rc;
}

// Conta números, identificadores e operadores com uma máquina de estados.
static void lex(const char *s, int *nums, int *ids, int *ops) {
  const char *p = s;
  *nums = *ids = *ops = 0;
start:
  if (!*p) goto done;
  if (*p >= '0' && *p <= '9') { (*nums)++; goto in_num; }
  if ((*p >= 'a' && *p <= 'z') || *p == '_') { (*ids)++; goto in_id; }
  if (*p != ' ') (*ops)++;
  p++;
  goto start;
in_num:
  p++;
  if (*p >= '0' && *p <= '9') goto in_num;
  goto start;
in_id:
  p++;
  if ((*p >= 'a' && *p <= 'z') || *p == '_' || (*p >= '0' && *p <= '9')) goto in_id;
  goto start;
done:
  return;
}

static unsigned collatz_steps(unsigned n) {
  unsigned steps = 0;
again:
  if (n == 1) return steps;
  n = (n & 1) ? 3 * n + 1 : n / 2;
  steps++;
  goto again;
}

int main(void) {
  int a[] = {8, 3, 11, 7, 2, 15, 4};
  int i = -1, j = -1;
  int ok = find_pair(a, 7, 19, &i, &j);
  ks_put_kv("found", ok);
  ks_put_kv("i", i);
  ks_put_kv("j", j);
  ks_put_kv("none", find_pair(a, 7, 1000, &i, &j));
  for (int f = 0; f <= 4; f++)
    ks_put_kv("acquire", acquire_all(f));
  log_buf[log_pos] = 0;
  ks_put_str(log_buf);
  ks_put_chr('\n');
  int n, id, op;
  lex("x1 = foo_bar + 42 * (y - 7) / zz9;", &n, &id, &op);
  ks_put_kv("nums", n);
  ks_put_kv("ids", id);
  ks_put_kv("ops", op);
  unsigned best = 0, arg = 0;
  for (unsigned k = 1; k < 3000; k++) {
    unsigned s = collatz_steps(k);
    if (s > best) best = s, arg = k;
  }
  ks_put_kv("collatz arg", (int)arg);
  ks_put_kv("collatz steps", (int)best);
  // goto para trás dentro de um bloco com variável
  int total = 0, k = 0;
loop: {
    int sq = k * k;
    total += sq;
    if (++k < 10) goto loop;
  }
  ks_put_kv("squares", total);
  return 0;
}
