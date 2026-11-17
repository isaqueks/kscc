// Tabela hash com encadeamento (entradas numa arena estática), chaves
// string: contagem de palavras de um texto, busca, remoção e rehash para uma
// tabela maior.
#include "kstest.h"

typedef struct Entry {
  const char *key;
  int klen;
  int count;
  struct Entry *next;
} Entry;

#define MAXE 256
static Entry pool[MAXE];
static int npool;

typedef struct {
  Entry **buckets;
  unsigned nb;
  int size;
} Table;

static unsigned hash(const char *s, int n) {
  unsigned h = 5381;
  for (int i = 0; i < n; i++)
    h = h * 33u + (unsigned char)s[i];
  return h;
}

static int key_eq(const Entry *e, const char *s, int n) {
  if (e->klen != n)
    return 0;
  for (int i = 0; i < n; i++)
    if (e->key[i] != s[i])
      return 0;
  return 1;
}

static Entry *lookup(Table *t, const char *s, int n, int create) {
  unsigned b = hash(s, n) % t->nb;
  for (Entry *e = t->buckets[b]; e; e = e->next)
    if (key_eq(e, s, n))
      return e;
  if (!create || npool == MAXE)
    return 0;
  Entry *e = &pool[npool++];
  e->key = s;
  e->klen = n;
  e->count = 0;
  e->next = t->buckets[b];
  t->buckets[b] = e;
  t->size++;
  return e;
}

static int erase(Table *t, const char *s, int n) {
  unsigned b = hash(s, n) % t->nb;
  for (Entry **pp = &t->buckets[b]; *pp; pp = &(*pp)->next)
    if (key_eq(*pp, s, n)) {
      *pp = (*pp)->next;
      t->size--;
      return 1;
    }
  return 0;
}

static void rehash(Table *dst, Table *src) {
  for (unsigned i = 0; i < src->nb; i++) {
    Entry *e = src->buckets[i];
    while (e) {
      Entry *nx = e->next;
      unsigned b = hash(e->key, e->klen) % dst->nb;
      e->next = dst->buckets[b];
      dst->buckets[b] = e;
      dst->size++;
      e = nx;
    }
    src->buckets[i] = 0;
  }
  src->size = 0;
}

static int is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

static const char text[] =
    "the quick brown fox jumps over the lazy dog and the dog sleeps while "
    "the fox runs over the hill; a quick fox is a happy fox, said the dog. "
    "KSVM runs C now and the compiler compiles the compiler test for the fox";

static void stats(const char *label, Table *t) {
  int longest = 0, used = 0;
  for (unsigned i = 0; i < t->nb; i++) {
    int len = 0;
    for (Entry *e = t->buckets[i]; e; e = e->next)
      len++;
    if (len) used++;
    if (len > longest) longest = len;
  }
  ks_put_str(label);
  ks_put_str(": size=");
  ks_put_int(t->size);
  ks_put_str(" used=");
  ks_put_int(used);
  ks_put_str(" longest=");
  ks_put_int(longest);
  ks_put_chr('\n');
}

static void show(Table *t, const char *w) {
  int n = 0;
  while (w[n]) n++;
  Entry *e = lookup(t, w, n, 0);
  ks_put_str(w);
  ks_put_str(" -> ");
  ks_put_int(e ? e->count : -1);
  ks_put_chr('\n');
}

int main(void) {
  Entry *b7[7] = {0};
  Entry *b61[61] = {0};
  Table small = {b7, 7, 0};
  Table big = {b61, 61, 0};
  int words = 0;
  for (const char *p = text; *p;) {
    while (*p && !is_alpha(*p)) p++;
    const char *s = p;
    while (is_alpha(*p)) p++;
    if (p > s) {
      lookup(&small, s, (int)(p - s), 1)->count++;
      words++;
    }
  }
  ks_put_kv("words", words);
  stats("small", &small);
  show(&small, "the");
  show(&small, "fox");
  show(&small, "dog");
  show(&small, "cat");
  ks_put_kv("erase(dog)", erase(&small, "dog", 3));
  ks_put_kv("erase(dog)", erase(&small, "dog", 3));
  show(&small, "dog");
  rehash(&big, &small);
  stats("small", &small);
  stats("big", &big);
  show(&big, "quick");
  show(&big, "compiler");
  show(&big, "KSVM");
  // soma de contagens ponderada pelo hash, independente da ordem
  unsigned acc = 0;
  for (unsigned i = 0; i < big.nb; i++)
    for (Entry *e = big.buckets[i]; e; e = e->next)
      acc += hash(e->key, e->klen) * (unsigned)e->count;
  ks_put_kx("acc", acc);
  return 0;
}
